#include "NOWAPrecompiled.h"
#include "ThirdPersonOcclusionCamera.h"
#include "OgreTimer.h"
#include "gameobject/GameObjectComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"

#include "OgreNewt_Body.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_ConvexCast.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_World.h"

namespace
{
    /**
     * @brief True if body's owning GameObject's id matches ignoreGameObjectId.
     *        Compares by GameObject ID rather than scene node name OR pointer
     *        identity - both prior approaches were fragile: a cached SceneNode
     *        pointer can go stale if the node is ever recreated after construction,
     *        and a node NAME can be changed at any time in NOWA-Design, silently
     *        breaking the exclusion. A GameObject's id is permanent for its entire
     *        lifetime and never editable from the editor, so this cannot regress the
     *        same way twice. Mirrors the established pattern already used elsewhere
     *        in this codebase (see PhysicsBuoyancyComponent's trigger callback):
     *        OgreNewt::any_cast<PhysicsComponent*>(body->getUserData()) ->
     *        getOwner() -> getId(). any_cast throws on a type mismatch (e.g. a body
     *        with no PhysicsComponent userData at all), so this is wrapped in
     *        try/catch, matching PhysicsActiveComponent::getFixedContactToDirection's
     *        own handling of the same call.
     */
    bool isPlayerBody(OgreNewt::Body* body, unsigned long ignoreGameObjectId)
    {
        if (nullptr == body || 0 == ignoreGameObjectId)
        {
            return false;
        }
        try
        {
            NOWA::PhysicsComponent* hitPhysicsComponent = OgreNewt::any_cast<NOWA::PhysicsComponent*>(body->getUserData());
            if (nullptr != hitPhysicsComponent)
            {
                NOWA::GameObjectPtr hitGameObjectPtr = hitPhysicsComponent->getOwner();
                if (nullptr != hitGameObjectPtr && hitGameObjectPtr->getId() == ignoreGameObjectId)
                {
                    return true;
                }
            }
        }
        catch (...)
        {
            // getUserData() did not hold a PhysicsComponent* - not the player, not an error.
        }
        return false;
    }

    /**
     * @brief Sphere sweep that excludes the player's own body AND Terra specifically.
     *        Sweeping against Terra crashes inside Newton Dynamics 4's own continuous
     *        collision code (ndShapeConvexPolygon::GenerateConvexCap reading past the
     *        end of m_adjacentFaceEdgeNormalIndex for certain welded terrain faces) -
     *        a bug in the physics engine, not in this calling code. Terra is checked
     *        separately via OcclusionRaycast below instead.
     *
     *        Must use the zero-arg BasicConvexcast() base constructor and call go()
     *        from inside THIS class's own constructor body, not via the base class's
     *        other constructor (which calls go() during base construction, before this
     *        class's vtable is active - userPreFilterCallback would silently dispatch
     *        to the base implementation instead of this override).
     */
    class OcclusionConvexcast : public OgreNewt::BasicConvexcast
    {
    public:
        OcclusionConvexcast(const OgreNewt::World* world, const OgreNewt::CollisionPtr& shape, const Ogre::Vector3& startpt, const Ogre::Quaternion& orientation, const Ogre::Vector3& endpt, int maxContactsCount, int threadIndex,
            OgreNewt::Body* ignoreBody, unsigned long ignoreGameObjectId, bool debugLog) :
            OgreNewt::BasicConvexcast(),
            mIgnoreGameObjectId(ignoreGameObjectId),
            bDebugLog(debugLog)
        {
            this->setIgnoreBody(ignoreBody);
            this->go(world, shape, startpt, orientation, endpt, maxContactsCount, threadIndex);
        }

