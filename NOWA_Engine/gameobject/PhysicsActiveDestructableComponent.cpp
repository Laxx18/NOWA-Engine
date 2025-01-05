#include "NOWAPrecompiled.h"
#include "PhysicsActiveDestructableComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "JointComponents.h"
#include "tinyxml.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	typedef boost::shared_ptr<JointComponent> JointCompPtr;

	PhysicsActiveDestructableComponent::PhysicsActiveDestructableComponent()
		: PhysicsActiveComponent(),
		jointCount(0),
		partsCount(0),
		firstTimeBroken(false),
		breakForce(new Variant(PhysicsActiveDestructableComponent::AttrBreakForce(), 85.0f, this->attributes)),
		breakTorque(new Variant(PhysicsActiveDestructableComponent::AttrBreakTorque(), 85.0f, this->attributes)),
		density(new Variant(PhysicsActiveDestructableComponent::AttrDensity(), 60.0f, this->attributes)),
		motionless(new Variant(PhysicsActiveDestructableComponent::AttrMeshMotionless(), true, this->attributes)),
		splitMeshButton(new Variant(PhysicsActiveDestructableComponent::AttrSplitMesh(), false, this->attributes))
	{
		// What is this?
		this->splitMeshButton->addUserData("button");
		this->splitMeshButton->setDescription("Splits the mesh manually, if does not exist.");
		this->motionless->setDescription("Sets whether this mesh can be moved or not.");
		this->gyroscopicTorque->setValue(false);
		this->gyroscopicTorque->setVisible(false);
	}

	PhysicsActiveDestructableComponent::~PhysicsActiveDestructableComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent] Destructor physics active destructable component for game object: " 
			+ this->gameObjectPtr->getName());

		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); ++it)
		{
			delete it->second;
		}
		this->parts.clear();
	}

	bool PhysicsActiveDestructableComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MeshBreakForce")
		{
			this->breakForce->setValue(XMLConverter::getAttribReal(propertyElement, "data", 85.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MeshBreakTorque")
		{
			this->breakTorque->setValue(XMLConverter::getAttribReal(propertyElement, "data", 85.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		// unfortunately BreakTorgue is missing in hinge joint, that is: When a object is falling to fast, it would break apart
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MeshDensity")
		{
			this->density->setValue(XMLConverter::getAttribReal(propertyElement, "data", 60.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MeshMotionless")
		{
			this->motionless->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}

		// PhysicsActiveComponent::parseCompoundGameObjectProperties(propertyElement, filename);

		return true;
	}

	GameObjectCompPtr PhysicsActiveDestructableComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsActiveDestructableCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveDestructableComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMass(this->mass->getReal());
		clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
		clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
		clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
		clonedCompPtr->setGravity(this->gravity->getVector3());
		// clonedCompPtr->applyAngularImpulse(this->angularImpulse);
		// clonedCompPtr->applyAngularVelocity(this->angularVelocity->getVector3());
		clonedCompPtr->applyForce(this->force->getVector3());
		clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
		clonedCompPtr->setSpeed(this->speed->getReal());
		clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
		// clonedCompPtr->setDefaultPoseName(this->defaultPoseName);
		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		// do not use constraintAxis variable, because its being manipulated during physics body creation
		// clonedCompPtr->setConstraintAxis(this->initConstraintAxis);

		clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
		// clonedCompPtr->setCollidable(this->collidable->getBool());

		clonedCompPtr->setBreakForce(this->breakForce->getReal());
		clonedCompPtr->setBreakTorque(this->breakTorque->getReal());
		clonedCompPtr->setDensity(this->density->getReal());
		clonedCompPtr->setMotionLess(this->motionless->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsActiveDestructableComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent] Init physics active destructable component for game object: " 
			+ this->gameObjectPtr->getName());

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		Ogre::Item* item = nullptr;
		if (nullptr == entity)
		{
			item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr == entity)
			{
				return false;
			}
		}

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		Ogre::String meshName = "";
		if (nullptr != entity)
		{
			meshName = entity->getMesh()->getName();
		}
		else
		{
			meshName = item->getMesh()->getName();
		}

		size_t pos = meshName.rfind(".");
		if (Ogre::String::npos != pos)
			this->meshSplitConfigFile = meshName.substr(0, pos);

		this->meshSplitConfigFile += ".xml";
		
		return success;
	}

	bool PhysicsActiveDestructableComponent::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		Ogre::Item* item = nullptr;
		if (nullptr == entity)
		{
			item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr == entity)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Error cannot create static body, because the game object has no entity with mesh for game object: " + this->gameObjectPtr->getName());
				return false;
			}
		}
		
		// first hide the origin entity and its holding mesh, because it was just a placeholder for the breakable meshes
		// but only if the object is motion less, looks better!
		if (false == this->motionless->getBool())
		{
			this->gameObjectPtr->getSceneNode()->setVisible(false);
		}
		if (!this->createDynamicBody())
		{
			return false;
		}

		return success;
	}

	bool PhysicsActiveDestructableComponent::disconnect(void)
	{
		bool success = PhysicsActiveComponent::disconnect();

		for (auto& i = this->parts.cbegin(); i != this->parts.cend(); i++)
		{
			delete i->second;
		}
		this->parts.clear();
		this->gameObjectPtr->getSceneNode()->setVisible(true);

		return success;
	}

	bool PhysicsActiveDestructableComponent::createDynamicBody(void)
	{
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
		this->collisionType->setValue("ConvexHull");

		// calculate force for breaking joint (scale * density)
		Ogre::Real breakForceScale = this->initialScale.x * this->initialScale.y * this->initialScale.z * this->density->getReal();
		// Attention: This was uncommented
		// this->breakForce *= breakForceScale;
		// this->breakTorque *= breakForceScale;

		Ogre::Real partBreakForce = this->breakForce->getReal() * breakForceScale;
		Ogre::Real partBreakTorque = this->breakTorque->getReal() * breakForceScale;

		Ogre::DataStreamPtr stream;
		try
		{
			stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->meshSplitConfigFile);
			if (stream.isNull())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Could not open mesh split file: "
					+ this->meshSplitConfigFile + " for game object: " + this->gameObjectPtr->getName() + " so it will be generated. This can take a while.");

			}
		}
		catch (const Ogre::FileNotFoundException&)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Could not open mesh split file: "
				+ this->meshSplitConfigFile + " for game object: " + this->gameObjectPtr->getName() + " so it will be generated. This can take a while.");

			stream = this->execSplitMesh();
		}

		if (nullptr == stream)
		{
			return false;
		}

		TiXmlDocument document;
		// Get the file contents
		Ogre::String data = stream->getAsString();

		// Parse the XML
		document.Parse(data.c_str());
		stream->close();
		if (true == document.Error())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Could not parse mesh split file: "
				+ this->meshSplitConfigFile + " for game object: " + this->gameObjectPtr->getName());

			// Sent event with feedback
			boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{MeshSplitMissing}"));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);

			return false;
		}
		
		// Go through all parts and connections in the XML and create the breakable parts and joint them
		TiXmlElement* rootElement = document.FirstChildElement();
		TiXmlElement* childElement = rootElement->FirstChildElement();

		for (childElement = rootElement->FirstChildElement("mesh"); childElement; childElement = childElement->NextSiblingElement("mesh"))
		{
			this->partsCount++;
		}

		// Reset, to go from the beginning
		rootElement = document.FirstChildElement();
		childElement = rootElement->FirstChildElement();

		while (childElement)
		{
			SplitPart* part1 = getOrCreatePart(childElement->Attribute("name"));
			TiXmlElement* connectionElement = childElement->FirstChildElement();
			while (connectionElement)
			{
				SplitPart* part2 = getOrCreatePart(connectionElement->Attribute("target"));

				TiXmlElement* attributeElement = connectionElement->FirstChildElement();

				/*double jointPositionX = 0.0;
				double jointPositionY = 0.0;
				double jointPositionZ = 0.0;
				Ogre::Vector3 jointPosition = Ogre::Vector3::ZERO;*/

				// use the initial calulcated forces and multiplate with the fractions forces of the parts
				while (attributeElement)
				{
					double d;
					if (attributeElement->ValueTStr() == "rel_force")
					{
						attributeElement->Attribute("value", &d);
						if (0.0 == d)
						{
							d = 5.0;
						}
						partBreakForce += static_cast<Ogre::Real>(d);
					}
					if (attributeElement->ValueTStr() == "rel_torque")
					{
						attributeElement->Attribute("value", &d);
						if (0.0 == d)
						{
							d = 5.0;
						}
						partBreakTorque += static_cast<Ogre::Real>(d);
					}
					/*if (attributeElement->ValueTStr() == "joint_center")
					{
					attributeElement->Attribute("x", &jointPositionX);
					attributeElement->Attribute("y", &jointPositionY);
					attributeElement->Attribute("z", &jointPositionZ);
					jointPosition = Ogre::Vector3(static_cast<Ogre::Real>(jointPositionX), static_cast<Ogre::Real>(jointPositionY), static_cast<Ogre::Real>(jointPositionZ));
					}*/

					attributeElement = attributeElement->NextSiblingElement();
				}
				// jointPosition = this->gameObjectPtr->getPosition() + jointPosition;

				/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Required force: "
				+ Ogre::StringConverter::toString(partBreakForce) + " required break torque force: " + Ogre::StringConverter::toString(partBreakTorque));*/
				// connect the parts

				this->createJoint(part1, part2, this->gameObjectPtr->getPosition(), partBreakForce, partBreakTorque);

				/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent] JointPosition: "
				+ Ogre::StringConverter::toString(jointPosition));*/

				connectionElement = connectionElement->NextSiblingElement();
			}
			childElement = childElement->NextSiblingElement();
		}

		Ogre::Real wholeMass = 0.0f;
		size_t middleId = this->parts.size() / 2;
		unsigned int i = 0;
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			// The middle part is the main body
			if (i == middleId)
			{
				this->physicsBody = it->second->getBody();
			}
			i++;
			wholeMass += it->second->getBody()->getMass();
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent] Whole mass: "
			+ Ogre::StringConverter::toString(wholeMass));


		return true;
	}

	void PhysicsActiveDestructableComponent::createJoint(SplitPart* part1, SplitPart* part2, const Ogre::Vector3& jointPosition, Ogre::Real breakForce, Ogre::Real breakTorque)
	{
		Ogre::String jointName1 = "DestructableJoint" + Ogre::StringConverter::toString(this->jointCount) + "_" + this->gameObjectPtr->getName();
		this->jointCount++;
		Ogre::String jointName2 = "DestructableJoint" + Ogre::StringConverter::toString(this->jointCount) + "_" + this->gameObjectPtr->getName();
		this->jointCount++;

		auto& jointHingeCompPtr1 = boost::make_shared<JointHingeComponent>();
		jointHingeCompPtr1->setType(JointHingeComponent::getStaticClassName());
		jointHingeCompPtr1->setBody(part1->getBody());
		// jointHingeCompPtr1->setLimitsEnabled(true);
		// jointHingeCompPtr1->setSpring(true, 0.9f, 0.0f, 10.0f);
		// either or, both will not work together because of max dof
		// jointHingeCompPtr1->setMinMaxAngleLimit(Ogre::Degree(0.0f), Ogre::Degree(0.0f));
		// jointHingeCompPtr1->setBrakeForce(100.0f);
		// do not collide with other parts!
		jointHingeCompPtr1->setJointRecursiveCollisionEnabled(false);
		jointHingeCompPtr1->setStiffness(0.99f);
		jointHingeCompPtr1->setBreakForce(breakForce);
		jointHingeCompPtr1->setBreakTorque(breakTorque);
		// jointHingeCompPtr1->setBreakForce(breakForce);
		// jointHingeCompPtr1->setBreakTorque(breakTorque);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent] Creating joint part: " + jointName1);
		AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(jointHingeCompPtr1);

		// connect the current joint to the predecessor, (internally gets its body for joint creation)
		auto& jointHingeCompPtr2 = boost::make_shared<JointHingeComponent>();
		jointHingeCompPtr2->setType(JointHingeComponent::getStaticClassName());
		jointHingeCompPtr2->setPredecessorId(jointHingeCompPtr1->getId());
		jointHingeCompPtr2->connectPredecessorCompPtr(jointHingeCompPtr1);
		jointHingeCompPtr2->setBody(part2->getBody());
		// jointHingeCompPtr2->setLimitsEnabled(true);
		// jointHingeCompPtr2->setMinMaxAngleLimit(Ogre::Degree(0.0f), Ogre::Degree(0.0f));
		jointHingeCompPtr2->setBreakForce(breakForce);
		jointHingeCompPtr2->setBreakTorque(breakTorque);
		// jointHingeCompPtr2->setSpring(true, 0.9f, 0.0f, 10.0f);
		
		// jointHingeCompPtr2->setBreakForce(breakForce);
		// jointHingeCompPtr2->setBreakTorque(breakTorque);
		// do not collide with other parts!
		// either or, both will not work together because of max dof
		jointHingeCompPtr2->setJointRecursiveCollisionEnabled(false);
		jointHingeCompPtr2->setStiffness(0.99f);
		 Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent] Creating joint part: " + jointName2);
		AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(jointHingeCompPtr2);


		jointHingeCompPtr1->setPredecessorId(jointHingeCompPtr2->getId());
		jointHingeCompPtr1->connectPredecessorCompPtr(jointHingeCompPtr2);

		// calculate the pin direction from one part and another 
		Ogre::Vector3 direction = part1->getPartPosition() - part2->getPartPosition();
		direction.normalise();
		// take the orientation of the game object into account
		jointHingeCompPtr2->setPin(this->initialOrientation * direction); // direction to neighbour (predecessor)
		bool success = jointHingeCompPtr2->createJoint(jointPosition);
		jointHingeCompPtr2->getJoint()->setCollisionState(0);
		if (false == success)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Could not create joint of type hinge for joint1 name: "
				+ jointName1 + " joint2 name: " + jointName2 + " for game object: " + this->gameObjectPtr->getName());
		}

		// set configured break values
		part1->setBreakForce(breakForce);
		part1->setBreakTorque(breakTorque);
		part1->addJointComponent(jointHingeCompPtr1);
		
		part2->setBreakForce(breakForce);
		part2->setBreakTorque(breakTorque);
		part2->addJointComponent(jointHingeCompPtr2);

		/*Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Joint1 name: "
		+ jointName1 + " required force to break: " + Ogre::StringConverter::toString(breakForce) + " torque force to break: "
		+ Ogre::StringConverter::toString(breakTorque) + " for game object: " + this->gameObjectPtr->getName());*/

		// do not add to GOC, because it would the joints too!
	}

	Ogre::DataStreamPtr PhysicsActiveDestructableComponent::execSplitMesh(void)
	{
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		Ogre::Item* item = nullptr;
		if (nullptr == entity)
		{
			item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr == item)
			{
				return Ogre::DataStreamPtr();
			}
		}

		// Only split if there is yet no file
		Ogre::DataStreamPtr stream;
		try
		{
			stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->meshSplitConfigFile);
		}
		catch (const Ogre::FileNotFoundException&)
		{
			/*
			* -smooth -relsize 0.35 -cut_surface "ice" -rel_uv 2 -random "Data/meteor.mesh" "Data/BrokenMeteor/"
			* MeshSplitter.exe -smooth -relsize 0.35 -cut_surface "Material/SOLID/TEX/Case3.png" -random "Data/Case3.mesh" "Data/BrockenCase3/"
			* MeshSplitter.exe -smooth -relsize 0.35 -cut_surface "Brick1_big1" -random "Data2/Brick1_big.mesh" "Data2/"
			* # D:\Ogre\GameEngineDevelopment\media\models>MeshSplitter.exe
			* MeshSplitter.exe -smooth -relsize 0.35 -cut_surface "Material/SOLID/TEX/tower1_Cube.tga" -random "Structures/tower.01.mesh" "Structures/"
			* Note: Folder must exist
			*/
			
			Ogre::String applicationFilePathName = Core::getSingletonPtr()->replaceSlashes(Core::getSingletonPtr()->getApplicationFilePathName(), false);
			Ogre::String applicationFolder = Core::getSingletonPtr()->getDirectoryNameFromFilePathName(applicationFilePathName);
			Ogre::String rootFolder;

			size_t found = applicationFolder.find("/bin/");
			if (found != Ogre::String::npos)
			{
				rootFolder = applicationFolder.substr(0, found);
			}
			else
			{
				return Ogre::DataStreamPtr();
			}

			// applicationFolder = Ogre::StringUtil::replaceAll(applicationFolder, "Debug", "Release");

			Ogre::String exeName = "/MeshSplitterExec.exe";
			size_t findDebug = applicationFolder.find("Debug");
			if (Ogre::String::npos != findDebug)
			{
				exeName = "/MeshSplitterExec_d.exe";
			}

			// "D:\Ogre\GameEngineDevelopment\bin\Release\MeshSplitterExec.exe"
			Ogre::String meshSplitFilePathName = applicationFolder + exeName;
			meshSplitFilePathName = Core::getSingletonPtr()->replaceSlashes(meshSplitFilePathName, false);

			// "Material/SOLID/TEX/case1.png"
			Ogre::String datablockName;
			Ogre::FileInfoListPtr fileInfoList;
				
			if (nullptr != entity)
			{
				datablockName = *entity->getSubEntity(0)->getDatablock()->getNameStr();
				Ogre::String meshResourceFolderName = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource(entity->getMesh()->getName());
				fileInfoList = Ogre::FileInfoListPtr(Ogre::ResourceGroupManager::getSingletonPtr()->findResourceFileInfo(meshResourceFolderName, entity->getMesh()->getName()));
				if (fileInfoList->empty())
				{
					return Ogre::DataStreamPtr();
				}
			}
			else
			{
				datablockName = *item->getSubItem(0)->getDatablock()->getNameStr();
				Ogre::String meshResourceFolderName = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource(item->getMesh()->getName());
				fileInfoList = Ogre::FileInfoListPtr(Ogre::ResourceGroupManager::getSingletonPtr()->findResourceFileInfo(meshResourceFolderName, item->getMesh()->getName()));
				if (fileInfoList->empty())
				{
					return Ogre::DataStreamPtr();
				}
			}

			if (fileInfoList.isNull())
			{
				return Ogre::DataStreamPtr();
			}

			// "D:\Ogre\GameEngineDevelopment\media/models/Objects"
			Ogre::String sourceFolder = fileInfoList->at(0).archive->getName();
			sourceFolder = rootFolder + "/" + sourceFolder.substr(6, sourceFolder.length());

			// "D:\Ogre\GameEngineDevelopment\media\models\Objects\Case1.mesh"
			Ogre::String sourceFile;
			if (nullptr != entity)
			{
				sourceFile = sourceFolder + "/" + entity->getMesh()->getName();
			}
			else
			{
				sourceFile = sourceFolder + "/" + item->getMesh()->getName();
			}
				
			sourceFile = Core::getSingletonPtr()->replaceSlashes(sourceFile, false);

			// "D:\Ogre\GameEngineDevelopment\media\models\Destructable\"
			Ogre::String destinationFolder = rootFolder + "/media/models/Destructable/";
			destinationFolder = Core::getSingletonPtr()->replaceSlashes(destinationFolder, false);

			Ogre::String param;
			param += "-smooth ";
			param += "-relsize ";
			param += "0.35 ";
			param += "-cut_surface ";
			param += "\"";
			param += datablockName;
			param += "\" ";
			param += "-random ";
			param += "\"";
			param += sourceFile;
			param += "\" ";
			param += "\"";
			param += destinationFolder;
			param += "\"";

			// D:\Ogre\GameEngineDevelopment\external\MeshSplitterDemos\OgrePhysX04_Demo_Win\bin>MeshSplitter.exe -smooth -relsize 0.35 -cut_surface "Material/SOLID/TEX/case1.png" -random "D:\Ogre\GameEngineDevelopment\media/models/Objects\Case1.mesh" "D:\Ogre\GameEngineDevelopment\media\models\Destructable\"

			STARTUPINFO startupInfo;
			memset(&startupInfo, 0, sizeof(startupInfo));
			startupInfo.cb = sizeof(startupInfo);
			startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			startupInfo.wShowWindow = SW_HIDE;
			PROCESS_INFORMATION processInformation;

			BOOL result = ::CreateProcess(meshSplitFilePathName.c_str(), (char*)param.c_str(), nullptr, nullptr, FALSE,
				CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, nullptr, applicationFolder.c_str(), &startupInfo, &processInformation);

			if (result)
			{
				// Wait for external application to finish
				WaitForSingleObject(processInformation.hProcess, INFINITE);

				DWORD exitCode = 0;
				// Get the exit code.
				result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

				// Close the handles.
				CloseHandle(processInformation.hProcess);
				CloseHandle(processInformation.hThread);

				if (FALSE == result)
				{
					// Could not get exit code.
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Could not get exit code from MeshSplitterExec.");
					
					return Ogre::DataStreamPtr();
				}
			}
			else
			{
				Ogre::String lastError = Ogre::StringConverter::toString(GetLastError());
				Core::getSingletonPtr()->displayError("Could not create process for MeshSplitterExec.exe", GetLastError());

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent] Cannot continue, because the MeshSplitter detected an error during mesh split process for mesh file: "
					+ this->meshSplitConfigFile + " for game object: " + this->gameObjectPtr->getName() + ".");

				// Sent event with feedback
				boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{MeshSplitCreationFail}"));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
			}

			try
			{
				stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->meshSplitConfigFile);
			}
			catch (const Ogre::FileNotFoundException&)
			{
				// File still does not exist
				// Sent event with feedback
				boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{MeshSplitCreationFail}"));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);

					return Ogre::DataStreamPtr();
			}
		}

		return stream;
	}

	Ogre::String PhysicsActiveDestructableComponent::getClassName(void) const
	{
		return "PhysicsActiveDestructableComponent";
	}

	Ogre::String PhysicsActiveDestructableComponent::getParentClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsActiveDestructableComponent::getParentParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	void PhysicsActiveDestructableComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			PhysicsComponent::update(dt, notSimulating);

			for (auto& it = this->parts.begin(); it != this->parts.end(); ++it)
			{
				for (auto& jointCompPtr : it->second->getJointComponents())
				{
					jointCompPtr->update(dt);
				}
			}

			//for (auto& it = this->parts.begin(); it != this->parts.end(); ++it)
			//{
			//	/*Ogre::Real breakForceLength = it->second->getBody()->getForce().length();
			//	Ogre::Real breakTorqueLength = it->second->getBody()->getOmega().length();*/

			//	// PhysicsActiveDestructableComponent::SplitPart* part = OgreNewt::any_cast<PhysicsActiveDestructableComponent::SplitPart*>(body->getUserData());
			//	// go through all joint handlers for a split part and if the acting force or torgue is stronger as the configured value, 
			//	for (auto& jointCompPtr : it->second->getJointComponents())
			//	{
			//		if (nullptr == jointCompPtr->getJoint())
			//			continue;

			//		Ogre::Real breakForceLength = jointCompPtr->getJoint()->getForce0().length();
			//		Ogre::Real breakTorqueLength = jointCompPtr->getJoint()->getTorque0().length();

			//		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Acting Force: "
			//			+ Ogre::StringConverter::toString(breakForceLength) + " torque: " + Ogre::StringConverter::toString(breakTorqueLength)
			//			+ " Required Force: " + Ogre::StringConverter::toString(it->second->getBreakForce()) + " Torque: " + Ogre::StringConverter::toString(it->second->getBreakTorque()));

			//		// check if the force acting on the body is higher as the specified force to break the joint
			//		if (breakForceLength >= it->second->getBreakForce() || breakTorqueLength >= it->second->getBreakTorque())
			//		{
			//			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Force high enough!!!!");
			//			// If at least one part is broken, hide the placeholder scene node and show all parts
			//			if (false == this->firstTimeBroken && true == this->motionless->getBool())
			//			{
			//				this->gameObjectPtr->getSceneNode()->setVisible(false);
			//				
			//				it->second->getSceneNode()->setVisible(true);
	
			//				this->firstTimeBroken = true;
			//			}

			//			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Acting Force: "
			//			// 	+ Ogre::StringConverter::toString(body->getForce().squaredLength()) + " required break force: " + Ogre::StringConverter::toString(part->getBreakForce()));
			//			// can be called by x-configured threads, therefore lock here the remove operation
			//			// mutex.lock();

			//			if (jointCompPtr->getJoint())
			//			{
			//				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Part: "
			//					+ jointCompPtr->getName() + " broken, break force: " + Ogre::StringConverter::toString(it->second->getBreakForce())
			//					+ " break torque: " + Ogre::StringConverter::toString(it->second->getBreakTorque())
			//					+ " acting force: " + Ogre::StringConverter::toString(breakForceLength)
			//					+ " acting torque: " + Ogre::StringConverter::toString(breakTorqueLength));

			//				it->second->setMotionLess(false);
			//				this->motionless->setValue(false);

			//				this->delayedDeleteJointComponentList.emplace(jointCompPtr);
			//			}
			//			// Also check all its predecessors and remove the joints if necessary
			//			auto& predecessorJointComponent = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(jointCompPtr->getPredecessorId()));
			//			if (predecessorJointComponent && predecessorJointComponent->getJoint())
			//			{
			//				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Predecessor Part: "
			//					+ jointCompPtr->getName() + " broken, break force: " + Ogre::StringConverter::toString(it->second->getBreakForce())
			//					+ " break torque: " + Ogre::StringConverter::toString(it->second->getBreakTorque())
			//					+ " acting force: " + Ogre::StringConverter::toString(breakForceLength)
			//					+ " acting torque: " + Ogre::StringConverter::toString(breakTorqueLength));

			//				this->delayedDeleteJointComponentList.emplace(predecessorJointComponent);
			//			}
			//			// mutex.unlock();
			//			break;
			//		}
			//	}
			//}

			//// check if there are any to be deleted joint handlers and release them
			//for (auto& it = this->delayedDeleteJointComponentList.cbegin(); it != this->delayedDeleteJointComponentList.cend();)
			//{
			//	JointCompPtr jointCompPtr = *it;

			//	if (jointCompPtr)
			//	{
			//		jointCompPtr->releaseJoint();
			//		it++;
			//	}
			//	else
			//	{
			//		++it;
			//	}

			//}
			//this->delayedDeleteJointComponentList.clear();
			// if the mesh is configured as motionless, apply the current position to each part. The object will behave like a normal static physics object
			if (this->motionless)
			{
				for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
				{
					if (it->second->isMotionLess())
						// if (it->second->getJointHandlerPtr()->getJoint())
					{
						it->second->getBody()->setPositionOrientation(this->initialPosition, this->initialOrientation);
					}
				}
			}
		}
	}

	void PhysicsActiveDestructableComponent::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsActiveDestructableComponent::AttrBreakForce() == attribute->getName())
		{
			this->breakForce->setValue(attribute->getReal());
		}
		else if (PhysicsActiveDestructableComponent::AttrBreakTorque() == attribute->getName())
		{
			this->breakTorque->setValue(attribute->getReal());
		}
		else if (PhysicsActiveDestructableComponent::AttrDensity() == attribute->getName())
		{
			this->density->setValue(attribute->getReal());
		}
		else if (PhysicsActiveDestructableComponent::AttrMeshMotionless() == attribute->getName())
		{
			this->motionless->setValue(attribute->getBool());
		}
		else if (PhysicsActiveDestructableComponent::AttrSplitMesh() == attribute->getName())
		{
			this->execSplitMesh();
		}
	}

	void PhysicsActiveDestructableComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MeshBreakForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->breakForce->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MeshBreakTorque"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->breakTorque->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MeshDensity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->density->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MeshMotionless"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->motionless->getBool())));
		propertiesXML->append_node(propertyXML);

		// PhysicsActiveComponent::writeCompoundGameObjectProperties(propertiesXML, doc);
	}

	void PhysicsActiveDestructableComponent::setMotionLess(bool motionless)
	{
		this->motionless->setValue(motionless);
	}

	PhysicsActiveDestructableComponent::SplitPart* PhysicsActiveDestructableComponent::getOrCreatePart(const Ogre::String& meshName)
	{
		auto& find = this->parts.find(meshName);
		if (find != this->parts.cend())
		{
			return find->second;
		}

		SplitPart* part = new SplitPart(this, meshName, this->gameObjectPtr->getSceneManager());
		this->parts.emplace(meshName, part);

		return part;
	}

	void PhysicsActiveDestructableComponent::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			// this->parts[0] wrong because its a map
			it->second->getBody()->setPositionOrientation(Ogre::Vector3(x, y, z), this->parts[0]->getBody()->getOrientation());
		}
		PhysicsComponent::setPosition(Ogre::Vector3(x, y, z));
	}

	void PhysicsActiveDestructableComponent::setPosition(const Ogre::Vector3& position)
	{
		this->gameObjectPtr->setAttributePosition(position);
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			it->second->getBody()->setPositionOrientation(position, this->parts[0]->getBody()->getOrientation());
		}
		PhysicsComponent::setPosition(position);
	}

	void PhysicsActiveDestructableComponent::translate(const Ogre::Vector3& relativePosition)
	{
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			it->second->getBody()->setPositionOrientation(it->second->getBody()->getPosition() + relativePosition, this->parts[0]->getBody()->getOrientation());
		}
	}

	void PhysicsActiveDestructableComponent::addImpulse(const Ogre::Vector3& deltaVector)
	{
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			// Attention: 1.0f / 60.0f
			it->second->getBody()->addImpulse(deltaVector, it->second->getBody()->getPosition(), 1.0f / 60.0f);
		}
	}

	void PhysicsActiveDestructableComponent::setOrientation(const Ogre::Quaternion& orientation)
	{
		this->gameObjectPtr->setAttributeOrientation(orientation);
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			it->second->getBody()->setPositionOrientation(it->second->getBody()->getPosition(), orientation);
		}
		PhysicsComponent::setOrientation(orientation);
	}

	void PhysicsActiveDestructableComponent::setVelocity(const Ogre::Vector3& velocity)
	{
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			it->second->getBody()->setVelocity(velocity);
		}
	}

	void PhysicsActiveDestructableComponent::setBreakForce(Ogre::Real breakForce)
	{
		this->breakForce->setValue(breakForce);
	}

	void PhysicsActiveDestructableComponent::setBreakTorque(Ogre::Real breakTorque)
	{
		this->breakTorque->setValue(breakTorque);
	}

	void PhysicsActiveDestructableComponent::setDensity(Ogre::Real density)
	{
		this->density->setValue(density);
	}

	void PhysicsActiveDestructableComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		for (auto& it = this->parts.cbegin(); it != this->parts.cend(); it++)
		{
			if (false == this->activated->getBool())
			{
				it->second->getBody()->freeze();
			}
			else
			{
				it->second->getBody()->unFreeze();
				// it->second->getBody()->setVelocity(Ogre::Vector3(0.0f, 0.1f, 0.0f));
			}
		}
	}

	////////////////////////////////////////////////////SplitPart////////////////////////////////////////////////////////////////////////////////////////

	PhysicsActiveDestructableComponent::SplitPart::SplitPart(PhysicsActiveDestructableComponent* physicsActiveDestructableComponent, 
		const Ogre::String& meshName, Ogre::SceneManager* sceneManager)
		: physicsActiveDestructableComponent(physicsActiveDestructableComponent),
		sceneManager(physicsActiveDestructableComponent->getOwner()->getSceneManager()),
		entity(nullptr),
		item(nullptr),
		sceneNode(nullptr),
		splitPartBody(nullptr),
		partPosition(Ogre::Vector3::ZERO),
		breakForce(0.0f),
		motionless(physicsActiveDestructableComponent->motionless->getBool())
	{
		// Attention: here with getOwner()->getSceneNode()->createChildSceneNode() ??
		this->sceneNode = this->physicsActiveDestructableComponent->getOwner()->getSceneNode()->createChildSceneNode();
		this->sceneNode->setName("DestructableNode"
			+ Ogre::StringConverter::toString(this->physicsActiveDestructableComponent->parts.size()) + "_" + this->physicsActiveDestructableComponent->getOwner()->getUniqueName());

		if (GameObject::ENTITY == this->physicsActiveDestructableComponent->getOwner()->getType())
		{
			this->entity = physicsActiveDestructableComponent->getOwner()->getSceneManager()->createEntity(meshName);
			this->sceneNode->attachObject(entity);
		}
		else if (GameObject::ITEM == this->physicsActiveDestructableComponent->getOwner()->getType())
		{
			this->item = physicsActiveDestructableComponent->getOwner()->getSceneManager()->createItem(meshName);
			this->sceneNode->attachObject(item);
		}
		
		Ogre::Vector3 originScale = this->physicsActiveDestructableComponent->getOwner()->getSceneNode()->getScale();
		this->sceneNode->setScale(originScale);

		// set the part invisible, so that it looks as if its a normal mesh object
		if (true == this->physicsActiveDestructableComponent->motionless->getBool())
		{
			this->sceneNode->setVisible(false);
		}
	
		// set the collision hull smaller as the object, because its convex and hence would collide with another piece else
		Ogre::Real shapeSizeFactor = 1.0f;


		// Attention: the position of all parts is always ZERO! But each part's bounding box center is away from the ZERO by an offset, where the part actually is located
		// For prove take the OgreMeshViewer load a part, activate the gizmo axis and see the offset from the axis, also see the bounding box center value, this value is
		// exactly the offset of the part! Hence playing with body->getPosition does not make sense, because its always ZERO, but for joints a position and pin direction is
		// required, hence the direction is the difference of two part positions and from the XML the joint position is gathered.
// Attention: Is this correct?

		Ogre::Vector3 boundingBoxHalfSize = Ogre::Vector3::ZERO;
		Ogre::Vector3 scale = originScale * shapeSizeFactor;
		if (GameObject::ENTITY == this->physicsActiveDestructableComponent->getOwner()->getType())
		{
			this->partPosition = (entity->getMesh()->getBounds().getCenter()) * originScale;
			boundingBoxHalfSize = entity->getMesh()->getBounds().getHalfSize() * scale;
		}
		else if (GameObject::ITEM == this->physicsActiveDestructableComponent->getOwner()->getType())
		{
			this->partPosition = (item->getMesh()->getAabb().mCenter) * originScale;
			boundingBoxHalfSize = item->getMesh()->getAabb().mHalfSize * scale;
		}
		/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] BoundingBox center: "
			+ Ogre::StringConverter::toString(this->partPosition));*/

