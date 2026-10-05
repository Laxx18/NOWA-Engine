#ifndef FOLLOW_CAMERA_2D_H
#define FOLLOW_CAMERA_2D_H

#include "BaseCamera.h"
#include "main/Events.h"

namespace NOWA
{

    class EXPORTED FollowCamera2D : public BaseCamera
    {
    public:
        /**
         * @brief A rectangular region in the XY play plane that imposes its own zoom while the
         *        player is inside it. This is the "camera zone per room" idea: narrow corridors
         *        get a zoom below 1 so they feel tight, halls get a zoom above 1 so they feel
         *        open. Zones are axis aligned and tested against the player, not the camera -
         *        testing the camera would make the zoom react to its own smoothing and oscillate.
         */
        struct CameraZone
        {
            Ogre::String zoneId;
            Ogre::Vector2 minimum;
            Ogre::Vector2 maximum;
            Ogre::Real zoom;
        };

    public:
        FollowCamera2D(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue = 0.0f);

        virtual ~FollowCamera2D();

        virtual void moveCamera(Ogre::Real dt);

        virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

        virtual Ogre::Vector3 getPosition(void);

        virtual Ogre::Quaternion getOrientation(void);

        virtual Ogre::String getBehaviorType(void)
        {
            return "FOLLOW_CAMERA_" + Ogre::StringConverter::toString(this->id);
        }

        static Ogre::String BehaviorType(void)
        {
            return "FOLLOW_CAMERA";
        }

        /**
         * @brief		Sets the offset for the camera to the player
         * @param[in]	offset	The offset vector to set
         * @Note			Alone this offset is sufficient to set the camera always correctly when a level gets loaded.
         *				That is, the camera position depends on the position of the target scene node.
         */
        void setOffset(const Ogre::Vector3& offsetPosition);

        /**
         * @brief		Sets the border offset, at which the camera will no more move
         * @param[in]	borderOffset	The border offset vector to set
         */
        void setBorderOffset(const Ogre::Vector3& borderOffset);

        void setBounds(const Ogre::Vector3& minimumBounds, const Ogre::Vector3& maximumBounds);

        void alwaysShowGameObject(bool show, const Ogre::String& category, Ogre::SceneManager* sceneManager);

        void setSceneNode(Ogre::SceneNode* sceneNode);

        // ── Lookahead ────────────────────────────────────────────────────────────────

        /**
         * @brief Leads the camera in the player's direction of travel.
         * @param[in] factor   Seconds of the player's current velocity to lead by, per axis.
         *                     0.3 to 0.4 on x reads as anticipation; y is usually left at 0,
         *                     because leading a jump upwards makes the whole screen pump.
         *                     DEFAULT IS ZERO - the camera behaves exactly as it did before the
         *                     lookahead existed until this is dialled up. A lead has to be read
         *                     against the view width to be judged: at a 5 meter camera distance
         *                     and 65 degrees field of view the half width is under 6 meters, so
         *                     two meters of lead is already a third of the screen and reads as a
         *                     lurch on every start and stop. Start at 0.1 and work up.
         * @param[in] maximum  Hard cap on the lead distance in meters, per axis. Without it a
         *                     dash or a long fall throws the camera off the player entirely.
         * @param[in] smooth   Settle time of the lead in SECONDS, not a per-frame weight. This is
         *                     deliberately separate from the camera's own smooth value: a lead
         *                     that arrives as fast as the camera moves reads as a twitch on every
         *                     direction tap. 0.5 to 0.8 turns the lead into a drift.
         */
        void setLookahead(const Ogre::Vector2& factor, const Ogre::Vector2& maximum, Ogre::Real smooth);

        Ogre::Vector2 getLookahead(void) const;

        // ── Zoom, camera zones and punch zoom ────────────────────────────────────────

        /**
         * @brief Scripted zoom. 1.0 is the authored framing, below 1 moves the camera closer,
         *        above 1 pulls it back. Implemented by scaling the DISTANCE to the play plane,
         *        not the field of view, so the perspective never breathes - only the framing
         *        changes. The view extents and therefore the bounds clamping follow along.
         * @param[in] blendTime Seconds to settle. 0 snaps.
         */
        void setZoom(Ogre::Real zoom, Ogre::Real blendTime);

        Ogre::Real getZoom(void) const;

        /**
         * @brief The zoom actually in effect this frame: the scripted zoom, times the zone the
         *        player is standing in, times the current punch. Read only.
         */
        Ogre::Real getAppliedZoom(void) const;

        void setZoomBlendTime(Ogre::Real blendTime);

        /**
         * @brief A short, self-decaying zoom kick for impacts.
         * @param[in] amount   Relative change: -0.08 snaps 8% closer, +0.08 snaps 8% back.
         *                     Negative is the usual one - a hit pulls the camera in.
         * @param[in] duration Seconds until it is fully gone. 0.12 to 0.2 is the useful range;
         *                     longer stops reading as an impact and starts reading as a zoom.
         */
        void punchZoom(Ogre::Real amount, Ogre::Real duration);

