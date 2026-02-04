#ifndef IESLIGHTCOMPONENT_H
#define IESLIGHTCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace Ogre
{
	class LightProfiles;
}

namespace NOWA
{
	class LightSpotComponent;
	class LightPointComponent;

	/**
	  * @brief		IES Light Profile Component for photometric lighting using real-world light distribution data.
	  *				Works with Area Lights and Spotlights. Point and Directional lights are not supported.
	  */
	class EXPORTED IESLightComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<IESLightComponent> IESLightComponentPtr;
	public:

		IESLightComponent();

		virtual ~IESLightComponent();

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
		virtual void setActivated(bool activated) override;

		/**
		 * @brief Gets whether the component is activated.
		 * @return True if activated
		 */
		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets the IES profile file to use for this light
		 * @param[in] iesFileName The name of the .ies file (e.g. "bollard.ies")
		 */
		void setIESProfile(const Ogre::String& iesFileName);

		/**
		 * @brief Gets the currently assigned IES profile filename
		 * @return The IES profile filename
		 */
		Ogre::String getIESProfile(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("IESLightComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "IESLightComponent";
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
			return "Usage: IES (Illuminating Engineering Society) light profiles for photometric lighting. "
				"Applies real-world light distribution patterns to area lights and spotlights. "
				"Requirements: LightAreaComponent or LightSpotComponent.";
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
		static const Ogre::String AttrIESProfile(void)
		{
			return "IES Profile";
		}
	private:
		/**
		 * @brief Initializes the global LightProfiles manager if not already created
		 */
		void initializeLightProfilesManager(void);

		/**
		 * @brief Loads and assigns the IES profile to the light
		 */
		void applyIESProfile(void);

		/**
		 * @brief Removes the IES profile from the light
		 */
		void removeIESProfile(void);

		/**
		 * @brief Populates the available IES files list from the IES resource group
		 */
		void updateAvailableIESFiles(void);

	private:
		Ogre::String name;

		Variant* activated;
		Variant* iesProfile;        // File chooser for .ies files

		GameObjectComponent* lightComponent;  // Reference to any light component type
		Ogre::Light* ogreLight;               // Direct reference to Ogre light

		bool iesProfileApplied;     // Track if profile is currently applied

		// Static shared manager across all IES components
		static Ogre::LightProfiles* s_lightProfilesManager;
		static unsigned int s_instanceCount;
	};

}; // namespace end

#endif