/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef WATERVOLUMECOMPONENT_H
#define WATERVOLUMECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class WorkspaceBaseComponent;

	/**
	  * @brief		Turns the game object's mesh (typically a scaled box) into a water region: everything inside it or seen through it
	  *				gets distorted, fogged, tinted and lit by caustics like being under water, with an animated waterline on top.
	  *				Works exactly like the DistortionComponent: the item is moved into its own render queue
	  *				(WorkspaceBaseComponent::WATER_VOLUME_RENDER_QUEUE), which is rendered only into a separate render target
	  *				(rt_waterVolume), and a fullscreen pass (WaterVolume/Quad) applies the effect only where that target is set.
	  *				Completely independent from the ocean underwater effect (camera-below-ocean fullscreen effect for 3D).
	  *
	  *				What the item writes into rt_waterVolume (Unlit, no blending, depth check on, depth write off, no culling):
	  *				- R: 1 = pixel is covered by a water volume
	  *				- G: view depth of the volume's FRONT, divided by the camera far clip distance
	  *				- B: view depth of the volume's BACK, divided by the camera far clip distance
	  *				- A: this volume's 'Intensity'
	  *				Front/back are computed every frame from the item's world AABB and the camera. The shader then knows how much
	  *				water lies between the viewer and every pixel (thickness = min(scene depth, back) - front), which is what makes
	  *				the fog work for a camera OUTSIDE the water (2.5D side view) instead of fogging by full camera distance.
	  *
	  *				Look attributes (tints, fog, distortion, caustics, waterline) are uniforms of the one shared fullscreen pass,
	  *				so they are GLOBAL for all water volumes - the last changed/created volume wins. Only 'Intensity' is per volume.
	  */
	class EXPORTED WaterVolumeComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<WaterVolumeComponent> WaterVolumeComponentPtr;
	public:

		WaterVolumeComponent();

		virtual ~WaterVolumeComponent();

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

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
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setIntensity(Ogre::Real intensity);

		Ogre::Real getIntensity(void) const;

		void setWaterTint(const Ogre::Vector3& waterTint);

		Ogre::Vector3 getWaterTint(void) const;

		void setDeepWaterTint(const Ogre::Vector3& deepWaterTint);

		Ogre::Vector3 getDeepWaterTint(void) const;

		void setFogDensity(Ogre::Real fogDensity);

		Ogre::Real getFogDensity(void) const;

		void setDeepThickness(Ogre::Real deepThickness);

		Ogre::Real getDeepThickness(void) const;

		void setAbsorption(Ogre::Real absorption);

		Ogre::Real getAbsorption(void) const;

		void setDistortion(Ogre::Real distortion);

		Ogre::Real getDistortion(void) const;

		void setCausticStrength(Ogre::Real causticStrength);

		Ogre::Real getCausticStrength(void) const;

		void setSurfaceLineStrength(Ogre::Real surfaceLineStrength);

		Ogre::Real getSurfaceLineStrength(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("WaterVolumeComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "WaterVolumeComponent";
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
			return "Usage: Turns this game object's mesh (typically a scaled box) into a water region. Everything inside it or seen through it "
				   "gets distorted, fogged, tinted and lit by caustics like being under water, with an animated waterline on top - e.g. a pool in "
				   "a 2.5D Jump'n'Run the player can dive into. Works like the DistortionComponent: the mesh itself becomes invisible while active "
				   "and only acts as the region. Only the main camera is supported. "
				   "Note: Tints, fog, distortion, caustics and waterline are shared by ALL water volumes (one fullscreen pass) - the last changed "
				   "or created volume wins. 'Intensity' is per volume. "
				   "Requirements: The game object must be an item (mesh). The main camera's workspace gets 'Use Water Volume' activated automatically.";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrIntensity(void) { return "Intensity"; }
		static const Ogre::String AttrWaterTint(void) { return "Water Tint"; }
		static const Ogre::String AttrDeepWaterTint(void) { return "Deep Water Tint"; }
		static const Ogre::String AttrFogDensity(void) { return "Fog Density"; }
		static const Ogre::String AttrDeepThickness(void) { return "Deep Thickness"; }
		static const Ogre::String AttrAbsorption(void) { return "Absorption"; }
		static const Ogre::String AttrDistortion(void) { return "Distortion"; }
		static const Ogre::String AttrCausticStrength(void) { return "Caustic Strength"; }
		static const Ogre::String AttrSurfaceLineStrength(void) { return "Surface Line Strength"; }
	private:
		void createWaterVolume(void);

		void destroyWaterVolume(void);

		/**
		 * @brief RENDER-THREAD ONLY. Recomputes the front/back view depth of the item's world AABB for the given camera and writes
		 *        mask/front/back/intensity into the datablock colour (see class comment for the channel layout).
		 */
		void updateVolumeDepthsInternal(Ogre::Camera* camera, Ogre::Real intensity);

		/**
		 * @brief RENDER-THREAD ONLY. Pushes the shared look attributes into the WaterVolume/Quad fragment program parameters.
		 */
		void applyLookParametersInternal(void);

		/**
		 * @brief Logic-thread entry point for applyLookParametersInternal(), used by all look attribute setters.
		 */
		void applyLookParameters(void);

		Ogre::String getUpdateClosureId(void) const;
	private:
		Ogre::String name;
		Ogre::String oldDatablockName;
		unsigned int oldRenderQueueIndex;
		bool oldCastShadows;
		/// True while the item's original render queue / cast shadows are stored, i.e. while the volume is created
		bool hasOldRenderQueueIndex;
		Ogre::HlmsUnlitDatablock* waterVolumeDatablock;

		Variant* activated;
		Variant* intensity;
		Variant* waterTint;
		Variant* deepWaterTint;
		Variant* fogDensity;
		Variant* deepThickness;
		Variant* absorption;
		Variant* distortion;
		Variant* causticStrength;
		Variant* surfaceLineStrength;
	};

}; // namespace end

#endif
