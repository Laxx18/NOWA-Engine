#ifndef PHYSICS_EXPLOSION_COMPONENT_H
#define PHYSICS_EXPLOSION_COMPONENT_H

#include "GameObjectComponent.h"
#include <boost/make_shared.hpp>
#include "main/Events.h"

namespace NOWA
{
	class PhysicsComponent;
	class Ogre::SphereSceneQuery;
	
	class EXPORTED PhysicsExplosionComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsExplosionComponent> PhysicsExplosionCompPtr;
		typedef boost::shared_ptr<PhysicsComponent> PhysicsCompPtr;
	public:
		/**
		* @class IExplosionCallback
		* @brief This interface can be implemented to react each time a game object explodes, e.g. to play a sound or add a particle effect etc.
		*/
		class EXPORTED IExplosionCallback
		{
		public:
			virtual ~IExplosionCallback()
			{
			}

			virtual IExplosionCallback* clone(void) = 0;

			/**
			* @brief		Called each second when the explosion timer ticked
			* @param[in]	originGameObject	The origin game object to do something with, when the timer ticks, like playing a sound.
			*/
			virtual void onTimerSecondTick(GameObject* originGameObject) = 0;

			/**
			* @brief		Called when bomb explodes.
			* @param[in]	originGameObject	The origin game object which did explode
			* @note			This function is called only once the bomb explodes.
			*/
			virtual void onExplode(GameObject* originGameObject) = 0;
			
			/**
			* @brief		Called when an affected game object explodes.
			* @param[in]	originGameObject	The origin game object which did explode
			* @param[in]	spawnedGameObject	The affected game objects that had been in range of the detonation
			* @param[in]	distance			The distance from the affected game object to the bomb
			* @param[in]	strength			The detonation strength. Note: The more away the affected the game object is located from the bomb, the less the detonation strength.
			* @note			This function is called x-times, that is for each game object that is affected by the explosion
			*/
			virtual void onExplodeAffectedGameObject(GameObject* originGameObject, GameObject* affectedGameObject, Ogre::Real distance, unsigned int strength) = 0;
		};
	public:
		PhysicsExplosionComponent();

		virtual ~PhysicsExplosionComponent();

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsExplosionComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsExplosionComponent";
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
			return "Usage: With this component a nice physically explosion effect is created. That is all physics active components, which are located in the critical radius will be thrown away.";
		}

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
		 * @brief Activates the components behaviour, so that explosion will be controller by the explosion timer
		 * @param activated Whether to activate or deactivate the explosion.
		 * @note A nice scenario would be, that the bomb has a timer, which counts down because the component has been activated.
		 *		Now a player may disarm the bomb, so that the component will be deactivated and the timer stops.
		 */
		virtual void setActivated(bool activated) override;

	   /**
		* @brief Gets whether the component behaviour is activated or not.
		* @return activated True, if the components behaviour is activated, else false
		*/
		virtual bool isActivated(void) const override;

		/**
		* @brief Sets affected categories
		* @param categoryIds	The affected categories to set.
		* @note	This function can be used e.g. to exclude some game object, even if there were at range when the detonation occurred.
		* @see GameObjectController::generateCategoryId() for further details how combine categories
		*/
		void setAffectedCategories(const Ogre::String& categories);

		/**
		* @brief Gets affected categories
		* @return categories The affected categories
		*/
		Ogre::String getAffectedCategories(void);
	
		/**
		* @brief Sets the explosion timer in seconds. The timer starts to count down, when the component is activated.
		* @param countDownMS	The time in seconds to set.
		*/
		void setExplosionCountDownSec(Ogre::Real countDown);

		/**
		* @brief Gets the spawn interval in seconds.
		* @return explosionCountDown The interval in seconds
		*/
		Ogre::Real getExplosionCountDownSec(void) const;

		/**
		* @brief Sets the explosion radius in meters.
		* @param explosionRadius The explosion radius to set
		* @note	The explosion radius is important. Game object that are within this radius will be affected by the explosion.
		*/
		void setExplosionRadius(Ogre::Real radius);

		/**
		* @brief Gets the explosion radius.
		* @return radius The explosion radius to get
		*/
		Ogre::Real getExplosionRadius(void) const;

		/**
		* @brief Sets the explosion strength in newton.
		* @param explosionStrengthNM	The explosion strength to set.
		* @note	The given explosion strength is a maximal value. The far away an affected game object is away from the explosion center the weaker the detonation.
		*/
		void setExplosionStrengthN(unsigned int strength);

		/**
		* @brief Gets the explosion strength in newton.
		* @return strength The explosion strength in newton
		*/
		unsigned int getExplosionStrengthN(void) const;

		/*
		* @brief Sets the explosion callback to react at the moment when a game object detonates.
		* @param[in]	explosionCallback	The explosion callback
		* @note	A newly created explosion callback heap pointer must be passed for the IExplosionCallback. It will be deleted internally,
		*		if the object is not required anymore.
		*/
		void setExplosionCallback(IExplosionCallback* explosionCallback);

		/*
		* @brief Sets the explosion callback script file to react in script at the moment when a game object detonates.
		* @param[in]	explosionCallbackScriptFile	The explosion callback script file
		* @note			The file must have the following functions implemented and the module name must be called like the lua file without the lua extension. E.g. 'Explosion.lua':
		*	module("Explosion", package.seeall); 
		*	function onTimerSecondTick(originGameObject)
		*	end
		*
		*	function onExplode(originGameObject)
		*	end
		*
		*	function onExplodeAffectedGameObject(originGameObject, affectedGameObject, distanceToBomb, detonationStrength)
		*	end
		*/
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrCountDown(void) { return "Count Down (sec)"; }
		static const Ogre::String AttrRadius(void) { return "Radius"; }
		static const Ogre::String AttrStrength(void) { return "Strength (n)"; }
		static const Ogre::String AttrCategories(void) { return "Categories"; }
	protected:
		Variant* activated;
		Variant* countDown;
		Variant* radius;
		Variant* strength;

		IExplosionCallback* explosionCallback;
		// also a mask?
		Ogre::SphereSceneQuery* sphereSceneQuery;
		
		unsigned int categoryIds;
		Variant* categories;
		Ogre::Real countDownTimer;
		Ogre::Real secondsUpdateTimer;
		std::vector<GameObject*> affectedGameObjects;
	};

}; //namespace end

#endif