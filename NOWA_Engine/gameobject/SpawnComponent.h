#ifndef SPAWN_COMPONENT_H
#define SPAWN_COMPONENT_H

#include "GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{
	class PhysicsComponent;
	class LuaScript;

	/**
	 * @class 	SpawnComponent
	 * @brief 	This component can be used to spawn other game objects and control their life cycle.
	 *			Info: Only one component can be attached to the owner game object.
	 *			Example: A cannon that shots bullets.
	 *			Requirements: None
	 * 			Lua Interface Functions when LuaScriptComponent is involved: 
	 *				onSpawn(clonedGameObject, ownerGameObject)
	 *				onVanish(clonedGameObject, ownerGameObject)
	 */
	class EXPORTED SpawnComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<SpawnComponent> SpawnCompPtr;
		typedef boost::shared_ptr<PhysicsComponent> PhysicsCompPtr;
	public:
		/**
		* @class ISpawnObserver
		* @brief This interface can be implemented to react each time a game object will be spawned, e.g. to add an particle effect
		*/
		class EXPORTED ISpawnObserver
		{
		public:
			virtual ~ISpawnObserver()
			{
			}

			/**
			* @brief		Called when a game object gets spawned
			* @param[in]	spawnedGameObject	The game object to react on
			* @param[in]	originGameObject	The origin game object from which the spawning does come from
			* @Note			It could be useful e.g. to attach a simple sound component to the origin game object
			*				and when the callback is called, get those component and play the sound
			*/
			virtual void onSpawn(NOWA::GameObject* spawnedGameObject, NOWA::GameObject* originGameObject) = 0;

			/**
			* @brief		Called when a game object's life time is at its end and it will be deleted
			* @param[in]	spawnedGameObject	The game object to react on
			* @param[in]	originGameObject	The origin game object from which the spawning does come from
			* @Note			Do not manipulate the game object since it will be deleted shortly. So this is for read only, like last position to create a particle effect etc.
			*/
			virtual void onVanish(NOWA::GameObject* spawnedGameObject, NOWA::GameObject* originGameObject) = 0;
		};
	public:
		SpawnComponent();

		virtual ~SpawnComponent();

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
			return NOWA::getIdFromName("SpawnComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "SpawnComponent";
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
			return "Usage: With this component other game objects are spawned. E.g. creating a cannon that throws gold coins :). Note: "
				"If this game object contains a @LuaScriptComponent, its possible to react in script at the moment when a game object is spawned: function onSpawn(spawnedGameObject, originGameObject) or function onVanish(spawnedGameObject, originGameObject). "
				"Also note: Think carefully, which components a to be cloned game object does possess. E.g. if each game object has a @SimpleSoundComponent, it will be cloned each time, which may cause frame drops. "
				"So maybe, since its the sound is always the same, use the @MainGameObject and play each time the sound from there";
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
		 * @brief Activates the components behaviour, so that the spawning will begin.
		 * @param[in] activated Whether to activate or deactivate the spawning
		 */
		virtual void setActivated(bool activated) override;

		/**
		 * @brief Gets whether the component behaviour is activated or not.
		 * @return activated True, if the components behaviour is activated, else false
		 */
		virtual bool isActivated(void) const override;

		/**
		* @brief Sets the spawn interval in milliseconds.
		* @param[in] interval The interval in milliseconds to set.
		* @note	E.g. an interval of 10000 would spawn a game object each 10 seconds.
		*/
		void setIntervalMS(unsigned int intervalMS);

		/**
		* @brief Gets the spawn interval in milliseconds.
		* @return interval The interval in milliseconds
		*/
		unsigned int getIntervalMS(void) const;

		/**
		* @brief Sets the spawn count.
		* @param[in] count The spawn count to set
		* @note	E.g. a count of 100 would spawn 100 game objects of this type. When the count is set to 0, the spawning will never stop.
		*/
		void setCount(unsigned int count);

		/**
		* @brief Gets the spawn count.
		* @return count The spawn count to get
		*/
		unsigned int getCount(void) const;

		/**
		* @brief Sets the spawned game object life time in milliseconds. When the life time is set to 0, the spawned gameobject will live forever.
		* @param[in] lifeTimeMs The life time in milliseconds to set.
		* @note	E.g. a life time of 10000 would let live the spawned game object for 10 seconds.
		*/
		void setLifeTimeMS(unsigned int lifeTimeMS);

		/**
		* @brief Gets the life time in milliseconds.
		* @return lifeTime The life time in milliseconds
		*/
		unsigned int getLifeTimeMS(void) const;

		/**
		* @brief Sets the offset position at which the spawned game objects will occur relative to the origin game object.
		* @param[in] offsetPosition The offset position to set
		*/
		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		/**
		* @brief Gets the offset position of the to be spawned game objects.
		* @return offsetPosition The offset position to get
		*/
		const Ogre::Vector3 getOffsetPosition(void) const;

		/**
		 * @brief Sets an offset orientation at which the spawned game objects will occur relative to the origin game object.
		 * @param[in] offsetOrientation The offset orientation to set (degreeX, degreeY, degreeZ)
		 */
		void setOffsetOrientation(const Ogre::Vector3& offsetOrientation);

		/**
		 * @brief Gets an offset orientation at which the spawned game objects will occur relative to the origin game object.
		 * @return offsetOrientation The offset orientation to get (degreeX, degreeY, degreeZ)
		 */
		Ogre::Vector3 getOffsetOrientation(void) const;

		/**
		* @brief Sets whether the spawn new game objects always at the init position or the current position of the original game object.
		* @param[in] spawnAtOrigin The spawn at origin to set.
		* @note	If spawn at origin is set to false. New game objects will be spawned at the current position of the original game object. Thus complex scenarios are possible!
		*		E.g. pin the original active physical game object at a rotating block. When spawning a new game object implement the spawn callback and add for the spawned game object
		*		an impulse at the current direction. Spawned game objects will be catapultated in all directions while the original game object is being rotated!
		*/
		void setSpawnAtOrigin(bool spawnAtOrigin);

		/**
		* @brief Gets whether the game objects are spawned at initial position and orientation of the original game object.
		* @return spawnAtOrigin True if the game object are spawned at origin, else false
		*/
		bool isSpawnedAtOrigin(void) const;

		/**
		* @brief Sets spawn target game object id.
		* @param[in] spawnTargetId The spawn target id to set.
		* @note	There can also another game object name (scenenode name in Ogitor) be specified which should be spawned
		*		at the position of this game object. E.g. a rotating case shall spawn coins, so the case has the spawn component and as spawn target the coin scene node name.
		*/
		void setSpawnTargetId(unsigned long spawnTargetId);

		/**
		* @brief Gets the spawn target game object id
		* @return spawnTargetId The spawn target id to get
		*/
		unsigned long getSpawnTargetId(void) const;

		/**
		 * @brief Sets whether to clone a potential datablock, or use the just the same from the original one. This could be taken into account due to performance reasons.
		 * @param[in] cloneDatablock The cloneDatablock flag to set.
		 */
		void setCloneDatablock(bool cloneDatablock);

		/**
		* @brief Gets whether a potential datablock is cloned, or the same from the original game object used.
		* @return spawnTargetId The spawn target id to get
		*/
		bool getCloneDatablock(void) const;

		/**
		* @brief Sets the spawn observer to react at the moment when a game object has been spawned.
		* @param[in]	spawnObserver	The spawn observer
		* @note	A newly created spawn observer heap pointer must be passed for the ISpawnObserver. It will be deleted internally,
		*		if the object is not required anymore.
		*/
		void setSpawnObserver(ISpawnObserver* spawnObserver);

		/**
		 * @brief Sets whether keep alive spawned game objects, when the component has ben de-activated and re-activated again.
		 * @param[in]	keepAlive	The flag
		 *		
		 */
		void setKeepAliveSpawnedGameObjects(bool keepAlive);

		/**
		 * @brief Lua closure function gets called in order to react, when a game object has been spawned.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnSpawn(luabind::object closureFunction);

		/**
		 * @brief Lua closure function gets called in order to react, when a game object has been vanished.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnVanish(luabind::object closureFunction);

		/**
		* @brief Sets the spawn callback script file to react in script at the moment when a game object is spawned.
		* @param[in]	spawnScriptFile	The spawn callback script file
		* @note			The file must have the following functions implemented and the module name must be called like the lua file without the lua extension. E.g. 'Spawn.lua':
		*	module("Spawn", package.seeall);
		*	function onSpawn(spawnedGameObject, originGameObject)
		*	end
		*
		*	function onVanish(spawnedGameObject, originGameObject)
		*	end
		*
		*/
	public:
			// Attribute constants
			static const Ogre::String AttrActivated(void) { return "Activated"; }
			static const Ogre::String AttrInterval(void) { return "Interval (ms)"; }
			static const Ogre::String AttrCount(void) { return "Count"; }
			static const Ogre::String AttrLifeTime(void) { return "Lifetime (ms)"; }
			static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
			static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orientation"; }
			static const Ogre::String AttrSpawnAtOrigin(void) { return "Spawn At Origin"; }
			static const Ogre::String AttrSpawnTargetId(void) { return "Spawn Target Id"; }
			static const Ogre::String AttrCloneDatablock(void) { return "Clone Datablock"; }
			static const Ogre::String AttrOnSpawnFunctionName(void) { return "On Spawn Function Name"; }
			static const Ogre::String AttrOnVanishFunctionName(void) { return "On Vanish Function Name"; }
	private:
		void reset(void);

		bool initSpawnData(void);

		void deleteGameObjectDelegate(EventDataPtr eventData);
	protected:
		Variant* activated;
		Variant* interval;
		Variant* count;
		Variant* lifeTime;
		Variant* offsetPosition;
		Variant* offsetOrientation;
		Variant* spawnAtOrigin;
		Variant* spawnTargetId;
		Variant* cloneDatablock;

		Ogre::Vector3 initPosition;
		Ogre::Quaternion initOrientation;
		GameObject* spawnTargetGameObject;
		bool firstTimeSetSpawnTarget;
		PhysicsCompPtr physicsCompPtr;
		unsigned int currentCount;
		Ogre::Real spawnTimer;
		std::list<std::pair<unsigned long, GameObject*>> lifeTimeQueue;
		std::vector<GameObject*> clonedGameObjectsInScene;
		bool keepAlive;

		ISpawnObserver* spawnObserver;

		luabind::object spawnClosureFunction;
		luabind::object vanishClosureFunction;
	};

}; //namespace end

#endif