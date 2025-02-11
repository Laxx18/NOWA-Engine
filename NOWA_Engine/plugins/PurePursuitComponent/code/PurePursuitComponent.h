/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PUREPURSUITCOMPONENT_H
#define PUREPURSUITCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

#include "ki/Path.h"

namespace NOWA
{

	class PhysicsActiveVehicleComponent;

	/**
	  * @brief		To calculate the steering angle of a car moving along waypoints, we can use the Pure Pursuit algorithm. This algorithm is a simple yet effective method for path tracking in autonomous vehicles. 
					The Pure Pursuit algorithm calculates the necessary steering angle to drive the vehicle towards a lookahead point on the path.
	  */
	class EXPORTED PurePursuitComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<PurePursuitComponent> PurePursuitComponentPtr;
	public:

		PurePursuitComponent();

		virtual ~PurePursuitComponent();

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

		void setManuallyControlled(bool manuallyControlled);

		bool getIsManuallyControlled(void) const;

		void setWaypointsCount(unsigned int waypointsCount);

		unsigned int getWaypointsCount(void) const;

		void setWaypointId(unsigned int index, unsigned long id);

		void setWaypointStrId(unsigned int index, const Ogre::String& id);

		void addWaypointId(unsigned long id);

		void addWaypointStrId(const Ogre::String& id);

		void removeWaypointStrId(const Ogre::String& id);

		void prependWaypointStrId(int index, const Ogre::String& id);

		unsigned long getWaypointId(unsigned int index);

		Ogre::String getWaypointStrId(unsigned int index);

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setLookaheadDistance(Ogre::Real lookaheadDistance);

		Ogre::Real getLookaheadDistance(void) const;

		void setCurvatureThreshold(Ogre::Real curvatureThreshold);

		Ogre::Real getCurvatureThreshold(void) const;

		void setMaxMotorForce(Ogre::Real maxMotorForce);

		Ogre::Real getMaxMotorForce(void) const;

		void setMotorForceVariance(Ogre::Real motorForceVariance);

		Ogre::Real getMotorForceVariance(void) const;

		void setOvertakingMotorForce(Ogre::Real overtakingMotorForce);

		Ogre::Real getOvertakingMotorForce(void) const;
		
		void setMinMaxSteerAngle(const Ogre::Vector2& minMaxSteerAngle);

		Ogre::Vector2 getMinMaxSteerAngle(void) const;

		void setCheckWaypointY(bool checkWaypointY);

		bool getCheckWaypointY(void) const;

		void setWaypointVariance(Ogre::Real waypointVariance);

		Ogre::Real getWaypointVariance(void) const;

		void setVarianceIndividual(bool varianceIndividual);

		bool getVarianceIndividual(void) const;

		void setObstacleCategory(const Ogre::String& obstacleCategory);

		Ogre::String getObstacleCategory(void) const;

		void setRammingBehavior(bool rammingBehavior);

		bool getRammingBehavior(void) const;

		void setOvertakingBehavior(bool overtakingBehavior);

		bool getOvertakingBehavior(void) const;

		Ogre::Real calculateSteeringAngle(const Ogre::Vector3& lookaheadPoint);

		Ogre::Real calculatePitchAngle(const Ogre::Vector3& lookaheadPoint);

		/**
		 * @brief Adapts the motor force based on the detected curvature. Internally determines the curvature of the path using the change in direction between consecutive waypoints.
		 * @return motorForce	The adjusted motor force.
		 */
		Ogre::Real calculateMotorForce(void);

		void reorderWaypoints(void);

		Ogre::Real getMotorForce(void) const;

		Ogre::Real getSteerAmount(void) const;

		Ogre::Real getPitchAmount(void) const;

		int getCurrentWaypointIndex(void);

		Ogre::Vector3 getCurrentWaypoint(void) const;

