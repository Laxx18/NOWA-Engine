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

	class SetMirrorDatablockProcess : public NOWA::Process
	{
	public:
		explicit SetMirrorDatablockProcess(PlanarReflectionComponent* planarReflectionComponent)
			: planarReflectionComponent(planarReflectionComponent)
		{

		}
	protected:
		virtual void onInit(void) override
		{
			this->succeed();
			this->planarReflectionComponent->setDatablockName(planarReflectionComponent->getDatablockName());
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	private:
		PlanarReflectionComponent* planarReflectionComponent;
	};

	PlanarReflectionComponent::PlanarReflectionComponent()
		: GameObjectComponent(),
		transformUpdateTimer(0.0f),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		trackedRenderable(nullptr),
		planarReflectionMeshCreated(false),
		useAccurateLighting(new Variant(PlanarReflectionComponent::AttrAccurateLighting(), true, this->attributes)),
		imageSize(new Variant(PlanarReflectionComponent::AttrImageSize(), Ogre::Vector2(512.0f, 512.0f), this->attributes)),
		useMipmaps(new Variant(PlanarReflectionComponent::AttrMipmaps(), true, this->attributes)),
		useMipmapMethodCompute(new Variant(PlanarReflectionComponent::AttrMipmapMethodCompute(), true, this->attributes)),
		mirrorSize(new Variant(PlanarReflectionComponent::AttrMirrorSize(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		datablockName(new Variant(PlanarReflectionComponent::AttrDatablockName(), "NOWAGlassRoughness2", this->attributes)),
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
	}

	PlanarReflectionComponent::~PlanarReflectionComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanarReflectionComponent] Destructor planar reflection component for game object: " + this->gameObjectPtr->getName());
	}

	bool PlanarReflectionComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

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

		this->createPlane();

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
					Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
					if (nullptr != entity)
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
			}
			else
			{
				// Modify position if larger than epsilon
				if (this->gameObjectPtr->getPosition().squaredDistance(this->oldPosition) > 0.1f * 0.1f || false == this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(2.0f)))
				{
					if (nullptr != this->trackedRenderable)
					{
						Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
						if (nullptr != entity)
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
					this->oldOrientation = this->gameObjectPtr->getOrientation();
					this->oldPosition = this->gameObjectPtr->getPosition();
				}
			}
		}
	}

