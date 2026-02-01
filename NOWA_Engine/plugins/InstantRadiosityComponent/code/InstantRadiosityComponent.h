#ifndef INSTANT_RADIOSITY_COMPONENT_H
#define INSTANT_RADIOSITY_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace Ogre
{
	class InstantRadiosity;
	class IrradianceVolume;
}

namespace NOWA
{
	class LightAreaOfInterestComponent;

	/**
	  * @brief		Instant Radiosity (IR) component for Global Illumination.
	  *				IR traces rays in CPU and creates VPLs (Virtual Point Lights) at every hit point
	  *				to mimic the effect of Global Illumination.
	  *				Features:
	  *				- Works on dynamic objects
	  *				- VPLs get averaged and clustered based on cluster size to improve speed
	  *				- Can use Irradiance Volumes instead of VPLs for better performance
	  *				- Automatically collects LightAreaOfInterestComponents for directional light ray targeting
	  *				Tips to fight light leaking:
	  *				- Smaller cluster sizes are more accurate but slower (more VPLs)
	  *				- Smaller VplMaxRange is faster and leaks less but has lower illumination reach
	  *				- Bias pushes VPL placement away from true position (not physically accurate but helps)
	  *				- VplThreshold removes weak VPLs, improving performance
	  *
	  *				For directional lights to work properly in enclosed spaces (buildings), place
	  *				LightAreaOfInterestComponent markers at windows/skylights/openings.
	  */
	class EXPORTED InstantRadiosityComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<InstantRadiosityComponent> InstantRadiosityComponentPtr;
	public:

		InstantRadiosityComponent();

		virtual ~InstantRadiosityComponent();

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

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
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @brief Sets whether the component is activated.
		 * @param[in] activated Whether to activate the component
		 */
		void setActivated(bool activated);

		/**
		 * @brief Gets whether the component is activated.
		 * @return True if activated
		 */
		bool isActivated(void) const;

		/**
		 * @brief Sets the number of rays to cast for IR calculation.
		 * @param[in] numRays Number of rays (power of 2 recommended, max 32768)
		 */
		void setNumRays(unsigned int numRays);

		/**
		 * @brief Gets the number of rays.
		 * @return Number of rays
		 */
		unsigned int getNumRays(void) const;

		/**
		 * @brief Sets the number of ray bounces.
		 * @param[in] numRayBounces Number of bounces (0-5)
		 */
		void setNumRayBounces(unsigned int numRayBounces);

		/**
		 * @brief Gets the number of ray bounces.
		 * @return Number of bounces
		 */
		unsigned int getNumRayBounces(void) const;

		/**
		 * @brief Sets the number of spread iterations for VPL clustering.
		 * @param[in] numSpreadIterations Number of iterations (0-10)
		 */
		void setNumSpreadIterations(unsigned int numSpreadIterations);

		/**
		 * @brief Gets the number of spread iterations.
		 * @return Number of spread iterations
		 */
		unsigned int getNumSpreadIterations(void) const;

		/**
		 * @brief Sets the cell/cluster size for VPL grouping.
		 *        Smaller values are more accurate but slower.
		 * @param[in] cellSize Cell size in world units
		 */
		void setCellSize(Ogre::Real cellSize);

		/**
		 * @brief Gets the cell size.
		 * @return Cell size
		 */
		Ogre::Real getCellSize(void) const;

		/**
		 * @brief Sets the bias for VPL placement.
		 *        Pushes VPLs away from surfaces to reduce light leaking.
		 * @param[in] bias Bias value (0.0 - 1.0)
		 */
		void setBias(Ogre::Real bias);

		/**
		 * @brief Gets the bias value.
		 * @return Bias value
		 */
		Ogre::Real getBias(void) const;

		/**
		 * @brief Sets the maximum range for VPLs.
		 *        Smaller values are faster and leak less but have lower illumination reach.
		 * @param[in] vplMaxRange Maximum range in world units
		 */
		void setVplMaxRange(Ogre::Real vplMaxRange);

		/**
		 * @brief Gets the VPL maximum range.
		 * @return VPL maximum range
		 */
		Ogre::Real getVplMaxRange(void) const;

		/**
		 * @brief Sets the power boost multiplier for VPLs.
		 * @param[in] vplPowerBoost Power boost multiplier
		 */
		void setVplPowerBoost(Ogre::Real vplPowerBoost);

