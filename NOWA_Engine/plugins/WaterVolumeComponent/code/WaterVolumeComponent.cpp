#include "NOWAPrecompiled.h"
#include "WaterVolumeComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "modules/GraphicsModule.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/WorkspaceComponents.h"
#include "gameobject/CameraComponent.h"

#include "OgreHlmsUnlit.h"
#include "OgreHlmsUnlitDatablock.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	WaterVolumeComponent::WaterVolumeComponent()
		: GameObjectComponent(),
		name("WaterVolumeComponent"),
		oldRenderQueueIndex(0),
		oldCastShadows(true),
		hasOldRenderQueueIndex(false),
		waterVolumeDatablock(nullptr),
		activated(new Variant(WaterVolumeComponent::AttrActivated(), true, this->attributes)),
		intensity(new Variant(WaterVolumeComponent::AttrIntensity(), 1.0f, this->attributes)),
		waterTint(new Variant(WaterVolumeComponent::AttrWaterTint(), Ogre::Vector3(0.1f, 0.35f, 0.45f), this->attributes)),
		deepWaterTint(new Variant(WaterVolumeComponent::AttrDeepWaterTint(), Ogre::Vector3(0.02f, 0.12f, 0.22f), this->attributes)),
		fogDensity(new Variant(WaterVolumeComponent::AttrFogDensity(), 0.15f, this->attributes)),
		deepThickness(new Variant(WaterVolumeComponent::AttrDeepThickness(), 12.0f, this->attributes)),
		absorption(new Variant(WaterVolumeComponent::AttrAbsorption(), 0.08f, this->attributes)),
		distortion(new Variant(WaterVolumeComponent::AttrDistortion(), 0.006f, this->attributes)),
		causticStrength(new Variant(WaterVolumeComponent::AttrCausticStrength(), 0.25f, this->attributes)),
		surfaceLineStrength(new Variant(WaterVolumeComponent::AttrSurfaceLineStrength(), 0.6f, this->attributes))
	{
		this->activated->setDescription("Activates the water volume. While active, the mesh itself is invisible and only acts as the water region.");

		this->intensity->setDescription("Per volume: scales distortion, fog, absorption, caustics and waterline of THIS volume (0 = no effect).");
		this->intensity->setConstraints(0.0f, 1.0f);

		this->waterTint->setDescription("Shared by all water volumes: fog colour for thin water.");
		this->waterTint->addUserData(GameObject::AttrActionColorDialog());

		this->deepWaterTint->setDescription("Shared by all water volumes: fog colour once the water is 'Deep Thickness' meters thick.");
		this->deepWaterTint->addUserData(GameObject::AttrActionColorDialog());

		this->fogDensity->setDescription("Shared by all water volumes: fog per meter of water between viewer and pixel. Fog = 1 - exp(-density * thickness).");
		this->fogDensity->setConstraints(0.0f, 5.0f);

		this->deepThickness->setDescription("Shared by all water volumes: water thickness in meters at which the fog colour reaches 'Deep Water Tint'.");
		this->deepThickness->setConstraints(0.1f, 1000.0f);

		this->absorption->setDescription("Shared by all water volumes: light absorption per meter (red is absorbed first, blue last).");
		this->absorption->setConstraints(0.0f, 2.0f);

		this->distortion->setDescription("Shared by all water volumes: wave distortion strength in screen UV units, also drives the waterline wave height.");
		this->distortion->setConstraints(0.0f, 0.05f);

		this->causticStrength->setDescription("Shared by all water volumes: brightness of the animated caustics on geometry inside the water.");
		this->causticStrength->setConstraints(0.0f, 2.0f);

		this->surfaceLineStrength->setDescription("Shared by all water volumes: brightness of the thin highlight along the waterline.");
		this->surfaceLineStrength->setConstraints(0.0f, 2.0f);
	}

	WaterVolumeComponent::~WaterVolumeComponent(void)
	{

	}

	void WaterVolumeComponent::initialise()
	{

	}

	const Ogre::String& WaterVolumeComponent::getName() const
	{
		return this->name;
	}

	void WaterVolumeComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<WaterVolumeComponent>(WaterVolumeComponent::getStaticClassId(), WaterVolumeComponent::getStaticClassName());
	}

	void WaterVolumeComponent::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void WaterVolumeComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void WaterVolumeComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool WaterVolumeComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrActivated())
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrIntensity())
		{
			this->intensity->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 1.0f), 0.0f, 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrWaterTint())
		{
			this->waterTint->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrDeepWaterTint())
		{
			this->deepWaterTint->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrFogDensity())
		{
			this->fogDensity->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.15f), 0.0f, 5.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrDeepThickness())
		{
			this->deepThickness->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 12.0f), 0.1f, 1000.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrAbsorption())
		{
			this->absorption->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.08f), 0.0f, 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrDistortion())
		{
			this->distortion->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.006f), 0.0f, 0.05f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrCausticStrength())
		{
			this->causticStrength->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.25f), 0.0f, 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == WaterVolumeComponent::AttrSurfaceLineStrength())
		{
			this->surfaceLineStrength->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.6f), 0.0f, 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr WaterVolumeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		WaterVolumeComponentPtr clonedCompPtr(boost::make_shared<WaterVolumeComponent>());

		// Plain value copies without side effects - the volume itself is created on connect()/setActivated(), like DistortionComponent
		clonedCompPtr->activated->setValue(this->activated->getBool());
		clonedCompPtr->intensity->setValue(this->intensity->getReal());
		clonedCompPtr->waterTint->setValue(this->waterTint->getVector3());
		clonedCompPtr->deepWaterTint->setValue(this->deepWaterTint->getVector3());
		clonedCompPtr->fogDensity->setValue(this->fogDensity->getReal());
		clonedCompPtr->deepThickness->setValue(this->deepThickness->getReal());
		clonedCompPtr->absorption->setValue(this->absorption->getReal());
		clonedCompPtr->distortion->setValue(this->distortion->getReal());
		clonedCompPtr->causticStrength->setValue(this->causticStrength->getReal());
		clonedCompPtr->surfaceLineStrength->setValue(this->surfaceLineStrength->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool WaterVolumeComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WaterVolumeComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// For now it will only work for main camera (same as DistortionComponent)
		auto mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_CAMERA_ID);
		if (nullptr != mainCameraGameObject)
		{
			auto workspaceBaseCompPtr = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<WorkspaceBaseComponent>());
			if (nullptr != workspaceBaseCompPtr)
			{
				// Rebuilds the workspace once if not yet active; saved with the scene afterwards, so later loads build it directly
				if (false == workspaceBaseCompPtr->getUseWaterVolume())
				{
					workspaceBaseCompPtr->setUseWaterVolume(true);
				}
			}
		}

		return true;
	}

	bool WaterVolumeComponent::connect(void)
	{
		this->setActivated(this->activated->getBool());

		return true;
	}

	bool WaterVolumeComponent::disconnect(void)
	{
		this->destroyWaterVolume();
		return true;
	}

	bool WaterVolumeComponent::onCloned(void)
	{

		return true;
	}

	void WaterVolumeComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->destroyWaterVolume();
	}

	Ogre::String WaterVolumeComponent::getUpdateClosureId(void) const
	{
		return this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
	}

	void WaterVolumeComponent::createWaterVolume(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		GraphicsModule::getInstance()->enqueueAndWait([this]()
			{
				if (NOWA::ITEM != this->gameObjectPtr->getType())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WaterVolumeComponent] Game object is not an item (mesh), water volume cannot be created: " + this->gameObjectPtr->getName());
					return;
				}

				Ogre::Item* item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
				if (nullptr == item)
				{
					return;
				}

				// Store the item's original state once (guarded by its own flag, so a second activation or a reused datablock
				// never overwrites it with the water volume state) - restored in destroyWaterVolume()
				if (false == this->hasOldRenderQueueIndex)
				{
					auto datablockNames = this->gameObjectPtr->getDatablockNames();
					if (false == datablockNames.empty())
					{
						this->oldDatablockName = datablockNames[0];
					}
					// Own flag instead of DistortionComponent's "0 != oldRenderQueueIndex" check, so render queue 0 is restored as well
					this->oldRenderQueueIndex = this->gameObjectPtr->getRenderQueueIndex();
					this->oldCastShadows = item->getCastShadows();
					this->hasOldRenderQueueIndex = true;
				}

				if (nullptr == this->waterVolumeDatablock)
				{
					Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
					assert(dynamic_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT)));
					Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlmsManager->getHlms(Ogre::HLMS_UNLIT));

					const Ogre::String datablockName = "WaterVolumeMaterial_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

					// May still exist if an earlier destroy could not delete it (still linked at that time) - reuse it then
					Ogre::HlmsDatablock* existingDatablock = hlmsUnlit->getDatablock(datablockName);
					if (nullptr != existingDatablock)
					{
						this->waterVolumeDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(existingDatablock);
					}
					else
					{
						// Data is written into rt_waterVolume, so it must arrive EXACTLY as computed:
						// - no blending (default blendblock = replace), otherwise mask/depth values would get mixed
						// - depth check on (occluded by scene geometry in front of the water), no depth write
						// - no culling, so the mask also exists when the camera is inside the volume
						Ogre::HlmsMacroblock macroblock;
						macroblock.mDepthCheck = true;
						macroblock.mDepthWrite = false;
						macroblock.mCullMode = Ogre::CULL_NONE;

						Ogre::HlmsBlendblock blendblock;

						this->waterVolumeDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(hlmsUnlit->createDatablock(datablockName, datablockName, macroblock, blendblock, Ogre::HlmsParamVec()));
					}

					this->waterVolumeDatablock->setUseColour(true);
					// R = 0 until updateVolumeDepthsInternal() ran once - avoids one frame with front/back depth 0 (fully fogged)
					this->waterVolumeDatablock->setColour(Ogre::ColourValue(0.0f, 0.0f, 0.0f, 0.0f));
				}

				item->setDatablock(this->waterVolumeDatablock);
				// Rendered only into rt_waterVolume, excluded from the main scene passes (see WorkspaceBaseComponent::splitScenePassAroundEffectQueues)
				this->gameObjectPtr->setRenderQueueIndex(WorkspaceBaseComponent::WATER_VOLUME_RENDER_QUEUE);
				// Shadow caster passes are not limited by the main pass render queue ranges - without this the invisible box
				// would still cast a box shaped shadow onto the pool floor
				item->setCastShadows(false);

				// Already on the render thread - call the internal variant directly, no nested dispatch
				this->applyLookParametersInternal();
			},
			"WaterVolumeComponent::createWaterVolume");
	}

	void WaterVolumeComponent::destroyWaterVolume(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->getUpdateClosureId());

		GraphicsModule::getInstance()->enqueueAndWait([this]()
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();

				if (true == this->hasOldRenderQueueIndex)
				{
					this->gameObjectPtr->setRenderQueueIndex(this->oldRenderQueueIndex);
					if (nullptr != item)
					{
						item->setCastShadows(this->oldCastShadows);
					}
					this->hasOldRenderQueueIndex = false;
				}
				if (nullptr != item && false == this->oldDatablockName.empty())
				{
					// Set back the original datablock FIRST - this unlinks the item from the water volume datablock,
					// which must not be destroyed while still linked (crashes inside ~HlmsDatablock())
					item->setDatablock(this->oldDatablockName);
				}

				if (nullptr != this->waterVolumeDatablock)
				{
					// Only destroy if the datablock is not used else where
					if (true == this->waterVolumeDatablock->getLinkedRenderables().empty())
					{
						this->waterVolumeDatablock->getCreator()->destroyDatablock(this->waterVolumeDatablock->getName());
						this->waterVolumeDatablock = nullptr;
					}
				}
			},
			"WaterVolumeComponent::destroyWaterVolume");
	}

	void WaterVolumeComponent::updateVolumeDepthsInternal(Ogre::Camera* camera, Ogre::Real intensity)
	{
		// RUNS ON RENDER THREAD (tracked closure from update())
		if (nullptr == this->waterVolumeDatablock || nullptr == camera)
		{
			return;
		}

		Ogre::Item* item = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>();
		if (nullptr == item)
		{
			return;
		}

		// Depths are stored normalized by the far clip distance (keeps them in [0, 1], independent of Unlit output clamping).
		// An infinite far clip (0) cannot be normalized - effect stays off then.
		const Ogre::Real farClipDistance = camera->getFarClipDistance();
		if (farClipDistance <= 0.0f)
		{
			return;
		}

		const Ogre::Aabb worldAabb = item->getWorldAabbUpdated();
		const Ogre::Vector3 minimum = worldAabb.getMinimum();
		const Ogre::Vector3 maximum = worldAabb.getMaximum();

		const Ogre::Vector3 cameraPosition = camera->getDerivedPosition();
		const Ogre::Vector3 cameraDirection = camera->getDerivedDirection();

		// View depth (distance along the view axis, same as the linearized depth buffer in the shader) of the nearest and the
		// farthest AABB corner. For a box facing the camera (2.5D side view) this is exactly the depth of its front and back face,
		// constant over the whole face. Rotated meshes are approximated by their world AABB.
		Ogre::Real frontDepth = std::numeric_limits<Ogre::Real>::max();
		Ogre::Real backDepth = -std::numeric_limits<Ogre::Real>::max();

		for (unsigned int i = 0; i < 8u; ++i)
		{
			const Ogre::Vector3 corner((i & 1u) ? maximum.x : minimum.x, (i & 2u) ? maximum.y : minimum.y, (i & 4u) ? maximum.z : minimum.z);
			const Ogre::Real depth = (corner - cameraPosition).dotProduct(cameraDirection);

			frontDepth = std::min(frontDepth, depth);
			backDepth = std::max(backDepth, depth);
		}

		// Camera inside the volume: the water starts right at the camera
		frontDepth = std::max(frontDepth, 0.0f);
		backDepth = std::max(backDepth, frontDepth);

		const Ogre::ColourValue packed(1.0f,
			Ogre::Math::Clamp(frontDepth / farClipDistance, 0.0f, 1.0f),
			Ogre::Math::Clamp(backDepth / farClipDistance, 0.0f, 1.0f),
			Ogre::Math::Clamp(intensity, 0.0f, 1.0f));

		this->waterVolumeDatablock->setColour(packed);
	}

	void WaterVolumeComponent::applyLookParametersInternal(void)
	{
		// RUNS ON RENDER THREAD
		Ogre::MaterialPtr materialWaterVolume = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("WaterVolume/Quad", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));
		if (true == materialWaterVolume.isNull())
		{
			return;
		}

		Ogre::Pass* pass = materialWaterVolume->getTechnique(0)->getPass(0);
		Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

		psParams->setNamedConstant("waterTint", this->waterTint->getVector3());
		psParams->setNamedConstant("deepWaterTint", this->deepWaterTint->getVector3());
		psParams->setNamedConstant("fogDensity", this->fogDensity->getReal());
		psParams->setNamedConstant("deepThickness", this->deepThickness->getReal());
		psParams->setNamedConstant("absorption", this->absorption->getReal());
		psParams->setNamedConstant("distortion", this->distortion->getReal());
		psParams->setNamedConstant("causticStrength", this->causticStrength->getReal());
		psParams->setNamedConstant("surfaceLineStrength", this->surfaceLineStrength->getReal());
	}

	void WaterVolumeComponent::applyLookParameters(void)
	{
		// Synchronous on purpose (rare, user driven edits): the logic thread waits, so the internal variant can read the
		// Variants without racing against the next setter.
		GraphicsModule::getInstance()->enqueueAndWait([this]()
			{
				this->applyLookParametersInternal();
			},
			"WaterVolumeComponent::applyLookParameters");
	}

	void WaterVolumeComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Also runs in the editor (notSimulating), so an activated volume shows correctly while moving the camera there too
		if (false == this->activated->getBool())
		{
			return;
		}

		// Camera fetched on the logic thread, only its pointer is handed to the render thread
		auto mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_CAMERA_ID);
		if (nullptr == mainCameraGameObject)
		{
			return;
		}

		auto cameraCompPtr = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr)
		{
			return;
		}

		Ogre::Camera* camera = cameraCompPtr->getCamera();
		const Ogre::Real intensity = this->intensity->getReal();

		auto closureFunction = [this, camera, intensity](Ogre::Real renderDt)
		{
			this->updateVolumeDepthsInternal(camera, intensity);
		};
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->getUpdateClosureId(), closureFunction, false);
	}

	void WaterVolumeComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (WaterVolumeComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (WaterVolumeComponent::AttrIntensity() == attribute->getName())
		{
			this->setIntensity(attribute->getReal());
		}
		else if (WaterVolumeComponent::AttrWaterTint() == attribute->getName())
		{
			this->setWaterTint(attribute->getVector3());
		}
		else if (WaterVolumeComponent::AttrDeepWaterTint() == attribute->getName())
		{
			this->setDeepWaterTint(attribute->getVector3());
		}
		else if (WaterVolumeComponent::AttrFogDensity() == attribute->getName())
		{
			this->setFogDensity(attribute->getReal());
		}
		else if (WaterVolumeComponent::AttrDeepThickness() == attribute->getName())
		{
			this->setDeepThickness(attribute->getReal());
		}
		else if (WaterVolumeComponent::AttrAbsorption() == attribute->getName())
		{
			this->setAbsorption(attribute->getReal());
		}
		else if (WaterVolumeComponent::AttrDistortion() == attribute->getName())
		{
			this->setDistortion(attribute->getReal());
		}
		else if (WaterVolumeComponent::AttrCausticStrength() == attribute->getName())
		{
			this->setCausticStrength(attribute->getReal());
		}
		else if (WaterVolumeComponent::AttrSurfaceLineStrength() == attribute->getName())
		{
			this->setSurfaceLineStrength(attribute->getReal());
		}
	}

	void WaterVolumeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrActivated().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrIntensity().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->intensity->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrWaterTint().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waterTint->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrDeepWaterTint().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->deepWaterTint->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrFogDensity().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fogDensity->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrDeepThickness().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->deepThickness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrAbsorption().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->absorption->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrDistortion().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->distortion->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrCausticStrength().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->causticStrength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(WaterVolumeComponent::AttrSurfaceLineStrength().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceLineStrength->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String WaterVolumeComponent::getClassName(void) const
	{
		return "WaterVolumeComponent";
	}

	Ogre::String WaterVolumeComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void WaterVolumeComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			this->createWaterVolume();
		}
		else
		{
			this->destroyWaterVolume();
		}
	}

	bool WaterVolumeComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void WaterVolumeComponent::setIntensity(Ogre::Real intensity)
	{
		// Per volume - picked up by the next update() closure, no shader parameter involved
		this->intensity->setValue(Ogre::Math::Clamp(intensity, 0.0f, 1.0f));
	}

	Ogre::Real WaterVolumeComponent::getIntensity(void) const
	{
		return this->intensity->getReal();
	}

	void WaterVolumeComponent::setWaterTint(const Ogre::Vector3& waterTint)
	{
		this->waterTint->setValue(waterTint);
		this->applyLookParameters();
	}

	Ogre::Vector3 WaterVolumeComponent::getWaterTint(void) const
	{
		return this->waterTint->getVector3();
	}

	void WaterVolumeComponent::setDeepWaterTint(const Ogre::Vector3& deepWaterTint)
	{
		this->deepWaterTint->setValue(deepWaterTint);
		this->applyLookParameters();
	}

	Ogre::Vector3 WaterVolumeComponent::getDeepWaterTint(void) const
	{
		return this->deepWaterTint->getVector3();
	}

	void WaterVolumeComponent::setFogDensity(Ogre::Real fogDensity)
	{
		this->fogDensity->setValue(Ogre::Math::Clamp(fogDensity, 0.0f, 5.0f));
		this->applyLookParameters();
	}

	Ogre::Real WaterVolumeComponent::getFogDensity(void) const
	{
		return this->fogDensity->getReal();
	}

	void WaterVolumeComponent::setDeepThickness(Ogre::Real deepThickness)
	{
		this->deepThickness->setValue(Ogre::Math::Clamp(deepThickness, 0.1f, 1000.0f));
		this->applyLookParameters();
	}

	Ogre::Real WaterVolumeComponent::getDeepThickness(void) const
	{
		return this->deepThickness->getReal();
	}

	void WaterVolumeComponent::setAbsorption(Ogre::Real absorption)
	{
		this->absorption->setValue(Ogre::Math::Clamp(absorption, 0.0f, 2.0f));
		this->applyLookParameters();
	}

	Ogre::Real WaterVolumeComponent::getAbsorption(void) const
	{
		return this->absorption->getReal();
	}

	void WaterVolumeComponent::setDistortion(Ogre::Real distortion)
	{
		this->distortion->setValue(Ogre::Math::Clamp(distortion, 0.0f, 0.05f));
		this->applyLookParameters();
	}

	Ogre::Real WaterVolumeComponent::getDistortion(void) const
	{
		return this->distortion->getReal();
	}

	void WaterVolumeComponent::setCausticStrength(Ogre::Real causticStrength)
	{
		this->causticStrength->setValue(Ogre::Math::Clamp(causticStrength, 0.0f, 2.0f));
		this->applyLookParameters();
	}

	Ogre::Real WaterVolumeComponent::getCausticStrength(void) const
	{
		return this->causticStrength->getReal();
	}

	void WaterVolumeComponent::setSurfaceLineStrength(Ogre::Real surfaceLineStrength)
	{
		this->surfaceLineStrength->setValue(Ogre::Math::Clamp(surfaceLineStrength, 0.0f, 2.0f));
		this->applyLookParameters();
	}

	Ogre::Real WaterVolumeComponent::getSurfaceLineStrength(void) const
	{
		return this->surfaceLineStrength->getReal();
	}

	// Lua registration part

	WaterVolumeComponent* getWaterVolumeComponent(GameObject* gameObject)
	{
		return makeStrongPtr<WaterVolumeComponent>(gameObject->getComponent<WaterVolumeComponent>()).get();
	}

	WaterVolumeComponent* getWaterVolumeComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<WaterVolumeComponent>(gameObject->getComponentFromName<WaterVolumeComponent>(name)).get();
	}

	void WaterVolumeComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<WaterVolumeComponent, GameObjectComponent>("WaterVolumeComponent")
			.def("setActivated", &WaterVolumeComponent::setActivated)
			.def("isActivated", &WaterVolumeComponent::isActivated)
			.def("setIntensity", &WaterVolumeComponent::setIntensity)
			.def("getIntensity", &WaterVolumeComponent::getIntensity)
			.def("setWaterTint", &WaterVolumeComponent::setWaterTint)
			.def("getWaterTint", &WaterVolumeComponent::getWaterTint)
			.def("setDeepWaterTint", &WaterVolumeComponent::setDeepWaterTint)
			.def("getDeepWaterTint", &WaterVolumeComponent::getDeepWaterTint)
			.def("setFogDensity", &WaterVolumeComponent::setFogDensity)
			.def("getFogDensity", &WaterVolumeComponent::getFogDensity)
			.def("setDeepThickness", &WaterVolumeComponent::setDeepThickness)
			.def("getDeepThickness", &WaterVolumeComponent::getDeepThickness)
			.def("setAbsorption", &WaterVolumeComponent::setAbsorption)
			.def("getAbsorption", &WaterVolumeComponent::getAbsorption)
			.def("setDistortion", &WaterVolumeComponent::setDistortion)
			.def("getDistortion", &WaterVolumeComponent::getDistortion)
			.def("setCausticStrength", &WaterVolumeComponent::setCausticStrength)
			.def("getCausticStrength", &WaterVolumeComponent::getCausticStrength)
			.def("setSurfaceLineStrength", &WaterVolumeComponent::setSurfaceLineStrength)
			.def("getSurfaceLineStrength", &WaterVolumeComponent::getSurfaceLineStrength)
		];

		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "class inherits GameObjectComponent", WaterVolumeComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setActivated(bool activated)", "Sets whether the water volume is active. While active, the mesh itself is invisible and only acts as the water region.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "bool isActivated()", "Gets whether the water volume is active.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setIntensity(float intensity)", "Per volume: sets the effect intensity (0 - 1) of this volume, e.g. to fade a draining pool out.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getIntensity()", "Gets the effect intensity of this volume.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setWaterTint(Vector3 colour)", "Shared by all water volumes: sets the fog colour for thin water.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "Vector3 getWaterTint()", "Gets the fog colour for thin water.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setDeepWaterTint(Vector3 colour)", "Shared by all water volumes: sets the fog colour for deep water.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "Vector3 getDeepWaterTint()", "Gets the fog colour for deep water.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setFogDensity(float density)", "Shared by all water volumes: sets the fog per meter of water (0 - 5).");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getFogDensity()", "Gets the fog per meter of water.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setDeepThickness(float meters)", "Shared by all water volumes: sets the water thickness at which the deep water tint is reached.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getDeepThickness()", "Gets the water thickness at which the deep water tint is reached.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setAbsorption(float absorption)", "Shared by all water volumes: sets the light absorption per meter (0 - 2).");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getAbsorption()", "Gets the light absorption per meter.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setDistortion(float distortion)", "Shared by all water volumes: sets the wave distortion strength in screen UV units (0 - 0.05).");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getDistortion()", "Gets the wave distortion strength.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setCausticStrength(float strength)", "Shared by all water volumes: sets the caustics brightness (0 - 2).");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getCausticStrength()", "Gets the caustics brightness.");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "void setSurfaceLineStrength(float strength)", "Shared by all water volumes: sets the waterline highlight brightness (0 - 2).");
		LuaScriptApi::getInstance()->addClassToCollection("WaterVolumeComponent", "float getSurfaceLineStrength()", "Gets the waterline highlight brightness.");

		gameObjectClass.def("getWaterVolumeComponentFromName", &getWaterVolumeComponentFromName);
		gameObjectClass.def("getWaterVolumeComponent", (WaterVolumeComponent * (*)(GameObject*)) & getWaterVolumeComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WaterVolumeComponent getWaterVolumeComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WaterVolumeComponent getWaterVolumeComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castWaterVolumeComponent", &GameObjectController::cast<WaterVolumeComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "WaterVolumeComponent castWaterVolumeComponent(WaterVolumeComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool WaterVolumeComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto waterVolumeCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<WaterVolumeComponent>());
		if (nullptr == waterVolumeCompPtr)
		{
			return true;
		}

		return false;
	}

}; //namespace end
