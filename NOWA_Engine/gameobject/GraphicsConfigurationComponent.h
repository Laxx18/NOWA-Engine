/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef GRAPHICSCONFIGURATIONCOMPONENT_H
#define GRAPHICSCONFIGURATIONCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{

    /**
     * @brief		This component can be used as building block in order to have an in game graphics configuration menu.
     *				It replaces the Ogre configuration dialog, which is unusable on console like targets such as Steam Deck.
     *				It can be placed as root via the relative position or using a parent id to be placed as a child in a parent MyGUI window.
     * @note		Only Direct3D11 options are configurable. Options like floating point mode, fast shader build hack,
     *				vendor extensions and sRGB gamma conversion are fixed by the engine and are not exposed here.
     * @note		Render distance and shadow far distance are NOT exposed here, because those are authored per scene by the
     *				level designer via ProjectParameter and must not be overridden by the player.
     */
    class EXPORTED GraphicsConfigurationComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<GraphicsConfigurationComponent> GraphicsConfigurationComponentPtr;

    public:
        GraphicsConfigurationComponent();

        virtual ~GraphicsConfigurationComponent();

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
        virtual void onRemoveComponent(void) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

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

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        /**
         * @see		GameObjectComponent::isActivated
         */
        virtual bool isActivated(void) const override;

        /**
         * @brief		Sets the relative position of the graphics configuration window (0 = top/left, 1 = bottom/right).
         * @param[in]	relativePosition	The relative position to set.
         */
        void setRelativePosition(const Ogre::Vector2& relativePosition);

        /**
         * @brief		Gets the relative position of the graphics configuration window.
         * @return		The current relative position.
         */
        Ogre::Vector2 getRelativePosition(void) const;

        /**
         * @brief		Sets the relative size of the graphics configuration window.
         * @param[in]	relativeSize	The relative size to set.
         */
        void setRelativeSize(const Ogre::Vector2& relativeSize);

        /**
         * @brief		Gets the relative size of the graphics configuration window.
         * @return		The current relative size.
         */
        Ogre::Vector2 getRelativeSize(void) const;

        /**
         * @brief		Sets the skin, which is used for the graphics configuration window.
         * @param[in]	skin	The skin to set, e.g. 'WoodWindow'.
         */
        void setSkin(const Ogre::String& skin);

        /**
         * @brief		Gets the used skin name.
         * @return		The skin name to get.
         */
        Ogre::String getSkin(void) const;

        /**
         * @brief		Sets the MyGUI layer on which the window is placed. Must be a layer that receives mouse picking,
         *				e.g. 'Overlapped'. Layers reserved for passive overlays (like a tooltip layer) never receive
         *				mouse focus, so widgets placed there cannot be interacted with at all, not even natively by MyGUI.
         * @param[in]	layer	The layer to set.
         */
        void setLayer(const Ogre::String& layer);

        /**
         * @brief		Gets the used MyGUI layer name.
         * @return		The layer name to get.
         */
        Ogre::String getLayer(void) const;

        /**
         * @brief		Sets the parent id (another MyGUI window) under which this building block component can be placed.
         * @param[in]	parentId	The parent id to set. May be left off (0), when this building block should be placed as root.
         */
        void setParentId(unsigned long parentId);

        /**
         * @brief		Gets the parent MyGUI window id.
         * @return		The parent MyGUI window id to get or 0, if it does not exist.
         */
        unsigned long getParentId(void) const;

        /**
         * @brief		Sets whether the shadow quality option is shown besides the pure display options.
         * @param[in]	showAdvancedOptions		If true, the shadow quality option is shown.
         */
        void setShowAdvancedOptions(bool showAdvancedOptions);

        /**
         * @brief		Gets whether the shadow quality option is shown.
         * @return		True, if the shadow quality option is shown.
         */
        bool getShowAdvancedOptions(void) const;

        void setOkClickEventName(const Ogre::String& okClickEventName);

        Ogre::String getOkClickEventName(void) const;

        void setApplyClickEventName(const Ogre::String& applyClickEventName);

        Ogre::String getApplyClickEventName(void) const;

        void setCancelClickEventName(const Ogre::String& cancelClickEventName);

        Ogre::String getCancelClickEventName(void) const;

        /**
         * @brief		Gets the MyGUI window, in order to manipulate it further from the outside.
         * @return		The window to get or nullptr, if this component is not activated.
         */
        MyGUI::Window* getWindow(void) const;

    public:
        /**
         * @see		GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("GraphicsConfigurationComponent");
        }

        /**
         * @see		GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "GraphicsConfigurationComponent";
        }

        /**
         * @see		GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: This component can be used as building block in order to have an in game graphics configuration menu. "
                   "It replaces the Ogre configuration dialog, which is unusable on console like targets such as Steam Deck. "
                   "It can be placed as root via the relative position or using a parent id to be placed as a child in a parent MyGUI window. "
                   "Note: Resolution, fullscreen and vsync are applied immediately. Anti aliasing is stored and requires an application restart. "
                   "Note: Render distance and shadow far distance are authored per scene by the level designer and are not configurable here. "
                   "Note: In order to get rid of the Ogre configuration dialog at application start, the flag 'useDefaultGraphicsOptions' must be set in the CoreConfiguration.";
        }

        /**
         * @see	GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrRelativePosition(void)
        {
            return "Relative Position";
        }
        static const Ogre::String AttrRelativeSize(void)
        {
            return "Relative Size";
        }
        static const Ogre::String AttrSkin(void)
        {
            return "Skin";
        }
        static const Ogre::String AttrLayer(void)
        {
            return "Layer";
        }
        static const Ogre::String AttrParentId(void)
        {
            return "Parent Id";
        }
        static const Ogre::String AttrShowAdvancedOptions(void)
        {
            return "Show Advanced Options";
        }
        static const Ogre::String AttrOkClickEventName(void)
        {
            return "Ok Click Event Name";
        }
        static const Ogre::String AttrApplyClickEventName(void)
        {
            return "Apply Click Event Name";
        }
        static const Ogre::String AttrCancelClickEventName(void)
        {
            return "Cancel Click Event Name";
        }

    private:
        /**
         * @brief	Holds one complete set of graphics settings. Used to snapshot the state when the
         *			window is opened, so that the cancel button can restore exactly that state.
         */
        struct GraphicsSettings
        {
            GraphicsSettings() : width(1280), height(720), fullscreen(false), vsync(true), vsyncInterval(1), fsaa("None"), shadowQuality(-1)
            {
            }

            unsigned int width;
            unsigned int height;
            bool fullscreen;
            bool vsync;
            unsigned int vsyncInterval;
            Ogre::String fsaa;
            short shadowQuality; // -1 = scene default
        };

    private:
        void createMyGuiWidgets(void);

        void destroyMyGUIWidgets(void);

        void wireEvents(void);

        void unwireEvents(void);

        void captureCurrentSettings(void);

        void populateWidgetsFromSettings(void);

        void readSettingsFromWidgets(void);

        void applySettings(const GraphicsSettings& settings);

        void saveSettings(void);

        void buttonHit(MyGUI::Widget* sender);

        void notifyComboAccept(MyGUI::ComboBox* sender, size_t index);

        void notifyCheckBoxClick(MyGUI::Widget* sender);

        MyGUI::EditBox* createLabel(const Ogre::String& caption, Ogre::Real posY);

        bool parseResolution(const Ogre::String& videoMode, unsigned int& outWidth, unsigned int& outHeight) const;

    private:
        Ogre::String name;
        MyGUI::Window* widget;
        bool hasParent;
        bool isSimulating;
        bool eventsWired;

        MyGUI::ComboBox* resolutionCombo;
        MyGUI::Button* fullscreenCheck;
        MyGUI::Button* vsyncCheck;
        MyGUI::ComboBox* vsyncIntervalCombo;
        MyGUI::ComboBox* fsaaCombo;
        MyGUI::ComboBox* shadowQualityCombo;
        MyGUI::EditBox* restartRequiredLabel;
        MyGUI::Button* applyButton;
        MyGUI::Button* okButton;
        MyGUI::Button* cancelButton;

        std::vector<MyGUI::Widget*> createdWidgets;

        GraphicsSettings initialSettings;
        GraphicsSettings currentSettings;
        bool restartRequired;

        Variant* activated;
        Variant* relativePosition;
        Variant* relativeSize;
        Variant* skin;
        Variant* layer;
        Variant* parentId;
        Variant* showAdvancedOptions;
        Variant* okClickEventName;
        Variant* applyClickEventName;
        Variant* cancelClickEventName;
    };

}; // namespace end

#endif