/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
*/

#ifndef CROWDCOMPONENT_H
#define CROWDCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

class OgreDetourCrowd;
class OgreDetourTileCache;
struct dtCrowdAgent;

namespace NOWA
{
	class PhysicsActiveComponent;

	/**
	  * @brief		This component is used, in order to tag this game object, that it will be part of OgreRecast crowd system.
	  *				Its possible to move an army of game objects as crowd.
	  */
	class EXPORTED CrowdComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<CrowdComponent> CrowdComponentPtr;
	public:

		CrowdComponent();

		virtual ~CrowdComponent();

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
		  * The height of the agent for this character.
		  **/
		Ogre::Real getAgentHeight(void) const;

		/**
		  * The radius of the agent for this character.
		  **/
		Ogre::Real getAgentRadius(void) const;

		/**
		  * Update the destination for this agent.
		  * If updatePreviousPath is set to true the previous path will be reused instead
		  * of calculating a completely new path, but this can only be used if the new
		  * destination is close to the previous (eg. when chasing a moving entity).
		  **/
		void updateDestination(Ogre::Vector3 destination, bool updatePreviousPath = false);

		/**
		  * Update current position of this character to the current position of its agent.
		  **/
		Ogre::Vector3 beginUpdateVelocity(void);

		void endUpdateVelocity(void);

		/**
		  * Set destination member variable directly without updating the agent state.
		  * Usually you should call updateDestination() externally, unless you are controlling
		  * the agents directly and need to update the corresponding character class to reflect
		  * the change in state (see OgreRecastApplication friendship).
		  **/
		void setDestination(const Ogre::Vector3& destination);

		/**
		 * The destination set for this character.
		 **/
		Ogre::Vector3 getDestination(void) const;

		/**
		  * Place character at new position.
		  * The character will start following the globally set destination in the detourCrowd,
		  * unless you give it an individual destination using updateDestination().
		  **/
		void setPosition(Ogre::Vector3& position);

		/**
		  * Returns true when this character has reached its set destination.
		  **/
		bool destinationReached(void);

		/**
		  * Request to set a manual velocity for this character, to control it
		  * manually.
		  * The set velocity will stay active, meaning that the character will
		  * keep moving in the set direction, until you stop() it.
		  * Manually controlling a character offers no absolute control as the
		  * laws of acceleration and max speed still apply to an agent, as well
		  * as the fact that it cannot steer off the navmesh or into other agents.
		  * You will notice small corrections to steering when walking into objects
		  * or the border of the navmesh (which is usually a wall or object).
		  **/
		void setVelocity(Ogre::Vector3 velocity);

		/**
		  * Stop any movement this character is currently doing. This means losing
		  * the requested velocity or target destination.
		  **/
		void stop(void);

		/**
		  * The current velocity (speed and direction) this character is traveling
		  * at.
		  **/
		Ogre::Vector3 getVelocity(void);

		/**
		  * The current speed this character is traveling at.
		  **/
		Ogre::Real getSpeed(void);

		/**
		  * The maximum speed this character can attain.
		  * This parameter is configured for the agent controlling this character.
		  **/
		Ogre::Real getMaxSpeed(void);

		/**
		  * The maximum acceleration this character has towards its maximum speed.
		  * This parameter is configured for the agent controlling this character.
		  **/
		Ogre::Real getMaxAcceleration(void);

		/**
		  * Returns true if this character is moving.
		  **/
		bool isMoving(void);

		/**
		  * Set whether this character is controlled by an agent or whether it
		  * will position itself independently based on the requested velocity.
		  * Set to true to let the character be controlled by an agent.
		  * Set to false to manually control it without agent, you need to set
		  * detourTileCache first.
		  **/
		void setControlled(bool controlled);

		/**
		  * Determines whether this character is controlled by an agent.
		  **/
		bool isControlled(void) const;

		bool isActive(void) const;

		void setGoalRadius(Ogre::Real goalRadius);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CrowdComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "CrowdComponent";
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
			return "Usage: This component is used, in order to tag this game object, that it will be part of OgreRecast crowd system. Its possible to move an army of game objects as crowd.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrManuallyControlled(void) { return "Manually Controlled"; }
	private:
		OgreDetourCrowd* detourCrowd;
		OgreDetourTileCache* detourTileCache;
		// Id of agent within the crowd
		int agentId;
		// The agent controlling this character
		const dtCrowdAgent* agent;
		PhysicsActiveComponent* physicsActiveComponent;
		Ogre::Vector3 destination;
		/**
		 * Obstacle used when character is not being controlled by agent.
		 * The temp obstacle is placed on the current position of this character
		 * so that other agents in the crowd will not walk through it.
		 **/
		unsigned int tempObstacle;

		/**
		 * Velocity set for this agent for manually controlling it.
		 * If this is not zero then a manually set velocity is currently controlling the movement
		 * of this character (not pathplanning towards a set destination).
		 **/
		Ogre::Vector3 manualVelocity;

		bool stopped;

		bool inSimulation;

		Ogre::Real goalRadius;

		Variant* activated;
		Variant* controlled;
	};

}; // namespace end

#endif

