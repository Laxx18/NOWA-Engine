#include "NOWAPrecompiled.h"
#include "PlanarReflectionComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/DeployResourceModule.h"
#include "modules/WorkspaceModule.h"
#include "WorkspaceComponents.h"
#include "PlaneComponent.h"
#include "PhysicsComponent.h"
#include "PhysicsArtifactComponent.h"
#include "main/ProcessManager.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PlanarReflectionComponent::PlanarReflectionComponent()
		: GameObjectComponent(),
		transformUpdateTimer(0.0f),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		trackedRenderable(nullptr),
		planarReflectionMeshCreated(false),
		mirrorPlaneItem(nullptr),
		useAccurateLighting(new Variant(PlanarReflectionComponent::AttrAccurateLighting(), true, this->attributes)),
		imageSize(new Variant(PlanarReflectionComponent::AttrImageSize(), Ogre::Vector2(512.0f, 512.0f), this->attributes)),
		useMipmaps(new Variant(PlanarReflectionComponent::AttrMipmaps(), true, this->attributes)),
		useMipmapMethodCompute(new Variant(PlanarReflectionComponent::AttrMipmapMethodCompute(), true, this->attributes)),
		mirrorSize(new Variant(PlanarReflectionComponent::AttrMirrorSize(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		datablockName(new Variant(PlanarReflectionComponent::AttrDatablockName(), "NOWAGlassRoughness", this->attributes)),
		transformUpdateRateSec(new Variant(PlanarReflectionComponent::AttrTransformUpdateRateSec(), 0.5f, this->attributes))
	{
		this->useAccurateLighting->setDescription("When true, overall scene CPU usage may be higher, but lighting information in the reflections will"
			"be accurate. Turning it to false is faster. This is a performance optimization that rarely has a noticeable impact on quality."
			"Note: Value change will be applied when scene is loaded next time.");
		this->imageSize->setDescription("Resolution of the RTT assigned to this reflection plane."
			"Note: Value change will be applied when scene is loaded next time.");
		this->useMipmaps->setDescription("When true, mipmaps will be created; which are useful for glossy reflections (i.e. roughness close to 1)."
			"Set this to false if you only plan on using it for a mirror (which are always a perfect reflection... unless you want to use a roughness map to add scratches and stains to it)."
			"Note: Value change will be applied when scene is loaded next time.");

		this->useMipmapMethodCompute->setDescription("Set to true if the workspace assigned via PlanarReflectionActor::workspaceName will filter the RTT with a compute filter (usually for higher quality)."
			"Note: Value change will be applied when scene is loaded next time.");

		// this->mirrorSize->setDescription("The mirror plane size.");
		this->datablockName->setDescription("Datablock to set for more heavy mirror computation. When left empty cheaper unlit is used automatically.");
		this->transformUpdateRateSec->setDescription("Sets the update rate at which the mirror will be re-adjusted. Default is each halb second due to performance reasons.");

		this->oldDatablockName = this->datablockName->getString();
	}

	PlanarReflectionComponent::~PlanarReflectionComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanarReflectionComponent] Destructor planar reflection component for game object: " + this->gameObjectPtr->getName());
	}

	bool PlanarReflectionComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AccurateLighting")
		{
			this->setUseAccurateLighting(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ImageSize")
		{
			this->setImageSize(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Mipmaps")
		{
			this->setUseMipmaps(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MipmapMethodCompute")
		{
			this->setUseMipmapMethodCompute(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MirrorSize")
		{
			this->setMirrorSize(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DatablockName")
		{
			this->datablockName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TransformUpdateRateSec")
		{
			this->setTransformUpdateRateSec(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PlanarReflectionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PlanarReflectionCompPtr clonedCompPtr(boost::make_shared<PlanarReflectionComponent>());

		clonedCompPtr->setUseAccurateLighting(this->useAccurateLighting->getBool());
		clonedCompPtr->setImageSize(this->imageSize->getVector2());
		clonedCompPtr->setUseMipmaps(this->useMipmaps->getBool());
		clonedCompPtr->setUseMipmapMethodCompute(this->useMipmapMethodCompute->getBool());
		clonedCompPtr->setMirrorSize(this->mirrorSize->getVector2());
		clonedCompPtr->setDatablockName(this->datablockName->getString());
		clonedCompPtr->setTransformUpdateRateSec(this->transformUpdateRateSec->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PlanarReflectionComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanarReflectionComponent] Init planar reflection component for game object: " + this->gameObjectPtr->getName());

		this->setDatablockName(this->datablockName->getString());

		return true;
	}

	void PlanarReflectionComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
		if (nullptr != item)
		{
			WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
			if (nullptr != workspaceBaseComponent)
			{
				auto planarReflections = workspaceBaseComponent->getPlanarReflections();
				if (nullptr != planarReflections)
				{
					planarReflections->removeRenderable(item->getSubItem(0));
					workspaceBaseComponent->removePlanarReflectionsActor(this->gameObjectPtr->getId());
				}
				if (nullptr != this->trackedRenderable)
				{
					delete this->trackedRenderable;
					this->trackedRenderable = nullptr;
				}
			}
		}
		this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(true);
	}

	void PlanarReflectionComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->transformUpdateTimer += dt;
		// Check if mirror has been rotated in an specific interval
		if (this->transformUpdateTimer >= this->transformUpdateRateSec->getReal())
		{
			this->transformUpdateTimer = 0.0f;

			if (this->transformUpdateRateSec->getReal() == 0.0f)
			{
				if (nullptr != this->trackedRenderable)
				{
					WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
					if (nullptr != workspaceBaseComponent && true == workspaceBaseComponent->getUsePlanarReflection())
					{
						workspaceBaseComponent->setPlanarMaxReflections(this->gameObjectPtr->getId(), this->useAccurateLighting->getBool(),
							static_cast<unsigned int>(this->imageSize->getVector2().x), static_cast<unsigned int>(this->imageSize->getVector2().y),
							this->useMipmaps->getBool(), this->useMipmapMethodCompute->getBool(), this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(),
							this->mirrorSize->getVector2());
					}
				}
			}
			else
			{
				// Modify position if larger than epsilon
				if (this->gameObjectPtr->getPosition().squaredDistance(this->oldPosition) > 0.1f * 0.1f || false == this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(2.0f)))
				{
					if (nullptr != this->trackedRenderable)
					{
						WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
						if (nullptr != workspaceBaseComponent && true == workspaceBaseComponent->getUsePlanarReflection())
						{
							workspaceBaseComponent->setPlanarMaxReflections(this->gameObjectPtr->getId(), this->useAccurateLighting->getBool(),
								static_cast<unsigned int>(this->imageSize->getVector2().x), static_cast<unsigned int>(this->imageSize->getVector2().y),
								this->useMipmaps->getBool(), this->useMipmapMethodCompute->getBool(), this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(),
								this->mirrorSize->getVector2());
						}
					}
					this->oldOrientation = this->gameObjectPtr->getOrientation();
					this->oldPosition = this->gameObjectPtr->getPosition();
				}
			}
		}
	}

	void PlanarReflectionComponent::createPlane(void)
	{
		if (nullptr == WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent())
		{
			return;
		}

		if ((nullptr != this->gameObjectPtr && nullptr != WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections() && false == this->planarReflectionMeshCreated)
			|| (nullptr == this->mirrorPlaneItem))
		{
			this->oldPosition = this->gameObjectPtr->getPosition();
			this->oldOrientation = this->gameObjectPtr->getOrientation();

			this->gameObjectPtr->setUseReflection(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);

			WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->removePlanarReflectionsActor(this->gameObjectPtr->getId());

			// Create new mirror plane
			Ogre::String meshName = this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_PlaneMirrorUnlit";

			// Do not scale twice!
			Ogre::Vector2 planeScale = Ogre::Vector2::UNIT_SCALE;
			if (1.0f == this->gameObjectPtr->getSceneNode()->getScale().x)
				planeScale = this->mirrorSize->getVector2().x;
			if (1.0f == this->gameObjectPtr->getSceneNode()->getScale().y)
				planeScale = this->mirrorSize->getVector2().y;

			Ogre::ResourcePtr resourceV1 = Ogre::v1::MeshManager::getSingletonPtr()->getResourceByName(meshName);
			// Destroy a potential plane v1, because an error occurs (plane with name ... already exists)
			if (nullptr != resourceV1)
			{
				Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
				Ogre::v1::MeshManager::getSingletonPtr()->remove(resourceV1->getHandle());
			}

			// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
			Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(meshName);
			if (nullptr != resourceV2)
			{
				Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
				Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
			}

			Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				Ogre::Plane(Ogre::Vector3::UNIT_Z, 0.0f), planeScale.x, planeScale.y, 1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y,
				Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

			Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);

			// Later: Make scene node and entity static!
			this->mirrorPlaneItem = this->gameObjectPtr->getSceneManager()->createItem(planeMesh, Ogre::SCENE_DYNAMIC);

			this->gameObjectPtr->setDynamic(true);
			this->mirrorPlaneItem->setCastShadows(this->gameObjectPtr->getCastShadows());
			this->gameObjectPtr->getSceneNode()->attachObject(this->mirrorPlaneItem);

			// Set the here newly created entity for this game object
			this->gameObjectPtr->init(this->mirrorPlaneItem);

			// Get the used data block name 0
			/*Ogre::String dataBlock = this->gameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0")->getString();
			if ("Missing" != dataBlock)
				this->mirrorPlaneItem->setDatablock(dataBlock);*/

			this->mirrorPlaneItem->setName(this->gameObjectPtr->getName() + "mesh");

			// When plane is re-created actualize the data block component so that the plane gets the data block

			// Get the used data block name, if not set
			if (true == this->datablockName->getString().empty())
			{
				this->datablockName->setValue(this->gameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0")->getString());
			}

			this->mirrorPlaneItem->setVisible(this->gameObjectPtr->isVisible());

			// Check if a physics artifact component does exist for this game object and recreate the collision shape when the plane has changed
			auto& physicsArtifactComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
			if (nullptr != physicsArtifactComponent)
			{
				// Attention: what is with joints in post init, is it required to recreate also the joints, if the collision does change for the physics body??????????
				if (nullptr != physicsArtifactComponent->getBody())
				{
					physicsArtifactComponent->reCreateCollision();
				}
				else
				{
					physicsArtifactComponent->createStaticBody();
				}
			}

			// - 1 else other v2 items will not by visible
			this->gameObjectPtr->setRenderQueueIndex(NOWA::RENDER_QUEUE_V2_MESH - 1);

			// Register after the component has been created
			AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);


			bool canUseComputeMipmaps = Ogre::Root::getSingletonPtr()->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_COMPUTE_PROGRAM);
			if (false == canUseComputeMipmaps)
			{
				this->useMipmapMethodCompute->setValue(false);
			}

			this->gameObjectPtr->getSceneNode()->setScale(Ogre::Vector3(this->mirrorSize->getVector2().x, this->mirrorSize->getVector2().y, 1.0f));
			// If there is a physics component, it needs to be scaled too
			auto& physicsCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PhysicsComponent>());
			if (nullptr != physicsCompPtr)
				physicsCompPtr->setScale(this->gameObjectPtr->getSceneNode()->getScale());


			this->planarReflectionMeshCreated = true;
		}
	}

	void PlanarReflectionComponent::destroyPlane(void)
	{
		if (nullptr == WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent())
		{
			return;
		}

		// Note: This is enough, as in game object init(...) the old movable object is destroyed, if there comes in a new movable object pointer
		this->mirrorPlaneItem = nullptr;
		if (nullptr != this->trackedRenderable)
		{
			WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections()->removeRenderable(this->trackedRenderable->renderable);

			delete this->trackedRenderable;
			this->trackedRenderable = nullptr;
		}
	}

	void PlanarReflectionComponent::actualizePlanarReflection(void)
	{
		if (nullptr == WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent())
		{
			return;
		}

		if (nullptr != this->gameObjectPtr && nullptr != WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections())
		{
			WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->setPlanarMaxReflections(this->gameObjectPtr->getId(), this->useAccurateLighting->getBool(),
				static_cast<unsigned int>(this->imageSize->getVector2().x), static_cast<unsigned int>(this->imageSize->getVector2().y),
				this->useMipmaps->getBool(), this->useMipmapMethodCompute->getBool(), this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(),
				this->mirrorSize->getVector2());
		}
	}

	void PlanarReflectionComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PlanarReflectionComponent::AttrAccurateLighting() == attribute->getName())
		{
			this->setUseAccurateLighting(attribute->getBool());
		}
		else if (PlanarReflectionComponent::AttrImageSize() == attribute->getName())
		{
			this->setImageSize(attribute->getVector2());
		}
		else if (PlanarReflectionComponent::AttrMipmaps() == attribute->getName())
		{
			this->setUseMipmaps(attribute->getBool());
		}
		else if (PlanarReflectionComponent::AttrMipmapMethodCompute() == attribute->getName())
		{
			this->setUseMipmapMethodCompute(attribute->getBool());
		}
		else if (PlanarReflectionComponent::AttrMirrorSize() == attribute->getName())
		{
			this->setMirrorSize(attribute->getVector2());
		}
		else if (PlanarReflectionComponent::AttrDatablockName() == attribute->getName())
		{
			this->setDatablockName(attribute->getString());
		}
		else if (PlanarReflectionComponent::AttrTransformUpdateRateSec() == attribute->getName())
		{
			this->setTransformUpdateRateSec(attribute->getReal());
		}
	}

	void PlanarReflectionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "AccurateLighting"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useAccurateLighting->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ImageSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->imageSize->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Mipmaps"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useMipmaps->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MipmapMethodCompute"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useMipmapMethodCompute->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MirrorSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mirrorSize->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DatablockName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablockName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TransformUpdateRateSec"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->transformUpdateRateSec->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PlanarReflectionComponent::getClassName(void) const
	{
		return "PlanarReflectionComponent";
	}

	Ogre::String PlanarReflectionComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PlanarReflectionComponent::setUseAccurateLighting(bool accurateLighting)
	{
		this->useAccurateLighting->setValue(accurateLighting);
	}

	bool PlanarReflectionComponent::getUseAccurateLighting(void) const
	{
		return this->useAccurateLighting->getBool();
	}

	void PlanarReflectionComponent::setImageSize(const Ogre::Vector2& imageSize)
	{
		this->imageSize->setValue(imageSize);
		this->actualizePlanarReflection();
	}

	Ogre::Vector2 PlanarReflectionComponent::getImageSize(void) const
	{
		return this->imageSize->getVector2();
	}

	void PlanarReflectionComponent::setUseMipmaps(bool useMipmaps)
	{
		this->useMipmaps->setValue(useMipmaps);
		this->actualizePlanarReflection();
	}

	bool PlanarReflectionComponent::getUseMipmaps(void) const
	{
		return this->useMipmaps->getBool();
	}

	void PlanarReflectionComponent::setUseMipmapMethodCompute(bool useMipmapMethodCompute)
	{
		this->useMipmapMethodCompute->setValue(useMipmapMethodCompute);
		this->actualizePlanarReflection();
	}

	bool PlanarReflectionComponent::getUseMipmapMethodCompute(void) const
	{
		return this->useMipmapMethodCompute->getBool();
	}

	void PlanarReflectionComponent::setMirrorSize(const Ogre::Vector2& mirrorSize)
	{
		this->mirrorSize->setValue(mirrorSize);
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->getSceneNode()->setScale(Ogre::Vector3(this->mirrorSize->getVector2().x, this->mirrorSize->getVector2().y, 1.0f));
			// If there is a physics component, it needs to be scaled too
			auto& physicsCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PhysicsComponent>());
			if (nullptr != physicsCompPtr)
				physicsCompPtr->setScale(this->gameObjectPtr->getSceneNode()->getScale());
		}
		this->actualizePlanarReflection();
	}

	Ogre::Vector2 PlanarReflectionComponent::getMirrorSize(void) const
	{
		return this->mirrorSize->getVector2();
	}

	void PlanarReflectionComponent::setDatablockName(const Ogre::String& datablockName)
	{
		this->datablockName->setValue(datablockName);

		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		if (false == this->planarReflectionMeshCreated)
		{
			this->destroyPlane();
			this->createPlane();
		}

		WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		if (nullptr != workspaceBaseComponent && false == workspaceBaseComponent->getUsePlanarReflection())
		{
			return;
		}

		WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->setPlanarMaxReflections(this->gameObjectPtr->getId(), this->useAccurateLighting->getBool(),
			static_cast<unsigned int>(this->imageSize->getVector2().x), static_cast<unsigned int>(this->imageSize->getVector2().y),
			this->useMipmaps->getBool(), this->useMipmapMethodCompute->getBool(), this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(),
			this->mirrorSize->getVector2());

		Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
		if (nullptr != item)
		{
			item->setVisibilityFlags(1u); // Do not render this plane during the reflection phase.

			if (false == this->datablockName->getString().empty())
			{
				item->setDatablock(this->datablockName->getString());
			}
			else
			{
				Ogre::Hlms* hlmsUnlit = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);

				Ogre::HlmsMacroblock macroblock;
				Ogre::HlmsBlendblock blendblock;

				Ogre::String datablockName("Mirror_Unlit");
				Ogre::HlmsUnlitDatablock* mirror = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock(datablockName, datablockName, macroblock, blendblock, Ogre::HlmsParamVec()));

				// mPlanarReflections->reserve(0, actor);
				//Make sure it's always activated (i.e. always win against other actors)
				//unless it's not visible by the camera.
				mirror->setTexture(0, WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections()->getTexture(0));
				mirror->setEnablePlanarReflection(0, true);
				item->setDatablock(mirror);
			}

			WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->addPlanarReflectionsActor(this->gameObjectPtr->getId(), this->useAccurateLighting->getBool(),
				static_cast<unsigned int>(this->imageSize->getVector2().x), static_cast<unsigned int>(this->imageSize->getVector2().y),
				this->useMipmaps->getBool(), this->useMipmapMethodCompute->getBool(), this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(),
				this->mirrorSize->getVector2());

			if (nullptr == this->trackedRenderable)
			{
				this->trackedRenderable = new Ogre::PlanarReflections::TrackedRenderable(item->getSubItem(0), item, Ogre::Vector3::UNIT_Z, Ogre::Vector3(0, 0, 0));
				WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections()->addRenderable(*trackedRenderable);
			}
		}
	}

	Ogre::String PlanarReflectionComponent::getDatablockName(void) const
	{
		return this->datablockName->getString();
	}

	void PlanarReflectionComponent::setTransformUpdateRateSec(Ogre::Real transformUpdateRateSec)
	{
		this->transformUpdateRateSec->setValue(transformUpdateRateSec);
	}

	Ogre::Real PlanarReflectionComponent::getTransformUpdateRateSec(void) const
	{
		return this->transformUpdateRateSec->getReal();
	}

}; // namespace end