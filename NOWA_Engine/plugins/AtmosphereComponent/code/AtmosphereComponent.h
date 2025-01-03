/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
*/

#ifndef ATMOSPHERECOMPONENT_H
#define ATMOSPHERECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "OgreAtmosphereNpr.h"

namespace NOWA
{
	class LightDirectionalComponent;

	/**
	  * @brief		Your documentation
	  */
	class EXPORTED AtmosphereComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<AtmosphereComponent> AtmosphereComponentPtr;
	public:

		AtmosphereComponent();

		virtual ~AtmosphereComponent();

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
		* @see		GameObjectComponent::disconnect
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
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AtmosphereComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "AtmosphereComponent";
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
			return "Usage: Shows an atmospheric effect on the scene. The technique is called Atmosphere NPR(non - physically - based - render) Component. Fog is also applied to objects based on the atmosphere."
				"It supports blending between multiple presets at different Times of Day (day & night)."
				"This allows fine tunning the brightness at different timedays, specially the fake GI"
				"multipliers in LDR.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		/**
		 * @brief Sets whether sky is enabled and visible.
		 * @param[in] enableSky 				The flag to set
		 */
		void setEnableSky(bool enableSky);

		/**
		 * @brief Gets whether sky is enabled and visible.
		 * @return enableSky 					If true, sky is enabled, else not
		 */
		bool getEnableSky(void) const;

		/**
		 * @brief Sets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight.
		 * @param[in] startTime				The start time to set.
		 */
		void setStartTime(const Ogre::String& startTime);

		/**
		 * @brief Gets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight.
		  * @return startTime 					The start time as string to get
		 */
		Ogre::String getStartTime(void) const;

		/**
		 * @brief Gets the current time of day as string.
		 * @return currentTimeOfDay 			The current time of day as string
		 */
		Ogre::String getCurrentTimeOfDay(void);

		/**
		 * @brief Gets the current time of day in minutes.
		 * @return currentTimeOfDayMinutes 		The current time of day in minutes
		 */
		int getCurrentTimeOfDayMinutes(void);

		/**
		 * @brief Sets the time multiplier. How long a day lasts.
		 * @note								Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].
		 * @param[in] timeMultiplicator 		The flag to set
		 */
		void setTimeMultiplicator(Ogre::Real timeMultiplicator);

		/**
		 * @brief Gets the time multiplier. How long a day lasts.
		 * @note								Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].
		 * @return timeMultiplicator			The time multiplicator
		 */
		Ogre::Real getTimeMultiplicator(void) const;

		/**
		 * @brief Sets the time of day in range [-1; 1] where range [-1; 0] is night and [0; 1].
		 * @param[in] timeOfDay 				The time of day to set
		 */
		void setTimeOfDay(Ogre::Real timeOfDay);

		/**
		 * @brief Gets the time of day in the range [-1; 1] where range [-1; 0] is night and [0; 1].
		  * @return timeOfDay			The time of day
		 */
		Ogre::Real getTimeOfDay(void) const;

		/**
		 * @brief Sets the azimuth in the range [0; 2PI].
		 * @param[in] azimuth 				The azimuth to set
		 */
		void setAzimuth(Ogre::Real azimuth);

		/**
		 * @brief Gets the azimuth in the range [0; 2PI].
		 * @return azimuth			The azimuth
		 */
		Ogre::Real getAzimuth(void) const;

		/**
		 * @brief Sets the preset count.
		 * @param[in]							presetCount The preset count to set
		 * @note								Each preset will store atmospheric parameters and if the current time reaches the corresponding preset, those parameters will become active, for the given atmospheric effect.
		 *										There are only up to 12 presets configurable.
		 */
		void setPresetCount(unsigned int presetCount);

		/**
		 * @brief Gets the preset count
		 * @return presetCount					The preset count
		 */
		unsigned int getPresetCount(void) const;

		/**
		 * @brief Sets the time for the preset in the format hh:mm, e.g. 16:45.
		 * @note								The time must be in range [0; 24[.
		 * @param[in] index 					The index of the preset
		 * @param[in] time 						The time to set
		 */
		void setTime(unsigned int index, const Ogre::String& time);

