#ifndef NAV_MESH_TERRA_COMPONENT_H
#define NAV_MESH_TERRA_COMPONENT_H

#include "NOWAPrecompiled.h"
#include "GameObjectComponent.h"
#include "modules/OgreRecastModule.h"
#include "utilities/XMLConverter.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED NavMeshTerraComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<NavMeshTerraComponent> NavMeshTerraCompPtr;
	public:

		NavMeshTerraComponent();

		virtual ~NavMeshTerraComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("NavMeshTerraComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "NavMeshTerraComponent";
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
			return "Usage: This terra cell will be used for path finding. "
				"Requirements: This component can be used just for terra and if OgreRecast A* path navigation is actived.";
		}

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setTerraLayers(const Ogre::String& terraLayers);

		Ogre::String getTerraLayers(void) const;

		std::vector<int> getTerraLayerList(void) const;

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTerraLayers(void) { return "Terra Layers"; }
	private:
		void checkAndSetTerraLayers(const Ogre::String& terraLayers);
	private:
		std::vector<int> terraLayerList;

		Variant* activated;
		Variant* terraLayers;
	};

}; //namespace end

#endif