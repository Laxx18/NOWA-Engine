#ifndef VALUE_BAR_COMPONENT_H
#define VALUE_BAR_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class GameObjectTitleComponent;

	/**
	 * @class 	ValueBarComponent
	 * @brief 	This component can be used to draw an value bar at the game object with an offset. The current value level can also be manipulated.
	 */
	class EXPORTED ValueBarComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<ValueBarComponent> ValueBarCompPtr;
	public:
	
		ValueBarComponent();

		virtual ~ValueBarComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ValueBarComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ValueBarComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see  GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

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
			return "This component can be used to draw an value bar at the game object with an offset. The current value level can also be manipulated.";
		}

		/**
		 * @brief Sets whether to show the value bar.
		 * @param[in] activated if set to true the value bar will be rendered.
		 */
		virtual void setActivated(bool activated) override;

		/**
		 * @brief gets whether the value bar is renderd.
		 * @return activated true if the value bar is rendered rendered
		 */
		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets whether the value bar should be rendered on two sides
		 * @param[in] twoSided if set to true the value bar will be rendered two sided
		 */
		void setTwoSided(bool twoSided);

		/**
		 * @brief gets whether the value bar should be rendered on two sides
		 * @return castShadows true if the value bar are rendered two sided
		 */
		bool getTwoSided(void) const;

		/**
		 * @brief Sets the inner color for the value bar
		 * @param[in] color The color to set (r, g, b)
		 */
		void setInnerColor(const Ogre::Vector3& color);

		/**
		 * @brief Gets the inner color for the value bar
		 * @return color The color (r, g, b) to get
		 */
		Ogre::Vector3 getInnerColor(void) const;
		
		/**
		 * @brief Sets the outer color for the value bar
		 * @param[in] color The color to set (r, g, b)
		 */
		void setOuterColor(const Ogre::Vector3& color);

		/**
		 * @brief Gets the outer color for the value bar
		 * @return color The color (r, g, b) to get
		 */
		Ogre::Vector3 getOuterColor(void) const;

		/**
		 * @brief Sets the border size for the outer rectangle of the value bar
		 * @param[in] borderSize The border size to set
		 */
		void setBorderSize(Ogre::Real borderSize);

		/**
		 * @brief Gets the border size for the outer rectangle of the value bar
		 * @return borderSize The border size to get
		 */
		Ogre::Real getBorderSize(void) const;

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

		/**
		 * @brief Sets the orientation target id, at which this enery bar should be automatically orientated
		 * @param[in] targetId The optional target id to set
		 */
		void setOrientationTargetId(unsigned long targetId);

		/**
		 * @brief Gets the orientation target id, at which this enery bar is orientated
		 * @return targetId The target id to get
		 */
		unsigned long getOrientationTargetId(unsigned int index) const;

		/**
		 * @brief Sets the width of the value bar
		 * @param[in] width The width to set
		 */
		void setWidth(Ogre::Real width);

		/**
		 * @brief Gets the with of the value bar
		 * @returnendPosition The end position to get
		 */
		Ogre::Real getWidth(void) const;

		/**
		 * @brief Sets the height of the value bar
		 * @param[in] height The height to set
		 */
		void setHeight(Ogre::Real height);

		/**
		 * @brief Gets the width of the value bar
		 * @return height The height to get
		 */
		Ogre::Real getHeight(void) const;

		/**
		 * @brief Sets the max value for the value bar
		 * @param[in] maxValue The max value to set
		 */
		void setMaxValue(unsigned int maxValue);

		/**
		 * @brief Gets the max value of the value bar
		 * @return maxValue The max value to get
		 */
		unsigned int getMaxValue(void) const;

		/**
		 * @brief Sets the current value for the value bar
		 * @param[in] currentValue The current value to set
		 */
		void setCurrentValue(unsigned int currentValue);

		/**
		 * @brief Gets the current value of the value bar
		 * @return currentValue The current value to get
		 */
		unsigned int getCurrentValue(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTwoSided(void) { return "Two Sided"; }
		static const Ogre::String AttrInnerColor(void) { return "Inner Color (r, g, b)"; }
		static const Ogre::String AttrOuterColor(void) { return "Outer Color (r, g, b)"; }
		static const Ogre::String AttrBorderSize(void) { return "Border Size"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
		static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orientation"; }
		static const Ogre::String AttrOrientationTargetId(void) { return "Orientation Target Id"; }
		static const Ogre::String AttrWidth(void) { return "Width"; }
		static const Ogre::String AttrHeight(void) { return "Height"; }
		static const Ogre::String AttrMaxValue(void) { return "Max Value"; }
		static const Ogre::String AttrCurrentValue(void) { return "Current Value"; }
	protected:
		virtual void drawValueBar(void);

		void createValueBar(void);

		void destroyValueBar(void);
	protected:
		Ogre::SceneNode* lineNode;
		Ogre::ManualObject* manualObject;
		GameObject* orientationTargetGameObject;
		GameObjectTitleComponent* gameObjectTitleComponent;
		Variant* activated;
		Variant* twoSided;
		Variant* innerColor;
		Variant* outerColor;
		Variant* borderSize;
		Variant* offsetPosition;
		Variant* offsetOrientation;
		Variant* orientationTargetId;
		Variant* width;
		Variant* height;
		Variant* maxValue;
		Variant* currentValue;
		unsigned long indices;
	};

}; //namespace end

#endif