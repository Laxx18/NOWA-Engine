#include "NOWAPrecompiled.h"

#include "OceanComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "camera/CameraManager.h"
#include "modules/WorkspaceModule.h"
#include "ocean/OgreHlmsOcean.h"
#include "WorkspaceComponents.h"
#include "CameraComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	OceanComponent::OceanComponent()
		: GameObjectComponent(),
		ocean(nullptr),
		datablock(nullptr),
		postInitDone(false),
		usedCamera(nullptr),
		cameraId(new Variant(OceanComponent::AttrCameraId(), static_cast<unsigned long>(0), this->attributes, true)),
		deepColour(new Variant(OceanComponent::AttrDeepColour(), Ogre::Vector3(0.0f, 0.03f, 0.05f), this->attributes)),
		shallowColour(new Variant(OceanComponent::AttrShallowColour(), Ogre::Vector3(0.0f, 0.08f, 0.1f), this->attributes)),
		brdf(new Variant(OceanComponent::AttrBrdf(),
			{ "Default", "CookTorrance", "BlinnPhong",
			  "DefaultUncorrelated", "DefaultSeparateDiffuseFresnel",
			  "CookTorranceSeparateDiffuseFresnel", "BlinnPhongSeparateDiffuseFresnel" }, this->attributes)),
		shaderWavesScale(new Variant(OceanComponent::AttrShaderWavesScale(), 1.0f, this->attributes)),
		wavesIntensity(new Variant(OceanComponent::AttrWavesIntensity(), 0.8f, this->attributes)),
		oceanWavesScale(new Variant(OceanComponent::AttrOceanWavesScale(), 1.6f, this->attributes)),
		oceanSize(new Variant(OceanComponent::AttrOceanSize(), Ogre::Vector2(200.0f, 200.0f), this->attributes)),
		oceanCenter(new Variant(OceanComponent::AttrOceanCenter(), Ogre::Vector3::ZERO, this->attributes))
	{
		this->cameraId->setDescription("The optional camera game object id which can be set. E.g. useful if the MinimapComponent is involved, to set the minimap camera, so that ocean is painted correctly on minimap. Can be left of, default is the main active camera.");

		this->deepColour->setDescription("Ocean deep water colour (Vector3 RGB).");
		this->shallowColour->setDescription("Ocean shallow water colour (Vector3 RGB).");
		this->brdf->setDescription("Ocean BRDF model (enum as integer, e.g. BlinnPhong).");
		this->shaderWavesScale->setDescription("Shader-side wave/detail scale.");
		this->wavesIntensity->setDescription("Wave amplitude/intensity.");
		this->oceanWavesScale->setDescription("Wave/tiling scale.");
		this->oceanSize->setDescription("Ocean size in world units (Vector2).");
		this->oceanCenter->setDescription("Ocean center position in world space (Vector3).");
	}

	OceanComponent::~OceanComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OceanComponent] Destructor ocean component for game object: " + this->gameObjectPtr->getName());
		
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &OceanComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());

		this->destroyOcean();
	}

	bool OceanComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraId")
		{
			this->cameraId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DeepColour")
		{
			this->deepColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.2f, 0.4f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShallowColour")
		{
			this->shallowColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.6f, 0.8f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Brdf")
		{
			this->brdf->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShaderWavesScale")
		{
			this->shaderWavesScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WavesIntensity")
		{
			this->wavesIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.8f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OceanWavesScale")
		{
			this->oceanWavesScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.6f));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Size")
		{
			this->oceanSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(200.0f, 200.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Center")
		{
			this->oceanCenter->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr OceanComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool OceanComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OceanComponent] Init ocean component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &OceanComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());

		this->gameObjectPtr->setDoNotDestroyMovableObject(true);

		// this->gameObjectPtr->setUseReflection(false);
		// this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);

		this->gameObjectPtr->getAttribute(GameObject::AttrCastShadows())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrClampY())->setVisible(false);

		this->postInitDone = true;

		this->createOcean();
		return true;
	}

	void OceanComponent::handleSwitchCamera(EventDataPtr eventData)
	{
		// When camera changed event must be triggered, to set the new camera for the ocean
		WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		if (nullptr != workspaceBaseComponent)
		{
			boost::shared_ptr<EventDataSwitchCamera> castEventData = boost::static_pointer_cast<EventDataSwitchCamera>(eventData);
			unsigned long id = std::get<0>(castEventData->getCameraGameObjectData());
			unsigned int index = std::get<1>(castEventData->getCameraGameObjectData());
			bool active = std::get<2>(castEventData->getCameraGameObjectData());

			// if a camera has been set as active, go through all game objects and set all camera components as active false
			if (true == active)
			{
				auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
				for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
				{
					GameObject* gameObject = it->second.get();
					if (id != gameObject->getId())
					{
						auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
						if (nullptr != cameraComponent)
						{
							Ogre::String s1 = cameraComponent->getCamera()->getName();
							Ogre::String s2 = this->usedCamera ? this->usedCamera->getName() : "null";

							if (true == cameraComponent->isActivated() && cameraComponent->getCamera() != this->usedCamera)
							{
								this->destroyOcean();
								this->createOcean();
							}
						}
					}
				}
			}
		}
	}

	bool OceanComponent::connect(void)
	{
		return true;
	}

	bool OceanComponent::disconnect(void)
	{
		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		return true;
	}

	void OceanComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			auto closureFunction = [this](Ogre::Real weight)
				{
					this->ocean->update();
				};
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
		}
	}

	void OceanComponent::createOcean(void)
	{
		if (nullptr == this->ocean && nullptr != AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera() && true == this->postInitDone)
		{
			ENQUEUE_RENDER_COMMAND("OceanComponent::createOcean",
			{
				if (this->cameraId->getULong() != 0)
				{
					GameObjectPtr cameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraId->getULong());
					if (nullptr != cameraGameObjectPtr)
					{
						auto& cameraCompPtr = NOWA::makeStrongPtr(cameraGameObjectPtr->getComponent<CameraComponent>());
						if (nullptr != cameraCompPtr)
						{
							this->usedCamera = cameraCompPtr->getCamera();
						}
					}
				}
				else
				{
					this->usedCamera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
				}

				// Create ocean
				this->ocean = new Ogre::Ocean(Ogre::Id::generateNewId<Ogre::MovableObject>(), &this->gameObjectPtr->getSceneManager()->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC),
					this->gameObjectPtr->getSceneManager(), 0, Ogre::Root::getSingletonPtr()->getCompositorManager2(), this->usedCamera);

				this->ocean->setCastShadows(false);
				this->ocean->setName(this->gameObjectPtr->getName());

				Ogre::Vector3 center = this->oceanCenter->getVector3();
				Ogre::Vector2 size = this->oceanSize->getVector2();
				this->ocean->create(center, size);

				// Runtime wave parameters (Ocean side)
				this->ocean->setWavesIntensity(this->wavesIntensity->getReal());
				this->ocean->setWavesScale(this->oceanWavesScale->getReal());

				auto* hlmsOcean = static_cast<Ogre::HlmsOcean*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER1));

				hlmsOcean->setOceanDataTextureName("oceanData.dds");
				hlmsOcean->setWeightTextureName("weight.dds");

				// IMPORTANT: do NOT set oceanDataTex as env probe!
				// hlmsOcean->setEnvProbe( ... ) must be a cubemap probe (optional).

				// Optional: only if you actually have a cubemap probe texture:
				//// Ogre::TextureGpu* probeCube = ...;
				//// hlmsOcean->setEnvProbe(probeCube);

				if (nullptr == this->datablock)
				{
					Ogre::String datablockName = "Ocean_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
					this->datablock = static_cast<Ogre::HlmsOceanDatablock*>(hlmsOcean->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));

					this->ocean->setDatablock(this->datablock);

					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setDeepColour(this->deepColour->getVector3());
					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setShallowColour(this->shallowColour->getVector3());

					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setBrdf(this->mapStringToOceanBrdf(this->brdf->getListSelectedValue()));

					// Shader-side detail scale
					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setWavesScale(this->shaderWavesScale->getReal());
				}

				this->ocean->setStatic(false);
				this->gameObjectPtr->setDynamic(true);
				this->gameObjectPtr->getSceneNode()->attachObject(this->ocean);

				this->gameObjectPtr->init(this->ocean);
				// Register after the component has been created
				AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);

				WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
				if (nullptr != workspaceBaseComponent)
				{
					if (false == workspaceBaseComponent->getUseOcean())
					{
						workspaceBaseComponent->setUseOcean(true);
					}
				}
			});
		}
	}

	void OceanComponent::destroyOcean(void)
	{
		if (nullptr == this->ocean)
		{
			return;
		}

		Ogre::Ocean* ocean = this->ocean;
		GameObjectPtr gameObjectPtr = this->gameObjectPtr;
		Ogre::HlmsDatablock* datablock = this->datablock;

		WorkspaceModule* workspaceModule = WorkspaceModule::getInstance();
		WorkspaceBaseComponent* workspaceBaseComponent = nullptr;
		if (nullptr != workspaceModule)
		{
			workspaceBaseComponent = workspaceModule->getPrimaryWorkspaceComponent();
		}

		AppStateManager* appState = AppStateManager::getSingletonPtr();

		// Clear pointers on *this* immediately
		this->ocean = nullptr;
		this->datablock = nullptr;

		NOWA::GraphicsModule::DestroyCommand destroyCommand =
			[ocean, gameObjectPtr, workspaceBaseComponent, appState, datablock]() mutable
			{
				if (nullptr != workspaceBaseComponent)
				{
					if (nullptr != appState && false == appState->bShutdown)
					{
						workspaceBaseComponent->setUseOcean(false);
					}
				}

				if (nullptr != ocean)
				{
					if (true == ocean->isAttached())
					{
						Ogre::SceneNode* parentNode = ocean->getParentSceneNode();
						if (nullptr != parentNode)
						{
							parentNode->detachObject(ocean);
						}
					}
					else if (nullptr != gameObjectPtr && nullptr != gameObjectPtr->getSceneNode())
					{
						// Defensive: detach from GO node if it is still there (even if ocean thinks it's not attached)
						gameObjectPtr->getSceneNode()->detachObject(ocean);
					}

					// Created with factory not with scene manager, so manual delete!
					delete ocean;
					ocean = nullptr;
				}

				if (nullptr != gameObjectPtr)
				{
					gameObjectPtr->movableObject = nullptr;
				}

				if (nullptr != gameObjectPtr && nullptr != gameObjectPtr->sceneManager && nullptr != gameObjectPtr->boundingBoxDraw)
				{
					gameObjectPtr->sceneManager->destroyWireAabb(gameObjectPtr->boundingBoxDraw);
					gameObjectPtr->boundingBoxDraw = nullptr;
				}

				if (nullptr != datablock)
				{
					Ogre::Hlms* hlms = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER1);
					if (nullptr != hlms)
					{
						Ogre::HlmsDatablock* db = hlms->getDatablock(datablock->getName());
						if (nullptr != db)
						{
							auto& linkedRenderables = db->getLinkedRenderables();
							if (true == linkedRenderables.empty())
							{
								db->getCreator()->destroyDatablock(db->getName());
							}
						}
					}

					datablock = nullptr;
				}
			};

		GraphicsModule::getInstance()->enqueueDestroy(destroyCommand, "OceanComponent::destroyOcean");
	}

	void OceanComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (OceanComponent::AttrCameraId() == attribute->getName())
		{
			this->setCameraId(attribute->getULong());
		}
		else if (OceanComponent::AttrDeepColour() == attribute->getName())
		{
			this->setDeepColour(attribute->getVector3());
		}
		else if (OceanComponent::AttrShallowColour() == attribute->getName())
		{
			this->setShallowColour(attribute->getVector3());
		}
		else if (OceanComponent::AttrBrdf() == attribute->getName())
		{
			this->setBrdf(attribute->getListSelectedValue());
		}
		else if (OceanComponent::AttrShaderWavesScale() == attribute->getName())
		{
			this->setShaderWavesScale(attribute->getReal());
		}
		else if (OceanComponent::AttrWavesIntensity() == attribute->getName())
		{
			this->setWavesIntensity(attribute->getReal());
		}
		else if (OceanComponent::AttrOceanWavesScale() == attribute->getName())
		{
			this->setShaderWavesScale(attribute->getReal());
		}
		else if (OceanComponent::AttrOceanSize() == attribute->getName())
		{
			this->setOceanSize(attribute->getVector2());
			this->destroyOcean();
			this->createOcean();
		}
		else if (OceanComponent::AttrOceanCenter() == attribute->getName())
		{
			this->setOceanCenter(attribute->getVector3());
			this->destroyOcean();
			this->createOcean();
		}
	}

	void OceanComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraId->getULong())));
		propertiesXML->append_node(propertyXML);

		// DeepColour
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DeepColour"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->deepColour->getVector3())));
		propertiesXML->append_node(propertyXML);

		// ShallowColour
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShallowColour"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shallowColour->getVector3())));
		propertiesXML->append_node(propertyXML);

		// Brdf
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Brdf"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brdf->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		// ShaderWavesScale
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShaderWavesScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shaderWavesScale->getReal())));
		propertiesXML->append_node(propertyXML);

		// WavesIntensity
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WavesIntensity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wavesIntensity->getReal())));
		propertiesXML->append_node(propertyXML);

		// OceanWavesScale
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OceanWavesScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oceanWavesScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Size"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oceanSize->getVector2())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Center"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oceanCenter->getVector3())));
		propertiesXML->append_node(propertyXML);
		
	}

	/*void OceanComponent::setActivated(bool activated)
	{
		if (nullptr != this->light)
		{
			this->light->setVisible(activated);
		}
	}*/

	void OceanComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		if (nullptr != workspaceBaseComponent && false == AppStateManager::getSingletonPtr()->bShutdown)
		{
			workspaceBaseComponent->setUseTerra(false);
		}

		this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(true);

		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(true);
	}

	Ogre::String OceanComponent::getClassName(void) const
	{
		return "OceanComponent";
	}

	Ogre::String OceanComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	Ogre::String OceanComponent::mapOceanBrdfToString(Ogre::OceanBrdf::OceanBrdf brdf)
	{
		// Detect flags & base BRDF (mask is defined in OceanBrdf enum)
		const uint32_t flags = static_cast<uint32_t>(brdf);
		const uint32_t base = flags & Ogre::OceanBrdf::BRDF_MASK;

		const bool uncorrelated = (flags & Ogre::OceanBrdf::FLAG_UNCORRELATED) != 0u;
		const bool sepDiffuse = (flags & Ogre::OceanBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL) != 0u;

		// Prefer the combined/flagged names first
		if (base == Ogre::OceanBrdf::Default && uncorrelated)
			return "DefaultUncorrelated";
		if (base == Ogre::OceanBrdf::Default && sepDiffuse)
			return "DefaultSeparateDiffuseFresnel";
		if (base == Ogre::OceanBrdf::CookTorrance && sepDiffuse)
			return "CookTorranceSeparateDiffuseFresnel";
		if (base == Ogre::OceanBrdf::BlinnPhong && sepDiffuse)
			return "BlinnPhongSeparateDiffuseFresnel";

		// Base names
		if (base == Ogre::OceanBrdf::CookTorrance)
			return "CookTorrance";
		if (base == Ogre::OceanBrdf::BlinnPhong)
			return "BlinnPhong";

		return "Default";
	}

	Ogre::OceanBrdf::OceanBrdf OceanComponent::mapStringToOceanBrdf(const Ogre::String& strBrdf)
	{
		uint32_t brdf = Ogre::OceanBrdf::Default;

		if ("CookTorrance" == strBrdf)
			brdf = Ogre::OceanBrdf::CookTorrance;
		else if ("BlinnPhong" == strBrdf)
			brdf = Ogre::OceanBrdf::BlinnPhong;
		else if ("DefaultUncorrelated" == strBrdf)
			brdf = Ogre::OceanBrdf::Default | Ogre::OceanBrdf::FLAG_UNCORRELATED;
		else if ("DefaultSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::OceanBrdf::Default | Ogre::OceanBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL;
		else if ("CookTorranceSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::OceanBrdf::CookTorrance | Ogre::OceanBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL;
		else if ("BlinnPhongSeparateDiffuseFresnel" == strBrdf)
			brdf = Ogre::OceanBrdf::BlinnPhong | Ogre::OceanBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL;

		return static_cast<Ogre::OceanBrdf::OceanBrdf>(brdf);
	}

	void OceanComponent::setCameraId(unsigned long cameraId)
	{
		this->cameraId->setValue(cameraId);

		this->destroyOcean();
		this->createOcean();
	}

	void OceanComponent::setDeepColour(const Ogre::Vector3& deepColour)
	{
		this->deepColour->setValue(deepColour);

		if (nullptr != this->datablock)
		{
			const Ogre::Vector3 c = deepColour;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setDeepColour", _1(c),
			{
				if (nullptr != this->datablock)
				{
					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setDeepColour(c);
				}
			});
		}
	}

	Ogre::Vector3 OceanComponent::getDeepColour(void) const
	{
		return this->deepColour->getVector3();
	}

	void OceanComponent::setShallowColour(const Ogre::Vector3& shallowColour)
	{
		this->shallowColour->setValue(shallowColour);

		if (nullptr != this->datablock)
		{
			const Ogre::Vector3 c = shallowColour;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setShallowColour", _1(c),
			{
				if (nullptr != this->datablock)
				{
					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setShallowColour(c);
				}
			});
		}
	}

	Ogre::Vector3 OceanComponent::getShallowColour(void) const
	{
		return this->shallowColour->getVector3();
	}

	void OceanComponent::setBrdf(const Ogre::String& brdfStr)
	{
		// Keep Variant in sync
		if (this->brdf)
			this->brdf->setListSelectedValue(brdfStr);

		if (!this->datablock)
			return;

		const Ogre::OceanBrdf::OceanBrdf brdfFlags = this->mapStringToOceanBrdf(brdfStr);

		// Apply on render thread (match your macro name/signature)
		ENQUEUE_RENDER_COMMAND_MULTI("OceanComponent::setBrdf", _1(brdfFlags),
		{
			auto* oceanDb = static_cast<Ogre::HlmsOceanDatablock*>(this->datablock);
			if (oceanDb)
				oceanDb->setBrdf(brdfFlags);
		});
	}

	Ogre::String OceanComponent::getBrdf(void) const
	{
		if (this->brdf)
			return this->brdf->getListSelectedValue();
		return "Default";
	}

	void OceanComponent::setShaderWavesScale(Ogre::Real wavesScale)
	{
		if (wavesScale < Ogre::Real(0.0f))
			wavesScale = Ogre::Real(0.0f);

		this->shaderWavesScale->setValue(wavesScale);

		if (nullptr != this->datablock)
		{
			const Ogre::Real s = wavesScale;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setShaderWavesScale", _1(s),
			{
				if (nullptr != this->datablock)
				{
					static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)->setWavesScale(s);
				}
			});
		}
	}

	Ogre::Real OceanComponent::getShaderWavesScale(void) const
	{
		return this->shaderWavesScale->getReal();
	}

	void OceanComponent::setWavesIntensity(Ogre::Real intensity)
	{
		if (intensity < Ogre::Real(0.0f))
			intensity = Ogre::Real(0.0f);

		this->wavesIntensity->setValue(intensity);

		if (nullptr != this->ocean)
		{
			const Ogre::Real s = intensity;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setWavesIntensity", _1(s),
			{
				if (nullptr != this->ocean)
				{
					this->ocean->setWavesIntensity(s);
				}
			});
		}
	}

	Ogre::Real OceanComponent::getWavesIntensity(void) const
	{
		return this->wavesIntensity->getReal();
	}

	void OceanComponent::setOceanWavesScale(Ogre::Real wavesScale)
	{
		if (wavesScale < Ogre::Real(0.0f))
			wavesScale = Ogre::Real(0.0f);

		this->oceanWavesScale->setValue(wavesScale);

		if (nullptr != this->ocean)
		{
			const Ogre::Real s = wavesScale;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setOceanWavesScale", _1(s),
			{
				if (nullptr != this->ocean)
				{
					this->ocean->setWavesScale(s);
				}
			});
		}
	}

	Ogre::Real OceanComponent::getOceanWavesScale(void) const
	{
		return this->oceanWavesScale->getReal();
	}
	
	void OceanComponent::setOceanSize(const Ogre::Vector2& size)
	{
		this->oceanSize->setValue(size);
	}

	Ogre::Vector2 OceanComponent::getOceanSize(void) const
	{
		return this->oceanSize->getVector2();
	}

	void OceanComponent::setOceanCenter(const Ogre::Vector3& center)
	{
		this->oceanCenter->setValue(center);
	}

	Ogre::Vector3 OceanComponent::getOceanCenter(void) const
	{
		return this->oceanCenter->getVector3();
	}

	unsigned int OceanComponent::geCameraId(void) const
	{
		return this->cameraId->getULong();
	}

	Ogre::Ocean* OceanComponent::getOcean(void) const
	{
		return this->ocean;
	}

}; // namespace end