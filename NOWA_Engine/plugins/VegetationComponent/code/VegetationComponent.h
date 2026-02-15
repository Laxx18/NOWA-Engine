/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef VEGETATION_COMPONENT_H
#define VEGETATION_COMPONENT_H

#include "NOWAPrecompiled.h"
#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include <atomic>
#include <vector>

namespace Ogre
{
    class Terra;
    class RaySceneQuery;
    namespace v2
    {
        class Item;
    }
}

namespace NOWA
{
    /**
     * @class VegetationComponent
     * @brief Ogre-Next vegetation system with automatic Hlms batching
     *
     * HOW IT WORKS (Ogre-Next):
     * 1. Create multiple Ogre::Item objects from same mesh
     * 2. Share ONE HlmsDatablock between all Items of same type
     * 3. Hlms automatically batches them into instanced draw calls
     * 4. Result: 100k Items = ~10 draw calls per mesh type
     *
     * PERFORMANCE:
     * - With Hlms batching: 100k instances = ~10 draw calls ✓
     * - Without batching: 100k instances = 100k draw calls ✗
     *
     * THREADING:
     * - Position calculation: Parallel on main thread (thread-safe)
     * - Ogre object creation: Serialized on render thread (required)
     *
     * INTEGRATION:
     * - Uses your GraphicsModule::enqueue() for render thread dispatch
     * - Uses your GraphicsModule::enqueueAndWait() for blocking cleanup
     */
    class EXPORTED VegetationComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<VegetationComponent> VegetationComponentPtr;

        // ============================================================================
        // Helper structures for vegetation instance data (CPU-side)
        // ============================================================================
        struct VegetationInstance
        {
            Ogre::Vector3 position;
            Ogre::Quaternion orientation;
            Ogre::Vector3 scale;

            VegetationInstance() = default;
            VegetationInstance(const Ogre::Vector3& pos, const Ogre::Quaternion& rot, const Ogre::Vector3& scl) : position(pos), orientation(rot), scale(scl)
            {
            }
        };

        // Per-mesh-type batch (Ogre-Next automatically batches Items with same datablock)
        struct VegetationBatch
        {
            Ogre::String meshName;
            std::vector<VegetationInstance> instances;

            // Render thread only! Never access from main thread
            std::vector<Ogre::Item*> items; // V2 Items (Ogre-Next)
            std::vector<Ogre::SceneNode*> nodes;

            VegetationBatch() = default;
            VegetationBatch(const Ogre::String& name) : meshName(name)
            {
            }
        };
    public:
        VegetationComponent();
        virtual ~VegetationComponent();

        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see		Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see		Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see		Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::onOtherComponentAdded
         */
        virtual void onOtherComponentAdded(unsigned int index) override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("VegetationComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "VegetationComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Create different types of roads with realistic terrain following.";
        }

        virtual Ogre::String getClassName(void) const override
        {
            return "VegetationComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

        void setTargetGameObjectId(unsigned long targetGameObjectId);
        unsigned long getTargetGameObjectId(void) const;

        void setDensity(Ogre::Real density);
        Ogre::Real getDensity(void) const;

        void setPositionXZ(const Ogre::Vector2& positionXZ);
        Ogre::Vector2 getPositionXZ(void) const;

        void setScale(Ogre::Real scale);
        Ogre::Real getScale(void) const;

        void setAutoOrientation(bool autoOrientation);
        bool getAutoOrientation(void) const;

        void setCategories(const Ogre::String& categories);
        Ogre::String getCategories(void) const;

        void setMaxHeight(Ogre::Real maxHeight);
        Ogre::Real getMaxHeight(void) const;

        void setRenderDistance(Ogre::Real renderDistance);
        Ogre::Real getRenderDistance(void) const;

        void setTerraLayers(const Ogre::String& terraLayers);
        Ogre::String getTerraLayers(void) const;

        void setVegetationTypesCount(unsigned int vegetationTypesCount);
        unsigned int getVegetationTypesCount(void) const;

        void setVegetationMeshName(unsigned int index, const Ogre::String& vegetationMeshName);
        Ogre::String getVegetationMeshName(unsigned int index) const;

        /**
         * @brief Set cluster size for spatial grouping (future LOD/culling)
         */
        void setClusterSize(Ogre::Real clusterSize);
        Ogre::Real getClusterSize(void) const;

    public:
        static const Ogre::String AttrRegenerate(void)
        {
            return "Regenerate";
        }
        static const Ogre::String AttrTargetGameObjectId(void)
        {
            return "Target GameObject Id";
        }
        static const Ogre::String AttrDensity(void)
        {
            return "Density";
        }
        static const Ogre::String AttrPositionXZ(void)
        {
            return "Position XZ";
        }
        static const Ogre::String AttrScale(void)
        {
            return "Scale";
        }
        static const Ogre::String AttrAutoOrientation(void)
        {
            return "Auto Orientation";
        }
        static const Ogre::String AttrCategories(void)
        {
            return "Categories";
        }
        static const Ogre::String AttrMaxHeight(void)
        {
            return "Max Height";
        }
        static const Ogre::String AttrRenderDistance(void)
        {
            return "Render Distance";
        }
        static const Ogre::String AttrTerraLayers(void)
        {
            return "Terra Layers";
        }
        static const Ogre::String AttrVegetationTypesCount(void)
        {
            return "Vegetation Types Count";
        }
        static const Ogre::String AttrVegetationMeshName(void)
        {
            return "Vegetation Mesh Name ";
        }
        static const Ogre::String AttrClusterSize(void)
        {
            return "Cluster Size";
        }

    protected:
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

        /**
         * @brief Main regeneration (runs on main thread)
         */
        void regenerateVegetation();

        /**
         * @brief Calculate positions (parallel, main thread workers)
         * @return Batches with calculated instance transforms
         */
        std::vector<VegetationBatch> calculateVegetationPositions();

        /**
         * @brief Dispatch to render thread
         */
        void createVegetationOnRenderThread(std::vector<VegetationBatch>&& batches);

        /**
         * @brief Create Ogre Items on render thread
         * MUST RUN ON RENDER THREAD!
         */
        void createVegetationItems(std::vector<VegetationBatch>& batches);

        /**
         * @brief Destroy all vegetation (dispatch to render thread)
         */
        void destroyVegetation();

        /**
         * @brief Actually destroy Ogre objects
         * MUST RUN ON RENDER THREAD!
         */
        void destroyVegetationOnRenderThread();

        void checkAndSetTerraLayers(const Ogre::String& terraLayers);

        void handleUpdateBounds(NOWA::EventDataPtr eventData);
        void handleSceneParsed(NOWA::EventDataPtr eventData);

    protected:
        Ogre::String name;
        Ogre::Vector3 minimumBounds;
        Ogre::Vector3 maximumBounds;
        Ogre::RaySceneQuery* raySceneQuery;
        std::vector<int> terraLayerList;

        // Vegetation data (render thread only after creation!)
        std::vector<VegetationBatch> vegetationBatches;

        // Attributes
        Variant* regenerate;
        Variant* targetGameObjectId;
        Variant* density;
        Variant* positionXZ;
        Variant* scale;
        Variant* autoOrientation;
        Variant* categories;
        Variant* maxHeight;
        Variant* renderDistance;
        Variant* terraLayers;
        Variant* vegetationTypesCount;
        Variant* clusterSize;

        std::vector<Variant*> vegetationMeshNames;
    };

}; // namespace end

#endif
