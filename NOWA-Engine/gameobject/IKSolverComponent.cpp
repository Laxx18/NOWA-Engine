//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Modified and adapted by Lukas Kalinowski for NOWA-Engine

#include "NOWAPrecompiled.h"
#include "IKSolverComponent.h"
#include "IKEffectorComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "AnimationComponent.h"

#include <ik/effector.h>
#include <ik/node.h>
#include <ik/solver.h>
#include <ik/util.h>

namespace
{
	vec3_t ogreVector3ToIK(const Ogre::Vector3& vec)
	{
		vec3_t ret;
		ret.v.x = vec.x_;
		ret.v.y = vec.y_;
		ret.v.z = vec.z_;
		return ret;
	}

	Vector3 vector3IKToOgre(const vec3_t* ik)
	{
		return Ogre::Vector3(ik->v.x, ik->v.y, ik->v.z);
	}

	quat_t ogreQuaternionToIK(const Ogre::Quaternion& quat)
	{
		quat_t ret;
		ret.q.x = quat.x_;
		ret.q.y = quat.y_;
		ret.q.z = quat.z_;
		ret.q.w = quat.w_;
		return ret;
	}

	Quaternion quaternionIKToOgre(const quat_t* ik)
	{
		return Ogre::Quaternion(ik->q.w, ik->q.x, ik->q.y, ik->q.z);
	}
}

namespace NOWA
{
	using namespace rapidxml;
	
	IKSolverComponent::IKSolverComponent()
		: GameObjectComponent(),
		alreadyConnected(false),
		targetBone(nullptr),
		options(AUTO_SOLVE | JOINT_ROTATIONS | UPDATE_ACTIVE_POSE),
		chainTreesNeedUpdating(false),
		treeNeedsRebuild(true),
		solverTreeValid(false)
	{
		this->targetBoneName = new Variant(IKSolverComponent::AttrTargetBoneName(), std::vector<Ogre::String>(), this->attributes);
		this->algorithm = new Variant(IKSolverComponent::AttrAlgorithm(), { "Fabrik", "One Bone", "Two Bone" }, this->attributes);
		this->jointRotations = new Variant(IKSolverComponent::AttrJointRotations(), true, this->attributes);
		this->targetRotations = new Variant(IKSolverComponent::AttrTargetRotations(), false, this->attributes);
		this->updateOriginalPose = new Variant(IKSolverComponent::AttrUpdateOriginalPose(), false, this->attributes);
		this->updateActivePose = new Variant(IKSolverComponent::AttrUpdateActivePose(), true, this->attributes);
		this->useOriginalPose = new Variant(IKSolverComponent::AttrUseOriginalPose(), false, this->attributes);
		this->constraints = new Variant(IKSolverComponent::AttrConstraints(), false, this->attributes);
		this->autoSolve = new Variant(IKSolverComponent::AttrAutoSolve(), true, this->attributes);
		this->maxIterations = new Variant(IKSolverComponent::AttrMaxIterations(), static_cast<unsigned int>(20), this->attributes);
		this->tolerance = new Variant(IKSolverComponent::AttrTolerance(), 0.001f, this->attributes);
	}

