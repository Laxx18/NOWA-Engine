#ifndef THIRDPERSONOCCLUSIONCAMERA_H
#define THIRDPERSONOCCLUSIONCAMERA_H

#include "BaseCamera.h"
#include "OgreNewt_Prerequisites.h"
#include <vector>

namespace Ogre
{
    class SceneNode;
}

namespace OgreNewt
{
    class World;
    class Body;
}

namespace NOWA
{
    /**
     * @brief ThirdPersonCamera-equivalent that additionally prevents the camera from
     *        clipping through level geometry between the player and the spring's ideal
     *        camera position - the "spring arm + shape cast" technique used by Unreal's
     *        SpringArmComponent and most modern third-person AAA games.
     *
     *        Rewritten to use OgreNewt::BasicConvexcast (a real sphere sweep against
     *        the PHYSICS broad-phase) instead of a cluster of Ogre::RaySceneQuery rays
     *        against the render scene graph. This is both cheaper (one purpose-built
     *        broad-phase sweep instead of several independent scene queries) AND
     *        entirely logic-thread-resident: physics stepping already happens off the
     *        render thread in this engine (see PhysicsActiveComponent::moveCallback
     *        and PhysicsComponent's direct, non-enqueued use of OgreNewt::BasicRaycast),
     *        and the sphere shape used for the sweep is a pure analytic primitive
     *        (no mesh data read), so building it needs no enqueueAndWait either - see
     *        BasePhysicsCamera::onSetData constructing its Ellipsoid the same way,
     *        directly, with no render-thread dispatch.
     *
     *        Practical consequence: this class has NO cross-thread state at all
     *        anymore. No LockFreeMailbox, no tracked closure, no render-thread-only
     *        cache. Everything - spring math AND occlusion sweep - runs synchronously
     *        inside moveCamera() on the logic thread, exactly like a normal function
     *        call. Grass has no physics body (would be insane for physics perf to give
     *        every blade one), so the sweep correctly never reacts to it - the
     *        flickering hit-distance noise seen with the old Ogre-scene-graph approach
     *        should not recur, since that noise came specifically from hitting grass
     *        geometry the camera had no business treating as an obstruction. Trees and
     *        static level geometry DO have real collision (confirmed: "TreeCollision"
     *        in this engine refers to a BVH-optimized static collision type used for
     *        large static interiors, not a property of plant trees - actual trees use
     *        ordinary collision shapes), so the sweep will react to them correctly.
     *
     *        CRITICAL: the spring evolves from internalSpringPosition, a private member
     *        that tracks the spring's own natural, unconstrained trajectory - it is
     *        NEVER read back from this->camera->getPosition(). Occlusion only affects
     *        what is actually published to the real camera (finalPositionVector in
     *        moveCamera()); the spring's internal state has no knowledge that occlusion
     *        exists at all. See moveCamera() for the full reasoning - this part is
     *        unchanged from the previous version and remains necessary regardless of
     *        which occlusion mechanism is used.
     *
     *        A small position-delta cache (probeCacheEpsilon) still skips re-sweeping
     *        when the player hasn't moved meaningfully - now just a plain same-thread
     *        check, no synchronization concerns at all since there is only one thread
     *        involved.
     *
     *        NOTE: duplicates ThirdPersonCamera's spring math rather than subclassing it,
     *        because BaseCamera.h/ThirdPersonCamera.h were not available to verify member
     *        access levels at the time this was written.
     *
     *        VERIFY: getDistanceToFirstHit() on OgreNewt::BasicConvexcast/BasicRaycast
     *        is a normalized [0,1] sweep/ray parameter (per Convexcast::go()'s comment
     *        and RayCastTire's "m_hitParam" naming), NOT a world-space distance -
     *        performOcclusionProbe() multiplies it by the swept distance accordingly.
     *
     *        IMPORTANT: the convex sweep deliberately excludes any body whose
     *        OgreNewt::Body::getCollisionPrimitiveType() reports HeighFieldPrimitiveType
     *        (Terra - confirmed via PhysicsComponent::createHeightFieldCollision()) or
     *        TreeCollisionPrimitiveType (e.g. a house with interior - excluded
     *        defensively, see below). Sweeping a convex shape against either still
     *        triggers a crash inside Newton Dynamics 4's own continuous-collision code
     *        (ndShapeConvexPolygon::GenerateConvexCap reading past the end of
     *        m_adjacentFaceEdgeNormalIndex), meaning Newton routes both heightfields
     *        and true BVH static meshes through the same generic "convex vs polygon
     *        soup, continuous" contact path internally, regardless of which concrete
     *        shape feeds it - a bug in the physics engine itself, not in this calling
     *        code. TreeCollision has not been directly observed crashing yet, but
     *        shares the same underlying shape category as the confirmed Terra crash,
     *        so it is excluded proactively rather than waiting to hit it near a house.
     *
     *        getCollisionPrimitiveType() is a real, already-existing OgreNewt::Body
     *        accessor (confirmed from its own implementation, which walks Newton's
     *        GetAsShapeXxx() RTTI-style methods internally) - not a guess at Newton
     *        internals or a dynamic_cast against a wrapper class.
     *
     *        Terra is checked separately via a plain (non-continuous)
     *        OgreNewt::BasicRaycast along the same line, which does not exercise the
     *        crashing code path regardless of how the sweep's exclusion is implemented.
     *        This same raycast also naturally covers TreeCollision bodies (it filters
     *        only the player's own body, nothing else), so excluding TreeCollision from
     *        the sweep does not leave houses undetected.
     */
    class EXPORTED ThirdPersonOcclusionCamera : public BaseCamera
    {
    public:
        ThirdPersonOcclusionCamera(unsigned int id, Ogre::SceneNode* sceneNode, OgreNewt::World* ogreNewt, const Ogre::Vector3& defaultDirection, const Ogre::Vector3& offsetPosition, const Ogre::Vector3& lookAtOffset, Ogre::Real cameraSpring,
            Ogre::Real cameraFriction, Ogre::Real cameraSpringLength);

