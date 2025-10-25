#include "OgreNewt_PlayerControllerBody.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_BodyNotify.h"

namespace
{
    static inline Ogre::Real normalizeRadian(Ogre::Real rad)
    {
        const Ogre::Real twoPi = Ogre::Math::TWO_PI;
        rad = std::fmod(rad, twoPi);
        if (rad < 0) rad += twoPi;
        return rad;
    }
}

namespace OgreNewt
{
    // ================================================================
    // PlayerControllerBody
    // ================================================================
    PlayerControllerBody::PlayerControllerBody(
        World* world, Ogre::SceneManager* sceneManager,
        const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction,
        Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, unsigned int categoryId,
        PlayerCallback* playerCallback)
        : Body(world, sceneManager, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC)
        , m_worldRef(world)
        , m_player(nullptr)
        , m_startPosition(startPosition)
        , m_startOrientation(startOrientation)
        , m_direction(direction)
        , m_mass(mass)
        , m_radius(radius)
        , m_height(height)
        , m_stepHeight(stepHeight)
        , m_collisionPositionOffset(Ogre::Vector3::ZERO)
        , m_forwardSpeed(0.0f)
        , m_sideSpeed(0.0f)
        , m_heading(0.0f)
        , m_startHeading(0.0f)
        , m_walkSpeed(10.0f)
        , m_jumpSpeed(20.0f)
        , m_canJump(false)
        , m_categoryId(categoryId)
        , m_playerCallback(playerCallback)
    {
        reCreatePlayer(startOrientation, startPosition, direction, mass, radius, height, stepHeight, categoryId, playerCallback);
    }

    PlayerControllerBody::~PlayerControllerBody()
    {
        if (m_playerCallback)
        {
            delete m_playerCallback;
            m_playerCallback = nullptr;
        }
        // m_player is owned by the ndWorld via shared_ptr; no manual delete
        m_player = nullptr;
    }

    void PlayerControllerBody::createPlayer(const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition)
    {
        ndWorld* world = m_worldRef->getNewtonWorld();
        if (!world) return;

        // Build local axis: up = Y, "forward axis" from m_direction
        ndMatrix localAxis(ndGetIdentityMatrix());
        localAxis[0] = ndVector(0.0f, 1.0f, 0.0f, 0.0f); // up in ND4 player is axis 0 (as used in samples)
        localAxis[1] = ndVector(m_direction.x, m_direction.y, m_direction.z, 0.0f);
        localAxis[2] = localAxis[0].CrossProduct(localAxis[1]);

        // Create ND4 player capsule
        auto* playerBody = new OgreNewtPlayerCapsule(this, localAxis,
            ndFloat32(m_mass), ndFloat32(m_radius),
            ndFloat32(m_height), ndFloat32(m_stepHeight));

        // Set start transform
        ndMatrix start(ndGetIdentityMatrix());
        // convert orientation + position
        {
            ndMatrix tmp;
            OgreNewt::Converters::QuatPosToMatrix(startOrientation, startPosition, tmp);
            start = tmp;
        }
        playerBody->SetMatrix(start);

        // Attach our notify so visual updates/gravity pipeline stay consistent with Body
        if (!m_bodyNotify)
            m_bodyNotify = new BodyNotify(this);
        playerBody->SetNotifyCallback(m_bodyNotify);

        // Add to ND4 world
        ndSharedPtr<ndBody> bodyPtr(playerBody);
        world->AddBody(bodyPtr);

        // Tie into OgreNewt::Body base (so all Body API keeps working)
        m_body = playerBody;
        m_player = playerBody;

        // category (API parity; store only on Ogre side)
        setType(m_categoryId);
    }

    void PlayerControllerBody::reCreatePlayer(const Ogre::Quaternion& startOrientation, const Ogre::Vector3& startPosition, const Ogre::Vector3& direction,
        Ogre::Real mass, Ogre::Real radius, Ogre::Real height, Ogre::Real stepHeight, unsigned int categoryId, PlayerCallback* playerCallback)
    {
        m_startOrientation = startOrientation;
        m_startPosition = startPosition;
        m_direction = direction;
        m_mass = mass;
        m_radius = radius;
        m_height = height;
        m_stepHeight = stepHeight;

        if (m_playerCallback && (m_playerCallback != playerCallback))
            delete m_playerCallback;
        m_playerCallback = playerCallback;

        // In ND4, removing a body requires keeping the shared_ptr handle; since we don't keep it,
        // we let the world own & recycle. For re-create, we just make a new capsule and reassign.
        createPlayer(m_startOrientation, m_startPosition);

        // Initialize heading from orientation yaw as in your old code
        move(0.0f, 0.0f, m_startOrientation.getYaw());
    }

