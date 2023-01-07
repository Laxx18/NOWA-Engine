#ifndef GAME_OBJECT_GUIDE_H
#define GAME_OBJECT_GUIDE_H

#include "gameobject/GameObject.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/AttributesComponent.h"
#include "gameobject/PhysicsTerrainComponent.h"
#include "gameobject/PhysicsRagDollComponent.h"
#include "ki/StateMachine.h"
#include "utilities/AnimationBlender.h"

namespace NOWA
{
	typedef boost::shared_ptr<GameObject> GameObjectPtr;
	typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;
	typedef boost::shared_ptr<AttributesComponent>AttributesCompPtr;
	typedef boost::shared_ptr<PhysicsArtifactComponent> PhysicsArtifactCompPtr;
	// typedef boost::shared_ptr<PhysicsTerrainComponent> PhysicsTerrainCompPtr;
	typedef boost::shared_ptr<PhysicsRagDollComponent> PhysicsRagDollCompPtr;

	template<class Owner>
	class EXPORTED GameObjectGuide
	{
		
	public:
		GameObjectGuide(const Ogre::Vector3& initialOrientationAxis, const Ogre::String& currentAppStateName)
			: initialOrientationAxis(initialOrientationAxis),
			currentAppStateName(currentAppStateName),
			animationBlender(nullptr),
			stateMachine(nullptr),
			moveWeight(0.0f),
			jumpWeight(0.0f),
			idle(true),
			canMove(true),
			canJump(false)
		{
		
		};

		virtual ~GameObjectGuide(void)
		{
			if (this->stateMachine)
			{
				delete this->stateMachine;
				this->stateMachine = nullptr;
			}
			if (animationBlender)
			{
				delete animationBlender;
				animationBlender = nullptr;
			}
		};

		virtual bool init(void) = 0;

		virtual void update(Ogre::Real dt) = 0;

		virtual void handleKeyInput(int keyCode, bool down)
		{

		}
		
		virtual void handleMouseInput(int mouseButton, bool down, int x, int y, int z = 0)
		{

		}

		virtual bool handleEvent(const Event& ev)
		{
			return false;
		}

		GameObject* getGameObject(void) const
		{
			return this->gameObjectPtr.get();
		}

		AnimationBlender* getAnimationBlender(void) const
		{
			return this->animationBlender;
		}

		PhysicsActiveComponent* getPhysicsComponent(void) const
		{
			return this->physicsCompPtr.get();
		}

		KI::StateMachine<Owner>* getStateMaschine(void) const
		{
			return this->stateMachine;
		}

		AttributesComponent* getAttributesComponent(void) const
		{
			return this->attributesCompPtr.get();
		}

		Ogre::Vector3 getInitialOrientationAxis(void) const
		{
			return this->initialOrientationAxis;
		}

		void setMoveWeight(Ogre::Real moveWeight)
		{
			this->moveWeight = moveWeight;
		}

		Ogre::Real getMoveWeight(void) const
		{
			return this->moveWeight;
		}

		void setJumpWeight(Ogre::Real jumpWeight)
		{
			this->jumpWeight = jumpWeight;
		}

		Ogre::Real getJumpWeight(void) const
		{
			return this->jumpWeight;
		}

		void setIdle(bool idle)
		{
			this->idle = idle;
		}

		bool isIdle(void) const
		{
			return this->idle;
		}

		void setCanMove(bool canMove)
		{
			this->canMove = canMove;
		}

		bool getCanMove(void) const
		{
			return this->canMove;
		}

		void setCanJump(bool canJump)
		{
			this->canJump = canJump;
		}

		bool getCanJump(void) const
		{
			return this->canJump;
		}

		Ogre::String getCurrentAppStateName(void) const
		{
			return this->currentAppStateName;
		}
	protected:
		Ogre::Vector3 initialOrientationAxis;
		NOWA::AnimationBlender* animationBlender;
		NOWA::GameObjectPtr gameObjectPtr;
		NOWA::PhysicsActiveCompPtr physicsCompPtr;
		NOWA::AttributesCompPtr attributesCompPtr;
		KI::StateMachine<Owner>* stateMachine;
		Ogre::String currentAppStateName;
		Ogre::Real moveWeight;
		Ogre::Real jumpWeight;
		bool idle;
		bool canMove;
		bool canJump;
	};

}; //Namespace end

#endif