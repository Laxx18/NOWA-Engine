#ifndef PHYSICS_ARTIFACT_COMPONENT_H
#define PHYSICS_ARTIFACT_COMPONENT_H

#include "PhysicsComponent.h"

namespace NOWA
{
	class EXPORTED PhysicsArtifactComponent : public PhysicsComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::PhysicsArtifactComponent> PhysicsArtifactCompPtr;
	public:
	
		PhysicsArtifactComponent();

		virtual ~PhysicsArtifactComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool isMovable(void) const override
		{
			return false;
		}

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsArtifactComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsArtifactComponent";
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
			return "Usage: This Component is used for a non movable maybe complex collision hull, like an whole world, or terra.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override { };

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		virtual void showDebugData(void) override;

		virtual void reCreateCollision(bool overwrite = false) override;

		void setSerialize(bool serialize);

		bool getSerialize(void) const;

		bool createStaticBody(void);

		void changeCollisionFaceId(unsigned int id);
	public:
		static const Ogre::String AttrSerialize(void) { return "Serialize"; }
	protected:
		Variant* serialize;
	};

}; //namespace end

#endif