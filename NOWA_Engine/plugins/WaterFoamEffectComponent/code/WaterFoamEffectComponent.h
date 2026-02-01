/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef WATERFOAMEFFECTCOMPONENT_H
#define WATERFOAMEFFECTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class OceanComponent;

	class EXPORTED WaterFoamEffectComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<NOWA::WaterFoamEffectComponent> WaterFoamEffectCompPtr;

		WaterFoamEffectComponent();
		virtual ~WaterFoamEffectComponent();

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

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
		virtual bool postInit(void) override;

		virtual bool connect(void) override;
		virtual bool disconnect(void) override;

		virtual void onRemoveComponent(void) override;

		virtual Ogre::String getClassName(void) const override;
		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
		virtual bool onCloned(void) override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;
		virtual void actualizeValue(Variant* attribute) override;
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("WaterFoamEffectComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "WaterFoamEffectComponent";
		}

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		static Ogre::String getStaticInfoText(void)
		{
			return
				"WaterFoamEffectComponent spawns and controls a ParticleUniverse wake/foam effect for a player.\n"
				"\n"
				"What it does:\n"
				"- Looks up an OceanComponent via Ocean Id.\n"
				"- Checks if the owner is close enough to the ocean surface.\n"
				"- If touching water: creates + plays a ParticleUniverse system and keeps it on the surface.\n"
				"- If not touching water: stops and removes the particle system.\n"
				"\n"
				"Typical use:\n"
				"- Attach this component to a Player GameObject.\n"
				"- Set Ocean Id to the GameObject id that owns the OceanComponent.\n"
				"- Use a ParticleUniverse system like 'wake' (converted from your old Water/Wake).\n";
		}

		// Properties
		void setOceanId(unsigned long oceanId);
		unsigned long getOceanId(void) const;

		void setParticleTemplateName(const Ogre::String& templateName);
		Ogre::String getParticleTemplateName(void) const;

		void setParticleScale(Ogre::Real scale);
		Ogre::Real getParticleScale(void) const;

		void setPlayTimeMs(Ogre::Real playTimeMs);
		Ogre::Real getPlayTimeMs(void) const;

		void setWaterContactHeight(Ogre::Real waterContactHeight);
		Ogre::Real getWaterContactHeight(void) const;

		void setSurfaceOffset(Ogre::Real surfaceOffset);
		Ogre::Real getSurfaceOffset(void) const;

		void setParticleOffset(const Ogre::Vector3& offset);
		Ogre::Vector3 getParticleOffset(void) const;

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrOceanId(void)
		{
			return "Ocean Id";
		}
		static const Ogre::String AttrParticleTemplateName(void)
		{
			return "Particle Template";
		}
		static const Ogre::String AttrParticleScale(void)
		{
			return "Particle Scale";
		}
		static const Ogre::String AttrPlayTimeMs(void)
		{
			return "Play Time (ms)";
		}
		static const Ogre::String AttrWaterContactHeight(void)
		{
			return "Water Contact Height";
		}
		static const Ogre::String AttrSurfaceOffset(void)
		{
			return "Surface Offset";
		}
		static const Ogre::String AttrParticleOffset(void)
		{
			return "Particle Offset";
		}

	private:
		void resolveOceanComponent(void);

		void startEffect(void);
		void stopEffect(void);
		void updateEffectTransform(void);

		Ogre::String getParticleSystemInstanceName(void) const;
		Ogre::String getTrackedClosureId(void) const;

	private:
		Ogre::String name;
		OceanComponent* oceanComponent;
		bool postInitDone;
		bool effectActive;

		Variant* oceanId;
		Variant* particleTemplateName;
		Variant* particleScale;
		Variant* playTimeMs;
		Variant* waterContactHeight;
		Variant* surfaceOffset;
		Variant* particleOffset;
	};

}; // namespace end

#endif

