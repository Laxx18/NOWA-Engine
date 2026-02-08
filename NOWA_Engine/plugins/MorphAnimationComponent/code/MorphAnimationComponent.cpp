#include "NOWAPrecompiled.h"
#include "MorphAnimationComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreEntity.h"
#include "OgreSubEntity.h"
#include "OgreMesh.h"
#include "OgreSubMesh.h"
#include "OgreSkeleton.h"
#include "Animation/OgreBone.h"
#include "OgrePose.h"
#include "OgreAnimation.h"
#include "OgreAnimationState.h"
#include "OgreAnimationTrack.h"
#include "OgreKeyFrame.h"
#include "OgreBitwise.h"
#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	// Internal animation name for pose blending
	static const Ogre::String POSE_ANIMATION_NAME = "_MorphAnimationComponent_PoseAnim";

	MorphAnimationComponent::MorphAnimationComponent()
		: GameObjectComponent(),
		name("MorphAnimationComponent"),
		activated(new Variant(MorphAnimationComponent::AttrActivated(), true, this->attributes)),
		boneName(new Variant(MorphAnimationComponent::AttrBoneName(), std::vector<Ogre::String>(), this->attributes)),
		poseAnimationCount(new Variant(MorphAnimationComponent::AttrPoseAnimationCount(), static_cast<unsigned int>(0), this->attributes)),
		accumulator(0.0f),
		entity(nullptr),
		mesh(nullptr),
		skeleton(nullptr),
		poseAnimationState(nullptr),
		poseKeyFrame(nullptr)
	{
		this->boneName->addUserData(GameObject::AttrActionAutoComplete());
		this->poseAnimationCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	MorphAnimationComponent::~MorphAnimationComponent(void)
	{

	}

	void MorphAnimationComponent::initialise()
	{

	}

	const Ogre::String& MorphAnimationComponent::getName() const
	{
		return this->name;
	}

	void MorphAnimationComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MorphAnimationComponent>(MorphAnimationComponent::getStaticClassId(), MorphAnimationComponent::getStaticClassName());
	}

	void MorphAnimationComponent::shutdown()
	{
		// Do nothing here, because its called far too late and nothing is there of NOWA-Engine anymore! 
		// Use @onRemoveComponent in order to destroy something.
	}

	void MorphAnimationComponent::uninstall()
	{
		// Do nothing here, because its called far too late and nothing is there of NOWA-Engine anymore! 
		// Use @onRemoveComponent in order to destroy something.
	}

	void MorphAnimationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool MorphAnimationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BoneName")
		{
			this->boneName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PoseAnimationCount")
		{
			this->poseAnimationCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Read pose animation attributes
		for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
		{
			// Pose name
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PoseName" + Ogre::StringConverter::toString(i))
			{
				if (i < this->poseNames.size())
				{
					this->poseNames[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				else
				{
					this->poseNames.push_back(new Variant(MorphAnimationComponent::AttrPoseName() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes));
					this->poseNames[i]->addUserData(GameObject::AttrActionAutoComplete());
				}
				propertyElement = propertyElement->next_sibling("property");
			}

			// Weight function
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WeightFunction" + Ogre::StringConverter::toString(i))
			{
				if (i < this->weightFunctions.size())
				{
					this->weightFunctions[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				else
				{
					this->weightFunctions.push_back(new Variant(MorphAnimationComponent::AttrWeightFunction() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes));
				}
				propertyElement = propertyElement->next_sibling("property");
			}

			// Speed
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed" + Ogre::StringConverter::toString(i))
			{
				if (i < this->speeds.size())
				{
					this->speeds[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->speeds.push_back(new Variant(MorphAnimationComponent::AttrSpeed() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes));
					this->speeds[i]->setConstraints(0.01f, 100.0f);
				}
				propertyElement = propertyElement->next_sibling("property");
			}

			// Time offset
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimeOffset" + Ogre::StringConverter::toString(i))
			{
				if (i < this->timeOffsets.size())
				{
					this->timeOffsets[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				else
				{
					this->timeOffsets.push_back(new Variant(MorphAnimationComponent::AttrTimeOffset() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
		}

		return success;
	}

	GameObjectCompPtr MorphAnimationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MorphAnimationComponentPtr clonedCompPtr(boost::make_shared<MorphAnimationComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setBoneName(this->boneName->getListSelectedValue());
		clonedCompPtr->setPoseAnimationCount(this->poseAnimationCount->getUInt());

		for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
		{
			clonedCompPtr->setPoseName(i, this->poseNames[i]->getListSelectedValue());
			clonedCompPtr->setWeightFunction(i, this->weightFunctions[i]->getString());
			clonedCompPtr->setSpeed(i, this->speeds[i]->getReal());
			clonedCompPtr->setTimeOffset(i, this->timeOffsets[i]->getReal());
		}

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MorphAnimationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// Get the v1::Entity from the game object
		this->entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr == this->entity)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Error: GameObject '" + this->gameObjectPtr->getName() + "' has no v1::Entity!");
			return false;
		}

		// Get the mesh
		this->mesh = this->entity->getMesh().get();
		if (nullptr == this->mesh)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Error: Entity has no mesh!");
			return false;
		}

		// Get skeleton if available
		if (this->entity->hasSkeleton())
		{
			this->skeleton = this->entity->getSkeleton();
		}

		// Initialize bone data
		this->initializeBoneData();

		// Initialize pose data
		this->initializePoseData();

		// Set the available bone names list for the variant
		this->boneName->setValue(this->availableBoneNames);

		// Create attributes if we have a count but no attributes yet
		if (this->poseAnimationCount->getUInt() > 0 && this->poseNames.empty())
		{
			this->createPoseAnimationAttributes(0);
		}

		// Parse the mathematical functions
		if (this->poseAnimationCount->getUInt() > 0)
		{
			if (false == this->parseMathematicalFunctions())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Failed to parse mathematical functions for game object: " + this->gameObjectPtr->getName());
			}
		}

		// Setup pose animation system
		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->setupPoseAnimation();
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::setupPoseAnimation");
		
		return true;
	}

	bool MorphAnimationComponent::connect(void)
	{
		GameObjectComponent::connect();

		// Reset accumulator on connect
		this->accumulator = 0.0f;

		// Enable pose animation state if available
		if (nullptr != this->poseAnimationState)
		{
			GraphicsModule::RenderCommand renderCommand = [this]()
			{
				this->poseAnimationState->setEnabled(true);
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::connect");
		}

		return true;
	}

	bool MorphAnimationComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		// Disable pose animation state
		if (nullptr != this->poseAnimationState)
		{
			GraphicsModule::RenderCommand renderCommand = [this]()
			{
				this->poseAnimationState->setEnabled(false);
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::disconnect");
		}

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		return true;
	}

	void MorphAnimationComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		// Remove the pose animation we created
		if (nullptr != this->mesh && this->mesh->hasAnimation(POSE_ANIMATION_NAME))
		{
			this->mesh->removeAnimation(POSE_ANIMATION_NAME);
		}

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
	}

	void MorphAnimationComponent::setupPoseAnimation(void)
	{
		if (nullptr == this->mesh || nullptr == this->entity)
		{
			return;
		}

		// Check if mesh has any poses
		if (this->mesh->getPoseCount() == 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Mesh has no poses, skipping animation setup");
			return;
		}

		// Remove existing animation if present
		if (this->mesh->hasAnimation(POSE_ANIMATION_NAME))
		{
			this->mesh->removeAnimation(POSE_ANIMATION_NAME);
		}

		// Create a new animation for pose blending
		// Length of 1.0 allows us to use time position 0 for our keyframe
		Ogre::v1::Animation* animation = this->mesh->createAnimation(POSE_ANIMATION_NAME, 1.0f);

		// Create a vertex animation track for the mesh (target 0 = shared geometry)
		// We'll create tracks for all targets that have poses
		std::set<unsigned short> poseTargets;
		for (size_t i = 0; i < this->mesh->getPoseCount(); ++i)
		{
			Ogre::v1::Pose* pose = this->mesh->getPose(static_cast<unsigned short>(i));
			if (nullptr != pose)
			{
				poseTargets.insert(pose->getTarget());
			}
		}

		// Create tracks for each target
		for (unsigned short target : poseTargets)
		{
			Ogre::v1::VertexAnimationTrack* track = animation->createVertexTrack(target, Ogre::v1::VAT_POSE);

			// Create a keyframe at time 0 where we'll set pose influences
			this->poseKeyFrame = track->createVertexPoseKeyFrame(0.0f);
		}

		// Refresh entity animation state
		this->entity->refreshAvailableAnimationState();

		// Get the animation state
		this->poseAnimationState = this->entity->getAnimationState(POSE_ANIMATION_NAME);
		if (nullptr != this->poseAnimationState)
		{
			this->poseAnimationState->setEnabled(true);
			this->poseAnimationState->setLoop(false);
			this->poseAnimationState->setTimePosition(0.0f);
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Pose animation setup complete");
	}

	void MorphAnimationComponent::updatePoseKeyFrame(void)
	{
		if (nullptr == this->mesh || nullptr == this->poseAnimationState)
		{
			return;
		}

		// Get the animation
		if (!this->mesh->hasAnimation(POSE_ANIMATION_NAME))
		{
			return;
		}

		Ogre::v1::Animation* animation = this->mesh->getAnimation(POSE_ANIMATION_NAME);

		// Update all tracks
		Ogre::v1::Animation::VertexTrackIterator trackIt = animation->getVertexTrackIterator();
		while (trackIt.hasMoreElements())
		{
			Ogre::v1::VertexAnimationTrack* track = trackIt.getNext();
			if (track->getNumKeyFrames() > 0)
			{
				Ogre::v1::VertexPoseKeyFrame* keyFrame = track->getVertexPoseKeyFrame(0);

				// Clear existing pose references
				keyFrame->removeAllPoseReferences();

				// Add pose references with current weights
				for (const auto& pair : this->currentPoseWeights)
				{
					// Find pose index by name
					for (size_t i = 0; i < this->mesh->getPoseCount(); ++i)
					{
						Ogre::v1::Pose* pose = this->mesh->getPose(static_cast<unsigned short>(i));
						if (nullptr != pose && pose->getName() == pair.first && pose->getTarget() == track->getHandle())
						{
							keyFrame->addPoseReference(static_cast<unsigned short>(i), pair.second);
							break;
						}
					}
				}
			}
		}

		// Force animation state update
		this->poseAnimationState->setTimePosition(0.0f);
	}

	void MorphAnimationComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool() && this->poseAnimationCount->getUInt() > 0)
		{
			if (nullptr == this->entity || nullptr == this->mesh)
			{
				return;
			}

			// Update accumulator
			this->accumulator += dt;

			auto closureFunction = [this](Ogre::Real renderDt)
				{
					bool weightsChanged = false;

					// Iterate through all configured pose animations
					for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
					{
						if (i >= this->poseNames.size() || i >= this->functionParsers.size())
						{
							continue;
						}

						const Ogre::String& poseName = this->poseNames[i]->getListSelectedValue();
						if (poseName.empty())
						{
							continue;
						}

						// Calculate the time value with speed and offset
						Ogre::Real speed = (i < this->speeds.size()) ? this->speeds[i]->getReal() : 1.0f;
						Ogre::Real timeOffset = (i < this->timeOffsets.size()) ? this->timeOffsets[i]->getReal() : 0.0f;
						double t = static_cast<double>(this->accumulator * speed + timeOffset);

						// Evaluate the function
						double varT[] = { t };
						double weight = this->functionParsers[i].Eval(varT);

						// Clamp weight to valid range
						weight = std::max(0.0, std::min(1.0, weight));

						// Update current pose weight
						Ogre::Real newWeight = static_cast<Ogre::Real>(weight);
						if (this->currentPoseWeights[poseName] != newWeight)
						{
							this->currentPoseWeights[poseName] = newWeight;
							weightsChanged = true;
						}
					}

					// Update keyframe if weights changed
					if (weightsChanged)
					{
						this->updatePoseKeyFrame();
					}
				};

			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
		}
	}

	void MorphAnimationComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MorphAnimationComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MorphAnimationComponent::AttrBoneName() == attribute->getName())
		{
			this->setBoneName(attribute->getListSelectedValue());
		}
		else if (MorphAnimationComponent::AttrPoseAnimationCount() == attribute->getName())
		{
			this->setPoseAnimationCount(attribute->getUInt());
		}
		else
		{
			// Check for pose-specific attributes
			for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
			{
				if ((MorphAnimationComponent::AttrPoseName() + Ogre::StringConverter::toString(i)) == attribute->getName())
				{
					this->setPoseName(i, attribute->getListSelectedValue());
				}
				else if ((MorphAnimationComponent::AttrWeightFunction() + Ogre::StringConverter::toString(i)) == attribute->getName())
				{
					this->setWeightFunction(i, attribute->getString());
				}
				else if ((MorphAnimationComponent::AttrSpeed() + Ogre::StringConverter::toString(i)) == attribute->getName())
				{
					this->setSpeed(i, attribute->getReal());
				}
				else if ((MorphAnimationComponent::AttrTimeOffset() + Ogre::StringConverter::toString(i)) == attribute->getName())
				{
					this->setTimeOffset(i, attribute->getReal());
				}
			}
		}
	}

	void MorphAnimationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BoneName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->boneName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PoseAnimationCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->poseAnimationCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
		{
			// Pose name
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "PoseName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->poseNames[i]->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);

			// Weight function
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "WeightFunction" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->weightFunctions[i]->getString())));
			propertiesXML->append_node(propertyXML);

			// Speed
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Speed" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speeds[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			// Time offset
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimeOffset" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timeOffsets[i]->getReal())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String MorphAnimationComponent::getClassName(void) const
	{
		return "MorphAnimationComponent";
	}

	Ogre::String MorphAnimationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MorphAnimationComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (nullptr != this->poseAnimationState)
		{
			this->poseAnimationState->setEnabled(activated);
		}
	}

	bool MorphAnimationComponent::getActivated(void) const
	{
		return this->activated->getBool();
	}

	void MorphAnimationComponent::setBoneName(const Ogre::String& boneName)
	{
		this->boneName->setListSelectedValue(boneName);
	}

	Ogre::String MorphAnimationComponent::getBoneName(void) const
	{
		return this->boneName->getListSelectedValue();
	}

	void MorphAnimationComponent::setPoseAnimationCount(unsigned int poseAnimationCount)
	{
		unsigned int prevCount = this->poseAnimationCount->getUInt();
		this->poseAnimationCount->setValue(poseAnimationCount);

		if (poseAnimationCount > prevCount)
		{
			this->createPoseAnimationAttributes(prevCount);
		}
		else if (poseAnimationCount < prevCount)
		{
			this->removePoseAnimationAttributes(poseAnimationCount);
		}

		// Re-parse functions when count changes
		this->parseMathematicalFunctions();
	}

	unsigned int MorphAnimationComponent::getPoseAnimationCount(void) const
	{
		return this->poseAnimationCount->getUInt();
	}

	void MorphAnimationComponent::setPoseName(unsigned int index, const Ogre::String& poseName)
	{
		if (index < this->poseNames.size())
		{
			this->poseNames[index]->setListSelectedValue(poseName);
		}
	}

	Ogre::String MorphAnimationComponent::getPoseName(unsigned int index) const
	{
		if (index < this->poseNames.size())
		{
			return this->poseNames[index]->getListSelectedValue();
		}
		return "";
	}

	void MorphAnimationComponent::setWeightFunction(unsigned int index, const Ogre::String& weightFunction)
	{
		if (index < this->weightFunctions.size())
		{
			this->weightFunctions[index]->setValue(weightFunction);
			// Re-parse the function
			this->parseMathematicalFunctions();
		}
	}

	Ogre::String MorphAnimationComponent::getWeightFunction(unsigned int index) const
	{
		if (index < this->weightFunctions.size())
		{
			return this->weightFunctions[index]->getString();
		}
		return "";
	}

	void MorphAnimationComponent::setSpeed(unsigned int index, Ogre::Real speed)
	{
		if (index < this->speeds.size())
		{
			this->speeds[index]->setValue(speed);
		}
	}

	Ogre::Real MorphAnimationComponent::getSpeed(unsigned int index) const
	{
		if (index < this->speeds.size())
		{
			return this->speeds[index]->getReal();
		}
		return 1.0f;
	}

	void MorphAnimationComponent::setTimeOffset(unsigned int index, Ogre::Real timeOffset)
	{
		if (index < this->timeOffsets.size())
		{
			this->timeOffsets[index]->setValue(timeOffset);
		}
	}

	Ogre::Real MorphAnimationComponent::getTimeOffset(unsigned int index) const
	{
		if (index < this->timeOffsets.size())
		{
			return this->timeOffsets[index]->getReal();
		}
		return 0.0f;
	}

	std::vector<Ogre::String> MorphAnimationComponent::getAvailablePoseNames(void) const
	{
		return this->availablePoseNames;
	}

	std::vector<Ogre::String> MorphAnimationComponent::getAvailableBoneNames(void) const
	{
		return this->availableBoneNames;
	}

	void MorphAnimationComponent::setPoseWeight(const Ogre::String& poseName, Ogre::Real weight)
	{
		weight = std::max(0.0f, std::min(1.0f, weight));
		this->currentPoseWeights[poseName] = weight;

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
			{
				this->updatePoseKeyFrame();
			};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::setPoseWeight");
	}

	Ogre::Real MorphAnimationComponent::getPoseWeight(const Ogre::String& poseName) const
	{
		auto it = this->currentPoseWeights.find(poseName);
		if (it != this->currentPoseWeights.end())
		{
			return it->second;
		}
		return 0.0f;
	}

	void MorphAnimationComponent::resetTime(void)
	{
		this->accumulator = 0.0f;
	}

	Ogre::Real MorphAnimationComponent::getAccumulatedTime(void) const
	{
		return this->accumulator;
	}

	bool MorphAnimationComponent::createPose(const Ogre::String& poseName, unsigned short target)
	{
		if (nullptr == this->mesh)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Cannot create pose: no mesh!");
			return false;
		}

		// Check if pose already exists
		try
		{
			this->mesh->getPose(poseName);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Pose '" + poseName + "' already exists!");
			return false;
		}
		catch (Ogre::Exception&)
		{
			// Pose doesn't exist, we can create it
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this, poseName, target]()
		{
			this->mesh->createPose(target, poseName);

			// Refresh pose data
			this->initializePoseData();

			// Re-setup animation
			this->setupPoseAnimation();
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::createPose");

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Created pose '" + poseName + "' with target " + Ogre::StringConverter::toString(target));

		return true;
	}

	bool MorphAnimationComponent::addPoseVertex(const Ogre::String& poseName, size_t vertexIndex,
		const Ogre::Vector3& offset, const Ogre::Vector3& normalOffset)
	{
		if (nullptr == this->mesh)
		{
			return false;
		}

		Ogre::v1::Pose* pose = nullptr;
		try
		{
			pose = this->mesh->getPose(poseName);
		}
		catch (Ogre::Exception&)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Pose '" + poseName + "' not found!");
			return false;
		}

		if (nullptr == pose)
		{
			return false;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [pose, vertexIndex, offset, normalOffset]()
			{
				if (normalOffset != Ogre::Vector3::ZERO)
				{
					pose->addVertex(vertexIndex, offset, normalOffset);
				}
				else
				{
					pose->addVertex(vertexIndex, offset);
				}
			};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::addPoseVertex");

		return true;
	}

	bool MorphAnimationComponent::removePose(const Ogre::String& poseName)
	{
		if (nullptr == this->mesh)
		{
			return false;
		}

		try
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, poseName]()
			{
				this->mesh->removePose(poseName);

				// Remove from current weights
				this->currentPoseWeights.erase(poseName);

				// Refresh pose data
				this->initializePoseData();

				// Re-setup animation
				this->setupPoseAnimation();
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MorphAnimationComponent::removePose");

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Removed pose '" + poseName + "'");
			return true;
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MorphAnimationComponent] Failed to remove pose '" + poseName + "': " + e.what());
			return false;
		}
	}

	size_t MorphAnimationComponent::getMeshPoseCount(void) const
	{
		if (nullptr == this->mesh)
		{
			return 0;
		}
		return this->mesh->getPoseCount();
	}

	Ogre::v1::Pose* MorphAnimationComponent::getPoseByIndex(size_t index) const
	{
		if (nullptr == this->mesh || index >= this->mesh->getPoseCount())
		{
			return nullptr;
		}
		return this->mesh->getPose(static_cast<unsigned short>(index));
	}

	Ogre::v1::Pose* MorphAnimationComponent::getPoseByName(const Ogre::String& poseName) const
	{
		if (nullptr == this->mesh)
		{
			return nullptr;
		}
		try
		{
			return this->mesh->getPose(poseName);
		}
		catch (Ogre::Exception&)
		{
			return nullptr;
		}
	}

	bool MorphAnimationComponent::parseMathematicalFunctions(void)
	{
		this->functionParsers.clear();
		this->functionParsers.resize(this->poseAnimationCount->getUInt());

		for (unsigned int i = 0; i < this->poseAnimationCount->getUInt(); i++)
		{
			// Add the PI constant
			this->functionParsers[i].AddConstant("PI", 3.1415926535897932);
			this->functionParsers[i].AddConstant("pi", 3.1415926535897932);

			if (i < this->weightFunctions.size())
			{
				Ogre::String function = this->weightFunctions[i]->getString();
				if (function.empty())
				{
					// Default function similar to Ogre sample: (sin(t) + 1) / 2
					function = "(sin(t) + 1) / 2";
					this->weightFunctions[i]->setValue(function);
				}

				// Parse the function with 't' as the variable
				int result = this->functionParsers[i].Parse(function, "t");
				if (result >= 0)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
						"[MorphAnimationComponent] Mathematical function parse error for pose " + Ogre::StringConverter::toString(i) + ": "
						+ Ogre::String(this->functionParsers[i].ErrorMsg()));
					return false;
				}
			}
		}
		return true;
	}

	void MorphAnimationComponent::initializePoseData(void)
	{
		this->availablePoseNames.clear();

		if (nullptr == this->mesh)
		{
			return;
		}

		// Get poses from the v1::Mesh using getPoseList()
		const auto& poseList = this->mesh->getPoseList();
		for (const Ogre::v1::Pose* pose : poseList)
		{
			if (nullptr != pose)
			{
				this->availablePoseNames.push_back(pose->getName());
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Found " + Ogre::StringConverter::toString(this->availablePoseNames.size()) + " poses for game object: " + this->gameObjectPtr->getName());

		// Update existing pose name variants with new list
		for (size_t i = 0; i < this->poseNames.size(); i++)
		{
			Ogre::String currentSelection = this->poseNames[i]->getListSelectedValue();
			this->poseNames[i]->setValue(this->availablePoseNames);
			if (!currentSelection.empty())
			{
				this->poseNames[i]->setListSelectedValue(currentSelection);
			}
		}
	}

	void MorphAnimationComponent::initializeBoneData(void)
	{
		this->availableBoneNames.clear();

		// Add empty option for "no bone"
		this->availableBoneNames.push_back("");

		if (nullptr == this->skeleton)
		{
			return;
		}

		// Get all bones from the skeleton
		unsigned short numBones = this->skeleton->getNumBones();
		for (unsigned short i = 0; i < numBones; ++i)
		{
			Ogre::v1::OldBone* bone = this->skeleton->getBone(i);
			if (nullptr != bone)
			{
				this->availableBoneNames.push_back(bone->getName());
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MorphAnimationComponent] Found " + Ogre::StringConverter::toString(numBones) + " bones for game object: " + this->gameObjectPtr->getName());
	}

	void MorphAnimationComponent::createPoseAnimationAttributes(unsigned int prevIndex)
	{
		for (unsigned int i = prevIndex; i < this->poseAnimationCount->getUInt(); i++)
		{
			// Pose name - with autocomplete from available poses
			Variant* poseName = new Variant(MorphAnimationComponent::AttrPoseName() + Ogre::StringConverter::toString(i),
				this->availablePoseNames, this->attributes);
			poseName->addUserData(GameObject::AttrActionAutoComplete());
			if (i < this->availablePoseNames.size())
			{
				poseName->setListSelectedValue(this->availablePoseNames[i]);
			}
			this->poseNames.push_back(poseName);

			// Weight function - default to Ogre sample style function with time offset
			Ogre::String defaultFunction = "(sin(t + " + Ogre::StringConverter::toString(static_cast<float>(i)) + ") + 1) / 2";
			this->weightFunctions.push_back(new Variant(MorphAnimationComponent::AttrWeightFunction() + Ogre::StringConverter::toString(i),
				defaultFunction, this->attributes));

			// Speed
			Variant* speed = new Variant(MorphAnimationComponent::AttrSpeed() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
			speed->setConstraints(0.01f, 100.0f);
			this->speeds.push_back(speed);

			// Time offset
			Variant* timeOffset = new Variant(MorphAnimationComponent::AttrTimeOffset() + Ogre::StringConverter::toString(i),
				static_cast<Ogre::Real>(i), this->attributes);
			this->timeOffsets.push_back(timeOffset);
		}
	}

	void MorphAnimationComponent::removePoseAnimationAttributes(unsigned int count)
	{
		// Remove excess attributes
		while (this->poseNames.size() > count)
		{
			unsigned int idx = static_cast<unsigned int>(this->poseNames.size() - 1);

			this->eraseVariants(this->poseNames, count);
			this->eraseVariants(this->weightFunctions, count);
			this->eraseVariants(this->speeds, count);
			this->eraseVariants(this->timeOffsets, count);
		}
	}

	bool MorphAnimationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if the game object has a v1::Entity
		Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			// Also ensure we don't already have this component
			auto morphCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<MorphAnimationComponent>());
			if (nullptr == morphCompPtr)
			{
				return true;
			}
		}
		return false;
	}

	// Lua registration part

	MorphAnimationComponent* getMorphAnimationComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MorphAnimationComponent>(gameObject->getComponentWithOccurrence<MorphAnimationComponent>(occurrenceIndex)).get();
	}

	MorphAnimationComponent* getMorphAnimationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MorphAnimationComponent>(gameObject->getComponent<MorphAnimationComponent>()).get();
	}

	MorphAnimationComponent* getMorphAnimationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MorphAnimationComponent>(gameObject->getComponentFromName<MorphAnimationComponent>(name)).get();
	}

	void MorphAnimationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
			[
				class_<MorphAnimationComponent, GameObjectComponent>("MorphAnimationComponent")
					.def("setActivated", &MorphAnimationComponent::setActivated)
					.def("getActivated", &MorphAnimationComponent::getActivated)
					.def("setBoneName", &MorphAnimationComponent::setBoneName)
					.def("getBoneName", &MorphAnimationComponent::getBoneName)
					.def("setPoseAnimationCount", &MorphAnimationComponent::setPoseAnimationCount)
					.def("getPoseAnimationCount", &MorphAnimationComponent::getPoseAnimationCount)
					.def("setPoseName", &MorphAnimationComponent::setPoseName)
					.def("getPoseName", &MorphAnimationComponent::getPoseName)
					.def("setWeightFunction", &MorphAnimationComponent::setWeightFunction)
					.def("getWeightFunction", &MorphAnimationComponent::getWeightFunction)
					.def("setSpeed", &MorphAnimationComponent::setSpeed)
					.def("getSpeed", &MorphAnimationComponent::getSpeed)
					.def("setTimeOffset", &MorphAnimationComponent::setTimeOffset)
					.def("getTimeOffset", &MorphAnimationComponent::getTimeOffset)
					.def("setPoseWeight", &MorphAnimationComponent::setPoseWeight)
					.def("getPoseWeight", &MorphAnimationComponent::getPoseWeight)
					.def("resetTime", &MorphAnimationComponent::resetTime)
					.def("getAccumulatedTime", &MorphAnimationComponent::getAccumulatedTime)
					.def("createPose", &MorphAnimationComponent::createPose)
					.def("addPoseVertex", &MorphAnimationComponent::addPoseVertex)
					.def("removePose", &MorphAnimationComponent::removePose)
					.def("getMeshPoseCount", &MorphAnimationComponent::getMeshPoseCount)
			];

		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "class inherits GameObjectComponent", MorphAnimationComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setActivated(bool activated)", "Sets whether the morph animation is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "bool getActivated()", "Gets whether the morph animation is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setBoneName(String boneName)", "Sets the bone name to associate with (optional).");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "String getBoneName()", "Gets the associated bone name.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseAnimationCount(unsigned int count)", "Sets the number of pose animations to control.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getPoseAnimationCount()", "Gets the number of pose animations.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseName(unsigned int index, String poseName)", "Sets the pose name at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "String getPoseName(unsigned int index)", "Gets the pose name at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setWeightFunction(unsigned int index, String weightFunction)", "Sets the weight function for the pose at the given index. Example: '(sin(t) + 1) / 2'");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "String getWeightFunction(unsigned int index)", "Gets the weight function for the pose at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setSpeed(unsigned int index, float speed)", "Sets the speed multiplier for the pose at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getSpeed(unsigned int index)", "Gets the speed multiplier for the pose at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setTimeOffset(unsigned int index, float timeOffset)", "Sets the time offset for the pose at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getTimeOffset(unsigned int index)", "Gets the time offset for the pose at the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void setPoseWeight(String poseName, float weight)", "Manually sets the weight for a pose by name.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getPoseWeight(String poseName)", "Gets the current weight for a pose by name.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "void resetTime()", "Resets the time accumulator to zero.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "float getAccumulatedTime()", "Gets the current time accumulator value.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "bool createPose(String poseName, unsigned short target = 0)", "Creates a new pose on the mesh. Target 0 = shared geometry, 1+ = submesh index.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "bool addPoseVertex(String poseName, unsigned int vertexIndex, Vector3 offset, Vector3 normalOffset = Vector3::ZERO)", "Adds a vertex offset to an existing pose.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "bool removePose(String poseName)", "Removes a pose from the mesh.");
		LuaScriptApi::getInstance()->addClassToCollection("MorphAnimationComponent", "unsigned int getMeshPoseCount()", "Gets the number of poses on the mesh.");

		gameObjectClass.def("getMorphAnimationComponentFromName", &getMorphAnimationComponentFromName);
		gameObjectClass.def("getMorphAnimationComponent", (MorphAnimationComponent * (*)(GameObject*)) & getMorphAnimationComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MorphAnimationComponent getMorphAnimationComponentFromName(String name)", "Gets the morph animation component by the given name. Returns nil if the component does not exist.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MorphAnimationComponent getMorphAnimationComponent()", "Gets the morph animation component. Returns nil if the component does not exist.");
	}

}; // namespace end