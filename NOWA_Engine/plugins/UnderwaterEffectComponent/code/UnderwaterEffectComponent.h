/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef UNDERWATEREFFECTCOMPONENT_H
#define UNDERWATEREFFECTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

	/**
	  * @brief   Underwater post effect component with predefined underwater presets.
	  *          Presets can be selected; if any value is modified manually, preset becomes "Custom".
	  *          Writes constants into material "Ocean/UnderwaterPost".
	  *
	  * Requirements: Must be placed on a GameObject with OceanComponent (post effect depends on camera projection params).
	  */
	class EXPORTED UnderwaterEffectComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<UnderwaterEffectComponent> UnderwaterEffectComponentPtr;
	public:

		UnderwaterEffectComponent();

		virtual ~UnderwaterEffectComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

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
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("UnderwaterEffectComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "UnderwaterEffectComponent";
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
			return "Usage: Enables underwater post effect via material 'Ocean/UnderwaterPost' with several predefined presets "
				"(Clear Tropical, Murky, Deep Ocean, Shallow Reef, Stormy, Balanced, Custom). "
				"Requirements: This component must be placed on a GameObject with OceanComponent.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
	private:
		Ogre::String name;

		void setCameraId(unsigned long cameraId);
		unsigned int geCameraId(void) const;

		// Preset
		void setPresetName(const Ogre::String& presetName);
		Ogre::String getPresetName(void) const;

		// Parameters
		void setWaterTint(const Ogre::Vector3& waterTint);
		Ogre::Vector3 getWaterTint(void) const;

		void setDeepWaterTint(const Ogre::Vector3& deepWaterTint);
		Ogre::Vector3 getDeepWaterTint(void) const;

		void setFogDensity(Ogre::Real fogDensity);
		Ogre::Real getFogDensity(void) const;

		void setFogStart(Ogre::Real fogStart);
		Ogre::Real getFogStart(void) const;

		void setMaxFogDepth(Ogre::Real maxFogDepth);
		Ogre::Real getMaxFogDepth(void) const;

		void setAbsorptionScale(Ogre::Real absorptionScale);
		Ogre::Real getAbsorptionScale(void) const;

		void setGodRayStrength(Ogre::Real godRayStrength);
		Ogre::Real getGodRayStrength(void) const;

		void setGodRayDensity(Ogre::Real godRayDensity);
		Ogre::Real getGodRayDensity(void) const;

		void setMaxGodRayDepth(Ogre::Real maxGodRayDepth);
		Ogre::Real getMaxGodRayDepth(void) const;

		void setSunScreenPos(const Ogre::Vector2& sunScreenPos);
		Ogre::Vector2 getSunScreenPos(void) const;

		void setCausticStrength(Ogre::Real causticStrength);
		Ogre::Real getCausticStrength(void) const;

		void setMaxCausticDepth(Ogre::Real maxCausticDepth);
		Ogre::Real getMaxCausticDepth(void) const;

		void setParticleStrength(Ogre::Real particleStrength);
		Ogre::Real getParticleStrength(void) const;

		void setMaxParticleDepth(Ogre::Real maxParticleDepth);
		Ogre::Real getMaxParticleDepth(void) const;

		void setBubbleStrength(Ogre::Real bubbleStrength);
		Ogre::Real getBubbleStrength(void) const;

		void setScatterDensity(Ogre::Real scatterDensity);
		Ogre::Real getScatterDensity(void) const;

		void setScatterColor(const Ogre::Vector3& scatterColor);
		Ogre::Vector3 getScatterColor(void) const;

		void setDistortion(Ogre::Real distortion);
		Ogre::Real getDistortion(void) const;

		void setContrast(Ogre::Real contrast);
		Ogre::Real getContrast(void) const;

		void setSaturation(Ogre::Real saturation);
		Ogre::Real getSaturation(void) const;

		void setVignette(Ogre::Real vignette);
		Ogre::Real getVignette(void) const;

		void setChromaticAberration(Ogre::Real chromaticAberration);
		Ogre::Real getChromaticAberration(void) const;

	private:
		struct UnderwaterParams
		{
			Ogre::Vector3 waterTint;
			Ogre::Vector3 deepWaterTint;

			Ogre::Real fogDensity;
			Ogre::Real fogStart;
			Ogre::Real maxFogDepth;

			Ogre::Real absorptionScale;

			Ogre::Real godRayStrength;
			Ogre::Real godRayDensity;
			Ogre::Real maxGodRayDepth;
			Ogre::Vector2 sunScreenPos;

			Ogre::Real causticStrength;
			Ogre::Real maxCausticDepth;

			Ogre::Real particleStrength;
			Ogre::Real maxParticleDepth;

			Ogre::Real bubbleStrength;

			Ogre::Real scatterDensity;
			Ogre::Vector3 scatterColor;

			Ogre::Real distortion;
			Ogre::Real contrast;
			Ogre::Real saturation;
			Ogre::Real vignette;
			Ogre::Real chromaticAberration;
		};

	private:
		void applyUnderwaterParamsToMaterial(void);
		UnderwaterParams gatherParams(void) const;

		void markCustomAndApply(void);

		void handleSwitchCamera(EventDataPtr eventData);

	public:
		static const Ogre::String AttrCameraId(void) { return "Camera Id"; }
		static const Ogre::String AttrPresetName(void) { return "Preset Name"; }
		static const Ogre::String AttrWaterTint(void) { return "Water Tint"; }
		static const Ogre::String AttrDeepWaterTint(void) { return "Deep Water Tint"; }
		static const Ogre::String AttrFogDensity(void) { return "Fog Density"; }
		static const Ogre::String AttrFogStart(void) { return "Fog Start"; }
		static const Ogre::String AttrMaxFogDepth(void) { return "Max Fog Depth"; }
		static const Ogre::String AttrAbsorptionScale(void) { return "Absorption Scale"; }
		static const Ogre::String AttrGodRayStrength(void) { return "God Ray Strength"; }
		static const Ogre::String AttrGodRayDensity(void) { return "God Ray Density"; }
		static const Ogre::String AttrMaxGodRayDepth(void) { return "Max God Ray Depth"; }
		static const Ogre::String AttrSunScreenPos(void) { return "Sun Screen Pos"; }
		static const Ogre::String AttrCausticStrength(void) { return "Caustic Strength"; }
		static const Ogre::String AttrMaxCausticDepth(void) { return "Max Caustic Depth"; }
		static const Ogre::String AttrParticleStrength(void) { return "Particle Strength"; }
		static const Ogre::String AttrMaxParticleDepth(void) { return "Max Particle Depth"; }
		static const Ogre::String AttrBubbleStrength(void) { return "Bubble Strength"; }
		static const Ogre::String AttrScatterDensity(void) { return "Scatter Density"; }
		static const Ogre::String AttrScatterColor(void) { return "Scatter Color"; }
		static const Ogre::String AttrDistortion(void) { return "Distortion"; }
		static const Ogre::String AttrContrast(void) { return "Contrast"; }
		static const Ogre::String AttrSaturation(void) { return "Saturation"; }
		static const Ogre::String AttrVignette(void) { return "Vignette"; }
		static const Ogre::String AttrChromaticAberration(void) { return "Chromatic Aberration"; }

	private:
		Variant* cameraId;
		Variant* presetName;

		Variant* waterTint;
		Variant* deepWaterTint;

		Variant* fogDensity;
		Variant* fogStart;
		Variant* maxFogDepth;

		Variant* absorptionScale;

		Variant* godRayStrength;
		Variant* godRayDensity;
		Variant* maxGodRayDepth;
		Variant* sunScreenPos;

		Variant* causticStrength;
		Variant* maxCausticDepth;

		Variant* particleStrength;
		Variant* maxParticleDepth;

		Variant* bubbleStrength;

		Variant* scatterDensity;
		Variant* scatterColor;

		Variant* distortion;
		Variant* contrast;
		Variant* saturation;
		Variant* vignette;
		Variant* chromaticAberration;

		Ogre::Camera* usedCamera;
	};

}; // namespace end

#endif

