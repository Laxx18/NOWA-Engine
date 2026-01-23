/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef OCEANEFFECTCOMPONENT_H
#define OCEANEFFECTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	  * @brief   Ocean visual/wave preset component.
	  *          Requires OceanComponent on the same GameObject. Camera is NOT required.
	  *
	  * Presets affect the main wave parameters (Intensity/Scales/Time/Frequency/Chaos).
	  * If any parameter is changed manually, preset becomes "Custom".
	  */
	class EXPORTED OceanEffectComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<NOWA::OceanEffectComponent> OceanEffectCompPtr;

	public:
		OceanEffectComponent();
		virtual ~OceanEffectComponent();

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
		 *@see		GameObjectComponent::disconnect
		 */
		virtual bool disconnect(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

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
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("OceanEffectComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "OceanEffectComponent";
		}

		static bool canStaticAddComponent(GameObject* gameObject);

		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Controls the OceanComponent with presets and manual tuning of waves/colors/roughness/foam etc. "
				"Requirements: This component can only be set on a GameObject with OceanComponent.";
		}

	public:
		// Preset selection
		void setPresetName(const Ogre::String& presetName);
		Ogre::String getPresetName(void) const;

		// Ocean visual settings
		void setDeepColour(const Ogre::Vector3& deepColour);
		Ogre::Vector3 getDeepColour(void) const;

		void setShallowColour(const Ogre::Vector3& shallowColour);
		Ogre::Vector3 getShallowColour(void) const;

		void setBrdf(const Ogre::String& brdf);
		Ogre::String getBrdf(void) const;

		// Shader-side detail scale (HlmsOceanDatablock::setWavesScale)
		void setShaderWavesScale(Ogre::Real wavesScale);
		Ogre::Real getShaderWavesScale(void) const;

		// Amplitude (Ocean::setWavesIntensity)
		void setWavesIntensity(Ogre::Real intensity);
		Ogre::Real getWavesIntensity(void) const;

		// Texture tiling / wave size (Ocean::setWavesScale)
		void setOceanWavesScale(Ogre::Real wavesScale);
		Ogre::Real getOceanWavesScale(void) const;

		// Skirts (to hide LOD cracks). Needs ocean rebuild to take effect.
		void setUseSkirts(bool useSkirts);
		bool getUseSkirts(void) const;

		// Extra runtime wave controls (packed into cellData.oceanTime.yzw)
		void setWaveTimeScale(Ogre::Real timeScale);
		Ogre::Real getWaveTimeScale(void) const;

		void setWaveFrequencyScale(Ogre::Real freqScale);
		Ogre::Real getWaveFrequencyScale(void) const;

		void setWaveChaos(Ogre::Real chaos);
		Ogre::Real getWaveChaos(void) const;

		void setOceanSize(const Ogre::Vector2& size);
		Ogre::Vector2 getOceanSize(void) const;

		void setOceanCenter(const Ogre::Vector3& center);
		Ogre::Vector3 getOceanCenter(void) const;

		void setReflectionStrength(Ogre::Real v);
		Ogre::Real getReflectionStrength(void) const;

		void setBaseRoughness(Ogre::Real v);
		Ogre::Real getBaseRoughness(void) const;

		void setFoamRoughness(Ogre::Real v);
		Ogre::Real getFoamRoughness(void) const;

		void setAmbientReduction(Ogre::Real v);
		Ogre::Real getAmbientReduction(void) const;

		void setDiffuseScale(Ogre::Real v);
		Ogre::Real getDiffuseScale(void) const;

		void setFoamIntensity(Ogre::Real v);
		Ogre::Real getFoamIntensity(void) const;

	private:
		void applyAllSettingsToOcean(void);
		void markCustom(void);

	public:
		static const Ogre::String AttrPresetName(void)
		{
			return "Preset Name";
		}
		static const Ogre::String AttrDeepColour(void)
		{
			return "Deep Colour";
		}
		static const Ogre::String AttrShallowColour(void)
		{
			return "Shallow Colour";
		}
		static const Ogre::String AttrBrdf(void)
		{
			return "BRDF";
		}

		static const Ogre::String AttrShaderWavesScale(void)
		{
			return "Shader Waves Scale";
		}
		static const Ogre::String AttrWavesIntensity(void)
		{
			return "Waves Intensity";
		}
		static const Ogre::String AttrOceanWavesScale(void)
		{
			return "Ocean Waves Scale";
		}
		static const Ogre::String AttrUseSkirts(void)
		{
			return "Use Skirts";
		}

		static const Ogre::String AttrWaveTimeScale(void)
		{
			return "Wave Time Scale";
		}
		static const Ogre::String AttrWaveFrequencyScale(void)
		{
			return "Wave Frequency Scale";
		}
		static const Ogre::String AttrWaveChaos(void)
		{
			return "Wave Chaos";
		}

		static const Ogre::String AttrOceanSize(void)
		{
			return "Ocean Size";
		}
		static const Ogre::String AttrOceanCenter(void)
		{
			return "Ocean Center";
		}

		static const Ogre::String AttrReflectionStrength(void)
		{
			return "Reflection Strength";
		}
		static const Ogre::String AttrBaseRoughness(void)
		{
			return "Base Roughness";
		}
		static const Ogre::String AttrFoamRoughness(void)
		{
			return "Foam Roughness";
		}
		static const Ogre::String AttrAmbientReduction(void)
		{
			return "Ambient Reduction";
		}
		static const Ogre::String AttrDiffuseScale(void)
		{
			return "Diffuse Scale";
		}
		static const Ogre::String AttrFoamIntensity(void)
		{
			return "Foam Intensity";
		}

	private:
		Ogre::String name;

		Variant* presetName;

		Variant* deepColour;
		Variant* shallowColour;
		Variant* brdf;

		Variant* shaderWavesScale;
		Variant* wavesIntensity;
		Variant* oceanWavesScale;
		Variant* useSkirts;

		Variant* waveTimeScale;
		Variant* waveFrequencyScale;
		Variant* waveChaos;

		Variant* oceanSize;
		Variant* oceanCenter;

		Variant* reflectionStrength;
		Variant* baseRoughness;
		Variant* foamRoughness;
		Variant* ambientReduction;
		Variant* diffuseScale;
		Variant* foamIntensity;

		OceanComponent* oceanComponent;

		// internal guard to avoid "preset -> custom" during preset application
		bool applyingPreset;
	};

}; // namespace end

#endif

