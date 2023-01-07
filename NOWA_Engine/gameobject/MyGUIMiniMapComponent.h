#ifndef MYGUI_MINI_MAP_COMPONENT_H
#define MYGUI_MINI_MAP_COMPONENT_H

#include "MyGUIComponents.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IResource.h"
#include "MyGUI_ResourceManager.h"
#include "BaseLayout/BaseLayout.h"
#include "ItemBox/BaseCellView.h"
#include "ItemBox/BaseItemBox.h"
	
namespace NOWA
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class MiniMapToolTip
	{
	public:
		MiniMapToolTip();

		void show(const MyGUI::IntPoint& point, const Ogre::String& description);
		void hide();
		void move(const MyGUI::IntPoint& point);
	private:
		/** Move a widget to a point while making it stay in the viewport.
			@param moving The widget that will be moving.
			@param point The desired destination viewport coordinates
						 (which may not be the final resting place of the widget).
		 */
		void boundedMove(MyGUI::Widget* moving, const MyGUI::IntPoint& point);
	private:
		MyGUI::Widget* toolTip;
		MyGUI::EditBox* textDescription;
		Ogre::String description;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class EXPORTED MyGUIMiniMapComponent : public MyGUIWindowComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIMiniMapComponent> MyGUIMinMapCompPtr;
	public:

		MyGUIMiniMapComponent();

		virtual ~MyGUIMiniMapComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGUIMiniMapComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIMiniMapComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI minimap component. This component will search for all exit components in all scenes of the current project and build a minimap of all scenes with a target game object painted as dot."
				"Note: The minimap component can use the 'mouseClickEventName' in order to specify a function name for being executed in lua script, if the corresponding minimap tile has been clicked.";
		}

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setStartPosition(const Ogre::Vector2& startPosition);

		Ogre::Vector2 getStartPosition(void) const;

		void setScaleFactor(Ogre::Real scaleFactor);

		Ogre::Real getScaleFactor(void) const;

		void setSkinName(unsigned int index, const Ogre::String& skinName);

		Ogre::String getSkinName(unsigned int index) const;
		
		void setUseToolTip(bool useToolTip);

		bool getUseToolTip(void) const;

		void setAxis(const Ogre::String& axis);
	
		/**
		 * @brief Gets the mini map tiles count
		 * @return mapCount The mini map tiles count
		 */
		unsigned int getMiniMapTilesCount(void) const;
		
		void setMiniMapTileColor(unsigned int index, const Ogre::Vector3& color);

		Ogre::Vector3 getMiniMapTileColor(unsigned int index);
		
		void setToolTipDescription(unsigned int index, const Ogre::String& description);

		Ogre::String getToolTipDescription(unsigned int index);

		void setMiniMapTileVisible(unsigned int index, bool miniMapTileVisible);

		bool isMiniMapTileVisible(unsigned int index) const;

		/**
		 * @brief Sets the trackable count.
		 * @param[in] trackable The trackable count to set
		 */
		void setTrackableCount(unsigned int trackableCount);

		/**
		 * @brief Gets the trackable count
		 * @return trackableCount The trackable count
		 */
		unsigned int getTrackableCount(void) const;

		/**
		 * @brief Sets the trackable id, that is shown on the mini map
		 * @param[in] index The index to set the trackable id for
		 * @param[in] id The id as string. Note: If the trackable id is in another scene, the scene name must be specified. For example 'scene3:2341435213'.
		 *				Will search in scene3 for the game object with the id 2341435213 and in conjunction with the image attribute, the image will be placed correctly on the minimap.
		 *				If the scene name is missing, its assumed, that the id is an global one (like the player which is available for each scene) and has the same id for each scene.
		 */
		void setTrackableId(unsigned int index, const Ogre::String& id);

		Ogre::String getTrackableId(unsigned int index);

		void setTrackableImage(unsigned int index, const Ogre::String& imageName);

		Ogre::String getTrackableImage(unsigned int index);

		void setTrackableImageTileSize(unsigned int index, const Ogre::Vector2& imageTileSize);

		Ogre::Vector2 getTrackableImageTileSize(unsigned int index);

		void setTrackableImageAnimationSpeed(Ogre::Real speed);

		Ogre::Real getTrackableImageAnimationSpeed(void) const;
		
		void setShowNames(bool showNames);

		void showMiniMap(bool bShow);

		bool isMiniMapShown(void) const;

		void generateMiniMap(void);

		void generateTrackables(void);
	public:
		static const Ogre::String AttrStartPosition(void) { return "Start Position"; }
		static const Ogre::String AttrScaleFactor(void) { return "Scale Factor"; }
		static const Ogre::String AttrUseToolTip(void) { return "Use ToolTip"; }
		static const Ogre::String AttrAxis(void) { return "Axis"; }
		static const Ogre::String AttrShowNames(void) { return "Show Names"; }
		static const Ogre::String AttrSkinName(void) { return "Skin Name "; }
		static const Ogre::String AttrMiniMapTileColor(void) { return "Tile Color "; }
		static const Ogre::String AttrToolTipDescription(void) { return "ToolTip Description "; }
		static const Ogre::String AttrTrackableCount(void) { return "Trackable Count"; }
		static const Ogre::String AttrTrackableId(void) { return "Trackable Id "; }
		static const Ogre::String AttrTrackableImage(void) { return "Trackable Image "; }
		static const Ogre::String AttrTrackableImageTileSize(void) { return "Trackable Image Tile Size "; }
		static const Ogre::String AttrTrackableImageAnimationSpeed(void) { return "Trackable Image Anim. Speed "; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
		void notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch);
		void notifyWindowButtonPressed(MyGUI::Window* sender, const std::string& button);
	private:
		void notifyToolTip(MyGUI::Widget* sender, const MyGUI::ToolTipInfo& info);
		void destroyMiniMap(void);
		void destroyTrackables(void);
	private:
		MiniMapToolTip* toolTip;
		
		unsigned int miniMapTilesCount;
		std::vector<MyGUI::Widget*> windowMapTiles;
		std::vector<MyGUI::TextBox*> textBoxMapTiles;
		std::vector<MyGUI::ImageBox*> trackableImageBoxes;

		Variant* startPosition;
		Variant* scaleFactor;
		Variant* useToolTip;
		Variant* axis;
		Variant* showNames;
		Variant* trackableCount;
		Variant* trackableImageAnimationSpeed;

		std::vector<Variant*> skinNames;
		std::vector<Variant*> miniMapTilesColors;
		std::vector<Variant*> toolTipDescriptions;
		std::vector<Variant*> trackableIds;
		std::vector<Variant*> trackableImages;
		std::vector<Variant*> trackableImageTileSizes;
		std::vector<int> spriteAnimationIndices;

		bool bShowMiniMap;
		Ogre::Real timeSinceLastUpdate;
	};

}; //namespace end

#endif