        virtual ~ThirdPersonOcclusionCamera();

        /**
         * @see		BaseCamera::onSetData
         */
        virtual void onSetData(void) override;

        /**
         * @see		BaseCamera::onClearData
         */
        virtual void onClearData(void) override;

        /**
         * @see		BaseCamera::moveCamera
         */
        virtual void moveCamera(Ogre::Real dt) override;

        /**
         * @see		BaseCamera::rotateCamera
         */
        virtual void rotateCamera(Ogre::Real dt, bool forJoyStick) override;

        /**
         * @see		BaseCamera::getPosition
         */
        virtual Ogre::Vector3 getPosition(void) override;

        /**
         * @see		BaseCamera::getOrientation
         */
        virtual Ogre::Quaternion getOrientation(void) override;

        void setOffsetPosition(const Ogre::Vector3& offsetPosition);

        void setCameraSpring(Ogre::Real cameraSpring);

        void setCameraFriction(Ogre::Real cameraFriction);

        void setCameraSpringLength(Ogre::Real cameraSpringLength);

        void setLookAtOffset(const Ogre::Vector3& lookAtOffset);

        /**
         * @brief The player's own physics body - read directly for position/orientation
         *        (avoiding the one-frame SceneNode buffering lag at speed, same reasoning
         *        ThirdPersonCamera already uses this for) AND passed as a first, cheap
         *        ignore-body check to the sweep/raycast. This pointer alone was found
         *        unreliable in practice (observed null at runtime despite being set) -
         *        see setIgnoreGameObjectId() for the actual robust exclusion mechanism.
         */
        void setPhysicsBody(OgreNewt::Body* physicsBody);

        /**
         * @brief The player's own GameObject id - the actual, robust mechanism that
         *        keeps the occlusion sweep/raycast from treating the player's own
         *        collision as an obstruction. Two earlier approaches were tried and
         *        both failed in practice: a cached SceneNode pointer (went stale if
         *        the node was ever recreated after construction) and a SceneNode name
         *        comparison (broke the moment the node was renamed in NOWA-Design). A
         *        GameObject's id is permanent for its whole lifetime and never
         *        editable from the editor, so this is the one that should actually
         *        hold up. See isPlayerBody() in the .cpp file.
         */
        void setIgnoreGameObjectId(unsigned long ignoreGameObjectId);

