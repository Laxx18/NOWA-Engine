/*
Copyright (c) 2022 Lukas Kalinowski
/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef VEGETATIONCOMPONENT_H
#define VEGETATIONCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class TerraComponent;

	/**
	  * @brief		Its possible to generate controlled random vegetation and map over a target game object or whole scene.
	  */
	class EXPORTED VegetationComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<VegetationComponent> VegetationComponentPtr;
	public:

		VegetationComponent();

		virtual ~VegetationComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		*/
		virtual void initialise() override;

		/**
		* @see		Ogre::Plugin::shutdown
		*/
		virtual void shutdown() override;

		/**
		* @see		Ogre::Plugin::uninstall
		*/
		virtual void uninstall() override;

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;
		
		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setTargetGameObjectId(unsigned long targetGameObjectId);

		unsigned long getTargetGameObjectId(void) const;

		void setDensity(Ogre::Real density);

		Ogre::Real getDensity(void) const;

		void setPositionXZ(const Ogre::Vector2& positionXZ);

		Ogre::Vector2 getPositionXZ(void) const;

		void setScale(Ogre::Real scale);

		Ogre::Real getScale(void) const;

		void setAutoOrientation(bool autoOrientation);

		bool getAutoOrientation(void) const;

		void setCategories(const Ogre::String& categories);

		Ogre::String getCategories(void) const;

		void setMaxHeight(Ogre::Real maxHeight);

		Ogre::Real getMaxHeight(void) const;

		void setRenderDistance(Ogre::Real renderDistance);

		Ogre::Real getRenderDistance(void) const;

		void setTerraLayers(const Ogre::String& terraLayers);

		Ogre::String getTerraLayers(void) const;

		void setVegetationTypesCount(unsigned int vegetationTypesCount);

		unsigned int getVegetationTypesCount(void) const;

		void setVegetationMeshName(unsigned int index, const Ogre::String& vegetationMeshName);

		Ogre::String getVegetationMeshName(unsigned int index) const;
	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("VegetationComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "VegetationComponent";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Its possible to generate controlled random vegetation and map over a target game object or whole scene. Requirements: Must be placed under a Main-Gameobject";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrTargetGameObjectId(void) { return "Target Id"; }
		static const Ogre::String AttrDensity(void) { return "Density"; }
		static const Ogre::String AttrPositionXZ(void) { return "Position X-Z"; }
		static const Ogre::String AttrScale(void) { return "Scale"; }
		static const Ogre::String AttrAutoOrientation(void) { return "Auto Orientation"; }
		static const Ogre::String AttrCategories(void) { return "Categories"; }
		static const Ogre::String AttrMaxHeight(void) { return "Max Height"; }
		static const Ogre::String AttrRenderDistance(void) { return "Render Distance"; }
		static const Ogre::String AttrTerraLayers(void) { return "Terra Layers"; }
		static const Ogre::String AttrVegetationTypesCount(void) { return "Vegetation Types Count"; }
		static const Ogre::String AttrVegetationMeshName(void) { return "Vegetation Mesh Name "; }
		static const Ogre::String AttrRegenerate(void) { return "Regenerate"; }
	protected:
		virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
	private:
		void clearLists();
		void regenerateVegetation();
		void checkAndSetTerraLayers(const Ogre::String& terraLayers);

		void handleUpdateBounds(NOWA::EventDataPtr eventData);
		void handleSceneParsed(NOWA::EventDataPtr eventData);
	private:
		Ogre::String name;

		Ogre::Vector3 minimumBounds;
		Ogre::Vector3 maximumBounds;
		std::vector<Ogre::Item*> vegetationItemList;
		Ogre::RaySceneQuery* raySceneQuery;
		std::vector<int> terraLayerList;

		Variant* regenerate;
		Variant* targetGameObjectId;
		Variant* density;
		Variant* positionXZ;
		Variant* scale;
		Variant* autoOrientation;
		Variant* categories;
		Variant* maxHeight;
		Variant* renderDistance;
		Variant* terraLayers;
		Variant* vegetationTypesCount;
		std::vector<Variant*> vegetationMeshNames;
	};

}; // namespace end

#endif