// Attention: Is this correct?
		
		Ogre::Vector3 scaledOffset = this->partPosition * shapeSizeFactor;
		Ogre::Vector3 collisionOffset = this->partPosition - scaledOffset;

		Ogre::Vector3 inertia = Ogre::Vector3::ZERO;
		Ogre::Vector3 massOrigin = Ogre::Vector3::ZERO;
		// how to do with volume for buoyancy, since when the object will be split in parts, what is with volume?

		if (boundingBoxHalfSize.x > 0.005f && boundingBoxHalfSize.y > 0.005f && boundingBoxHalfSize.z > 0.005f)
		{
			// square scale the collision, e.g. when scene node is scaled to 2, 2, 2 the collision scale will be 4, 4, 4
			// when scene node is scaled to 0.5, 0.5, 0.5 the collision scale will be 0.25, 0.25, 0.25
			// This works because the collision is made of peaces and each peace has an offset position, so scaling will also more offsetting the position in all directions
			OgreNewt::CollisionPrimitives::ConvexHull* col = nullptr;
			if (GameObject::ENTITY == this->physicsActiveDestructableComponent->getOwner()->getType())
			{
				col = new OgreNewt::CollisionPrimitives::ConvexHull(
				this->physicsActiveDestructableComponent->getOgreNewt(), entity, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, 0.001f, scale * scale);
			}
			else if (GameObject::ITEM == this->physicsActiveDestructableComponent->getOwner()->getType())
			{
				col = new OgreNewt::CollisionPrimitives::ConvexHull(
					this->physicsActiveDestructableComponent->getOgreNewt(), item, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, 0.001f, scale * scale);
			}
			OgreNewt::CollisionPtr collisionPtr = OgreNewt::CollisionPtr(col);
			// calculate interia and mass center of the body
			col->calculateInertialMatrix(inertia, massOrigin);

			this->splitPartBody = new OgreNewt::Body(this->physicsActiveDestructableComponent->getOgreNewt(), this->physicsActiveDestructableComponent->gameObjectPtr->getSceneManager(), collisionPtr);
		}
		else
		{
			// is boundingBoxHalfSize correct? Breakpoint if ever this case does occur
			this->splitPartBody = new OgreNewt::Body(this->physicsActiveDestructableComponent->getOgreNewt(), this->physicsActiveDestructableComponent->gameObjectPtr->getSceneManager(),
				this->physicsActiveDestructableComponent->createCollisionPrimitive("Box", Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY, boundingBoxHalfSize * scale, inertia, massOrigin, this->physicsActiveDestructableComponent->getOwner()->getCategoryId()));
		}
		// this->splitPartBody->setJointRecursiveCollision(0);

		this->splitPartBody->setGravity(this->physicsActiveDestructableComponent->gravity->getVector3());

		// calculate mass for each part
