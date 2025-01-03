#ifndef LINES_COMPONENT_H
#define LINES_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	/**
	 * @class 	LinesComponent
	 * @brief 	This component can be used to draw one or several lines and ideally used in lua script e.g. for an audio spectrum visualization.
	 */
	class EXPORTED LinesComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<LinesComponent> LinesCompPtr;
	public:
	
		LinesComponent();

		virtual ~LinesComponent();

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
			return NOWA::getIdFromName("LinesComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LinesComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating) override;

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
			return "Usage: Creates one or several lines. It can be used in lua script e.g. for an audio spectrum visualization";
		}

		/**
		 * @brief Sets whether the lines should be connected together
		 * @param[in] connected if set to true the lines will be connected forming a more complex object
		 */
		void setConnected(bool connected);

		/**
		 * @brief Gets whether the lines are connected
		 * @return connected true if the lines are connected
		 */
		bool getIsConnected(void) const;

	   /**
		* @brief Sets the operation type
		* @param[in] operationType the operation type to set. Possible values are:
		* A list of points, 1 vertex per point
		* OT_POINT_LIST = 1,
		*  A list of lines, 2 vertices per line
		* 	OT_LINE_LIST = 2,
		*  A strip of connected lines, 1 vertex per line plus 1 start vertex
		* 	OT_LINE_STRIP = 3,
		*  A list of triangles, 3 vertices per triangle
		* 	OT_TRIANGLE_LIST = 4,
		*  A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that
		* 	OT_TRIANGLE_STRIP = 5,
		*  A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
		* 	OT_TRIANGLE_FAN = 6, Note: Is not more supported
	    */
		void setOperationType(const Ogre::String& operationType);

		/**
		 * @brief Gets the operation type
		 * @return operationType The operation type to get
		 */
		Ogre::String getOperationType(void) const;

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
		 * @brief Sets the lines count
		 * @param[in] linesCount The lines count to set
		 */
		void setLinesCount(unsigned int linesCount);

		/**
		 * @brief Gets the lines count
		 * @return linesCount the lines count to get
		 */
		unsigned int getLinesCount(void) const;
		
		/**
		 * @brief Sets color for the line
		 * @param[in] index The meant line index
		 * @param[in] color The color to set (r, g, b)
		 */
		void setColor(unsigned int index, const Ogre::Vector3& color);

		/**
		 * @brief Gets the color for the line
		 * @param[in] index The meant line index
		 * @return color The color (r, g, b) to get
		 */
		Ogre::Vector3 getColor(unsigned int index) const;

		/**
		 * @brief Sets the start position of the line
		 * @param[in] index The meant line index
		 * @param[in] startPosition The start position to set
		 */
		void setStartPosition(unsigned int index, const Ogre::Vector3& startPosition);

		/**
		 * @brief Gets the start position of the line
		 * @param[in] index The meant line index
		 * @return startPosition The start position to get
		 */
		Ogre::Vector3 getStartPosition(unsigned int index) const;

		/**
		 * @brief Sets the end position of the line
		 * @param[in] index The meant line index
		 * @param[in] endPosition The end position to set
		 */
		void setEndPosition(unsigned int index, const Ogre::Vector3& endPosition);

		/**
		 * @brief Gets the end position of the line
		 * @param[in] index The meant line index
		 * @returnendPosition The end position to get
		 */
		Ogre::Vector3 getEndPosition(unsigned int index) const;

	public:
		static const Ogre::String AttrConnected(void) { return "Connected"; }
		static const Ogre::String AttrOperationType(void) { return "Operation Type"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrLinesCount(void) { return "Lines Count"; }
		static const Ogre::String AttrColor(void) { return "Color (r, g, b) "; }
		static const Ogre::String AttrStartPosition(void) { return "Start Position "; }
		static const Ogre::String AttrEndPosition(void) { return "End Position "; }
	protected:
		void createLine(unsigned int index);

		virtual void drawLine(unsigned int index);

		void destroyLine(unsigned int index);

		void destroyLines(void);

		Ogre::String mapOperationTypeToString(Ogre::OperationType operationType);
		Ogre::OperationType mapStringToOperationType(const Ogre::String& strOperationType);
	protected:
		Ogre::SceneNode* lineNode;
		std::vector<Ogre::ManualObject*> lineObjects;
		Ogre::OperationType internalOperationType;
		Ogre::v1::Entity* dummyEntity;
		Variant* connected;
		Variant* operationType;
		Variant* castShadows;
		Variant* linesCount;
		std::vector<Variant*> colors;
		std::vector<Variant*> startPositions;
		std::vector<Variant*> endPositions;
	};

}; //namespace end

#endif