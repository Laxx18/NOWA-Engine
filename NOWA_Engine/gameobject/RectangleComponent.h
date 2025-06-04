#ifndef RECTANGLE_COMPONENT_H
#define RECTANGLE_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	/**
	 * @class 	RectangleComponent
	 * @brief 	This component can be used to draw one or several rectangles and ideally used in lua script e.g. for an audio spectrum visualization.
	 *          It is much more performant as LinesComponent, because only one manual object with x start- and end-positions is created.
	 *			The difference to LinesComponent is, that each end-position is also the next start position.
	 */
	class EXPORTED RectangleComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<RectangleComponent> RectangleCompPtr;
	public:
	
		RectangleComponent();

		virtual ~RectangleComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("RectangleComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "RectangleComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

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
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Creates one or several rectangles. It can be used in lua script e.g. for an audio spectrum visualization"
				"It is much more performant as LinesComponent, because only one manual object with x start- and width/height is created."
				"The difference to LinesComponent is, that each end - position is also the next start position.";
		}

		/**
		 * @brief Sets whether to cast shadows
		 * @param[in] castShadows if set to true shadows will be rendered.
		 */
		void setCastShadows(bool castShadows);

		/**
		 * @brief gets whether shadows are casted
		 * @return castShadows true if shadows are rendered.
		 */
		bool getCastShadows(void) const;

		/**
		 * @brief Sets whether the rectangles should be rendered on two sides
		 * @param[in] twoSided if set to true the rectangles will be rendered two sided.
		 */
		void setTwoSided(bool twoSided);

		/**
		 * @brief gets whether the rectangles should be rendered on two sides
		 * @return castShadows true if the rectangles are rendered two sided.
		 */
		bool getTwoSided(void) const;

		/**
		 * @brief Sets the rectangles count
		 * @param[in] rectanglesCount The rectangles count to set
		 */
		void setRectanglesCount(unsigned int rectanglesCount);

		/**
		 * @brief Gets the rectangles count
		 * @return rectanglesCount the rectangles count to get
		 */
		unsigned int getRectanglesCount(void) const;
		
		/**
		 * @brief Sets color 1 for the rectangle
		 * @param[in] index The meant rectangle index
		 * @param[in] color The color to set (r, g, b)
		 */
		void setColor1(unsigned int index, const Ogre::Vector3& color);

		/**
		 * @brief Gets the color 1 for the line
		 * @param[in] index The meant rectangle index
		 * @return color The color (r, g, b) to get
		 */
		Ogre::Vector3 getColor1(unsigned int index) const;

		/**
		 * @brief Sets colour 2 for the rectangle
		 * @param[in] index The meant rectangle index
		 * @param[in] color The color to set (r, g, b)
		 */
		void setColor2(unsigned int index, const Ogre::Vector3& color);

		/**
		 * @brief Gets the color 2 for the line
		 * @param[in] index The meant rectangle index
		 * @return color The color (r, g, b) to get
		 */
		Ogre::Vector3 getColor2(unsigned int index) const;

		/**
		 * @brief Sets the start position of the rectangle
		 * @param[in] index The meant rectangle index
		 * @param[in] startPosition The start position to set
		 */
		void setStartPosition(unsigned int index, const Ogre::Vector3& startPosition);

		/**
		 * @brief Gets the start position of the rectangle
		 * @param[in] index The meant rectangle index
		 * @return startPosition The start position to get
		 */
		Ogre::Vector3 getStartPosition(unsigned int index) const;

		/**
		 * @brief Sets the start orientation of the rectangle
		 * @param[in] index The meant rectangle index
		 * @param[in] startOrientation The start orientation to set (degreeX, degreeY, degreeZ)
		 */
		void setStartOrientation(unsigned int index, const Ogre::Vector3& startOrientation);

		/**
		 * @brief Gets the start orientation of the rectangle
		 * @param[in] index The meant rectangle index
		 * @return startOrientation The start orientation to get (degreeX, degreeY, degreeZ)
		 */
		Ogre::Vector3 getStartOrientation(unsigned int index) const;

		/**
		 * @brief Sets the width of the rectangle
		 * @param[in] index The meant rectangle index
		 * @param[in] width The width to set
		 */
		void setWidth(unsigned int index, Ogre::Real width);

		/**
		 * @brief Gets the width of the rectangle
		 * @param[in] index The meant rectangle index
		 * @return width The width to get
		 */
		Ogre::Real getWidth(unsigned int index) const;

		/**
		 * @brief Sets the height of the rectangle
		 * @param[in] index The meant rectangle index
		 * @param[in] height The height to set
		 */
		void setHeight(unsigned int index, Ogre::Real height);

		/**
		 * @brief Gets the height of the rectangle
		 * @param[in] index The meant rectangle index
		 * @return height The height to get
		 */
		Ogre::Real getHeight(unsigned int index) const;

	public:
		static const Ogre::String AttrConnected(void) { return "Connected"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrTwoSided(void) { return "Two Sided"; }
		static const Ogre::String AttrRectanglesCount(void) { return "Rectangles Count"; }
		static const Ogre::String AttrColor1(void) { return "Color 1 (r, g, b) "; }
		static const Ogre::String AttrColor2(void) { return "Color 2 (r, g, b) "; }
		static const Ogre::String AttrStartPosition(void) { return "Start Position "; }
		static const Ogre::String AttrStartOrientation(void) { return "Start Orientation (degX, degY, degZ)"; }
		static const Ogre::String AttrWidth(void) { return "Width "; }
		static const Ogre::String AttrHeight(void) { return "Height "; }
	protected:
		virtual void drawRectangle(Ogre::ManualObject* manualObject, unsigned int index);

		void destroyRectangles(void);
	protected:
		Ogre::SceneNode* lineNode;
		Ogre::ManualObject* manualObject;
		Ogre::v1::Entity* dummyEntity;
		Variant* castShadows;
		Variant* twoSided;
		Variant* rectanglesCount;
		std::vector<Variant*> colors1;
		std::vector<Variant*> colors2;
		std::vector<Variant*> startPositions;
		std::vector<Variant*> startOrientations;
		std::vector<Variant*> widths;
		std::vector<Variant*> heights;
		unsigned long indices;
	};

}; //namespace end

#endif