// Attention: Is this correct?
		Ogre::v1::Entity* ownerEntity = nullptr;
		Ogre::Item* ownerItem = nullptr;
		Ogre::Vector3 splitPartBoundingBoxSize = Ogre::Vector3::ZERO;
		Ogre::Vector3 originalBoundingBoxSize = Ogre::Vector3::ZERO;

		if (GameObject::ENTITY == this->physicsActiveDestructableComponent->getOwner()->getType())
		{
			ownerEntity = this->physicsActiveDestructableComponent->getOwner()->getMovableObjectUnsafe<Ogre::v1::Entity>();
			splitPartBoundingBoxSize = ownerEntity->getMesh()->getBounds().getSize() * scale;
			originalBoundingBoxSize = entity->getMesh()->getBounds().getSize() * scale;
		}
		else if (GameObject::ITEM == this->physicsActiveDestructableComponent->getOwner()->getType())
		{
			ownerItem = this->physicsActiveDestructableComponent->getOwner()->getMovableObjectUnsafe<Ogre::Item>();
			splitPartBoundingBoxSize = ownerItem->getMesh()->getAabb().getSize() * scale;
			originalBoundingBoxSize = item->getMesh()->getAabb().getSize() * scale;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Error cannot create split part, because the game object has no entity with mesh for game object: " + this->physicsActiveDestructableComponent->getOwner()->getName());
			return;
			// throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsActiveDestructableComponent::SplitPart] Error cannot create split part, because the game object has no entity with mesh for game object: " + this->physicsActiveDestructableComponent->getOwner()->getName() + ".\n", "NOWA");
		}

		Ogre::Vector3 splitPartFractionVolume = splitPartBoundingBoxSize / originalBoundingBoxSize;

		Ogre::Real originalVolume = originalBoundingBoxSize.x * originalBoundingBoxSize.y * originalBoundingBoxSize.z;
		Ogre::Real splitPartVolume = splitPartFractionVolume.x * splitPartFractionVolume.y * splitPartFractionVolume.z;
		Ogre::Real splitPartMass = splitPartVolume * this->physicsActiveDestructableComponent->getMass() / static_cast<Ogre::Real>(this->physicsActiveDestructableComponent->partsCount);
		// prevent object with 0 mass, because else they would be immobile and another part that is connected with that one, would also be fixed forever!
		if (splitPartMass <= 0.1f)
		{
			splitPartMass = 1.0f;
		}

		// splitPartMass = 50.0f;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Mass: "
			+ Ogre::StringConverter::toString(splitPartMass));

		this->splitPartBody->setMassMatrix(splitPartMass, inertia * splitPartMass);
		this->splitPartBody->setCenterOfMass(massOrigin);

		Ogre::Vector3 originalCenterOffset = this->physicsActiveDestructableComponent->getOwner()->getCenterOffset();

		/*this->splitPartBody->setPositionOrientation((originalCenterOffset - this->physicsActiveDestructableComponent->getOwner()->getPosition())
			+ (this->physicsActiveDestructableComponent->getOwner()->getOrientation() * splitPartPosition),
			this->physicsActiveDestructableComponent->getOwner()->getOrientation());*/

		/*this->splitPartBody->setPositionOrientation(this->physicsActiveDestructableComponent->getOwner()->getPosition() + collisionOffset - originalCenterOffset,
			this->physicsActiveDestructableComponent->getOwner()->getOrientation());*/

		Ogre::Real linearDamping = this->physicsActiveDestructableComponent->getLinearDamping();
		Ogre::Vector3 angularDamping = this->physicsActiveDestructableComponent->getAngularDamping();

		// if (this->linearDamping != 0.0f)
		{
			this->splitPartBody->setLinearDamping(linearDamping);
		}
		// if (this->angularDamping != Ogre::Vector3::ZERO)
		{
			this->splitPartBody->setAngularDamping(angularDamping);
		}

		this->splitPartBody->setCustomForceAndTorqueCallback<PhysicsActiveDestructableComponent::SplitPart>(&PhysicsActiveDestructableComponent::SplitPart::splitPartMoveCallback, this);
		// pin the object stand in pose and not fall down
		// this->applyPose(this->splitPartBody, this->pose);

		if (false == this->physicsActiveDestructableComponent->isActivated())
		{
			this->splitPartBody->freeze();
		}
		else
		{
			this->splitPartBody->unFreeze();
			this->splitPartBody->setAutoSleep(0);
		}

		// set user data for ogrenewt
		this->splitPartBody->setUserData(OgreNewt::Any(this));

		this->splitPartBody->attachNode(this->sceneNode);

		this->splitPartBody->setPositionOrientation(this->physicsActiveDestructableComponent->getOwner()->getPosition(), this->physicsActiveDestructableComponent->getOwner()->getOrientation());

		this->splitPartBody->setCollidable(this->physicsActiveDestructableComponent->collidable->getBool());

		// Important: Set the type and material group id for each piece the same as the root body for material pair functionality and collision callbacks
		this->splitPartBody->setType(this->physicsActiveDestructableComponent->getOwner()->getCategoryId());
		// this->splitPartBody->setMaterialGroupID(this->physicsActiveDestructableComponent->getBody()->getMaterialGroupID());

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->physicsActiveDestructableComponent->gameObjectPtr.get(),
			this->physicsActiveDestructableComponent->getOgreNewt());
		this->splitPartBody->setMaterialGroupID(materialId);

		// OgreNewt::MaterialPair* materialPair = new OgreNewt::MaterialPair(this->physicsActiveDestructableComponent->getOgreNewt(), this->splitPartBody->getMaterialGroupID(), this->splitPartBody->getMaterialGroupID());
		// set the data
		// materialPair->setDefaultCollidable(0);
	}

	PhysicsActiveDestructableComponent::SplitPart::~SplitPart()
	{
		if (nullptr != this->entity)
		{
// Attention: is this correct?
			if (this->sceneManager->hasMovableObject(this->entity))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Destroying entity: "
					+ this->entity->getName());

				this->sceneManager->destroyEntity(this->entity);
				this->entity = nullptr;
			}
		}
		else if (nullptr != this->item)
		{
			// Attention: is this correct?
			if (this->sceneManager->hasMovableObject(this->item))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Destroying item: "
					+ this->item->getName());

				this->sceneManager->destroyItem(this->item);
				this->item = nullptr;
			}
		}
		if (this->sceneNode)
		{
			auto nodeIt = this->sceneNode->getChildIterator();
			while (nodeIt.hasMoreElements())
			{
				//go through all scenenodes in the scene
				Ogre::Node* subNode = nodeIt.getNext();
				subNode->removeAllChildren();
				//add them to the tree as parents
			}

			this->sceneNode->detachAllObjects();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Destroying scene node: "
				+ this->sceneNode->getName());
			this->sceneManager->destroySceneNode(this->sceneNode);
		}

		for (auto& it = this->jointComponents.begin(); it != this->jointComponents.end(); ++it)
		{
			(*it).reset();
		}
		this->jointComponents.clear();

		if (this->splitPartBody)
		{
			this->splitPartBody->setCollision(OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Null(this->physicsActiveDestructableComponent->ogreNewt)));
			if (this->physicsActiveDestructableComponent->physicsBody == this->splitPartBody)
			{
				this->physicsActiveDestructableComponent->physicsBody = nullptr;
			}
			this->splitPartBody->removeForceAndTorqueCallback();
			this->splitPartBody->removeNodeUpdateNotify();
			this->splitPartBody->detachNode();
			this->splitPartBody->removeDestructorCallback();
			delete this->splitPartBody;
			this->splitPartBody = nullptr;
		}
	}

	OgreNewt::Body* PhysicsActiveDestructableComponent::SplitPart::getBody()
	{
		return this->splitPartBody;
	}

	Ogre::Vector3 PhysicsActiveDestructableComponent::SplitPart::getPartPosition(void) const
	{
		return this->partPosition;
	}

	void PhysicsActiveDestructableComponent::SplitPart::setBreakForce(Ogre::Real breakForce)
	{
		this->breakForce = breakForce;
	}

	Ogre::Real PhysicsActiveDestructableComponent::SplitPart::getBreakForce(void) const
	{
		return this->breakForce;
	}

	void PhysicsActiveDestructableComponent::SplitPart::setBreakTorque(Ogre::Real breakTorque)
	{
		this->breakTorque = breakTorque;
	}

	Ogre::Real PhysicsActiveDestructableComponent::SplitPart::getBreakTorque(void) const
	{
		return this->breakTorque;
	}

	void PhysicsActiveDestructableComponent::SplitPart::addJointComponent(JointCompPtr jointCompPtr)
	{
		this->jointComponents.emplace_back(jointCompPtr);
	}

	std::vector<JointCompPtr>& PhysicsActiveDestructableComponent::SplitPart::getJointComponents(void)
	{
		return this->jointComponents;
	}

	void PhysicsActiveDestructableComponent::SplitPart::setMotionLess(bool motionless)
	{
		this->motionless = motionless;
	}

	bool PhysicsActiveDestructableComponent::SplitPart::isMotionLess(void) const
	{
		return this->motionless;
	}

	Ogre::SceneNode* PhysicsActiveDestructableComponent::SplitPart::getSceneNode(void) const
	{
		return this->sceneNode;
	}

	void PhysicsActiveDestructableComponent::SplitPart::splitPartMoveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		this->physicsActiveDestructableComponent->moveCallback(body, timeStep, threadIndex);

		//// will not work here, because force is 0 above, why???
		//Ogre::Real mass;
		//Ogre::Vector3 inertia;
		//// calculate gravity
		//body->getMassMatrix(mass, inertia);
		//Ogre::Vector3 force = this->physicsActiveDestructableComponent->getGravity();
		//force *= mass;
		//// add the force that pushed towards earth
		//body->setForce(force);

		//Ogre::Real breakForceLength = body->getForce().length();
		//Ogre::Real breakTorqueLength = body->getOmega().length();

		//PhysicsActiveDestructableComponent::SplitPart* part = OgreNewt::any_cast<PhysicsActiveDestructableComponent::SplitPart*>(body->getUserData());
		//// go through all joint handlers for a split part and if the acting force or torgue is stronger as the configured value, 
		//for (auto& jointCompPtr : part->getJointComponents())
		//{
		//	Ogre::Real breakForceLength = Ogre::Math::Abs(boost::static_pointer_cast<JointHingeComponent>(jointCompPtr)->getActingForce());
		//	/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Acting Force: "
		//		+ Ogre::StringConverter::toString(breakForceLength) + " torque: " + Ogre::StringConverter::toString(breakTorqueLength)
		//		+ " Required Force: " + Ogre::StringConverter::toString(part->getBreakForce()) + " Torque: " + Ogre::StringConverter::toString(part->getBreakTorque()));*/

		//	// check if the force acting on the body is higher as the specified force to break the joint
		//	if (breakForceLength >= part->getBreakForce()/* * part->getBreakForce()*/ /*|| breakTorqueLength >= part->getBreakTorque()*/ /** part->getBreakTorque()*/)
		//	{
		//		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Force high enough!!!!");
		//		// If at least one part is broken, hide the placeholder scene node and show all parts
		//		if (false == this->physicsActiveDestructableComponent->firstTimeBroken && true == this->physicsActiveDestructableComponent->motionless->getBool())
		//		{
		//			this->physicsActiveDestructableComponent->gameObjectPtr->getSceneNode()->setVisible(false);
		//			for (auto& it = this->physicsActiveDestructableComponent->parts.cbegin(); it != this->physicsActiveDestructableComponent->parts.cend(); it++)
		//			{
		//				it->second->sceneNode->setVisible(true);
		//			}
		//			this->physicsActiveDestructableComponent->firstTimeBroken = true;
		//		}

		//		/* Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Acting Force: "
		//		 	+ Ogre::StringConverter::toString(body->getForce().squaredLength()) + " required break force: " + Ogre::StringConverter::toString(part->getBreakForce()));*/
		//		// can be called by x-configured threads, therefore lock here the remove operation
		////		mutex.lock();
		//			NewtonWorldCriticalSectionLock(body->getWorld()->getNewtonWorld(), threadIndex);

		//		if (jointCompPtr->getJoint())
		//		{
		//			/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveDestructableComponent::SplitPart] Part: "
		//				+ jointCompPtr->getName() + " broken, break force: " + Ogre::StringConverter::toString(part->getBreakForce())
		//				+ " break torque: " + Ogre::StringConverter::toString(part->getBreakTorque())
		//				+ " acting force: " + Ogre::StringConverter::toString(breakForceLength)
		//				+ " acting torque: " + Ogre::StringConverter::toString(breakTorqueLength));*/

		//			part->setMotionLess(false);
		//			this->physicsActiveDestructableComponent->motionless->setValue(false);

		//			this->physicsActiveDestructableComponent->delayedDeleteJointComponentList.emplace(jointCompPtr);
		//		}
		//		// Also check all its predecessors and remove the joints if necessary
		//		auto& predecessorJointComponent = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(jointCompPtr->getPredecessorId()));
		//		if (predecessorJointComponent && predecessorJointComponent->getJoint())
		//		{
		//			/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Predecessor Part: "
		//				+ jointCompPtr->getName() + " broken, break force: " + Ogre::StringConverter::toString(part->getBreakForce())
		//				+ " break torque: " + Ogre::StringConverter::toString(part->getBreakTorque())
		//				+ " acting force: " + Ogre::StringConverter::toString(breakForceLength)
		//				+ " acting torque: " + Ogre::StringConverter::toString(breakTorqueLength));*/

		//			this->physicsActiveDestructableComponent->delayedDeleteJointComponentList.emplace(predecessorJointComponent);
		//		}

		////		mutex.unlock();
		//      NewtonWorldCriticalSectionUnlock(body->getWorld()->getNewtonWorld(), threadIndex);
		//	}
		//}
	}

}; // namespace end