#include "NOWAPrecompiled.h"
#include "AtmosphereComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "utilities/Interpolator.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/LightDirectionalComponent.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AtmosphereComponent::AtmosphereComponent()
		: GameObjectComponent(),
		name("AtmosphereComponent"),
		activated(new Variant(AtmosphereComponent::AttrActivated(), true, this->attributes)),
		enableSky(new Variant(AtmosphereComponent::AttrEnableSky(), true, this->attributes)),
		startTime(new Variant(AtmosphereComponent::AttrStartTime(), Ogre::String("12:00"), this->attributes)),
		timeMultiplicator(new Variant(AtmosphereComponent::AttrTimeMultiplicator(), 0.01f, this->attributes)),
		lightDirectionalComponent(nullptr),
		atmosphereNpr(nullptr),
		timeOfDay(Ogre::Math::PI),
		azimuth(0.0f),
		hasLoaded(false),
		dayTimeCurrentMinutes(0),
		oldLightDirection(Ogre::Vector3(-1.0f, -1.0f, -1.0f))
	{
		this->activated->setDescription("If activated, the atmospheric effects will take place.");
		this->enableSky->setDescription("Sets whether sky is enabled and visible.");
		this->startTime->setDescription("Sets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight.");
		this->timeMultiplicator->setDescription("Sets the time multiplier. How long a day lasts. Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].");

		this->presetCount = new Variant(AtmosphereComponent::AttrPresetsCount(), 6, this->attributes);
		this->presetCount->setDescription("Sets the count of the atmosphere presets.");

		this->times.resize(this->presetCount->getUInt());
		this->densityCoefficients.resize(this->presetCount->getUInt());
		this->densityDiffusions.resize(this->presetCount->getUInt());
		this->horizonLimits.resize(this->presetCount->getUInt());
		this->sunPowers.resize(this->presetCount->getUInt());
		this->skyPowers.resize(this->presetCount->getUInt());
		this->skyColors.resize(this->presetCount->getUInt());
		this->fogDensities.resize(this->presetCount->getUInt());
		this->fogBreakMinBrightnesses.resize(this->presetCount->getUInt());
		this->fogBreakFalloffs.resize(this->presetCount->getUInt());
		this->linkedLightPowers.resize(this->presetCount->getUInt());
		this->linkedSceneAmbientUpperPowers.resize(this->presetCount->getUInt());
		this->linkedSceneAmbientLowerPowers.resize(this->presetCount->getUInt());
		this->envmapScales.resize(this->presetCount->getUInt());

		// Since when node game object count is changed, the whole properties must be refreshed, so that new field may come for node tracks
		this->presetCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->presetCount->addUserData(GameObject::AttrActionSeparator());
	}

	AtmosphereComponent::~AtmosphereComponent(void)
	{
		this->lightDirectionalComponent = nullptr;
		if (nullptr != this->atmosphereNpr)
		{
			this->atmosphereNpr->setLight(nullptr);
			delete this->atmosphereNpr;
			this->atmosphereNpr = nullptr;
		}
	}

	void AtmosphereComponent::generateDefaultData(unsigned short i)
	{
		switch (i)
		{
			case 0:
			{
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "06:30", this->attributes);
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.37f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.035f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.01f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.01f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), Math::PI * 10.0f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
			case 1:
			{
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "12:00", this->attributes); // Midday
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.58f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.025f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), Math::PI * 20.0f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(1.0f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (1; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
			case 2:
			{
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "20:00", this->attributes);
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.58f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.025f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), Math::PI * 10.0f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (1; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
			case 3:
			{
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "22:00", this->attributes);
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.08f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.025f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 0.01f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 0.2f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), Ogre::Math::PI * 7.5f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI * 0.001f, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI * 0.001f, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (1; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
			case 4:
			{
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "00:00", this->attributes); // Midnight
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.021f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.025f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 0.005f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 0.2f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI * 0.001f, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI * 0.001f, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
			case 5:
			{
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "05:00", this->attributes);
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.08f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.025f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 0.01f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 0.2f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), 5.0f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI * 0.001f, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI * 0.001f, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
			default:
			{
				// First again for all others
				this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), "06:30", this->attributes);
				this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.47f, this->attributes);
				this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
				this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
				this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
				this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.025f, this->attributes);
				this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
				this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->sunPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->skyPowers[i]->setConstraints(0.0f, 100.0f);
				this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
				this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0001f, this->attributes);
				this->fogDensities[i]->setConstraints(0.0f, 1.0f);
				this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.25f, this->attributes);
				this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
				this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.1f, this->attributes);
				this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
				this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), Math::PI * 10.0f, this->attributes);
				this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
				this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.1f * Math::PI, this->attributes);
				this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
				this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.01f * Math::PI, this->attributes);
				this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->envmapScales[i]->setConstraints(0.01f, 1.0f);
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());

				this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
				this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
				this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
				this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
				this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
				this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
				this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
				this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
				this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
					"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
					"becoming more visible.Range: [0; 100).");
				this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
					"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
				this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
				this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
				this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
				this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");

				break;
			}
		}
	}

	void AtmosphereComponent::initialise()
	{

	}

	const Ogre::String& AtmosphereComponent::getName() const
	{
		return this->name;
	}

	void AtmosphereComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AtmosphereComponent>(AtmosphereComponent::getStaticClassId(), AtmosphereComponent::getStaticClassName());
	}

	void AtmosphereComponent::shutdown()
	{

	}

	void AtmosphereComponent::uninstall()
	{

	}

	void AtmosphereComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool AtmosphereComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableSky")
		{
			this->enableSky->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartTime")
		{
			this->setStartTime(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimeMultiplicator")
		{
			this->timeMultiplicator->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PresetsCount")
		{
			this->presetCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->times.size() > this->presetCount->getUInt())
		{
			this->times.resize(this->presetCount->getUInt());
			this->densityCoefficients.resize(this->presetCount->getUInt());
			this->densityDiffusions.resize(this->presetCount->getUInt());
			this->horizonLimits.resize(this->presetCount->getUInt());
			this->sunPowers.resize(this->presetCount->getUInt());
			this->skyPowers.resize(this->presetCount->getUInt());
			this->skyColors.resize(this->presetCount->getUInt());
			this->fogDensities.resize(this->presetCount->getUInt());
			this->fogBreakMinBrightnesses.resize(this->presetCount->getUInt());
			this->fogBreakFalloffs.resize(this->presetCount->getUInt());
			this->linkedLightPowers.resize(this->presetCount->getUInt());
			this->linkedSceneAmbientUpperPowers.resize(this->presetCount->getUInt());
			this->linkedSceneAmbientLowerPowers.resize(this->presetCount->getUInt());
			this->envmapScales.resize(this->presetCount->getUInt());
		}
		else
		{
			this->eraseVariants(this->times, this->presetCount->getUInt());
			this->eraseVariants(this->densityCoefficients, this->presetCount->getUInt());
			this->eraseVariants(this->densityDiffusions, this->presetCount->getUInt());
			this->eraseVariants(this->horizonLimits, this->presetCount->getUInt());
			this->eraseVariants(this->sunPowers, this->presetCount->getUInt());
			this->eraseVariants(this->skyPowers, this->presetCount->getUInt());
			this->eraseVariants(this->skyColors, this->presetCount->getUInt());
			this->eraseVariants(this->fogDensities, this->presetCount->getUInt());
			this->eraseVariants(this->fogBreakMinBrightnesses, this->presetCount->getUInt());
			this->eraseVariants(this->fogBreakFalloffs, this->presetCount->getUInt());
			this->eraseVariants(this->linkedLightPowers, this->presetCount->getUInt());
			this->eraseVariants(this->linkedSceneAmbientUpperPowers, this->presetCount->getUInt());
			this->eraseVariants(this->linkedSceneAmbientUpperPowers, this->presetCount->getUInt());
			this->eraseVariants(this->linkedSceneAmbientLowerPowers, this->presetCount->getUInt());
			this->eraseVariants(this->envmapScales, this->presetCount->getUInt());
		}

		for (size_t i = 0; i < this->times.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Time" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->times[i])
				{
					this->times[i] = new Variant(AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->setTime(static_cast<unsigned int>(i), XMLConverter::getAttrib(propertyElement, "data"));
				}
				else
				{
					this->times[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DensityCoefficient" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->densityCoefficients[i])
				{
					this->densityCoefficients[i] = new Variant(AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->densityCoefficients[i]->setConstraints(0.0f, 1.0f);
					this->setDensityCoefficient(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->densityCoefficients[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DensityDiffusion" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->densityDiffusions[i])
				{
					this->densityDiffusions[i] = new Variant(AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->densityDiffusions[i]->setConstraints(0.0f, 1.0f);
					this->setDensityDiffusion(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->densityDiffusions[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HorizonLimit" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->horizonLimits[i])
				{
					this->horizonLimits[i] = new Variant(AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->horizonLimits[i]->setConstraints(0.0f, 100.0f);
					this->setHorizonLimit(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->horizonLimits[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SunPower" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->sunPowers[i])
				{
					this->sunPowers[i] = new Variant(AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->sunPowers[i]->setConstraints(0.0f, 100.0f);
					this->setSunPower(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->sunPowers[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkyPower" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->skyPowers[i])
				{
					this->skyPowers[i] = new Variant(AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->skyPowers[i]->setConstraints(0.0f, 100.0f);
					this->setSkyPower(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->skyPowers[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkyColor" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->skyColors[i])
				{
					this->skyColors[i] = new Variant(AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.334f, 0.57f, 1.0f), this->attributes);
					this->skyColors[i]->addUserData(GameObject::AttrActionColorDialog());
					this->setSkyColor(static_cast<unsigned int>(i), XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				else
				{
					this->skyColors[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FogDensity" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->fogDensities[i])
				{
					this->fogDensities[i] = new Variant(AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->fogDensities[i]->setConstraints(0.0f, 1.0f);
					this->setFogDensity(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->fogDensities[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FogBreakMinBrightness" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->fogBreakMinBrightnesses[i])
				{
					this->fogBreakMinBrightnesses[i] = new Variant(AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->fogBreakMinBrightnesses[i]->setConstraints(0.001f, 1.0f);
					this->setFogBreakMinBrightness(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->fogBreakMinBrightnesses[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FogBreakFalloff" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->fogBreakFalloffs[i])
				{
					this->fogBreakFalloffs[i] = new Variant(AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->fogBreakFalloffs[i]->setConstraints(0.001f, 1.0f);
					this->setFogBreakFalloff(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->fogBreakFalloffs[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinkedLightPower" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->linkedLightPowers[i])
				{
					this->linkedLightPowers[i] = new Variant(AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->linkedLightPowers[i]->setConstraints(0.01f, 100.0f);
					this->setLinkedLightPower(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->linkedLightPowers[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinkedAmbientUpperPower" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->linkedSceneAmbientUpperPowers[i])
				{
					this->linkedSceneAmbientUpperPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->linkedSceneAmbientUpperPowers[i]->setConstraints(0.01f, 1.0f);
					this->setLinkedAmbientUpperPower(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->linkedSceneAmbientUpperPowers[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinkedAmbientLowerPower" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->linkedSceneAmbientLowerPowers[i])
				{
					this->linkedSceneAmbientLowerPowers[i] = new Variant(AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->linkedSceneAmbientLowerPowers[i]->setConstraints(0.01f, 1.0f);
					this->setLinkedAmbientLowerPower(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->linkedSceneAmbientLowerPowers[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnvmapScale" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->envmapScales[i])
				{
					this->envmapScales[i] = new Variant(AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
					this->envmapScales[i]->setConstraints(0.01f, 1.0f);
					this->setEnvmapScale(static_cast<unsigned int>(i), XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->envmapScales[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				// Adds separator for the next round
				this->envmapScales[i]->addUserData(GameObject::AttrActionSeparator());
			}

			this->times[i]->setDescription(" Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
			this->densityCoefficients[i]->setDescription("Sets the density coefficient for the preset. Valid values are in range [0; 100).");
			this->densityDiffusions[i]->setDescription("Sets the density diffusion for the preset. Valid values are in range [0; 100).");
			this->horizonLimits[i]->setDescription("Sets the horizon limit for the preset. Valid values are in range [0; 100). Most relevant in sunsets and sunrises.");
			this->sunPowers[i]->setDescription("Sets the sun power for the preset. Valid values are in range [0; 100). For HDR (affects the sun on the sky).");
			this->skyPowers[i]->setDescription("Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
			this->skyColors[i]->setDescription("Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 100). Sky must be enabled.");
			this->fogDensities[i]->setDescription("Sets the fog density for the preset. In range [0; 100). Affects objects' fog (not sky).");
			this->fogBreakMinBrightnesses[i]->setDescription("Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
				"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
				"becoming more visible.Range: [0; 100).");
			this->fogBreakFalloffs[i]->setDescription("Sets the fog break falloff for the preset. How fast bright objects appear in fog."
				"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
			this->linkedLightPowers[i]->setDescription("Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
			this->linkedSceneAmbientUpperPowers[i]->setDescription("Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
			this->linkedSceneAmbientLowerPowers[i]->setDescription("Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
			this->envmapScales[i]->setDescription("Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");
		}

		this->hasLoaded = true;

		return true;
	}

	GameObjectCompPtr AtmosphereComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// No cloning
		return AtmosphereComponentPtr();
	}

	bool AtmosphereComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AtmosphereComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// If scene has not been loaded generate default presets
		if (false == this->hasLoaded)
		{
			for (size_t i = 0; i < this->times.size(); i++)
			{
				this->generateDefaultData(static_cast<unsigned short>(i));
			}
			this->hasLoaded = false;
		}

		// Get the sun light (directional light for sun power setting)
		GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_LIGHT_ID);

		if (nullptr == lightGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AtmosphereComponent] Could not find 'SunLight' for this component! Affected game object: " + this->gameObjectPtr->getName());
			return false;
		}

		auto& lightDirectionalCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
		if (nullptr != lightDirectionalCompPtr)
		{
			this->lightDirectionalComponent = lightDirectionalCompPtr.get();
		}
		else
		{
			return false;
		}

		if (nullptr != this->atmosphereNpr)
		{
			this->atmosphereNpr->setLight(nullptr);
			delete this->atmosphereNpr;
			this->atmosphereNpr = nullptr;
		}

		this->atmosphereNpr = new Ogre::AtmosphereNpr(this->gameObjectPtr->getSceneManager()->getDestinationRenderSystem()->getVaoManager());

		this->oldLightDirection = this->lightDirectionalComponent->getDirection();
		// Todo: If terra is set, update for terra, see sample
		this->atmosphereNpr->setLight(this->lightDirectionalComponent->getOgreLight());

		{
			// Preserve the Power Scale explicitly set by the sample
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.linkedLightPower = this->lightDirectionalComponent->getOgreLight()->getPowerScale();
			this->atmosphereNpr->setPreset(preset);
		}

		this->atmosphereNpr->setSky(this->gameObjectPtr->getSceneManager(), this->enableSky->getBool());

		return true;
	}

	bool AtmosphereComponent::connect(void)
	{
		this->setStartTime(this->startTime->getString());

		Ogre::AtmosphereNpr::PresetArray presets;

		for (size_t i = 0; i < this->times.size(); i++)
		{
			presets.push_back(Ogre::AtmosphereNpr::Preset());
			presets.back().time = this->convertTime(this->times[i]->getString());
			presets.back().densityCoeff = this->densityCoefficients[i]->getReal();
			presets.back().densityDiffusion = this->densityDiffusions[i]->getReal();
			presets.back().horizonLimit = this->horizonLimits[i]->getReal();
			presets.back().sunPower = this->sunPowers[i]->getReal();
			if (true == this->enableSky->getBool())
			{
				presets.back().skyPower = this->skyPowers[i]->getReal();
				presets.back().skyColour = this->skyColors[i]->getVector3();
			}
			presets.back().fogDensity = this->fogDensities[i]->getReal();
			presets.back().fogBreakMinBrightness = this->fogBreakMinBrightnesses[i]->getReal();
			presets.back().fogBreakFalloff = this->fogBreakFalloffs[i]->getReal();
			presets.back().linkedLightPower = this->linkedLightPowers[i]->getReal();
			presets.back().linkedSceneAmbientUpperPower = this->linkedSceneAmbientUpperPowers[i]->getReal();
			presets.back().linkedSceneAmbientLowerPower = this->linkedSceneAmbientLowerPowers[i]->getReal();
			presets.back().envmapScale = this->envmapScales[i]->getReal();
		}

		this->atmosphereNpr->setPresets(presets);

		// this->lightDirectionalComponent->setPowerScale(presets.front().sunPower);

		return true;
	}

	bool AtmosphereComponent::disconnect(void)
	{
		this->setStartTime(this->startTime->getString());

		// TODO: Reset to start time and convert timeOfDay variable
		if (nullptr != this->atmosphereNpr)
		{
			this->update(0.016, false);
		}

		this->lightDirectionalComponent->getOgreLight()->setDirection(this->oldLightDirection);

		return true;
	}

	void AtmosphereComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			Ogre::String time = getCurrentTimeOfDay();

			Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
			// Reupdate ambient light for better render results and getting rid of strange flimmer. See: https://forums.ogre3d.org/viewtopic.php?t=96576&start=25
			sceneManager->setAmbientLight(sceneManager->getAmbientLightUpperHemisphere() /** 1.5f*/,
				sceneManager->getAmbientLightLowerHemisphere(),
				sceneManager->getAmbientLightHemisphereDir());

			//sceneManager->setAmbientLight(sceneManager->getAmbientLightUpperHemisphere() /** 1.5f*/,
			//							  	sceneManager->getAmbientLightLowerHemisphere(),
			//							   -this->lightDirectionalComponent->getOgreLight()->getDerivedDirectionUpdated() + Ogre::Vector3::UNIT_Y * 0.2f);
			

#if 0
			this->timeOfDay += this->timeMultiplicator->getReal() * dt;
			if (this->timeOfDay >= Ogre::Math::PI)
			{
				this->timeOfDay = -Ogre::Math::PI + std::fmod(this->timeOfDay, Ogre::Math::PI);
			}

			this->azimuth += this->timeMultiplicator->getReal() * dt; // azimuth multiplier?
			this->azimuth = fmodf(this->azimuth, Ogre::Math::TWO_PI);

			this->atmosphereNpr->updatePreset(sunDir, this->timeOfDay / Ogre::Math::PI);
#endif
			// Issue: Since ranges are not equal, short ranges will durate longer as longer ranges

			// Day has 4 zones aka 6 hours. See @setStartTime table
			Ogre::Real stabilisationFactor = 0.0f;
			if (this->timeOfDay >= 0.0f && this->timeOfDay <= 0.5f)
			{
				// 6 hours
				stabilisationFactor = 6.0f / 6.0f;
			}
			else if (this->timeOfDay > 0.5f && this->timeOfDay <= 1.0f)
			{
				// 9 hours
				stabilisationFactor = 6.0f / 9.0f;
			}
			else if (this->timeOfDay >= -1.0f && this->timeOfDay < -0.5f)
			{
				// 3 hours
				stabilisationFactor = 6.0f / 3.0f;
			}
			else if (this->timeOfDay >= -0.5f && this->timeOfDay < 0.0f)
			{
				// 6 hours
				stabilisationFactor = 6.0f / 6.0f;
			}

			this->timeOfDay += this->timeMultiplicator->getReal() * stabilisationFactor * dt;
			if (this->timeOfDay >= 1.0f)
			{
				this->timeOfDay = -1.0f + std::fmod(this->timeOfDay, 1.0f);
			}

			this->azimuth += this->timeMultiplicator->getReal() * dt; // azimuth multiplier?
			this->azimuth = fmodf(this->azimuth, Ogre::Math::TWO_PI);

			const Ogre::Vector3 sunDir(Ogre::Quaternion(Ogre::Radian(this->azimuth), Ogre::Vector3::UNIT_Y) * Ogre::Vector3(cosf(fabsf(this->timeOfDay)), -sinf(fabsf(this->timeOfDay)), 0.0).normalisedCopy());

			// this->lightDirectionalComponent->getOgreLight()->setDirection(sunDir);
			Ogre::Quaternion newOrientation = MathHelper::getInstance()->faceDirectionSlerp(this->lightDirectionalComponent->getOwner()->getSceneNode()->getOrientation(), sunDir, lightDirectionalComponent->getOwner()->getDefaultDirection(), dt, 60.0f);
			this->lightDirectionalComponent->getOwner()->getSceneNode()->_setDerivedOrientation(newOrientation);
			// Not used, because multpile presets and updatePreset is used
			// this->atmosphereNpr->setSunDir(this->lightDirectionalComponent->getOgreLight()->getDerivedDirectionUpdated(), this->timeOfDay / Ogre::Math::PI);
			this->atmosphereNpr->updatePreset(sunDir, this->timeOfDay/* / Ogre::Math::PI*/);

			if (true == this->bShowDebugData)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AtmosphereComponent] Time: " + time + " timeOfDayNormalized: " + Ogre::StringConverter::toString(this->timeOfDay));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AtmosphereComponent] OgreLight PowerScale: " + Ogre::StringConverter::toString(this->lightDirectionalComponent->getOgreLight()->getPowerScale()));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AtmosphereComponent] Preset SunPower: " + Ogre::StringConverter::toString(this->atmosphereNpr->getPreset().sunPower));
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AtmosphereComponent] Preset LinkedLightPower: " + Ogre::StringConverter::toString(this->atmosphereNpr->getPreset().linkedLightPower));
			}
		}
	}

	void AtmosphereComponent::setStartTime(const Ogre::String& startTime)
	{
		// Time mapping:
		//       0   0.5   1/-1   -0.5    0    0.5   1/-1   -0.5    0
		// Clock 6   12    21     24/0    6    12    21     24/0    6
		// Linear interpolation will work with those ranges from the table above in both directions
		this->startTime->setValue(startTime);

		this->timeOfDay = this->convertTime(startTime);
	}

	Ogre::String AtmosphereComponent::getStartTime(void) const
	{
		return this->startTime->getString();
	}

	Ogre::Real AtmosphereComponent::convertTime(const Ogre::String& strTimeOfDay)
	{
		// Time mapping:
		//       0   0.5   1/-1   -0.5    0    0.5   1/-1   -0.5    0
		// Clock 6   12    21     24/0    6    12    21     24/0    6
		// Linear interpolation will work with those ranges from the table above in both directions
		Ogre::Real normalizedTime = 0.0f;
		size_t hoursFound = strTimeOfDay.find(":");
		if (Ogre::String::npos != hoursFound)
		{
			int hours = Ogre::StringConverter::parseInt(strTimeOfDay.substr(0, hoursFound));
			if (hours >= 24)
			{
				hours = 0;
			}
			int minutes = Ogre::StringConverter::parseInt(strTimeOfDay.substr(hoursFound + 1, strTimeOfDay.length() - 1));
			if (minutes >= 60)
			{
				minutes = 0;
			}
			Ogre::Real factor = (hours + (minutes / 60.0f));
			if (factor >= 6.0f && factor <= 12.0f)
			{
				normalizedTime = Interpolator::getInstance()->linearInterpolation(factor, 6.0f, 12.0f, 0.0f, 0.5f);
			}
			else if (factor > 12.0f && factor <= 21.0f)
			{
				normalizedTime = Interpolator::getInstance()->linearInterpolation(factor, 12.0f, 21.0f, 0.5f, 1.0f);
			}
			else if (factor > 21.0f && factor < 24.0f)
			{
				normalizedTime = Interpolator::getInstance()->linearInterpolation(factor, 21.0f, 24.0f, -1.0f, -0.5f);
			}
			else if (factor >= 0.0f && factor < 6.0f)
			{
				normalizedTime = Interpolator::getInstance()->linearInterpolation(factor, 0.0f, 6.0f, -0.5f, -0.0f);
			}
		}
		return normalizedTime;
	}

	Ogre::String AtmosphereComponent::getCurrentTimeOfDay(void)
	{
		Ogre::String time = "";

		Ogre::Real value = 0.0f;

		if (this->timeOfDay >= 0.0f && this->timeOfDay <= 0.5f)
		{
			value = Interpolator::getInstance()->linearInterpolation(this->timeOfDay, 0.0f, 0.5f, 6.0f, 12.0f);
		}
		else if (this->timeOfDay > 0.5f && this->timeOfDay <= 1.0f)
		{
			value = Interpolator::getInstance()->linearInterpolation(this->timeOfDay, 0.5f, 1.0f, 12.0f, 21.0f);
		}
		else if (this->timeOfDay >= -1.0f && this->timeOfDay < -0.5f)
		{
			value = Interpolator::getInstance()->linearInterpolation(this->timeOfDay, -1.0f, -0.5f, 21.0f, 24.0f);
		}
		else if (this->timeOfDay >= -0.5f && this->timeOfDay < 0.0f)
		{
			value = Interpolator::getInstance()->linearInterpolation(this->timeOfDay, -0.5f, -0.0f, 0.0f, 6.0f);
		}

		int hours = value;

		time += (hours < 10 ? "0" + Ogre::StringConverter::toString(hours) : Ogre::StringConverter::toString(hours)) + ":";
		int minutes = static_cast<int>(value * 60) % 60;
		time += minutes < 10 ? "0" + Ogre::StringConverter::toString(minutes) : Ogre::StringConverter::toString(minutes);

		this->dayTimeCurrentMinutes = (hours * 60) + minutes;
		return time;
	}

	int AtmosphereComponent::getCurrentTimeOfDayMinutes(void)
	{
		return this->dayTimeCurrentMinutes;
	}

	void AtmosphereComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (AtmosphereComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AtmosphereComponent::AttrEnableSky() == attribute->getName())
		{
			this->setEnableSky(attribute->getBool());
		}
		else if (AtmosphereComponent::AttrStartTime() == attribute->getName())
		{
			this->setStartTime(attribute->getString());
		}
		else if (AtmosphereComponent::AttrTimeMultiplicator() == attribute->getName())
		{
			this->setTimeMultiplicator(attribute->getReal());
		}
		else if (AtmosphereComponent::AttrPresetsCount() == attribute->getName())
		{
			this->setPresetCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->times.size()); i++)
			{
				if (AtmosphereComponent::AttrTime() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setTime(i, attribute->getString());
				}
				else if (AtmosphereComponent::AttrDensityCoefficient() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setDensityCoefficient(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrDensityDiffusion() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setDensityDiffusion(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrHorizonLimit() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setHorizonLimit(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrSunPower() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setSunPower(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrSkyPower() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setSkyPower(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrSkyColor() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setSkyColor(i, attribute->getVector3());
				}
				else if (AtmosphereComponent::AttrFogDensity() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setFogDensity(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrFogBreakMinBrightness() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setFogBreakMinBrightness(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrFogBreakFalloff() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setFogBreakFalloff(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrLinkedLightPower() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setLinkedLightPower(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrLinkedAmbientUpperPower() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setLinkedAmbientUpperPower(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrLinkedAmbientLowerPower() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setLinkedAmbientLowerPower(i, attribute->getReal());
				}
				else if (AtmosphereComponent::AttrEnvmapScale() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setEnvmapScale(i, attribute->getReal());
				}
			}
		}
	}

	void AtmosphereComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableSky"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableSky->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startTime->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TimeMultiplicator"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timeMultiplicator->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PresetsCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->presetCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->times.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Time" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->times[i]->getString())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "DensityCoefficient" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->densityCoefficients[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "DensityDiffusion" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->densityDiffusions[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "HorizonLimit" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->horizonLimits[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SunPower" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sunPowers[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SkyPower" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skyPowers[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SkyColor" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skyColors[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "FogDensity" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fogDensities[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "FogBreakMinBrightness" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fogBreakMinBrightnesses[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "FogBreakFalloff" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fogBreakFalloffs[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "LinkedLightPower" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linkedLightPowers[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "LinkedAmbientUpperPower" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linkedSceneAmbientUpperPowers[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "LinkedAmbientLowerPower" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linkedSceneAmbientLowerPowers[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "EnvmapScale" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->envmapScales[i]->getReal())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String AtmosphereComponent::getClassName(void) const
	{
		return "AtmosphereComponent";
	}

	Ogre::String AtmosphereComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool AtmosphereComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto atmosphereCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<AtmosphereComponent>());
		// Constraints: Can only be placed under a main light object and only once
		if (gameObject->getId() == GameObjectController::MAIN_LIGHT_ID && nullptr == atmosphereCompPtr)
		{
			return true;
		}
		return false;
	}

	void AtmosphereComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool AtmosphereComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AtmosphereComponent::setEnableSky(bool enableSky)
	{
		this->enableSky->setValue(enableSky);
		this->atmosphereNpr->setSky(this->gameObjectPtr->getSceneManager(), enableSky);
	}

	bool AtmosphereComponent::getEnableSky(void) const
	{
		return this->enableSky->getBool();
	}

	void AtmosphereComponent::setTimeMultiplicator(Ogre::Real timeMultiplicator)
	{
		this->timeMultiplicator->setValue(timeMultiplicator);
	}

	Ogre::Real AtmosphereComponent::getTimeMultiplicator(void) const
	{
		return this->timeMultiplicator->getReal();
	}

	void AtmosphereComponent::setTimeOfDay(Ogre::Real timeOfDay)
	{
		this->timeOfDay = timeOfDay;
	}

	Ogre::Real AtmosphereComponent::getTimeOfDay(void) const
	{
		return this->timeOfDay;
	}

	void AtmosphereComponent::setAzimuth(Ogre::Real azimuth)
	{
		this->azimuth = azimuth;
	}

	Ogre::Real AtmosphereComponent::getAzimuth(void) const
	{
		return this->azimuth;
	}

	void AtmosphereComponent::setPresetCount(unsigned int presetCount)
	{
		if (presetCount > 12)
		{
			presetCount = 12;
		}

		this->presetCount->setValue(presetCount);

		size_t oldSize = this->times.size();

		if (presetCount > oldSize)
		{
			// Resize the array for count
			this->times.resize(presetCount);
			this->densityCoefficients.resize(presetCount);
			this->densityDiffusions.resize(presetCount);
			this->horizonLimits.resize(presetCount);
			this->sunPowers.resize(presetCount);
			this->skyPowers.resize(presetCount);
			this->skyColors.resize(presetCount);
			this->fogDensities.resize(presetCount);
			this->fogBreakMinBrightnesses.resize(presetCount);
			this->fogBreakFalloffs.resize(presetCount);
			this->linkedLightPowers.resize(presetCount);
			this->linkedSceneAmbientUpperPowers.resize(presetCount);
			this->linkedSceneAmbientLowerPowers.resize(presetCount);
			this->envmapScales.resize(presetCount);

			for (size_t i = oldSize; i < this->times.size(); i++)
			{
				this->generateDefaultData(static_cast<unsigned short>(i));
			}
		}
		else if (presetCount < oldSize)
		{
			this->eraseVariants(this->times, presetCount);
			this->eraseVariants(this->densityCoefficients, presetCount);
			this->eraseVariants(this->densityDiffusions, presetCount);
			this->eraseVariants(this->horizonLimits, presetCount);
			this->eraseVariants(this->sunPowers, presetCount);
			this->eraseVariants(this->skyPowers, presetCount);
			this->eraseVariants(this->skyColors, presetCount);
			this->eraseVariants(this->fogDensities, presetCount);
			this->eraseVariants(this->fogBreakMinBrightnesses, presetCount);
			this->eraseVariants(this->fogBreakFalloffs, presetCount);
			this->eraseVariants(this->linkedLightPowers, presetCount);
			this->eraseVariants(this->linkedSceneAmbientUpperPowers, presetCount);
			this->eraseVariants(this->linkedSceneAmbientLowerPowers, presetCount);
			this->eraseVariants(this->envmapScales, presetCount);
		}
	}

	unsigned int AtmosphereComponent::getPresetCount(void) const
	{
		return this->presetCount->getUInt();
	}

	void AtmosphereComponent::setTime(unsigned int index, const Ogre::String& time)
	{
		if (index > this->times.size())
			index = static_cast<unsigned int>(this->times.size()) - 1;

		Ogre::Real normalizedTime = this->convertTime(time);

		this->times[index]->setValue(time);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.time = normalizedTime;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::String AtmosphereComponent::getTime(unsigned int index)
	{
		return this->times[index]->getString();
	}

	void AtmosphereComponent::setDensityCoefficient(unsigned int index, Ogre::Real densityCoefficient)
	{
		if (index > this->densityCoefficients.size())
			index = static_cast<unsigned int>(this->densityCoefficients.size()) - 1;

		if (densityCoefficient < 0.0f)
		{
			densityCoefficient = 0.0f;
		}

		this->densityCoefficients[index]->setValue(densityCoefficient);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.densityCoeff = densityCoefficient;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getDensityCoefficient(unsigned int index)
	{
		return this->densityCoefficients[index]->getReal();
	}

	void AtmosphereComponent::setDensityDiffusion(unsigned int index, Ogre::Real densityDiffusion)
	{
		if (index > this->densityDiffusions.size())
			index = static_cast<unsigned int>(this->densityDiffusions.size()) - 1;

		if (densityDiffusion < 0.0f)
		{
			densityDiffusion = 0.0f;
		}

		this->densityDiffusions[index]->setValue(densityDiffusion);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.densityDiffusion = densityDiffusion;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getDensityDiffusion(unsigned int index)
	{
		return this->densityDiffusions[index]->getReal();
	}

	void AtmosphereComponent::setHorizonLimit(unsigned int index, Ogre::Real horizonLimit)
	{
		if (index > this->horizonLimits.size())
			index = static_cast<unsigned int>(this->horizonLimits.size()) - 1;

		if (horizonLimit < 0.0f)
		{
			horizonLimit = 0.0f;
		}

		this->horizonLimits[index]->setValue(horizonLimit);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.horizonLimit = horizonLimit;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getHorizonLimit(unsigned int index)
	{
		return this->horizonLimits[index]->getReal();
	}

	void AtmosphereComponent::setSunPower(unsigned int index, Ogre::Real sunPower)
	{
		if (index > this->sunPowers.size())
			index = static_cast<unsigned int>(this->sunPowers.size()) - 1;

		if (sunPower < 0.0f)
		{
			sunPower = 0.0f;
		}

		this->sunPowers[index]->setValue(sunPower);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.sunPower = sunPower;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getSunPower(unsigned int index)
	{
		return this->sunPowers[index]->getReal();
	}

	void AtmosphereComponent::setSkyPower(unsigned int index, Ogre::Real skyPower)
	{
		if (index > this->skyPowers.size())
			index = static_cast<unsigned int>(this->skyPowers.size()) - 1;

		if (skyPower < 0.0f)
		{
			skyPower = 0.0f;
		}

		this->skyPowers[index]->setValue(skyPower);

		if (true == this->enableSky->getBool())
		{
			if (nullptr != this->atmosphereNpr)
			{
				Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
				preset.skyPower = skyPower;

				this->atmosphereNpr->setPreset(preset);
			}
		}
	}

	Ogre::Real AtmosphereComponent::getSkyPower(unsigned int index)
	{
		return this->skyPowers[index]->getReal();
	}

	void AtmosphereComponent::setSkyColor(unsigned int index, const Ogre::Vector3& skyColor)
	{
		if (index > this->skyColors.size())
			index = static_cast<unsigned int>(this->skyColors.size()) - 1;

		Ogre::Vector3 tempSkyColor(skyColor);
		if (tempSkyColor.x < 0.0f)
		{
			tempSkyColor.x = 0.0f;
		}
		else if (tempSkyColor.x > 1.0f)
		{
			tempSkyColor.x = 1.0f;
		}

		if (tempSkyColor.y < 0.0f)
		{
			tempSkyColor.y = 0.0f;
		}
		else if (tempSkyColor.y > 1.0f)
		{
			tempSkyColor.y = 1.0f;
		}

		if (tempSkyColor.z < 0.0f)
		{
			tempSkyColor.z = 0.0f;
		}
		else if (tempSkyColor.z > 1.0f)
		{
			tempSkyColor.z = 1.0f;
		}

		this->skyColors[index]->setValue(tempSkyColor);

		if (true == this->enableSky->getBool())
		{
			if (nullptr != this->atmosphereNpr)
			{
				Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
				preset.skyColour = tempSkyColor;

				this->atmosphereNpr->setPreset(preset);
			}
		}
	}

	Ogre::Vector3 AtmosphereComponent::getSkyColor(unsigned int index)
	{
		return this->skyColors[index]->getVector3();
	}

	void AtmosphereComponent::setFogDensity(unsigned int index, Ogre::Real fogDensity)
	{
		if (index > this->fogDensities.size())
			index = static_cast<unsigned int>(this->fogDensities.size()) - 1;

		if (fogDensity < 0.0f)
		{
			fogDensity = 0.0f;
		}
		else if (fogDensity > 1.0f)
		{
			fogDensity = 1.0f;
		}

		this->fogDensities[index]->setValue(fogDensity);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.fogDensity = fogDensity;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getFogDensity(unsigned int index)
	{
		return this->fogDensities[index]->getReal();
	}

	void AtmosphereComponent::setFogBreakMinBrightness(unsigned int index, Ogre::Real fogBreakMinBrightness)
	{
		if (index > this->fogBreakMinBrightnesses.size())
			index = static_cast<unsigned int>(this->fogBreakMinBrightnesses.size()) - 1;

		if (fogBreakMinBrightness < 0.0f)
		{
			fogBreakMinBrightness = 0.0f;
		}

		this->fogBreakMinBrightnesses[index]->setValue(fogBreakMinBrightness);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.fogBreakMinBrightness = fogBreakMinBrightness;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getFogBreakMinBrightness(unsigned int index)
	{
		return this->fogBreakMinBrightnesses[index]->getReal();
	}

	void AtmosphereComponent::setFogBreakFalloff(unsigned int index, Ogre::Real fogBreakFalloff)
	{
		if (index > this->fogBreakFalloffs.size())
			index = static_cast<unsigned int>(this->fogBreakFalloffs.size()) - 1;

		if (fogBreakFalloff < 0.01f)
		{
			fogBreakFalloff = 0.01f;
		}

		this->fogBreakFalloffs[index]->setValue(fogBreakFalloff);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.fogBreakFalloff = fogBreakFalloff;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getFogBreakFalloff(unsigned int index)
	{
		return this->fogBreakFalloffs[index]->getReal();
	}

	void AtmosphereComponent::setLinkedLightPower(unsigned int index, Ogre::Real linkedLightPower)
	{
		if (index > this->linkedLightPowers.size())
			index = static_cast<unsigned int>(this->linkedLightPowers.size()) - 1;

		if (linkedLightPower < 0.01f)
		{
			linkedLightPower = 0.01f;
		}

		this->linkedLightPowers[index]->setValue(linkedLightPower);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.linkedLightPower = linkedLightPower;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getLinkedLightPower(unsigned int index)
	{
		return this->linkedLightPowers[index]->getReal();
	}

	void AtmosphereComponent::setLinkedAmbientUpperPower(unsigned int index, Ogre::Real linkedAmbientUpperPower)
	{
		if (index > this->linkedSceneAmbientUpperPowers.size())
			index = static_cast<unsigned int>(this->linkedSceneAmbientUpperPowers.size()) - 1;

		if (linkedAmbientUpperPower < 0.01f)
		{
			linkedAmbientUpperPower = 0.01f;
		}

		this->linkedSceneAmbientUpperPowers[index]->setValue(linkedAmbientUpperPower);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.linkedSceneAmbientUpperPower = linkedAmbientUpperPower;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getLinkedAmbientUpperPower(unsigned int index)
	{
		return this->linkedSceneAmbientUpperPowers[index]->getReal();
	}

	void AtmosphereComponent::setLinkedAmbientLowerPower(unsigned int index, Ogre::Real linkedAmbientLowerPower)
	{
		if (index > this->linkedSceneAmbientLowerPowers.size())
			index = static_cast<unsigned int>(this->linkedSceneAmbientLowerPowers.size()) - 1;

		if (linkedAmbientLowerPower < 0.01f)
		{
			linkedAmbientLowerPower = 0.01f;
		}

		this->linkedSceneAmbientLowerPowers[index]->setValue(linkedAmbientLowerPower);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.linkedSceneAmbientLowerPower = linkedAmbientLowerPower;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getLinkedAmbientLowerPower(unsigned int index)
	{
		return this->linkedSceneAmbientLowerPowers[index]->getReal();
	}

	void AtmosphereComponent::setEnvmapScale(unsigned int index, Ogre::Real envmapScale)
	{
		if (index > this->envmapScales.size())
			index = static_cast<unsigned int>(this->envmapScales.size()) - 1;

		if (envmapScale < 0.01f)
		{
			envmapScale = 0.01f;
		}
		else if (envmapScale > 1.0f)
		{
			envmapScale = 1.0f;
		}

		this->envmapScales[index]->setValue(envmapScale);

		if (nullptr != this->atmosphereNpr)
		{
			Ogre::AtmosphereNpr::Preset preset = this->atmosphereNpr->getPreset();
			preset.envmapScale = envmapScale;

			this->atmosphereNpr->setPreset(preset);
		}
	}

	Ogre::Real AtmosphereComponent::getEnvmapScale(unsigned int index)
	{
		return this->envmapScales[index]->getReal();
	}

	void AtmosphereComponent::setSunDir(const Ogre::Radian& sunAltitude, const Ogre::Radian& azimuth)
	{
		this->atmosphereNpr->setSunDir(sunAltitude, azimuth);
	}

	void AtmosphereComponent::setSunDir(const Ogre::Vector3& sunDir, const Ogre::Real normalizedTimeOfDay)
	{
		this->atmosphereNpr->setSunDir(sunDir, normalizedTimeOfDay);
	}

	Ogre::Vector3 AtmosphereComponent::getAtmosphereAt(const Ogre::Vector3& cameraDir, bool bSkipSun)
	{
		return this->atmosphereNpr->getAtmosphereAt(cameraDir, bSkipSun);
	}

	// Lua registration part

	AtmosphereComponent* getAtmosphereComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AtmosphereComponent>(gameObject->getComponentWithOccurrence<AtmosphereComponent>(occurrenceIndex)).get();
	}

	AtmosphereComponent* getAtmosphereComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AtmosphereComponent>(gameObject->getComponent<AtmosphereComponent>()).get();
	}

	AtmosphereComponent* getAtmosphereComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AtmosphereComponent>(gameObject->getComponentFromName<AtmosphereComponent>(name)).get();
	}

	void AtmosphereComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<AtmosphereComponent, GameObjectComponent>("AtmosphereComponent")
			.def("setActivated", &AtmosphereComponent::setActivated)
			.def("isActivated", &AtmosphereComponent::isActivated)
			.def("setEnableSky", &AtmosphereComponent::setEnableSky)
			.def("getEnableSky", &AtmosphereComponent::getEnableSky)
			.def("setTimeMultiplicator", &AtmosphereComponent::setTimeMultiplicator)
			.def("getTimeMultiplicator", &AtmosphereComponent::getTimeMultiplicator)
			.def("setPresetCount", &AtmosphereComponent::setPresetCount)
			.def("getPresetCount", &AtmosphereComponent::getPresetCount)
			.def("setTime", &AtmosphereComponent::setTime)
			.def("getTime", &AtmosphereComponent::getTime)
			.def("setDensityCoefficient", &AtmosphereComponent::setDensityCoefficient)
			.def("getDensityCoefficient", &AtmosphereComponent::getDensityCoefficient)
			.def("setDensityDiffusion", &AtmosphereComponent::setDensityDiffusion)
			.def("getDensityDiffusion", &AtmosphereComponent::getDensityDiffusion)
			.def("setHorizonLimit", &AtmosphereComponent::setHorizonLimit)
			.def("getHorizonLimit", &AtmosphereComponent::getHorizonLimit)
			.def("setSunPower", &AtmosphereComponent::setSunPower)
			.def("getSunPower", &AtmosphereComponent::getSunPower)
			.def("setSkyPower", &AtmosphereComponent::setSkyPower)
			.def("getSkyPower", &AtmosphereComponent::getSkyPower)
			.def("setSkyColor", &AtmosphereComponent::setSkyColor)
			.def("getSkyColor", &AtmosphereComponent::getSkyColor)
			.def("setFogDensity", &AtmosphereComponent::setFogDensity)
			.def("getFogDensity", &AtmosphereComponent::setFogDensity)
			.def("setFogBreakMinBrightness", &AtmosphereComponent::setFogBreakMinBrightness)
			.def("setFogBreakMinBrightness", &AtmosphereComponent::getFogBreakMinBrightness)
			.def("setFogBreakFalloff", &AtmosphereComponent::setFogBreakFalloff)
			.def("getFogBreakFalloff", &AtmosphereComponent::getFogBreakFalloff)
			.def("setLinkedLightPower", &AtmosphereComponent::setLinkedLightPower)
			.def("getLinkedLightPower", &AtmosphereComponent::getLinkedLightPower)
			.def("setLinkedAmbientUpperPower", &AtmosphereComponent::setLinkedAmbientUpperPower)
			.def("getLinkedAmbientUpperPower", &AtmosphereComponent::getLinkedAmbientUpperPower)
			.def("setLinkedAmbientLowerPower", &AtmosphereComponent::setLinkedAmbientLowerPower)
			.def("getLinkedAmbientLowerPower", &AtmosphereComponent::getLinkedAmbientLowerPower)
			.def("setEnvmapScale", &AtmosphereComponent::setEnvmapScale)
			.def("getEnvmapScale", &AtmosphereComponent::getEnvmapScale)
			.def("getCurrentTimeOfDay", &AtmosphereComponent::getCurrentTimeOfDay)
			.def("getCurrentTimeOfDayMinutes", &AtmosphereComponent::getCurrentTimeOfDayMinutes)
			.def("setStartTime", &AtmosphereComponent::setStartTime)
			.def("getStartTime", &AtmosphereComponent::getStartTime)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "class inherits GameObjectComponent", AtmosphereComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setActivated(bool activated)", "If activated, the atmospheric effects will take place.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setEnableSky(bool enableSky)", "Sets whether sky is enabled and visible.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "bool getEnableSky()", "Gets whether sky is enabled and visible.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setTimeMultiplicator(float timeMultiplicator)", "Sets the time multiplier. How long a day lasts. Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getTimeMultiplicator()", "Gets the time multiplier. How long a day lasts. Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setPresetCount(unsigned int presetCount)", "Sets the count of the atmosphere presets.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "unsigned int getPresetCount()", "Gets the count of the atmosphere presets.");

		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setTime(unsigned int index, ´string time)", "Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "string getTime(unsigned int index)", "Gets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setDensityCoefficient(unsigned int index, float densityCoefficient)", " Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getDensityCoefficient(unsigned int index)", "Gets the normalized time for the preset. The time must be in range [-1; 1] where range [-1; 0] is night and [0; 1] is day.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setDensityDiffusion(unsigned int index, float densityDiffusion)", "Sets the density diffusion for the preset. Valid values are in range [0; 1].");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getDensityDiffusion(unsigned int index)", "Gets the density diffusion for the preset. Valid values are in range [0; 1]..");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setHorizonLimit(unsigned int index, float horizonLimit)", "Sets the horizon limit for the preset. Valid values are in range [0; 1]. Most relevant in sunsets and sunrises.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getHorizonLimit(unsigned int index)", "Gets the horizon limit for the preset. Valid values are in range [0; 1]. Most relevant in sunsets and sunrises.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setSunPower(unsigned int index, float sunPower)", "Sets the sun power for the preset. Valid values are in range [0; ]. For HDR (affects the sun on the sky).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getSunPower(unsigned int index)", "Gets the sun power for the preset. Valid values are in range [0; ]. For HDR (affects the sun on the sky).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setSkyPower(unsigned int index, float skyPower)", "Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getSkyPower(unsigned int index)", "Gets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setSkyColor(unsigned int index, Vector3 skyColor)", "Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 1]. Sky must be enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "Vector3 getSkyColor(unsigned int index)", "Gets the sky color for the preset. In range [0; 1], [0; 1], [0; 1]. Sky must be enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setFogDensity(unsigned int index, float fogDensity)", "Sets the fog density for the preset. In range [0; 1]. Affects objects' fog (not sky).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getFogDensity(unsigned int index)", "Gets the fog density for the preset. In range [0; 1]. Affects objects' fog (not sky).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setFogBreakMinBrightness(unsigned int index, float fogBreakMinBrightness)", "Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
			"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
			"becoming more visible.Range: [0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getFogBreakMinBrightness(unsigned int index)", "Gets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog."
			"A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start "
			"becoming more visible.Range: [0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setFogBreakFalloff(unsigned int index, float fogBreakFalloff)", "Sets the fog break falloff for the preset. How fast bright objects appear in fog."
			"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getFogBreakFalloff(unsigned int index)", "Gets the fog break falloff for the preset. How fast bright objects appear in fog."
			"Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setLinkedLightPower(unsigned int index, float linkedLightPower)", "Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getLinkedLightPower(unsigned int index)", "Gets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setLinkedAmbientUpperPower(unsigned int index, float linkedAmbientUpperPower)", "Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getLinkedAmbientUpperPower(unsigned int index)", "Gets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setLinkedAmbientLowerPower(unsigned int index, float linkedAmbientLowerPower)", "Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getLinkedAmbientLowerPower(unsigned int index)", "Gets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setEnvmapScale(unsigned int index, float envmapScale)", "Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "float getEnvmapScale(unsigned int index)", "Gets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "string getCurrentTimeOfDay()", "Gets current time of day as string.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "int getCurrentTimeOfDayMinutes()", "Gets current time of day in minutes.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "void setStartTime(string startTime)", "Sets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight.");
		LuaScriptApi::getInstance()->addClassToCollection("AtmosphereComponent", "string getStartTime()", "Gets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight..");

		gameObjectClass.def("getAtmosphereComponentFromName", &getAtmosphereComponentFromName);
		gameObjectClass.def("getAtmosphereComponent", (AtmosphereComponent * (*)(GameObject*)) & getAtmosphereComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getAtmosphereComponentFromIndex", (AtmosphereComponent * (*)(GameObject*, unsigned int)) & getAtmosphereComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AtmosphereComponent getAtmosphereComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AtmosphereComponent getAtmosphereComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castAtmosphereComponent", &GameObjectController::cast<AtmosphereComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AtmosphereComponent castAtmosphereComponent(AtmosphereComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end