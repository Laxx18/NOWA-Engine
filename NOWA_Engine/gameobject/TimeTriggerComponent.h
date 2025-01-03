#ifndef TIME_TRIGGER_COMPONENT_H
#define TIME_TRIGGER_COMPONENT_H

#include "GameObjectComponent.h"

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
	class EXPORTED TimeTriggerComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::TimeTriggerComponent> TimeTriggerCompPtr;
	public:

		TimeTriggerComponent();

		virtual ~TimeTriggerComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

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
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("TimeTriggerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "TimeTriggerComponent";
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
			return "This component can be used to activate a prior component of this game object after a time and for an duration. "
				  "Info: Several tag child node components can be added to one game object.Each time trigger component should have prior activate "
				  " - able component, or one time trigger component will activate all prior coming components. "
				  "Example : Activating a movement of physics slider for a specific amount of timeand starting an animation of the slider. "
				  "Used components : PhysicsActiveComponent->ActiveSliderJointComponent->TimeTriggerComponent->AnimationComponent->TimeTriggerComponent "
				  "Requirements : A prior component that is activate - able.";
		}

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

		virtual bool isActivated(void) const override;

		/**
		 * @see		GameObjectComponent::connect
		 */
		virtual bool connect(void) override;

		/**
		 * @see		GameObjectComponent::disconnect
		 */
		virtual bool disconnect(void) override;

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
		static const Ogre::String AttrStartTime(void) { return "Start Time"; }
		static const Ogre::String AttrDuration(void){ return "Duration"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrDeactivateAfterwards(void) { return "Deactivate Afterwards"; }
	private:
		Variant* activated;
		Variant* startTime;
		Variant* duration;
		Variant* repeat;
		Variant* deactivateAfterwards;
		Ogre::Real timeDt;
		bool startCounting;
		bool firstTimeActivated;
	};

}; //namespace end

#endif