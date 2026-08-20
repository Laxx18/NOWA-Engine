/*
Copyright (c) 2025 Lukas Kalinowski
GPL v3
*/

#ifndef GAMEOBJECTPLACECOMPONENT_H
#define GAMEOBJECTPLACECOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

namespace Ogre
{
    class Terra;
}

namespace NOWA
{
    class PhysicsComponent;

    /**
     * @brief   Enables placing/cloning pre-configured template game objects in the world during simulation.
     *          Configure template game object IDs in the editor. Call activatePlacement(id) from Lua
     *          (e.g. on inventory drop event). Move preview with mouse, left-click to place, right-click or ESC to cancel.
     *
     * Workflow:
     *   1. In NOWA-Design: create the template game objects (houses etc.) with all components/config, set visible=false.
     *   2. Add this component to a manager game object. Configure the template IDs in the PlaceObjectCount list.
     *   3. In Lua, react to inventory drop: call gameObjectPlaceComp:activatePlacement("12345678")
     *   4. A bare, component-less preview (mesh + datablocks copied from the template) becomes visible and
     *      follows the mouse. The template game object itself is never made visible, moved, or otherwise
     *      touched during this — it has no involvement in the physics simulation at any point.
     *   5. Left-click: the template is cloned (with all its components) at the world position, the preview
     *      is destroyed, reactOnGameObjectPlaced fires.
     *   6. Right-click or ESC: cancel, preview is destroyed, reactOnPlacementCancelled fires.
     */
    class EXPORTED GameObjectPlaceComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<GameObjectPlaceComponent> GameObjectPlaceComponentPtr;

    public:
        GameObjectPlaceComponent();
        virtual ~GameObjectPlaceComponent();

        virtual void install(const Ogre::NameValuePairList* options) override;

        virtual void initialise() override;

        virtual void shutdown() override;

        virtual void uninstall() override;

        virtual const Ogre::String& getName() const override;

        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        virtual bool postInit(void) override;

        virtual bool connect(void) override;

        virtual bool disconnect(void) override;

        virtual bool onCloned(void) override;

        virtual void onRemoveComponent(void) override;

        virtual void onOtherComponentRemoved(unsigned int index) override;

        virtual void onOtherComponentAdded(unsigned int index) override;

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        virtual void actualizeValue(Variant* attribute) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        void setCategories(const Ogre::String& categories);

        Ogre::String getCategories(void) const;

        void setShowPreview(bool showPreview);

        bool getShowPreview(void) const;

        void setRotateEnabled(bool rotateEnabled);

        bool getRotateEnabled(void) const;

        void setAlignToTerrain(bool alignToTerrain);

        bool getAlignToTerrain() const;

        void setSpacing(Ogre::Real spacing);

        Ogre::Real getSpacing(void) const;

        void setMaxPlacementGradient(Ogre::Real maxGradientDegrees);

        Ogre::Real getMaxPlacementGradient(void) const;

        void setTargetTerraId(unsigned long id);

        unsigned long getTargetTerraId(void) const;

        void setForbiddenTerraLayers(const Ogre::String& terraLayers);

        Ogre::String getForbiddenTerraLayers(void) const;

        /**
         * @brief Sets the number of template game object slots.
         */
        void setPlaceObjectCount(unsigned int count);

        unsigned int getPlaceObjectCount(void) const;

        /**
         * @brief Sets the template game object id for the given slot index.
         * @param[in] index  Slot index.
         * @param[in] id     Game object id as string (unsigned long).
         */
        void setGameObjectId(unsigned int index, const Ogre::String& id);

        Ogre::String getGameObjectId(unsigned int index) const;

        /**
         * @brief Called from Lua to begin placement mode for the given template game object id.
         *        A bare preview (mesh + datablocks copied from the template) becomes visible and
         *        tracks the mouse until placed or cancelled. The template itself is never touched.
         * @param[in] gameObjectId  Id of the pre-created template game object to preview and clone from.
         */
        void activatePlacement(const Ogre::String& gameObjectId);

        /**
         * @brief Cancels active placement mode programmatically.
         */
        void cancelPlacement(void);

        /**
         * @brief Lua closure called after a game object was successfully placed.
         *        Signature: function(newGameObjectId: string)
         */
        void reactOnGameObjectPlaced(luabind::object closureFunction);

        /**
         * @brief Lua closure called when placement is cancelled.
         *        Signature: function()
         */
        void reactOnPlacementCancelled(luabind::object closureFunction);