        /**
         * @brief Radius of the swept sphere, in world units (roughly the camera lens's
         *        physical size - a few tens of centimeters, NOT meters). Rebuilds the
         *        probe shape immediately if called after onSetData(). Default 0.25.
         */
        void setProbeRadius(Ogre::Real probeRadius);

        /// Extra pull-in beyond the actual hit point so the lens doesn't sit on the surface. Default 0.15.
        void setSkinMargin(Ogre::Real skinMargin);

        /// Camera is never allowed closer to the pivot than this, even if fully boxed in. Default 0.5.
        void setMinDistance(Ogre::Real minDistance);

        /**
         * @brief Smoothing used ONLY when releasing back outward (obstruction
         *        clearing). Pulling in is now an INSTANT hard snap with no smoothing
         *        at all (see moveCamera()) - the earlier justification for smoothing
         *        it (hiding noise from Ogre-scene-graph raycasts hitting grass) no
         *        longer applies now that occlusion uses real physics geometry. A
         *        SMALL value here means MORE smoothing (see MathHelper::lowPassFilter:
         *        result = (1-scale)*old + scale*new) - default 0.1 is fairly gradual.
         *        No dt term anywhere in this class, by design - this project does not
         *        run with vsync.
         */
        void setReleaseSmoothValue(Ogre::Real releaseSmoothValue);


        void setPullInSmoothValue(Ogre::Real pullInSmoothValue);

        /**
         * @brief Minimum combined movement of the sweep origin AND target (in world
         *        units) before the sweep is re-run at all. Below this, last frame's
         *        result is reused verbatim with no physics query. Default 0.02 (2cm).
         */
        void setProbeCacheEpsilon(Ogre::Real probeCacheEpsilon);

        /**
         * @brief Logs one line per actual sweep (cache misses only): origin/target/
         *        distance, elapsed microseconds, hit count, hit body name if any,
         *        result distance. Default false.
         */
        void setShowDebugData(bool showDebugData);

    private:
        /**
         * @brief Builds (or rebuilds) the swept sphere shape from the current
         *        probeRadius. Pure analytic primitive - no render-thread dispatch
         *        needed, see the class comment above.
         */
        void rebuildProbeShape(void);

        /**
         * @brief Runs the actual convex sweep, synchronously, on whichever thread calls
         *        it (always the logic thread, via moveCamera()). Returns the allowed
         *        camera distance along the origin->target line - either the full
         *        desiredDistance if nothing was hit, or the hit distance minus
         *        skinMargin (clamped to minDistance).
         */
        Ogre::Real performOcclusionProbe(const Ogre::Vector3& origin, const Ogre::Vector3& target, Ogre::Real desiredDistance);

    private:
        Ogre::SceneNode* sceneNode;
        OgreNewt::World* ogreNewt;
        Ogre::Vector3 offsetPosition;
        Ogre::Vector3 lookAtOffset;
        Ogre::Real cameraSpring;
        Ogre::Real cameraFriction;
        Ogre::Real cameraSpringLength;
        OgreNewt::Body* physicsBody;
        unsigned long ignoreGameObjectId;

        OgreNewt::CollisionPtr probeShape;
        Ogre::Real probeRadius;

        Ogre::Real skinMargin;
        Ogre::Real minDistance;
        Ogre::Real releaseSmoothValue;
        Ogre::Real pullInSmoothValue;

        Ogre::Real currentCollisionDistance;
        bool firstOcclusionSample;

        // The spring's own state, decoupled from this->camera->getPosition() - see the
        // class comment above and moveCamera() for why.
        Ogre::Vector3 internalSpringPosition;
        bool firstSpringSample;

        // Same-thread cache now (no cross-thread concerns at all) - skips re-sweeping
        // when the player hasn't moved meaningfully since the last sweep.
        Ogre::Vector3 lastProbedOrigin;
        Ogre::Vector3 lastProbedTarget;
        Ogre::Real lastProbeResultDistance;
        bool hasProbedOnce;
        Ogre::Real probeCacheEpsilon;

        bool bShowDebugData;
    };

}; // namespace end

#endif