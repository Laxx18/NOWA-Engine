#include "NOWAPrecompiled.h"
#include "PccPerPixelGridPlacementComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/WorkspaceComponents.h"

#include "OgreBitwise.h"
#include "OgreAbiUtils.h"
#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"
#include "Cubemaps/OgrePccPerPixelGridPlacement.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PccPerPixelGridPlacementComponent::PccPerPixelGridPlacementComponent()
		: GameObjectComponent(),
		name("PccPerPixelGridPlacementComponent"),
		activated(new Variant(PccPerPixelGridPlacementComponent::AttrActivated(), true, this->attributes)),
		regionCenter(new Variant(PccPerPixelGridPlacementComponent::AttrRegionCenter(), Ogre::Vector3::ZERO, this->attributes)),
		regionHalfSize(new Variant(PccPerPixelGridPlacementComponent::AttrRegionHalfSize(), Ogre::Vector3(50.0f, 5.0f, 50.0f), this->attributes)),
		overlapFactor(new Variant(PccPerPixelGridPlacementComponent::AttrOverlapFactor(), 1.25f, this->attributes)),
		probeResolution(new Variant(PccPerPixelGridPlacementComponent::AttrProbeResolution(), static_cast<unsigned int>(256), this->attributes)),
		nearPlane(new Variant(PccPerPixelGridPlacementComponent::AttrNearPlane(), 0.02f, this->attributes)),
		farPlane(new Variant(PccPerPixelGridPlacementComponent::AttrFarPlane(), 10.0f, this->attributes)),
		useDpm2DArray(new Variant(PccPerPixelGridPlacementComponent::AttrUseDpm2DArray(), false, this->attributes)),
		workspaceBaseComponent(nullptr),
		cameraComponent(nullptr)
	{
		this->overlapFactor->setDescription("Quality/performance trade-off. Higher values create more probe overlap for smoother transitions but reduce performance. Range: 1.0 (minimal overlap) to 2.0 (maximum quality).");
		this->overlapFactor->setConstraints(1.0f, 1.5f);

		this->probeResolution->setDescription("Resolution of each cubemap probe in pixels. Higher values provide better quality but consume more memory. Common values: 128, 256, 512, 1024.");
		std::vector<Ogre::String> resolutions;
		resolutions.push_back("128");
		resolutions.push_back("256");
		resolutions.push_back("512");
		resolutions.push_back("1024");
		this->probeResolution->setListSelectedValue("256");
		this->probeResolution->setValue(resolutions);

		this->nearPlane->setDescription("Near clipping distance for probe rendering. Smaller values capture closer details but may cause depth precision issues.");
		this->nearPlane->setConstraints(0.02f, 1.0f);

		this->farPlane->setDescription("Far clipping distance for probe rendering. Should match the maximum distance you want reflections to show.");
		this->farPlane->setConstraints(1.0f, 1000.0f);

		this->regionCenter->setDescription("World-space center of the AABB region to be covered by PCC probes.");
		this->regionHalfSize->setDescription("Half-dimensions of the AABB region. For example, (10, 5, 10) creates a 20x10x20 region.");

		this->useDpm2DArray->setDescription("Toggle between Dual Paraboloid Mapping (more efficient) and Cubemap Arrays (better quality). DPM is recommended for most use cases.");
	}

	PccPerPixelGridPlacementComponent::~PccPerPixelGridPlacementComponent(void)
	{
		
	}

	void PccPerPixelGridPlacementComponent::initialise()
	{

	}

	const Ogre::String& PccPerPixelGridPlacementComponent::getName() const
	{
		return this->name;
	}

	void PccPerPixelGridPlacementComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PccPerPixelGridPlacementComponent>(PccPerPixelGridPlacementComponent::getStaticClassId(), PccPerPixelGridPlacementComponent::getStaticClassName());
	}

	void PccPerPixelGridPlacementComponent::shutdown()
	{
		// Do nothing here, because its called far too late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void PccPerPixelGridPlacementComponent::uninstall()
	{
		// Do nothing here, because its called far too late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void PccPerPixelGridPlacementComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool PccPerPixelGridPlacementComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RegionCenter")
		{
			this->regionCenter->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RegionHalfSize")
		{
			this->regionHalfSize->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OverlapFactor")
		{
			this->overlapFactor->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ProbeResolution")
		{
			this->probeResolution->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NearPlane")
		{
			this->nearPlane->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FarPlane")
		{
			this->farPlane->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseDpm2DArray")
		{
			this->useDpm2DArray->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PccPerPixelGridPlacementComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool PccPerPixelGridPlacementComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Init component for game object: " + this->gameObjectPtr->getName());

		Ogre::MaterialPtr depthCompMaterial = Ogre::MaterialManager::getSingleton().getByName("PCC/DepthCompressor");
		if (depthCompMaterial.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PccPerPixelGridPlacementComponent] ERROR: PCC/DepthCompressor material not found! " "PCC system cannot function without this material. Please add it to your resources.");
			return false;
		}

		// Get required components
		auto& cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PccPerPixelGridPlacementComponent] Error: CameraComponent is required but not found on game object: " + this->gameObjectPtr->getName());
			return false;
		}
		this->cameraComponent = cameraCompPtr.get();

		auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
		if (nullptr == workspaceBaseCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PccPerPixelGridPlacementComponent] Error: WorkspaceBaseComponent is required but not found on game object: " + this->gameObjectPtr->getName());
			return false;
		}
		this->workspaceBaseComponent = workspaceBaseCompPtr.get();

		// Check for conflicting ReflectionCameraComponent
		auto& reflectionCameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<ReflectionCameraComponent>());
		if (nullptr != reflectionCameraCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PccPerPixelGridPlacementComponent] Error: Cannot use PccPerPixelGridPlacementComponent with ReflectionCameraComponent. "
				"Remove ReflectionCameraComponent first. Game object: " + this->gameObjectPtr->getName());
			return false;
		}

		// Enable PCC in workspace and recreate it
		if (true == this->activated->getBool())
		{
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(false);
			this->workspaceBaseComponent->setUsePCC(true);
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(true);

			// Create the PCC system
			this->createPccSystem();
		}

		return true;
	}

	bool PccPerPixelGridPlacementComponent::connect(void)
	{
		GameObjectComponent::connect();

		return true;
	}

	bool PccPerPixelGridPlacementComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		return true;
	}

	void PccPerPixelGridPlacementComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Destructor component for game object: " + this->gameObjectPtr->getName());

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		this->destroyPccSystem();

		// Disable PCC in workspace
		if (nullptr != this->workspaceBaseComponent)
		{
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(false);
			this->workspaceBaseComponent->setUsePCC(false);
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(true);
		}
	}

	void PccPerPixelGridPlacementComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			// Update camera tracking per frame
			if (nullptr != this->workspaceBaseComponent && nullptr != this->workspaceBaseComponent->getParallaxCorrectedCubemap())
			{
				auto closureFunction = [this](Ogre::Real renderDt)
				{
						this->workspaceBaseComponent->getParallaxCorrectedCubemap()->setUpdatedTrackedDataFromCamera(this->cameraComponent->getCamera());
				};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
			}
		}
	}

	void PccPerPixelGridPlacementComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PccPerPixelGridPlacementComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (PccPerPixelGridPlacementComponent::AttrRegionCenter() == attribute->getName())
		{
			this->setRegionCenter(attribute->getVector3());
		}
		else if (PccPerPixelGridPlacementComponent::AttrRegionHalfSize() == attribute->getName())
		{
			this->setRegionHalfSize(attribute->getVector3());
		}
		else if (PccPerPixelGridPlacementComponent::AttrOverlapFactor() == attribute->getName())
		{
			this->setOverlapFactor(attribute->getReal());
		}
		else if (PccPerPixelGridPlacementComponent::AttrProbeResolution() == attribute->getName())
		{
			this->setProbeResolution(attribute->getUInt());
		}
		else if (PccPerPixelGridPlacementComponent::AttrNearPlane() == attribute->getName())
		{
			this->setNearPlane(attribute->getReal());
		}
		else if (PccPerPixelGridPlacementComponent::AttrFarPlane() == attribute->getName())
		{
			this->setFarPlane(attribute->getReal());
		}
		else if (PccPerPixelGridPlacementComponent::AttrUseDpm2DArray() == attribute->getName())
		{
			this->setUseDpm2DArray(attribute->getBool());
		}
	}

	void PccPerPixelGridPlacementComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RegionCenter"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->regionCenter->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RegionHalfSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->regionHalfSize->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OverlapFactor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->overlapFactor->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ProbeResolution"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->probeResolution->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NearPlane"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearPlane->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FarPlane"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->farPlane->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseDpm2DArray"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useDpm2DArray->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PccPerPixelGridPlacementComponent::getClassName(void) const
	{
		return "PccPerPixelGridPlacementComponent";
	}

	Ogre::String PccPerPixelGridPlacementComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PccPerPixelGridPlacementComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (nullptr == this->workspaceBaseComponent)
		{
			// Not initialized yet, will be handled in postInit
			return;
		}

		if (true == activated)
		{
			// ACTIVATE PCC
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Activating PCC for game object: " + this->gameObjectPtr->getName());

			// Enable PCC in workspace
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(false);
			this->workspaceBaseComponent->setUsePCC(true);
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(true);

			// Check if PCC system needs to be created
			if (nullptr == this->workspaceBaseComponent->getParallaxCorrectedCubemap())
			{
				// Create the PCC system
				this->createPccSystem();
			}
			else
			{
				// PCC system already exists, just force update
				this->forceUpdateAllProbes();
			}
		}
		else
		{
			// DEACTIVATE PCC
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Deactivating PCC for game object: " + this->gameObjectPtr->getName());

			// Destroy PCC system
			this->destroyPccSystem();

			// Disable PCC in workspace
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(false);
			this->workspaceBaseComponent->setUsePCC(false);
			this->workspaceBaseComponent->getAttribute(NOWA::WorkspaceBaseComponent::AttrUsePCC())->setReadOnly(true);
		}
	}

	bool PccPerPixelGridPlacementComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void PccPerPixelGridPlacementComponent::setRegionCenter(const Ogre::Vector3& regionCenter)
	{
		this->regionCenter->setValue(regionCenter);
		if (true == this->activated->getBool())
		{
			this->rebuildProbeGrid();
		}
	}

	Ogre::Vector3 PccPerPixelGridPlacementComponent::getRegionCenter(void) const
	{
		return this->regionCenter->getVector3();
	}

	void PccPerPixelGridPlacementComponent::setRegionHalfSize(const Ogre::Vector3& regionHalfSize)
	{
		this->regionHalfSize->setValue(regionHalfSize);
		if (true == this->activated->getBool())
		{
			this->rebuildProbeGrid();
		}
	}

	Ogre::Vector3 PccPerPixelGridPlacementComponent::getRegionHalfSize(void) const
	{
		return this->regionHalfSize->getVector3();
	}

	void PccPerPixelGridPlacementComponent::setOverlapFactor(Ogre::Real overlapFactor)
	{
		// Clamp overlap factor to safe range
		// Values above 1.5 can cause probe areas to exceed probe shapes
		overlapFactor = Ogre::Math::Clamp(overlapFactor, 1.0f, 1.5f);

		this->overlapFactor->setValue(overlapFactor);
		if (true == this->activated->getBool())
		{
			this->rebuildProbeGrid();
		}
	}

	Ogre::Real PccPerPixelGridPlacementComponent::getOverlapFactor(void) const
	{
		return this->overlapFactor->getReal();
	}

	void PccPerPixelGridPlacementComponent::setProbeResolution(unsigned int probeResolution)
	{
		this->probeResolution->setValue(probeResolution);
		if (true == this->activated->getBool())
		{
			this->rebuildProbeGrid();
		}
	}

	unsigned int PccPerPixelGridPlacementComponent::getProbeResolution(void) const
	{
		return this->probeResolution->getUInt();
	}

	void PccPerPixelGridPlacementComponent::setNearPlane(Ogre::Real nearPlane)
	{
		nearPlane = Ogre::Math::Clamp(nearPlane, 0.02f, 1.0f);

		this->nearPlane->setValue(nearPlane);
		if (true == this->activated->getBool())
		{
			this->rebuildProbeGrid();
		}
	}

	Ogre::Real PccPerPixelGridPlacementComponent::getNearPlane(void) const
	{
		return this->nearPlane->getReal();
	}

	void PccPerPixelGridPlacementComponent::setFarPlane(Ogre::Real farPlane)
	{
		this->farPlane->setValue(farPlane);
		if (true == this->activated->getBool())
		{
			this->rebuildProbeGrid();
		}
	}

	Ogre::Real PccPerPixelGridPlacementComponent::getFarPlane(void) const
	{
		return this->farPlane->getReal();
	}

	void PccPerPixelGridPlacementComponent::setUseDpm2DArray(bool useDpm2DArray)
	{
		this->useDpm2DArray->setValue(useDpm2DArray);
		if (true == this->activated->getBool())
		{
			// Changing DPM mode requires full rebuild
			this->destroyPccSystem();
			this->createPccSystem();
		}
	}

	bool PccPerPixelGridPlacementComponent::getUseDpm2DArray(void) const
	{
		return this->useDpm2DArray->getBool();
	}

	void PccPerPixelGridPlacementComponent::forceUpdateAllProbes(void)
	{
		ENQUEUE_RENDER_COMMAND("PccPerPixelGridPlacementComponent::forceUpdateAllProbes",
		{
			if (nullptr != this->workspaceBaseComponent && nullptr != this->workspaceBaseComponent->getParallaxCorrectedCubemap())
			{
				const Ogre::CubemapProbeVec& probes =
					this->workspaceBaseComponent->getParallaxCorrectedCubemap()->getProbes();

				Ogre::CubemapProbeVec::const_iterator itor = probes.begin();
				Ogre::CubemapProbeVec::const_iterator end = probes.end();

				while (itor != end)
				{
					(*itor)->mDirty = true;
					++itor;
				}

				this->workspaceBaseComponent->getParallaxCorrectedCubemap()->updateAllDirtyProbes();

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Forced update of " + Ogre::StringConverter::toString(probes.size()) + " probes");
			}
		});
	}

	void PccPerPixelGridPlacementComponent::rebuildProbeGrid(void)
	{
		if (nullptr == this->workspaceBaseComponent)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND("PccPerPixelGridPlacementComponent::rebuildProbeGrid",
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,"[PccPerPixelGridPlacementComponent] Rebuilding probe grid for game object: " + this->gameObjectPtr->getName());

			if (nullptr != this->workspaceBaseComponent->getParallaxCorrectedCubemap())
			{
				// Create and configure grid placement
				Ogre::PccPerPixelGridPlacement pccPlacement;

				Ogre::Aabb region(this->regionCenter->getVector3(), this->regionHalfSize->getVector3());
				pccPlacement.setFullRegion(region);

				pccPlacement.setOverlap(Ogre::Vector3(this->overlapFactor->getReal()));

				pccPlacement.setParallaxCorrectedCubemapAuto(this->workspaceBaseComponent->getParallaxCorrectedCubemap());

				// Build the probe grid
				pccPlacement.buildStart(this->probeResolution->getUInt(), this->cameraComponent->getCamera(), Ogre::PFG_RGBA8_UNORM_SRGB, this->nearPlane->getReal(), this->farPlane->getReal());
				pccPlacement.buildEnd();

				// Force update all probes
				this->forceUpdateAllProbes();
			}
		});
	}

	void PccPerPixelGridPlacementComponent::createPccSystem(void)
	{
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Creating PCC system for game object: " + this->gameObjectPtr->getName());

			Ogre::Root* root = Ogre::Root::getSingletonPtr();
			Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
			Ogre::CompositorManager2* compositorManager = root->getCompositorManager2();

			// Force update all probes initially
			this->forceUpdateAllProbes();

			// Get the probe workspace definition (should already be created by WorkspaceBaseComponent)
			Ogre::CompositorWorkspaceDef* probeWorkspaceDef = compositorManager->getWorkspaceDefinition("LocalCubemapsProbeWorkspace");

			if (nullptr == probeWorkspaceDef)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PccPerPixelGridPlacementComponent] Error: LocalCubemapsProbeWorkspace definition not found! "
					"Make sure workspace component has created PCC nodes.");
				return;
			}

			// Create ParallaxCorrectedCubemapAuto
			this->workspaceBaseComponent->setParallaxCorrectedCubemap(new Ogre::ParallaxCorrectedCubemapAuto(Ogre::Id::generateNewId<Ogre::ParallaxCorrectedCubemapAuto>(),root, sceneManager, probeWorkspaceDef));

			// Configure DPM vs Cubemap Arrays
			this->workspaceBaseComponent->getParallaxCorrectedCubemap()->setUseDpm2DArray(this->useDpm2DArray->getBool());

			// The setting may be overridden by the system if not supported
			bool actualDpmMode = this->workspaceBaseComponent->getParallaxCorrectedCubemap()->getUseDpm2DArray();
			if (actualDpmMode != this->useDpm2DArray->getBool())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] DPM 2D Array mode was overridden by system. Using: " + Ogre::String(actualDpmMode ? "DPM 2D Array" : "Cubemap Arrays"));
				this->useDpm2DArray->setValue(actualDpmMode);
			}

			// Create and configure grid placement
			Ogre::PccPerPixelGridPlacement pccPlacement;

			Ogre::Aabb region(this->regionCenter->getVector3(), this->regionHalfSize->getVector3());
			pccPlacement.setFullRegion(region);

			pccPlacement.setOverlap(Ogre::Vector3(this->overlapFactor->getReal()));

			pccPlacement.setParallaxCorrectedCubemapAuto(this->workspaceBaseComponent->getParallaxCorrectedCubemap());

			// Build the probe grid
			pccPlacement.buildStart(this->probeResolution->getUInt(), this->cameraComponent->getCamera(), Ogre::PFG_RGBA8_UNORM_SRGB, this->nearPlane->getReal(), this->farPlane->getReal() );
			pccPlacement.buildEnd();

			// ===== Also need to ensure objects are inside probe regions =====
			auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
			int objectsInRegion = 0;
			Ogre::Aabb pccRegion(this->regionCenter->getVector3(), this->regionHalfSize->getVector3());

			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				auto& gameObjectPtr = it->second;
				if (nullptr != gameObjectPtr)
				{
					Ogre::SceneNode* sceneNode = gameObjectPtr->getSceneNode();
					if (nullptr != sceneNode)
					{
						Ogre::Vector3 pos = sceneNode->_getDerivedPosition();
						if (pccRegion.contains(pos))
						{
							objectsInRegion++;
						}
					}
				}
			}

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,"[PccPerPixelGridPlacementComponent] Objects in PCC region: " + Ogre::StringConverter::toString(objectsInRegion));

			// Connect to HlmsPbs
			Ogre::HlmsManager* hlmsManager = root->getHlmsManager();
			Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));
			hlmsPbs->setParallaxCorrectedCubemap(this->workspaceBaseComponent->getParallaxCorrectedCubemap());

			// Log probe count
			const Ogre::CubemapProbeVec& probes = this->workspaceBaseComponent->getParallaxCorrectedCubemap()->getProbes();

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Created " + Ogre::StringConverter::toString(probes.size()) + " probes in grid");

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PccPerPixelGridPlacementComponent] ===== PCC DEBUG INFO =====");
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Probe count: " + Ogre::StringConverter::toString(probes.size()));
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Region center: " + Ogre::StringConverter::toString(this->regionCenter->getVector3()));
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Region half-size: " + Ogre::StringConverter::toString(this->regionHalfSize->getVector3()));

			// Check first probe details
			if (probes.size() > 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "First probe position: " + Ogre::StringConverter::toString(probes[0]->getProbeCameraPos()));
			}

			// Force update all probes initially
			this->forceUpdateAllProbes();
		};

		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PccPerPixelGridPlacementComponent::createPccSystem");
	}

	void PccPerPixelGridPlacementComponent::destroyPccSystem(void)
	{
		if (nullptr == this->workspaceBaseComponent)
		{
			return;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PccPerPixelGridPlacementComponent] Destroying PCC system for game object: " + this->gameObjectPtr->getName());

			if (nullptr != this->workspaceBaseComponent->getParallaxCorrectedCubemap())
			{
				Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
				Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));
				hlmsPbs->setParallaxCorrectedCubemap(nullptr);

				// Cast to FrameListener* before unregistering
				Ogre::Root::getSingleton().removeFrameListener(static_cast<Ogre::FrameListener*>(this->workspaceBaseComponent->getParallaxCorrectedCubemap()));
			}

			this->workspaceBaseComponent->destroyPccSystem();
		};

		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PccPerPixelGridPlacementComponent::destroyPccSystem");
	}

	// Lua registration part

	PccPerPixelGridPlacementComponent* getPccPerPixelGridPlacementComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<PccPerPixelGridPlacementComponent>(gameObject->getComponentWithOccurrence<PccPerPixelGridPlacementComponent>(occurrenceIndex)).get();
	}

	PccPerPixelGridPlacementComponent* getPccPerPixelGridPlacementComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PccPerPixelGridPlacementComponent>(gameObject->getComponent<PccPerPixelGridPlacementComponent>()).get();
	}

	PccPerPixelGridPlacementComponent* getPccPerPixelGridPlacementComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PccPerPixelGridPlacementComponent>(gameObject->getComponentFromName<PccPerPixelGridPlacementComponent>(name)).get();
	}

	void PccPerPixelGridPlacementComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<PccPerPixelGridPlacementComponent, GameObjectComponent>("PccPerPixelGridPlacementComponent")
			.def("setActivated", &PccPerPixelGridPlacementComponent::setActivated)
			.def("isActivated", &PccPerPixelGridPlacementComponent::isActivated)
			.def("setRegionCenter", &PccPerPixelGridPlacementComponent::setRegionCenter)
			.def("getRegionCenter", &PccPerPixelGridPlacementComponent::getRegionCenter)
			.def("setRegionHalfSize", &PccPerPixelGridPlacementComponent::setRegionHalfSize)
			.def("getRegionHalfSize", &PccPerPixelGridPlacementComponent::getRegionHalfSize)
			.def("setOverlapFactor", &PccPerPixelGridPlacementComponent::setOverlapFactor)
			.def("getOverlapFactor", &PccPerPixelGridPlacementComponent::getOverlapFactor)
			.def("setProbeResolution", &PccPerPixelGridPlacementComponent::setProbeResolution)
			.def("getProbeResolution", &PccPerPixelGridPlacementComponent::getProbeResolution)
			.def("setNearPlane", &PccPerPixelGridPlacementComponent::setNearPlane)
			.def("getNearPlane", &PccPerPixelGridPlacementComponent::getNearPlane)
			.def("setFarPlane", &PccPerPixelGridPlacementComponent::setFarPlane)
			.def("getFarPlane", &PccPerPixelGridPlacementComponent::getFarPlane)
			.def("setUseDpm2DArray", &PccPerPixelGridPlacementComponent::setUseDpm2DArray)
			.def("getUseDpm2DArray", &PccPerPixelGridPlacementComponent::getUseDpm2DArray)
			.def("forceUpdateAllProbes", &PccPerPixelGridPlacementComponent::forceUpdateAllProbes)
			.def("rebuildProbeGrid", &PccPerPixelGridPlacementComponent::rebuildProbeGrid)
		];

		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "class inherits GameObjectComponent", PccPerPixelGridPlacementComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setActivated(bool activated)", "Activates or deactivates the PCC system. When deactivated, all probes are destroyed and reflections disabled.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "bool isActivated()", "Returns true if PCC system is currently active.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setRegionCenter(Vector3 regionCenter)", "Sets the world-space center position of the AABB region to be covered by PCC reflection probes. Triggers probe grid rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "Vector3 getRegionCenter()", "Gets the current center position of the PCC probe coverage region.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setRegionHalfSize(Vector3 regionHalfSize)", "Sets the half-dimensions (extents) of the AABB region. For example, (10,5,10) creates a 20x10x20 meter region. Triggers probe grid rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "Vector3 getRegionHalfSize()", "Gets the current half-size (extents) of the PCC probe coverage region.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setOverlapFactor(Real overlapFactor)", "Sets the probe overlap factor (1.0-2.0). Higher values create smoother transitions between probes but reduce performance. Default: 1.25. Triggers probe grid rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "Real getOverlapFactor()", "Gets the current probe overlap factor value.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setProbeResolution(unsigned int probeResolution)", "Sets the resolution of each cubemap probe in pixels (e.g., 128, 256, 512, 1024). Higher values provide better quality but use more memory. Triggers probe grid rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "unsigned int getProbeResolution()", "Gets the current cubemap resolution per probe in pixels.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setNearPlane(Real nearPlane)", "Sets the near clipping distance for probe rendering. Smaller values capture closer details. Range: 0.01-1.0. Triggers probe grid rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "Real getNearPlane()", "Gets the current near plane distance for probe rendering.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setFarPlane(Real farPlane)", "Sets the far clipping distance for probe rendering. Should match the maximum distance you want reflections to show. Triggers probe grid rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "Real getFarPlane()", "Gets the current far plane distance for probe rendering.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void setUseDpm2DArray(bool useDpm2DArray)", "Toggles between Dual Paraboloid Mapping (true, more efficient) and Cubemap Arrays (false, better quality). Triggers full system rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "bool getUseDpm2DArray()", "Returns true if using Dual Paraboloid Mapping, false if using Cubemap Arrays.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void forceUpdateAllProbes()", "Forces all reflection probes to re-render. Useful after significant scene changes like adding/removing objects or changing lighting.");
		LuaScriptApi::getInstance()->addClassToCollection("PccPerPixelGridPlacementComponent", "void rebuildProbeGrid()", "Completely rebuilds the entire probe grid based on current settings. Called automatically when region or overlap settings change.");

		gameObjectClass.def("getPccPerPixelGridPlacementComponent", (PccPerPixelGridPlacementComponent * (*)(GameObject*)) & getPccPerPixelGridPlacementComponent);
		gameObjectClass.def("getPccPerPixelGridPlacementComponent", (PccPerPixelGridPlacementComponent * (*)(GameObject*, unsigned int)) & getPccPerPixelGridPlacementComponent);
		gameObjectClass.def("getPccPerPixelGridPlacementComponentFromName", (PccPerPixelGridPlacementComponent * (*)(GameObject*, const Ogre::String&)) & getPccPerPixelGridPlacementComponentFromName);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PccPerPixelGridPlacementComponent getPccPerPixelGridPlacementComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PccPerPixelGridPlacementComponent getPccPerPixelGridPlacementComponent(unsigned int occurrenceIndex)", "Gets the component from occurence index.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PccPerPixelGridPlacementComponent getPccPerPixelGridPlacementComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castPccPerPixelGridPlacementComponent", &GameObjectController::cast<PccPerPixelGridPlacementComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PccPerPixelGridPlacementComponent castPccPerPixelGridPlacementComponent(PccPerPixelGridPlacementComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool PccPerPixelGridPlacementComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if PCC component already exists
		auto pccCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PccPerPixelGridPlacementComponent>());
		if (nullptr != pccCompPtr)
		{
			return false; // Only one PCC component per camera
		}

		// Check if camera component exists
		auto cameraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr)
		{
			return false; // Must have a camera
		}

		// Check if workspace component exists
		auto workspaceCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<WorkspaceBaseComponent>());
		if (nullptr == workspaceCompPtr)
		{
			return false; // Must have a workspace
		}

		// Check if ReflectionCameraComponent exists (conflict)
		auto reflectionCameraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<ReflectionCameraComponent>());
		if (nullptr != reflectionCameraCompPtr)
		{
			return false; // Cannot coexist with ReflectionCameraComponent
		}

		return true;
	}

}; //namespace end