		/**
		 * @brief Gets the VPL power boost.
		 * @return VPL power boost
		 */
		Ogre::Real getVplPowerBoost(void) const;

		/**
		 * @brief Sets the threshold for removing weak VPLs.
		 *        Higher values remove more VPLs, improving performance.
		 * @param[in] vplThreshold Threshold value
		 */
		void setVplThreshold(Ogre::Real vplThreshold);

		/**
		 * @brief Gets the VPL threshold.
		 * @return VPL threshold
		 */
		Ogre::Real getVplThreshold(void) const;

		/**
		 * @brief Sets whether to use intensity for max range calculation.
		 * @param[in] useIntensityForMaxRange Whether to use intensity
		 */
		void setVplUseIntensityForMaxRange(bool useIntensityForMaxRange);

		/**
		 * @brief Gets whether intensity is used for max range.
		 * @return True if using intensity for max range
		 */
		bool getVplUseIntensityForMaxRange(void) const;

		/**
		 * @brief Sets the intensity range multiplier.
		 * @param[in] vplIntensityRangeMultiplier Multiplier value
		 */
		void setVplIntensityRangeMultiplier(Ogre::Real vplIntensityRangeMultiplier);

		/**
		 * @brief Gets the VPL intensity range multiplier.
		 * @return Intensity range multiplier
		 */
		Ogre::Real getVplIntensityRangeMultiplier(void) const;

		/**
		 * @brief Sets whether to show debug markers for VPLs.
		 * @param[in] enableDebugMarkers Whether to show debug markers
		 */
		void setEnableDebugMarkers(bool enableDebugMarkers);

		/**
		 * @brief Gets whether debug markers are enabled.
		 * @return True if debug markers are enabled
		 */
		bool getEnableDebugMarkers(void) const;

		/**
		 * @brief Sets whether to use Irradiance Volume instead of VPLs.
		 * @param[in] useIrradianceVolume Whether to use irradiance volume
		 */
		void setUseIrradianceVolume(bool useIrradianceVolume);

		/**
		 * @brief Gets whether irradiance volume is used.
		 * @return True if using irradiance volume
		 */
		bool getUseIrradianceVolume(void) const;

		/**
		 * @brief Sets the irradiance volume cell size.
		 * @param[in] irradianceCellSize Cell size for irradiance volume
		 */
		void setIrradianceCellSize(Ogre::Real irradianceCellSize);

		/**
		 * @brief Gets the irradiance volume cell size.
		 * @return Irradiance cell size
		 */
		Ogre::Real getIrradianceCellSize(void) const;

		/**
		 * @brief Sets whether to use scene bounds as fallback AoI when no LightAoI components exist.
		 * @param[in] useSceneBoundsAsFallback Whether to use scene bounds
		 */
		void setUseSceneBoundsAsFallback(bool useSceneBoundsAsFallback);

		/**
		 * @brief Gets whether scene bounds is used as fallback AoI.
		 * @return True if using scene bounds as fallback
		 */
		bool getUseSceneBoundsAsFallback(void) const;

		/**
		 * @brief Manually triggers a rebuild of the IR data.
		 *        Call this after scene changes or when lights move significantly.
		 */
		void build(void);

		/**
		 * @brief Updates existing VPLs without full rebuild.
		 *        Use this for parameter changes that don't require full rebuild.
		 */
		void updateExistingVpls(void);

		/**
		 * @brief Refreshes the Areas of Interest from all LightAreaOfInterestComponents in the scene.
		 *        Call this if you add/remove LightAoI components at runtime.
		 */
		void refreshAreasOfInterest(void);

