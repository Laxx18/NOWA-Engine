#ifndef TAG_POINT_COMPONENT_H
#define TAG_POINT_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreTagPoint.h"

namespace NOWA
{

	class PhysicsActiveComponent;

	/**
	 * @class 	TagPointComponent
	 * @brief 	This component can be used to attach another source game object to the local tag point (bone).
	 *			Info: Several tag point components can be added to one game object.
	 *			Example: A weapon (source game object) could be attached to the right hand of the player.
	 *			Requirements: The entity of this game object must have a skeleton with bones.
	 */
	class EXPORTED TagPointComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<TagPointComponent> TagPointCompPtr;
		typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;
	public:
	
		TagPointComponent();

		virtual ~TagPointComponent();

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

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
			return NOWA::getIdFromName("TagPointComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "TagPointComponent";
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
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) override;

		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "This component can be used to attach another source game object to the local tag point (bone). "
				"Info: Several tag point components can be added to one game object. "
				"Example : A weapon(source game object) could be attached to the right hand of the player. "
				"Requirements : Tags a game object to a bone.Requirements: Entity must have animations.";
		}

		/**
		 * @brief Sets the tag point name the source game object should be attached to
		 * @param[in] tagPointName The tag point name to set
		 */
		void setTagPointName(const Ogre::String& tagPointName);

		/**
		 * @brief Gets the current active tag point name
		 * @return tagPointName The current tag point name to get
		 */
		Ogre::String getTagPointName(void) const;

		/**
		 * @brief Sets source id for the game object that should be attached to this tag point.
		 * @param[in] sourceId The sourceId to set
		 */
		void setSourceId(unsigned long sourceId);

		/**
		 * @brief Gets the source id for the game object that is attached to this tag point.
		 * @return sourceId The sourceId to get
		 */
		unsigned long getSourceId(void) const;

		/**
		 * @brief Sets an offset position at which the source game object should be attached
		 * @param[in] offsetPosition The offset position to set
		 */
		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		/**
		 * @brief Gets the offset position at which the source game object is attached
		 * @return offsetPosition The offset position to get
		 */
		Ogre::Vector3 getOffsetPosition(void) const;

		/**
		 * @brief Sets an offset orientation at which the source game object should be attached
		 * @param[in] offsetOrientation The offset orientation to set (degreeX, degreeY, degreeZ)
		 */
		void setOffsetOrientation(const Ogre::Vector3& offsetOrientation);

		/**
		 * @brief Gets the offset orientation at which the source game object is attached
		 * @return offsetOrientation The offset orientation to get (degreeX, degreeY, degreeZ)
		 */
		Ogre::Vector3 getOffsetOrientation(void) const;

		/**
		 * @brief Gets the tag point Ogre pointer to work directly with the tag point
		 * @return tagPoint The tag point pointer to get
		 */
		Ogre::v1::TagPoint* getTagPoint(void) const;

		/**
		 * @brief Gets the tag point Ogre scene node that is used for this tag point
		 * @return tagPointNode The tag point node to get
		 */
		Ogre::SceneNode* getTagPointNode(void) const;
		
		/**
		 * @brief Gets all for this skeleton available tag point names
		 * @return tagPointNames The tag point names to get as list
		 */
		std::vector<Ogre::String> getAvailableTagPointNames(void) const;
	public:
		static const Ogre::String AttrTagPointName(void) { return "Tag Point Name"; }
		static const Ogre::String AttrSourceId(void) { return "Source Id"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
		static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orientation"; }
	private:
		void generateDebugData(void);

		void destroyDebugData(void);

		void resetTagPoint(void);
	private:
		Ogre::v1::TagPoint* tagPoint;
		Ogre::SceneNode* tagPointNode;
		Variant* tagPoints;
		Variant* sourceId;
		Variant* offsetPosition;
		Variant* offsetOrientation;
		PhysicsActiveComponent* sourcePhysicsActiveComponent;
		bool alreadyConnected;
		std::vector<Ogre::String> tagPointNames;
		// Debug data
		Ogre::SceneNode* debugGeometryArrowNode;
		Ogre::SceneNode* debugGeometrySphereNode;
		Ogre::v1::Entity* debugGeometryArrowEntity;
		Ogre::v1::Entity* debugGeometrySphereEntity;
	};

}; //namespace end

#endif