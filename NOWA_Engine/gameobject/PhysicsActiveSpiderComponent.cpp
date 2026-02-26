#include "NOWAPrecompiled.h"
//#include "PhysicsActiveSpiderComponent.h"
//#include "LuaScriptComponent.h"
//#include "PhysicsComponent.h"
//#include "gameobject/GameObjectController.h"
//#include "main/AppStateManager.h"
//#include "main/Core.h"
//#include "modules/LuaScriptApi.h"
//#include "utilities/MathHelper.h"
//#include "utilities/XMLConverter.h"
//
//#include "OgreMesh2.h"
//#include "OgreMeshManager2.h"
//#include "OgreSubMesh2.h"
//
//namespace NOWA
//{
//    using namespace rapidxml;
//    using namespace luabind;
//
//    // =========================================================================
//    // PhysicsSpiderCallback
//    // =========================================================================
//    PhysicsActiveSpiderComponent::PhysicsSpiderCallback::PhysicsSpiderCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onMovementChangedFunctionName) :
//        OgreNewt::SpiderCallback(),
//        m_owner(owner),
//        m_luaScript(luaScript),
//        m_ogreNewt(ogreNewt),
//        m_onMovementChangedFunctionName(onMovementChangedFunctionName)
//    {
//        if (!m_luaScript)
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] GameObject '" + owner->getName() + "' has no LuaScriptComponent. Spider cannot be controlled via Lua.");
//        }
//    }
//
//    PhysicsActiveSpiderComponent::PhysicsSpiderCallback::~PhysicsSpiderCallback()
//    {
//    }
//
//    void PhysicsActiveSpiderComponent::PhysicsSpiderCallback::onMovementChanged(OgreNewt::SpiderMovementManipulation* manip, Ogre::Real dt)
//    {
//        if (m_luaScript && m_luaScript->isCompiled() && !m_onMovementChangedFunctionName.empty())
//        {
//            m_luaScript->callTableFunction(m_onMovementChangedFunctionName, manip, dt);
//        }
//    }
//
//    // =========================================================================
//    // Default per-leg positions for a 4-legged spider facing +X
//    // =========================================================================
//    static const Ogre::Vector3 k_defaultHipPos[PhysicsActiveSpiderComponent::MAX_LEGS] = {
//        Ogre::Vector3(0.3f, 0.0f, -0.3f),  // 0 Front-Left
//        Ogre::Vector3(0.3f, 0.0f, 0.3f),   // 1 Front-Right
//        Ogre::Vector3(-0.3f, 0.0f, -0.3f), // 2 Rear-Left
//        Ogre::Vector3(-0.3f, 0.0f, 0.3f)   // 3 Rear-Right
//    };
//
//    static const Ogre::Vector3 k_defaultFootRestPos[PhysicsActiveSpiderComponent::MAX_LEGS] = {
//        Ogre::Vector3(0.5f, -0.4f, -0.6f),
//        Ogre::Vector3(0.5f, -0.4f, 0.6f),
//        Ogre::Vector3(-0.5f, -0.4f, -0.6f),
//        Ogre::Vector3(-0.5f, -0.4f, 0.6f)
//    };
//
//    // =========================================================================
//    // Constructor
//    // =========================================================================
//    PhysicsActiveSpiderComponent::PhysicsActiveSpiderComponent() :
//        PhysicsActiveComponent(),
//        m_thighMeshName(new Variant(PhysicsActiveSpiderComponent::AttrThighMeshName(), Ogre::String(""), this->attributes)),
//        m_calfMeshName(new Variant(PhysicsActiveSpiderComponent::AttrCalfMeshName(), Ogre::String(""), this->attributes)),
//        m_heelMeshName(new Variant(PhysicsActiveSpiderComponent::AttrHeelMeshName(), Ogre::String(""), this->attributes)),
//        m_thighResourceGroup(Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME),
//        m_calfResourceGroup(Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME),
//        m_heelResourceGroup(Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME),
//        m_onMovementChangedFunctionName(new Variant(PhysicsActiveSpiderComponent::AttrOnMovementChangedFunctionName(), Ogre::String(""), this->attributes)),
//        m_walkCycleDuration(new Variant(PhysicsActiveSpiderComponent::AttrWalkCycleDuration(), Ogre::Real(2.0f), this->attributes)),
//        m_stepHeight(new Variant(PhysicsActiveSpiderComponent::AttrStepHeight(), Ogre::Real(0.2f), this->attributes)),
//        m_gaitSequence(new Variant(PhysicsActiveSpiderComponent::AttrGaitSequence(), Ogre::String("3,1,2,0"), this->attributes)),
//        m_legBoneAxis(new Variant(PhysicsActiveSpiderComponent::AttrLegBoneAxis(), std::vector<Ogre::String>({"Y", "X", "Z"}), this->attributes)),
//        m_legMeshScale(new Variant(PhysicsActiveSpiderComponent::AttrLegMeshScale(), Ogre::Real(1.0f), this->attributes)),
//        m_thighBoneLength(new Variant(PhysicsActiveSpiderComponent::AttrThighBoneLength(), Ogre::Real(0.0f), this->attributes)),
//        m_calfBoneLength(new Variant(PhysicsActiveSpiderComponent::AttrCalfBoneLength(), Ogre::Real(0.0f), this->attributes))
//    {
//        // Mesh file-open dialogs (same pattern as ProceduralFoliageVolume)
//        m_thighMeshName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
//        m_thighMeshName->setDescription("Mesh for all 4 upper-leg (thigh) segments. "
//                                        "Mesh origin MUST be at the HIP joint. "
//                                        "Mesh must extend along LegBoneAxis (default Y). "
//                                        "Bone length derived automatically from the mesh AABB.");
//
//        m_calfMeshName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
//        m_calfMeshName->setDescription("Mesh for all 4 lower-leg (calf) segments. "
//                                       "Mesh origin MUST be at the KNEE joint.");
//
//        m_heelMeshName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
//        m_heelMeshName->setDescription("Mesh for all 4 foot (heel) segments. "
//                                       "Mesh origin MUST be at the ANKLE joint.");
//
//        // Per-leg placement Variants
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            m_legHipPos[i] = new Variant(PhysicsActiveSpiderComponent::AttrLegHipPos(i), k_defaultHipPos[i], this->attributes);
//            m_legHipPos[i]->setDescription("Hip socket position for leg " + Ogre::StringConverter::toString(i) + " in TORSO-LOCAL space.  0=FL 1=FR 2=RL 3=RR (spider facing +X).");
//
//            m_legFootRestPos[i] = new Variant(PhysicsActiveSpiderComponent::AttrLegFootRestPos(i), k_defaultFootRestPos[i], this->attributes);
//            m_legFootRestPos[i]->setDescription("Neutral foot position for leg " + Ogre::StringConverter::toString(i) + " in TORSO-LOCAL space.  Y must be negative (below torso).");
//        }
//
//        m_walkCycleDuration->setDescription("Full gait cycle duration in seconds (default 2.0). Shorter = faster steps.");
//        m_stepHeight->setDescription("Foot arc height during swing (default 0.2 m).");
//        m_gaitSequence->setDescription("Comma-separated phase order.  4-legs: '3,1,2,0'  2-legs: '1,0'  6-legs: '5,2,4,1,3,0'.");
//        m_legBoneAxis->setDescription("Axis along which each mesh bone extends. Y = Blender/GLTF default.");
//        m_legBoneAxis->setListSelectedValue("Y");
//
//        m_legMeshScale->setDescription("Uniform scale applied to all 12 leg segment nodes (default 1.0). "
//                                       "Use < 1.0 to shrink large meshes (e.g. 0.1 for a cylinder meant for big objects), "
//                                       "> 1.0 to enlarge small meshes. "
//                                       "Does NOT affect bone length calculation – that comes from the mesh AABB before scaling. "
//                                       "Bone lengths and hip/foot positions must be set in the same units as the SCALED mesh.");
//        m_onMovementChangedFunctionName->setDescription("Lua callback: onMovementChanged(manip,dt). "
//                                                        "manip:setStride(0..0.5) to walk, manip:setOmega(+-0.25..0.4) to turn.");
//        m_onMovementChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), m_onMovementChangedFunctionName->getString() + "(manip, dt)");
//
//        // Physics defaults
//        this->asSoftBody->setVisible(false);
//        this->mass->setValue(30.0f);
//        this->massOrigin->setValue(Ogre::Vector3(0.0f, -0.1f, 0.0f));
//        this->collisionSize->setValue(Ogre::Vector3::ZERO);
//        this->collisionPosition->setValue(Ogre::Vector3::ZERO);
//        this->collisionType->setListSelectedValue("ConvexHull");
//        this->angularDamping->setValue(Ogre::Vector3(0.7f, 0.05f, 0.7f));
//        this->linearDamping->setValue(0.1f);
//
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            m_legVisuals[i] = LegVisual();
//        }
//    }
//
//    PhysicsActiveSpiderComponent::~PhysicsActiveSpiderComponent()
//    {
//        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] Destructor for: " + this->gameObjectPtr->getName());
//    }
//
//    // =========================================================================
//    // init  (XML load)
//    // =========================================================================
//    bool PhysicsActiveSpiderComponent::init(rapidxml::xml_node<>*& propertyElement)
//    {
//        PhysicsActiveComponent::parseCommonProperties(propertyElement);
//
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrThighMeshName())
//        {
//            this->setThighMeshName(XMLConverter::getAttrib(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCalfMeshName())
//        {
//            this->setCalfMeshName(XMLConverter::getAttrib(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHeelMeshName())
//        {
//            this->setHeelMeshName(XMLConverter::getAttrib(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLegHipPos(i))
//            {
//                this->setLegHipPos(i, XMLConverter::getAttribVector3(propertyElement, "data"));
//                propertyElement = propertyElement->next_sibling("property");
//            }
//            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLegFootRestPos(i))
//            {
//                this->setLegFootRestPos(i, XMLConverter::getAttribVector3(propertyElement, "data"));
//                propertyElement = propertyElement->next_sibling("property");
//            }
//        }
//
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOnMovementChangedFunctionName())
//        {
//            this->setOnMovementChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWalkCycleDuration())
//        {
//            this->setWalkCycleDuration(XMLConverter::getAttribReal(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrStepHeight())
//        {
//            this->setStepHeight(XMLConverter::getAttribReal(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGaitSequence())
//        {
//            this->setGaitSequence(XMLConverter::getAttrib(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLegBoneAxis())
//        {
//            this->setLegBoneAxis(XMLConverter::getAttrib(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLegMeshScale())
//        {
//            this->setLegMeshScale(XMLConverter::getAttribReal(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrThighBoneLength())
//        {
//            this->setThighBoneLength(XMLConverter::getAttribReal(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCalfBoneLength())
//        {
//            this->setCalfBoneLength(XMLConverter::getAttribReal(propertyElement, "data"));
//            propertyElement = propertyElement->next_sibling("property");
//        }
//
//        return true;
//    }
//
//    // =========================================================================
//    // postInit
//    // =========================================================================
//    bool PhysicsActiveSpiderComponent::postInit(void)
//    {
//        bool success = PhysicsComponent::postInit();
//
//        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] postInit for: " + this->gameObjectPtr->getName());
//
//        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
//        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
//        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
//
//        this->gameObjectPtr->setDynamic(true);
//        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
//
//        return success;
//    }
//
//    // =========================================================================
//    // onRemoveComponent
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::onRemoveComponent(void)
//    {
//        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
//        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
//
//        this->destroyLegMeshes();
//
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->clearLegs();
//        }
//    }
//
//    // =========================================================================
//    // connect  (simulation start)
//    // =========================================================================
//    bool PhysicsActiveSpiderComponent::connect(void)
//    {
//        bool success = PhysicsActiveComponent::connect();
//
//        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] connect for: " + this->gameObjectPtr->getName());
//
//        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
//        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
//        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
//
//        if (!this->createDynamicBody())
//        {
//            return false;
//        }
//
//        // Create all 12 visual nodes/items on the render thread FIRST –
//        // registerLegChains() reads their scene nodes immediately after.
//        if (!this->createLegMeshes())
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] createLegMeshes failed for: " + this->gameObjectPtr->getName());
//            return false;
//        }
//
//        this->registerLegChains();
//        this->setCanMove(this->activated->getBool());
//
//        if (this->physicsBody && this->bShowDebugData)
//        {
//            ENQUEUE_RENDER_COMMAND_WAIT("PhysicsActiveSpiderComponent::showDebug", { this->physicsBody->showDebugCollision(false, this->bShowDebugData); });
//        }
//
//        return success;
//    }
//
//    // =========================================================================
//    // disconnect  (simulation stop)
//    // =========================================================================
//    bool PhysicsActiveSpiderComponent::disconnect(void)
//    {
//        bool success = PhysicsActiveComponent::disconnect();
//
//        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
//        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
//
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->clearLegs();
//            this->destroyCollision();
//            this->destroyBody();
//        }
//
//        this->destroyLegMeshes();
//
//        return success;
//    }
//
//    // =========================================================================
//    // update
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::update(Ogre::Real dt, bool notSimulating)
//    {
//        PhysicsActiveComponent::update(dt, notSimulating);
//
//        if (!notSimulating && this->physicsBody)
//        {
//            this->updateLegNodes(dt);
//        }
//    }
//
//    // =========================================================================
//    // createDynamicBody
//    // =========================================================================
//    bool PhysicsActiveSpiderComponent::createDynamicBody(void)
//    {
//        Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
//        Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;
//
//        if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
//        {
//            collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
//        }
//
//        Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;
//        OgreNewt::CollisionPtr collisionPtr;
//
//        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveSpiderComponent::createCollision", _4(&inertia, &collisionPtr, collisionOrientation, &calculatedMassOrigin),
//            { collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId()); });
//
//        if (!collisionPtr)
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Collision is null for: " + this->gameObjectPtr->getName());
//            return false;
//        }
//
//        if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
//        {
//            calculatedMassOrigin = this->massOrigin->getVector3();
//        }
//
//        LuaScript* luaScript = this->gameObjectPtr->getLuaScript();
//
//        this->physicsBody = new OgreNewt::SpiderBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr, this->mass->getReal(), calculatedMassOrigin, this->gravity->getVector3(), this->gameObjectPtr->getDefaultDirection(),
//            new PhysicsSpiderCallback(this->gameObjectPtr.get(), luaScript, this->ogreNewt, m_onMovementChangedFunctionName->getString()));
//
//        OgreNewt::SpiderBody* spider = static_cast<OgreNewt::SpiderBody*>(this->physicsBody);
//        spider->setWalkCycleDuration(m_walkCycleDuration->getReal());
//        spider->setStepHeight(m_stepHeight->getReal());
//
//        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);
//
//        this->physicsBody->setGravity(this->gravity->getVector3());
//
//        if (this->linearDamping->getReal() != 0.0f)
//        {
//            this->physicsBody->setLinearDamping(this->linearDamping->getReal());
//        }
//        if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
//        {
//            this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
//        }
//
//        this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveSpiderComponent>(&PhysicsActiveSpiderComponent::spiderMoveCallback, this);
//
//        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
//        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());
//
//        this->setPosition(this->initialPosition);
//        this->setOrientation(this->initialOrientation);
//
//        this->setConstraintAxis(this->constraintAxis->getVector3());
//        this->setConstraintDirection(this->constraintDirection->getVector3());
//        this->setCollidable(this->collidable->getBool());
//
//        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
//
//        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
//        this->physicsBody->setMaterialGroupID(materialId);
//
//        this->setActivated(this->activated->getBool());
//        this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());
//
//        return true;
//    }
//
//    // =========================================================================
//    // createLegMeshes  (render thread via ENQUEUE_RENDER_COMMAND_WAIT)
//    //
//    // Pattern follows ProceduralFoliageVolume:
//    //   For each of the 3 mesh types (thigh / calf / heel):
//    //     1. Load mesh via MeshManager.
//    //     2. Create FIRST item, check getNumSubItems() > 0.
//    //     3. Harvest all sub-item datablocks from the first item.
//    //     4. Create the remaining 3 items and share those datablocks.
//    //        Shared datablocks allow Ogre to batch all 4 segment instances.
//    //   All items are attached to child nodes of the torso scene node
//    //   and positioned at their initial bind-pose world positions.
//    // =========================================================================
//    bool PhysicsActiveSpiderComponent::createLegMeshes(void)
//    {
//        const Ogre::String thighMesh = m_thighMeshName->getString();
//        const Ogre::String calfMesh = m_calfMeshName->getString();
//        const Ogre::String heelMesh = m_heelMeshName->getString();
//
//        if (thighMesh.empty() || calfMesh.empty() || heelMesh.empty())
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] One or more mesh names are empty. "
//                                                                                "Set ThighMeshName, CalfMeshName and HeelMeshName.");
//            return false;
//        }
//
//        const int boneAxis = boneAxisFromString(m_legBoneAxis->getString());
//
//        // Derive bone lengths on the main thread before the render enqueue.
//        // Multiply by meshScale: scaling a node also scales the effective bone reach seen by the IK solver.
//
//
//        // Snapshot all data the render command needs (captured by value).
//        struct CreateData
//        {
//            Ogre::String thighMesh, thighRG;
//            Ogre::String calfMesh, calfRG;
//            Ogre::String heelMesh, heelRG;
//            bool castShadows;
//            bool visible;
//            Ogre::uint8 renderQueue;
//            Ogre::Real renderDistance;
//            Ogre::Real shadowRenderingDistance;
//            Ogre::Real meshScale; // uniform scale applied to each segment node
//        };
//
//        CreateData cd;
//        cd.thighMesh = thighMesh;
//        cd.thighRG = m_thighResourceGroup;
//        cd.calfMesh = calfMesh;
//        cd.calfRG = m_calfResourceGroup;
//        cd.heelMesh = heelMesh;
//        cd.heelRG = m_heelResourceGroup;
//        cd.castShadows = this->gameObjectPtr->getCastShadows();
//        cd.visible = this->gameObjectPtr->isVisible();
//        cd.renderQueue = static_cast<Ogre::uint8>(this->gameObjectPtr->getRenderQueueIndex());
//        cd.renderDistance = static_cast<Ogre::Real>(this->gameObjectPtr->getRenderDistance());
//        cd.shadowRenderingDistance = static_cast<Ogre::Real>(this->gameObjectPtr->getShadowRenderingDistance());
//        cd.meshScale = m_legMeshScale->getReal();
//
//        // Compute initial world positions so nodes look correct before first IK frame.
//
//        Ogre::Vector3 thighLocalPos[MAX_LEGS];
//        Ogre::Vector3 calfLocalPos[MAX_LEGS];
//        Ogre::Vector3 heelLocalPos[MAX_LEGS];
//
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            thighLocalPos[i] = m_legHipPos[i]->getVector3();
//            heelLocalPos[i] = m_legFootRestPos[i]->getVector3();
//            calfLocalPos[i] = (thighLocalPos[i] + heelLocalPos[i]) * 0.5f;
//        }
//
//        Ogre::SceneNode* torsoNode = this->gameObjectPtr->getSceneNode();
//        Ogre::SceneManager* sceneMgr = this->gameObjectPtr->getSceneManager();
//        LegVisual* visuals = m_legVisuals;
//        bool success = false;
//
//        NOWA::GraphicsModule::RenderCommand renderCommand = [this, &cd, torsoNode, sceneMgr, visuals, &thighLocalPos, &calfLocalPos, &heelLocalPos, &success]()
//        {
//            // ── Load mesh with error guard ─────────────────────────────────────
//            auto loadMesh = [&](const Ogre::String& name, const Ogre::String& rg) -> Ogre::MeshPtr
//            {
//                Ogre::MeshPtr ptr;
//                try
//                {
//                    ptr = Ogre::MeshManager::getSingletonPtr()->load(name, rg);
//                }
//                catch (const Ogre::Exception& e)
//                {
//                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Failed to load mesh '" + name + "': " + e.getDescription());
//                }
//                return ptr;
//            };
//
//            Ogre::MeshPtr thighMeshPtr = loadMesh(cd.thighMesh, cd.thighRG);
//            Ogre::MeshPtr calfMeshPtr = loadMesh(cd.calfMesh, cd.calfRG);
//            Ogre::MeshPtr heelMeshPtr = loadMesh(cd.heelMesh, cd.heelRG);
//
//            if (!thighMeshPtr || !calfMeshPtr || !heelMeshPtr)
//            {
//                success = false;
//                return;
//            }
//
//            // ── Apply all render properties from the root torso GameObject ─────
//            auto applyRenderProps = [&](Ogre::Item* item)
//            {
//                item->setCastShadows(cd.castShadows);
//                item->setVisible(cd.visible);
//                item->setRenderQueueGroup(cd.renderQueue);
//                item->setRenderingDistance(cd.renderDistance);
//                item->setShadowRenderingDistance(cd.shadowRenderingDistance);
//            };
//
//            // ── Per-mesh-type batch: first item stores datablocks; rest share them
//            struct SegBatch
//            {
//                Ogre::MeshPtr meshPtr;
//                const Ogre::String prefix;
//                Ogre::Vector3* positions;   // [MAX_LEGS]
//                Ogre::SceneNode** outNodes; // [MAX_LEGS]
//                Ogre::Item** outItems;      // [MAX_LEGS]
//            };
//
//            Ogre::SceneNode* thighNodes[MAX_LEGS] = {};
//            Ogre::SceneNode* calfNodes[MAX_LEGS] = {};
//            Ogre::SceneNode* heelNodes[MAX_LEGS] = {};
//            Ogre::Item* thighItems[MAX_LEGS] = {};
//            Ogre::Item* calfItems[MAX_LEGS] = {};
//            Ogre::Item* heelItems[MAX_LEGS] = {};
//
//            SegBatch batches[3] = {
//                {thighMeshPtr, "Spider_Thigh_Leg", thighLocalPos, thighNodes, thighItems},
//                {calfMeshPtr,  "Spider_Calf_Leg",  calfLocalPos,  calfNodes,  calfItems},
//                {heelMeshPtr,  "Spider_Heel_Leg",  heelLocalPos,  heelNodes,  heelItems}
//            };
//
//            bool allOk = true;
//
//            for (SegBatch& batch : batches)
//            {
//                // ── CREATE FIRST ITEM, harvest datablocks ──────────────────────
//                Ogre::Item* firstItem = sceneMgr->createItem(batch.meshPtr, Ogre::SCENE_DYNAMIC);
//
//                if (firstItem->getNumSubItems() == 0)
//                {
//                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Mesh '" + batch.meshPtr->getName() + "' has no sub-items.");
//                    sceneMgr->destroyItem(firstItem);
//                    allOk = false;
//                    break;
//                }
//
//                std::vector<Ogre::HlmsDatablock*> datablocks;
//                datablocks.reserve(firstItem->getNumSubItems());
//                for (size_t s = 0; s < firstItem->getNumSubItems(); ++s)
//                {
//                    datablocks.push_back(firstItem->getSubItem(s)->getDatablock());
//                }
//
//                Ogre::SceneNode* node0 = torsoNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
//                node0->setName(batch.prefix + "0");
//                node0->setPosition(batch.positions[0]); // world position (torso has identity orient at t=0, close enough)
//                if (cd.meshScale != 1.0f)
//                {
//                    node0->setScale(Ogre::Vector3(cd.meshScale));
//                }
//                node0->attachObject(firstItem);
//
//                applyRenderProps(firstItem);
//
//                batch.outNodes[0] = node0;
//                batch.outItems[0] = firstItem;
//
//                // ── CREATE REMAINING ITEMS – SHARE DATABLOCKS ──────────────────
//                for (int i = 1; i < MAX_LEGS; ++i)
//                {
//                    Ogre::Item* item = sceneMgr->createItem(batch.meshPtr, Ogre::SCENE_DYNAMIC);
//
//                    if (item->getNumSubItems() == 0)
//                    {
//                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Mesh '" + batch.meshPtr->getName() + "' item " + Ogre::StringConverter::toString(i) + " has no sub-items.");
//                        sceneMgr->destroyItem(item);
//                        allOk = false;
//                        break;
//                    }
//
//                    // Share datablocks from the first item (enables Ogre batching).
//                    for (size_t s = 0; s < item->getNumSubItems() && s < datablocks.size(); ++s)
//                    {
//                        item->getSubItem(s)->setDatablock(datablocks[s]);
//                    }
//
//                    Ogre::SceneNode* nodeI = torsoNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
//                    nodeI->setName(batch.prefix + Ogre::StringConverter::toString(i));
//                    nodeI->setPosition(batch.positions[i]);
//                    if (cd.meshScale != 1.0f)
//                    {
//                        nodeI->setScale(Ogre::Vector3(cd.meshScale));
//                    }
//                    nodeI->attachObject(item);
//
//                    applyRenderProps(item);
//
//                    batch.outNodes[i] = nodeI;
//                    batch.outItems[i] = item;
//                }
//
//                if (!allOk)
//                {
//                    break;
//                }
//            }
//
//            if (allOk)
//            {
//                for (int i = 0; i < MAX_LEGS; ++i)
//                {
//                    visuals[i].thighNode = thighNodes[i];
//                    visuals[i].thighItem = thighItems[i];
//                    visuals[i].calfNode = calfNodes[i];
//                    visuals[i].calfItem = calfItems[i];
//                    visuals[i].heelNode = heelNodes[i];
//                    visuals[i].heelItem = heelItems[i];
//                }
//                success = true;
//            }
//            else
//            {
//                // Clean up any partially created nodes/items.
//                auto destroySegment = [&](Ogre::SceneNode*& node, Ogre::Item*& item)
//                {
//                    if (nullptr != node)
//                    {
//                        if (nullptr != item)
//                        {
//                            node->detachObject(item);
//                            sceneMgr->destroyItem(item);
//                            item = nullptr;
//                        }
//                        node->getParentSceneNode()->removeAndDestroyChild(node);
//                        node = nullptr;
//                    }
//                };
//                for (int i = 0; i < MAX_LEGS; ++i)
//                {
//                    destroySegment(thighNodes[i], thighItems[i]);
//                    destroySegment(calfNodes[i], calfItems[i]);
//                    destroySegment(heelNodes[i], heelItems[i]);
//                }
//                success = false;
//            }
//        };
//        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsActiveSpiderComponent::createLegMeshes");
//
//        return success;
//    }
//
//    // =========================================================================
//    // destroyLegMeshes  (render thread via ENQUEUE_RENDER_COMMAND_WAIT)
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::destroyLegMeshes(void)
//    {
//        bool hasAny = false;
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            if (m_legVisuals[i].thighNode)
//            {
//                hasAny = true;
//                break;
//            }
//        }
//        if (!hasAny)
//        {
//            return;
//        }
//
//        Ogre::SceneManager* sceneMgr = this->gameObjectPtr->getSceneManager();
//        LegVisual* visuals = m_legVisuals;
//
//        NOWA::GraphicsModule::RenderCommand renderCommand = [this, sceneMgr, visuals]()
//        {
//            auto destroySegment = [&](Ogre::SceneNode*& node, Ogre::Item*& item)
//            {
//                if (nullptr != node)
//                {
//                    if (nullptr != item)
//                    {
//                        node->detachObject(item);
//                        sceneMgr->destroyItem(item);
//                        item = nullptr;
//                    }
//                    GraphicsModule::getInstance()->removeTrackedNode(node);
//                    node->getParentSceneNode()->removeAndDestroyChild(node);
//                    node = nullptr;
//                }
//            };
//
//            for (int i = 0; i < MAX_LEGS; ++i)
//            {
//                destroySegment(visuals[i].thighNode, visuals[i].thighItem);
//                destroySegment(visuals[i].calfNode, visuals[i].calfItem);
//                destroySegment(visuals[i].heelNode, visuals[i].heelItem);
//            }
//        };
//        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsActiveSpiderComponent::destroyLegMeshes");
//    }
//
//    // =========================================================================
//    // spiderMoveCallback  (Newton worker thread – reads cached manip only)
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::spiderMoveCallback(OgreNewt::Body* body, Ogre::Real dt, int threadIndex)
//    {
//        PhysicsActiveComponent::moveCallback(body, dt, threadIndex);
//
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->applyLocomotionImpulses(dt);
//        }
//    }
//
//    // =========================================================================
//    // registerLegChains  (called once in connect, after createLegMeshes)
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::registerLegChains(void)
//    {
//        if (!this->physicsBody)
//        {
//            return;
//        }
//
//        OgreNewt::SpiderBody* spider = static_cast<OgreNewt::SpiderBody*>(this->physicsBody);
//        spider->clearLegs();
//
//        const int boneAxis = boneAxisFromString(m_legBoneAxis->getString());
//        int activeLegCount = 0;
//
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            const LegVisual& lv = m_legVisuals[i];
//
//            if (!lv.thighNode || !lv.calfNode || !lv.heelNode)
//            {
//                continue;
//            }
//
//            const Ogre::Vector3 hipLocalPos = m_legHipPos[i]->getVector3();
//            const Ogre::Vector3 footRestLocal = m_legFootRestPos[i]->getVector3();
//
//            if (footRestLocal.y >= hipLocalPos.y)
//            {
//                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] Leg " + Ogre::StringConverter::toString(i) + ": LegFootRestPos.y must be below LegHipPos.y. Skipping.");
//                continue;
//            }
//
//            // Bone lengths: if user has set explicit values (> 0) use those.
//            // Otherwise auto-compute as half the hip-to-foot distance (equal thigh/calf segments),
//            // matching Newton's approach of deriving length from actual joint positions.
//            const Ogre::Real hipToFoot = (footRestLocal - hipLocalPos).length();
//            const Ogre::Real autoLen = hipToFoot * 0.5f;
//            const Ogre::Real thighLen = (m_thighBoneLength->getReal() > 0.0f) ? m_thighBoneLength->getReal() : autoLen;
//            const Ogre::Real calfLen  = (m_calfBoneLength->getReal() > 0.0f)  ? m_calfBoneLength->getReal()  : autoLen;
//
//            // Swivel sign: negative-Z side = left legs, positive-Z = right legs.
//            const Ogre::Real swivelSign = (hipLocalPos.z >= 0.0f) ? 1.0f : -1.0f;
//
//            spider->addLegChain(lv.thighNode, lv.calfNode, lv.heelNode, hipLocalPos, footRestLocal, thighLen, calfLen, swivelSign, boneAxis);
//
//            ++activeLegCount;
//
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] Leg " + Ogre::StringConverter::toString(i) + " hip=" + Ogre::StringConverter::toString(hipLocalPos) + " foot=" + Ogre::StringConverter::toString(footRestLocal) + " hipToFoot=" + Ogre::StringConverter::toString(hipToFoot) + " thighLen=" + Ogre::StringConverter::toString(thighLen) + " calfLen=" + Ogre::StringConverter::toString(calfLen));
//        }
//
//        if (activeLegCount > 0)
//        {
//            spider->setGaitSequence(parseGaitSequence(m_gaitSequence->getString(), activeLegCount));
//        }
//    }
//
//    // =========================================================================
//    // updateLegNodes  (main thread, every frame)
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::updateLegNodes(Ogre::Real dt)
//    {
//        OgreNewt::SpiderBody* spider = static_cast<OgreNewt::SpiderBody*>(this->physicsBody);
//
//        if (!spider || spider->getLegChains().empty())
//        {
//            return;
//        }
//
//        // Main thread: query Lua callback, cache stride/omega for the Newton thread.
//        spider->updateMovementInput(dt);
//
//        // Main thread: gait tick + IK, fills world transforms in each LegChainInfo.
//        spider->computeLegTransforms(dt);
//
//        // Push all 12 segment transforms to GraphicsModule's lock-free buffer.
//        // GraphicsModule::updateNodeTransform calls node->setPosition() which on a child
//        // node is PARENT-RELATIVE, not world-space.
//
//        auto closureFunction = [this, spider](Ogre::Real renderDt)
//        {
//            for (const OgreNewt::LegChainInfo& leg : spider->getLegChains())
//            {
//                if (leg.thigh)
//                {
//                    leg.thigh->setPosition(leg.thighWorldPos);
//                    leg.thigh->setOrientation(leg.thighWorldOrient);
//                }
//                if (leg.calf)
//                {
//                    leg.calf->setPosition(leg.kneeWorldPos);
//                    leg.calf->setOrientation(leg.calfWorldOrient);
//                }
//                if (leg.heel)
//                {
//                    leg.heel->setPosition(leg.footWorldPos);
//                    leg.heel->setOrientation(leg.heelWorldOrient);
//                }
//            }
//        };
//        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
//        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
//    }
//
//    // =========================================================================
//    // deriveBoneLength  (from mesh AABB along boneAxis)
//    // =========================================================================
//    Ogre::Real PhysicsActiveSpiderComponent::deriveBoneLength(const Ogre::String& meshName, const Ogre::String& resourceGroup, int boneAxis, Ogre::Real fallback)
//    {
//        if (meshName.empty())
//        {
//            return fallback;
//        }
//
//        try
//        {
//            Ogre::MeshPtr mesh = Ogre::MeshManager::getSingletonPtr()->load(meshName, resourceGroup);
//            if (nullptr == mesh)
//            {
//                return fallback;
//            }
//
//            const Ogre::Aabb aabb = mesh->getAabb();
//            Ogre::Real length = fallback;
//            switch (boneAxis)
//            {
//            case 0:
//                length = aabb.mHalfSize.x * 2.0f;
//                break;
//            case 2:
//                length = aabb.mHalfSize.z * 2.0f;
//                break;
//            default:
//                length = aabb.mHalfSize.y * 2.0f;
//                break; // Y
//            }
//            return (length > 0.01f) ? length : fallback;
//        }
//        catch (const Ogre::Exception& e)
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveSpiderComponent] deriveBoneLength failed for '" + meshName + "': " + e.getDescription());
//            return fallback;
//        }
//    }
//
//    // =========================================================================
//    // resolveResourceGroup  (same pattern as ProceduralFoliageVolume)
//    // =========================================================================
//    Ogre::String PhysicsActiveSpiderComponent::resolveResourceGroup(Variant* meshNameVariant)
//    {
//        if (!meshNameVariant)
//        {
//            return Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
//        }
//
//        const Ogre::String folderPath = meshNameVariant->getUserDataValue("PathToFolder");
//        if (!folderPath.empty())
//        {
//            return Core::getSingletonPtr()->extractResourceGroupFromPath(folderPath);
//        }
//        return Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
//    }
//
//    // =========================================================================
//    // parseGaitSequence
//    // =========================================================================
//    std::vector<int> PhysicsActiveSpiderComponent::parseGaitSequence(const Ogre::String& str, int legCount)
//    {
//        std::vector<int> result;
//
//        if (!str.empty())
//        {
//            Ogre::StringVector tokens = Ogre::StringUtil::split(str, ",");
//            for (const Ogre::String& tok : tokens)
//            {
//                Ogre::String t = tok;
//                Ogre::StringUtil::trim(t);
//                if (!t.empty())
//                {
//                    result.push_back(Ogre::StringConverter::parseInt(t));
//                }
//            }
//        }
//
//        bool valid = (static_cast<int>(result.size()) == legCount);
//        if (valid)
//        {
//            for (int v : result)
//            {
//                if (v < 0 || v >= legCount)
//                {
//                    valid = false;
//                    break;
//                }
//            }
//        }
//
//        if (!valid)
//        {
//            result.clear();
//            switch (legCount)
//            {
//            case 2:
//                result = {1, 0};
//                break;
//            case 4:
//                result = {3, 1, 2, 0};
//                break;
//            case 6:
//                result = {5, 2, 4, 1, 3, 0};
//                break;
//            default:
//                for (int i = legCount - 1; i >= 0; --i)
//                {
//                    result.push_back(i);
//                }
//                break;
//            }
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] GaitSequence '" + str + "' invalid for " + Ogre::StringConverter::toString(legCount) + " legs; using default.");
//        }
//        return result;
//    }
//
//    int PhysicsActiveSpiderComponent::boneAxisFromString(const Ogre::String& axis)
//    {
//        if (axis == "X" || axis == "x")
//        {
//            return 0;
//        }
//        if (axis == "Z" || axis == "z")
//        {
//            return 2;
//        }
//        return 1;
//    }
//
//    // =========================================================================
//    // setActivated
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::setActivated(bool activated)
//    {
//        PhysicsActiveComponent::setActivated(activated);
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setCanMove(activated);
//        }
//    }
//
//    // =========================================================================
//    // Mesh name setters / getters
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::setThighMeshName(const Ogre::String& meshName)
//    {
//        m_thighMeshName->setValue(meshName);
//        m_thighResourceGroup = resolveResourceGroup(m_thighMeshName);
//        if (!meshName.empty())
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] ThighMesh='" + meshName + "' group='" + m_thighResourceGroup + "'");
//        }
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getThighMeshName(void) const
//    {
//        return m_thighMeshName->getString();
//    }
//
//    void PhysicsActiveSpiderComponent::setCalfMeshName(const Ogre::String& meshName)
//    {
//        m_calfMeshName->setValue(meshName);
//        m_calfResourceGroup = resolveResourceGroup(m_calfMeshName);
//        if (!meshName.empty())
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] CalfMesh='" + meshName + "' group='" + m_calfResourceGroup + "'");
//        }
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getCalfMeshName(void) const
//    {
//        return m_calfMeshName->getString();
//    }
//
//    void PhysicsActiveSpiderComponent::setHeelMeshName(const Ogre::String& meshName)
//    {
//        m_heelMeshName->setValue(meshName);
//        m_heelResourceGroup = resolveResourceGroup(m_heelMeshName);
//        if (!meshName.empty())
//        {
//            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveSpiderComponent] HeelMesh='" + meshName + "' group='" + m_heelResourceGroup + "'");
//        }
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getHeelMeshName(void) const
//    {
//        return m_heelMeshName->getString();
//    }
//
//    // =========================================================================
//    // Per-leg placement setters / getters
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::setLegHipPos(int i, const Ogre::Vector3& pos)
//    {
//        if (i >= 0 && i < MAX_LEGS)
//        {
//            m_legHipPos[i]->setValue(pos);
//        }
//    }
//    Ogre::Vector3 PhysicsActiveSpiderComponent::getLegHipPos(int i) const
//    {
//        return (i >= 0 && i < MAX_LEGS) ? m_legHipPos[i]->getVector3() : Ogre::Vector3::ZERO;
//    }
//    void PhysicsActiveSpiderComponent::setLegFootRestPos(int i, const Ogre::Vector3& pos)
//    {
//        if (i >= 0 && i < MAX_LEGS)
//        {
//            m_legFootRestPos[i]->setValue(pos);
//        }
//    }
//    Ogre::Vector3 PhysicsActiveSpiderComponent::getLegFootRestPos(int i) const
//    {
//        return (i >= 0 && i < MAX_LEGS) ? m_legFootRestPos[i]->getVector3() : Ogre::Vector3::ZERO;
//    }
//
//    // =========================================================================
//    // Gait setters / getters
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::setOnMovementChangedFunctionName(const Ogre::String& name)
//    {
//        m_onMovementChangedFunctionName->setValue(name);
//        m_onMovementChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), name + "(manip, dt)");
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getOnMovementChangedFunctionName(void) const
//    {
//        return m_onMovementChangedFunctionName->getString();
//    }
//
//    void PhysicsActiveSpiderComponent::setWalkCycleDuration(Ogre::Real d)
//    {
//        m_walkCycleDuration->setValue(d);
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setWalkCycleDuration(d);
//        }
//    }
//    Ogre::Real PhysicsActiveSpiderComponent::getWalkCycleDuration(void) const
//    {
//        return m_walkCycleDuration->getReal();
//    }
//
//    void PhysicsActiveSpiderComponent::setStepHeight(Ogre::Real h)
//    {
//        m_stepHeight->setValue(h);
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setStepHeight(h);
//        }
//    }
//    Ogre::Real PhysicsActiveSpiderComponent::getStepHeight(void) const
//    {
//        return m_stepHeight->getReal();
//    }
//
//    void PhysicsActiveSpiderComponent::setGaitSequence(const Ogre::String& s)
//    {
//        m_gaitSequence->setValue(s);
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getGaitSequence(void) const
//    {
//        return m_gaitSequence->getString();
//    }
//
//    void PhysicsActiveSpiderComponent::setLegBoneAxis(const Ogre::String& a)
//    {
//        m_legBoneAxis->setListSelectedValue(a);
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getLegBoneAxis(void) const
//    {
//        return m_legBoneAxis->getString();
//    }
//
//    void PhysicsActiveSpiderComponent::setLegMeshScale(Ogre::Real scale)
//    {
//        m_legMeshScale->setValue(scale);
//        // If nodes already exist (live edit during simulation), rescale them immediately.
//        // This runs on the main thread; the enqueue is needed because setScale touches the scene graph.
//        LegVisual* visuals = m_legVisuals;
//        bool hasAny = false;
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            if (visuals[i].thighNode)
//            {
//                hasAny = true;
//                break;
//            }
//        }
//        if (hasAny)
//        {
//            const Ogre::Vector3 scaleVec(scale);
//            NOWA::GraphicsModule::RenderCommand renderCommand = [visuals, scaleVec]()
//            {
//                for (int i = 0; i < MAX_LEGS; ++i)
//                {
//                    if (visuals[i].thighNode)
//                    {
//                        visuals[i].thighNode->setScale(scaleVec);
//                    }
//                    if (visuals[i].calfNode)
//                    {
//                        visuals[i].calfNode->setScale(scaleVec);
//                    }
//                    if (visuals[i].heelNode)
//                    {
//                        visuals[i].heelNode->setScale(scaleVec);
//                    }
//                }
//            };
//            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsActiveSpiderComponent::rescaleLegNodes");
//        }
//    }
//    Ogre::Real PhysicsActiveSpiderComponent::getLegMeshScale(void) const
//    {
//        return m_legMeshScale->getReal();
//    }
//
//    void PhysicsActiveSpiderComponent::setThighBoneLength(Ogre::Real length)
//    {
//        m_thighBoneLength->setValue(length);
//    }
//
//    Ogre::Real PhysicsActiveSpiderComponent::getThighBoneLength(void) const
//    {
//        return m_thighBoneLength->getReal();
//    }
//
//    void PhysicsActiveSpiderComponent::setCalfBoneLength(Ogre::Real length)
//    {
//        m_calfBoneLength->setValue(length);
//    }
//
//    Ogre::Real PhysicsActiveSpiderComponent::getCalfBoneLength(void) const
//    {
//        return m_calfBoneLength->getReal();
//    }
//
//    void PhysicsActiveSpiderComponent::setCanMove(bool canMove)
//    {
//        if (this->physicsBody)
//        {
//            static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->setCanMove(canMove);
//        }
//    }
//
//    bool PhysicsActiveSpiderComponent::isOnGround(void) const
//    {
//        if (!this->physicsBody)
//        {
//            return false;
//        }
//        return static_cast<OgreNewt::SpiderBody*>(this->physicsBody)->isOnGround();
//    }
//
//    OgreNewt::SpiderBody* PhysicsActiveSpiderComponent::getSpiderBody(void) const
//    {
//        return static_cast<OgreNewt::SpiderBody*>(this->physicsBody);
//    }
//
//    // =========================================================================
//    // actualizeValue  (live editor attribute changes)
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::actualizeValue(Variant* attribute)
//    {
//        PhysicsActiveComponent::actualizeCommonValue(attribute);
//
//        if (attribute->getName() == AttrThighMeshName())
//        {
//            this->setThighMeshName(attribute->getString());
//        }
//        else if (attribute->getName() == AttrCalfMeshName())
//        {
//            this->setCalfMeshName(attribute->getString());
//        }
//        else if (attribute->getName() == AttrHeelMeshName())
//        {
//            this->setHeelMeshName(attribute->getString());
//        }
//        else if (attribute->getName() == AttrOnMovementChangedFunctionName())
//        {
//            this->setOnMovementChangedFunctionName(attribute->getString());
//        }
//        else if (attribute->getName() == AttrWalkCycleDuration())
//        {
//            this->setWalkCycleDuration(attribute->getReal());
//        }
//        else if (attribute->getName() == AttrStepHeight())
//        {
//            this->setStepHeight(attribute->getReal());
//        }
//        else if (attribute->getName() == AttrGaitSequence())
//        {
//            this->setGaitSequence(attribute->getString());
//        }
//        else if (attribute->getName() == AttrLegBoneAxis())
//        {
//            this->setLegBoneAxis(attribute->getString());
//        }
//        else if (attribute->getName() == AttrLegMeshScale())
//        {
//            this->setLegMeshScale(attribute->getReal());
//        }
//        else if (attribute->getName() == AttrThighBoneLength())
//        {
//            this->setThighBoneLength(attribute->getReal());
//        }
//        else if (attribute->getName() == AttrCalfBoneLength())
//        {
//            this->setCalfBoneLength(attribute->getReal());
//        }
//        else
//        {
//            for (int i = 0; i < MAX_LEGS; ++i)
//            {
//                if (attribute->getName() == AttrLegHipPos(i))
//                {
//                    this->setLegHipPos(i, attribute->getVector3());
//                }
//                else if (attribute->getName() == AttrLegFootRestPos(i))
//                {
//                    this->setLegFootRestPos(i, attribute->getVector3());
//                }
//            }
//        }
//    }
//
//    // =========================================================================
//    // writeXML
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
//    {
//        PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);
//
//        xml_node<>* propertyXML = nullptr;
//
//        auto writeString = [&](const Ogre::String& attrName, const Ogre::String& value)
//        {
//            propertyXML = doc.allocate_node(node_element, "property");
//            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
//            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
//            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
//            propertiesXML->append_node(propertyXML);
//        };
//
//        auto writeReal = [&](const Ogre::String& attrName, Ogre::Real value)
//        {
//            propertyXML = doc.allocate_node(node_element, "property");
//            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
//            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
//            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
//            propertiesXML->append_node(propertyXML);
//        };
//
//        auto writeVec3 = [&](const Ogre::String& attrName, const Ogre::Vector3& value)
//        {
//            propertyXML = doc.allocate_node(node_element, "property");
//            propertyXML->append_attribute(doc.allocate_attribute("type", "10")); // Vector3
//            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
//            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
//            propertiesXML->append_node(propertyXML);
//        };
//
//        writeString(AttrThighMeshName(), m_thighMeshName->getString());
//        writeString(AttrCalfMeshName(), m_calfMeshName->getString());
//        writeString(AttrHeelMeshName(), m_heelMeshName->getString());
//
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            writeVec3(AttrLegHipPos(i), m_legHipPos[i]->getVector3());
//            writeVec3(AttrLegFootRestPos(i), m_legFootRestPos[i]->getVector3());
//        }
//
//        writeString(AttrOnMovementChangedFunctionName(), m_onMovementChangedFunctionName->getString());
//        writeReal(AttrWalkCycleDuration(), m_walkCycleDuration->getReal());
//        writeReal(AttrStepHeight(), m_stepHeight->getReal());
//        writeString(AttrGaitSequence(), m_gaitSequence->getString());
//        writeString(AttrLegBoneAxis(), m_legBoneAxis->getString());
//        writeReal(AttrLegMeshScale(), m_legMeshScale->getReal());
//        writeReal(AttrThighBoneLength(), m_thighBoneLength->getReal());
//        writeReal(AttrCalfBoneLength(), m_calfBoneLength->getReal());
//    }
//
//    // =========================================================================
//    // getClassName
//    // =========================================================================
//    Ogre::String PhysicsActiveSpiderComponent::getClassName(void) const
//    {
//        return "PhysicsActiveSpiderComponent";
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getParentClassName(void) const
//    {
//        return "PhysicsActiveComponent";
//    }
//    Ogre::String PhysicsActiveSpiderComponent::getParentParentClassName(void) const
//    {
//        return "PhysicsComponent";
//    }
//
//    // =========================================================================
//    // clone
//    // =========================================================================
//    GameObjectCompPtr PhysicsActiveSpiderComponent::clone(GameObjectPtr clonedGameObjectPtr)
//    {
//        auto cloned = boost::make_shared<PhysicsActiveSpiderComponent>();
//
//        cloned->setThighMeshName(m_thighMeshName->getString());
//        cloned->setCalfMeshName(m_calfMeshName->getString());
//        cloned->setHeelMeshName(m_heelMeshName->getString());
//
//        for (int i = 0; i < MAX_LEGS; ++i)
//        {
//            cloned->setLegHipPos(i, m_legHipPos[i]->getVector3());
//            cloned->setLegFootRestPos(i, m_legFootRestPos[i]->getVector3());
//        }
//
//        cloned->setOnMovementChangedFunctionName(m_onMovementChangedFunctionName->getString());
//        cloned->setWalkCycleDuration(m_walkCycleDuration->getReal());
//        cloned->setStepHeight(m_stepHeight->getReal());
//        cloned->setGaitSequence(m_gaitSequence->getString());
//        cloned->setLegBoneAxis(m_legBoneAxis->getString());
//        cloned->setLegMeshScale(m_legMeshScale->getReal());
//        cloned->setThighBoneLength(m_thighBoneLength->getReal());
//        cloned->setCalfBoneLength(m_calfBoneLength->getReal());
//
//        cloned->setOwner(clonedGameObjectPtr);
//        return cloned;
//    }
//
//    // =========================================================================
//    // Free Lua getter functions
//    // =========================================================================
//    PhysicsActiveSpiderComponent* getPhysicsActiveSpiderComponent(GameObject* gameObject)
//    {
//        return makeStrongPtr<PhysicsActiveSpiderComponent>(gameObject->getComponent<PhysicsActiveSpiderComponent>()).get();
//    }
//
//    PhysicsActiveSpiderComponent* getPhysicsActiveSpiderComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
//    {
//        return makeStrongPtr<PhysicsActiveSpiderComponent>(gameObject->getComponentWithOccurrence<PhysicsActiveSpiderComponent>(occurrenceIndex)).get();
//    }
//
//    PhysicsActiveSpiderComponent* getPhysicsActiveSpiderComponentFromName(GameObject* gameObject, const Ogre::String& name)
//    {
//        return makeStrongPtr<PhysicsActiveSpiderComponent>(gameObject->getComponentFromName<PhysicsActiveSpiderComponent>(name)).get();
//    }
//
//    // =========================================================================
//    // createStaticApiForLua
//    // =========================================================================
//    void PhysicsActiveSpiderComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
//    {
//        using namespace luabind;
//
//        module(lua)[class_<OgreNewt::SpiderMovementManipulation>("SpiderMovementManipulation")
//                        .def("setStride", &OgreNewt::SpiderMovementManipulation::setStride)
//                        .def("getStride", &OgreNewt::SpiderMovementManipulation::getStride)
//                        .def("setOmega", &OgreNewt::SpiderMovementManipulation::setOmega)
//                        .def("getOmega", &OgreNewt::SpiderMovementManipulation::getOmega)
//                        .def("setBodySwayX", &OgreNewt::SpiderMovementManipulation::setBodySwayX)
//                        .def("getBodySwayX", &OgreNewt::SpiderMovementManipulation::getBodySwayX)
//                        .def("setBodySwayZ", &OgreNewt::SpiderMovementManipulation::setBodySwayZ)
//                        .def("getBodySwayZ", &OgreNewt::SpiderMovementManipulation::getBodySwayZ),
//
//            class_<PhysicsActiveSpiderComponent, bases<PhysicsActiveComponent>>("PhysicsActiveSpiderComponent")
//                .def("setThighMeshName", &PhysicsActiveSpiderComponent::setThighMeshName)
//                .def("getThighMeshName", &PhysicsActiveSpiderComponent::getThighMeshName)
//                .def("setCalfMeshName", &PhysicsActiveSpiderComponent::setCalfMeshName)
//                .def("getCalfMeshName", &PhysicsActiveSpiderComponent::getCalfMeshName)
//                .def("setHeelMeshName", &PhysicsActiveSpiderComponent::setHeelMeshName)
//                .def("getHeelMeshName", &PhysicsActiveSpiderComponent::getHeelMeshName)
//                .def("setLegHipPos", &PhysicsActiveSpiderComponent::setLegHipPos)
//                .def("getLegHipPos", &PhysicsActiveSpiderComponent::getLegHipPos)
//                .def("setLegFootRestPos", &PhysicsActiveSpiderComponent::setLegFootRestPos)
//                .def("getLegFootRestPos", &PhysicsActiveSpiderComponent::getLegFootRestPos)
//                .def("setWalkCycleDuration", &PhysicsActiveSpiderComponent::setWalkCycleDuration)
//                .def("getWalkCycleDuration", &PhysicsActiveSpiderComponent::getWalkCycleDuration)
//                .def("setStepHeight", &PhysicsActiveSpiderComponent::setStepHeight)
//                .def("getStepHeight", &PhysicsActiveSpiderComponent::getStepHeight)
//                .def("setCanMove", &PhysicsActiveSpiderComponent::setCanMove)
//                .def("isOnGround", &PhysicsActiveSpiderComponent::isOnGround)];
//
//        gameObjectClass.def("getPhysicsActiveSpiderComponent", (PhysicsActiveSpiderComponent * (*)(GameObject*)) & getPhysicsActiveSpiderComponent)
//            .def("getPhysicsActiveSpiderComponentFromIndex", (PhysicsActiveSpiderComponent * (*)(GameObject*, unsigned int)) & getPhysicsActiveSpiderComponentFromIndex)
//            .def("getPhysicsActiveSpiderComponentFromName", &getPhysicsActiveSpiderComponentFromName);
//
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "class inherits PhysicsActiveComponent",
//            "Procedural IK spider. One physics torso; 12 leg segment nodes created/owned by the component. "
//            "Set ThighMeshName/CalfMeshName/HeelMeshName and 4 hip/foot rest positions.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setCanMove(boolean canMove)", "Enable/disable locomotion.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "boolean isOnGround()", "True if at least one active torso contact.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setWalkCycleDuration(number seconds)", "Full gait cycle duration (default 2.0 s).");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "number getWalkCycleDuration()", "Gets gait cycle duration.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setStepHeight(number m)", "Foot arc height during swing (default 0.2 m).");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "number getStepHeight()", "Gets step height.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setThighMeshName(string name)", "Upper-leg mesh (shared by all 4 legs).");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setCalfMeshName(string name)", "Lower-leg mesh.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setHeelMeshName(string name)", "Foot/ankle mesh.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setLegHipPos(int i, Vector3 pos)", "Hip socket position for leg i (0..3) in torso-local space.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "Vector3 getLegHipPos(int i)", "Gets hip socket position for leg i.");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "void setLegFootRestPos(int i, Vector3 p)", "Neutral foot position for leg i (torso-local, Y < 0).");
//        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveSpiderComponent", "Vector3 getLegFootRestPos(int i)", "Gets neutral foot position for leg i.");
//
//        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "SpiderMovementManipulation", "Passed to onMovementChanged.");
//        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setStride(number stride)", "Forward stride (0=stand, ~0.3=walk, ~0.5=run).");
//        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setOmega(number omega)", "Yaw rate rad/s (+-0.25..0.4).");
//        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setBodySwayX(number sway)", "Lateral lean (cosmetic).");
//        LuaScriptApi::getInstance()->addClassToCollection("SpiderMovementManipulation", "void setBodySwayZ(number sway)", "Forward lean (cosmetic).");
//
//        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveSpiderComponent getPhysicsActiveSpiderComponent()", "Gets the component.");
//        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveSpiderComponent getPhysicsActiveSpiderComponentFromIndex(unsigned int occurrenceIndex)", "Gets by index.");
//        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveSpiderComponent getPhysicsActiveSpiderComponentFromName(String name)", "Gets by name.");
//
//        gameObjectControllerClass.def("castPhysicsActiveSpiderComponent", &GameObjectController::cast<PhysicsActiveSpiderComponent>);
//        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PhysicsActiveSpiderComponent castPhysicsActiveSpiderComponent(PhysicsActiveSpiderComponent other)", "Casts type for Lua auto completion.");
//    }
//
//} // namespace NOWA