        bool userPreFilterCallback(OgreNewt::Body* body) override
        {
            if (false == OgreNewt::BasicConvexcast::userPreFilterCallback(body))
            {
                return false;
            }

            // Second, independent exclusion path for the player's own body -
            // does NOT depend on ignoreBody (the body pointer passed to
            // setIgnoreBody()) being correctly wired. this->mIgnoreGameObjectId is the
            // player's GameObject id, permanent for its whole lifetime and never
            // editable from NOWA-Design - see isPlayerBody()'s comment for why this
            // replaced both an earlier pointer-based and name-based attempt, neither
            // of which held up. So this cannot silently become a no-op the way the
            // body-pointer path apparently has been - see the class comment in the
            // header for the diagnosis (resultDistance pulled in to ~half spring
            // length, hitBody consistently the player's
            // own character name, every single probe).
            if (true == isPlayerBody(body, this->mIgnoreGameObjectId))
            {
                return false;
            }

            if (true == this->bDebugLog)
            {
                Ogre::String nodeName = "<no node>";
                if (nullptr != body && nullptr != body->getOgreNode())
                {
                    nodeName = body->getOgreNode()->getName();
                }
                // Logs the ACTUAL filter result, not just that the body was
                // considered - "testing against" alone doesn't prove whether the
                // exclusion logic said yes or no, only that Newton evaluated it.
                bool wouldExcludeAsPlayer = isPlayerBody(body, this->mIgnoreGameObjectId);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OcclusionConvexcast] testing against body node='" + nodeName + "' isPlayerBody=" + Ogre::StringConverter::toString(wouldExcludeAsPlayer));
            }

            // Exclude HeightField (Terra) AND TreeCollision (e.g. a house with
            // interior) - both are genuine, already-existing OgreNewt accessors via
            // Body::getCollisionPrimitiveType(), not a guess at Newton internals or a
            // dynamic_cast against a wrapper class. Confirmed via
            // PhysicsComponent::createHeightFieldCollision(): Terra's collision is a
            // heightfield (ndShapeHeightfield), a different Newton shape class than
            // TreeCollision's static-mesh BVH (ndShapeStatic_bvh), yet both trigger the
            // identical GenerateConvexCap crash - meaning Newton routes both through
            // the same generic "convex vs polygon soup, continuous" contact code
            // internally. TreeCollision is excluded defensively here even though only
            // Terra has been confirmed to crash so far - any house using TreeCollision
            // is built on the same underlying shape category and should be expected to
            // crash the same way the moment the camera sweeps near one.
            if (nullptr != body)
            {
                OgreNewt::CollisionPrimitiveType primType = body->getCollisionPrimitiveType();
                if (OgreNewt::HeighFieldPrimitiveType == primType || OgreNewt::TreeCollisionPrimitiveType == primType)
                {
                    return false;
                }
            }

            return true;
        }

    private:
        unsigned long mIgnoreGameObjectId;
        bool bDebugLog;
    };

    /**
     * @brief Plain raycast excluding only the player's own body - used specifically to
     *        catch Terra (which OcclusionConvexcast above deliberately skips). A plain
     *        raycast does not go through Newton's continuous convex-vs-polygon-soup
     *        code, so it does not exercise the crash above. Same zero-arg-base-then-
     *        go()-in-body constructor pattern as OcclusionConvexcast, for the same
     *        virtual-dispatch-during-construction reason.
     */
    class OcclusionRaycast : public OgreNewt::BasicRaycast
    {
    public:
        OcclusionRaycast(const OgreNewt::World* world, const Ogre::Vector3& startpt, const Ogre::Vector3& endpt, bool sorted, OgreNewt::Body* ignoreBody, unsigned long ignoreGameObjectId, bool debugLog) :
            mIgnoreBody(ignoreBody),
            mIgnoreGameObjectId(ignoreGameObjectId),
            bDebugLog(debugLog)
        {
            this->go(world, startpt, endpt, sorted);
        }

        bool userPreFilterCallback(OgreNewt::Body* body) override
        {
            bool wouldExcludeAsPlayer = isPlayerBody(body, this->mIgnoreGameObjectId);

            // This log was missing before - the convex sweep's candidates were all
            // visible but the raycast's were not, which is exactly why "Lexy_0" showed
            // up as the final hitBody with no way to see where it actually came from.
            // Now logs the ACTUAL filter result (isPlayerBody=), not just that the
            // body was considered - that distinction matters: if this prints true for
            // Lexy_0 and it STILL ends up as the final hitBody, the bug is not in this
            // comparison logic at all, it is in whether the underlying Newton/OgreNewt
            // raycast actually honors userPreFilterCallback's return value.
            if (true == this->bDebugLog)
            {
                Ogre::String nodeName = "<no node>";
                if (nullptr != body && nullptr != body->getOgreNode())
                {
                    nodeName = body->getOgreNode()->getName();
                }
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OcclusionRaycast] testing against body node='" + nodeName + "' isPlayerBody=" + Ogre::StringConverter::toString(wouldExcludeAsPlayer));
            }

            if (nullptr != this->mIgnoreBody && body == this->mIgnoreBody)
            {
                return false;
            }
            // Same independent GameObject-id-based fallback as OcclusionConvexcast -
            // see isPlayerBody()'s comment for why this is needed in addition to the
            // body-pointer check.
            if (true == wouldExcludeAsPlayer)
            {
                return false;
            }
            return true;
        }

    private:
        bool bDebugLog;
        OgreNewt::Body* mIgnoreBody;
        unsigned long mIgnoreGameObjectId;
    };
}

