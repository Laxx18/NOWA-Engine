/*
Copyright (c) 2025 Lukas Kalinowski
GPL v3
*/

#ifndef GAMEOBJECTPLACECOMPONENT_H
#define GAMEOBJECTPLACECOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{
    class PhysicsComponent;

    /**
     * @brief   Enables placing/cloning pre-configured shadow game objects in the world during simulation.
     *          Configure shadow game object IDs in the editor. Call activatePlacement(id) from Lua
     *          (e.g. on inventory drop event). Move preview with mouse, left-click to place, right-click or ESC to cancel.
     *
     * Workflow:
     *   1. In NOWA-Design: create the shadow game objects (houses etc.) with all components/config, set visible=false.
     *   2. Add this component to a manager game object. Configure the shadow IDs in the PlaceObjectCount list.
     *   3. In Lua, react to inventory drop: call gameObjectPlaceComp:activatePlacement("12345678")
     *   4. The shadow object becomes visible and follows the mouse as a preview.
     *   5. Left-click: clone is created at world position, shadow is hidden again, reactOnGameObjectPlaced fires.
     *   6. Right-click or ESC: cancel, reactOnPlacementCancelled fires.
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

        /**
         * @brief Sets the number of shadow game object slots.
         */
        void setPlaceObjectCount(unsigned int count);

        unsigned int getPlaceObjectCount(void) const;

        /**
         * @brief Sets the shadow game object id for the given slot index.
         * @param[in] index  Slot index.
         * @param[in] id     Game object id as string (unsigned long).
         */
        void setGameObjectId(unsigned int index, const Ogre::String& id);

        Ogre::String getGameObjectId(unsigned int index) const;

        /**
         * @brief Called from Lua to begin placement mode for the given shadow game object id.
         *        The shadow object becomes visible and tracks the mouse until placed or cancelled.
         * @param[in] gameObjectId  Id of the pre-created shadow game object to use as preview and clone source.
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
            return "Usage: Enables placing/cloning pre-configured shadow game objects during simulation. "
                   "Pre-create shadow objects in the editor (set visible=false), configure their IDs here, "
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
        static const Ogre::String AttrPlaceObjectCount(void)
        {
            return "Place Object Count";
        }
        static const Ogre::String AttrGameObjectId(void)
        {
            return "GameObjectId";
        }

    protected:
        virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

        virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

    private:
        /**
         * @brief Hides the shadow object and resets placement state.
         */
        void endPlacement(bool cancelled);

        /**
         * @brief Casts a ray from mouse screen position and returns the world hit point.
         *        Returns Ogre::Vector3::ZERO if nothing was hit.
         */
        Ogre::Vector3 getMouseWorldPosition(const OIS::MouseEvent& evt);

        void resolveShadowPhysicsComponent(void);

        void applyPreviewTransparency(GameObjectPtr shadowGameObjectPtr);

        void resetPreviewTransparency(GameObjectPtr shadowGameObjectPtr);

    private:
        Ogre::String name;

        Variant* activated;
        Variant* categories;
        Variant* showPreview;
        Variant* placeObjectCount;
        std::vector<Variant*> gameObjectIds;

        unsigned long activeGameObjectId; // shadow game object currently being placed
        bool isPlacing;
        Ogre::Vector3 currentHitPoint;

        Ogre::RaySceneQuery* raySceneQuery;

        luabind::object placedClosureFunction;
        luabind::object cancelledClosureFunction;

        unsigned int categoryId;
        
        PhysicsComponent* shadowPhysicsComponent;
        bool oldWasDynamic;
    };

} // namespace end

#endif // GAMEOBJECTPLACECOMPONENT_H