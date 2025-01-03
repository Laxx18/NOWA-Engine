#ifndef PHYSICS_ACTIVE_COMPOUND_COMPONENT_H
#define PHYSICS_ACTIVE_COMPOUND_COMPONENT_H

#include "PhysicsActiveComponent.h"

namespace NOWA
{
	class EXPORTED PhysicsActiveCompoundComponent : public PhysicsActiveComponent
	{
	public:

		typedef boost::shared_ptr<PhysicsActiveCompoundComponent> PhysicsActiveCompoundCompPtr;
	protected:
		struct CollisionData
		{
			CollisionData()
				: collisionType("None"),
				collisionPosition(Ogre::Vector3::ZERO),
				collisionSize(Ogre::Vector3::ZERO),
				collisionOrientation(Ogre::Quaternion::IDENTITY)
			{

			}

			Ogre::String collisionType;
			Ogre::Vector3 collisionPosition;
			Ogre::Vector3 collisionSize;
			Ogre::Quaternion collisionOrientation;
			OgreNewt::CollisionPtr collisionPtr;
		};
	public:

		PhysicsActiveCompoundComponent();

		virtual ~PhysicsActiveCompoundComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual Ogre::String getParentParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual bool isMovable(void) const override
		{
			return true;
		}

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsActiveCompoundComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsActiveCompoundComponent";
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
			return "Usage: This Component is used for physics motion, the collision hull is build of several bodies defined in a XML script file.";
		}

		void setMeshCompoundConfigFile(const Ogre::String& meshCompoundConfigFile);
	public:
		static const Ogre::String AttrMeshCompoundConfigFile(void) { return "Mesh Config. File"; }
	protected:

		virtual bool createDynamicBody(void);

		void copyCollisionData(std::vector<CollisionData> collisionDataList);
	protected:
		
		std::vector<CollisionData> collisionDataList;
		Variant* meshCompoundConfigFile;

	};

}; //namespace end

#endif