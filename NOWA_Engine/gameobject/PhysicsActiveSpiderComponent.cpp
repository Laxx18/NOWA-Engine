#include "NOWAPrecompiled.h"
#include "PhysicsActiveSpiderComponent.h"
#include "LuaScriptComponent.h"
#include "PhysicsComponent.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    // PhysicsSpiderCallback
    // =========================================================================
    PhysicsActiveSpiderComponent::PhysicsSpiderCallback::PhysicsSpiderCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onMovementChangedFunctionName) :
        OgreNewt::SpiderCallback(),
        m_owner(owner),
        m_luaScript(luaScript),
        m_ogreNewt(ogreNewt),
        m_onMovementChangedFunctionName(onMovementChangedFunctionName)
    {
        if (!m_luaScript)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] GameObject '" + owner->getName() + "' has no LuaScriptComponent. Spider cannot be controlled via Lua.");
        }
    }

    PhysicsActiveSpiderComponent::PhysicsSpiderCallback::~PhysicsSpiderCallback()
    {
    }

    void PhysicsActiveSpiderComponent::PhysicsSpiderCallback::onMovementChanged(OgreNewt::SpiderMovementManipulation* manip, Ogre::Real dt)
    {
        if (m_luaScript && m_luaScript->isCompiled() && !m_onMovementChangedFunctionName.empty())
        {
            m_luaScript->callTableFunction(m_onMovementChangedFunctionName, manip, dt);
        }
    }

    // =========================================================================
    // Constructor / Destructor
    // =========================================================================
    PhysicsActiveSpiderComponent::PhysicsActiveSpiderComponent() :
        PhysicsActiveComponent(),
        m_onMovementChangedFunctionName(new Variant(PhysicsActiveSpiderComponent::AttrOnMovementChangedFunctionName(), Ogre::String(""), this->attributes)),
        m_walkCycleDuration(new Variant(PhysicsActiveSpiderComponent::AttrWalkCycleDuration(), Ogre::Real(2.0f), this->attributes)),
        m_stepHeight(new Variant(PhysicsActiveSpiderComponent::AttrStepHeight(), Ogre::Real(0.2f), this->attributes)),
        m_gaitSequence(new Variant(PhysicsActiveSpiderComponent::AttrGaitSequence(), Ogre::String("3,1,2,0"), this->attributes)),
        m_legBoneAxis(new Variant(PhysicsActiveSpiderComponent::AttrLegBoneAxis(), std::vector<Ogre::String>({"Y", "X", "Z"}), this->attributes))
    {
        for (int i = 0; i < MAX_LEGS; ++i)
        {
            m_thighIds[i] = new Variant(PhysicsActiveSpiderComponent::AttrThighId(i), static_cast<unsigned long>(0UL), this->attributes);
            m_thighIds[i]->setDescription("Thigh (upper leg) GameObject ID for leg " + Ogre::StringConverter::toString(i) + ". 0 = unused.");

            m_calfIds[i] = new Variant(PhysicsActiveSpiderComponent::AttrCalfId(i), static_cast<unsigned long>(0UL), this->attributes);
            m_calfIds[i]->setDescription("Calf (lower leg) GameObject ID for leg " + Ogre::StringConverter::toString(i) + ". 0 = unused.");

            m_heelIds[i] = new Variant(PhysicsActiveSpiderComponent::AttrHeelId(i), static_cast<unsigned long>(0UL), this->attributes);
            m_heelIds[i]->setDescription("Heel (ankle/foot) GameObject ID for leg " + Ogre::StringConverter::toString(i) + ". 0 = unused.");
        }

        this->asSoftBody->setVisible(false);

        // Sensible defaults for a small spider-like creature.
        this->mass->setValue(30.0f);
        this->massOrigin->setValue(Ogre::Vector3(0.0f, -0.1f, 0.0f));
        this->collisionSize->setValue(Ogre::Vector3::ZERO); // ZERO = use mesh extents
        this->collisionPosition->setValue(Ogre::Vector3::ZERO);
        this->collisionType->setListSelectedValue("ConvexHull");

        // Low pitch/roll damping; light yaw so turning impulses work.
        this->angularDamping->setValue(Ogre::Vector3(0.7f, 0.05f, 0.7f));
        this->linearDamping->setValue(0.1f);

        m_walkCycleDuration->setDescription("Duration of one full walk cycle in seconds (default 2.0). "
                                            "Shorter = faster steps.");

        m_stepHeight->setDescription("How high each foot arcs during the swing phase (default 0.2 m). "
                                     "Increase for larger creatures or rough terrain.");

        m_gaitSequence->setDescription("Comma-separated phase sequence for each leg (e.g. \"3,1,2,0\" for "
                                       "diagonal 4-legged walk). Length must match active leg count. "
                                       "Defaults: 4 legs = 3,1,2,0;  2 legs = 1,0.");

        m_legBoneAxis->setDescription("Local axis along which each limb mesh extends (default Y). "
                                      "Y: bone points along +Y – Blender/GLTF default for limbs. "
                                      "X: bone points along +X. "
                                      "Z: bone points along +Z. "
                                      "Must match how the artist modelled the leg segments.");
        m_legBoneAxis->setListSelectedValue("Y");

        m_onMovementChangedFunctionName->setDescription("Lua callback: onMovementChanged(manip, dt). "
                                                        "Call manip:setStride(f) [0..0.5] to walk, "
                                                        "manip:setOmega(f) [±0.25] to turn.");
        m_onMovementChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), m_onMovementChangedFunctionName->getString() + "(manip, dt)");
    }

    PhysicsActiveSpiderComponent::~PhysicsActiveSpiderComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] Destructor for: " + this->gameObjectPtr->getName());
    }

    // =========================================================================
    // init  (XML load)
    // =========================================================================
    bool PhysicsActiveSpiderComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        PhysicsActiveComponent::parseCommonProperties(propertyElement);

        for (int i = 0; i < MAX_LEGS; ++i)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrThighId(i))
            {
                this->setThighId(i, XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCalfId(i))
            {
                this->setCalfId(i, XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHeelId(i))
            {
                this->setHeelId(i, XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOnMovementChangedFunctionName())
        {
            this->setOnMovementChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWalkCycleDuration())
        {
            this->setWalkCycleDuration(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrStepHeight())
        {
            this->setStepHeight(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGaitSequence())
        {
            this->setGaitSequence(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLegBoneAxis())
        {
            this->setLegBoneAxis(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    // =========================================================================
    // postInit
    // =========================================================================
    bool PhysicsActiveSpiderComponent::postInit(void)
    {
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] postInit for: " + this->gameObjectPtr->getName());

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        return success;
    }

    // =========================================================================
    // onRemoveComponent
    // =========================================================================
    void PhysicsActiveSpiderComponent::onRemoveComponent(void)
    {
        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->clearLegs();
        }
    }

    // =========================================================================
    // connect  (simulation start)
    // =========================================================================
    bool PhysicsActiveSpiderComponent::connect(void)
    {
        bool success = PhysicsActiveComponent::connect();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] connect for: " + this->gameObjectPtr->getName());

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        if (!this->createDynamicBody())
        {
            return false;
        }

        this->registerLegChains();
        this->setCanMove(this->activated->getBool());

        if (this->physicsBody && this->bShowDebugData)
        {
            ENQUEUE_RENDER_COMMAND_WAIT("PhysicsActiveSpiderComponent::showDebug", { this->physicsBody->showDebugCollision(false, this->bShowDebugData); });
        }

        return success;
    }

    // =========================================================================
    // disconnect  (simulation stop)
    // =========================================================================
    bool PhysicsActiveSpiderComponent::disconnect(void)
    {
        bool success = PhysicsActiveComponent::disconnect();

        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->clearLegs();
            delete this->physicsBody;
            this->physicsBody = nullptr;
        }

        return success;
    }

    // =========================================================================
    // update  (logic thread, every frame)
    // =========================================================================
    void PhysicsActiveSpiderComponent::update(Ogre::Real dt, bool notSimulating)
    {
        PhysicsActiveComponent::update(dt, notSimulating);

        if (!notSimulating && this->physicsBody)
        {
            this->updateLegNodes(dt);
        }
    }

    // =========================================================================
    // createDynamicBody
    // =========================================================================
    bool PhysicsActiveSpiderComponent::createDynamicBody(void)
    {
        Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
        Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;

        if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
        {
            collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
        }

        Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

        OgreNewt::CollisionPtr collisionPtr;

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveSpiderComponent::createCollision", _4(&inertia, &collisionPtr, collisionOrientation, &calculatedMassOrigin),
            { collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId()); });

        if (!collisionPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Collision is null for: " + this->gameObjectPtr->getName());
            return false;
        }

        if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
        {
            calculatedMassOrigin = this->massOrigin->getVector3();
        }

        LuaScript* luaScript = this->gameObjectPtr->getLuaScript();

        this->physicsBody = new OgreNewt::SpiderBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr, this->mass->getReal(), calculatedMassOrigin, this->gravity->getVector3(), this->gameObjectPtr->getDefaultDirection(),
            new PhysicsSpiderCallback(this->gameObjectPtr.get(), luaScript, this->ogreNewt, m_onMovementChangedFunctionName->getString()));

        OgreNewt::SpiderBody* spider = static_cast<OgreNewt::SpiderBody*>(this->physicsBody);

        spider->setWalkCycleDuration(m_walkCycleDuration->getReal());
        spider->setStepHeight(m_stepHeight->getReal());

        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

        this->physicsBody->setGravity(this->gravity->getVector3());

        if (this->linearDamping->getReal() != 0.0f)
        {
            this->physicsBody->setLinearDamping(this->linearDamping->getReal());
        }
        if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
        }

        this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveSpiderComponent>(&PhysicsActiveSpiderComponent::spiderMoveCallback, this);

        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        this->setPosition(this->initialPosition);
        this->setOrientation(this->initialOrientation);

        this->setConstraintAxis(this->constraintAxis->getVector3());
        this->setConstraintDirection(this->constraintDirection->getVector3());
        this->setCollidable(this->collidable->getBool());

        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        this->setActivated(this->activated->getBool());
        this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

        return true;
    }

    // =========================================================================
    // spiderMoveCallback  (Newton worker thread)
    // =========================================================================
    void PhysicsActiveSpiderComponent::spiderMoveCallback(OgreNewt::Body* body, Ogre::Real dt, int threadIndex)
    {
        // 1. Base: gravity, speed limits, constraint axes
        PhysicsActiveComponent::moveCallback(body, dt, threadIndex);

        // 2. Locomotion impulses (forward drive, lateral grip, yaw)
        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->applyLocomotionImpulses(dt);
        }
    }

    // =========================================================================
    // registerLegChains  (called once in connect)
    // =========================================================================
    void PhysicsActiveSpiderComponent::registerLegChains(void)
    {
        if (!this->physicsBody)
        {
            return;
        }

        OgreNewt::SpiderBody* spider = static_cast<OgreNewt::SpiderBody*>(this->physicsBody);
        spider->clearLegs();

        const Ogre::Vector3 chassisWorldPos = this->physicsBody->getPosition();
        const Ogre::Quaternion chassisWorldOrient = this->physicsBody->getOrientation();
        const Ogre::Quaternion chassisWorldInv = chassisWorldOrient.Inverse();

        const int boneAxis = boneAxisFromString(m_legBoneAxis->getString());

        int activeLegCount = 0;

        for (int i = 0; i < MAX_LEGS; ++i)
        {
            const unsigned long thighId = m_thighIds[i]->getULong();
            const unsigned long calfId = m_calfIds[i]->getULong();
            const unsigned long heelId = m_heelIds[i]->getULong();

            if (thighId == 0UL || calfId == 0UL || heelId == 0UL)
            {
                continue; // slot unused
            }

            auto* ctrl = AppStateManager::getSingletonPtr()->getGameObjectController();
            GameObjectPtr thighGO = ctrl->getGameObjectFromId(thighId);
            GameObjectPtr calfGO = ctrl->getGameObjectFromId(calfId);
            GameObjectPtr heelGO = ctrl->getGameObjectFromId(heelId);

            if (!thighGO || !calfGO || !heelGO)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Leg " + Ogre::StringConverter::toString(i) + ": one or more GameObjects not found. Skipping.");
                continue;
            }

            Ogre::SceneNode* thighNode = thighGO->getSceneNode();
            Ogre::SceneNode* calfNode = calfGO->getSceneNode();
            Ogre::SceneNode* heelNode = heelGO->getSceneNode();

            if (!thighNode || !calfNode || !heelNode)
            {
                continue;
            }

            // ── Bind-pose world positions ─────────────────────────────────────
            const Ogre::Vector3 thighWorldPos = thighNode->_getDerivedPosition();
            const Ogre::Vector3 calfWorldPos = calfNode->_getDerivedPosition();
            const Ogre::Vector3 heelWorldPos = heelNode->_getDerivedPosition();

            // ── Chassis-local positions ───────────────────────────────────────
            const Ogre::Vector3 hipLocalPos = chassisWorldInv * (thighWorldPos - chassisWorldPos);
            const Ogre::Vector3 footRestLocal = chassisWorldInv * (heelWorldPos - chassisWorldPos);

            // ── Bone lengths ──────────────────────────────────────────────────
            const Ogre::Real thighLength = (calfWorldPos - thighWorldPos).length();
            const Ogre::Real calfLength = (heelWorldPos - calfWorldPos).length();

            if (thighLength < 0.01f || calfLength < 0.01f)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Leg " + Ogre::StringConverter::toString(i) + ": degenerate bone length. Check node placement. Skipping.");
                continue;
            }

            // ── Swivel sign: positive = knee bends away from body X axis ──────
            // The lateral axis in chassis space is perpendicular to both the
            // default forward direction and world up.
            // For a symmetric 4-legged spider with legs along ±Z, the sign
            // follows the Z component of the hip local position.
            const Ogre::Real swivelSign = (hipLocalPos.dotProduct(chassisWorldInv * Ogre::Vector3::UNIT_Z) >= 0.0f) ? 1.0f : -1.0f;

            spider->addLegChain(thighNode, calfNode, heelNode, hipLocalPos, footRestLocal, thighLength, calfLength, swivelSign, boneAxis);

            ++activeLegCount;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] Registered leg " + Ogre::StringConverter::toString(i) + " thigh='" + thighGO->getName() +
                                                                                   "'"
                                                                                   " calf='" +
                                                                                   calfGO->getName() +
                                                                                   "'"
                                                                                   " heel='" +
                                                                                   heelGO->getName() +
                                                                                   "'"
                                                                                   " hipLocal=" +
                                                                                   Ogre::StringConverter::toString(hipLocalPos) + " thighLen=" + Ogre::StringConverter::toString(thighLength) +
                                                                                   " calfLen=" + Ogre::StringConverter::toString(calfLength));
        }

        // ── Apply gait sequence ───────────────────────────────────────────────
        if (activeLegCount > 0)
        {
            const std::vector<int> seq = parseGaitSequence(m_gaitSequence->getString(), activeLegCount);
            spider->setGaitSequence(seq);
        }
    }

    // =========================================================================
    // updateLegNodes  (logic thread, every frame)
    // =========================================================================
    void PhysicsActiveSpiderComponent::updateLegNodes(Ogre::Real dt)
    {
        OgreNewt::SpiderBody* spider = static_cast<OgreNewt::SpiderBody*>(this->physicsBody);

        if (!spider || spider->getLegChains().empty())
        {
            return;
        }

        // ── MAIN THREAD: query Lua, cache stride/omega for the Newton thread ──
        spider->updateMovementInput(dt);

        // ── MAIN THREAD: gait + IK, writes world transforms ──────────────────
        spider->computeLegTransforms(dt);

        // Push all segment transforms into GraphicsModule's lock-free buffer.
        for (const OgreNewt::LegChainInfo& leg : spider->getLegChains())
        {
            if (leg.thigh)
            {
                GraphicsModule::getInstance()->updateNodeTransform(leg.thigh, leg.thighWorldPos, leg.thighWorldOrient);
            }
            if (leg.calf)
            {
                GraphicsModule::getInstance()->updateNodeTransform(leg.calf, leg.kneeWorldPos, leg.calfWorldOrient);
            }
            if (leg.heel)
            {
                GraphicsModule::getInstance()->updateNodeTransform(leg.heel, leg.footWorldPos, leg.heelWorldOrient);
            }
        }
    }

    // =========================================================================
    // Gait sequence parser
    // =========================================================================
    std::vector<int> PhysicsActiveSpiderComponent::parseGaitSequence(const Ogre::String& str, int legCount)
    {
        std::vector<int> result;

        if (!str.empty())
        {
            Ogre::StringVector tokens = Ogre::StringUtil::split(str, ",");
            for (const Ogre::String& tok : tokens)
            {
                Ogre::String trimmed = tok;
                Ogre::StringUtil::trim(trimmed);
                if (!trimmed.empty())
                {
                    result.push_back(Ogre::StringConverter::parseInt(trimmed));
                }
            }
        }

        // Validate: must have exactly legCount entries, all in [0, legCount-1].
        bool valid = (static_cast<int>(result.size()) == legCount);
        if (valid)
        {
            for (int v : result)
            {
                if (v < 0 || v >= legCount)
                {
                    valid = false;
                    break;
                }
            }
        }

        if (!valid)
        {
            result.clear();
            // Sensible defaults per leg count.
            switch (legCount)
            {
            case 2:
                result = {1, 0};
                break;
            case 4:
                result = {3, 1, 2, 0};
                break;
            case 6:
                result = {5, 2, 4, 1, 3, 0};
                break;
            default:
                // Generic: reverse order
                for (int i = legCount - 1; i >= 0; --i)
                {
                    result.push_back(i);
                }
                break;
            }
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] GaitSequence '" + str + "' is invalid for " + Ogre::StringConverter::toString(legCount) + " legs; using default.");
        }

        return result;
    }

    int PhysicsActiveSpiderComponent::boneAxisFromString(const Ogre::String& axis)
    {
        if (axis == "X" || axis == "x")
        {
            return 0;
        }
        if (axis == "Z" || axis == "z")
        {
            return 2;
        }
        return 1; // default Y
    }

    // =========================================================================
    // setActivated
    // =========================================================================
    void PhysicsActiveSpiderComponent::setActivated(bool activated)
    {
        PhysicsActiveComponent::setActivated(activated);
        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setCanMove(activated);
        }
    }

    // =========================================================================
    // Attribute setters / getters
    // =========================================================================
    void PhysicsActiveSpiderComponent::setThighId(int i, unsigned long id)
    {
        if (i >= 0 && i < MAX_LEGS)
        {
            m_thighIds[i]->setValue(id);
        }
    }
    unsigned long PhysicsActiveSpiderComponent::getThighId(int i) const
    {
        return (i >= 0 && i < MAX_LEGS) ? m_thighIds[i]->getULong() : 0UL;
    }

    void PhysicsActiveSpiderComponent::setCalfId(int i, unsigned long id)
    {
        if (i >= 0 && i < MAX_LEGS)
        {
            m_calfIds[i]->setValue(id);
        }
    }
    unsigned long PhysicsActiveSpiderComponent::getCalfId(int i) const
    {
        return (i >= 0 && i < MAX_LEGS) ? m_calfIds[i]->getULong() : 0UL;
    }

    void PhysicsActiveSpiderComponent::setHeelId(int i, unsigned long id)
    {
        if (i >= 0 && i < MAX_LEGS)
        {
            m_heelIds[i]->setValue(id);
        }
    }
    unsigned long PhysicsActiveSpiderComponent::getHeelId(int i) const
    {
        return (i >= 0 && i < MAX_LEGS) ? m_heelIds[i]->getULong() : 0UL;
    }

    void PhysicsActiveSpiderComponent::setOnMovementChangedFunctionName(const Ogre::String& name)
    {
        m_onMovementChangedFunctionName->setValue(name);
        m_onMovementChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), name + "(manip, dt)");
    }
    Ogre::String PhysicsActiveSpiderComponent::getOnMovementChangedFunctionName() const
    {
        return m_onMovementChangedFunctionName->getString();
    }

    void PhysicsActiveSpiderComponent::setWalkCycleDuration(Ogre::Real duration)
    {
        m_walkCycleDuration->setValue(duration);
        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setWalkCycleDuration(duration);
        }
    }
    Ogre::Real PhysicsActiveSpiderComponent::getWalkCycleDuration() const
    {
        return m_walkCycleDuration->getReal();
    }

    void PhysicsActiveSpiderComponent::setStepHeight(Ogre::Real height)
    {
        m_stepHeight->setValue(height);
        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setStepHeight(height);
        }
    }
    Ogre::Real PhysicsActiveSpiderComponent::getStepHeight() const
    {
        return m_stepHeight->getReal();
    }

    void PhysicsActiveSpiderComponent::setGaitSequence(const Ogre::String& sequence)
    {
        m_gaitSequence->setValue(sequence);
    }
    Ogre::String PhysicsActiveSpiderComponent::getGaitSequence() const
    {
        return m_gaitSequence->getString();
    }

    void PhysicsActiveSpiderComponent::setLegBoneAxis(const Ogre::String& axis)
    {
        m_legBoneAxis->setListSelectedValue(axis);
    }
    Ogre::String PhysicsActiveSpiderComponent::getLegBoneAxis() const
    {
        return m_legBoneAxis->getString();
    }

    void PhysicsActiveSpiderComponent::setCanMove(bool canMove)
    {
        if (this->physicsBody)
        {
            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setCanMove(canMove);
        }
    }

    bool PhysicsActiveSpiderComponent::isOnGround() const
    {
        if (!this->physicsBody)
        {
            return false;
        }
        return static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->isOnGround();
    }

    OgreNewt::SpiderBody* PhysicsActiveSpiderComponent::getSpiderBody() const
    {
        return static_cast<OgreNewt::SpiderBody*>(this->physicsBody);
    }

    // =========================================================================
    // actualizeValue  (live editor attribute changes)
    // =========================================================================
    void PhysicsActiveSpiderComponent::actualizeValue(Variant* attribute)
    {
        PhysicsActiveComponent::actualizeCommonValue(attribute);

        for (int i = 0; i < MAX_LEGS; ++i)
        {
            if (attribute->getName() == AttrThighId(i))
            {
                this->setThighId(i, attribute->getULong());
            }
            else if (attribute->getName() == AttrCalfId(i))
            {
                this->setCalfId(i, attribute->getULong());
            }
            else if (attribute->getName() == AttrHeelId(i))
            {
                this->setHeelId(i, attribute->getULong());
            }
        }

        if (attribute->getName() == AttrOnMovementChangedFunctionName())
        {
            this->setOnMovementChangedFunctionName(attribute->getString());
        }
        else if (attribute->getName() == AttrWalkCycleDuration())
        {
            this->setWalkCycleDuration(attribute->getReal());
        }
        else if (attribute->getName() == AttrStepHeight())
        {
            this->setStepHeight(attribute->getReal());
        }
        else if (attribute->getName() == AttrGaitSequence())
        {
            this->setGaitSequence(attribute->getString());
        }
        else if (attribute->getName() == AttrLegBoneAxis())
        {
            this->setLegBoneAxis(attribute->getString());
        }
    }

    // =========================================================================
    // writeXML
    // =========================================================================
    void PhysicsActiveSpiderComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

        xml_node<>* propertyXML = nullptr;

        for (int i = 0; i < MAX_LEGS; ++i)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // ULong
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrThighId(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_thighIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // ULong
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrCalfId(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_calfIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // ULong
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrHeelId(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_heelIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrOnMovementChangedFunctionName())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_onMovementChangedFunctionName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrWalkCycleDuration())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_walkCycleDuration->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrStepHeight())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_stepHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrGaitSequence())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_gaitSequence->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String (list selection)
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, PhysicsActiveSpiderComponent::AttrLegBoneAxis())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, m_legBoneAxis->getString())));
        propertiesXML->append_node(propertyXML);
    }

    // =========================================================================
    // getClassName / getParentClassName
    // =========================================================================
    Ogre::String PhysicsActiveSpiderComponent::getClassName() const
    {
        return "PhysicsActiveSpiderComponent";
    }

    Ogre::String PhysicsActiveSpiderComponent::getParentClassName() const
    {
        return "PhysicsActiveComponent";
    }

    Ogre::String PhysicsActiveSpiderComponent::getParentParentClassName() const
    {
        return "PhysicsComponent";
    }

    // =========================================================================
    // clone
    // =========================================================================
    GameObjectCompPtr PhysicsActiveSpiderComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        auto cloned = boost::make_shared<PhysicsActiveSpiderComponent>();

        for (int i = 0; i < MAX_LEGS; ++i)
        {
            cloned->setThighId(i, this->getThighId(i));
            cloned->setCalfId(i, this->getCalfId(i));
            cloned->setHeelId(i, this->getHeelId(i));
        }

        cloned->setOnMovementChangedFunctionName(m_onMovementChangedFunctionName->getString());
        cloned->setWalkCycleDuration(m_walkCycleDuration->getReal());
        cloned->setStepHeight(m_stepHeight->getReal());
        cloned->setGaitSequence(m_gaitSequence->getString());
        cloned->setLegBoneAxis(m_legBoneAxis->getString());

        cloned->setOwner(clonedGameObjectPtr);
        return cloned;
    }

    // =========================================================================
    // createStaticApiForLua  (expose manip to Lua)
    // =========================================================================

    PhysicsActiveSpiderComponent* getPhysicsActiveSpiderComponent(GameObject* gameObject)
    {
        return makeStrongPtr<PhysicsActiveSpiderComponent>(gameObject->getComponent<PhysicsActiveSpiderComponent>()).get();
    }

    PhysicsActiveSpiderComponent* getPhysicsActiveSpiderComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<PhysicsActiveSpiderComponent>(gameObject->getComponentWithOccurrence<PhysicsActiveSpiderComponent>(occurrenceIndex)).get();
    }

    PhysicsActiveSpiderComponent* getPhysicsActiveSpiderComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<PhysicsActiveSpiderComponent>(gameObject->getComponentFromName<PhysicsActiveSpiderComponent>(name)).get();
    }

    void PhysicsActiveSpiderComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        using namespace luabind;

        module(lua)
        [
            class_<OgreNewt::SpiderMovementManipulation>("SpiderMovementManipulation")
            .def("setStride", &OgreNewt::SpiderMovementManipulation::setStride)
            .def("getStride", &OgreNewt::SpiderMovementManipulation::getStride)
            .def("setOmega", &OgreNewt::SpiderMovementManipulation::setOmega)
            .def("getOmega", &OgreNewt::SpiderMovementManipulation::getOmega)
            .def("setBodySwayX", &OgreNewt::SpiderMovementManipulation::setBodySwayX)
            .def("getBodySwayX", &OgreNewt::SpiderMovementManipulation::getBodySwayX)
            .def("setBodySwayZ", &OgreNewt::SpiderMovementManipulation::setBodySwayZ)
            .def("getBodySwayZ", &OgreNewt::SpiderMovementManipulation::getBodySwayZ),

            class_<PhysicsActiveSpiderComponent, bases<PhysicsActiveComponent>>("PhysicsActiveSpiderComponent")
            .def("setThighId", &PhysicsActiveSpiderComponent::setThighId)
            .def("getThighId", &PhysicsActiveSpiderComponent::getThighId)
            .def("setCalfId", &PhysicsActiveSpiderComponent::setCalfId)
            .def("getCalfId", &PhysicsActiveSpiderComponent::getCalfId)
            .def("setHeelId", &PhysicsActiveSpiderComponent::setHeelId)
            .def("getHeelId", &PhysicsActiveSpiderComponent::getHeelId)
            .def("setWalkCycleDuration", &PhysicsActiveSpiderComponent::setWalkCycleDuration)
            .def("getWalkCycleDuration", &PhysicsActiveSpiderComponent::getWalkCycleDuration)
            .def("setStepHeight", &PhysicsActiveSpiderComponent::setStepHeight)
            .def("getStepHeight", &PhysicsActiveSpiderComponent::getStepHeight)
            .def("setCanMove", &PhysicsActiveSpiderComponent::setCanMove)
            .def("isOnGround", &PhysicsActiveSpiderComponent::isOnGround)
        ];

        gameObjectClass.def("getPhysicsActiveSpiderComponent", (PhysicsActiveSpiderComponent * (*)(GameObject*)) & getPhysicsActiveSpiderComponent);
        gameObjectClass.def("getPhysicsActiveSpiderComponentFromIndex", (PhysicsActiveSpiderComponent * (*)(GameObject*, unsigned int)) & getPhysicsActiveSpiderComponentFromIndex);
        gameObjectClass.def("getPhysicsActiveSpiderComponentFromName", &getPhysicsActiveSpiderComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "class inherits PhysicsActiveComponent", "Procedural IK spider. One physics torso body; legs are purely visual SceneNodes driven by gait + 2-link IK.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setCanMove(boolean canMove)", "Enables or disables spider locomotion without removing the body.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "boolean isOnGround()", "Returns true if at least one active contact exists on the torso body.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setWalkCycleDuration(number seconds)", "Sets duration of one full gait cycle in seconds (default 2.0).");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "number getWalkCycleDuration()", "Gets the current walk cycle duration.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setStepHeight(number height)", "Sets how high each foot arcs during the swing phase (default 0.2).");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "number getStepHeight()", "Gets the current step height.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setThighId(int index, ulong id)", "Sets the thigh (upper leg) GameObject ID for leg slot 0..3.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "ulong getThighId(int index)", "Gets the thigh GameObject ID for leg slot 0..3.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setCalfId(int index, ulong id)", "Sets the calf (lower leg) GameObject ID for leg slot 0..3.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "ulong getCalfId(int index)", "Gets the calf GameObject ID for leg slot 0..3.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setHeelId(int index, ulong id)", "Sets the heel (ankle/foot) GameObject ID for leg slot 0..3.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "ulong getHeelId(int index)", "Gets the heel GameObject ID for leg slot 0..3.");

        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "SpiderMovementManipulation", "Passed into onMovementChanged to control spider locomotion.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setStride(number stride)", "Sets forward stride per step. 0 = stand still. Typical range: 0.0..0.5.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "number getStride()", "Gets the current stride value.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setOmega(number omega)", "Sets yaw rate in rad/s. Positive = right, negative = left. Typical: ±0.25..±0.4.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "number getOmega()", "Gets the current yaw rate.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setBodySwayX(number sway)", "Sets lateral body sway (cosmetic). Scales with omega for natural lean-into-turn feel.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "number getBodySwayX()", "Gets the current lateral body sway.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setBodySwayZ(number sway)", "Sets forward body sway (cosmetic). Scales with stride for a subtle forward lean.");
        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "number getBodySwayZ()", "Gets the current forward body sway.");

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveSpiderComponent getPhysicsActiveSpiderComponent()", "Gets the PhysicsActiveSpiderComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveSpiderComponent getPhysicsActiveSpiderComponentFromIndex(unsigned int occurrenceIndex)", "Gets the PhysicsActiveSpiderComponent by occurrence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveSpiderComponent getPhysicsActiveSpiderComponentFromName(String name)", "Gets the PhysicsActiveSpiderComponent by name.");

        gameObjectControllerClass.def("castPhysicsActiveSpiderComponent", &GameObjectController::cast<PhysicsActiveSpiderComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PhysicsActiveSpiderComponent castPhysicsActiveSpiderComponent(PhysicsActiveSpiderComponent other)", "Casts an incoming type from a function for Lua auto completion.");
    }

} // namespace NOWA
