#include "NOWAPrecompiled.h"
#include "FollowCamera2D.h"
#include "gameobject/GameObject.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"

namespace NOWA
{

    FollowCamera2D::FollowCamera2D(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue) :
        BaseCamera(id, 0, 0, smoothValue),
        sceneNode(sceneNode),
        sceneManager(nullptr),
        lastMoveValue(Ogre::Vector3::ZERO),
        firstTimeValueSet(true),
        firstTimeMoveValueSet(true),
        offset(offsetPosition),
        borderOffset(Ogre::Vector3(50.0f, 0.0f, 0.0f)),
        showGameObject(false),
        raySceneQuery(nullptr),
        hiddenSceneNode(nullptr),
        fadeValue(0.0f),
        fadingFinished(true),
        mostRightUp(Ogre::Vector3::ZERO),
        smoothValue(smoothValue),
        minimumBounds(Ogre::Vector3::ZERO),
        maximumBounds(Ogre::Vector3::ZERO),
        trackedCameraPosition(Ogre::Vector3::ZERO),
        pDebugLine(nullptr),
        edgeOrthographicClosureRegistered(false),
        edgeOrthographicActive(false),
        edgePlayerPosition(Ogre::Vector3::ZERO),
        edgeCameraPosition(Ogre::Vector3::ZERO),
        edgeOrthoWindow(Ogre::Vector2::ZERO),
        edgeCameraClampedX(false),
        edgeCameraClampedY(false),
        // Lookahead OFF by default. With it on, this camera has to behave exactly as it did
        // before the feature existed - a default that changes the feel of every existing level is
        // not a default, it is a regression with a nice name.
        lookaheadFactor(Ogre::Vector2::ZERO),
        lookaheadMaximum(Ogre::Vector2(2.0f, 1.0f)),
        lookaheadSmooth(0.6f),
        currentLookahead(Ogre::Vector2::ZERO),
        lastPlayerPosition(Ogre::Vector3::ZERO),
        firstTimeLookaheadSet(true),
        requestedZoom(1.0f),
        currentZoom(1.0f),
        appliedZoom(1.0f),
        zoomBlendTime(0.5f),
        punchAmount(0.0f),
        punchDuration(0.0f),
        punchRemaining(0.0f),
        punchCurrent(0.0f),
        playPlaneZ(0.0f),
        playPlaneZSet(false),
        baseHalfHeight(0.0f),
        baseAspectRatio(0.0f),
        shakeTrauma(0.0f),
        shakeDecay(1.6f),
        shakeFrequency(22.0f),
        shakeMaximumOffset(Ogre::Vector2(0.6f, 0.45f)),
        shakeTime(0.0f),
        shakeOffset(Ogre::Vector2::ZERO),
        hitstopRemaining(0.0f),
        edgeOrthographicEnabled(false),
        edgeOrthoBlendTime(0.35f),
        edgeOrthoBlend(0.0f),
        edgeOrthoMorph(0.0f),
        lastLoggedOrthographicState(false)
    {
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
    }

    FollowCamera2D::~FollowCamera2D()
    {
        if (true == this->edgeOrthographicClosureRegistered)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure("FollowCamera2D::edgeOrthographic");

            this->edgeOrthographicClosureRegistered = false;
        }

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

        this->sceneNode = nullptr;