		/**
		 * @brief Gets the InstantRadiosity object for advanced usage.
		 * @return Pointer to Ogre::InstantRadiosity or nullptr
		 */
		Ogre::InstantRadiosity* getInstantRadiosity(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("InstantRadiosityComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "InstantRadiosityComponent";
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
			return "Usage: Instant Radiosity for Global Illumination. Traces rays and creates Virtual Point Lights (VPLs) "
				"at hit points to simulate indirect lighting. Works on dynamic objects.\n\n"

				"For directional lights in enclosed spaces (buildings with windows), place LightAreaOfInterestComponent markers "
				"at windows/openings. The AoI tells the system: \"Shoot rays HERE specifically\" - because directional light rays "
				"come from infinity and need to know where openings are to enter a building.\n\n"

				"Scenario example:\n"
				"You have a house mesh with 3 windows. Without AoIs, directional light rays would be shot randomly and most would "
				"hit the exterior walls, wasting computation and missing the interior.\n\n"

				"Solution:\n"
				"Create empty GameObjects at each window opening and add LightAreaOfInterestComponent.\n\n"

				"Scene:\n"
				"--> House (mesh with windows baked in)\n"
				"--> Window1_AoI (empty GO at window position)\n"
				"  |--> LightAreaOfInterestComponent (halfSize matches window size)\n"
				"--> Window2_AoI\n"
				"  |--> LightAreaOfInterestComponent\n"
				"--> Window3_AoI\n"
				"  |--> LightAreaOfInterestComponent\n"
				"--> SunLight (directional)\n"
				"--> IRController\n"
				"  | --> InstantRadiosityComponent\n\n"
				"When is EventDataBoundsUpdated sufficient?\n"
				"For outdoor scenes or scenes with only point/spot lights - you don't need specific AoIs. "
				"The scene bounds fallback works fine because:\n"
				"- Point/spot lights have a defined position, rays radiate from there\n"
				"- Outdoor scenes don't have \"openings\" to target\n\n"

				"Requirements: Forward+ rendering with VPLs enabled. Only one instance allowed per scene.";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrActivated(void)
		{
			return "Activated";
		}
		static const Ogre::String AttrNumRays(void)
		{
			return "Num Rays";
		}
		static const Ogre::String AttrNumRayBounces(void)
		{
			return "Num Ray Bounces";
		}
		static const Ogre::String AttrNumSpreadIterations(void)
		{
			return "Spread Iterations";
		}
		static const Ogre::String AttrCellSize(void)
		{
			return "Cell Size";
		}
		static const Ogre::String AttrBias(void)
		{
			return "Bias";
		}
		static const Ogre::String AttrVplMaxRange(void)
		{
			return "VPL Max Range";
		}
		static const Ogre::String AttrVplPowerBoost(void)
		{
			return "VPL Power Boost";
		}
		static const Ogre::String AttrVplThreshold(void)
		{
			return "VPL Threshold";
		}
		static const Ogre::String AttrVplUseIntensityForMaxRange(void)
		{
			return "Use Intensity For Max Range";
		}
		static const Ogre::String AttrVplIntensityRangeMultiplier(void)
		{
			return "Intensity Range Multiplier";
		}
		static const Ogre::String AttrEnableDebugMarkers(void)
		{
			return "Enable Debug Markers";
		}
		static const Ogre::String AttrUseIrradianceVolume(void)
		{
			return "Use Irradiance Volume";
		}
		static const Ogre::String AttrIrradianceCellSize(void)
		{
			return "Irradiance Cell Size";
		}
		static const Ogre::String AttrUseSceneBoundsAsFallback(void)
		{
			return "Use Scene Bounds As Fallback";
		}

	private:
		void createInstantRadiosity(void);
		void destroyInstantRadiosity(void);
		void internalUpdateIrradianceVolume(void);
		void internalBuild(void);
		void collectAreasOfInterest(void);

		void handleUpdateBounds(NOWA::EventDataPtr eventData);
	private:
		Ogre::String name;

		Ogre::InstantRadiosity* instantRadiosity;
		Ogre::IrradianceVolume* irradianceVolume;

		Variant* activated;
		Variant* numRays;
		Variant* numRayBounces;
		Variant* numSpreadIterations;
		Variant* cellSize;
		Variant* bias;
		Variant* vplMaxRange;
		Variant* vplPowerBoost;
		Variant* vplThreshold;
		Variant* vplUseIntensityForMaxRange;
		Variant* vplIntensityRangeMultiplier;
		Variant* enableDebugMarkers;
		Variant* useIrradianceVolume;
		Variant* irradianceCellSize;
		Variant* useSceneBoundsAsFallback;

		bool needsRebuild;
		bool needsVplUpdate;
		bool needsIrradianceVolumeUpdate;
		Ogre::AxisAlignedBox sceneBounds;
	};

}; // namespace end

#endif