#ifndef LIGHT_AREA_OF_INTEREST_COMPONENT_H
#define LIGHT_AREA_OF_INTEREST_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	  * @brief		Light Area of Interest component for Instant Radiosity.
	  *				Place this component on GameObjects that represent light entry points
	  *				like windows, skylights, or openings where directional light enters a building.
	  *				The InstantRadiosityComponent will automatically collect all LightAoIs in the scene.
	  *
	  *				The AoI defines a box (AABB) where rays from directional lights should be targeted,
	  *				and a sphere radius that determines how far rays can travel through the opening.
	  *
	  *				Usage: Create an empty GameObject, position it at your window/opening, add this component,
	  *				and adjust the half-size to match the opening dimensions.
	  */
	class EXPORTED LightAreaOfInterestComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<LightAreaOfInterestComponent> LightAreaOfInterestComponentPtr;
	public:

		LightAreaOfInterestComponent();

		virtual ~LightAreaOfInterestComponent();

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
		 * @brief Sets the half-size of the AoI box.
		 *        The full box extends from (center - halfSize) to (center + halfSize).
		 * @param[in] halfSize Half-size in each dimension (x, y, z)
		 */
		void setHalfSize(const Ogre::Vector3& halfSize);

		/**
		 * @brief Gets the half-size of the AoI box.
		 * @return Half-size vector
		 */
		Ogre::Vector3 getHalfSize(void) const;

		/**
		 * @brief Sets the sphere radius for ray tracing.
		 *        Larger radius ensures rays hit walls behind the opening instead of passing through.
		 * @param[in] sphereRadius Sphere radius in world units
		 */
		void setSphereRadius(Ogre::Real sphereRadius);

		/**
		 * @brief Gets the sphere radius.
		 * @return Sphere radius
		 */
		Ogre::Real getSphereRadius(void) const;

		/**
		 * @brief Gets the center position of the AoI (from the GameObject's position).
		 * @return Center position in world space
		 */
		Ogre::Vector3 getCenter(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("LightAreaOfInterestComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "LightAreaOfInterestComponent";
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
			return "Usage: Marks a light entry point (window, skylight, opening) for Instant Radiosity. "
				"Place on an empty GameObject at the opening position. The InstantRadiosityComponent "
				"will automatically use all LightAreaOfInterestComponents in the scene.\n\n"

				"Why is this needed?\n"
				"You need it for directional lights in enclosed spaces (buildings with windows).\n"
				"The AoI tells the system: \"Shoot rays HERE specifically\" - because directional light rays come from infinity "
				"and need to know where the openings are to enter a building.\n\n"

				"Scenario example:\n"
				"You have a house mesh with 3 windows. Without AoIs, directional light rays would be shot randomly and most would "
				"hit the exterior walls, wasting computation and missing the interior.\n\n"

				"Solution:\n"
				"Create 3 empty GameObjects, position each at a window opening, add LightAreaOfInterestComponent to each.\n"
				"(halfSize matches window size)\n\n"

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
				"- Outdoor scenes don't have \"openings\" to target\n";
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
		static const Ogre::String AttrHalfSize(void)
		{
			return "Half Size";
		}
		static const Ogre::String AttrSphereRadius(void)
		{
			return "Sphere Radius";
		}

	private:
		Ogre::String name;

		Variant* activated;
		Variant* halfSize;
		Variant* sphereRadius;
	};

}; // namespace end

#endif