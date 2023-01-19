#ifndef HDR_EFFECT_COMPONENT_H
#define HDR_EFFECT_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	/*
	Light sand or snow in full or slightly hazy sunlight (distinct shadows)a	16
	Typical scene in full or slightly hazy sunlight (distinct shadows)a, b	15
	Typical scene in hazy sunlight (soft shadows)	14
	Typical scene, cloudy bright (no shadows)	13
	Typical scene, heavy overcast	12
	Areas in open shade, clear sunlight	12
	Outdoor, natural light
	Rainbows
	Clear sky background	15
	Cloudy sky background	14
	Sunsets and skylines
	Just before sunset	12–14
	At sunset	12
	Just after sunset	9–11
	The Moon,c altitude > 40°
	Full	15
	Gibbous	14
	Quarter	13
	Crescent	12
	Blood	0 to 3[6]
	Moonlight, Moon altitude > 40°
	Full	−3 to −2
	Gibbous	−4
	Quarter	−6
	Aurora borealis and australis
	Bright	−4 to −3
	Medium	−6 to −5
	Milky Way galactic center	−11 to −9
	Outdoor, artificial light
	Neon and other bright signs	9–10
	Night sports	9
	Fires and burning buildings	9
	Bright street scenes	8
	Night street scenes and window displays	7–8
	Night vehicle traffic	5
	Fairs and amusement parks	7
	Christmas tree lights	4–5
	Floodlit buildings, monuments, and fountains	3–5
	Distant views of lighted buildings	2
	Indoor, artificial light
	Galleries	8–11
	Sports events, stage shows, and the like	8–9
	Circuses, floodlit	8
	Ice shows, floodlit	9
	Offices and work areas	7–8
	Home interiors	5–7
	Christmas tree lights	4–5
	*/

	class LightDirectionalComponent;
	class WorkspaceBaseComponent;

	class EXPORTED HdrEffectComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::HdrEffectComponent> HdrEffectCompPtr;
	public:

		HdrEffectComponent();

		virtual ~HdrEffectComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

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
		virtual void onRemoveComponent(void);

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("HdrEffectComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "HdrEffectComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Enables HDR with several predefined profiles and possibility to create own HDR calibration. "
				   "Requirements: This component can only be set directly under a workspace component.";
		}

		void setEffectName(const Ogre::String& effectName);

		Ogre::String getEffectName(void) const;

		void setSkyColor(const Ogre::Vector4& skyColor);

		Ogre::Vector4 getSkyColor(void) const;

		void setUpperHemisphere(const Ogre::Vector4& upperHemisphere);

		Ogre::Vector4 getUpperHemisphere(void) const;

		void setLowerHemisphere(const Ogre::Vector4& lowerHemisphere);

		Ogre::Vector4 getLowerHemisphere(void) const;
		
		void setSunPower(Ogre::Real sunPower);

		Ogre::Real getSunPower(void) const;

		void setExposure(Ogre::Real exposure);

		Ogre::Real getExposure(void) const;

		void setMinAutoExposure(Ogre::Real minAutoExposure);

		Ogre::Real getMinAutoExposure(void) const;

		void setMaxAutoExposure(Ogre::Real maxAutoExposure);

		Ogre::Real getMaxAutoExposure(void) const;

		void setBloom(Ogre::Real bloom);

		Ogre::Real getBloom(void) const;

		void setEnvMapScale(Ogre::Real envMapScale);

		Ogre::Real getEnvMapScale(void) const;

		void applyHdrSkyColor(const Ogre::ColourValue& color, Ogre::Real multiplier);

		/** Modifies the HDR Materials for the new exposure parameters
			By default the HDR implementation will try to auto adjust the
			exposure based on the scene's average luminance.
		@par
			If left unbounded, even the darkest scenes can look well lit
			and the brigthest scenes appear too normal.
		@par
			These parameters are useful to prevent the auto exposure from
			jumping too much from one extreme to the other and provide
			a consistent experience within the same lighting conditions.
			(e.g. you may want to change the params when going from indoors to
			outdoors)
		@par
			The smaller the gap between minAutoExposure & maxAutoExposure, the
			less the auto exposure tries to auto adjust to the scene's lighting
			conditions.
		@param exposure
			Exposure in EV. Valid range is [-inf; inf]
			Low values will make the picture darker.
			Higher values will make the picture brighter.
		@param minAutoExposure
			It's in EV stops. Valid range is [-inf; inf]
			Must be minAutoExposure <= maxAutoExposure
			Controls how much auto exposure darkens a bright scene.

			To prevent that looking at a very bright object makes the rest of
			the scene really dark, use higher values.
		@param maxAutoExposure
			It's in EV stops. Valid range is [-inf; inf]
			Must be minAutoExposure <= maxAutoExposure
			Controls how much auto exposure brightens a dark scene.

			To prevent that looking at a very dark object makes the rest of
			the scene really bright, use lower values.
		*/
		void applyExposure(Ogre::Real exposure, Ogre::Real minAutoExposure, Ogre::Real maxAutoExposure);

		/** Controls the bloom intensity.
		@param minThreshold
			Colors darker than minThreshold will not contribute to bloom.
			Higher thresholds reduce the amount of bloom.
			Scale is in lumens / 1024.
		@param fullColorThreshold
			The bloom fades between minThreshold & fullColorThreshold.
			Must be > minThreshold. If it is too close to it, bloom fading
			may be too harsh/sudden.
			Scale is the same as minThreshold.
		*/
		void applyBloomThreshold(Ogre::Real minThreshold, Ogre::Real fullColorThreshold);
	private:
			void resetShining(void);
	public:
		static const Ogre::String AttrEffectName(void) { return "Effect Name"; }
		static const Ogre::String AttrSkyColor(void) { return "Sky Color"; }
		static const Ogre::String AttrUpperHemisphere(void) { return "Upper Hemisphere"; }
		static const Ogre::String AttrLowerHemisphere(void) { return "Lower Hemisphere"; }
		static const Ogre::String AttrSunPower(void) { return "Sun Power"; }
		static const Ogre::String AttrExposure(void) { return "Exposure"; }
		static const Ogre::String AttrMinAutoExposure(void) { return "Min Auto Exposure"; }
		static const Ogre::String AttrMaxAutoExposure(void) { return "Max Auto Exposure"; }
		static const Ogre::String AttrBloom(void) { return "Bloom"; }
		static const Ogre::String AttrEnvMapScale(void) { return "Env Map Scale"; }
	private:
		Variant* effectName;
		Variant* skyColor;
		Variant* upperHemisphere;
		Variant* lowerHemisphere;
		Variant* sunPower;
		Variant* exposure;
		Variant* minAutoExposure;
		Variant* maxAutoExposure;
		Variant* bloom;
		Variant* envMapScale;
		LightDirectionalComponent* lightDirectionalComponent;
		WorkspaceBaseComponent* workspaceBaseComponent;
	};

}; //namespace end

#endif