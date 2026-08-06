#ifndef BASE_CAMERA_H
#define BASE_CAMERA_H

#include "defines.h"

namespace NOWA
{
    class Ogre::SceneManager;
    class Ogre::Camera;
    class Ogre::SceneNode;

    class EXPORTED BaseCamera
    {
    public:
        friend class CameraManager;

        BaseCamera(unsigned int id, Ogre::Real moveSpeed = 20.0f, Ogre::Real rotateSpeed = 1.0f, Ogre::Real smoothValue = 0.3f, const Ogre::Vector3& defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z);

        virtual ~BaseCamera();

        virtual void setDefaultDirection(const Ogre::Vector3& defaultDirection);

        virtual void setPhysicsBody(OgreNewt::Body* body);

        virtual void moveCamera(Ogre::Real dt);

        virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

        virtual void snapToPosition(const Ogre::Vector3& position);

        virtual void snapToOrientation(const Ogre::Quaternion& orientation);

        virtual Ogre::Vector3 getPosition(void);

        virtual Ogre::Quaternion getOrientation(void);

        virtual void setMoveSpeed(Ogre::Real moveSpeed);

        virtual void setRotationSpeed(Ogre::Real rotationSpeed);

        virtual void reset(void);

        /**
         * @brief Immediately snaps the camera's transform, spring state, and occlusion
         *        cache to the target scene node's current transform, discarding all
         *        smoothing/interpolation history.
         *
         * Performs the same one-shot hard snap that onSetData() applies at initial
         * camera activation (GraphicsModule::setCameraTransform() to the target node's
         * derived position/orientation), without onSetData()'s other setup-only side
         * effects (it does not call removeTrackedCamera() and does not rebuild the
         * occlusion probe shape).
         *
         * Call this whenever the target's transform has just changed instantaneously
         * by external code -- rather than gradually through normal movement -- and
         * the camera should reflect that change immediately rather than spring-easing
         * toward it over subsequent frames. Without this, moveCamera() eases
         * internalSpringPosition and currentCollisionDistance from their prior
         * (now-stale) values toward the target's new transform, which is visible as
         * the camera lagging or appearing tilted for several frames.
         *
         * Example: UniversumComponent::requestTakeoff() sets the spaceship's
         * orientation directly via actComp->setOrientation(uprightOrient) to snap it
         * upright on liftoff. Calling snapToTarget() right after that keeps the
         * camera's spring/occlusion state consistent with the ship's new orientation
         * instead of visibly catching up to it.
         *
         * @note Resets firstSpringSample, firstOcclusionSample, hasProbedOnce, and
         *       hasFrameOrigin, and restores currentCollisionDistance to
         *       cameraSpringLength. The next moveCamera() call re-reads
         *       internalSpringPosition from the camera's just-snapped position, so no
         *       manual reset of that member is needed by the caller.
         * @warning Does nothing (and logs a warning) if sceneNode is nullptr.
         */
        virtual void snapToTarget(void);

        virtual Ogre::String getBehaviorType(void)
        {
            return "BASE_MOVE_CAMERA_" + Ogre::StringConverter::toString(this->id);
        }

        static Ogre::String BehaviorType(void)
        {
            return "BASE_MOVE_CAMERA";
        }

        unsigned int getId(void) const;

        Ogre::Camera* getCamera(void) const;

        void setSmoothValue(Ogre::Real smoothValue);

        Ogre::Real getSmoothValue(void) const;

        void setCameraControlLocked(bool cameraControlLocked);

        bool getCameraControlLocked(void) const;

        void setGravityDirection(const Ogre::Vector3& gravityDirection);

    protected:
        virtual void onSetData(void);

        virtual void onClearData(void);

    private:
        void postInitialize(Ogre::Camera* camera);

    protected:
        unsigned int id;
        Ogre::Camera* camera;

        Ogre::SceneNode* cameraNode;
        OgreNewt::Body* physicsBody;
        Ogre::Vector3 defaultDirection;
        Ogre::Real moveSpeed;
        Ogre::Real rotateSpeed;
        Ogre::Vector2 lastValue;
        bool firstTimeValueSet;
        Ogre::Vector3 lastMoveValue;
        bool firstTimeMoveValueSet;
        Ogre::Real smoothValue;
        bool cameraControlLocked;
        Ogre::Real moveCameraWeight;
        Ogre::Real rotateCameraWeight;
        Ogre::Vector3 gravityDirection;
        Ogre::Radian currentYaw;
        Ogre::Radian currentPitch;
        Ogre::Quaternion gravityBaseOrientation;
        Ogre::Vector3 lastUpVector;
    };

}; // namespace end

#endif