    public:
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("GameObjectPlaceComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "GameObjectPlaceComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Enables placing/cloning pre-configured template game objects during simulation. "
                   "Pre-create template objects in the editor (set visible=false), configure their IDs here, "
                   "then call activatePlacement(id) from Lua. Left-click places, right-click/ESC cancels.";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrCategories(void)
        {
            return "Categories";
        }
        static const Ogre::String AttrShowPreview(void)
        {
            return "Show Preview";
        }
        static Ogre::String AttrRotateEnabled()
        {
            return "Rotate Enabled";
        }
        static Ogre::String AttrAlignToTerrain()
        {
            return "Align To Terrain";
        }
        static const Ogre::String AttrSpacing(void)
        {
            return "Spacing";
        }
        static const Ogre::String AttrMaxPlacementGradient(void)
        {
            return "Max Placement Gradient";
        }
        static const Ogre::String AttrTargetTerraId(void)
        {
            return "Target Terra Id";
        }
        static const Ogre::String AttrForbiddenTerraLayers(void)
        {
            return "Forbidden Terra Layers";
        }
        static const Ogre::String AttrPlaceObjectCount(void)
        {
            return "Place Object Count";
        }
        static const Ogre::String AttrGameObjectId(void)
        {
            return "GameObject Id";
        }

    protected:
        virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

        virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

    private:
        /**
         * @brief Destroys the preview object (if any) and resets placement state.
         */
        void endPlacement(bool cancelled);

        /**
         * @brief Casts a ray from mouse screen position and returns the world hit point.
         *        Returns Ogre::Vector3::ZERO if nothing was hit.
         */
        Ogre::Vector3 getMouseWorldPosition(const OIS::MouseEvent& evt);

        /**
         * @brief Builds a bare preview SceneNode + Item (mesh and per-subitem datablocks copied from
         *        templateGameObjectPtr) on the render thread. No GameObject, no components — in particular
         *        no PhysicsComponent, so the preview can never participate in collision/physics resolution
         *        while being dragged around the world. Returns false if the template has no Item movable.
         */
        bool createPreviewObject(GameObjectPtr templateGameObjectPtr);

        /**
         * @brief Destroys the preview SceneNode + Item on the render thread, if present.
         */
        void destroyPreviewObject(void);

        void applyPreviewTransparency(void);

        void resetPreviewTransparency(void);

        void parseExcludedCategories(const Ogre::String& categories);

        void applyForbiddenVisual(void);

        void resetForbiddenVisual(void);

        Ogre::Quaternion computeTerrainAlignOrientation(const Ogre::Vector3& hitPoint, const Ogre::Quaternion& baseYRotation);

        bool checkExcludedCategoryOverlap(const Ogre::Vector3& position);

        bool checkGradientForbidden(const Ogre::Vector3& position) const;

        void checkAndSetForbiddenTerraLayers(const Ogre::String& terraLayers);

        bool checkTerraLayerForbidden(const Ogre::Vector3& position) const;

    private:
        Ogre::String name;

        Variant* activated;
        Variant* categories;
        Variant* showPreview;
        Variant* rotateEnabled;
        Variant* alignToTerrain;
        Variant* spacing;
        Variant* maxPlacementGradient;
        Variant* targetTerraId;
        Variant* forbiddenTerraLayers;
        Variant* placeObjectCount;
        std::vector<Variant*> gameObjectIds;

        unsigned long activeGameObjectId; // template game object id currently being previewed/placed
        bool isPlacing;
        Ogre::Vector3 currentHitPoint;

        Ogre::RaySceneQuery* raySceneQuery;
        Ogre::RaySceneQuery* terrainRayQuery;
        Ogre::PlaneBoundedVolumeListSceneQuery* volumeQuery;

        luabind::object placedClosureFunction;
        luabind::object cancelledClosureFunction;

        Ogre::uint32 categoryId;

        // Bare preview object — no GameObject, no components (no PhysicsComponent in particular).
        // Built fresh from the template's mesh/datablocks in createPreviewObject(), destroyed in
        // destroyPreviewObject(). This is the entire fix for the physics-push bug: there is nothing
        // here for Newton to resolve collisions against.
        Ogre::SceneNode* previewSceneNode;
        Ogre::Item* previewItem;

        Ogre::Real currentRotationDegrees; // accumulated Y rotation via mousewheel
        // Per-subitem cloned datablocks for transparency — key = subitem index
        std::vector<std::pair<Ogre::HlmsDatablock*, unsigned int>> clonedDatablocks;

        Ogre::uint32 excludedCategoryId;
        bool isOnForbiddenSurface;
        bool isForbiddenVisualActive;
        std::vector<std::pair<Ogre::HlmsDatablock*, unsigned int>> forbiddenClonedDatablocks;
        Ogre::MovableObject* lastHitObject;
        Ogre::Quaternion currentPlacementOrientation;
        std::vector<int> forbiddenTerraLayerList;
        Ogre::Terra* terra;
    };

} // namespace end

#endif // GAMEOBJECTPLACECOMPONENT_H