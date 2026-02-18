#ifndef PHYSICS_ARTIFACT_COMPONENT_H
#define PHYSICS_ARTIFACT_COMPONENT_H

#include "PhysicsComponent.h"
#include "main/Events.h"

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

		virtual void update(Ogre::Real dt, bool notSimulating = false) override { };

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

        /**
         * @brief Creates static body from compound collision
         * @param collisions Vector of child collision shapes (e.g., cylinders for trees)
         * @param collisionName Unique name for serialization (e.g., "FoliageCollision")
         * @return true if successful
         */
        bool createCompoundBody(const std::vector<OgreNewt::CollisionPtr>& childCollisions, const Ogre::String& collisionName);
	public:
		static const Ogre::String AttrSerialize(void) { return "Serialize"; }

	protected:
        enum CollisionMode
        {
            COLLISION_TREE,    // TreeCollision from mesh
            COLLISION_COMPOUND // CompoundCollision from child shapes
        };

    protected:
        void handleGameObjectChanged(NOWA::EventDataPtr eventData);
	protected:
		Variant* serialize;
        CollisionMode collisionMode;
        Ogre::String compoundCollisionName;
        std::vector<OgreNewt::CollisionPtr> compoundChildCollisions; // Store for recreation
	};

}; //namespace end

#endif