		NOWA::KI::Path* getPath(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PurePursuitComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "PurePursuitComponent";
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
			return "Usage: To calculate the steering angle of a car moving along waypoints, we can use the Pure Pursuit algorithm. This algorithm is a simple yet effective method for path tracking in autonomous vehicles. "
				"The Pure Pursuit algorithm calculates the necessary steering angle to drive the vehicle towards a lookahead point on the path. "
				"Requirements: A LuaScriptComponent, because calculateSteeringAngle and/or calculatePitchAngle shall be called in script to get the steering/pitch results."
				"Note: Check the orientation and global mesh direction of the checkpoints, in order to get correct results, if the manually controlled vehicle is driving in the correct direction. If the RaceGoalComponent is used."
				"Since there are a lot of waypoints necessary and setting than manually may be disturbing, hence this function will help. You can put this peace of code in the lua console, which can be opened via ^-key. Adapt the id and set proper string quotes."
				"local thisGameObject = AppStateManager:getGameObjectController():getGameObjectFromId('528669575'); "
				"local purePursuitComp = thisGameObject:getPurePursuitComponent(); "
				"local nodeGameObjects = AppStateManager:getGameObjectController():getGameObjectsFromComponent('NodeComponent'); "
				"purePursuitComp:setWaypointsCount(#nodeGameObjects + 1); "
				"for i = 0, #nodeGameObjects do "
				"   local gameObject = nodeGameObjects[i]; "
				"   purePursuitComp:setWaypointId(i, gameObject:getId()); "
				"end "
				"purePursuitComp:reorderWaypoints(); ";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrManuallyControlled(void) { return "Manually Controlled"; }
		
		static const Ogre::String AttrWaypointsCount(void) { return "Waypoints Count"; }
		static const Ogre::String AttrWaypoint(void) { return "Waypoint Id "; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrLookaheadDistance(void) { return "Lookahead Distance"; }
		static const Ogre::String AttrCurvatureThreshold(void) { return "Curvature Threshold"; }
		static const Ogre::String AttrMaxMotorForce(void) { return "Max. Motorforce"; }
		static const Ogre::String AttrMotorForceVariance(void) { return "Motorforce Variance"; }
		static const Ogre::String AttrOvertakingMotorForce(void) { return "Overtaking Motorforce"; }
		static const Ogre::String AttrMinMaxSteerAngle(void) { return "Min. Max. Steerangle"; }
		static const Ogre::String AttrCheckWaypointY(void) { return "Check Waypoint Y"; }
		static const Ogre::String AttrWaypointVariance(void) { return "Waypoint Variance"; }
		static const Ogre::String AttrVarianceIndividual(void) { return "Variance Individual"; }
		static const Ogre::String AttrObstacleCategory(void) { return "Obstacle Category"; }
		static const Ogre::String AttrRammingBehavior(void) { return "Ramming Behavior"; }
		static const Ogre::String AttrOvertakingBehavior(void) { return "Overtaking Behavior"; }
	private:
		Ogre::Real normalizeAngle(Ogre::Real angle);

		/**
		* @brief Determines the curvature of the path using the change in direction between consecutive waypoints.
		*/
		Ogre::Real calculateCurvature(void);

		void highlightWaypoint(const Ogre::String& datablockName);

		void addRandomVarianceToWaypoints(void);

		void handleGameObjectsToClose();

		bool detectObstacle(Ogre::Real detectionRange);
		
		void adjustPathAroundObstacle(Ogre::Vector3& lookaheadPoint);
	private:
		void handleCountdownActive(NOWA::EventDataPtr eventData);
	private:
		Ogre::String name;
		Ogre::Real motorForce;
		Ogre::Real steerAmount;
		Ogre::Real pitchAmount;

		Variant* activated;
		Variant* manuallyControlled;
		Variant* waypointsCount;
		std::vector<Variant*> waypoints;
		Variant* repeat;
		Variant* lookaheadDistance;
		Variant* curvatureThreshold;
		Variant* maxMotorForce;
		Variant* motorForceVariance;
		Variant* minMaxSteerAngle;
		Variant* checkWaypointY;
		Variant* waypointVariance;
		Variant* varianceIndividual;
		Variant* overtakingMotorForce;
		Variant* obstacleCategory;
		Variant* rammingBehavior;
		Variant* overtakingBehavior;

		Ogre::Real lastSteerAngle;
		Ogre::Real lastMotorForce;
		bool firstTimeSet;
		Path* pPath;
		std::pair<bool, Ogre::Vector3> currentWaypoint;
		Ogre::Real previousDistance;
		Ogre::Vector3 previousPosition;
		Ogre::Real stuckTime;
		Ogre::Real maxStuckTime;
		Ogre::String oldDatablockName;
		unsigned int categoriesId;
		bool canSwitchBehavior;
		bool canDrive;
		PhysicsActiveVehicleComponent* vehicleComponent;

		enum BehaviorType { NONE, RAMMING, OVERTAKING } behaviorType;

		std::vector<PurePursuitComponent*> otherPurePursuits;
		std::vector<GameObject*> obstacles;
	};

}; // namespace end

#endif

