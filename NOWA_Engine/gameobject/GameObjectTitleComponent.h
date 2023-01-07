#ifndef GAME_OBJECT_TITLE_COMPONENT_H
#define GAME_OBJECT_TITLE_COMPONENT_H

#include "GameObjectComponent.h"
#include "utilities/MovableText.h"

namespace NOWA
{
	class EXPORTED GameObjectTitleComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<GameObjectTitleComponent> GameObjectTitleCompPtr;
	public:

		GameObjectTitleComponent();

		virtual ~GameObjectTitleComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("GameObjectTitleComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "GameObjectTitleComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Adds a specified text to the GameObject visible in the scene.";
		}

		void setFontName(const Ogre::String& fontName);

		Ogre::String getFontName(void) const;

		/**
		 * @brief Sets the text caption.
		 * @param[in] caption The text caption to set
		 */
		void setCaption(const Ogre::String& caption);

		/**
		 * @brief Gets the text caption.
		 * @return caption The text caption to get
		 */
		Ogre::String getCaption(void) const;

		void setAlwaysPresent(bool alwaysPresent);

		bool getAlwaysPresent(void) const;

		void setCharHeight(Ogre::Real charHeight);

		Ogre::Real getCharHeight(void) const;

		/**
		 * @brief Sets the offset position of the value bar
		 * @param[in] offsetPosition The offset position to set
		 */
		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		/**
		 * @brief Gets the offset position of the value bar
		 * @return offsetPosition The offset position to get
		 */
		Ogre::Vector3 getOffsetPosition(void) const;

		/**
		 * @brief Sets the offset orientation of the value bar
		 * @param[in] offsetOrientation The offset orientation to set (degreeX, degreeY, degreeZ)
		 */
		void setOffsetOrientation(const Ogre::Vector3& offsetOrientation);

		/**
		 * @brief Gets the offset orientation of the value bar
		 * @param[in] index The meant value bar index
		 * @return offsetOrientation The offset orientation to get (degreeX, degreeY, degreeZ)
		 */
		Ogre::Vector3 getOffsetOrientation(void) const;

		void setColor(const Ogre::Vector4& colour);

		Ogre::Vector4 getColor(void) const;
	
		void setAlignment(const Ogre::Vector2& alignment);

		Ogre::Vector2 getAlignment(void) const;

		void setLookAtCamera(bool lookAtCamera);

		bool getLookAtCamera(void) const;

		MovableText* getMovableText(void) const;
	public:
		static const Ogre::String AttrFontName(void) { return "Font Name"; }
		static const Ogre::String AttrCaption(void) { return "Caption"; }
		static const Ogre::String AttrAlwaysPresent(void) { return "Always Present"; }
		static const Ogre::String AttrCharHeight(void) { return "Char Height"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
		static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orientation"; }
		static const Ogre::String AttrColor(void) { return "Color (r, g, b, a)"; }
		static const Ogre::String AttrAlignment(void) { return "Alignment"; }
		static const Ogre::String AttrLookAtCamera(void) { return "Look At Camera"; }
	private:
		Variant* fontName;
		Variant* caption;
		Variant* alwaysPresent;
		Variant* charHeight;
		Variant* offsetPosition;
		Variant* offsetOrientation;
		Variant* alignment;
		Variant* color;
		Variant* lookAtCamera;
		MovableText* movableText;
		Ogre::SceneNode* textNode;
	};

}; //namespace end

#endif