#ifndef PCCPERPIXELGRIDPLACEMENTCOMPONENT_H
#define PCCPERPIXELGRIDPLACEMENTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace Ogre
{
	class ParallaxCorrectedCubemapAuto;
}

namespace NOWA
{
	class CameraComponent;
	class WorkspaceBaseComponent;

	/**
	  * @brief		Automatically places and manages Parallax Corrected Cubemap (PCC) reflection probes in a grid pattern.
	  *				This component analyzes scene geometry per-pixel to intelligently determine optimal probe placement,
	  *				size, and blending for high-quality reflections in interior spaces and complex scenes.
	  */
	class EXPORTED PccPerPixelGridPlacementComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<PccPerPixelGridPlacementComponent> PccPerPixelGridPlacementComponentPtr;
	public:

		PccPerPixelGridPlacementComponent();

		virtual ~PccPerPixelGridPlacementComponent();

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
		 * @brief Activates or deactivates the PCC system
		 * @param[in] activated True to enable PCC, false to disable
		 */
		void setActivated(bool activated);

		/**
		 * @brief Gets whether PCC is currently active
		 * @return True if active, false otherwise
		 */
		bool isActivated(void) const;

		/**
		 * @brief Sets the center position of the region to be covered by PCC probes
		 * @param[in] regionCenter The center point of the AABB region
		 */
		void setRegionCenter(const Ogre::Vector3& regionCenter);

		/**
		 * @brief Gets the center position of the PCC region
		 * @return The region center vector
		 */
		Ogre::Vector3 getRegionCenter(void) const;

		/**
		 * @brief Sets the half-size (extents) of the region to be covered by PCC probes
		 * @param[in] regionHalfSize The half-dimensions of the AABB region
		 */
		void setRegionHalfSize(const Ogre::Vector3& regionHalfSize);

		/**
		 * @brief Gets the half-size of the PCC region
		 * @return The region half-size vector
		 */
		Ogre::Vector3 getRegionHalfSize(void) const;

		/**
		 * @brief Sets the overlap factor for probe blending (higher = better quality, lower performance)
		 * @param[in] overlapFactor The overlap multiplier (default 1.25)
		 */
		void setOverlapFactor(Ogre::Real overlapFactor);

		/**
		 * @brief Gets the current overlap factor
		 * @return The overlap factor value
		 */
		Ogre::Real getOverlapFactor(void) const;

		/**
		 * @brief Sets the resolution of each cubemap probe
		 * @param[in] probeResolution Resolution in pixels (e.g., 256, 512, 1024)
		 */
		void setProbeResolution(unsigned int probeResolution);

		/**
		 * @brief Gets the current probe resolution
		 * @return The probe resolution in pixels
		 */
		unsigned int getProbeResolution(void) const;

		/**
		 * @brief Sets the near plane distance for probe rendering
		 * @param[in] nearPlane Near clipping distance
		 */
		void setNearPlane(Ogre::Real nearPlane);

		/**
		 * @brief Gets the near plane distance
		 * @return The near plane value
		 */
		Ogre::Real getNearPlane(void) const;

		/**
		 * @brief Sets the far plane distance for probe rendering
		 * @param[in] farPlane Far clipping distance
		 */
		void setFarPlane(Ogre::Real farPlane);

		/**
		 * @brief Gets the far plane distance
		 * @return The far plane value
		 */
		Ogre::Real getFarPlane(void) const;

		/**
		 * @brief Toggles between Dual Paraboloid Mapping (DPM) and Cubemap Arrays
		 * @param[in] useDpm2DArray True to use DPM 2D arrays, false for cubemap arrays
		 * @note DPM can be more efficient but cubemap arrays may offer better quality
		 */
		void setUseDpm2DArray(bool useDpm2DArray);

		/**
		 * @brief Gets whether DPM 2D arrays are being used
		 * @return True if using DPM, false if using cubemap arrays
		 */
		bool getUseDpm2DArray(void) const;