#if 0
	void PlanarReflectionComponent::createPlanarReflection(void)
	{
		if (nullptr != this->gameObjectPtr && nullptr != WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections())
		{
			this->oldPosition = this->gameObjectPtr->getPosition();
			this->oldOrientation = this->gameObjectPtr->getOrientation();

			this->gameObjectPtr->setUseReflection(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);
			// Create new mirror plane
			Ogre::String meshName = this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

			// Problem: When there is a planar reflection active, the plane must not be destroyed!, else crash, 
			// so check if it needs to be created for the first time, if not and mirror size is changed

			// Check if it has an item, but not a missing one!
			if (nullptr == this->gameObjectPtr->getMovableObject<Ogre::Item>() || "Missing.mesh" == this->gameObjectPtr->getMovableObject<Ogre::Item>()->getMesh()->getName())
			{
				Ogre::Item* item = nullptr;

				Ogre::ResourcePtr resourcePtr = Ogre::MeshManager::getSingletonPtr()->getResourceByName(meshName);

				if (nullptr != resourcePtr)
				{
					Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
					Ogre::MeshManager::getSingletonPtr()->remove(resourcePtr->getHandle());
				}

				// Do not scale twice!
				Ogre::Vector2 planeScale = Ogre::Vector2::UNIT_SCALE;
				if (1.0f == this->gameObjectPtr->getSceneNode()->getScale().x)
					planeScale = this->mirrorSize->getVector2().x;
				if (1.0f == this->gameObjectPtr->getSceneNode()->getScale().y)
					planeScale = this->mirrorSize->getVector2().y;

				Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(this->gameObjectPtr->getName() + "_PlaneMirrorUnlit", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
					Ogre::Plane(Ogre::Vector3::UNIT_Z, 0.0f), planeScale.x, planeScale.y, 1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y,
					Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

				// MathHelper::getInstance()->ensureHasTangents(planeMeshV1.dynamicCast<Ogre::v1::Mesh>());

				Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(this->gameObjectPtr->getName() + "_PlaneMirrorUnlit", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);

				planeMeshV1 = Ogre::v1::MeshManager::getSingletonPtr()->createPlane(meshName, "General", Ogre::Plane(Ogre::Vector3::UNIT_Z, 0.0f), planeScale.x, planeScale.y,
					1, 1, true, 1, 1, 1, Ogre::Vector3::UNIT_Y, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

				// Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);

				// Later: Make scene node and item static!
				item = this->gameObjectPtr->getSceneManager()->createItem(planeMesh, Ogre::SCENE_DYNAMIC);
				item->setName(this->gameObjectPtr->getName() + "mesh");
				// MathHelper::getInstance()->substractOutTangentsForShader(item);

				// Check whether to cast shadows
				bool castShadows = this->gameObjectPtr->getMovableObject()->getCastShadows();
				bool visible = this->gameObjectPtr->getMovableObject()->getVisible();
				// this->gameObjectPtr->setDynamic(true);
				item->setCastShadows(castShadows);
				this->gameObjectPtr->getSceneNode()->attachObject(item);

				// Set the here newly created item for this game object
				this->gameObjectPtr->init(item);

				item->setVisible(visible);
				this->gameObjectPtr->getSceneNode()->setVisible(visible);

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
				// Register after the component has been created
				AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
			}
			else
			{
				this->gameObjectPtr->getSceneNode()->setScale(Ogre::Vector3(this->mirrorSize->getVector2().x, this->mirrorSize->getVector2().y, 1.0f));
				// If there is a physics component, it needs to be scaled too
				auto& physicsCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PhysicsComponent>());
				if (nullptr != physicsCompPtr)
					physicsCompPtr->setScale(this->gameObjectPtr->getSceneNode()->getScale());
			}

			bool canUseComputeMipmaps = Ogre::Root::getSingletonPtr()->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_COMPUTE_PROGRAM);
			if (false == canUseComputeMipmaps)
			{
				this->useMipmapMethodCompute->setValue(false);
			}

			this->planarReflectionMeshCreated = true;

			// this->setDatablockName("NOWAGlassRoughness");
			this->setDatablockName(this->datablockName->getString());

			NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(1.0f));
			delayProcess->attachChild(NOWA::ProcessPtr(new SetMirrorDatablockProcess(this)));
			NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
		}
	}