    // ---------------- movement API ----------------
    void PlayerControllerBody::move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngle)
    {
        m_forwardSpeed = forwardSpeed;
        m_sideSpeed = sideSpeed;
        const Ogre::Radian newHeading = m_startHeading + headingAngle;
        m_heading = Ogre::Radian(normalizeRadian(newHeading.valueRadians()));
    }

    void PlayerControllerBody::setHeading(const Ogre::Radian& headingAngle)
    {
        const Ogre::Radian newHeading = m_startHeading + headingAngle;
        m_heading = Ogre::Radian(normalizeRadian(newHeading.valueRadians()));
    }

    void PlayerControllerBody::stop()
    {
        m_forwardSpeed = 0.0f;
        m_sideSpeed = 0.0f;
    }

    void PlayerControllerBody::toggleCrouch()
    {
        // TODO: Implement by swapping shape height (requires recreating the capsule or ND4 support).
        // Keeping API: no-op for now.
    }

    // ---------------- kinematics helpers ----------------
    void PlayerControllerBody::setVelocity(const Ogre::Vector3& velocity)
    {
        if (!m_player) return;
        m_player->SetVelocity(ndVector(velocity.x, velocity.y, velocity.z, 0.0f));
    }

    Ogre::Vector3 PlayerControllerBody::getVelocity() const
    {
        if (!m_player) return Ogre::Vector3::ZERO;
        const ndVector v = m_player->GetVelocity();
        return Ogre::Vector3(v.m_x, v.m_y, v.m_z);
    }

    void PlayerControllerBody::setFrame(const Ogre::Quaternion& /*frame*/)
    {
        // API parity only. ND4 player uses SetHeadingAngle() each step; we control that via move()/setHeading()
    }

    Ogre::Quaternion PlayerControllerBody::getFrame() const
    {
        if (!m_player) return Ogre::Quaternion::IDENTITY;
        const ndMatrix m = m_player->GetMatrix();
        Ogre::Quaternion q; Ogre::Vector3 p;
        OgreNewt::Converters::MatrixToQuatPos(&m[0][0], q, p);
        return q;
    }

    // ---------------- parameters ----------------
    void PlayerControllerBody::setDirection(const Ogre::Vector3& direction)
    {
        m_direction = direction;
    }

    Ogre::Vector3 PlayerControllerBody::getDirection() const
    {
        return m_direction;
    }

    void PlayerControllerBody::setMass(Ogre::Real mass)
    {
        m_mass = mass; /* would need recreate to take effect */
    }

    void PlayerControllerBody::setCollisionPositionOffset(const Ogre::Vector3& collisionPositionOffset)
    {
        // Kept for API; ndBodyPlayerCapsule keeps shape centered. Store value only.
        m_collisionPositionOffset = collisionPositionOffset;
    }
    Ogre::Vector3 PlayerControllerBody::getCollisionPositionOffset() const { return m_collisionPositionOffset; }

    void PlayerControllerBody::setRadius(Ogre::Real radius)
    {
        m_radius = radius; /* recreate to take effect */
    }

    Ogre::Real PlayerControllerBody::getRadius() const
    {
        return m_radius;
    }

    void PlayerControllerBody::setHeight(Ogre::Real height)
    {
        m_height = height; /* recreate to take effect */ 
    }

    Ogre::Real PlayerControllerBody::getheight() const 
    { 
        return m_height; 
    }

    void PlayerControllerBody::setStepHeight(Ogre::Real stepHeight) 
    { 
        m_stepHeight = stepHeight; /* recreate to take effect */ 
    }

    Ogre::Real PlayerControllerBody::getStepHeight() const
    {
        return m_stepHeight;
    }

    Ogre::Vector3 PlayerControllerBody::getUpDirection() const
    {
        // Player up is localAxis[0] = Y
        return Ogre::Vector3(0, 1, 0);
    }

    bool PlayerControllerBody::isInFreeFall() const
    {
        return m_player ? !m_player->IsOnFloor() : false;
    }

    bool PlayerControllerBody::isOnFloor() const
    {
        return m_player ? m_player->IsOnFloor() : false;
    }

    bool PlayerControllerBody::isCrouched() const
    {
        // No crouch toggle implemented (see toggleCrouch TODO)
        return false;
    }

    void PlayerControllerBody::setWalkSpeed(Ogre::Real walkSpeed)
    {
        m_walkSpeed = walkSpeed;
    }

    Ogre::Real PlayerControllerBody::getWalkSpeed() const
    {
        return m_walkSpeed;
    }

    void PlayerControllerBody::setJumpSpeed(Ogre::Real jumpSpeed)
    { 
        m_jumpSpeed = jumpSpeed;
    }

    Ogre::Real PlayerControllerBody::getJumpSpeed() const
    {
        return m_jumpSpeed;
    }

    void PlayerControllerBody::setCanJump(bool canJump)
    {
        m_canJump = canJump;
    }

    bool PlayerControllerBody::getCanJump() const
    {
        return m_canJump;
    }

    Ogre::Real PlayerControllerBody::getForwardSpeed() const
    {
        return m_forwardSpeed;
    }

    Ogre::Real PlayerControllerBody::getSideSpeed() const
    { 
        return m_sideSpeed;
    }

    Ogre::Radian PlayerControllerBody::getHeading() const
    {
        return m_heading;
    }

    void PlayerControllerBody::setStartOrientation(const Ogre::Quaternion& startOrientation)
    {
        m_startOrientation = startOrientation;
    }

    Ogre::Quaternion PlayerControllerBody::getStartOrientation() const
    {
        return m_startOrientation;
    }

    PlayerCallback* PlayerControllerBody::getPlayerCallback() const
    {
        return m_playerCallback;
    }

} // namespace OgreNewt