        /**
         * @brief Adds or replaces a zone by id. The zone's zoom MULTIPLIES the scripted zoom
         *        rather than overriding it, so a scripted zoom out still works inside a tight
         *        corridor instead of being silently ignored.
         */
        void addCameraZone(const Ogre::String& zoneId, const Ogre::Vector2& minimum, const Ogre::Vector2& maximum, Ogre::Real zoom);

        void removeCameraZone(const Ogre::String& zoneId);

        void clearCameraZones(void);

        // ── Screen shake ─────────────────────────────────────────────────────────────

        /**
         * @brief Adds trauma, which decays on its own. Strength ACCUMULATES and is applied
         *        squared, so a light hit stays almost invisible while several hits in a row
         *        build into a real jolt. That is what makes one call carry impact strength;
         *        a fixed shake per hit reads as "something happened" and nothing more.
         * @param[in] strength 0 to 1. 0.15 light hit, 0.35 solid hit, 0.7 explosion.
         */
        void addShake(Ogre::Real strength);

        void setShakeParameters(const Ogre::Vector2& maxOffset, Ogre::Real frequency, Ogre::Real decay);

        Ogre::Real getShakeTrauma(void) const;

        // ── Hitstop ──────────────────────────────────────────────────────────────────

        /**
         * @brief Freezes the camera's FOLLOW for a moment while the shake and the punch zoom
         *        keep running - which is exactly what makes a hit land: the world stops, the
         *        lens does not. This only freezes the camera; freezing the simulation itself
         *        is the game logic's job.
         * @param[in] duration Seconds. 0.05 to 0.12 is the useful range.
         */
        void startHitstop(Ogre::Real duration);

        bool isHitstopActive(void) const;

        // ── Edge orthographic morph ──────────────────────────────────────────────────

        /**
         * @brief Morphs towards an orthographic projection while the camera is clamped against
         *        a horizontal bound and the player walks further out to the side. The point is
         *        the occlusion case: in perspective, the ray from the eye to a player standing
         *        at the screen edge runs diagonally through anything in front of the play plane,
         *        so a tall wall swallows him. Orthographic removes that ray entirely.
         *
         *        OFF BY DEFAULT, deliberately. The morph is continuous - no projection switch
         *        pops any more - but going orthographic collapses the parallax between depth
         *        layers, so every background and foreground band visibly slides while it blends.
         *        With decor bands in front of and behind the play plane that is very noticeable.
         *        Fading the occluder out instead (see alwaysShowGameObject) costs nothing
         *        visually; this is the fallback for when that is not an option.
         *
         * @param[in] blendTime Seconds to blend each way. 0 restores the old hard switch.
         */
        void setEdgeOrthographicEnabled(bool enabled, Ogre::Real blendTime);

        bool isEdgeOrthographicEnabled(void) const;

        /**
         * @brief 0 = pure perspective, 1 = pure orthographic, anything between = blending.
         */
        Ogre::Real getEdgeOrthographicMorph(void) const;

    protected:
        virtual void onSetData(void);

        virtual void onClearData(void) override;

    private:
        void handleUpdateBounds(NOWA::EventDataPtr eventData);

        /**
         * @brief Decides whether the edge morph should be on and advances the morph factor. Takes
         *        dt because the morph is driven HERE, on the logic thread - the render side only
         *        applies the value it finds. Driving it inside the render closure made the blend
         *        depend on what the tracked closure passes as its delta and on how often it is
         *        ticked, which is what let the way into orthographic ease over the blend time
         *        while the way back snapped.
         */
        void updateEdgeOrthographic(Ogre::Real dt, const Ogre::Vector3& playerPosition, const Ogre::Vector3& cameraPosition, bool cameraClampedX, bool cameraClampedY);

        /**
         * @brief Recomputes the half extents of the view at the play plane from the CURRENT zoom.
         *        Everything that clamps against the scene bounds reads mostRightUp, so this has
         *        to run before the clamping, every frame the zoom can have changed - otherwise a
         *        zoomed out camera clamps as if it were still zoomed in and shows the void
         *        outside the level.
         *
         *        Pure arithmetic over baseHalfHeight and baseAspectRatio. It deliberately reads
         *        NOTHING back from the camera - see setBounds for why that would be a trap.
         */
        void recomputeViewExtents(void);

        void updateLookahead(Ogre::Real dt, const Ogre::Vector3& playerPosition);

        void updateZoom(Ogre::Real dt, const Ogre::Vector3& playerPosition);

        void updateShake(Ogre::Real dt);

    protected:
        Ogre::SceneManager* sceneManager;
        bool firstTimeValueSet;
        Ogre::Vector3 lastMoveValue;
        bool firstTimeMoveValueSet;
        Ogre::Real smoothValue;
        Ogre::Vector3 offset;
        Ogre::Vector3 borderOffset;
        Ogre::Vector3 minimumBounds;
        Ogre::Vector3 maximumBounds;