        if (this->raySceneQuery)
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this]
            {
                this->sceneManager->destroyQuery(this->raySceneQuery);
            };

            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "FollowCamera2D::~FollowCamera2D");
        }
    }

    void FollowCamera2D::onSetData(void)
    {
        BaseCamera::onSetData();
        this->firstTimeMoveValueSet = true;
        this->firstTimeLookaheadSet = true;
    }

    void FollowCamera2D::onClearData(void)
    {
        BaseCamera::onClearData();
    }

    void FollowCamera2D::setOffset(const Ogre::Vector3& offset)
    {
        this->offset = offset;
        this->firstTimeMoveValueSet = true;
        this->playPlaneZSet = false;
    }

    void FollowCamera2D::handleUpdateBounds(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);

        this->setBounds(castEventData->getCalculatedBounds().first, castEventData->getCalculatedBounds().second);
    }

    void FollowCamera2D::setBorderOffset(const Ogre::Vector3& borderOffset)
    {
        this->borderOffset = borderOffset;
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::setBounds(const Ogre::Vector3& minimumBounds, const Ogre::Vector3& maximumBounds)
    {
        this->minimumBounds = minimumBounds;
        this->maximumBounds = maximumBounds;

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D] minimum bounds: " + Ogre::StringConverter::toString(this->minimumBounds) + " maximum bounds: " + Ogre::StringConverter::toString(this->maximumBounds));

        if (nullptr == this->camera)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!");

            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!\n", "NOWA");
        }

        // ── The view geometry, captured ONCE ─────────────────────────────────────────
        // Reading the aspect ratio back from the camera every frame is a trap, and it is the one
        // that sent the camera out of the world: Frustum::setOrthoWindow() OVERWRITES the aspect
        // ratio with the ortho window's width/height, and that window is derived from mostRightUp,
        // which was derived from the aspect ratio. Closing that loop made the extents diverge
        // within a handful of frames, the bounds clamp followed them out of the level, and nothing
        // healed it afterwards because nothing ever put the aspect ratio back - which is why it
        // survived switching the morph off and restarting. Captured here, used forever after.
        const Ogre::Radian halfFovY = this->camera->getFOVy() * 0.5f;

        this->baseHalfHeight = Ogre::Math::Abs(this->offset.z) * Ogre::Math::Tan(halfFovY);

        this->baseAspectRatio = this->camera->getAspectRatio();

        // A camera whose aspect ratio was already corrupted by an earlier run would otherwise
        // bake that corruption in right here.
        if (this->baseAspectRatio <= 0.0f || this->baseAspectRatio > 10.0f)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Implausible camera aspect ratio " + Ogre::StringConverter::toString(this->baseAspectRatio) + ", falling back to 16:9.");

            this->baseAspectRatio = 16.0f / 9.0f;
        }

        this->recomputeViewExtents();

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D] mostRightUp: " + Ogre::StringConverter::toString(this->mostRightUp) + " baseHalfHeight: " + Ogre::StringConverter::toString(this->baseHalfHeight) +
                                                                          " baseAspectRatio: " + Ogre::StringConverter::toString(this->baseAspectRatio));

        this->firstTimeValueSet = true;
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::recomputeViewExtents(void)
    {
        // Not set up yet: setBounds has not run, so there is nothing to derive from.
        if (this->baseHalfHeight <= 0.0f)
        {
            return;
        }

        // The zoom scales the DISTANCE to the play plane, so the half extents scale with it
        // linearly. Everything downstream - the follow preconditions, the bounds clamp, the
        // orthographic window - reads mostRightUp, which is why this is the single place the
        // extents are derived and why it runs before the clamping each frame.
        //
        // Both inputs come from the cached values, NEVER from the live camera. At zoom 1 this
        // produces bit for bit what setBounds used to compute once, so a scene with no zoom in
        // play is framed and clamped exactly as before.
        const Ogre::Real halfHeight = this->baseHalfHeight * this->appliedZoom;

        const Ogre::Real halfWidth = halfHeight * this->baseAspectRatio;

        this->mostRightUp = Ogre::Vector3(halfWidth, halfHeight, 0.0f);

        this->mostRightUp -= Ogre::Vector3(this->borderOffset.x, this->borderOffset.y, 0.0f);
    }

    void FollowCamera2D::alwaysShowGameObject(bool show, const Ogre::String& category, Ogre::SceneManager* sceneManager)
    {
        NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, show, category, sceneManager]
        {
            this->showGameObject = show;
            this->category = category;
            this->sceneManager = sceneManager;

            if (!this->showGameObject)
            {
                if (this->raySceneQuery)
                {
                    this->sceneManager->destroyQuery(this->raySceneQuery);

                    this->raySceneQuery = nullptr;
                }
            }
            else
            {
                this->raySceneQuery = this->sceneManager->createRayQuery(Ogre::Ray());
            }
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "FollowCamera2D::alwaysShowGameObject");
    }

    void FollowCamera2D::setSceneNode(Ogre::SceneNode* sceneNode)
    {
        this->sceneNode = sceneNode;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Lookahead
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void FollowCamera2D::setLookahead(const Ogre::Vector2& factor, const Ogre::Vector2& maximum, Ogre::Real smooth)
    {
        this->lookaheadFactor = factor;
        this->lookaheadMaximum = Ogre::Vector2(Ogre::Math::Abs(maximum.x), Ogre::Math::Abs(maximum.y));

        // Seconds, not a per-frame weight. Lower bound of one frame at 60 fps, so 0 means "as
        // fast as possible" instead of a division by zero.
        this->lookaheadSmooth = Ogre::Math::Clamp(smooth, 0.0f, 3.0f);
    }

    Ogre::Vector2 FollowCamera2D::getLookahead(void) const
    {
        return this->currentLookahead;
    }

    void FollowCamera2D::updateLookahead(Ogre::Real dt, const Ogre::Vector3& playerPosition)
    {
        // Velocity from the position delta rather than from the physics body. The delta is
        // exactly what the camera has to keep up with, it needs no assumption about which
        // physics API is attached, and it stays correct for a player moved by a path follower
        // or by a script - neither of which shows up in a rigid body's velocity.
        Ogre::Vector3 playerVelocity = Ogre::Vector3::ZERO;

        if (false == this->firstTimeLookaheadSet && dt > 0.0f)
        {
            playerVelocity = (playerPosition - this->lastPlayerPosition) / dt;
        }

        this->lastPlayerPosition = playerPosition;
        this->firstTimeLookaheadSet = false;

        // Nothing to do when the lead is switched off, which is the default. Leaving the filter
        // running on a zero target would still spend a few frames easing an old lead away after
        // the feature was turned off mid session.
        if (0.0f == this->lookaheadFactor.x && 0.0f == this->lookaheadFactor.y)
        {
            this->currentLookahead = Ogre::Vector2::ZERO;
            return;
        }

        Ogre::Vector2 rawLookahead(playerVelocity.x * this->lookaheadFactor.x, playerVelocity.y * this->lookaheadFactor.y);

        // The cap is what keeps a dash or a long fall from throwing the camera off the player
        // entirely - without it the lead grows with the velocity and has no upper bound at all.
        rawLookahead.x = Ogre::Math::Clamp(rawLookahead.x, -this->lookaheadMaximum.x, this->lookaheadMaximum.x);
        rawLookahead.y = Ogre::Math::Clamp(rawLookahead.y, -this->lookaheadMaximum.y, this->lookaheadMaximum.y);

        // Its own filter, slower than the position follow, and frame rate independent. A fixed
        // per-frame weight settles three times faster at 180 fps than at 60, and at ANY frame
        // rate it still arrives within about a quarter of a second - which for a lead of a metre
        // or more is a visible lurch when the player starts walking and a second one when he
        // stops. An exponential over a time constant in seconds turns the same lead into a drift.
        const Ogre::Real lookaheadAlpha = (this->lookaheadSmooth > 0.0f) ? (1.0f - std::exp(-dt / this->lookaheadSmooth)) : 1.0f;

        this->currentLookahead.x += (rawLookahead.x - this->currentLookahead.x) * lookaheadAlpha;
        this->currentLookahead.y += (rawLookahead.y - this->currentLookahead.y) * lookaheadAlpha;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Zoom, camera zones, punch zoom
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void FollowCamera2D::setZoom(Ogre::Real zoom, Ogre::Real blendTime)
    {
        this->requestedZoom = Ogre::Math::Clamp(zoom, 0.1f, 10.0f);
        this->zoomBlendTime = std::max(0.0f, blendTime);
    }

    Ogre::Real FollowCamera2D::getZoom(void) const
    {
        return this->requestedZoom;
    }

    Ogre::Real FollowCamera2D::getAppliedZoom(void) const
    {
        return this->appliedZoom;
    }

    void FollowCamera2D::setZoomBlendTime(Ogre::Real blendTime)
    {
        this->zoomBlendTime = std::max(0.0f, blendTime);
    }

    void FollowCamera2D::punchZoom(Ogre::Real amount, Ogre::Real duration)
    {
        this->punchAmount = Ogre::Math::Clamp(amount, -0.9f, 0.9f);
        this->punchDuration = std::max(0.001f, duration);
        this->punchRemaining = this->punchDuration;
    }

    void FollowCamera2D::addCameraZone(const Ogre::String& zoneId, const Ogre::Vector2& minimum, const Ogre::Vector2& maximum, Ogre::Real zoom)
    {
        CameraZone zone;
        zone.zoneId = zoneId;
        zone.minimum = Ogre::Vector2(std::min(minimum.x, maximum.x), std::min(minimum.y, maximum.y));
        zone.maximum = Ogre::Vector2(std::max(minimum.x, maximum.x), std::max(minimum.y, maximum.y));
        zone.zoom = Ogre::Math::Clamp(zoom, 0.1f, 10.0f);

        // Replace by id rather than append, so calling this again for the same room retunes it
        // instead of silently stacking a second zone that the first one then shadows forever.
        for (size_t i = 0; i < this->cameraZones.size(); ++i)
        {
            if (this->cameraZones[i].zoneId == zoneId)
            {
                this->cameraZones[i] = zone;
                return;
            }
        }

        this->cameraZones.push_back(zone);
    }

    void FollowCamera2D::removeCameraZone(const Ogre::String& zoneId)
    {
        for (size_t i = 0; i < this->cameraZones.size(); ++i)
        {
            if (this->cameraZones[i].zoneId == zoneId)
            {
                this->cameraZones.erase(this->cameraZones.begin() + i);
                return;
            }
        }
    }

    void FollowCamera2D::clearCameraZones(void)
    {
        this->cameraZones.clear();
    }

    void FollowCamera2D::updateZoom(Ogre::Real dt, const Ogre::Vector3& playerPosition)
    {
        // Zones are tested against the PLAYER, never against the camera. Testing the camera
        // would make the zoom depend on the camera's own smoothed position, which the zoom then
        // moves in turn - a feedback loop that oscillates on every zone border.
        Ogre::Real zoneZoom = 1.0f;

        for (size_t i = 0; i < this->cameraZones.size(); ++i)
        {
            const CameraZone& zone = this->cameraZones[i];

            if (playerPosition.x >= zone.minimum.x && playerPosition.x <= zone.maximum.x && playerPosition.y >= zone.minimum.y && playerPosition.y <= zone.maximum.y)
            {
                // First match wins, so overlapping zones resolve by insertion order instead of
                // by whatever the container happens to hold last.
                zoneZoom = zone.zoom;
                break;
            }
        }

        const Ogre::Real targetZoom = this->requestedZoom * zoneZoom;

        // Exponential approach, so the blend takes the same wall clock time at any frame rate.
        // A plain per-frame lerp factor would blend roughly three times faster at 180 fps than
        // at 60, which is the classic reason a camera feels different on another machine.
        const Ogre::Real zoomAlpha = (this->zoomBlendTime > 0.0f) ? (1.0f - std::exp(-dt / (this->zoomBlendTime * 0.33f))) : 1.0f;

        this->currentZoom += (targetZoom - this->currentZoom) * zoomAlpha;

        if (this->punchRemaining > 0.0f)
        {
            this->punchRemaining -= dt;

            // Squared ease out: the full amount lands on the first frame and tapers away. The
            // snap in and the smooth out are what make it read as an impact; easing BOTH ends
            // turns the same numbers into an ordinary zoom nobody notices.
            const Ogre::Real k = std::max(0.0f, this->punchRemaining / this->punchDuration);

            this->punchCurrent = this->punchAmount * k * k;
        }
        else
        {
            this->punchRemaining = 0.0f;
            this->punchCurrent = 0.0f;
        }

        this->appliedZoom = std::max(0.05f, this->currentZoom * (1.0f + this->punchCurrent));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Screen shake
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void FollowCamera2D::addShake(Ogre::Real strength)
    {
        this->shakeTrauma = Ogre::Math::Clamp(this->shakeTrauma + strength, 0.0f, 1.0f);
    }

    void FollowCamera2D::setShakeParameters(const Ogre::Vector2& maxOffset, Ogre::Real frequency, Ogre::Real decay)
    {
        this->shakeMaximumOffset = Ogre::Vector2(Ogre::Math::Abs(maxOffset.x), Ogre::Math::Abs(maxOffset.y));
        this->shakeFrequency = std::max(0.1f, frequency);
        this->shakeDecay = std::max(0.01f, decay);
    }

    Ogre::Real FollowCamera2D::getShakeTrauma(void) const
    {
        return this->shakeTrauma;
    }

    void FollowCamera2D::updateShake(Ogre::Real dt)
    {
        if (this->shakeTrauma <= 0.0f)
        {
            this->shakeTrauma = 0.0f;
            this->shakeOffset = Ogre::Vector2::ZERO;
            return;
        }

        this->shakeTrauma = std::max(0.0f, this->shakeTrauma - this->shakeDecay * dt);
        this->shakeTime += dt;

        // Trauma SQUARED. A linear falloff spends most of its life in the middle of the range
        // and reads as a long rumble; squared, a light hit stays nearly invisible and a heavy
        // one hits hard, which is what lets one accumulating value carry impact strength.
        const Ogre::Real amount = this->shakeTrauma * this->shakeTrauma;

        const Ogre::Real t = this->shakeTime * this->shakeFrequency;

        // Two incommensurable frequencies per axis, offset in phase. One sine per axis settles
        // into a visible wobble within a few frames; a random walk drifts off the real camera
        // position. This stays centred and never repeats on a noticeable period.
        this->shakeOffset.x = amount * this->shakeMaximumOffset.x * (std::sin(t) * 0.6f + std::sin(t * 2.17f + 1.3f) * 0.4f);
        this->shakeOffset.y = amount * this->shakeMaximumOffset.y * (std::sin(t * 1.43f + 0.7f) * 0.6f + std::sin(t * 2.91f + 2.1f) * 0.4f);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Hitstop
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void FollowCamera2D::startHitstop(Ogre::Real duration)
    {
        this->hitstopRemaining = std::max(this->hitstopRemaining, std::max(0.0f, duration));
    }

    bool FollowCamera2D::isHitstopActive(void) const
    {
        return this->hitstopRemaining > 0.0f;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Edge orthographic morph
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void FollowCamera2D::setEdgeOrthographicEnabled(bool enabled, Ogre::Real blendTime)
    {
        this->edgeOrthographicEnabled = enabled;
        this->edgeOrthoBlendTime = std::max(0.0f, blendTime);

        if (false == enabled)
        {
            this->edgeOrthographicActive = false;
        }
    }

    bool FollowCamera2D::isEdgeOrthographicEnabled(void) const
    {
        return this->edgeOrthographicEnabled;
    }

    Ogre::Real FollowCamera2D::getEdgeOrthographicMorph(void) const
    {
        return this->edgeOrthoMorph;
    }

    void FollowCamera2D::updateEdgeOrthographic(Ogre::Real dt, const Ogre::Vector3& playerPosition, const Ogre::Vector3& cameraPosition, bool cameraClampedX, bool cameraClampedY)
    {
        this->edgePlayerPosition = playerPosition;
        this->edgeCameraPosition = cameraPosition;
        this->edgeCameraClampedX = cameraClampedX;
        this->edgeCameraClampedY = cameraClampedY;

        const Ogre::Real cameraMinX = this->minimumBounds.x + this->mostRightUp.x;

        const Ogre::Real cameraMaxX = this->maximumBounds.x - this->mostRightUp.x;

        // The camera is smoothed, therefore it does not necessarily land
        // exactly on the mathematical clamp position.
        const Ogre::Real edgeTolerance = 1.0f;

        const bool cameraAtLeft = cameraPosition.x <= cameraMinX + edgeTolerance;

        const bool cameraAtRight = cameraPosition.x >= cameraMaxX - edgeTolerance;

        const bool playerBeyondLeft = cameraAtLeft && playerPosition.x < cameraPosition.x;

        const bool playerBeyondRight = cameraAtRight && playerPosition.x > cameraPosition.x;

        const bool shouldEnterOrtho = playerBeyondLeft || playerBeyondRight;

        // Keep the mode active for a small distance when the player
        // crosses back over the camera center. This prevents rapid
        // perspective/orthographic flickering.
        const Ogre::Real releaseDistance = 0.5f;

        bool shouldStayOrtho = false;

        if (true == this->edgeOrthographicActive)
        {
            if (cameraAtLeft)
            {
                shouldStayOrtho = playerPosition.x < cameraPosition.x + releaseDistance;
            }
            else if (cameraAtRight)
            {
                shouldStayOrtho = playerPosition.x > cameraPosition.x - releaseDistance;
            }
        }

        this->edgeOrthographicActive = this->edgeOrthographicEnabled && (shouldEnterOrtho || shouldStayOrtho);

        // ── The orthographic window ──────────────────────────────────────────────────
        // Built from the TRUE view extents at the play plane, NOT from mostRightUp.
        //
        // mostRightUp is the CLAMP MARGIN, not the view: it is deliberately the half extents
        // MINUS borderOffset, so that the camera stops short of the scene bound. Using it as the
        // window made the orthographic view narrower than the perspective one by exactly
        // borderOffset on each side - with a border offset of 1 that is the missing metre at the
        // right edge, and it also meant the two ends of the morph never showed the same thing, so
        // every blend carried a hidden zoom.
        //
        // No safety margin either. With the window matching the perspective view exactly at the
        // play plane, nothing that was visible can drop out, and a margin would be one more
        // reason for the blend to read as a zoom.
        const Ogre::Real viewHalfHeight = this->baseHalfHeight * this->appliedZoom;

        const Ogre::Real viewHalfWidth = viewHalfHeight * this->baseAspectRatio;

        this->edgeOrthoWindow = Ogre::Vector2(viewHalfWidth * 2.0f, viewHalfHeight * 2.0f);

        // ── The morph factor ─────────────────────────────────────────────────────────
        // Advanced here, on the logic thread, off the logic dt. The render closure only APPLIES
        // whatever value it finds, so the blend no longer depends on what the tracked closure
        // passes as its delta or on how often it gets ticked - which is what made the way into
        // orthographic ease over the blend time while the way back snapped in a single frame.
        const Ogre::Real morphTarget = (true == this->edgeOrthographicActive) ? 1.0f : 0.0f;

        if (this->edgeOrthoBlendTime > 0.0f)
        {
            // LINEAR in time, not exponential. An exponential approach is symmetric in the maths -
            // both directions reach the same threshold after the same number of seconds - but it
            // is not symmetric to the EYE, and that is what still read as a fast way back. An
            // exponential spends most of its speed at the START of the blend. Going towards
            // orthographic that speed falls in the 0.0 to 0.4 range, where the picture barely
            // changes, and the long slow tail lands in the part that is actually visible. Going
            // back, the very same curve spends its speed in the 1.0 to 0.6 range - which is
            // exactly where the projection changes most - so the whole visible part of the
            // transition is over in a fraction of a second and the invisible tail eats the rest
            // of the blend time.
            //
            // A linear ramp gives every part of the transition the same share of the time in both
            // directions, and it finishes in EXACTLY the blend time instead of approaching
            // forever, which also removes the need for any snap threshold at the ends.
            const Ogre::Real blendStep = dt / this->edgeOrthoBlendTime;

            if (this->edgeOrthoBlend < morphTarget)
            {
                this->edgeOrthoBlend = std::min(morphTarget, this->edgeOrthoBlend + blendStep);
            }
            else
            {
                this->edgeOrthoBlend = std::max(morphTarget, this->edgeOrthoBlend - blendStep);
            }
        }
        else
        {
            // Blend time 0 is the old hard switch, kept deliberately as an opt out.
            this->edgeOrthoBlend = morphTarget;
        }

        // Smoothstep over the linear ramp: soft start and soft end, and the curve is its own
        // mirror image, so neither direction begins or finishes with a visible step. The ease is
        // what the exponential was there for - the linear ramp underneath it is what makes the
        // two directions actually match.
        this->edgeOrthoMorph = this->edgeOrthoBlend * this->edgeOrthoBlend * (3.0f - 2.0f * this->edgeOrthoBlend);

        if (false == this->edgeOrthographicClosureRegistered)
        {
            this->edgeOrthographicClosureRegistered = true;

            // renderDt is intentionally unused: this closure integrates nothing any more.
            auto closureFunction = [this](Ogre::Real renderDt)
            {
                if (nullptr == this->camera)
                {
                    return;
                }

                // ── State change logging, throttled to transitions only ───────────────
                if (this->edgeOrthographicActive != this->lastLoggedOrthographicState)
                {
                    if (true == this->edgeOrthographicActive)
                    {
                        Ogre::String side = "UNKNOWN";

                        const Ogre::Real logCameraMinX = this->minimumBounds.x + this->mostRightUp.x;

                        const Ogre::Real logCameraMaxX = this->maximumBounds.x - this->mostRightUp.x;

                        if (this->edgeCameraPosition.x >= logCameraMaxX - 1.0f)
                        {
                            side = "RIGHT";
                        }
                        else if (this->edgeCameraPosition.x <= logCameraMinX + 1.0f)
                        {
                            side = "LEFT";
                        }

                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D][Ortho] START " + side + " player=" + Ogre::StringConverter::toString(this->edgePlayerPosition) +
                                                                                          " camera=" + Ogre::StringConverter::toString(this->edgeCameraPosition) + " clampedX=" + Ogre::StringConverter::toString(this->edgeCameraClampedX) +
                                                                                          " window=" + Ogre::StringConverter::toString(this->edgeOrthoWindow));
                    }
                    else
                    {
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL,
                            "[FollowCamera2D][Ortho] END player=" + Ogre::StringConverter::toString(this->edgePlayerPosition) + " camera=" + Ogre::StringConverter::toString(this->edgeCameraPosition));
                    }

                    this->lastLoggedOrthographicState = this->edgeOrthographicActive;
                }

                // ── Apply only ───────────────────────────────────────────────────────
                // The morph factor is advanced on the logic thread in updateEdgeOrthographic.
                // Nothing here integrates anything, so this closure is indifferent to what it is
                // handed as a delta and to how often it runs - it just draws the current value.
                if (0.0f == this->edgeOrthoMorph)
                {
                    // Fully perspective: hand the camera back to its own projection path so
                    // nothing downstream has to deal with a custom matrix in the common case.
                    this->camera->setCustomProjectionMatrix(false);
                    this->camera->setProjectionType(Ogre::PT_PERSPECTIVE);
                    return;
                }

                // Both matrices are READ BACK from the camera rather than built by hand, so the
                // near and far plane, the aspect ratio and the render system's depth convention
                // all come from exactly the same source the normal path uses. Getting any one of
                // those three wrong by hand produces a picture that looks almost right and clips
                // wrongly. Toggling the projection type only dirties a flag; the cost is two 4x4
                // recomputations per frame.
                const Ogre::Real savedAspectRatio = this->camera->getAspectRatio();

                this->camera->setCustomProjectionMatrix(false);

                this->camera->setProjectionType(Ogre::PT_PERSPECTIVE);
                const Ogre::Matrix4 perspectiveMatrix = this->camera->getProjectionMatrix();

                this->camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);

                // setOrthoWindowHeight, NOT setOrthoWindow. The two argument version OVERWRITES
                // the camera's aspect ratio with the window's width/height and nothing ever puts
                // it back, which silently corrupted the camera for the rest of the session. The
                // height alone, with the camera's own aspect ratio supplying the width, describes
                // the very same window - the window was built from that aspect ratio to begin with.
                this->camera->setOrthoWindowHeight(this->edgeOrthoWindow.y);

                const Ogre::Matrix4 orthographicMatrix = this->camera->getProjectionMatrix();

                // Back to perspective BEFORE installing the custom matrix. The type is what the
                // rest of the engine queries when it wants to know what kind of camera this is;
                // leaving it on orthographic while the matrix is a blend would have culling and
                // shadow code reason about a camera that does not exist.
                this->camera->setProjectionType(Ogre::PT_PERSPECTIVE);

                // Belt and braces: if any of the calls above still moves the aspect ratio in some
                // Ogre version, it is put back here rather than left for the next frame to inherit.
                this->camera->setAspectRatio(savedAspectRatio);

                Ogre::Matrix4 morphedMatrix;

                for (size_t row = 0; row < 4; ++row)
                {
                    for (size_t column = 0; column < 4; ++column)
                    {
                        morphedMatrix[row][column] = perspectiveMatrix[row][column] * (1.0f - this->edgeOrthoMorph) + orthographicMatrix[row][column] * this->edgeOrthoMorph;
                    }
                }

                // An element wise blend of two projection matrices is not a projectively
                // "correct" interpolation - there is no such thing between these two - but it is
                // continuous, monotonic and hits both ends exactly, which is all the eye needs.
                this->camera->setCustomProjectionMatrix(true, morphedMatrix);
            };

            NOWA::GraphicsModule::getInstance()->updateTrackedClosure("FollowCamera2D::edgeOrthographic", closureFunction, false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Main update
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void FollowCamera2D::moveCamera(Ogre::Real dt)
    {
        if (Ogre::Vector3::ZERO == this->mostRightUp)
        {
            return;
        }

        if (nullptr == this->sceneNode)
        {
            return;
        }

        if (nullptr == this->camera)
        {
            return;
        }

        if (true == this->firstTimeMoveValueSet)
        {
            this->lastMoveValue = Ogre::Vector3::ZERO;

            Ogre::Vector3 targetNodePosition = this->sceneNode->_getDerivedPositionUpdated();

            // Captured once. The camera's z is from here on always playPlaneZ + offset.z * zoom,
            // so a zoom moves it along its own view axis and a player who moves in z does not
            // drag it along - which matches what this camera did before the zoom existed.
            if (false == this->playPlaneZSet)
            {
                this->playPlaneZ = targetNodePosition.z;
                this->playPlaneZSet = true;
            }

            const Ogre::Vector3 initialPosition = targetNodePosition + this->offset;

            GraphicsModule::getInstance()->setCameraTransform(this->camera, initialPosition, this->camera->getOrientation());

            this->trackedCameraPosition = initialPosition;

            this->lastPlayerPosition = targetNodePosition;
            this->firstTimeLookaheadSet = true;
            this->currentLookahead = Ogre::Vector2::ZERO;

            this->firstTimeMoveValueSet = false;
        }

        Ogre::Vector3 playerPosition;

        if (nullptr != this->physicsBody)
        {
            playerPosition = this->physicsBody->getPosition();
        }
        else
        {
            playerPosition = this->sceneNode->getPosition();
        }

        // ── Zoom first ───────────────────────────────────────────────────────────────
        // It changes mostRightUp, and mostRightUp is what the follow preconditions and the
        // bounds clamp below are measured against. Updating it afterwards would clamp this
        // frame's position against last frame's framing and show the void outside the level
        // for exactly one frame per zoom step - which is all it takes to be visible.
        this->updateZoom(dt, playerPosition);
        this->recomputeViewExtents();

        // ── Hitstop ──────────────────────────────────────────────────────────────────
        // The follow is frozen, the shake and the punch zoom are not. That is the whole trick:
        // the world stops dead while the lens keeps moving, and the hit lands. Freezing all
        // three gives a dropped frame instead of an impact.
        if (this->hitstopRemaining > 0.0f)
        {
            this->hitstopRemaining = std::max(0.0f, this->hitstopRemaining - dt);

            this->updateShake(dt);

            Ogre::Vector3 frozenPosition = this->trackedCameraPosition;
            frozenPosition.x += this->shakeOffset.x;
            frozenPosition.y += this->shakeOffset.y;
            frozenPosition.z = this->playPlaneZ + this->offset.z * this->appliedZoom;

            GraphicsModule::getInstance()->updateCameraPosition(this->camera, frozenPosition);

            return;
        }

        this->updateLookahead(dt, playerPosition);

        const Ogre::Vector3 cameraPosition = this->trackedCameraPosition;

        // The point the camera wants to sit on: the player, plus the authored offset, plus the
        // lead. Folding the lead in here rather than adding it to the final position is what
        // makes it share the follow's smoothing and the bounds clamp - a lead added afterwards
        // would happily push the camera past the edge of the level.
        const Ogre::Real targetX = playerPosition.x + this->offset.x + this->currentLookahead.x;
        const Ogre::Real targetY = playerPosition.y + this->offset.y + this->currentLookahead.y;

        Ogre::Vector3 velocity = Ogre::Vector3::ZERO;

        if (targetX - this->mostRightUp.x > this->minimumBounds.x && targetX + this->mostRightUp.x < this->maximumBounds.x)
        {
            velocity.x = targetX - cameraPosition.x;

            if (Ogre::Math::RealEqual(velocity.x, 0.0f))
            {
                velocity.x = 0.0f;
            }
        }

        if (targetY - this->mostRightUp.y > this->minimumBounds.y && targetY + this->mostRightUp.y < this->maximumBounds.y)
        {
            velocity.y = targetY - cameraPosition.y;

            if (Ogre::Math::RealEqual(velocity.y, 0.0f))
            {
                velocity.y = 0.0f;
            }
        }

        velocity.x = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.x, this->lastMoveValue.x, this->smoothValue);

        velocity.y = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.y, this->lastMoveValue.y, this->smoothValue);

        this->lastMoveValue = velocity;

        Ogre::Vector3 finalPosition = cameraPosition + (velocity * this->moveCameraWeight);

        bool cameraClampedX = false;
        bool cameraClampedY = false;

        if (finalPosition.x + this->mostRightUp.x > this->maximumBounds.x)
        {
            finalPosition.x = this->maximumBounds.x - this->mostRightUp.x;

            cameraClampedX = true;
        }
        else if (finalPosition.x - this->mostRightUp.x < this->minimumBounds.x)
        {
            finalPosition.x = this->minimumBounds.x + this->mostRightUp.x;

            cameraClampedX = true;
        }

        if (finalPosition.y + this->mostRightUp.y > this->maximumBounds.y)
        {
            finalPosition.y = this->maximumBounds.y - this->mostRightUp.y;

            cameraClampedY = true;
        }
        else if (finalPosition.y - this->mostRightUp.y < this->minimumBounds.y)
        {
            finalPosition.y = this->minimumBounds.y + this->mostRightUp.y;

            cameraClampedY = true;
        }

        const Ogre::Real cameraMinX = this->minimumBounds.x + this->mostRightUp.x;

        const Ogre::Real cameraMaxX = this->maximumBounds.x - this->mostRightUp.x;

        const Ogre::Real cameraEdgeTolerance = 1.0f;

        if (Ogre::Math::Abs(finalPosition.x - cameraMinX) <= cameraEdgeTolerance || Ogre::Math::Abs(finalPosition.x - cameraMaxX) <= cameraEdgeTolerance)
        {
            cameraClampedX = true;
        }

        // The zoom drives the camera along its own view axis.
        finalPosition.z = this->playPlaneZ + this->offset.z * this->appliedZoom;

        // Tracked FIRST, and shake free. The tracked position is the reference the next
        // correction is computed against, so feeding the shake into it would make the follow
        // chase its own jitter and the shake would never settle.
        this->trackedCameraPosition = finalPosition;

        this->updateShake(dt);

        Ogre::Vector3 renderPosition = finalPosition;
        renderPosition.x += this->shakeOffset.x;
        renderPosition.y += this->shakeOffset.y;

        GraphicsModule::getInstance()->updateCameraPosition(this->camera, renderPosition);

        this->updateEdgeOrthographic(dt, playerPosition, finalPosition, cameraClampedX, cameraClampedY);
    }

    void FollowCamera2D::rotateCamera(Ogre::Real dt, bool forJoyStick)
    {
    }

    Ogre::Vector3 FollowCamera2D::getPosition(void)
    {
        return this->camera->getPosition();
    }

    Ogre::Quaternion FollowCamera2D::getOrientation(void)
    {
        return this->camera->getOrientation();
    }

}; // namespace end