		/**
		 * @brief Gets the time for the preset in the format hh:mm, e.g. 16:45.
		 * @note								The time is in range [0; 24[.
		 * @param[in] index 					The index of the preset
		 * @return time 						The time for the preset to get
		 */
		Ogre::String getTime(unsigned int index);

		/**
		 * @brief Sets the density coefficient for the preset.
		 * @note								Valid values are in range [0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] densityCoefficient 		The density coefficient to set
		 */
		void setDensityCoefficient(unsigned int index, Ogre::Real densityCoefficient);

		/**
		 * @brief Gets the density coefficient for the preset.
		 * @note								Valid values are in range [0; inf).
		 * @param[in] index 					The index of the preset
		 * @return density coefficient 			The density coefficient for the preset
		 */
		Ogre::Real getDensityCoefficient(unsigned int index);

		/**
		 * @brief Sets the density diffusion for the preset.
		 * @note								Valid values are in range [0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] densityDiffusion 		The density diffusion to set
		 */
		void setDensityDiffusion(unsigned int index, Ogre::Real densityDiffusion);

		/**
		 * @brief Gets the density diffusion for the preset.
		 * @note								Valid values are in range [0; inf).
		 * @param[in] index 					The index of the preset
		 * @return densityDiffusion 			The density diffusion for the preset
		 */
		Ogre::Real getDensityDiffusion(unsigned int index);

		/**
		 * @brief Sets the horizon limit for the preset.
		 * @note								Valid values are in range [0; inf). Most relevant in sunsets and sunrises.
		 * @param[in] index 					The index of the preset
		 * @param[in] horizonLimit 				The horizon limit to set
		 */
		void setHorizonLimit(unsigned int index, Ogre::Real horizonLimit);

		/**
		 * @brief Gets the horizon limit for the preset.
		 * @note								Valid values are in range [0; inf). Most relevant in sunsets and sunrises.
		 * @param[in] index 					The index of the preset
		 * @return horizonLimit 				The horizon limit for the preset
		 */
		Ogre::Real getHorizonLimit(unsigned int index);

		/**
		 * @brief Sets the sun power for the preset.
		 * @note								Valid values are in range [0; inf). For HDR (affects the sun on the sky).
		 * @param[in] index 					The index of the preset
		 * @param[in] sunPower 					The sun power to set
		 */
		void setSunPower(unsigned int index, Ogre::Real sunPower);

		/**
		 * @brief Gets the sun power for the preset.
		 * @note								Valid values are in range [0; inf). For HDR (affects the sun on the sky).
		 * @param[in] index 					The index of the preset
		 * @return sunPower 					The sun power for the preset
		 */
		Ogre::Real getSunPower(unsigned int index);

		/**
		 * @brief Sets the sky power for the preset.
		 * @note								Valid values are in range [0; inf). For HDR (affects skyColour). Sky must be enabled.
		 * @param[in] index 					The index of the preset
		 * @param[in] skyPower 					The sky power to set
		 */
		void setSkyPower(unsigned int index, Ogre::Real skyPower);

		/**
		 * @brief Gets the sky power for the preset.
		 * @note								Valid values are in range [0; inf). For HDR (affects skyColour). Sky must be enabled.
		 * @param[in] index 					The index of the preset
		 * @return skyPower 					The sky power for the preset
		 */
		Ogre::Real getSkyPower(unsigned int index);

		/**
		 * @brief Sets the sky color for the preset.
		 * @note								In range [0; 1], [0; 1], [0; 1]. Sky must be enabled.
		 * @param[in] index 					The index of the preset
		 * @param[in] skyColor					The sky color to set
		 */
		void setSkyColor(unsigned int index, const Ogre::Vector3& skyColor);

		/**
		 * @brief Gets the sky color for the preset.
		 * @note								In range [0; 1], [0; 1], [0; 1]. Sky must be enabled.
		 * @param[in] index 					The index of the preset
		 * @return skyColor 					The sky color for the preset
		 */
		Ogre::Vector3 getSkyColor(unsigned int index);

		/**
		 * @brief Sets the fog density for the preset.
		 * @note								In range [0; 1]. Affects objects' fog (not sky).
		 * @param[in] index 					The index of the preset
		 * @param[in] fogDensity				The fog density to set
		 */
		void setFogDensity(unsigned int index, Ogre::Real fogDensity);