        // This class's OWN record of the position it last told the camera to move to - NOT a readback
        // from the Ogre::Camera object. moveCamera() runs on the logic thread and issues its updates
        // through GraphicsModule, which applies them on the RENDER thread one or more frames later.
        // Reading this->camera->getPosition() back on the very next logic tick therefore does not
        // return what was just requested - it returns whatever the render thread last actually
        // applied, which can lag behind by an arbitrary number of frames depending on queue depth.
        // Using that stale value as the reference point for computing the NEXT correction removed the
        // one thing a feedback loop needs to converge: a reference that actually reflects what was
        // last commanded. See moveCamera() for the full explanation and the divergent-oscillation
        // symptom this caused in testing.
        //
        // It stays SHAKE FREE for the same reason: the shake is added on the way out to the render
        // thread only. Folding it into the tracked position would feed the shake back into the next
        // correction, and the follow would chase its own jitter.
        Ogre::Vector3 trackedCameraPosition;

        bool showGameObject;
        Ogre::RaySceneQuery* raySceneQuery;
        Ogre::String category;
        Ogre::SceneNode* hiddenSceneNode;
        Ogre::Real fadeValue;
        bool fadingFinished;
        Ogre::Vector3 mostRightUp;

        Ogre::ManualObject* pDebugLine;
        Ogre::SceneNode* sceneNode;

        // The view geometry at zoom 1, captured ONCE in setBounds from the camera's own field of
        // view and aspect ratio. Everything the zoom and the bounds clamp need is derived from
        // these two numbers, so no code path has to read the camera's projection back per frame.
        //
        // That is not a micro optimisation, it is the fix for a divergence: Frustum::setOrthoWindow
        // OVERWRITES the aspect ratio with the window's width/height, and the orthographic window
        // is itself derived from mostRightUp, which was derived from the aspect ratio. Reading the
        // aspect ratio back each frame closed that loop, so the extents ran away within a handful
        // of frames and the bounds clamp carried the camera out of the level - and it did not heal
        // when the morph was switched off again, because nothing put the aspect ratio back.
        Ogre::Real baseHalfHeight;
        Ogre::Real baseAspectRatio;

        bool edgeOrthographicClosureRegistered;
        bool edgeOrthographicActive;
        Ogre::Vector3 edgePlayerPosition;
        Ogre::Vector3 edgeCameraPosition;
        Ogre::Vector2 edgeOrthoWindow;
        bool edgeCameraClampedX;
        bool edgeCameraClampedY;

        // ── Lookahead ────────────────────────────────────────────────────────────────
        Ogre::Vector2 lookaheadFactor;
        Ogre::Vector2 lookaheadMaximum;
        Ogre::Real lookaheadSmooth;
        Ogre::Vector2 currentLookahead;
        Ogre::Vector3 lastPlayerPosition;
        bool firstTimeLookaheadSet;

        // ── Zoom, zones, punch ───────────────────────────────────────────────────────
        std::vector<CameraZone> cameraZones;
        Ogre::Real requestedZoom;
        Ogre::Real currentZoom;
        Ogre::Real appliedZoom;
        Ogre::Real zoomBlendTime;
        Ogre::Real punchAmount;
        Ogre::Real punchDuration;
        Ogre::Real punchRemaining;
        Ogre::Real punchCurrent;

        // The z of the play plane, captured once from the target node. The camera's z is then
        // always playPlaneZ + offset.z * zoom, so zooming moves it along its own view axis and
        // never drifts with a player who moves in z.
        Ogre::Real playPlaneZ;
        bool playPlaneZSet;

        // ── Shake ────────────────────────────────────────────────────────────────────
        Ogre::Real shakeTrauma;
        Ogre::Real shakeDecay;
        Ogre::Real shakeFrequency;
        Ogre::Vector2 shakeMaximumOffset;
        Ogre::Real shakeTime;
        Ogre::Vector2 shakeOffset;

        // ── Hitstop ──────────────────────────────────────────────────────────────────
        Ogre::Real hitstopRemaining;

        // ── Edge orthographic morph ──────────────────────────────────────────────────
        bool edgeOrthographicEnabled;
        Ogre::Real edgeOrthoBlendTime;

        // The blend as a LINEAR ramp in 0..1, stepped by dt / blendTime. edgeOrthoMorph below is
        // this value run through a smoothstep, and that is what the render side applies.
        //
        // Two values rather than one because an exponential approach is symmetric in the maths
        // and asymmetric to the eye: it spends most of its speed at the start of the blend, which
        // on the way to orthographic falls in the range where the picture hardly changes, and on
        // the way back falls exactly where the projection changes most. A linear ramp gives every
        // part of the transition the same share of the time in both directions, and the smoothstep
        // supplies the soft ends the exponential was there for.
        Ogre::Real edgeOrthoBlend;
        Ogre::Real edgeOrthoMorph;

        // Member, not a function-local static: a static is shared by EVERY FollowCamera2D in
        // the process, so with two behaviors alive the second one's state change would be
        // swallowed because the first had already set the flag.
        bool lastLoggedOrthographicState;
    };

}; // namespace end

#endif