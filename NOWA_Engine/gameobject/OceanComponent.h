#ifndef OCEAN_COMPONENT_H
#define OCEAN_COMPONENT_H

#include "GameObjectComponent.h"

#include "ocean/Ocean.h"

namespace NOWA
{
	class EXPORTED OceanComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::OceanComponent> OceanCompPtr;
	public:
	
		OceanComponent();

		virtual ~OceanComponent();

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

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("OceanComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "OceanComponent";
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
			return "Usage: Render a nice ocean.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/*virtual void setActivated(bool activated) override;

		void setDiffuseColor(const Ogre::Vector3& diffuseColor);

		Ogre::Vector3 getDiffuseColor(void) const;

		void setSpecularColor(const Ogre::Vector3& specularColor);

		Ogre::Vector3 getSpecularColor(void) const;

		void setPowerScale(Ogre::Real powerScale);

		Ogre::Real getPowerScale(void) const;

		void setDirection(const Ogre::Vector3& direction);

		Ogre::Vector3 getDirection(void) const;

		void setAffectParentNode(bool affectParentNode);

		bool getAffectParentNode(void) const;

		void setCastShadows(bool castShadows);

		bool getCastShadows(void) const;

		void setShowDummyEntity(bool showDummyEntity);

		bool getShowDummyEntity(void) const;*/

		Ogre::Ocean* getOcean(void) const;

		void createOcean(void);
	public:
		static const Ogre::String AttrDiffuseColor(void) { return "Diffuse Color"; }
		static const Ogre::String AttrSpecularColor(void) { return "Specular Color"; }
		static const Ogre::String AttrPowerScale(void) { return "Power Scale"; }
		static const Ogre::String AttrDirection(void) { return "Direction"; }
		static const Ogre::String AttrAffectParentNode(void) { return "Affect Node"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrShowDummyEntity(void) { return "Show Dummy Entity"; }
		// static const Ogre::String AttrAttenuationRadius(void) { return "Att-Radius"; }
		// static const Ogre::String AttrAttenuationLumThreshold(void) { return "Att-LumThreshold"; }
	private:

		void destroyOcean(void);
	private:
		Ogre::Ocean* ocean;
		Ogre::HlmsDatablock* datablock;
		// Variant* lightType;
		//Variant* diffuseColor;
		//Variant* specularColor;
		//Variant* powerScale;
		//Variant* direction;
		//Variant* affectParentNode;
		//Variant* castShadows;
		//// ShowDummyEntity will neither be loaded nor saved
		//Variant* showDummyEntity;
	};

}; //namespace end

#endif