	IKSolverComponent::~IKSolverComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKSolverComponent] Destructor ik solver component for game object: " + this->gameObjectPtr->getName());
		
		// Destroying the solver tree will destroy the effector objects, so remove
		// any references any of the IKEffector objects could be holding
		for (size_t i = 0; i < this->ikEffectorList.size(); i++)
		{
			this->ikEffectorList[i]->setIKEffectorNode(nullptr);
		}

		ik_solver_destroy(this->solver);
		// context_->ReleaseIK();
		
		// this->targetBone = nullptr;
		/*if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
			this->debugGeometryNode = nullptr;
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
			this->debugGeometryEntity = nullptr;
		}*/
	}

	bool IKSolverComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		propertyElement = propertyElement->next_sibling("property");
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetBoneName")
		{
			this->targetBoneName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Algorithm")
		{
			this->algorithm->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointRotations")
		{
			this->jointRotations->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetRotations")
		{
			this->targetRotations->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpdateOriginalPose")
		{
			this->updateOriginalPose->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpdateActivePose")
		{
			this->updateActivePose->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseOriginalPose")
		{
			this->useOriginalPose->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Constraints")
		{
			this->constraints->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoSolve")
		{
			this->AutoSolve->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxIterations")
		{
			this->maxIterations->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Tolerance")
		{
			this->tolerance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr IKSolverComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TagPointCompPtr clonedCompPtr(boost::make_shared<IKSolverComponent>());

		clonedCompPtr->setTargetBoneName(this->targetBoneName->getListSelectedValue());
		clonedCompPtr->setAlgorithm(this->algorithm->getListSelectedValue());
		clonedCompPtr->setJointRotations(this->jointRotations->getBool());
		clonedCompPtr->setTargetRotations(this->targetRotations->getBool());
		clonedCompPtr->setUpdateOriginalPose(this->updateOriginalPose->getBool());
		clonedCompPtr->setUpdateActivePose(this->updateActivePose->getBool());
		clonedCompPtr->setUseOriginalPose(this->useOriginalPose->getBool());
		clonedCompPtr->setConstraints(this->constraints->getBool());
		clonedCompPtr->setAutoSolve(this->autoSolve->getBool());
		
		clonedCompPtr->setMaxIterations(this->maxIterations->getUInt());
		clonedCompPtr->setTolerance(this->tolerance->getReal());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		return clonedCompPtr;
	}

	bool IKSolverComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKSolverComponent] Init ik solver component for game object: " + this->gameObjectPtr->getName());

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{

			Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
			if (nullptr != skeleton)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKSolverComponent] List all bones for mesh '" + entity->getMesh()->getName() + "':");

				std::vector<Ogre::String> targetBoneNames;

				unsigned short numBones = entity->getSkeleton()->getNumBones();
				for (unsigned short iBone = 0; iBone < numBones; iBone++)
				{
					Ogre::v1::OldBone* bone = entity->getSkeleton()->getBone(iBone);
					if (nullptr == bone)
					{
						continue;
					}

					// Absolutely HAVE to create bone representations first. Otherwise we would get the wrong child count
					// because an attached object counts as a child
					// Would be nice to have a function that only gets the children that are bones...
					unsigned short numChildren = bone->numChildren();
					if (numChildren == 0)
					{
						bool unique = true;
						for (size_t i = 0; i < targetBoneNames.size(); i++)
						{
							if (targetBoneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							targetBoneNames.emplace_back(bone->getName());
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKSolverComponent] Bone name: " + bone->getName());
						}
					}
					else
					{
						bool unique = true;
						for (size_t i = 0; i < targetBoneNames.size(); i++)
						{
							if (targetBoneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							targetBoneNames.emplace_back(bone->getName());
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKSolverComponent] Bone name: " + bone->getName());
						}
					}
				}

				// Add all available tag names to list
				this->targetBoneName->setValue(targetBoneNames);
				
				// Init the ik solver with fabrik algorithm as default
				this->setAlgorithm("Fabrik");
			}
		}
		return true;
	}

	bool IKSolverComponent::connect(void)
	{
		if (false == alreadyConnected)
		{
			auto& animationCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AnimationComponent>());
			if (nullptr == animationCompPtr)
			{
				// Sent event with feedback
				boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{AnimationComponentMissing} -> : " + this->gameObjectPtr->getName()));
				NOWA::EventManager::getInstance()->queueEvent(eventDataNavigationMeshFeedback);
			}
			
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr == entity)
				return false;

			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				Ogre::v1::OldBone* bone = this->skeleton->getBone(i);
				Ogre::Quaternion orientation = bone->_getDerivedOrientation();
				// Ogre::Quaternion orientation = this->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(bone->_getDerivedOrientation());
				bone->setManuallyControlled(true);
				bone->setInheritOrientation(false);
				bone->setOrientation(orientation);
				
				// Note: IKEffectorComponent's must be placed after the IKSolverComponent!
				for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
				{
					// Working here with shared_ptrs is evil, because of bidirectional referencing
					auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
					IKEffectorComponent* ikEffectorComponent = dynamic_cast<IKEffectorComponent*>(component);
					if (nullptr != ikEffectorComponent)
					{
						this->ikEffectorList.emplace_back(ikEffectorComponent);
					}
				}

				this->alreadyConnected = true;
			}
		}
		
		/*
			Scenario:
			// Create animation controller and play walk animation
			jackAnimCtrl_ = jackNode_->CreateComponent<AnimationController>();
			jackAnimCtrl_->PlayExclusive("Models/Jack_Walk.ani", 0, true, 0.0f);

			// We need to attach two inverse kinematic effectors to Jack's feet to
			// control the grounding.
			leftFoot_  = jackNode_->GetChild("Bip01_L_Foot", true);
			rightFoot_ = jackNode_->GetChild("Bip01_R_Foot", true);
			leftEffector_  = leftFoot_->CreateComponent<IKEffector>();
			rightEffector_ = rightFoot_->CreateComponent<IKEffector>();
			// Control 2 segments up to the hips
			leftEffector_->SetChainLength(2);
			rightEffector_->SetChainLength(2);

			// For the effectors to work, an IKSolver needs to be attached to one of
			// the parent nodes. Typically, you want to place the solver as close as
			// possible to the effectors for optimal performance. Since in this case
			// we're solving the legs only, we can place the solver at the spine.
			Node* spine = jackNode_->GetChild("Bip01_Spine", true);
			solver_ = spine->CreateComponent<IKSolver>();

			// Two-bone solver is more efficient and more stable than FABRIK (but only
			// works for two bones, obviously).
			solver_->SetAlgorithm(IKSolver::TWO_BONE);

			// Disable auto-solving, which means we need to call Solve() manually
			solver_->SetFeature(IKSolver::AUTO_SOLVE, false);

			// Only enable this so the debug draw shows us the pose before solving.
			// This should NOT be enabled for any other reason (it does nothing and is
			// a waste of performance).
			solver_->SetFeature(IKSolver::UPDATE_ORIGINAL_POSE, true);
		*/
		
		return true;
	}

	bool IKSolverComponent::disconnect(void)
	{
		if (nullptr != this->tagPointNode)
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				//Remove associated tag point, we don't need it anymore
				Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
				if (nullptr != oldSkeletonInstance)
				{
					this->alreadyConnected = false;
				}
			}
		}
		return true;
	}
	
	void IKSolverComponent::onRemoveComponent(void)
	{
		// Do here something??
	}

	void IKSolverComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			/*
			auto* phyWorld = scene_->GetComponent<PhysicsWorld>();
			Vector3 leftFootPosition = leftFoot_->_getDerivedPosition();
			Vector3 rightFootPosition = rightFoot_->_getDerivedPosition();

			// Cast ray down to get the normal of the underlying surface
			PhysicsRaycastResult result;
			phyWorld->RaycastSingle(result, Ray(leftFootPosition + Vector3(0, 1, 0), Vector3(0, -1, 0)), 2);
			if (result.body_)
			{
				// Cast again, but this time along the normal. Set the target position
				// to the ray intersection
				phyWorld->RaycastSingle(result, Ray(leftFootPosition + result.normal_, -result.normal_), 2);
				// The foot node has an offset relative to the root node
				float footOffset = leftFoot_->_getDerivedPosition().y_ - jackNode_->_getDerivedPosition().y_;
				leftEffector_->SetTargetPosition(result.position_ + result.normal_ * footOffset);
				// Rotate foot according to normal
				leftFoot_->Rotate(Quaternion(Vector3(0, 1, 0), result.normal_), TS_WORLD);
			}

			// Same deal with the right foot
			phyWorld->RaycastSingle(result, Ray(rightFootPosition + Vector3(0, 1, 0), Vector3(0, -1, 0)), 2);
			if (result.body_)
			{
				phyWorld->RaycastSingle(result, Ray(rightFootPosition + result.normal_, -result.normal_), 2);
				float footOffset = rightFoot_->_getDerivedPosition().y_ - jackNode_->_getDerivedPosition().y_;
				rightEffector_->SetTargetPosition(result.position_ + result.normal_ * footOffset);
				rightFoot_->Rotate(Quaternion(Vector3(0, 1, 0), result.normal_), TS_WORLD);
			}

			solver_->Solve();
			*/
		}
	}

	void IKSolverComponent::actualizeValue(Variant* attribute)
	{
		if (IKSolverComponent::AttrTargetBoneName() == attribute->getName())
		{
			this->setTargetBoneName(attribute->getListSelectedValue());
		}
		else if (IKSolverComponent::AttrAlgorithm() == attribute->getName())
		{
			this->setAlgorithm(attribute->getListSelectedValue());
		}
		else if (IKSolverComponent::AttrJointRotations() == attribute->getName())
		{
			this->setJointRotations(attribute->getBool());
		}
		else if (IKSolverComponent::AttrTargetRotations() == attribute->getName())
		{
			this->setTargetRotations(attribute->getBool());
		}
		else if (IKSolverComponent::AttrJointRotations() == attribute->getName())
		{
			this->setJointRotations(attribute->getBool());
		}
		else if (IKSolverComponent::AttrUpdateOriginalPose() == attribute->getName())
		{
			this->setUpdateOriginalPose(attribute->getBool());
		}
		else if (IKSolverComponent::AttrUpdateActivePose() == attribute->getName())
		{
			this->setUpdateActivePose(attribute->getBool());
		}
		else if (IKSolverComponent::AttrUseOriginalPose() == attribute->getName())
		{
			this->setUseOriginalPose(attribute->getBool());
		}
		else if (IKSolverComponent::AttrConstraints() == attribute->getName())
		{
			this->setConstraints(attribute->getBool());
		}
		else if (IKSolverComponent::AttrAutoSolve() == attribute->getName())
		{
			this->setAutoSolve(attribute->getBool());
		}
		else if (IKSolverComponent::AttrMaxIterations() == attribute->getName())
		{
			this->setMaxIterations(attribute->getUInt());
		}
		else if (IKSolverComponent::AttrTolerance() == attribute->getName())
		{
			this->setTolerance(attribute->getReal());
		}
	}

	void IKSolverComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ComponentIKSolver"));
		propertyXML->append_attribute(doc.allocate_attribute("data", "IKSolverComponent"));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetBoneName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetBoneName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Algorithm"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->algorithm->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointRotations"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->jointRotations->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetRotations"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetRotations->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UpdateOriginalPose"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->updateOriginalPose->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UpdateActivePose"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->updateActivePose->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseOriginalPose"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useOriginalPose->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Constraints"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->constraints->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AutoSolve"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->autoSolve->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxIterations"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->maxIterations->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Tolerance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->tolerance->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void IKSolverComponent::showDebugData(bool bShow)
	{
		if (true == bShow)
		{
			/*if (nullptr == this->debugGeometryNode && nullptr != this->tagPointNode)
			{
				this->debugGeometryNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
				this->debugGeometryNode->setName("IKSolverComponentArrowNode");
				this->debugGeometryNode->setPosition(this->tagPointNode->getPosition() + this->gameObjectPtr->getSceneNode()->getPosition());
				this->debugGeometryNode->setOrientation(this->tagPointNode->getOrientation() * this->gameObjectPtr->getSceneNode()->getOrientation());
				this->debugGeometryNode->setScale(0.1f, 0.1f, 0.1f);
				this->debugGeometryEntity = this->gameObjectPtr->getSceneManager()->createEntity("Arrow.mesh");
				this->debugGeometryEntity->setName("IKSolverComponentEntity");
				this->debugGeometryEntity->setDatablock("BaseYellowLine");
				this->debugGeometryEntity->setQueryFlags(0 << 0);
				this->debugGeometryEntity->setCastShadows(false);
				this->debugGeometryNode->attachObject(this->debugGeometryEntity);
			}*/
		}
		else
		{
			/*if (nullptr != this->debugGeometryNode)
			{
				this->debugGeometryNode->detachAllObjects();
				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
				this->debugGeometryNode = nullptr;
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
				this->debugGeometryEntity = nullptr;
			}*/
		}
	}

	Ogre::String IKSolverComponent::getClassName(void) const
	{
		return "IKSolverComponent";
	}

	Ogre::String IKSolverComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void IKSolverComponent::setActivated(bool activated)
	{

	}

	void IKSolverComponent::setTargetBoneName(const Ogre::String& targetBoneName)
	{
		this->targetBoneName->setListSelectedValue(targetBoneName);
		
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				if (oldSkeletonInstance->hasBone(targetBoneName))
				{
					this->targetBone = oldSkeletonInstance->getBone(targetBoneName)
				}
			}
		}
	}
	
	Ogre::String IKSolverComponent::getTargetBoneName(void) const
	{
		return this->targetBoneName->getListSelectedValue();
	}

	void IKSolverComponent::setAlgorithm(const Ogre::String& algorithm)
	{
		this->algorithm->setValue(algorithm);
		
		/* We need to rebuild the tree so make sure that the scene is in the
		 * initial pose when this occurs.*/
		if (this->targetBone != nullptr)
			this->applyOriginalPoseToScene();

		// Initial flags for when there is no solver to destroy
		uint8_t initialFlags = 0;

		// Destroys the tree and the solver
		if (this->solver != nullptr)
		{
			initialFlags = this->solver->flags;
			this->destroyTree();
			ik_solver_destroy(this->solver);
		}

		if ("One Bone" == algorithm)
			this->solver = ik_solver_create(SOLVER_ONE_BONE);
		else if ("Two Bone" == algorithm)
			this->solver = ik_solver_create(SOLVER_TWO_BONE);
		else
			this->solver = ik_solver_create(SOLVER_FABRIK);

		this->solver->flags = initialFlags;

		if (this->targetBone != nullptr)
			this->rebuildTree();
	}
	
	Ogre::String IKSolverComponent::getAlgorithm(void) const
	{
		return this->algorithm->getListSelectedValue();
	}
	
	void IKSolverComponent::setJointRotations(bool jointRotations)
	{
		this->jointRotations->setValue(jointRotations);
		
		this->options &= ~JOINT_ROTATIONS;
		if (true == jointRotations)
		{
			 this->options |= JOINT_ROTATIONS;
		}
	}
	
	bool IKSolverComponent::getJointRotations(void) const
	{
		return this->jointRotations->getBool();
	}
	
	void IKSolverComponent::setTargetRotations(bool targetRotations)
	{
		this->targetRotations->setValue(targetRotations);

		this->options &= ~TARGET_ROTATIONS;
		if (true == targetRotations)
		{
			this->solver->flags &= ~SOLVER_CALCULATE_TARGET_ROTATIONS;
            this->solver->flags |= SOLVER_CALCULATE_TARGET_ROTATIONS;
			this->options |= TARGET_ROTATIONS;
		}
	}
	
	bool IKSolverComponent::getTargetRotations(void) const
	{
		return this->targetRotations->getBool();
	}
	
	void IKSolverComponent::setUpdateOriginalPose(bool updateOriginalPose)
	{
		this->updateOriginalPose->setValue(updateOriginalPose);
		
		this->options &= ~UPDATE_ORIGINAL_POSE;
		if (true == updateOriginalPose)
		{
			 this->options |= UPDATE_ORIGINAL_POSE;
		}
	}
	
	bool IKSolverComponent::getUpdateOriginalPose(void) const
	{
		return this->updateOriginalPose->getBool();
	}
	
	void IKSolverComponent::setUpdateActivePose(bool updateActivePose)
	{
		this->updateActivePose->setValue(updateActivePose);
		
		this->options &= ~UPDATE_ACTIVE_POSE;
		if (true == updateActivePose)
		{
			 this->options |= UPDATE_ACTIVE_POSE;
		}
	}
	
	bool IKSolverComponent::getUpdateActivePose(void) const
	{
		return this->updateActivePose->getBool();
	}
	
	void IKSolverComponent::setUseOriginalPose(bool useOriginalPose)
	{
		this->useOriginalPose->setValue(useOriginalPose);
		
		this->options &= ~USE_ORIGINAL_POSE;
		if (true == useOriginalPose)
		{
			 this->options |= USE_ORIGINAL_POSE;
		}
	}
	
	bool IKSolverComponent::getUseOriginalPose(void) const
	{
		return this->useOriginalPose->getBool();
	}
	
	void IKSolverComponent::setConstraints(bool constraints)
	{
		this->constraints->setValue(constraints);
		
		this->options &= ~CONSTRAINTS;
		if (true == constraints)
		{
			this->solver->flags &= ~SOLVER_ENABLE_CONSTRAINTS;
            this->solver->flags |= SOLVER_ENABLE_CONSTRAINTS;
			this->options |= CONSTRAINTS;
		}
	}
	
	bool IKSolverComponent::getConstraints(void) const
	{
		return this->constraints->getBool();
	}
	
	void IKSolverComponent::setAutoSolve(bool autoSolve)
	{
		this->autoSolve->setValue(autoSolve);
		
		this->options &= ~AUTO_SOLVE;
		if (true == autoSolve)
		{
			this->options |= AUTO_SOLVE;
		}
	}
	
	bool IKSolverComponent::getAutoSolve(void) const
	{
		return this->autoSolve->getBool();
	}
	
	void IKSolverComponent::setMaxIterations(unsigned int maxIterations)
	{
		this->maxIterations->setValue(maxIterations);
		
		this->solver->max_iterations = maxIterations;
	}
	
	unsigned int IKSolverComponent::getMaxIterations(void) const
	{
		return this->maxIterations->getUInt();
	}
	
	void IKSolverComponent::setTolerance(Ogre::Real tolerance)
	{
		if (tolerance < Ogre::Math::Epsilon)
			tolerance = Ogre::Math::Epsilon;
		
		this->tolerance->setValue(tolerance);
		this->solver->tolerance = tolerance;
	}
	
	Ogre::Real IKSolverComponent::getTolerance(void) const
	{
		return return this->maxIterations->getReal();
	}
	
	Ogre::v1::OldBone* IKSolverComponent::getTargetBone(void) const
	{
		return this->targetBone;
	}
	
	ik_solver_t* IKSolverComponent::getSolver(void) const
	{
		return this->solver;
	}
	
	ik_node_t* IKSolverComponent::createIKNodeFromOgreBone(const Ogre::v1::OldBone* bone)
	{
		ik_node_t* ikNode = ik_node_create(bone->GetID());

		// Set initial position/rotation and pass in Bone* as user data for later
		ikNode->original_position = ogreVector3ToIK(bone->_getDerivedPosition());
		ikNode->original_rotation = OgreQuaternionToIK(bone->_getDerivedOrientation());
		ikNode->user_data = (void*)bone;

		/*
		 * If node has an effector, also create and attach one to the
		 * library's node. At this point, the IKEffector component shouldn't be
		 * holding a reference to any internal effector. Check this for debugging
		 * purposes and log if it does.
		 */
// Attention: How to solve this, is it required?
		/*auto* effector = node->GetComponent<IKEffector>();
		if (effector != nullptr)
		{
	#ifdef DEBUG
			if (effector->ikEffectorNode_ != nullptr)
				URHO3D_LOGWARNINGF("[ik] IKEffector (attached to node \"%s\") has a reference to a possibly invalid internal effector. Should be NULL.", effector->GetNode()->GetName().CString());
	#endif
			ik_effector_t* ikEffector = ik_effector_create();
			ik_node_attach_effector(ikNode, ikEffector); // ownership of effector
			
			// Set the weak ptr of this component to the effector
			effector->setSolver(this); // hier wie in joint components, enable_weak_from_this?
			effector->setIKEffectorNode(ikNode);
		}*/

		// Exact same deal with the constraint
		/*auto* constraint = node->GetComponent<IKConstraint>();
		if (constraint != nullptr)
		{
	#ifdef DEBUG
			if (constraint->ikConstraintNode_ != nullptr)
				URHO3D_LOGWARNINGF("[ik] IKConstraint (attached to node \"%s\") has a reference to a possibly invalid internal constraint. Should be NULL.", constraint->GetNode()->GetName().CString());
	#endif

			constraint->SetIKConstraintNode(ikNode);
		}*/

		return ikNode;
	}

	void IKSolverComponent::destroyTree(void)
	{
		ik_solver_destroy_tree(this->solver);
		this->ikEffectorList.clear();
	}

	void IKSolverComponent::rebuildTree(void)
	{
		assert (this->targetBone != nullptr);

		// Destroy current tree and set a new root node
		DestroyTree();
		ik_node_t* ikRoot = this->createIKNodeFromOgreBone(this->targetBone);
		ik_solver_set_tree(this->solver, ikRoot);

		/*
		 * Collect all effectors and constraints from children, and filter them to
		 * make sure they are in our subtree.
		 */
		// this->targetBone->GetComponents<IKEffector>(effectorList_, true);
		for (auto it = this->ikEffectorList.begin(); it != this->ikEffectorList.end();)
		{
			if (ComponentIsInOurSubtree(*it))
			{
				this->buildTreeToEffector(this->ikEffectorList[i]);
				++it;
			}
			else
			{
				it = this->ikEffectorList.erase(it);
			}
		}
	
		/*for (PODVector<IKConstraint*>::Iterator it = constraintList_.Begin(); it != constraintList_.End();)
		{
			if (ComponentIsInOurSubtree(*it))
				++it;
			else
				it = constraintList_.Erase(it);
		}*/

		this->treeNeedsRebuild = false;
		this->markChainsNeedUpdating();
	}

	bool IKSolverComponent::buildTreeToEffector(IKEffectorComponent* effectorComponent)
	{
		/*
		 * NOTE: This function makes the assumption that the node the effector is
		 * attached to is -- without a doubt -- in our subtree (by using
		 * ComponentIsInOurSubtree() first). If this is not the case, the program
		 * will abort.
		 */

		/*
		 * we need to build tree up to the node where this effector was added. Do
		 * this by following the chain of parent nodes until we hit a node that
		 * exists in the solver's subtree. Then iterate backwards again and add each
		 * missing node to the solver's tree.
		 */
		const Ogre::v1::OldBone* iterBone = effectorComponent->getTargetNode();
		ik_node_t* ikNode;
		std::vector<const Ogre::v1::OldBone*> missingBones;
		while ((ikNode = ik_node_find_child(this->solver->tree, iterBone->GetID())) == nullptr)
		{
			missingNodes.emplace_back(iterBone);
			iterBone = iterBone->GetParent();

			// Assert the assumptions made (described in the beginning of this function)
			assert(iterBone != nullptr);
			assert (iterBone->HasComponent<IKSolver>() == false || iterBone == this->targetBone);
		}

		while (missingNodes.Size() > 0)
		{
			iterBone = missingNodes.Back();
			missingNodes.pop();

			ik_node_t* ikChildNode = this->createIKNodeFromOgreBone(iterBone);
			ik_node_add_child(ikNode, ikChildNode);

			ikNode = ikChildNode;
		}

		return true;
	}

	/*bool IKSolverComponent::componentIsInOurSubtree(Component* component) const
	{
		const Node* iterNode = component->GetNode();
		while (true)
		{
			// Note part of our subtree
			if (iterNode == nullptr)
				return false;
			// Reached the root node, it's part of our subtree!
			if (iterNode == this->targetBone)
				return true;
			// Path to us is being blocked by another solver
			Component* otherSolver = iterNode->GetComponent<IKSolver>();
			if (otherSolver != nullptr && otherSolver != component)
				return false;

			iterNode = iterNode->GetParent();
		}

		return true;
	}*/

	void IKSolverComponent::rebuildChainTrees(void)
	{
		solverTreeValid_ = (ik_solver_rebuild_chain_trees(this->solver) == 0);
		ik_calculate_rotation_weight_decays(&this->solver->chain_tree);

		this->chainTreesNeedUpdating = false;
	}

	void IKSolverComponent::recalculateSegmentLengths(void)
	{
		ik_solver_recalculate_segment_lengths(this->solver);
	}

	void IKSolverComponent::calculateJointRotations(void)
	{
		ik_solver_calculate_joint_rotations(this->solver);
	}

	void IKSolverComponent::solve(void)
	{
		if (this->treeNeedsRebuild)
			this->rebuildTree();

		if (this->chainTreesNeedUpdating)
			this->rebuildChainTrees();

		if (this->isSolverTreeValid() == false)
			return;

		if (this->options & UPDATE_ORIGINAL_POSE)
			this->applySceneToOriginalPose();

		if (this->options & UPDATE_ACTIVE_POSE)
			this->applySceneToActivePose();

		if (this->options & USE_ORIGINAL_POSE)
			this->applyOriginalPoseToActivePose();

		for (auto it = this->ikEffectorList.begin(); it != this->ikEffectorList.end(); ++it)
		{
			(*it)->updateTargetNodePosition();
		}

		ik_solver_solve(this->solver);

		if (this->options & JOINT_ROTATIONS)
			ik_solver_calculate_joint_rotations(this->solver);

		this->applyActivePoseToScene();
	}

	static void applyInitialPoseToSceneCallback(ik_node_t* ikNode)
	{
		auto* bone = (Ogre::v1::OldBone*)ikNode->user_data;
		bone->SetWorldPosition(vector3IKToOgre(&ikNode->original_position));
		bone->SetWorldRotation(quaternionIKToOgre(&ikNode->original_rotation));
	}
	
	void IKSolverComponent::applyOriginalPoseToScene(void)
	{
		ik_solver_iterate_tree(this->solver, applyInitialPoseToSceneCallback);
	}

	static void applySceneToInitialPoseCallback(ik_node_t* ikNode)
	{
		auto* bone = (Ogre::v1::OldBone*)ikNode->user_data;
		ikNode->original_rotation = OgreQuaternionToIK(bone->_getDerivedOrientation());
		ikNode->original_position = OgreVector3ToIK(bone->_getDerivedPosition());
	}
	void IKSolverComponent::applySceneToOriginalPose(void)
	{
		ik_solver_iterate_tree(this->solver, applySceneToInitialPoseCallback);
	}

	static void applyActivePoseToSceneCallback(ik_node_t* ikNode)
	{
		auto* bone = (Ogre::v1::OldBone*)ikNode->user_data;
		bone->SetWorldPosition(OgreVector3ToIK(&ikNode->position));
		bone->SetWorldRotation(OgreQuaternionToIK(&ikNode->rotation));
	}
	
	void IKSolverComponent::applyActivePoseToScene(void)
	{
		ik_solver_iterate_tree(this->solver, applyActivePoseToSceneCallback);
	}

	static void applySceneToActivePoseCallback(ik_node_t* ikNode)
	{
		auto* bone = (Ogre::v1::OldBone*)ikNode->user_data;
		ikNode->position = OgreVector3ToIK(bone->_getDerivedPosition());
		ikNode->rotation = OgreQuaternionToIK(bone->_getDerivedOrientation());
	}
	
	void IKSolverComponent::applySceneToActivePose(void)
	{
		ik_solver_iterate_tree(this->solver, applySceneToActivePoseCallback);
	}

	void IKSolverComponent::applyOriginalPoseToActivePose(void)
	{
		ik_solver_reset_to_original_pose(this->solver);
	}

	void IKSolverComponent::markChainsNeedUpdating(void)
	{
		this->chainTreesNeedUpdating = true;
	}

	void IKSolverComponent::markTreeNeedsRebuild(void)
	{
		this->treeNeedsRebuild = true;
	}

	bool IKSolverComponent::isSolverTreeValid(void) const
	{
		return this->solverTreeValid;
	}
	
	/*
		// ----------------------------------------------------------------------------
	
	  This next section maintains the internal list of effector nodes. Whenever
	  nodes are deleted or added to the scene, or whenever components are added
	  or removed from nodes, we must check to see which of those nodes are/were
	  IK effector nodes and update our internal list accordingly.
	 
	  Unfortunately, E_COMPONENTREMOVED and E_COMPONENTADDED do not fire when a
	  parent node is removed/added containing child effector nodes, so we must
	  also monitor E_NODEREMOVED AND E_NODEADDED.
	 

	// ----------------------------------------------------------------------------
	void IKSolver::OnSceneSet(Scene* scene)
	{
		if (features_ & AUTO_SOLVE)
			SubscribeToEvent(scene, E_SCENEDRAWABLEUPDATEFINISHED, URHO3D_HANDLER(IKSolver, HandleSceneDrawableUpdateFinished));
	}

	// ----------------------------------------------------------------------------
	void IKSolver::OnNodeSet(Node* node)
	{
		ApplyOriginalPoseToScene();
		DestroyTree();

		if (node != nullptr)
			RebuildTree();
	}

	// ----------------------------------------------------------------------------
	void IKSolver::HandleComponentAdded(StringHash eventType, VariantMap& eventData)
	{
		using namespace ComponentAdded;
		(void)eventType;

		auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());
		auto* component = static_cast<Component*>(eventData[P_COMPONENT].GetPtr());

		
		  When a solver gets added into the scene, any parent solver's tree will
		  be invalidated. We need to find all parent solvers (by iterating up the
		  tree) and mark them as such.
		 
		if (component->GetType() == IKSolver::GetTypeStatic())
		{
			for (Node* iterNode = node; iterNode != nullptr; iterNode = iterNode->GetParent())
			{
				auto* parentSolver = iterNode->GetComponent<IKSolver>();
				if (parentSolver != nullptr)
					parentSolver->MarkTreeNeedsRebuild();

			}

			return; // No need to continue processing effectors or constraints
		}

		if (solver_->tree == nullptr)
			return;

		
		  Update tree if component is an effector and is part of our subtree.
		 
		if (component->GetType() == IKEffector::GetTypeStatic())
		{
			// Not interested in components that won't be part of our
			if (ComponentIsInOurSubtree(component) == false)
				return;

			BuildTreeToEffector(static_cast<IKEffector*>(component));
			effectorList_.Push(static_cast<IKEffector*>(component));
			return;
		}

		if (component->GetType() == IKConstraint::GetTypeStatic())
		{
			if (ComponentIsInOurSubtree(component) == false)
				return;

			constraintList_.Push(static_cast<IKConstraint*>(component));
		}
	}

	// ----------------------------------------------------------------------------
	void IKSolver::HandleComponentRemoved(StringHash eventType, VariantMap& eventData)
	{
		using namespace ComponentRemoved;

		if (solver_->tree == nullptr)
			return;

		auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());
		auto* component = static_cast<Component*>(eventData[P_COMPONENT].GetPtr());

		
		  When a solver gets added into the scene, any parent solver's tree will
		  be invalidated. We need to find all parent solvers (by iterating up the
		  tree) and mark them as such.
		 
		if (component->GetType() == IKSolver::GetTypeStatic())
		{
			for (Node* iterNode = node; iterNode != nullptr; iterNode = iterNode->GetParent())
			{
				auto* parentSolver = iterNode->GetComponent<IKSolver>();
				if (parentSolver != nullptr)
					parentSolver->MarkTreeNeedsRebuild();

			}

			return; // No need to continue processing effectors or constraints
		}

		// If an effector was removed, the tree will have to be rebuilt.
		if (component->GetType() == IKEffector::GetTypeStatic())
		{
			if (ComponentIsInOurSubtree(component) == false)
				return;

			ik_node_t* ikNode = ik_node_find_child(solver_->tree, node->GetID());
			assert(ikNode != nullptr);

			ik_node_destroy_effector(ikNode);
			static_cast<IKEffector*>(component)->SetIKEffectorNode(nullptr);
			effectorList_.RemoveSwap(static_cast<IKEffector*>(component));

			ApplyOriginalPoseToScene();
			MarkTreeNeedsRebuild();
			return;
		}

		// Remove the ikNode* reference the IKConstraint was holding
		if (component->GetType() == IKConstraint::GetTypeStatic())
		{
			if (ComponentIsInOurSubtree(component) == false)
				return;

			ik_node_t* ikNode = ik_node_find_child(solver_->tree, node->GetID());
			assert(ikNode != nullptr);

			static_cast<IKConstraint*>(component)->SetIKConstraintNode(nullptr);
			constraintList_.RemoveSwap(static_cast<IKConstraint*>(component));
		}
	}

	// ----------------------------------------------------------------------------
	void IKSolver::HandleNodeAdded(StringHash eventType, VariantMap& eventData)
	{
		using namespace NodeAdded;

		if (solver_->tree == nullptr)
			return;

		auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());

		PODVector<IKEffector*> effectors;
		node->GetComponents<IKEffector>(effectors, true);
		for (PODVector<IKEffector*>::ConstIterator it = effectors.Begin(); it != effectors.End(); ++it)
		{
			if (ComponentIsInOurSubtree(*it) == false)
				continue;

			BuildTreeToEffector(*it);
			effectorList_.Push(*it);
		}

		PODVector<IKConstraint*> constraints;
		node->GetComponents<IKConstraint>(constraints, true);
		for (PODVector<IKConstraint*>::ConstIterator it = constraints.Begin(); it != constraints.End(); ++it)
		{
			if (ComponentIsInOurSubtree(*it) == false)
				continue;

			constraintList_.Push(*it);
		}
	}

	// ----------------------------------------------------------------------------
	void IKSolver::HandleNodeRemoved(StringHash eventType, VariantMap& eventData)
	{
		using namespace NodeRemoved;

		if (solver_->tree == nullptr)
			return;

		auto* node = static_cast<Node*>(eventData[P_NODE].GetPtr());

		// Remove cached IKEffectors from our list
		PODVector<IKEffector*> effectors;
		node->GetComponents<IKEffector>(effectors, true);
		for (PODVector<IKEffector*>::ConstIterator it = effectors.Begin(); it != effectors.End(); ++it)
		{
			(*it)->SetIKEffectorNode(nullptr);
			effectorList_.RemoveSwap(*it);
		}

		PODVector<IKConstraint*> constraints;
		node->GetComponents<IKConstraint>(constraints, true);
		for (PODVector<IKConstraint*>::ConstIterator it = constraints.Begin(); it != constraints.End(); ++it)
		{
			constraintList_.RemoveSwap(*it);
		}

		// Special case, if the node being destroyed is the root node, destroy the
		// solver's tree instead of destroying the single node. Calling
		// ik_node_destroy() on the solver's root node will cause segfaults.
		ik_node_t* ikNode = ik_node_find_child(solver_->tree, node->GetID());
		if (ikNode != nullptr)
		{
			if (ikNode == solver_->tree)
				ik_solver_destroy_tree(solver_);
			else
				ik_node_destroy(ikNode);

			MarkChainsNeedUpdating();
		}
	}

	// ----------------------------------------------------------------------------
	void IKSolver::HandleSceneDrawableUpdateFinished(StringHash eventType, VariantMap& eventData)
	{
		Solve();
	}

	// ----------------------------------------------------------------------------
	void IKSolver::DrawDebugGeometry(bool depthTest)
	{
		auto* debug = GetScene()->GetComponent<DebugRenderer>();
		if (debug)
			DrawDebugGeometry(debug, depthTest);
	}

	// ----------------------------------------------------------------------------
	void IKSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
	{
		// Draws all scene segments
		for (PODVector<IKEffector*>::ConstIterator it = effectorList_.Begin(); it != effectorList_.End(); ++it)
			(*it)->DrawDebugGeometry(debug, depthTest);

		ORDERED_VECTOR_FOR_EACH(&solver_->effector_nodes_list, ik_node_t*, pnode)
			ik_effector_t* effector = (*pnode)->effector;

			// Calculate average length of all segments so we can determine the radius
			// of the debug spheres to draw
			int chainLength = effector->chain_length == 0 ? -1 : effector->chain_length;
			ik_node_t* a = *pnode;
			ik_node_t* b = a->parent;
			float averageLength = 0.0f;
			unsigned numberOfSegments = 0;
			while (b && chainLength-- != 0)
			{
				vec3_t v = a->original_position;
				vec3_sub_vec3(v.f, b->original_position.f);
				averageLength += vec3_length(v.f);
				++numberOfSegments;
				a = b;
				b = b->parent;
			}
			averageLength /= numberOfSegments;

			// connect all chained nodes together with lines
			chainLength = effector->chain_length == 0 ? -1 : effector->chain_length;
			a = *pnode;
			b = a->parent;
			debug->AddSphere(
				Sphere(Vec3IK2Urho(&a->original_position), averageLength * 0.1f),
				Color(0, 0, 255),
				depthTest
			);
			debug->AddSphere(
				Sphere(Vec3IK2Urho(&a->position), averageLength * 0.1f),
				Color(255, 128, 0),
				depthTest
			);
			while (b && chainLength-- != 0)
			{
				debug->AddLine(
					Vec3IK2Urho(&a->original_position),
					Vec3IK2Urho(&b->original_position),
					Color(0, 255, 255),
					depthTest
				);
				debug->AddSphere(
					Sphere(Vec3IK2Urho(&b->original_position), averageLength * 0.1f),
					Color(0, 0, 255),
					depthTest
				);
				debug->AddLine(
					Vec3IK2Urho(&a->position),
					Vec3IK2Urho(&b->position),
					Color(255, 0, 0),
					depthTest
				);
				debug->AddSphere(
					Sphere(Vec3IK2Urho(&b->position), averageLength * 0.1f),
					Color(255, 128, 0),
					depthTest
				);
				a = b;
				b = b->parent;
			}
		ORDERED_VECTOR_END_EACH
	}
	*/
	
} // namespace end
