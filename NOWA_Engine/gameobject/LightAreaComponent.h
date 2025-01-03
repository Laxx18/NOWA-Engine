#ifndef LIGHT_AREA_COMPONENT_H
#define LIGHT_AREA_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED LightAreaComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::LightAreaComponent> LightAreaCompPtr;
	public:
	
		LightAreaComponent();

		virtual ~LightAreaComponent();

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
			return NOWA::getIdFromName("LightAreaComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LightAreaComponent";
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
			return "Usage: Creates an erea light, from a given texture for nice light shinning patterns.";
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

		void setAttenuationRadius(const Ogre::Vector2& attenuationValues);

		Ogre::Vector2 getAttenuationRadius(void) const;
		
		void setRectSize(const Ogre::Vector2& rectSize);

		Ogre::Vector2 getRectSize(void) const;
		
		void setTextureName(const Ogre::String& textureName);

		Ogre::String getTextureName(void) const;
		
		void setDiffuseMipStart(Ogre::Real diffuseMipStart);

		Ogre::Real getDiffuseMipStart(void) const;
		
		/** For area lights and custom 2d shapes, specifies whether the light lits in both
         *   directions (positive & negative sides of the plane) or if only towards one.
         * @param doubleSided True to enable. Default: false.
         */
        void setDoubleSided(bool doubleSided);
		
        bool getDoubleSided(void) const;
		
		void setAreaLightType(const Ogre::String& areaLightType);

		Ogre::String getAreaLightType(void) const;

		Ogre::Light* getOgreLight(void) const;
	public:
		static const Ogre::String AttrDiffuseColor(void) { return "Diffuse Color"; }
		static const Ogre::String AttrSpecularColor(void) { return "Specular Color"; }
		static const Ogre::String AttrPowerScale(void) { return "Power Scale"; }
		static const Ogre::String AttrDirection(void) { return "Direction"; }
		static const Ogre::String AttrAffectParentNode(void) { return "Affect Node"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrShowDummyEntity(void) { return "Show Dummy Entity"; }
		static const Ogre::String AttrAttenuationRadius(void) { return "Att-Radius"; }
		static const Ogre::String AttrRectSize(void) { return "Rect Size"; }
		static const Ogre::String AttrTextureName(void) { return "Texturename"; }
		static const Ogre::String AttrDiffuseMipStart(void) { return "Diffuse Mip Start"; }
		static const Ogre::String AttrDoubleSided(void) { return "Double Sided"; }
		static const Ogre::String AttrAreaLightType(void) { return "Area Light Type"; }
	private:
		void createLight(void);
	private:
		Ogre::Light* light;
		Ogre::HlmsUnlitDatablock* datablock;
		Variant* diffuseColor;
		Variant* specularColor;
		Variant* powerScale;
		Variant* direction;
		Variant* affectParentNode;
		Variant* castShadows;
		Variant* attenuationRadius; // Vector2
		Variant* rectSize; // Vector2
		Variant* textureName; // String
		Variant* diffuseMipStart;
		Variant* doubleSided;
		Variant* areaLightType;
		// Attenuation based on radius: Will not work that way, because internally attentuation values are calculated from these two values
		
		// Variant* attenuationLumThreshold;
	};

}; //namespace end

#endif