		/**
		 * @brief Forces all probes to update/re-render
		 * @note Useful after significant scene changes
		 */
		void forceUpdateAllProbes(void);

		/**
		 * @brief Rebuilds the entire probe grid
		 * @note Called automatically when region or overlap changes
		 */
		void rebuildProbeGrid(void);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PccPerPixelGridPlacementComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "PccPerPixelGridPlacementComponent";
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
			return "Usage: Automatically places and manages Parallax Corrected Cubemap (PCC) reflection probes in a grid pattern throughout a defined region. "
				"The system analyzes scene geometry per-pixel to intelligently determine optimal probe placement, size, and blending for high-quality reflections in interior spaces and complex scenes. "
				"\n\n"
				"Key Features:\n"
				"- Automatic probe placement: Define a region (AABB), system places probes optimally\n"
				"- Parallax correction: Provides accurate reflections with proper depth perception\n"
				"- Quality control: Adjustable overlap factor trades quality vs performance\n"
				"- Multiple probes: Better coverage than single cubemap, especially for interiors\n"
				"\n\n"
				"Configuration:\n"
				"- Region Center/Half-Size: Defines the AABB volume to cover with probes\n"
				"- Overlap Factor: 1.0 (minimal, faster) to 2.0 (maximum quality, slower). Default: 1.25\n"
				"- Probe Resolution: 128, 256, 512, or 1024 pixels. Higher = better quality but more memory\n"
				"- Near/Far Plane: Clipping distances for probe rendering (default: 0.02 to 10.0)\n"
				"- DPM vs Cubemap Arrays: Toggle rendering method (DPM is more efficient)\n"
				"\n\n"
				"Requirements:\n"
				"- Camera component with workspace (WorkspacePbsComponent or similar)\n"
				"- Forward Clustered rendering must be enabled\n"
				"- PCC/DepthCompressor material must be available\n"
				"\n\n"
				"Constraints:\n"
				"- Cannot be used simultaneously with ReflectionCameraComponent (mutually exclusive)\n"
				"- Only one PCC component per camera game object\n"
				"- Objects must be within the defined region to receive reflections\n"
				"\n\n"
				"Performance Notes:\n"
				"- Probe count is automatically determined by region size and overlap factor\n"
				"- Memory usage: ProbeResolution × ProbeCount × 6 faces × pixel size\n"
				"- Higher overlap = smoother transitions but more GPU cost\n"
				"- Typical scene: 256x256 probes with 1.25 overlap for balanced quality/performance\n"
				"\n\n"
				"Best Use Cases:\n"
				"- Interior environments (rooms, hallways, buildings)\n"
				"- Complex architectural spaces\n"
				"- Areas requiring accurate parallax-corrected reflections\n"
				"- Scenes where single cubemap reflections look incorrect";
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
		static const Ogre::String AttrRegionCenter(void)
		{
			return "Region Center";
		}
		static const Ogre::String AttrRegionHalfSize(void)
		{
			return "Region Half Size";
		}
		static const Ogre::String AttrOverlapFactor(void)
		{
			return "Overlap Factor";
		}
		static const Ogre::String AttrProbeResolution(void)
		{
			return "Probe Resolution";
		}
		static const Ogre::String AttrNearPlane(void)
		{
			return "Near Plane";
		}
		static const Ogre::String AttrFarPlane(void)
		{
			return "Far Plane";
		}
		static const Ogre::String AttrUseDpm2DArray(void)
		{
			return "Use DPM 2D Array";
		}
	private:
		void createPccSystem(void);
		void destroyPccSystem(void);
	private:
		Ogre::String name;

		Variant* activated;
		Variant* regionCenter;
		Variant* regionHalfSize;
		Variant* overlapFactor;
		Variant* probeResolution;
		Variant* nearPlane;
		Variant* farPlane;
		Variant* useDpm2DArray;

		WorkspaceBaseComponent* workspaceBaseComponent;
		CameraComponent* cameraComponent;
	};

}; // namespace end

#endif