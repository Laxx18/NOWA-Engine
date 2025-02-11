/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef TRANSFORMHISTORYCOMPONENT_H
#define TRANSFORMHISTORYCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "main/Events.h"
#include "network/GameObjectStateHistory.h"

class PhysicsActiveComponent;
class PhysicsActiveKinematicComponent;

namespace NOWA
{

	/**
	  * @brief		Your documentation
	  */
	class EXPORTED TransformHistoryComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<TransformHistoryComponent> TransformHistoryComponentPtr;
	public:

		TransformHistoryComponent();

		virtual ~TransformHistoryComponent();

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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

		/**
		 * @brief Sets target id for the game object, which shall be linear interpolated.
		 * @param[in] targetId The target id to set
		 */
		void setTargetId(unsigned long targetId);

		/**
		 * @brief Gets the target id for the game object, which is linear interpolated.
		 * @return targetId The target id to get
		 */
		unsigned long getTargetId(void) const;

		/**
		 * @brief Gets the target game object for the given target id, which shall be linear interpolated.
		 * @return targetId The target game object to get, or null if does not exist.
		 */
		GameObject* getTargetGameObject(void) const;

		/**
		 * @brief Controls how long the history is and how often a value is saved, setHistoryLength(10) means that a value is saved every 100 ms and therefore 10 values in one second.
		 * @note	Calculated history size, this must be large enough even in the case of high latency and thus a high past value, have enough saved values available for interpolation.
		 * @param[in] historyLength The historyLength to set
		 */
		void setHistoryLength(unsigned int historyLength);

		/**
		 * @brief Gets the history length.
		 * @return historyLength The historyLength to get
		 */
		unsigned int getHistoryLength(void) const;

		/**
		 * @brief Sets how much the target game object is transformed in the past of the given game object.
		 * @param[in] pastTimeMS The past time in milliseconds to set
		 */
		void setPastTime(unsigned int pastTimeMS);

		/**
		 * @brief Gets how much the target game object is transformed in the past of the given game object.
		 * @return pastTimeMS The past time in milliseconds to get
		 */
		unsigned int getPastTime(void) const;

		/**
		 * @brief Sets whether the target game object also shall be orientated besides movement.
		 * @param[in] orientate True, if the target game object shall be orientated, else false.
		 */
		void setOrientate(bool orientate);

		/**
		 * @brief Gets whether the target game object also shall be orientated besides movement.
		 * @return orientate True, if the target game object shall be orientated, else false.
		 */
		bool getOrientate(void) const;

		/**
		 * @brief Sets whether to use delay on transform. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.
		 * @param[in] useDelay True, if delay shall be used, else false.
		 */
		void setUseDelay(bool useDelay);

		/**
		 * @brief Gets whether delay on transform is used. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.
		 * @return useDelay True,  if delay shall be used, else false.
		 */
		bool getUseDelay(void) const;

		/**
		 * @brief Sets whether the target game object shall stop its interpolation immediately, if this game object stops transform.
		 * @param[in] stopImmediately True, if the target game object shall stop its interpolation immediately, else false.
		 */
		void setStopImmediately(bool stopImmediately);

		/**
		 * @brief Gets whether the target game object stops its interpolation immediately, if this game object stops transform.
		 * @return orientate True, if the target game object stops its interpolation immediatelyd, else false.
		 */
		bool getStopImmediately(void) const;
	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("TransformHistoryComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "TransformHistoryComponent";
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
			return "Usage: This component can be used to linear interpolate a target game object from to the past transform of this game object. The schema is as follows:"
				 "()Go3<--target--()Go2<--target--()Go1. The game object1 is the root and has the game object2 as target id, the game object 2 can also have this component and have game object 3 as target id, and so one, so Go3 will follow Go2 and Go2 will follow Go1.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrHistoryLength(void) { return "History Length"; }
		static const Ogre::String AttrPastTime(void) { return "Past Time"; }
		static const Ogre::String AttrOrientate(void) { return "Orientate"; }
		static const Ogre::String AttrUseDelay(void) { return "Use Delay"; }
		static const Ogre::String AttrStopImmediately(void) { return "Stop Immediately"; }
	private:
		void handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData);
		Ogre::Vector3 seek(PhysicsActiveComponent* physicsActiveComponent, Ogre::Vector3 targetPosition, Ogre::Real dt);
		Ogre::Vector3 arrive(PhysicsActiveComponent* physicsActiveComponent, Ogre::Vector3 targetPosition, Ogre::Real dt);
	private:
		Ogre::String name;
		GameObject* targetGameObject;
		PhysicsActiveComponent* physicsActiveComponent;
		PhysicsActiveKinematicComponent* physicsActiveKinematicComponent;
		bool isTransforming;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		GameObjectStateHistory* gameObjectStateHistory;

		Variant* activated;
		Variant* targetId;
		Variant* historyLength;
		Variant* pastTime;
		Variant* orientate;
		Variant* stopImmediately;
		Variant* useDelay;
	};

}; // namespace end

#endif