		/**
		 * @brief Gets the fog density for the preset.
		 * @note								In range [0; 1]. Affects objects' fog (not sky).
		 * @param[in] index 					The index of the preset
		 * @return fogDensity 					The fog density for the preset
		 */
		Ogre::Real getFogDensity(unsigned int index);

		/**
		 * @brief Sets the fog break min brightness for the preset.
		 * @note								Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog.
	     *										A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e. in HDR) will start 
		 *										becoming more visible. Range: [0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] fogBreakMinBrightness		The fog break min brightness to set
		 */
		void setFogBreakMinBrightness(unsigned int index, Ogre::Real fogBreakMinBrightness);

		/**
		 * @brief Gets the fog break min brightness for the preset.
		 * @note								Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog.
	     *										A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e. in HDR) will start 
		 *										becoming more visible. Range: [0; inf).
		 * @param[in] index 					The index of the preset
		 * @return fogBreakMinBrightness 		The fog break min brightness for the preset
		 */
		Ogre::Real getFogBreakMinBrightness(unsigned int index);

		/**
		 * @brief Sets the fog break falloff for the preset.
		 * @note								How fast bright objects appear in fog.
		 *										Small values make objects appear very slowly after luminance > fogBreakMinBrightness. 
         *										Large values make objects appear very quickly. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] fogFogBreakFalloff		The fog break falloff to set
		 */
		void setFogBreakFalloff(unsigned int index, Ogre::Real fogBreakFalloff);

		/**
		 * @brief Gets the fog break falloff for the preset.
		 * @note								How fast bright objects appear in fog.
		 *										Small values make objects appear very slowly after luminance > fogBreakMinBrightness
         *										Large values make objects appear very quickly. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @return fogFogBreakFalloff 			The fog break falloff for the preset
		 */
		Ogre::Real getFogBreakFalloff(unsigned int index);

		/**
		 * @brief Sets the linked light power for the preset.
		 * @note								Power scale for the linked light. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] linkedLightPower			The linked light power to set
		 */
		void setLinkedLightPower(unsigned int index, Ogre::Real linkedLightPower);

		/**
		 * @brief Gets the linked light power for the preset.
		 * @note								Power scale for the linked light. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @return linkedLightPower 			The linked light power for the preset
		 */
		Ogre::Real getLinkedLightPower(unsigned int index);

		/**
		 * @brief Sets the linked ambient upper power for the preset.
		 * @note								Power scale for the upper hemisphere ambient light. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] linkedAmbientUpperPower	The linked ambient upper power to set
		 */
		void setLinkedAmbientUpperPower(unsigned int index, Ogre::Real linkedAmbientUpperPower);

		/**
		 * @brief Gets the linked ambient upper power for the preset.
		 * @note								Power scale for the upper hemisphere ambient light. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @return linkedAmbientUpperPower 		The linked ambient upper power for the preset
		 */
		Ogre::Real getLinkedAmbientUpperPower(unsigned int index);

		/**
		 * @brief Sets the linked ambient lower power for the preset.
		 * @note								Power scale for the lower hemisphere ambient light. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @param[in] linkedAmbientLowerPower	The linked ambient upper power to set
		 */
		void setLinkedAmbientLowerPower(unsigned int index, Ogre::Real linkedAmbientLowerPower);

		/**
		 * @brief Gets the linked ambient lower power for the preset.
		 * @note								Power scale for the lower hemisphere ambient light. Range: (0; inf).
		 * @param[in] index 					The index of the preset
		 * @return linkedAmbientLowerPower 		The linked ambient lower power for the preset
		 */
		Ogre::Real getLinkedAmbientLowerPower(unsigned int index);

		/**
		 * @brief Sets the envmap scale for the preset.
		 * @note								Value to send to SceneManager::setAmbientLight. Range: (0; 1].
		 * @param[in] index 					The index of the preset
		 * @param[in] envmapScale				The envmap scale to set
		 */
		void setEnvmapScale(unsigned int index, Ogre::Real envmapScale);

		/**
		 * @brief Gets the envmap scale for the preset.
		 * @note								Value to send to SceneManager::setAmbientLight. Range: (0; 1].
		 * @param[in] index 					The index of the preset
		 * @return envmapScale 					The envmap scale for the preset
		 */
		Ogre::Real getEnvmapScale(unsigned int index);