#endif

	void PlanarReflectionComponent::createPlane(void)
	{
		if (nullptr != this->gameObjectPtr && nullptr != WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections() && false == this->planarReflectionMeshCreated)
		{
			this->oldPosition = this->gameObjectPtr->getPosition();
			this->oldOrientation = this->gameObjectPtr->getOrientation();

			this->gameObjectPtr->setUseReflection(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(false);
			this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);

			WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->removePlanarReflectionsActor(this->gameObjectPtr->getId());

			// Create new mirror plane
			Ogre::String meshName = this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

			// Do not scale twice!
			Ogre::Vector2 planeScale = Ogre::Vector2::UNIT_SCALE;
			if (1.0f == this->gameObjectPtr->getSceneNode()->getScale().x)
				planeScale = this->mirrorSize->getVector2().x;
			if (1.0f == this->gameObjectPtr->getSceneNode()->getScale().y)
				planeScale = this->mirrorSize->getVector2().y;

			Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(this->gameObjectPtr->getName() + "_PlaneMirrorUnlit", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				Ogre::Plane(Ogre::Vector3::UNIT_Z, 0.0f), planeScale.x, planeScale.y, 1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Y,
				Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

			// MathHelper::getInstance()->ensureHasTangents(planeMeshV1.dynamicCast<Ogre::v1::Mesh>());

			Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(this->gameObjectPtr->getName() + "_PlaneMirrorUnlit", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);

			planeMeshV1 = Ogre::v1::MeshManager::getSingletonPtr()->createPlane(meshName, "General", Ogre::Plane(Ogre::Vector3::UNIT_Z, 0.0f), planeScale.x, planeScale.y,
				1, 1, true, 1, 1, 1, Ogre::Vector3::UNIT_Y, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

			// Check whether to cast shadows
			bool castShadows = this->gameObjectPtr->getMovableObject()->getCastShadows();
			bool visible = this->gameObjectPtr->getMovableObject()->getVisible();

			// Later: Make scene node and entity static!
			Ogre::Item* item = this->gameObjectPtr->getSceneManager()->createItem(planeMesh, Ogre::SCENE_DYNAMIC);

			this->gameObjectPtr->setDynamic(true);
			item->setCastShadows(castShadows);
			this->gameObjectPtr->getSceneNode()->attachObject(item);

			// Set the here newly created entity for this game object
			this->gameObjectPtr->init(item);

			// Get the used data block name 0
			/*Ogre::String dataBlock = this->gameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0")->getString();
			if ("Missing" != dataBlock)
				item->setDatablock(dataBlock);*/

			item->setName(this->gameObjectPtr->getName() + "mesh");

			// When plane is re-created actualize the data block component so that the plane gets the data block

			// Get the used data block name, if not set
			if (true == this->datablockName->getString().empty())
			{
				this->datablockName->setValue(this->gameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0")->getString());
			}

			/*
			Ogre::HlmsPbsDatablock* sourceDataBlock = static_cast<Ogre::HlmsPbsDatablock*>(Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablock(this->datablockName->getString()));
			if (nullptr != sourceDataBlock)
			{
				Ogre::String clonedDatablockName = *sourceDataBlock->getNameStr() + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
				bool duplicateItem = false;

				do
				{
					try
					{
						this->clonedDatablock = sourceDataBlock->clone(clonedDatablockName);
						duplicateItem = false;
					}
					catch (const Ogre::Exception& exception)
					{
						duplicateItem = true;
						clonedDatablockName = *sourceDataBlock->getNameStr() + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(static_cast<int>(Ogre::Math::RangeRandom(0.0f, 100.0f)));
					}
				} while (true == duplicateItem);

				if (nullptr != this->clonedDatablock)
				{
					item->setDatablock(this->clonedDatablock);
				}
			}
			
			for (size_t i = 0; i < item->getNumSubItems(); i++)
			{
				auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
				if (nullptr != sourceDataBlock)
				{
					// Deactivate fresnel by default, because it looks ugly
					if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
					{
						sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
					}
				}
			}*/


			item->setVisible(visible);
			this->gameObjectPtr->getSceneNode()->setVisible(visible);

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

			// this->setDatablockName(this->datablockName->getString());

			// Set first other datablock (causes in Ogre internal refresh etc.) and in delay process, the actual one is set
			/*Ogre::String oldDatablockName = this->datablockName->getString();
			if ("NOWAGlassRoughness" == this->datablockName->getString())
			{
				this->setDatablockName("NOWAGlassRoughness2");
			}
			else
			{
				this->setDatablockName("NOWAGlassRoughness");
			}
			this->setDatablockName(oldDatablockName);*/

			NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(1.0f));
			delayProcess->attachChild(NOWA::ProcessPtr(new SetMirrorDatablockProcess(this)));
			NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
		}
	}

	void PlanarReflectionComponent::actualizePlanarReflection(void)
	{
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
			// WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->removePlanarReflectionsActor(this->gameObjectPtr->getId());
			this->setDatablockName(attribute->getString());

			/*Ogre::String oldDatablockName = attribute->getString();
			if ("NOWAGlassRoughness" == oldDatablockName)
			{
				this->setDatablockName("NOWAGlassRoughness2");
			}
			else
			{
				this->setDatablockName("NOWAGlassRoughness");
			}
			this->setDatablockName(oldDatablockName);*/

		}
		else if (PlanarReflectionComponent::AttrTransformUpdateRateSec() == attribute->getName())
		{
			this->setTransformUpdateRateSec(attribute->getReal());
		}
	}

	void PlanarReflectionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

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

		if (nullptr == this->gameObjectPtr || false == this->planarReflectionMeshCreated)
			return;

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
				auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(0)->getDatablock());
				Ogre::TextureGpu* roughnessTexture = datablock->getTexture(Ogre::PBSM_ROUGHNESS);
				if (nullptr != roughnessTexture)
				{
					Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

					roughnessTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
					hlmsTextureManager->waitForStreamingCompletion();
				}
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

			if (nullptr != this->trackedRenderable)
			{
				WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections()->removeRenderable(this->trackedRenderable->renderable);

				delete this->trackedRenderable;
			}

			this->trackedRenderable = new Ogre::PlanarReflections::TrackedRenderable(item->getSubItem(0), item, Ogre::Vector3::UNIT_Z, Ogre::Vector3(0, 0, 0));
			WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->getPlanarReflections()->addRenderable(*trackedRenderable);
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