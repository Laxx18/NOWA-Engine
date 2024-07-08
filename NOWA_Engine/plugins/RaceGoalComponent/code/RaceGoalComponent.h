/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef RACEGOALCOMPONENT_H
#define RACEGOALCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

#include "ki/Path.h"

class PhysicsActiveKinematicComponent;
class PhysicsActiveComponent;

namespace NOWA
{

	/**
	  * @brief		Can be used for a player controlled vehicle against cheating :). That is create several checkpoints with the given
	  *				race direction, which the player must pass in the correct direction and also counts the laps.
	  */
	class EXPORTED RaceGoalComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<RaceGoalComponent> RaceGoalComponentPtr;
	public:

		RaceGoalComponent();

		virtual ~RaceGoalComponent();

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setCheckpointsCount(unsigned int waypointsCount);

		unsigned int getCheckpointsCount(void) const;

		void setCheckpointId(unsigned int index, unsigned long id);

		void setLapsCount(unsigned int lapsCount);

		unsigned int getLapsCount(void) const;

		bool getIsFinished(void) const;

		Ogre::Real getSpeedInKmh(void) const;

		/**
		 * @brief Sets the lua function name, to react when a game object collided with another game object.
		 * @param[in]	onFeedbackRaceFunctionName		The function name to set
		 */
		void setOnFeedbackRaceFunctionName(const Ogre::String& onFeedbackRaceFunctionName);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("RaceGoalComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "RaceGoalComponent";
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
			return "Usage: Can be used for a player controlled vehicle against cheating :). That is create several checkpoints with the given race direction, which the player must pass in the correct direction and also counts the laps. "
				"Requirements: Checkpoints must possess a PhysicsActiveKinematicComponent. The Checkpoint should be a wall, which is big enough, so that the vehicle will pass the wall in any case (even flying over a ramp :))";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrCheckpointsCount(void) { return "Checkpoints Count"; }
		static const Ogre::String AttrCheckpoint(void) { return "Checkpoint Id "; }
		static const Ogre::String AttrLapsCount(void) { return "Laps Count"; }
		static const Ogre::String AttrOnFeedbackRaceFunctionName(void) { return "On Feedback Race Function Name"; }
	private:
		bool isMovingTowardsCheckpoint(void);

		Ogre::Real calculateSpeedInKmh(void);
	private:
		Ogre::String name;

		Variant* checkpointsCount;
		Variant* onFeedbackRaceFunctionName;
		std::vector<Variant*> checkpoints;
		std::vector<PhysicsActiveKinematicComponent*> kinematicComponents;
		PhysicsActiveComponent* physicsActiveComponent;

		Variant* lapsCount;
		bool finished;
		unsigned int currentLap;
		bool wrongDirection;
		bool oldDirection;
		Ogre::Real lapTimeSec;
		KI::Path* pPath;
		Ogre::Vector3 checkpointDefaultDirection;
		std::pair<bool, std::pair<Ogre::Vector3, Ogre::Quaternion>> currentCheckpoint;
		Ogre::Real speedInKmh;
	};

}; // namespace end

#endif