	public:
		/** 
		 * @brief Sets the time of day.
		 * @remarks	Assumes Y is up
		 * @param[in] sunAltitude			Altitude of the sun. At 90° the sun is facing downards (i.e. 12pm) *Must* be in range [0; pi)
		 * @param[in] azimuth					Rotation around Y axis. In radians.
		 */
		void setSunDir(const Ogre::Radian& sunAltitude, const Ogre::Radian& azimuth = Radian(0.0f));

		/** 
		 * @brief More direct approach on setting time of day.
		 * @param[in] sunDir						Sun's light direction (or moon). Will be normalized.
		 * @param[in] normalizedTimeOfDay		In range [0; 1] where 0 is when the sun goes out and 1 when it's gone.
		 */
		void setSunDir(const Ogre::Vector3& sunDir, const Ogre::Real normalizedTimeOfDay);

		/**
		 * @brief	Gets the atmosphere color at the given direction
		 * @remarks					Assumes camera displacement is 0.
		 * @param[in] cameraDir		Point to look at
		 * @param[in] bSkipSun		When true, the sun disk is skipped
		 * @return	  The color value at that camera direction, given the current parameters
		*/
		Ogre::Vector3 getAtmosphereAt(const Ogre::Vector3& cameraDir, bool bSkipSun = false);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrEnableSky(void) { return "Enable Sky"; }
		static const Ogre::String AttrStartTime(void) { return "Start Time"; }
		static const Ogre::String AttrTimeMultiplicator(void) { return "Time Multiplicator"; }
		static const Ogre::String AttrPresetsCount(void) { return "Presets Count"; }
		static const Ogre::String AttrTime(void) { return "Time "; }
		static const Ogre::String AttrDensityCoefficient(void) { return "Density Coefficient "; }
		static const Ogre::String AttrDensityDiffusion(void) { return "Density Diffusion "; }
		static const Ogre::String AttrHorizonLimit(void) { return "Horizon Limit "; }
		static const Ogre::String AttrSunPower(void) { return "Sun Power "; }
		static const Ogre::String AttrSkyPower(void) { return "Sky Power "; }
		static const Ogre::String AttrSkyColor(void) { return "Sky Color "; }
		static const Ogre::String AttrFogDensity(void) { return "Fog Density "; }
		static const Ogre::String AttrFogBreakMinBrightness(void) { return "Fog Break Min Brightness "; }
		static const Ogre::String AttrFogBreakFalloff(void) { return "Fog Break Falloff "; }
		static const Ogre::String AttrLinkedLightPower(void) { return "Linked Light Power "; }
		static const Ogre::String AttrLinkedAmbientUpperPower(void) { return "Linked Ambient Upper Power "; }
		static const Ogre::String AttrLinkedAmbientLowerPower(void) { return "Linked Ambient Lower Power "; }
		static const Ogre::String AttrEnvmapScale(void) { return "Envmap Scale "; }
	private:

		void generateDefaultData(unsigned short i);

		Ogre::Real convertTime(const Ogre::String& strTimeOfDay);
	private:
		Ogre::String name;

		Variant* activated;
		Variant* enableSky;
		Variant* startTime;
		Variant* timeMultiplicator;
		Variant* presetCount;
		std::vector<Variant*> times;
		std::vector<Variant*> densityCoefficients;
		std::vector<Variant*> densityDiffusions;
		std::vector<Variant*> horizonLimits;
		std::vector<Variant*> sunPowers;
		std::vector<Variant*> skyPowers;
		std::vector<Variant*> skyColors;
		std::vector<Variant*> fogDensities;
		std::vector<Variant*> fogBreakMinBrightnesses;
		std::vector<Variant*> fogBreakFalloffs;
		std::vector<Variant*> linkedLightPowers;
		std::vector<Variant*> linkedSceneAmbientUpperPowers;
		std::vector<Variant*> linkedSceneAmbientLowerPowers;
		std::vector<Variant*> envmapScales;

		LightDirectionalComponent* lightDirectionalComponent;
		Ogre::Real timeOfDay;
		Ogre::Real azimuth;
		Ogre::AtmosphereNpr* atmosphereNpr;
		int dayTimeCurrentMinutes;
		bool hasLoaded;
		Ogre::Vector3 oldLightDirection;
	};

}; // namespace end

#endif

