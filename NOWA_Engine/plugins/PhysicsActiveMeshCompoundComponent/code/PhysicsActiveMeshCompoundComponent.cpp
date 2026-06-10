#include "NOWAPrecompiled.h"
#include "PhysicsActiveMeshCompoundComponent.h"
#include "gameobject/PhysicsCompoundConnectionComponent.h"
#include "main/AppStateManager.h"
#include "utilities/XMLConverter.h"
#include "OgreAbiUtils.h"

#include "OgreBitwise.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreVertexBufferPacked.h"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    //  Construction
    // =========================================================================

    PhysicsActiveMeshCompoundComponent::PhysicsActiveMeshCompoundComponent()
        : PhysicsActiveCompoundComponent(),
        name("PhysicsActiveMeshCompoundComponent"),
        convexDecompositionMode(nullptr),
        hullTolerance(nullptr)
    {
        // ── Decomposition mode dropdown ───────────────────────────────────────
        std::vector<Ogre::String> modes;
        modes.push_back("Single Hull");
        modes.push_back("Per Sub-Mesh");
        modes.push_back("Octant 2x2x2");
        // Will crash in ND4 capsule!
        // modes.push_back("Octant 3x3x3");

        this->convexDecompositionMode = new Variant(PhysicsActiveMeshCompoundComponent::AttrConvexDecompositionMode(), modes, this->attributes);
        this->convexDecompositionMode->setListSelectedValue("Octant 2x2x2");

        this->convexDecompositionMode->setDescription("How the mesh is decomposed into convex collision hulls:\n"
                                                      "  Single Hull   – one hull for the entire mesh.\n"
                                                      "  Per Sub-Mesh  – one hull per submesh.\n"
                                                      "  Octant 2x2x2  – AABB divided into 8 cells, one hull per cell.\n"
                                                      "  Octant 3x3x3  – AABB divided into 27 cells, one hull per cell.\n"
                                                      "Higher octant counts better approximate concave shapes at the cost of "
                                                      "more collision primitives.");

        // ── Hull vertex-tolerance ─────────────────────────────────────────────
        this->hullTolerance = new Variant(PhysicsActiveMeshCompoundComponent::AttrHullTolerance(), 0.001f, this->attributes);

        this->hullTolerance->setDescription("Newton vertex-merging threshold for each convex hull (default 0.001).\n"
                                            "Lower value = more precise hull geometry.\n"
                                            "Higher value = simplified hull with fewer vertices.");
    }

    PhysicsActiveMeshCompoundComponent::~PhysicsActiveMeshCompoundComponent()
    {
        
    }

    void PhysicsActiveMeshCompoundComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PhysicsActiveMeshCompoundComponent>(PhysicsActiveMeshCompoundComponent::getStaticClassId(), PhysicsActiveMeshCompoundComponent::getStaticClassName());
    }

    void PhysicsActiveMeshCompoundComponent::initialise()
    {
    }

    void PhysicsActiveMeshCompoundComponent::shutdown()
    {
    }

    void PhysicsActiveMeshCompoundComponent::uninstall()
    {
    }

    const Ogre::String& PhysicsActiveMeshCompoundComponent::getName() const
    {
        return this->name;
    }

    void PhysicsActiveMeshCompoundComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    // =========================================================================
    //  init  – read XML properties
    // =========================================================================

    bool PhysicsActiveMeshCompoundComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        // Read common physics properties (mass, damping, gravity, constraints …)
        PhysicsActiveComponent::parseCommonProperties(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == PhysicsActiveMeshCompoundComponent::AttrConvexDecompositionMode())
        {
            this->convexDecompositionMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Single Hull"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == PhysicsActiveMeshCompoundComponent::AttrHullTolerance())
        {
            this->hullTolerance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.001f));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    // =========================================================================
    //  postInit  – skip the parent's XML sanity checks; call our createDynamicBody
    // =========================================================================

    bool PhysicsActiveMeshCompoundComponent::postInit(void)
    {
        // Call PhysicsComponent::postInit (initialises this->ogreNewt etc.) but
        // deliberately skip PhysicsActiveCompoundComponent::postInit so we do not
        // run its meshCompoundConfigFile sanity checks or its createDynamicBody().
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] Init for game object: " + this->gameObjectPtr->getName());

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        // Must be dynamic — static objects cannot use active physics bodies
        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        // If a PhysicsCompoundConnectionComponent is present on this GO, it will
        // call createCompoundBody() for the whole connected group — do not also
        // create an independent body here.
        auto physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsCompoundConnectionComponent>());

        if (nullptr == physicsCompoundConnectionCompPtr)
        {
            if (!this->createDynamicBody())
            {
                return false;
            }
        }

        return success;
    }

    bool PhysicsActiveMeshCompoundComponent::connect(void)
    {
        return PhysicsActiveComponent::connect();
    }

    bool PhysicsActiveMeshCompoundComponent::disconnect(void)
    {
        return PhysicsActiveComponent::disconnect();
    }

    // =========================================================================
    //  Class name
    // =========================================================================

    Ogre::String PhysicsActiveMeshCompoundComponent::getClassName(void) const
    {
        return "PhysicsActiveMeshCompoundComponent";
    }

    Ogre::String PhysicsActiveMeshCompoundComponent::getParentClassName(void) const
    {
        return "PhysicsActiveCompoundComponent";
    }

    Ogre::String PhysicsActiveMeshCompoundComponent::getParentParentClassName(void) const
    {
        return "PhysicsActiveComponent";
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr PhysicsActiveMeshCompoundComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsActiveMeshCompoundCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveMeshCompoundComponent>());

        // ── Common physics properties (from PhysicsActiveComponent) ────────────
        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setMass(this->mass->getReal());
        clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
        clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
        clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
        clonedCompPtr->setGravity(this->gravity->getVector3());
        clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
        clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
        clonedCompPtr->setSpeed(this->speed->getReal());
        clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
        clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
        clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());

        // ── Our specific properties ────────────────────────────────────────────
        clonedCompPtr->setConvexDecompositionMode(this->convexDecompositionMode->getListSelectedValue());
        clonedCompPtr->setHullTolerance(this->hullTolerance->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    // =========================================================================
    //  actualizeValue  – property panel callback
    // =========================================================================

    void PhysicsActiveMeshCompoundComponent::actualizeValue(Variant* attribute)
    {
        // Route common physics properties to the grandparent handler
        PhysicsActiveComponent::actualizeCommonValue(attribute);

        if (PhysicsActiveMeshCompoundComponent::AttrConvexDecompositionMode() == attribute->getName())
        {
            this->setConvexDecompositionMode(attribute->getListSelectedValue());
        }
        else if (PhysicsActiveMeshCompoundComponent::AttrHullTolerance() == attribute->getName())
        {
            this->setHullTolerance(attribute->getReal());
        }
    }

    // =========================================================================
    //  writeXML
    // =========================================================================

    void PhysicsActiveMeshCompoundComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        // Write common physics properties (mass, damping, gravity, constraints …)
        PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

        // ── Decomposition mode ────────────────────────────────────────────────
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "7"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(PhysicsActiveMeshCompoundComponent::AttrConvexDecompositionMode().c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->convexDecompositionMode->getListSelectedValue())));
            propertiesXML->append_node(node);
        }

        // ── Hull tolerance ────────────────────────────────────────────────────
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "6"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(PhysicsActiveMeshCompoundComponent::AttrHullTolerance().c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->hullTolerance->getReal())));
            propertiesXML->append_node(node);
        }
    }

    // =========================================================================
    //  createDynamicBody  – the core override
    // =========================================================================

    bool PhysicsActiveMeshCompoundComponent::createDynamicBody(void)
    {
        this->destroyCollision();
        this->destroyBody();

        // Refresh transform snapshot in case the GO was moved since postInit
        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
        Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;
        Ogre::Real partVolume = 0.0f;

        std::vector<OgreNewt::CollisionPtr> collisionList;

        // ── Step 1: Extract vertices and build convex hulls on the render thread ─
        // Both readRequests/mapAsyncTickets (vertex extraction) and
        // OgreNewt::CollisionPrimitives::ConvexHull (Newton world access)
        // must run on the render thread.
        bool meshExtracted = false;

        NOWA::GraphicsModule::RenderCommand renderCmd = [this, &collisionList, &partVolume, &inertia, &meshExtracted]()
        {
            meshExtracted = this->buildCollisionListFromMesh(collisionList, partVolume, inertia);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCmd), "PhysicsActiveMeshCompoundComponent::buildCollisionListFromMesh");

        if (!meshExtracted || collisionList.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveMeshCompoundComponent] createDynamicBody: no convex hulls "
                                                                                "could be created for: " +
                                                                                    this->gameObjectPtr->getName() + ". Ensure the GameObject has an Ogre::Item with accessible vertex data.");
            return false;
        }

        // ── Step 2: Record accumulated volume ─────────────────────────────────
        this->volume = partVolume;
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[PhysicsActiveMeshCompoundComponent] Total volume: " + Ogre::StringConverter::toString(this->volume) + "  hulls: " + Ogre::StringConverter::toString(collisionList.size()) + "  GO: " + this->gameObjectPtr->getName());

        // ── Step 3: Assemble CompoundCollision from the hull list ─────────────
        OgreNewt::CollisionPrimitives::CompoundCollision* compoundCollision = new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, collisionList, this->gameObjectPtr->getCategoryId());

        OgreNewt::CollisionPtr collisionPtr = OgreNewt::CollisionPtr(compoundCollision);

        OgreNewt::ConvexCollisionPtr convexCollisionPtr = std::static_pointer_cast<OgreNewt::ConvexCollision>(collisionPtr);

        convexCollisionPtr->calculateInertialMatrix(inertia, calculatedMassOrigin);

        // ── Step 4: Create the Newton body ────────────────────────────────────
        this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), convexCollisionPtr);

        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

        this->setPosition(this->initialPosition);
        this->setOrientation(this->initialOrientation);

        this->physicsBody->setGravity(this->gravity->getVector3());

        // Tag the collision type so the editor property panel shows "Compound"
        this->collisionType->setValue("Compound");

        // Apply user-specified mass-origin override if set
        if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
        {
            calculatedMassOrigin = this->massOrigin->getVector3();
        }

        // ── Step 5: Mass and inertia ──────────────────────────────────────────
        inertia *= this->mass->getReal();
        this->physicsBody->setMassMatrix(this->mass->getReal(), inertia);
        this->physicsBody->setCenterOfMass(calculatedMassOrigin);

        // ── Step 6: Damping ───────────────────────────────────────────────────
        if (this->linearDamping->getReal() != 0.0f)
        {
            this->physicsBody->setLinearDamping(this->linearDamping->getReal());
        }
        if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
        }

        this->setConstraintAxis(this->constraintAxis->getVector3());
        this->setConstraintDirection(this->constraintDirection->getVector3());

        this->setActivated(this->activated->getBool());
        this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

        // ── Step 7: Wire body to scene node ───────────────────────────────────
        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        this->setCollidable(this->collidable->getBool());
        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] Body created successfully for: " + this->gameObjectPtr->getName());

        return true;
    }

    // =========================================================================
    //  extractVerticesFromMesh  (render thread)
    // =========================================================================

    bool PhysicsActiveMeshCompoundComponent::extractVerticesFromMesh(std::vector<std::vector<Ogre::Vector3>>& outVerticesPerSubMesh)
    {
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr == item)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveMeshCompoundComponent] extractVerticesFromMesh: "
                                                                                "no Ogre::Item on game object: " +
                                                                                    this->gameObjectPtr->getName());
            return false;
        }

        Ogre::MeshPtr mesh = item->getMesh();
        if (mesh.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveMeshCompoundComponent] extractVerticesFromMesh: "
                                                                                "Ogre::Item has no mesh on game object: " +
                                                                                    this->gameObjectPtr->getName());
            return false;
        }

        outVerticesPerSubMesh.clear();
        unsigned int totalExtracted = 0;

        for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
        {
            if (subMesh->mVao[Ogre::VpNormal].empty())
            {
                continue;
            }

            Ogre::VertexArrayObject* vao = subMesh->mVao[Ogre::VpNormal][0];

            // ── Detect position element type (FLOAT3 or HALF4) ─────────────────
            Ogre::VertexElementType posType = Ogre::VET_FLOAT3;
            size_t posStride = 0;

            for (const Ogre::VertexBufferPacked* vb : vao->getVertexBuffers())
            {
                for (const Ogre::VertexElement2& elem : vb->getVertexElements())
                {
                    if (elem.mSemantic == Ogre::VES_POSITION)
                    {
                        posType = elem.mType;
                        posStride = vb->getBytesPerElement();
                        goto foundPosFormat; // break out of both loops
                    }
                }
            }
        foundPosFormat:;

            if (posStride == 0)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] Submesh has no VES_POSITION — skipped.");
                continue;
            }

            // ── Async GPU readback: position channel only ─────────────────────
            Ogre::VertexArrayObject::ReadRequestsVec reqs;
            reqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

            vao->readRequests(reqs);
            vao->mapAsyncTickets(reqs);

            const unsigned int numVerts = static_cast<unsigned int>(reqs[0].vertexBuffer->getNumElements());

            std::vector<Ogre::Vector3> subMeshVerts;
            subMeshVerts.reserve(numVerts);

            const Ogre::uint8* posData = reinterpret_cast<const Ogre::uint8*>(reqs[0].data);
            const size_t stride = reqs[0].vertexBuffer->getBytesPerElement();

            for (unsigned int i = 0; i < numVerts; ++i)
            {
                Ogre::Vector3 pos = this->readPositionVertex(posData, reqs[0].type, stride);
                subMeshVerts.push_back(pos);
            }

            vao->unmapAsyncTickets(reqs);

            if (!subMeshVerts.empty())
            {
                totalExtracted += static_cast<unsigned int>(subMeshVerts.size());
                outVerticesPerSubMesh.push_back(std::move(subMeshVerts));
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] Extracted " + Ogre::StringConverter::toString(totalExtracted) + " vertices across " +
                                                                               Ogre::StringConverter::toString(outVerticesPerSubMesh.size()) + " submesh(es) for: " + this->gameObjectPtr->getName());

        return !outVerticesPerSubMesh.empty();
    }

    // ── Read one position vertex and advance the data pointer ─────────────────

    Ogre::Vector3 PhysicsActiveMeshCompoundComponent::readPositionVertex(const Ogre::uint8*& data, Ogre::VertexElementType posType, size_t stride) const
    {
        Ogre::Vector3 pos;

        if (posType == Ogre::VET_HALF4 || posType == Ogre::VET_HALF2)
        {
            // Compressed half-float format (e.g. VET_HALF4: x y z w, ignore w)
            const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(data);
            pos.x = Ogre::Bitwise::halfToFloat(p[0]);
            pos.y = Ogre::Bitwise::halfToFloat(p[1]);
            pos.z = Ogre::Bitwise::halfToFloat(p[2]);
        }
        else
        {
            // Standard VET_FLOAT3 (or VET_FLOAT4 — we only read x y z)
            const float* p = reinterpret_cast<const float*>(data);
            pos.x = p[0];
            pos.y = p[1];
            pos.z = p[2];
        }

        data += stride;
        return pos;
    }

    // =========================================================================
    //  buildCollisionListFromMesh  (render thread)
    // =========================================================================

    bool PhysicsActiveMeshCompoundComponent::buildCollisionListFromMesh(std::vector<OgreNewt::CollisionPtr>& collisionList, Ogre::Real& partVolume, Ogre::Vector3& inertia)
    {
        // ── 1. Extract per-submesh vertex positions ────────────────────────────
        std::vector<std::vector<Ogre::Vector3>> verticesPerSubMesh;
        if (!this->extractVerticesFromMesh(verticesPerSubMesh))
        {
            return false;
        }

        // ── 2. Scale every vertex to world space (Newton works in world units) ─
        // Note: only uniform scale is meaningful for physics.
        const Ogre::Vector3& scale = this->initialScale;

        for (auto& submeshVerts : verticesPerSubMesh)
        {
            for (Ogre::Vector3& v : submeshVerts)
            {
                v *= scale;
            }
        }

        // ── 3. Assemble vertex groups according to the chosen mode ─────────────
        const Ogre::String& mode = this->convexDecompositionMode->getListSelectedValue();

        const Ogre::Real tolerance = this->hullTolerance->getReal();

        std::vector<std::vector<Ogre::Vector3>> hullGroups;

        if (mode == "Single Hull")
        {
            // Merge all submeshes into one flat vertex cloud
            std::vector<Ogre::Vector3> allVerts;
            for (const auto& sv : verticesPerSubMesh)
            {
                allVerts.insert(allVerts.end(), sv.begin(), sv.end());
            }
            hullGroups.push_back(std::move(allVerts));
        }
        else if (mode == "Per Sub-Mesh")
        {
            // One group per submesh
            for (auto& sv : verticesPerSubMesh)
            {
                if (!sv.empty())
                {
                    hullGroups.push_back(std::move(sv));
                }
            }
        }
        else
        {
            // "Octant NxNxN" — determine grid dimension from the mode string
            int divisions = 2; // default: 2x2x2
            if (mode == "Octant 3x3x3")
            {
                divisions = 3;
            }

            // Merge all vertices into one cloud, then split by uniform grid
            std::vector<Ogre::Vector3> allVerts;
            for (const auto& sv : verticesPerSubMesh)
            {
                allVerts.insert(allVerts.end(), sv.begin(), sv.end());
            }

            this->splitVerticesByOctant(allVerts, divisions, hullGroups);
        }

        if (hullGroups.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveMeshCompoundComponent] No hull groups produced "
                                                                                "for mode '" +
                                                                                    mode + "' on: " + this->gameObjectPtr->getName());
            return false;
        }

        // ── 4. Create one OgreNewt ConvexHull per group ───────────────────────
        unsigned int hullsCreated = 0;

        for (const auto& group : hullGroups)
        {
            OgreNewt::CollisionPtr hull = this->tryCreateConvexHull(group, tolerance);

            if (nullptr != hull)
            {
                // calculateVolume() is available on ConvexCollision
                auto convexHull = std::static_pointer_cast<OgreNewt::ConvexCollision>(hull);
                partVolume += convexHull->calculateVolume();
                collisionList.emplace_back(hull);
                ++hullsCreated;
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[PhysicsActiveMeshCompoundComponent] Created " + Ogre::StringConverter::toString(hullsCreated) + " / " + Ogre::StringConverter::toString(hullGroups.size()) + " hull(s) using mode '" + mode + "' for: " + this->gameObjectPtr->getName());

        return hullsCreated > 0;
    }

    // =========================================================================
    //  splitVerticesByOctant
    // =========================================================================

    void PhysicsActiveMeshCompoundComponent::splitVerticesByOctant(const std::vector<Ogre::Vector3>& allVertices, int divisions, std::vector<std::vector<Ogre::Vector3>>& outGroups)
    {
        if (allVertices.empty())
        {
            return;
        }

        // ── Compute tight AABB ────────────────────────────────────────────────
        Ogre::Vector3 minBB(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

        for (const Ogre::Vector3& v : allVertices)
        {
            minBB.makeFloor(v);
            maxBB.makeCeil(v);
        }

        const Ogre::Vector3 extent = maxBB - minBB;

        // Guard: if any dimension is degenerate (flat plane / line), fall back
        // to a single hull to avoid division by near-zero.
        const float minExtent = 1e-4f;
        if (extent.x < minExtent || extent.y < minExtent || extent.z < minExtent)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] splitVerticesByOctant: "
                                                                               "degenerate extent detected — falling back to Single Hull.");
            outGroups.push_back(allVertices);
            return;
        }

        // ── Place every vertex into its grid cell ─────────────────────────────
        const int totalCells = divisions * divisions * divisions;
        std::vector<std::vector<Ogre::Vector3>> cells(totalCells);

        for (const Ogre::Vector3& v : allVertices)
        {
            // Normalised [0,1) position within the AABB
            int ix = static_cast<int>((v.x - minBB.x) / extent.x * divisions);
            int iy = static_cast<int>((v.y - minBB.y) / extent.y * divisions);
            int iz = static_cast<int>((v.z - minBB.z) / extent.z * divisions);

            // Clamp — vertices exactly on maxBB would otherwise go out of range
            ix = std::min(ix, divisions - 1);
            iy = std::min(iy, divisions - 1);
            iz = std::min(iz, divisions - 1);

            const int cellIdx = ix + iy * divisions + iz * divisions * divisions;

            cells[cellIdx].push_back(v);
        }

        // ── Merge cells with fewer than 4 vertices into the nearest non-empty ─
        // Newton's ConvexHull requires at least 4 non-coplanar points.
        // We merge tiny cells rather than silently discarding them so the total
        // vertex count is preserved and no geometry is lost.
        const int minHullVerts = 4;
        std::vector<Ogre::Vector3> overflow;

        for (auto& cell : cells)
        {
            if (cell.size() > 0 && static_cast<int>(cell.size()) < minHullVerts)
            {
                // Defer to the overflow bucket — we'll merge it after
                overflow.insert(overflow.end(), cell.begin(), cell.end());
                cell.clear();
            }
        }

        // Pour overflow into the first non-empty cell so they are not lost
        if (!overflow.empty())
        {
            bool placed = false;
            for (auto& cell : cells)
            {
                if (!cell.empty())
                {
                    cell.insert(cell.end(), overflow.begin(), overflow.end());
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                // Every cell had too few vertices: just create one single hull
                outGroups.push_back(allVertices);
                return;
            }
        }

        // ── Transfer non-empty cells to outGroups ─────────────────────────────
        for (auto& cell : cells)
        {
            if (!cell.empty())
            {
                outGroups.push_back(std::move(cell));
            }
        }

        if (outGroups.empty())
        {
            // Degenerate fallback
            outGroups.push_back(allVertices);
        }
    }

    // =========================================================================
    //  tryCreateConvexHull  (render thread)
    // =========================================================================

    OgreNewt::CollisionPtr PhysicsActiveMeshCompoundComponent::tryCreateConvexHull(const std::vector<Ogre::Vector3>& vertices, Ogre::Real tolerance)
    {
        // Newton requires at least 4 non-coplanar points to build a valid hull.
        // We check the count here; Newton itself will further validate coplanarity.
        if (vertices.size() < 4)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] tryCreateConvexHull: "
                                                                               "vertex group has only " +
                                                                                   Ogre::StringConverter::toString(vertices.size()) + " vertices — skipped (minimum 4 required).");
            return nullptr;
        }

        OgreNewt::CollisionPrimitives::ConvexHull* hull = nullptr;

        try
        {
            hull = new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, &vertices[0], vertices.size(), this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, static_cast<float>(tolerance));
        }
        catch (const std::exception& ex)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveMeshCompoundComponent] tryCreateConvexHull exception: " + Ogre::String(ex.what()));
            return nullptr;
        }

        if (nullptr == hull)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveMeshCompoundComponent] tryCreateConvexHull: Newton "
                                                                               "rejected a hull (likely coplanar / degenerate vertices) — skipped.");
            return nullptr;
        }

        return OgreNewt::CollisionPtr(hull);
    }

    // =========================================================================
    //  Attribute setters / getters
    // =========================================================================

    void PhysicsActiveMeshCompoundComponent::setConvexDecompositionMode(const Ogre::String& mode)
    {
        this->convexDecompositionMode->setListSelectedValue(mode);

        // Rebuild the physics body immediately if one already exists
        if (nullptr != this->physicsBody)
        {
            this->createDynamicBody();
        }
    }

    Ogre::String PhysicsActiveMeshCompoundComponent::getConvexDecompositionMode(void) const
    {
        return this->convexDecompositionMode->getListSelectedValue();
    }

    void PhysicsActiveMeshCompoundComponent::setHullTolerance(Ogre::Real tolerance)
    {
        // Clamp to a sane range: below 1e-6 Newton may crash; above 0.5 is useless
        tolerance = std::max(1e-6f, std::min(0.5f, static_cast<float>(tolerance)));
        this->hullTolerance->setValue(tolerance);

        if (nullptr != this->physicsBody)
        {
            this->createDynamicBody();
        }
    }

    Ogre::Real PhysicsActiveMeshCompoundComponent::getHullTolerance(void) const
    {
        return this->hullTolerance->getReal();
    }

} // namespace NOWA
