#ifndef LIGHT_SPOT_COMPONENT_H
#define LIGHT_SPOT_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED LightSpotComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::LightSpotComponent> LightSpotCompPtr;
	public:
	
		LightSpotComponent();

		virtual ~LightSpotComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("LightSpotComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LightSpotComponent";
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
			return "Usage: Creates a spot light.";
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
		
		void setAttenuationMode(const Ogre::String& attenuationMode);

		Ogre::String getAttenuationMode(void) const;

		void setAttenuationRange(Ogre::Real attenuationRange);

		Ogre::Real getAttenuationRange(void) const;

		void setAttenuationConstant(Ogre::Real attenuationConstant);

		Ogre::Real getAttenuationConstant(void) const;

		void setAttenuationLinear(Ogre::Real attenuationLinear);

		Ogre::Real getAttenuationLinear(void) const;

		void setAttenuationQuadratic(Ogre::Real attenuationQuadratic);

		Ogre::Real getAttenuationQuadratic(void) const;

		void setCastShadows(bool castShadows);

		bool getCastShadows(void) const;
		
		void setSize(const Ogre::Vector3& size);
		
		Ogre::Vector3 getSize(void) const;
		
		void setNearClipDistance(Ogre::Real nearClipDistance);
		
		Ogre::Real getNearClipDistance(void) const;

		void setShowDummyEntity(bool showDummyEntity);

		bool getShowDummyEntity(void) const;

		void setAttenuationRadius(Ogre::Real attenuationRadius);

		Ogre::Real getAttenuationRadius(void) const;

		void setAttenuationLumThreshold(Ogre::Real attenuationLumThreshold);

		Ogre::Real getAttenuationLumThreshold(void) const;

		Ogre::Light* getOgreLight(void) const;
	public:
		static const Ogre::String AttrDiffuseColor(void) { return "Diffuse Color"; }
		static const Ogre::String AttrSpecularColor(void) { return "Specular Color"; }
		static const Ogre::String AttrPowerScale(void) { return "Power Scale"; }
		static const Ogre::String AttrAttenuationMode(void) { return "Attenuation Mode"; }
		static const Ogre::String AttrAttenuationRange(void) { return "Att-range"; }
		static const Ogre::String AttrAttenuationConstant(void) { return "Att-Constant"; }
		static const Ogre::String AttrAttenuationLinear(void) { return "Att-Linear"; }
		static const Ogre::String AttrAttenuationQuadratic(void) { return "Att-Quadratic"; }
		static const Ogre::String AttrAttenuationRadius(void) { return "Att-Radius"; }
		static const Ogre::String AttrAttenuationLumThreshold(void) { return "Att-LumThreshold"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrSize(void) { return "Size: (angle, angle, float)"; }
		static const Ogre::String AttrNearClipDistance(void) { return "Near Clip Distance"; }
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
		Variant* attenuationMode;
		Variant* attenuationRange;
		Variant* attenuationConstant;
		Variant* attenuationLinear;
		Variant* attenuationQuadratic;
		Variant* attenuationRadius;
		Variant* attenuationLumThreshold;
		Variant* castShadows;
		Variant* size;
		Variant* nearClipDistance;
		// ShowDummyEntity will neither be loaded nor saved
		Variant* showDummyEntity;
	};

}; //namespace end

#endif