namespace NOWA
{
    ThirdPersonOcclusionCamera::ThirdPersonOcclusionCamera(unsigned int id, Ogre::SceneNode* sceneNode, OgreNewt::World* ogreNewt, const Ogre::Vector3& defaultDirection, const Ogre::Vector3& offsetPosition, const Ogre::Vector3& lookAtOffset,
        Ogre::Real cameraSpring, Ogre::Real cameraFriction, Ogre::Real cameraSpringLength) :
        BaseCamera(id, 0.0f, 0.0f, 0.0f, defaultDirection),
        sceneNode(sceneNode),
        ogreNewt(ogreNewt),
        offsetPosition(offsetPosition),
        lookAtOffset(lookAtOffset),
        cameraSpring(cameraSpring),
        cameraFriction(cameraFriction),
        cameraSpringLength(cameraSpringLength),
        physicsBody(nullptr),
        ignoreGameObjectId(0),
        probeRadius(0.25f),
        skinMargin(0.4f),
        minDistance(0.5f),
        releaseSmoothValue(0.85f),
        pullInSmoothValue(20.0f),
        currentCollisionDistance(cameraSpringLength),
        firstOcclusionSample(true),
        internalSpringPosition(Ogre::Vector3::ZERO),
        firstSpringSample(true),
        lastProbedOrigin(Ogre::Vector3::ZERO),
        lastProbedTarget(Ogre::Vector3::ZERO),
        lastProbeResultDistance(cameraSpringLength),
        hasProbedOnce(false),
        probeCacheEpsilon(0.01f),
        bShowDebugData(false)
    {
    }

    ThirdPersonOcclusionCamera::~ThirdPersonOcclusionCamera()
    {
        this->sceneNode = nullptr;
    }

    void ThirdPersonOcclusionCamera::rebuildProbeShape(void)
    {
        if (nullptr == this->ogreNewt)
        {
            return;
        }

        // Pure analytic primitive (sphere via uniform-radius Ellipsoid) - no mesh data
        // read, so unlike ConvexHull/ConcaveHull this needs no enqueueAndWait. Mirrors
        // BasePhysicsCamera::onSetData constructing its Ellipsoid the same way, directly,
        // with no render-thread dispatch.
        OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(this->ogreNewt, Ogre::Vector3(this->probeRadius, this->probeRadius, this->probeRadius), 0);
        this->probeShape = OgreNewt::CollisionPtr(col);
    }

    void ThirdPersonOcclusionCamera::onSetData(void)
    {
        BaseCamera::onSetData();

        this->firstTimeMoveValueSet = true;
        this->firstOcclusionSample = true;
        this->firstSpringSample = true;
        this->currentCollisionDistance = this->cameraSpringLength;
        this->hasProbedOnce = false;

        if (nullptr != this->sceneNode)
        {
            GraphicsModule::getInstance()->setCameraTransform(this->camera, this->sceneNode->_getDerivedPositionUpdated(), this->sceneNode->_getDerivedOrientationUpdated());
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ThirdPersonOcclusionCamera]: Warning cannot use the game object component for init positioning of the camera, since it is NULL.");
        }
        NOWA::GraphicsModule::getInstance()->removeTrackedCamera(this->camera);

