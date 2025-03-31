/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef TIMETRIGGERCOMPONENT_H
#define TIMETRIGGERCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

   /**
	* @class 	TimeTriggerComponent
	* @brief 	This component can be used to activate a prior component of this game object after a time and for an duration.
	*			Info: Several tag child node components can be added to one game object. Each time trigger component should have prior activate-able component, or one time trigger component will activate all prior coming components.
	*			Example: Activating a movement of physics slider for a specific amount of time and starting an animation of the slider.
	*					 Used components: PhysicsActiveComponent->ActiveSliderJointComponent->TimeTriggerComponent->AnimationComponent->TimeTriggerComponent
	*			Requirements: A prior component that is activate-able.
	*/
	class EXPORTED TimeTriggerComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<TimeTriggerComponent> TimeTriggerComponentPtr;
	public:

		TimeTriggerComponent();

		virtual ~TimeTriggerComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

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

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("TimeTriggerComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "TimeTriggerComponent";
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
			return "This component can be used to activate a prior component of this game object after a time and for an duration. "
				"Info: Several tag child node components can be added to one game object.Each time trigger component should have prior activate "
				" - able component, or one time trigger component will activate all prior coming components. "
				"Example : Activating a movement of physics slider for a specific amount of timeand starting an animation of the slider. "
				"Used components : PhysicsActiveComponent->ActiveSliderJointComponent->TimeTriggerComponent->AnimationComponent->TimeTriggerComponent "
				"Requirements : A prior component that is activate - able.";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		/**
		 * @brief Sets whether to activate just the one prior (predecessor) component, which is one above this component, or activate all components for this game object.
		 * @param[in] activateOne
		 */
		void setActivateOne(bool activateOne);

		/**
		 * @brief Gets whether to activate just the one prior (predecessor) component, which is one above this component, or activate all components for this game object.
		 * @return activateOne
		 */
		bool getActivateOne(void) const;

		/**
		 * @brief Sets the start time in milliseconds at which the prior component will be activated.
		 * @param[in] startTime The start time in milliseconds to set
		 */
		void setStartTime(Ogre::Real startTime);

		/**
		 * @brief Gets the start time in milliseconds at which the prior component is activated.
		 * @return startTime The start time in milliseconds to get
		 */
		Ogre::Real getStartTime(void) const;

		/**
		 * @brief Sets the duration in milliseconds how long the prior component will remain activated.
		 * @param[in] duration The duration in milliseconds to set
		 */
		void setDuration(Ogre::Real duration);

		/**
		 * @brief Gets the duration in milliseconds how long the prior component is remain activated.
		 * @return duration The duration in milliseconds to get
		 */
		Ogre::Real getDuration(void) const;

		/**
		 * @brief Sets whether to repeat the start of activation after the duration has been exceeded for the prior component
		 * @param[in] repeat Whether the whole procedure should be repeated or not.
		 */
		void setRepeat(bool repeat);

		/**
		 * @brief Gets whether the start of activation after the duration has been exceeded for the prior component is repeated
		 * @return repeat true, if the whole procedure should be repeated, else false
		 */
		bool getRepeat(void) const;

		void setDeactivateAfterwards(bool deactivateAfterwards);

		bool getDeactivateAfterwards(void) const;
	private:
		void activateComponent(bool bActivate);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrActivateOne(void) { return "Activate One"; }
		static const Ogre::String AttrStartTime(void) { return "Start Time"; }
		static const Ogre::String AttrDuration(void) { return "Duration"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrDeactivateAfterwards(void) { return "Deactivate Afterwards"; }
	private:
		Variant* activated;
		Variant* activateOne;
		Variant* startTime;
		Variant* duration;
		Variant* repeat;
		Variant* deactivateAfterwards;

		Ogre::String name;
		Ogre::Real timeDt;
		bool startCounting;
		bool firstTimeActivated;
	};

}; // namespace end

#endif

