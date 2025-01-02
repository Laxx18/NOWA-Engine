#ifndef PHYSICS_TERRAIN_COMPONENT_H
#define PHYSICS_TERRAIN_COMPONENT_H

#include "PhysicsComponent.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED PhysicsTerrainComponent : public PhysicsComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::PhysicsTerrainComponent> PhysicsTerrainCompPtr;
	public:

		PhysicsTerrainComponent();

		virtual ~PhysicsTerrainComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
		
		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual bool isMovable(void) const override
		{
			return false;
		}

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsTerrainComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsTerrainComponent";
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
			return "Usage: This component is used to create physics artifact collision hull for Ogre terra. "
				"Requirements: A game with TerraComponent.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating = false) override { };

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void showDebugData(void) override;

		virtual void reCreateCollision(bool overwrite = false) override;

		void setSerialize(bool serialize);

		bool getSerialize(void) const;

		void changeCollisionFaceId(unsigned int id);
	public:
		static const Ogre::String AttrSerialize(void) { return "Serialize"; }
	private:
		void handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData);
		void handleTerraChanged(NOWA::EventDataPtr eventData);
	private:
		Variant* serialize;
	};

}; //namespace end

#endif