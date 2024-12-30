#ifndef LIGHT_DIRECTIONAL_COMPONENT_H
#define LIGHT_DIRECTIONAL_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED LightDirectionalComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::LightDirectionalComponent> LightDirectionalCompPtr;
	public:
	
		LightDirectionalComponent();

		virtual ~LightDirectionalComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("LightDirectionalComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LightDirectionalComponent";
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
			return "Usage: Creates a directional light.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

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
		
		void setAttenuationRadius(Ogre::Real attenuationRadius);
	
		Ogre::Real getAttenuationRadius(void) const;
	
		void setAttenuationLumThreshold(Ogre::Real attenuationLumThreshold);

		Ogre::Real getAttenuationLumThreshold(void) const;

		void setShowDummyEntity(bool showDummyEntity);

		bool getShowDummyEntity(void) const;

		Ogre::Light* getOgreLight(void) const;
	public:
		static const Ogre::String AttrDiffuseColor(void) { return "Diffuse Color"; }
		static const Ogre::String AttrSpecularColor(void) { return "Specular Color"; }
		static const Ogre::String AttrPowerScale(void) { return "Power Scale"; }
		static const Ogre::String AttrDirection(void) { return "Direction"; }
		static const Ogre::String AttrAffectParentNode(void) { return "Affect Node"; }
		static const Ogre::String AttrAttenuationRadius(void) { return "Att-Radius"; }
		static const Ogre::String AttrAttenuationLumThreshold(void) { return "Att-LumThreshold"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrShowDummyEntity(void) { return "Show Dummy Entity"; }
	private:
		void createLight(void);
	private:
		Ogre::Light* light;
		Ogre::v1::Entity* dummyEntity;
		// Variant* lightType;
		Variant* diffuseColor;
		Variant* specularColor;
		Variant* powerScale;
		Variant* direction;
		Variant* affectParentNode;
		Variant* attenuationRadius;
		Variant* attenuationLumThreshold;
		Variant* castShadows;
		// ShowDummyEntity will neither be loaded nor saved
		Variant* showDummyEntity;
		
		Ogre::Real transformUpdateTimer;
		Ogre::Quaternion oldOrientation;
	};

}; //namespace end

#endif