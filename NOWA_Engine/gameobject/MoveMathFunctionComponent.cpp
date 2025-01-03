#include "NOWAPrecompiled.h"
#include "MoveMathFunctionComponent.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsActiveKinematicComponent.h"
#include "JointComponents.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	MoveMathFunctionComponent::MoveMathFunctionComponent()
		: GameObjectComponent(),
		oppositeDir(1.0f),
		progress(0.0f),
		round(0),
		totalIndex(0),
		physicsActiveComponent(nullptr),
		physicsActiveKinematicComponent(nullptr),
		originPosition(Ogre::Vector3::ZERO),
		originOrientation(Ogre::Quaternion::IDENTITY),
		oldPosition(Ogre::Vector3::ZERO),
		mathFunction(Ogre::Vector3::ZERO),
		firstTime(true),
		pathGoalObserver(nullptr),
		activated(new Variant(MoveMathFunctionComponent::AttrActivated(), true, this->attributes)),
		directionChange(new Variant(MoveMathFunctionComponent::AttrDirectionChange(), false, this->attributes)),
		repeat(new Variant(MoveMathFunctionComponent::AttrRepeat(), true, this->attributes)),
		autoOrientate(new Variant(MoveMathFunctionComponent::AttrAutoOrientate(), true, this->attributes)),
		functionCount(new Variant(MoveMathFunctionComponent::AttrFunctionCount(), static_cast<unsigned int>(0), this->attributes))
	{
		functionCount->addUserData(GameObject::AttrActionNeedRefresh());

		this->internalDirectionChange = this->directionChange->getBool();
	}

	MoveMathFunctionComponent::~MoveMathFunctionComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MoveMathFunctionComponent] Destructor move math function component for game object: " + this->gameObjectPtr->getName());
		
		if (nullptr != this->pathGoalObserver)
		{
			delete this->pathGoalObserver;
			this->pathGoalObserver = nullptr;
		}
	}

	bool MoveMathFunctionComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoOrientate")
		{
			this->autoOrientate->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FunctionCount")
		{
			this->functionCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->xFunctions.size() < this->functionCount->getUInt())
		{
			this->xFunctions.resize(this->functionCount->getUInt());
			this->yFunctions.resize(this->functionCount->getUInt());
			this->zFunctions.resize(this->functionCount->getUInt());
			this->minLengths.resize(this->functionCount->getUInt());
			this->maxLengths.resize(this->functionCount->getUInt());
			this->speeds.resize(this->functionCount->getUInt());
			this->rotationAxes.resize(this->functionCount->getUInt());
		}

		for (size_t i = 0; i < this->xFunctions.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "XFunction" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->xFunctions[i])
				{
					this->xFunctions[i] = new Variant(MoveMathFunctionComponent::AttrXFunction() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data", "cos(t)"), this->attributes);
					this->xFunctions[i]->setDescription("Sets the function for the x coordinate with the variable t.");
				}
				else
				{
					this->xFunctions[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "YFunction" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->yFunctions[i])
				{
					this->yFunctions[i] = new Variant(MoveMathFunctionComponent::AttrYFunction() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data", "sin(t)"), this->attributes);
					this->yFunctions[i]->setDescription("Sets the function for the y coordinate with the variable t.");
				}
				else
				{
					this->yFunctions[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ZFunction" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->zFunctions[i])
				{
					this->zFunctions[i] = new Variant(MoveMathFunctionComponent::AttrZFunction() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data", "0"), this->attributes);
				}
				else
				{
					this->zFunctions[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->zFunctions[i]->setDescription("Sets the function for the z coordinate with the variable t.");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinLength" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->minLengths[i])
				{
					this->minLengths[i] = new Variant(MoveMathFunctionComponent::AttrMinLength() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data", "0"), this->attributes);
				}
				else
				{
					this->minLengths[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->minLengths[i]->setDescription("Sets the minimum length for t.");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxLength" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->maxLengths[i])
				{
					this->maxLengths[i] = new Variant(MoveMathFunctionComponent::AttrMaxLength() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data", "2 * PI"), this->attributes);
				}
				else
				{
					this->maxLengths[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->minLengths[i]->setDescription("Sets the maximum length for t.");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->speeds[i])
				{
					this->speeds[i] = new Variant(MoveMathFunctionComponent::AttrSpeed() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
				}
				else
				{
					this->speeds[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->speeds[i]->setDescription("Sets speed for t.");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationAxis" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->rotationAxes[i])
				{
					this->rotationAxes[i] = new Variant(MoveMathFunctionComponent::AttrRotationAxis() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_Y), this->attributes);
				}
				else
				{
					this->rotationAxes[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->rotationAxes[i]->setDescription("Sets the rotation axis or axes, which are used for rotation. Note: The attribute AutoOrientation must be set to true.");
			}
			else
			{
				this->rotationAxes[i] = new Variant(MoveMathFunctionComponent::AttrRotationAxis() + Ogre::StringConverter::toString(i), Ogre::Vector3::UNIT_Y, this->attributes);
			}
		}
		return true;
	}

	GameObjectCompPtr MoveMathFunctionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MoveMathFunctionCompPtr clonedCompPtr(boost::make_shared<MoveMathFunctionComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setAutoOrientate(this->autoOrientate->getBool());

		clonedCompPtr->setFunctionCount(this->functionCount->getUInt());

		for (size_t i = 0; i < this->xFunctions.size(); i++)
		{
			clonedCompPtr->setXFunction(i, this->xFunctions[i]->getString());
			clonedCompPtr->setYFunction(i, this->yFunctions[i]->getString());
			clonedCompPtr->setZFunction(i, this->zFunctions[i]->getString());
			clonedCompPtr->setMinLength(i, this->minLengths[i]->getString());
			clonedCompPtr->setMaxLength(i, this->maxLengths[i]->getString());
			clonedCompPtr->setSpeed(i, this->speeds[i]->getReal());
			clonedCompPtr->setRotationAxis(i, this->rotationAxes[i]->getVector3());
		}

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MoveMathFunctionComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MoveMathFunctionComponent] Init move math function component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}
	
	bool MoveMathFunctionComponent::connect(void)
	{
		bool success = GameObjectComponent::connect();

		success |= this->parseMathematicalFunction();
		
		this->progress = 0.0f;
		this->internalDirectionChange = this->directionChange->getBool();
		this->round = 0;
		this->totalIndex = 0;
		this->oppositeDir = 1.0f;
		this->oldPosition = Ogre::Vector3::ZERO;
		this->mathFunction = Ogre::Vector3::ZERO;
		this->firstTime = true;
		
		if (true == success)
		{
			// Note: Special type must come first, because its a derived component from PhysicsActiveComponent
			auto physicsActiveKinematicCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveKinematicComponent>());
			if (nullptr != physicsActiveKinematicCompPtr)
			{
				this->physicsActiveKinematicComponent = physicsActiveKinematicCompPtr.get();
			}
			else
			{
				auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
				if (nullptr != physicsActiveCompPtr)
				{
					this->physicsActiveComponent = physicsActiveCompPtr.get();
				}
			}

			this->originPosition = this->gameObjectPtr->getSceneNode()->getPosition();
			this->originOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
		}
		
		return success;
	}

	bool MoveMathFunctionComponent::disconnect(void)
	{
		this->progress = 0.0f;
		this->internalDirectionChange = this->directionChange->getBool();
		this->round = 0;
		this->totalIndex = 0;
		this->oppositeDir = 1.0f;
		this->oldPosition = Ogre::Vector3::ZERO;
		this->mathFunction = Ogre::Vector3::ZERO;
		this->firstTime = true;

		if (nullptr != this->physicsActiveComponent)
		{
			this->physicsActiveComponent->setPosition(this->originPosition);
			this->physicsActiveComponent->setOrientation(this->originOrientation);
		}
		else if (nullptr != physicsActiveKinematicComponent)
		{
			this->physicsActiveKinematicComponent->setPosition(this->originPosition);
			this->physicsActiveKinematicComponent->setOrientation(this->originOrientation);
		}
		else
		{
			this->gameObjectPtr->getSceneNode()->setPosition(this->originPosition);
			this->gameObjectPtr->getSceneNode()->setOrientation(this->originOrientation);
		}

		this->physicsActiveComponent = nullptr;
		this->physicsActiveKinematicComponent = nullptr;

		if (nullptr != this->pathGoalObserver)
		{
			delete this->pathGoalObserver;
			this->pathGoalObserver = nullptr;
		}
		
		return true;
	}

	void MoveMathFunctionComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& physicsActiveKinematicCompPtr = boost::dynamic_pointer_cast<PhysicsActiveKinematicComponent>(gameObjectCompPtr);
			if (nullptr != physicsActiveKinematicCompPtr)
			{
				this->physicsActiveKinematicComponent = nullptr;
			}
			else
			{
				auto& physicsActiveCompPtr = boost::dynamic_pointer_cast<PhysicsActiveComponent>(gameObjectCompPtr);
				if (nullptr != physicsActiveCompPtr)
				{
					this->physicsActiveComponent = nullptr;
				}
			}
		}
	}

	void MoveMathFunctionComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && this->activated->getBool() && this->functionCount->getUInt() > 0)
		{
			if (false == this->firstTime)
			{
				this->oldPosition = this->gameObjectPtr->getPosition(); // this->originPosition + this->mathFunction;
			}

			if (Ogre::Math::RealEqual(this->oppositeDir, 1.0f))
			{
				this->progress += this->speeds[this->totalIndex]->getReal() * dt;

				double varX[] = { 0 };
				Ogre::Real maxLengthResult = static_cast<Ogre::Real>(this->functionParser[this->totalIndex * 5 + 4].Eval(varX));

				if (this->progress >= maxLengthResult)
				{

					this->round++;
					// Only change dir, if all functions have been run
					if (this->totalIndex >= this->functionCount->getUInt() - 1)
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "All functions finished +");
						if (true == this->internalDirectionChange)
						{
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Dir change from + to -");
							this->oppositeDir *= -1.0f;
						}
						else if (true == this->repeat->getBool())
						{
							// Note: progress must not be set to zero, because else, jumps will occure, when a function is finished after a while
							this->round = 0;
							if (this->totalIndex > 0)
								this->totalIndex--;
						}
					}
					else
					{
						this->totalIndex++;
						this->round = 0;
					}
				}
			}
			else
			{
				this->progress -= this->speeds[this->totalIndex]->getReal() * dt;

				double varX[] = { 0 };
				Ogre::Real minLengthResult = static_cast<Ogre::Real>(this->functionParser[this->totalIndex * 5 + 3].Eval(varX));

				if (this->progress <= minLengthResult)
				{
					this->round++;
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
					// Only change dir, if all functions have been run
					if (this->totalIndex == 0)
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "All functions finished -");
						if (true == this->internalDirectionChange)
						{
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Dir change from - to +");
							this->oppositeDir *= -1.0f;
						}
						else if (true == this->repeat->getBool())
						{
							this->round = 0;
							if (this->totalIndex >= this->functionCount->getUInt() - 1)
							{
								this->totalIndex--;
							}
						}
					
					}
					else
					{
						this->totalIndex--;
						this->round = 0;
					}
				}
				
			}
			// if the function took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
			// also take the progress into account, the function started at zero and should stop at zero
			// maybe here attribute in xml, to set how many rounds the function should proceed
			if (this->round == 2 && this->progress >= 0.0f)
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Function " + Ogre::StringConverter::toString(this->totalIndex) + " finished");
				if (nullptr != this->pathGoalObserver)
				{
					this->pathGoalObserver->onPathGoalReached();
				}

				// if repeat is off, only change the direction one time, to get back to its origin and leave
				if (false == this->repeat->getBool())
				{
					this->internalDirectionChange = false;
				}
				/*else
				{
					this->totalIndex = 0;
					this->round = 0;
				}*/
			}
			// When repeat is on, proceed
			// When direction change is on, proceed
			if (true == this->repeat->getBool() || true == this->internalDirectionChange || 0 == this->round)
			{
				// Ogre::Real moveValue = this->jointProperties.moveSpeed * oppositeDir;
				// set the x, y, z variable, which is the movement speed with positive or negative direction
				double varX[] = { this->progress };
				double varY[] = { this->progress };
				double varZ[] = { this->progress };
				// evauluate the function for x, y, z, to get the results
				this->mathFunction.x = static_cast<Ogre::Real>(this->functionParser[this->totalIndex * 5 + 0].Eval(varX));
				this->mathFunction.y = static_cast<Ogre::Real>(this->functionParser[this->totalIndex * 5 + 1].Eval(varY));
				this->mathFunction.z = static_cast<Ogre::Real>(this->functionParser[this->totalIndex * 5 + 2].Eval(varZ));

				
				if (true == this->firstTime)
				{
					this->oldPosition = this->originPosition + this->mathFunction;
					this->firstTime = false;
				}

				if (true == this->bShowDebugData)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "MoveMathFunctionComponent: Debug: x,y,z = " + Ogre::StringConverter::toString(this->progress) +
						" f(x) = " + Ogre::StringConverter::toString(this->mathFunction.x) +
						", f(y) = " + Ogre::StringConverter::toString(this->mathFunction.y) +
						", f(z) = " + Ogre::StringConverter::toString(this->mathFunction.z));
				}

				// No physics, just move the graphical game object
				if (nullptr == this->physicsActiveComponent && nullptr == this->physicsActiveKinematicComponent)
				{
					// this->gameObjectPtr->getSceneNode()->translate(mathFunction);
					// Note: Store the origin position and only add to that position, do not use translate, because all values will grow and grow!
					// e.g. speed 2 would create also radius of 2, which is fubbes
					// https://gamedev.stackexchange.com/questions/140394/moving-a-sprite-along-a-circle-given-by-an-implicit-equation
					this->gameObjectPtr->getSceneNode()->setPosition(this->originPosition + this->mathFunction);
					// this->gameObjectPtr->getSceneNode()->translate(mathFunction);

					if (true == this->autoOrientate->getBool())
					{
						Ogre::Quaternion newOrientation = ((this->originOrientation /** this->agent->getOrientation()*/)
							* this->gameObjectPtr->getDefaultDirection()).getRotationTo(this->gameObjectPtr->getPosition() - this->oldPosition);
						this->gameObjectPtr->getSceneNode()->setOrientation(newOrientation);
							// Ogre::Quaternion::Slerp(10.0f * dt, this->gameObjectPtr->getSceneNode()->getOrientation(), newOrientation, true));
					}
				}
				// PhysicsActiveComponent: Apply force and torque, but no velocity!
				// Scenario: Move along a math function, but it can be disturbed by other physics game objects!
				else if (nullptr != this->physicsActiveComponent)
				{
					Ogre::Vector3 desiredVelocity = (this->originPosition + this->mathFunction) - this->oldPosition;
					// desiredVelocity.normalise();
					// desiredVelocity *= this->speeds[this->totalIndex]->getReal();
					this->physicsActiveComponent->applyRequiredForceForVelocity(desiredVelocity);

					if (true == this->autoOrientate->getBool())
					{
						Ogre::Quaternion newOrientation = (this->physicsActiveComponent->getOrientation() * this->gameObjectPtr->getDefaultDirection()).getRotationTo(desiredVelocity);

						Ogre::Vector3 axis = Ogre::Vector3::ZERO;
						if (Ogre::Vector3::UNIT_X == this->rotationAxes[this->totalIndex]->getVector3())
							axis.x = newOrientation.getPitch().valueDegrees() * 0.1f;
						if (Ogre::Vector3::UNIT_Y == this->rotationAxes[this->totalIndex]->getVector3())
							axis.y = newOrientation.getYaw().valueDegrees() * 0.1f;
						if (Ogre::Vector3::UNIT_Z == this->rotationAxes[this->totalIndex]->getVector3())
							axis.z = newOrientation.getRoll().valueDegrees() * 0.1f;

						this->physicsActiveComponent->applyOmegaForce(axis);
						// Here: this->physicsActiveComponent->applyOmegaForceRotateTo()? Like in dancing scenario?
					}
					else
					{
						this->physicsActiveComponent->applyOmegaForceRotateTo(this->originOrientation, this->rotationAxes[this->totalIndex]->getVector3());
					}
				}
				// PhysicsActiveKinematicComponent: Apply velocity but no forces
				// Scenario: Move along a math function, but it cannot be disturbed by other physics game objects!
				else if (nullptr != this->physicsActiveKinematicComponent)
				{
					Ogre::Vector3 desiredVelocity = (this->originPosition + this->mathFunction) - this->oldPosition;
					// desiredVelocity.normalise();
					// desiredVelocity *= this->speeds[this->totalIndex]->getReal();
					this->physicsActiveKinematicComponent->setVelocity(desiredVelocity);

					if (true == this->autoOrientate->getBool())
					{
						Ogre::Quaternion newOrientation = (this->physicsActiveKinematicComponent->getOrientation() * this->gameObjectPtr->getDefaultDirection()).getRotationTo(desiredVelocity);
							
						Ogre::Vector3 axis = Ogre::Vector3::ZERO;
						if (Ogre::Vector3::UNIT_X == this->rotationAxes[this->totalIndex]->getVector3())
							axis.x = newOrientation.getPitch().valueDegrees() * 0.1f;
						if (Ogre::Vector3::UNIT_Y == this->rotationAxes[this->totalIndex]->getVector3())
							axis.y = newOrientation.getYaw().valueDegrees() * 0.1f;
						if (Ogre::Vector3::UNIT_Z == this->rotationAxes[this->totalIndex]->getVector3())
							axis.z = newOrientation.getRoll().valueDegrees() * 0.1f;
								
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Velocity: " + Ogre::StringConverter::toString(desiredVelocity) +  " Axis: " + Ogre::StringConverter::toString(axis));

						this->physicsActiveKinematicComponent->setOmegaVelocity(axis);
							// Here: this->physicsActiveKinematicComponent->setOmegaVelocityRotateTo()? Like in dancing scenario?
					}
					else
					{
						this->physicsActiveComponent->applyOmegaForceRotateTo(this->originOrientation, this->rotationAxes[this->totalIndex]->getVector3());
					}
				}
			}
			/*else
			{
				this->activated->setValue(false);
			}*/

			/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "TotalIndex: " + Ogre::StringConverter::toString(this->totalIndex) +
			 	" Round: " + Ogre::StringConverter::toString(this->round) + 
			 	" Progress: " + Ogre::StringConverter::toString(this->progress));*/
		}
	}

	bool MoveMathFunctionComponent::parseMathematicalFunction(void)
	{
		this->functionParser.clear();
		// 5 parser * function count (1 parser: xFunction, 2 parser yFunction, 3 parser zFunction, 4 parser minLength, 5 parser maxLength)
		this->functionParser.resize(5 * this->functionCount->getUInt());

		for (unsigned int i = 0; i < 5 * this->functionCount->getUInt(); i++)
		{
			// add the pi constant
			this->functionParser[i].AddConstant("PI", 3.1415926535897932);
		}

		for (unsigned int i = 0; i < this->functionCount->getUInt(); i++)
		{
			// parse the function for x, y, z
			int resX = this->functionParser[i * 5 + 0].Parse(this->xFunctions[i]->getString(), "t");
			if (resX > 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for x: "
					+ Ogre::String(this->functionParser[i * 5 + 0].ErrorMsg()));
				return false;
			}
	// Attention: X is used, test this
			int resY = this->functionParser[i * 5 + 1].Parse(this->yFunctions[i]->getString(), "t");
			if (resY > 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for y: "
					+ Ogre::String(this->functionParser[i * 5 + 1].ErrorMsg()));
				return false;
			}

			int resZ = this->functionParser[i * 5 + 2].Parse(this->zFunctions[i]->getString(), "t");
			if (resZ > 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for z: "
					+ Ogre::String(this->functionParser[i * 5 + 2].ErrorMsg()));
				return false;
			}

			int resMinLength = this->functionParser[i * 5 + 3].Parse(this->minLengths[i]->getString(), "t");
			if (resMinLength > 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for min length: "
					+ Ogre::String(this->functionParser[i * 5 + 3].ErrorMsg()));
				return false;
			}

			int resMaxLength = this->functionParser[i * 5 + 4].Parse(this->maxLengths[i]->getString(), "t");
			if (resMaxLength > 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for max length: "
					+ Ogre::String(this->functionParser[i * 5 +  4].ErrorMsg()));
				return false;
			}
		}
		return true;
	}

	void MoveMathFunctionComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MoveMathFunctionComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MoveMathFunctionComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (MoveMathFunctionComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (MoveMathFunctionComponent::AttrAutoOrientate() == attribute->getName())
		{
			this->setAutoOrientate(attribute->getBool());
		}
		else if (MoveMathFunctionComponent::AttrFunctionCount() == attribute->getName())
		{
			this->setFunctionCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->xFunctions.size()); i++)
			{
				if (MoveMathFunctionComponent::AttrXFunction() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setXFunction(i, attribute->getString());
				}
				else if (MoveMathFunctionComponent::AttrYFunction() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setYFunction(i, attribute->getString());
				}
				else if (MoveMathFunctionComponent::AttrZFunction() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setZFunction(i, attribute->getString());
				}
				else if (MoveMathFunctionComponent::AttrMinLength() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setMinLength(i, attribute->getString());
				}
				else if (MoveMathFunctionComponent::AttrMaxLength() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setMaxLength(i, attribute->getString());
				}
				else if (MoveMathFunctionComponent::AttrSpeed() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setSpeed(i, attribute->getReal());
				}
				else if (MoveMathFunctionComponent::AttrRotationAxis() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setRotationAxis(i, attribute->getVector3());
				}
			}
		}
	}

	void MoveMathFunctionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AutoOrientate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoOrientate->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FunctionCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->functionCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->xFunctions.size()); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "XFunction" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->xFunctions[i]->getString())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "YFunction" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->yFunctions[i]->getString())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "ZFunction" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->zFunctions[i]->getString())));
			propertiesXML->append_node(propertyXML);
		
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "MinLength" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minLengths[i]->getString())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "MaxLength" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxLengths[i]->getString())));
			propertiesXML->append_node(propertyXML);
		
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Speed" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speeds[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RotationAxis" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationAxes[i]->getVector3())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String MoveMathFunctionComponent::getClassName(void) const
	{
		return "MoveMathFunctionComponent";
	}

	Ogre::String MoveMathFunctionComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MoveMathFunctionComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (false == activated)
		{
			// If set to false, reset values: This is necessary if an effect should be played several times
			this->progress = 0.0f;
			this->internalDirectionChange = this->directionChange->getBool();
			this->round = 0;
			oppositeDir = 1.0f;
		}
	}

	bool MoveMathFunctionComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void MoveMathFunctionComponent::setFunctionCount(unsigned int functionCount)
	{
		this->functionParser.resize(5 * functionCount);

		this->functionCount->setValue(functionCount);

		size_t oldSize = this->xFunctions.size();

		if (functionCount > oldSize)
		{
			// Resize the waypoints array for count
			this->xFunctions.resize(this->functionCount->getUInt());
			this->yFunctions.resize(this->functionCount->getUInt());
			this->zFunctions.resize(this->functionCount->getUInt());
			this->minLengths.resize(this->functionCount->getUInt());
			this->maxLengths.resize(this->functionCount->getUInt());
			this->speeds.resize(this->functionCount->getUInt());
			this->rotationAxes.resize(this->functionCount->getUInt());

			for (size_t i = oldSize; i < this->xFunctions.size(); i++)
			{
			
				this->xFunctions[i] = new Variant(MoveMathFunctionComponent::AttrXFunction() + Ogre::StringConverter::toString(i), "cos(t)", this->attributes);
				this->xFunctions[i]->setDescription("Sets the function for the x coordinate with the variable t.");
				this->yFunctions[i] = new Variant(MoveMathFunctionComponent::AttrYFunction() + Ogre::StringConverter::toString(i), "0", this->attributes);
				this->yFunctions[i]->setDescription("Sets the function for the y coordinate with the variable t.");
				this->zFunctions[i] = new Variant(MoveMathFunctionComponent::AttrZFunction() + Ogre::StringConverter::toString(i), "sin(t)", this->attributes);
				this->zFunctions[i]->setDescription("Sets the function for the z coordinate with the variable t.");
				this->minLengths[i] = new Variant(MoveMathFunctionComponent::AttrMinLength() + Ogre::StringConverter::toString(i), "0", this->attributes);
				this->minLengths[i]->setDescription("Sets the maximum length for t.");
				this->maxLengths[i] = new Variant(MoveMathFunctionComponent::AttrMaxLength() + Ogre::StringConverter::toString(i), "2 * PI", this->attributes);
				this->speeds[i] = new Variant(MoveMathFunctionComponent::AttrSpeed() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->speeds[i]->setDescription("Sets speed for t.");
				this->rotationAxes[i] = new Variant(MoveMathFunctionComponent::AttrRotationAxis() + Ogre::StringConverter::toString(i), Ogre::Vector3::UNIT_Y, this->attributes);
				this->rotationAxes[i]->setDescription("Sets the rotation axis or axes, which are used for rotation. Note: The attribute AutoOrientation must be set to true.");
				this->rotationAxes[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (functionCount < oldSize)
		{
			this->eraseVariants(this->xFunctions, functionCount);
			this->eraseVariants(this->yFunctions, functionCount);
			this->eraseVariants(this->zFunctions, functionCount);
			this->eraseVariants(this->minLengths, functionCount);
			this->eraseVariants(this->maxLengths, functionCount);
			this->eraseVariants(this->speeds, functionCount);
			this->eraseVariants(this->rotationAxes, functionCount);
		}
	}

	unsigned int MoveMathFunctionComponent::getFunctionCount(void) const
	{
		return this->functionCount->getUInt();
	}

	void MoveMathFunctionComponent::setAutoOrientate(bool autoOrientate)
	{
		this->autoOrientate->setValue(autoOrientate);
	}

	bool MoveMathFunctionComponent::getAutoOrientate(void) const
	{
		return this->autoOrientate->getBool();
	}

	void MoveMathFunctionComponent::setXFunction(unsigned int index, const Ogre::String& xFunction)
	{
		this->functionParser.resize(5 * this->functionCount->getUInt());

		if (index > this->xFunctions.size())
			index = static_cast<unsigned int>(this->xFunctions.size()) - 1;

		this->xFunctions[index]->setValue(xFunction);
		int resX = this->functionParser[this->totalIndex * 5 + 0].Parse(xFunction, "t");
		if (resX > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for x: "
				+ Ogre::String(this->functionParser[this->totalIndex * 5 + 0].ErrorMsg()));
		}
	}

	void MoveMathFunctionComponent::setYFunction(unsigned int index, const Ogre::String& yFunction)
	{
		this->functionParser.resize(5 * this->functionCount->getUInt());

		if (index > this->yFunctions.size())
			index = static_cast<unsigned int>(this->yFunctions.size()) - 1;

		this->yFunctions[index]->setValue(yFunction);
// Attention: X is used, test this
		int resY = this->functionParser[this->totalIndex * 5 + 1].Parse(yFunction, "t");
		if (resY > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for y: "
				+ Ogre::String(this->functionParser[this->totalIndex * 5 + 1].ErrorMsg()));
		}
	}

	void MoveMathFunctionComponent::setZFunction(unsigned int index, const Ogre::String& zFunction)
	{
		this->functionParser.resize(5 * this->functionCount->getUInt());

		if (index > this->zFunctions.size())
			index = static_cast<unsigned int>(this->zFunctions.size()) - 1;

		this->zFunctions[index]->setValue(zFunction);
// Attention: X is used, test this
		int resZ = this->functionParser[this->totalIndex * 5 + 2].Parse(zFunction, "t");
		if (resZ > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MoveMathFunctionComponent] Mathematical function parse error for z: "
				+ Ogre::String(this->functionParser[this->totalIndex * 5 + 2].ErrorMsg()));
		}
	}

	Ogre::String MoveMathFunctionComponent::getXFunction(unsigned int index) const
	{
		if (index > this->xFunctions.size())
			return "";
		return this->xFunctions[index]->getString();
	}

	Ogre::String MoveMathFunctionComponent::getYFunction(unsigned int index) const
	{
		if (index > this->yFunctions.size())
			return "";
		return this->yFunctions[index]->getString();
	}

	Ogre::String MoveMathFunctionComponent::getZFunction(unsigned int index) const
	{
		if (index > this->zFunctions.size())
			return "";
		return this->zFunctions[index]->getString();
	}
	
	void MoveMathFunctionComponent::setMinLength(unsigned int index, const Ogre::String& minLength)
	{
		this->functionParser.resize(5 * this->functionCount->getUInt());

		if (index > this->minLengths.size())
			index = static_cast<unsigned int>(this->minLengths.size()) - 1;
		this->minLengths[index]->setValue(minLength);
	}
	
	Ogre::String MoveMathFunctionComponent::getMinLength(unsigned int index) const
	{
		if (index > this->minLengths.size())
			return "";
		return this->minLengths[index]->getString();
	}

	void MoveMathFunctionComponent::setMaxLength(unsigned int index, const Ogre::String& maxLength)
	{
		this->functionParser.resize(5 * this->functionCount->getUInt());

		if (index > this->maxLengths.size())
			index = static_cast<unsigned int>(this->maxLengths.size()) - 1;
		this->maxLengths[index]->setValue(maxLength);
	}

	Ogre::String MoveMathFunctionComponent::getMaxLength(unsigned int index) const
	{
		if (index > this->maxLengths.size())
			return "";
		return this->maxLengths[index]->getString();
	}
	
	void MoveMathFunctionComponent::setSpeed(unsigned int index, Ogre::Real speed)
	{
		if (index > this->speeds.size())
			index = static_cast<unsigned int>(this->speeds.size()) - 1;
		this->speeds[index]->setValue(speed);
	}
	
	Ogre::Real MoveMathFunctionComponent::getSpeed(unsigned int index) const
	{
		if (index > this->speeds.size())
			return 0;
		return this->speeds[index]->getReal();
	}

	void MoveMathFunctionComponent::setRotationAxis(unsigned int index, const Ogre::Vector3 pivotAxis)
	{
		if (index > this->rotationAxes.size())
			index = static_cast<unsigned int>(this->rotationAxes.size()) - 1;
		this->rotationAxes[index]->setValue(pivotAxis);
	}

	Ogre::Vector3 MoveMathFunctionComponent::getRotationAxis(unsigned int index) const
	{
		if (index > this->rotationAxes.size())
			return Ogre::Vector3::ZERO;
		return this->rotationAxes[index]->getVector3();
	}
	
	void MoveMathFunctionComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
		this->internalDirectionChange = directionChange;
	}

	bool MoveMathFunctionComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	void MoveMathFunctionComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
		this->progress = 0.0f;
		this->internalDirectionChange = this->directionChange->getBool();
		this->round = 0;
		this->totalIndex = 0;
		this->oppositeDir = 1.0f;
		this->oldPosition = Ogre::Vector3::ZERO;
		this->mathFunction = Ogre::Vector3::ZERO;
		this->firstTime = true;
	}

	bool MoveMathFunctionComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void MoveMathFunctionComponent::reactOnFunctionFinished(luabind::object closureFunction)
	{
		if (nullptr == this->pathGoalObserver)
		{
			this->pathGoalObserver = new PathGoalObserver();
			static_cast<PathGoalObserver*>(this->pathGoalObserver)->reactOnPathGoalReached(closureFunction);
		}
		else
		{
			static_cast<PathGoalObserver*>(this->pathGoalObserver)->reactOnPathGoalReached(closureFunction);
		}
	}

	// Functions:
	/*
	<property type="7" name="XFunction0" data="sin(t)*60"/>
	<property type="7" name="YFunction0" data="0"/>
	<property type="7" name="ZFunction0" data="t*60"/>
	<property type="7" name="MinLength0" data="-100"/>
	<property type="7" name="MaxLength0" data="100"/>
	<property type="6" name="Speed0" data="1"/>

	<property type="7" name="XFunction0" data="(sin(t)+cos(t))*60"/>
	<property type="7" name="YFunction0" data="0"/>
	<property type="7" name="ZFunction0" data="t*10"/>
	<property type="7" name="MinLength0" data="-100"/>
	<property type="7" name="MaxLength0" data="100"/>
	<property type="6" name="Speed0" data="1"/>

	//	/*cAbs  */
	//{
	//	"abs", 1, FuncDefinition::OkForInt
	//},
	//	/*cAcos */{ "acos",  1, FuncDefinition::AngleOut },
	//		/*cAcosh*/{ "acosh", 1, FuncDefinition::AngleOut },
	//		/*cArg */{ "arg",   1, FuncDefinition::AngleOut | FuncDefinition::ComplexOnly },
	//		/*cAsin */{ "asin",  1, FuncDefinition::AngleOut },
	//		/*cAsinh*/{ "asinh", 1, FuncDefinition::AngleOut },
	//		/*cAtan */{ "atan",  1, FuncDefinition::AngleOut },
	//		/*cAtan2*/{ "atan2", 2, FuncDefinition::AngleOut },
	//		/*cAtanh*/{ "atanh", 1, 0 },
	//		/*cCbrt */{ "cbrt",  1, 0 },
	//		/*cCeil */{ "ceil",  1, 0 },
	//		/*cConj */{ "conj",  1, FuncDefinition::ComplexOnly },
	//		/*cCos  */{ "cos",   1, FuncDefinition::AngleIn },
	//		/*cCosh */{ "cosh",  1, FuncDefinition::AngleIn },
	//		/*cCot  */{ "cot",   1, FuncDefinition::AngleIn },
	//		/*cCsc  */{ "csc",   1, FuncDefinition::AngleIn },
	//		/*cExp  */{ "exp",   1, 0 },
	//		/*cExp2 */{ "exp2",  1, 0 },
	//		/*cFloor*/{ "floor", 1, 0 },
	//		/*cHypot*/{ "hypot", 2, 0 },
	//		/*cIf   */{ "if",    0, FuncDefinition::OkForInt },
	//		/*cImag */{ "imag",  1, FuncDefinition::ComplexOnly },
	//		/*cInt  */{ "int",   1, 0 },
	//		/*cLog  */{ "log",   1, 0 },
	//		/*cLog10*/{ "log10", 1, 0 },
	//		/*cLog2 */{ "log2",  1, 0 },
	//		/*cMax  */{ "max",   2, FuncDefinition::OkForInt },
	//		/*cMin  */{ "min",   2, FuncDefinition::OkForInt },
	//		/*cPolar */{ "polar", 2, FuncDefinition::ComplexOnly | FuncDefinition::AngleIn },
	//		/*cPow  */{ "pow",   2, 0 },
	//		/*cReal */{ "real",  1, FuncDefinition::ComplexOnly },
	//		/*cSec  */{ "sec",   1, FuncDefinition::AngleIn },
	//		/*cSin  */{ "sin",   1, FuncDefinition::AngleIn },
	//		/*cSinh */{ "sinh",  1, FuncDefinition::AngleIn },
	//		/*cSqrt */{ "sqrt",  1, 0 },
	//		/*cTan  */{ "tan",   1, FuncDefinition::AngleIn },
	//		/*cTanh */{ "tanh",  1, FuncDefinition::AngleIn },
	//		/*cTrunc*/{ "trunc", 1, 0 }

}; // namespace end