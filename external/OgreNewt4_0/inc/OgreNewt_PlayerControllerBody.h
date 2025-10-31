#pragma once

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"

#include "OgreNewt_PlayerController.h"

// ND4
#include "ndBodyPlayerCapsule.h"
#include "ndWorld.h"
#include "ndSharedPtr.h"

namespace OgreNewt
{
    // ----------------------------------------------------------------
    // Public API-compatible wrapper
    // ----------------------------------------------------------------
    class _OgreNewtExport PlayerControllerBody : public Body, private OgreNewtPlayerController::Owner
    {
    public:
        PlayerControllerBody(World* world, Ogre::SceneManager* sceneManager, const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction,
            Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, const Ogre::Vector3& collisionPosition, unsigned int categoryId, PlayerCallback* playerCallback = nullptr);

        virtual ~PlayerControllerBody();

        // API preserved
        void move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngle);
        void setHeading(const Ogre::Radian& headingAngle);
        void stop();
        void toggleCrouch(); // TODO: optional (requires shape change) - stubbed for now

        void setVelocity(const Ogre::Vector3& velocity);
        Ogre::Vector3 getVelocity() const;

        void setFrame(const Ogre::Quaternion& frame);
        Ogre::Quaternion getFrame() const;

        void reCreatePlayer(
            const Ogre::Quaternion& startOrientation,
            const Ogre::Vector3& startPosition,
            const Ogre::Vector3& direction,
            Ogre::Real              mass,
            Ogre::Real              radius,
            Ogre::Real              height,
            Ogre::Real              stepHeight,
            const Ogre::Vector3& collisionPosition,   // API parity
            unsigned int            categoryId,
            PlayerCallback* playerCallback);

        void setDirection(const Ogre::Vector3& direction);
        Ogre::Vector3 getDirection() const;

        void setMass(Ogre::Real mass);
        void setCollisionPositionOffset(const Ogre::Vector3& collisionPositionOffset);
        Ogre::Vector3 getCollisionPositionOffset() const;

        void setRadius(Ogre::Real radius);
        Ogre::Real getRadius() const;

        void setHeight(Ogre::Real height);
        Ogre::Real getheight() const;

        void setStepHeight(Ogre::Real stepHeight);
        Ogre::Real getStepHeight() const;

        Ogre::Vector3 getUpDirection() const;
        bool isInFreeFall() const;
        bool isOnFloor() const;
        bool isCrouched() const; // TODO: stubbed false

        void setWalkSpeed(Ogre::Real walkSpeed);
        Ogre::Real getWalkSpeed() const;

        void setJumpSpeed(Ogre::Real jumpSpeed);
        Ogre::Real getJumpSpeed() const;

        void setCanJump(bool canJump);
        bool getCanJump() const;

        Ogre::Real getForwardSpeed() const;
        Ogre::Real getSideSpeed() const;
        Ogre::Radian getHeading() const;

        void setStartOrientation(const Ogre::Quaternion& startOrientation);
        Ogre::Quaternion getStartOrientation() const;

        void setGravityDirection(const Ogre::Vector3& gravityDir);
        void setActive(bool active);
        bool isActive(void) const override;

        PlayerCallback* getPlayerCallback() const;

    private:
        Ogre::Vector3   getGravity() const override { return m_gravity; }
        bool            consumeJumpFlag() override { const bool j = m_canJump; m_canJump = false; return j; }
        Ogre::Real      forwardCmd() const override { return m_forwardSpeed; }
        Ogre::Real      strafeCmd() const override { return m_sideSpeed; }
        Ogre::Radian    headingCmd() const override { return m_heading; }
        PlayerCallback* getCallback() const override { return m_playerCallback; }

        void createPlayer(const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition);
    private:
        World* m_worldRef;
        OgreNewtPlayerController* m_player; // owned by world (shared_ptr), we keep raw for quick access

        Ogre::Vector3   m_startPosition;
        Ogre::Quaternion m_startOrientation;
        Ogre::Vector3   m_direction;

        Ogre::Real      m_mass;
        Ogre::Real      m_radius;
        Ogre::Real      m_height;
        Ogre::Real      m_stepHeight;
        Ogre::Vector3   m_collisionPositionOffset; // kept for API parity

        Ogre::Real      m_forwardSpeed;
        Ogre::Real      m_sideSpeed;
        Ogre::Radian    m_heading;
        Ogre::Radian    m_startHeading;

        Ogre::Real      m_walkSpeed;
        Ogre::Real      m_jumpSpeed;
        bool            m_canJump;

        unsigned int    m_categoryId;

        bool          m_active{ true };
        Ogre::Vector3 m_gravity{ 0.0f, -16.8f, 0.0f };

        PlayerCallback* m_playerCallback;
    };

} // namespace OgreNewt
