#ifndef LINE_COMPONENT_H
#define LINE_COMPONENT_H

#include "GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{

	class PhysicsActiveComponent;

	/**
	 * @class 	LineComponent
	 * @brief 	This component can be used to draw a line from a source game object start position to a target id game object's end position.
	 *			It will be automatically adapted, when the game objects are moving.
	 */
	class EXPORTED LineComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<LineComponent> LineCompPtr;
	public:
	
		LineComponent();

		virtual ~LineComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

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
			return NOWA::getIdFromName("LineComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LineComponent";
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
			return "Usage: Creates a visible line between two game objects.";
		}

		/**
		 * @brief Sets target id for the game object, the line's end point should be attached to.
		 * @param[in] targetId The target id to set
		 */
		void setTargetId(unsigned long targetId);

		/**
		 * @brief Gets the target id for the game object, the line's end point should be attached to.
		 * @return targetId The target id to get
		 */
		unsigned long getTargetId(void) const;
		
		/**
		 * @brief Sets color for the line
		 * @param[in] color The color to set (r, g, b)
		 */
		void setColor(const Ogre::Vector3& color);

		/**
		 * @brief Gets the color for the line
		 * @return colour The color (r, g, b) to get
		 */
		Ogre::Vector3 getColor(void) const;

		/**
		 * @brief Sets an offset position at which the line start point should be attached
		 * @param[in] sourceOffsetPosition The source offset position to set
		 */
		void setSourceOffsetPosition(const Ogre::Vector3& sourceOffsetPosition);

		/**
		 * @brief Gets the source offset position at which the source line is attached
		 * @return sourceOffsetPosition The source offset position to get
		 */
		Ogre::Vector3 getSourceOffsetPosition(void) const;

		/**
		 * @brief Sets an offset position at which the line end point should be attached
		 * @param[in] targetOffsetPosition The source offset position to set
		 */
		void setTargetOffsetPosition(const Ogre::Vector3& targetOffsetPosition);

		/**
		 * @brief Gets the target offset position at which the target line is attached
		 * @return sourceOffsetPosition The target offset position to get
		 */
		Ogre::Vector3 getTargetOffsetPosition(void) const;

	public:
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrColor(void) { return "Color (r, g, b)"; }
		static const Ogre::String AttrSourceOffsetPosition(void) { return "Source Offset Pos."; }
		static const Ogre::String AttrTargetOffsetPosition(void) { return "Target Offset Pos."; }
	protected:
		void createLine(void);

		virtual void drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition);

		void destroyLine();
		
		void handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData);
	protected:
		Ogre::SceneNode* lineNode;
		Ogre::ManualObject* lineObject;
		GameObject* targetGameObject;
		Variant* targetId;
		Variant* color;
		Variant* sourceOffsetPosition;
		Variant* targetOffsetPosition;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * @class 	LineMeshScaleComponent
	 * @brief 	This component can be used to scale a mesh from a source game object start position to a target id game object's end position.
	 *			It will be automatically adapted, when the game objects are moving, so when the line gets longer more game objects will be placed and vice versa.
	 */
	class EXPORTED LineMeshScaleComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<LineMeshScaleComponent> LineMeshScaleCompPtr;
	public:

		LineMeshScaleComponent();

		virtual ~LineMeshScaleComponent();

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
			return NOWA::getIdFromName("LineMeshScaleComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LineMeshScaleComponent";
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
			return "Usage: Scales a mesh so that it fits between two game objects. Requirements: No physics component. "
				"Important: The used mesh must have its origin at center bottom for propery scaling in just one direction. "
				"If this is not the case use a converter tool like OgreMeshMagick. Example: OgreMeshMagick.exe transform -yalign=bottom myMesh.mesh";
		}

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
		 * @brief Sets the start position id for the game object, the mesh should be attached to.
		 * @param[in] startPositionId The start position id to set
		 */
		void setStartPositionId(unsigned long startPositionId);

		/**
		 * @brief Gets the start position id for the game object, the mesh should be attached to.
		 * @return startPositionId The start position id to get
		 */
		unsigned long getStartPositionId(void) const;

		/**
		 * @brief Sets the end position id for the game object, the mesh will be orientated and scaled to.
		 * @param[in] endPositionId The start position id to set
		 */
		void setEndPositionId(unsigned long endPositionId);

		/**
		 * @brief Gets the end position id for the game object, the mesh will be orientated and scaled to.
		 * @return endPositionId The end position id to get
		 */
		unsigned long getEndPositionId(void) const;

		/**
		 * @brief Sets the mesh scale axis
		 * @param[in] meshScaleAxis The mesh scale axis to set
		 */
		void setMeshScaleAxis(const Ogre::Vector3& meshScaleAxis);

		/**
		 * @brief Gets the mesh scale axis
		 * @return meshScale The meshes scale axis to get
		 */
		Ogre::Vector3 getMeshScaleAxis(void) const;
	public:
		static const Ogre::String AttrStartPositionId(void) { return "Start Pos. Id"; }
		static const Ogre::String AttrEndPositionId(void) { return "End Pos. Id"; }
		static const Ogre::String AttrMeshScaleAxis(void) { return "Mesh Scale Axis"; }
	private:
		void handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData);
	private:
		Variant* startPositionId;
		Variant* endPositionId;
		Variant* meshScaleAxis;
		GameObject* startPositionGameObject;
		GameObject* endPositionGameObject;
		Ogre::Vector3 positionOffsetSnapshot;
		Ogre::Vector3 positionSnapshot;
		Ogre::Quaternion orientationOffsetSnapshot;
		Ogre::Vector3 scaleSnapshot;
		Ogre::Vector3 sizeSnapshot;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * @class 	LineMeshComponent
	 * @brief 	This component can be used to draw a line of meshes from a source game object start position to a target id game object's end position.
	 *			It will be automatically adapted, when the game objects are moving, so when the line gets longer more game objects will be placed and vice versa.
	 */
	class EXPORTED LineMeshComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<LineMeshComponent> LineMeshCompPtr;
	public:

		LineMeshComponent();

		virtual ~LineMeshComponent();

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
			return NOWA::getIdFromName("LineMeshComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LineMeshComponent";
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
		 * @brief Sets the offset position factor at which the source game object should be attached (e.g. 2 1 1 would attach each 2x multiplicated factor)
		 * @param[in] offsetPositionFactor The offset position factor to set
		 */
		void setOffsetPositionFactor(const Ogre::Vector3& meshOffsetPositionFactor);

		/**
		 * @brief Gets the offset position factor at which the source game object is attached
		 * @return offsetPositionFactor The offset position factor to get
		 */
		Ogre::Vector3 getOffsetPositionFactor(void) const;

		/**
		 * @brief Sets an offset orientation at which the source game object should be attached
		 * @param[in] offsetOrientation The offset orientation to set (degreeX, degreeY, degreeZ)
		 */
		void setOffsetOrientation(const Ogre::Vector3& meshOffsetOrientation);

		/**
		 * @brief Gets the offset orientation at which the source game object is attached
		 * @return offsetOrientation The offset orientation to get (degreeX, degreeY, degreeZ)
		 */
		Ogre::Vector3 getOffsetOrientation(void) const;

		/**
		 * @brief Sets the expand axis, at which this game object will be duplicated.
		 * @param[in] expandAxis The axis to expand
		 */
		void setExpandAxis(const Ogre::Vector3& expandAxis);

		/**
		 * @brief Gets the expand axis
		 * @return expandAxis The expand axis
		 */
		Ogre::Vector3 getExpandAxis(void) const;

		/**
		 * @brief Sets the start position id for the game object, the mesh should be attached to.
		 * @param[in] startPositionId The start position id to set
		 */
		void setStartPositionId(unsigned long startPositionId);

		/**
		 * @brief Gets the start position id for the game object, the mesh should be attached to.
		 * @return startPositionId The start position id to get
		 */
		unsigned long getStartPositionId(void) const;

		/**
		 * @brief Sets the end position id for the game object, the mesh will be orientated and scaled to.
		 * @param[in] endPositionId The start position id to set
		 */
		void setEndPositionId(unsigned long endPositionId);

		/**
		 * @brief Gets the end position id for the game object, the mesh will be orientated and scaled to.
		 * @return endPositionId The end position id to get
		 */
		unsigned long getEndPositionId(void) const;
	protected:
		void setMeshCount(unsigned int meshCount, const Ogre::Vector3& direction);

		void pushMesh(const Ogre::Vector3& direction);

		void popMesh(void);

		void clearMeshes(void);

		void updateMeshes(const Ogre::Vector3& direction);

		Ogre::SceneNode* addSceneNode(void);

		void deleteSceneNode(Ogre::SceneNode* sceneNode);
	public:
		static const Ogre::String AttrStartPositionId(void) { return "Start Pos. Id"; }
		static const Ogre::String AttrEndPositionId(void) { return "End Pos. Id"; }
		static const Ogre::String AttrOffsetPositionFactor(void) { return "Offset Pos. Factor"; }
		static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orient."; }
		static const Ogre::String AttrExpandAxis(void) { return "Expand Axis"; }
	private:
		void handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData);
	private:
		Variant* offsetPositionFactor;
		Variant* offsetOrientation;
		Variant* expandAxis;
		Variant* startPositionId;
		Variant* endPositionId;
		GameObject* startPositionGameObject;
		GameObject* endPositionGameObject;
		Ogre::Vector3 positionOffsetSnapshot;
		Ogre::Vector3 positionSnapshot;
		Ogre::Quaternion orientationSnapshot;
		Ogre::Quaternion orientationOffsetSnapshot;
		Ogre::Vector3 scaleSnapshot;
		Ogre::Vector3 sizeSnapshot;
		std::vector<Ogre::SceneNode*> meshes;
		size_t oldCount;
		Ogre::Real meshSize;
	};

}; //namespace end

#endif