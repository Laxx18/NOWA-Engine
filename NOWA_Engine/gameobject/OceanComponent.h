#ifndef OCEAN_COMPONENT_H
#define OCEAN_COMPONENT_H

#include "GameObjectComponent.h"

#include "ocean/Ocean.h"
#include "ocean/OgreHlmsOceanDatablock.h"
#include "ocean/OgreHlmsOcean.h"
#include "OgreHlmsPbs.h"

#include "main/EventManager.h"

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
			return
				"OceanComponent renders a large scale animated ocean surface with LOD support and optional skirts.\n"
				"\n"
				"Main parameters:\n"
				"WavesIntensity controls overall wave height.\n"
				"OceanWavesScale controls large slow swells.\n"
				"ShaderWavesScale controls fine surface detail.\n"
				"WaveTimeScale controls animation speed.\n"
				"WaveFrequencyScale controls wave density.\n"
				"WaveChaos adds non uniform motion to reduce repetition.\n"
				"UseSkirts hides cracks between ocean LOD cells and should normally be enabled. But if underwater, they will be disabled automatically.\n"
				"\n"
				"Preset examples:\n"
				"Calm Lake: WavesIntensity=0.2 OceanWavesScale=1.0 ShaderWavesScale=1.0 WaveTimeScale=0.55 WaveFrequencyScale=0.65 WaveChaos=0.03\n"
				"Gentle Ocean: WavesIntensity=0.55 OceanWavesScale=1.0 ShaderWavesScale=1.0 WaveTimeScale=1.0 WaveFrequencyScale=1.0 WaveChaos=0.10\n"
				"Choppy Water: WavesIntensity=0.75 OceanWavesScale=0.85 ShaderWavesScale=1.15 WaveTimeScale=1.25 WaveFrequencyScale=1.45 WaveChaos=0.30\n"
				"Storm Sea: WavesIntensity=1.35 OceanWavesScale=1.15 ShaderWavesScale=1.10 WaveTimeScale=1.35 WaveFrequencyScale=0.95 WaveChaos=0.45\n"
				"Big Swell: WavesIntensity=1.10 OceanWavesScale=1.60 ShaderWavesScale=0.85 WaveTimeScale=0.75 WaveFrequencyScale=0.60 WaveChaos=0.12\n"
				"Fantasy Water: WavesIntensity=1.60 OceanWavesScale=1.10 ShaderWavesScale=1.30 WaveTimeScale=2.10 WaveFrequencyScale=1.35 WaveChaos=0.65\n";
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

		void setCameraId(unsigned long cameraId);

		unsigned int geCameraId(void) const;

		// Ocean visual settings (saved/loaded + live-updatable)
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
        void setUseSkirts( bool useSkirts );
        bool getUseSkirts( void ) const;

        // Extra runtime wave controls (packed into cellData.oceanTime.yzw)
        void setWaveTimeScale( Ogre::Real timeScale );
        Ogre::Real getWaveTimeScale( void ) const;

        void setWaveFrequencyScale( Ogre::Real freqScale );
        Ogre::Real getWaveFrequencyScale( void ) const;

        void setWaveChaos( Ogre::Real chaos );
        Ogre::Real getWaveChaos( void ) const;

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


		Ogre::Ocean* getOcean(void) const;

		Ogre::HlmsOceanDatablock* getDatablock(void) const;

		bool isCameraUnderwater(void) const;

		bool isUnderWater(const Ogre::Vector3& position) const;

		bool isUnderWater(GameObject* gameObject);

		void createOcean(void);

		void setEnvTexture(const Ogre::String& envTextureName);

		// Reflection cubemap (resource name from group "Skies", e.g. "SunnySky.dds")
		void setReflectionTextureName(const Ogre::String& textureName);
		Ogre::String getReflectionTextureName(void) const;

		// Refresh list of available reflection textures (Skies/*.dds)
		void setReflectionTextureNames(void);

		// Call it if e.g. PBS ambient mode or shadow settings changed
		void actualizeShading(void);
	public:
		static const Ogre::String AttrCameraId(void) { return "Camera Id"; }
		static const Ogre::String AttrDeepColour(void) { return "Deep Colour"; }
		static const Ogre::String AttrShallowColour(void) { return "Shallow Colour"; }
		static const Ogre::String AttrBrdf(void) { return "BRDF"; }
		static const Ogre::String AttrShaderWavesScale(void) { return "Shader Waves Scale"; }
		static const Ogre::String AttrWavesIntensity(void) { return "Waves Intensity"; }
		static const Ogre::String AttrOceanWavesScale(void) { return "Ocean Waves Scale"; }
		static const Ogre::String AttrOceanSize(void) { return "Ocean Size"; }
		static const Ogre::String AttrOceanCenter(void) { return "Ocean Center"; }
        static const Ogre::String AttrUseSkirts(void) { return "Use Skirts"; }
        static const Ogre::String AttrWaveTimeScale(void) { return "Wave Time Scale"; }
        static const Ogre::String AttrWaveFrequencyScale(void) { return "Wave Frequency Scale"; }
        static const Ogre::String AttrWaveChaos(void) { return "Wave Chaos"; }
		static const Ogre::String AttrReflectionStrength(void) { return "Reflection Strength"; }
		static const Ogre::String AttrBaseRoughness(void) { return "Base Roughness"; }
		static const Ogre::String AttrFoamRoughness(void) { return "Foam Roughness"; }
		static const Ogre::String AttrAmbientReduction(void) { return "Ambient Reduction"; }
		static const Ogre::String AttrDiffuseScale(void) { return "Diffuse Scale"; }
		static const Ogre::String AttrFoamIntensity(void) { return "Foam Intensity"; }
        static const Ogre::String AttrReflectionTexture(void) { return "Reflection Texture"; }
	private:

		void destroyOcean(void);

		void handleSwitchCamera(EventDataPtr eventData);

		Ogre::String mapOceanBrdfToString(Ogre::OceanBrdf::OceanBrdf brdf);
		Ogre::OceanBrdf::OceanBrdf mapStringToOceanBrdf(const Ogre::String& strBrdf);

		// Ambient mode: PBS -> Ocean
		Ogre::HlmsOcean::AmbientLightMode mapPbsLightModeToOceanMapMode(Ogre::HlmsPbs::AmbientLightMode pbsMode);

		// Shadow filter: PBS -> Ocean
		Ogre::HlmsOcean::ShadowFilter mapPbsShadowSettingsToOceanShadowSettings(Ogre::HlmsPbs::ShadowFilter pbsFilter);
	private:
		Ogre::Ocean* ocean;
		Ogre::HlmsOceanDatablock* datablock;
		bool postInitDone;
		Ogre::Camera* usedCamera;

		Variant* cameraId;
		Variant* deepColour;
		Variant* shallowColour;
		Variant* brdf;
		Variant* shaderWavesScale;
		Variant* wavesIntensity;
		Variant* oceanWavesScale;
		Variant* oceanSize;
		Variant* oceanCenter;
        Variant* useSkirts;
        Variant* waveTimeScale;
        Variant* waveFrequencyScale;
        Variant* waveChaos;
		Variant* reflectionStrength;
		Variant* baseRoughness;
		Variant* foamRoughness;
		Variant* ambientReduction;
		Variant* diffuseScale;
		Variant* foamIntensity;
		Variant* reflectionTextureName;
	};

} //namespace end

#endif