#ifndef OCEAN_COMPONENT_H
#define OCEAN_COMPONENT_H

#include "GameObjectComponent.h"

#include "ocean/Ocean.h"
#include "ocean/OgreHlmsOceanDatablock.h"

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

		void setOceanSize(const Ogre::Vector2& size);
		Ogre::Vector2 getOceanSize(void) const;

		void setOceanCenter(const Ogre::Vector3& center);
		Ogre::Vector3 getOceanCenter(void) const;

		Ogre::Ocean* getOcean(void) const;

		void createOcean(void);
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
	private:

		void destroyOcean(void);

		void handleSwitchCamera(EventDataPtr eventData);

		Ogre::String mapOceanBrdfToString(Ogre::OceanBrdf::OceanBrdf brdf);
		Ogre::OceanBrdf::OceanBrdf mapStringToOceanBrdf(const Ogre::String& strBrdf);
	private:
		Ogre::Ocean* ocean;
		Ogre::HlmsDatablock* datablock;
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
	};

}; //namespace end

#endif