/*
Copyright (c) 2023 Lukas Kalinowski

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

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

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

		void setMinMaxSteerAngle(const Ogre::Vector2& minMaxSteerAngle);

		Ogre::Vector2 getMinMaxSteerAngle(void) const;

		void setCheckWaypointY(bool checkWaypointY);

		bool getCheckWaypointY(void) const;

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
		static const Ogre::String AttrWaypointsCount(void) { return "Waypoints Count"; }
		static const Ogre::String AttrWaypoint(void) { return "Waypoint Id "; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrLookaheadDistance(void) { return "Lookahead Distance"; }
		static const Ogre::String AttrCurvatureThreshold(void) { return "Curvature Threshold"; }
		static const Ogre::String AttrMaxMotorForce(void) { return "Max. Motorforce"; }
		static const Ogre::String AttrMinMaxSteerAngle(void) { return "Min. Max. Steerangle"; }
		static const Ogre::String AttrCheckWaypointY(void) { return "Check Waypoint Y"; }
		
	private:
		Ogre::Vector3 findLookaheadPoint(const Ogre::Vector3& position);

		Ogre::Real normalizeAngle(Ogre::Real angle);

		/**
		* @brief Determines the curvature of the path using the change in direction between consecutive waypoints.
		*/
		Ogre::Real calculateCurvature(void);

		void highlightWayPoint(void);
		
	private:
		Ogre::String name;
		Ogre::Real motorForce;
		Ogre::Real steerAmount;
		Ogre::Real pitchAmount;

		Variant* activated;
		Variant* waypointsCount;
		std::vector<Variant*> waypoints;
		Variant* repeat;
		Variant* lookaheadDistance;
		Variant* curvatureThreshold;
		Variant* maxMotorForce;
		Variant* minMaxSteerAngle;
		Variant* checkWaypointY;
		Ogre::Real lastSteerAngle;
		bool firstTimeSet;
		Path* pPath;
		std::pair<bool, Ogre::Vector3> currentWaypoint;
		Ogre::Real previousDistance;
		Ogre::Vector3 previousPosition;
		Ogre::Real stuckTime;
		Ogre::Real maxStuckTime;
	};

}; // namespace end

#endif