        this->rebuildProbeShape();
    }

    void ThirdPersonOcclusionCamera::onClearData(void)
    {
        NOWA::GraphicsModule::getInstance()->removeTrackedCamera(this->camera);
        this->probeShape.reset();
    }

    void ThirdPersonOcclusionCamera::setOffsetPosition(const Ogre::Vector3& offsetPosition)
    {
        this->offsetPosition = offsetPosition;
    }

    void ThirdPersonOcclusionCamera::setCameraSpring(Ogre::Real cameraSpring)
    {
        this->cameraSpring = cameraSpring;
    }

    void ThirdPersonOcclusionCamera::setCameraFriction(Ogre::Real cameraFriction)
    {
        this->cameraFriction = cameraFriction;
    }

    void ThirdPersonOcclusionCamera::setCameraSpringLength(Ogre::Real cameraSpringLength)
    {
        this->cameraSpringLength = cameraSpringLength;
    }

    void ThirdPersonOcclusionCamera::setLookAtOffset(const Ogre::Vector3& lookAtOffset)
    {
        this->lookAtOffset = lookAtOffset;
    }

    void ThirdPersonOcclusionCamera::setPhysicsBody(OgreNewt::Body* physicsBody)
    {
        this->physicsBody = physicsBody;
    }

    void ThirdPersonOcclusionCamera::setIgnoreGameObjectId(unsigned long ignoreGameObjectId)
    {
        this->ignoreGameObjectId = ignoreGameObjectId;
    }

    void ThirdPersonOcclusionCamera::setProbeRadius(Ogre::Real probeRadius)
    {
        this->probeRadius = probeRadius;
        // Rebuild immediately if the shape already exists, so tuning this live works -
        // cheap, analytic, no render-thread dispatch, see rebuildProbeShape().
        if (nullptr != this->probeShape)
        {
            this->rebuildProbeShape();
        }
    }

    void ThirdPersonOcclusionCamera::setSkinMargin(Ogre::Real skinMargin)
    {
        this->skinMargin = skinMargin;
    }

    void ThirdPersonOcclusionCamera::setMinDistance(Ogre::Real minDistance)
    {
        this->minDistance = minDistance;
    }

    void ThirdPersonOcclusionCamera::setReleaseSmoothValue(Ogre::Real releaseSmoothValue)
    {
        this->releaseSmoothValue = releaseSmoothValue;
    }

    void ThirdPersonOcclusionCamera::setPullInSmoothValue(Ogre::Real pullInSmoothValue)
    {
        this->pullInSmoothValue = pullInSmoothValue;
    }

    void ThirdPersonOcclusionCamera::setProbeCacheEpsilon(Ogre::Real probeCacheEpsilon)
    {
        this->probeCacheEpsilon = probeCacheEpsilon;
    }

    void ThirdPersonOcclusionCamera::setShowDebugData(bool showDebugData)
    {
        this->bShowDebugData = showDebugData;
    }

    void ThirdPersonOcclusionCamera::moveCamera(Ogre::Real dt)
    {
        if (nullptr == this->sceneNode)
        {
            return;
        }

        Ogre::Vector3 gravityDir = this->gravityDirection;
        if (gravityDir.isZeroLength())
        {
            gravityDir = Ogre::Vector3::NEGATIVE_UNIT_Y;
        }
        Ogre::Vector3 localUp = -gravityDir.normalisedCopy();

        Ogre::Vector3 playerPosition;
        Ogre::Quaternion playerOrientation;

        if (nullptr != this->physicsBody)
        {
            playerPosition = this->physicsBody->getPosition();
            playerOrientation = this->physicsBody->getOrientation();
        }
        else
        {
            playerPosition = this->sceneNode->_getDerivedPositionUpdated();
            playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();
        }

        /*if (true == this->firstSpringSample)
        {
            this->internalSpringPosition = this->camera->getPosition();
            this->firstSpringSample = false;
        }
        Ogre::Vector3 cameraPosition = this->internalSpringPosition;*/

        Ogre::Vector3 cameraPosition = this->camera->getPosition();

        playerPosition += this->lookAtOffset;

        Ogre::Vector3 camForward = playerOrientation * this->defaultDirection;
        camForward = camForward - camForward.dotProduct(localUp) * localUp;
        if (camForward.squaredLength() < 1e-6f)
        {
            Ogre::Vector3 worldRef = (Ogre::Math::Abs(localUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
            camForward = worldRef - worldRef.dotProduct(localUp) * localUp;
        }
        camForward.normalise();

        Ogre::Vector3 camRight = localUp.crossProduct(camForward);
        camRight.normalise();

        Ogre::Vector3 localOffset = camRight * this->offsetPosition.x + localUp * this->offsetPosition.y + (-camForward) * this->offsetPosition.z;

        Ogre::Vector3 worldRef2 = (Ogre::Math::Abs(localUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
        Ogre::Vector3 stableRight = localUp.crossProduct(worldRef2).normalisedCopy();
        Ogre::Vector3 stableForward = stableRight.crossProduct(localUp).normalisedCopy();

        Ogre::Vector3 direction = cameraPosition - playerPosition;
        Ogre::Vector3 directionH = direction - direction.dotProduct(localUp) * localUp;
        Ogre::Radian angle = Ogre::Math::ATan2(directionH.z, directionH.x);

        Ogre::Vector3 offsetXZ = Ogre::Math::Cos(angle) * stableRight + Ogre::Math::Sin(angle) * stableForward;

        Ogre::Vector3 targetPosition = playerPosition + offsetXZ * this->cameraSpringLength + localOffset;

        Ogre::Vector3 velocityVector = (targetPosition - cameraPosition) * this->cameraSpring /** dt*/;
        velocityVector *= this->cameraFriction;

        Ogre::Vector3 tVector = -camForward * this->cameraSpringLength;
        Ogre::Vector3 supportTarget = playerPosition + tVector + localOffset;

        Ogre::Vector3 vVector = (supportTarget - cameraPosition) * this->cameraSpring * this->moveCameraWeight;
        vVector *= this->cameraFriction;

        Ogre::Vector3 totalDisplacement = velocityVector + vVector;
        Ogre::Vector3 idealPositionVector = cameraPosition + totalDisplacement;

        this->internalSpringPosition = idealPositionVector;

        // ---------------------------------------------------------------
        // Occlusion
        // ---------------------------------------------------------------
        Ogre::Vector3 castOrigin = playerPosition;
        Ogre::Vector3 castVector = idealPositionVector - castOrigin;
        Ogre::Real desiredDistance = castVector.length();
        Ogre::Vector3 finalPositionVector = idealPositionVector;

        if (desiredDistance > 1e-4f)
        {
            Ogre::Vector3 castDir = castVector / desiredDistance;

            Ogre::Real targetDistance = desiredDistance;
            bool shouldRecast = true;

            if (true == this->hasProbedOnce)
            {
                const Ogre::Real originDeltaSq = (castOrigin - this->lastProbedOrigin).squaredLength();
                const Ogre::Real targetDeltaSq = (idealPositionVector - this->lastProbedTarget).squaredLength();
                const Ogre::Real epsilonSq = this->probeCacheEpsilon * this->probeCacheEpsilon;

                if (originDeltaSq < epsilonSq && targetDeltaSq < epsilonSq)
                {
                    shouldRecast = false;
                    targetDistance = this->lastProbeResultDistance;
                }
            }

            if (true == shouldRecast)
            {
                targetDistance = this->performOcclusionProbe(castOrigin, idealPositionVector, desiredDistance);

                this->lastProbedOrigin = castOrigin;
                this->lastProbedTarget = idealPositionVector;
                this->lastProbeResultDistance = targetDistance;
                this->hasProbedOnce = true;
            }

            if (true == this->firstOcclusionSample)
            {
                this->currentCollisionDistance = targetDistance;
                this->firstOcclusionSample = false;
            }
            else if (targetDistance < this->currentCollisionDistance)
            {
                // Pulling in toward obstacle: use dt-scaled lerp so speed is
                // frame-rate independent. pullInSmoothValue is now a "fraction of
                // the error to close per second" -- typical good value: 15-25.
                // Without dt here: at 600 FPS, old 0.5 factor closed 0.5^600 per
                // second = instant snap. At 30 FPS it closed 0.5^30 = too slow.
                // Now: error *= (1 - pullInSmoothValue * dt) each frame, so at
                // any framerate the camera takes ~1/pullInSmoothValue seconds to
                // reach the obstacle.
                Ogre::Real pullFactor = 1.0f - std::exp(-this->pullInSmoothValue * dt);

                // Also: snap immediately if the obstacle is VERY close -- prevents
                // the camera ever clipping through something that appeared suddenly
                // (e.g. entering a tunnel). Threshold: less than half the skin margin
                // of headroom means we're already clipping.
                const bool snapRequired = (targetDistance < this->currentCollisionDistance - this->skinMargin * 2.0f);
                if (snapRequired)
                {
                    this->currentCollisionDistance = targetDistance;
                }
                else
                {
                    this->currentCollisionDistance = this->currentCollisionDistance + (targetDistance - this->currentCollisionDistance) * pullFactor;
                }
            }
            else
            {
                // Releasing back out: also dt-scaled, but intentionally slower
                // than pull-in. releaseSmoothValue is "fraction of error per second",
                // typical good value: 3-6.
                Ogre::Real releaseFactor = 1.0f - std::exp(-this->releaseSmoothValue * dt);
                this->currentCollisionDistance = this->currentCollisionDistance + (targetDistance - this->currentCollisionDistance) * releaseFactor;
            }

            // Hard clamp: never let the camera go closer than minDistance from the player.
            this->currentCollisionDistance = std::max(this->currentCollisionDistance, this->minDistance);

            finalPositionVector = castOrigin + castDir * this->currentCollisionDistance;
        }

        Ogre::Quaternion resultOrientation = MathHelper::getInstance()->computeLookAtQuaternion(finalPositionVector, playerPosition, localUp);

        this->camera->setFixedYawAxis(true, localUp);
        NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, resultOrientation);
        NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, finalPositionVector);
    }

    Ogre::Real ThirdPersonOcclusionCamera::performOcclusionProbe(const Ogre::Vector3& origin, const Ogre::Vector3& target, Ogre::Real desiredDistance)
    {
        if (nullptr == this->ogreNewt || nullptr == this->probeShape)
        {
            return desiredDistance;
        }

        Ogre::Timer probeTimer;

        Ogre::Real resultDistance = desiredDistance;
        Ogre::String hitName = "none";

        if (true == this->bShowDebugData)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[ThirdPersonOcclusionCamera] ignoreBody ptr=" + Ogre::StringConverter::toString((size_t)this->physicsBody) + " ignoreGameObjectId=" + Ogre::StringConverter::toString(this->ignoreGameObjectId));
        }

        // Sphere sweep against everything EXCEPT the player's own body and Terra - see
        // OcclusionConvexcast above for why Terra is excluded (a crash inside Newton's
        // own continuous-collision code, not a bug in this calling code).
        OcclusionConvexcast convex(this->ogreNewt, this->probeShape, origin, Ogre::Quaternion::IDENTITY, target, 1, 0, this->physicsBody, this->ignoreGameObjectId, this->bShowDebugData);

        if (convex.getContactsCount() > 0)
        {
            OgreNewt::BasicConvexcast::ConvexcastContactInfo info = convex.getInfoAt(0);

            // Normalized [0,1] sweep parameter - see header comment.
            Ogre::Real t = convex.getDistanceToFirstHit();
            if (t >= 0.0f)
            {
                Ogre::Real hitDistance = t * desiredDistance;
                Ogre::Real candidate = std::max(this->minDistance, hitDistance - this->skinMargin);
                if (candidate < resultDistance)
                {
                    resultDistance = candidate;
                    if (nullptr != info.mBody && nullptr != info.mBody->getOgreNode())
                    {
                        hitName = info.mBody->getOgreNode()->getName();
                    }
                }
            }
        }

        // Terra (and TreeCollision) cluster: a SMALL CLUSTER of parallel rays, not a
        // single ray. probeRadius does NOT help Terra at all otherwise - it only
        // sizes the convex sweep's sphere above, and Terra is deliberately excluded
        // from that sweep entirely (the crash workaround). A single ray reports
        // "clear" along one infinitely-thin line; on sloped ground the actual
        // terrain surface, which has width, can still intrude into the camera's real
        // physical footprint at a point the one ray never sampled - matching exactly
        // "camera sees partly the world, partly beyond it" on a Terra hill. Reusing
        // probeRadius here gives Terra the same footprint size the sweep already
        // gives everything else, via the cheapest mechanism that doesn't touch the
        // crashing continuous-collision code path.
        Ogre::Vector3 castDir = (target - origin);
        Ogre::Real castLen = castDir.length();
        if (castLen > 1e-4f)
        {
            castDir /= castLen;

            Ogre::Vector3 up = Ogre::Vector3::UNIT_Y;
            if (Ogre::Math::Abs(castDir.dotProduct(up)) > 0.95f)
            {
                up = Ogre::Vector3::UNIT_X;
            }
            Ogre::Vector3 right = castDir.crossProduct(up).normalisedCopy();
            Ogre::Vector3 trueUp = right.crossProduct(castDir).normalisedCopy();

            Ogre::Vector3 offsets[5] = {Ogre::Vector3::ZERO, right * this->probeRadius, -right * this->probeRadius, trueUp * this->probeRadius, -trueUp * this->probeRadius};

            for (int i = 0; i < 5; ++i)
            {
                OcclusionRaycast ray(this->ogreNewt, origin + offsets[i], target + offsets[i], true, this->physicsBody, this->ignoreGameObjectId, this->bShowDebugData);
                OgreNewt::BasicRaycast::BasicRaycastInfo rayInfo = ray.getFirstHit();

                if (nullptr != rayInfo.getBody())
                {
                    // Same normalized [0,1] convention as the sweep - see
                    // BasicRaycast::go()'s "normalized t" comment.
                    Ogre::Real t = rayInfo.getDistance();
                    if (t >= 0.0f)
                    {
                        Ogre::Real hitDistance = t * desiredDistance;
                        Ogre::Real candidate = std::max(this->minDistance, hitDistance - this->skinMargin);
                        if (candidate < resultDistance)
                        {
                            resultDistance = candidate;
                            hitName = (nullptr != rayInfo.getBody()->getOgreNode()) ? rayInfo.getBody()->getOgreNode()->getName() : "?";
                        }
                    }
                }
            }
        }

        if (true == this->bShowDebugData)
        {
            unsigned long elapsedMicroseconds = probeTimer.getMicroseconds();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ThirdPersonOcclusionCamera] PROBE origin=" + Ogre::StringConverter::toString(origin) + " target=" + Ogre::StringConverter::toString(target) +
                                                                                    " desiredDistance=" + Ogre::StringConverter::toString(desiredDistance) + " probeRadius=" + Ogre::StringConverter::toString(this->probeRadius) +
                                                                                    " elapsedMicroseconds=" + Ogre::StringConverter::toString((unsigned int)elapsedMicroseconds) + " hitBody='" + hitName + "'" +
                                                                                    " resultDistance=" + Ogre::StringConverter::toString(resultDistance));
        }

        return resultDistance;
    }

    void ThirdPersonOcclusionCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
    {
    }

    Ogre::Vector3 ThirdPersonOcclusionCamera::getPosition(void)
    {
        return this->camera->getPosition();
    }

    Ogre::Quaternion ThirdPersonOcclusionCamera::getOrientation(void)
    {
        return this->camera->getOrientation();
    }

}; // namespace end