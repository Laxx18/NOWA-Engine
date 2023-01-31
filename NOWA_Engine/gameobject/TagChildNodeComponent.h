#ifndef TAG_CHILD_NODE_COMPONENT_H
#define TAG_CHILD_NODE_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{

	class PhysicsActiveComponent;

	/**
	* @class 	TagChildNodeComponent
	* @brief 	This component can be used to add the scene node of another source game object as a child of the scene node of this game object.
	*			When this game object is transformed, all its children will also be transformed relative to this parent. A hierarchy can be formed.
	*			Info: Several tag child node components can be added to one game object
	*			Example: Adding a light as a child to ball. When the ball is moved and rotated the light will also rotate and move relative to the parents transform.
	*			Requirements: None
	*/
	class EXPORTED TagChildNodeComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<TagChildNodeComponent> TagChildNodeCompPtr;
		typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;
	public:
	
		TagChildNodeComponent();

		virtual ~TagChildNodeComponent();

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
			return NOWA::getIdFromName("TagChildNodeComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "TagChildNodeComponent";
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
			return "This component can be used to add the scene node of another source game object as a child of the scene node of this game object. "
				   "When this game object is transformed, all its children will also be transformed relative to this parent. A hierarchy can be formed. "
				   "Info: Several tag child node components can be added to one game object "
				   "Example: Adding a light as a child to ball. When the ball is moved and rotated the light will also rotateand move relative to the parents transform.";
		}

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
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;

		/**
		 * @brief Sets source id for the game object that should be added as a child to this components game object.
		 * @param[in] sourceId The sourceId to set
		 */
		void setSourceId(unsigned long sourceId);

		/**
		 * @brief Gets the source id for the game object that has been added as a child of this components game object.
		 * @return sourceId The sourceId to get
		 */
		unsigned long getSourceId(void) const;
	public:
		static const Ogre::String AttrSourceId(void) { return "Source Id"; }
	private:
		Ogre::SceneNode* sourceChildNode;
		Variant* sourceId;
		Ogre::Node* sourceParentOfChildNode;
		Ogre::Vector3 oldSourceChildPosition;
		Ogre::Quaternion oldSourceChildOrientation;
		bool alreadyConnected;
	};

}; //namespace end

#endif