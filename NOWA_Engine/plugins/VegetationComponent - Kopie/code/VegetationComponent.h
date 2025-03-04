/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
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
	  * @brief		Its possible to generate controlled random vegetation and map over a target game object.
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
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setTargetGameObjectId(unsigned long targetGameObjectId);

		unsigned long getTargetGameObjectId(void) const;

		void setDensity(Ogre::Real density);

		Ogre::Real getDensity(void) const;

		void setPositionXZ(const Ogre::Vector2& positionXZ);

		Ogre::Vector2 getPositionXZ(void) const;

		void setScale(Ogre::Real scale);

		Ogre::Real getScale(void) const;

		void setCategories(const Ogre::String& categories);

		Ogre::String getCategories(void) const;

		void setMaxHeight(Ogre::Real maxHeight);

		Ogre::Real getMaxHeight(void) const;

		void setRenderDistance(Ogre::Real renderDistance);

		Ogre::Real getRenderDistance(void) const;

		void setVegetationTypesCount(unsigned int vegetationTypesCount);

		unsigned int getVegetationTypesCount(void) const;

		void setVegetationGameObjectId(unsigned int index, unsigned long id);

		unsigned long getVegetationGameObjectId(unsigned int index) const;

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
			return "Usage: Its possible to generate controlled random vegetation and map over a target game object.";
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
		static const Ogre::String AttrCategories(void) { return "Categories"; }
		static const Ogre::String AttrMaxHeight(void) { return "Max Height"; }
		static const Ogre::String AttrRenderDistance(void) { return "Render Distance"; }
		static const Ogre::String AttrVegetationTypesCount(void) { return "Vegetation Types Count"; }
		static const Ogre::String AttrVegetationGameObjectId(void) { return "Vegetation GameObject Id "; }
		static const Ogre::String AttrVegetationMeshName(void) { return "Vegetation Mesh Name "; }
	private:
		void clearLists();
		void regenerateVegetation();

		void handleUpdateBounds(NOWA::EventDataPtr eventData);
		void handleSceneParsed(NOWA::EventDataPtr eventData);
	private:
		Ogre::String name;

		Ogre::Vector3 minimumBounds;
		Ogre::Vector3 maximumBounds;
		std::vector<unsigned long> vegetationGameObjectIdList;
		std::vector<Ogre::Item*> vegetationItemList;
		Ogre::RaySceneQuery* raySceneQuery;

		Variant* targetGameObjectId;
		Variant* density;
		Variant* positionXZ;
		Variant* scale;
		Variant* categories;
		Variant* maxHeight;
		Variant* renderDistance;
		Variant* vegetationTypesCount;
		std::vector<Variant*> vegetationGameObjectIds;
		std::vector<Variant*> vegetationMeshNames;
	};

}; // namespace end

#endif

