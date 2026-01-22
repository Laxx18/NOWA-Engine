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
		deepColour(new Variant(OceanComponent::AttrDeepColour(), Ogre::Vector3(0.3f, 0.8f, 2.2f), this->attributes)),
		shallowColour(new Variant(OceanComponent::AttrShallowColour(), Ogre::Vector3(0.6f, 1.4f, 2.2f), this->attributes)),
		brdf(new Variant(OceanComponent::AttrBrdf(),
			{ "Default", "CookTorrance", "BlinnPhong",
			  "DefaultUncorrelated", "DefaultSeparateDiffuseFresnel",
			  "CookTorranceSeparateDiffuseFresnel", "BlinnPhongSeparateDiffuseFresnel" }, this->attributes)),
		reflectionTextureName(new Variant(OceanComponent::AttrReflectionTexture(), std::vector<Ogre::String>(), this->attributes)),
		shaderWavesScale(new Variant(OceanComponent::AttrShaderWavesScale(), 1.0f, this->attributes)),
		wavesIntensity(new Variant(OceanComponent::AttrWavesIntensity(), 0.8f, this->attributes)),
		oceanWavesScale(new Variant(OceanComponent::AttrOceanWavesScale(), 1.6f, this->attributes)),
		oceanSize(new Variant(OceanComponent::AttrOceanSize(), Ogre::Vector2(200.0f, 200.0f), this->attributes)),
		oceanCenter(new Variant(OceanComponent::AttrOceanCenter(), Ogre::Vector3::ZERO, this->attributes)),
		useSkirts(new Variant(OceanComponent::AttrUseSkirts(), true, this->attributes)),
		waveTimeScale(new Variant(OceanComponent::AttrWaveTimeScale(), 2.0f, this->attributes)),
		waveFrequencyScale(new Variant(OceanComponent::AttrWaveFrequencyScale(), 1.0f, this->attributes)),
		waveChaos(new Variant(OceanComponent::AttrWaveChaos(), 0.0f, this->attributes)),
		reflectionStrength(new Variant(AttrReflectionStrength(), 1.0f, attributes)),
		baseRoughness(new Variant(AttrBaseRoughness(), 0.01f, attributes)),
		foamRoughness(new Variant(AttrFoamRoughness(), 0.5f, attributes)),
		ambientReduction(new Variant(AttrAmbientReduction(), 0.5f, attributes)),
		diffuseScale(new Variant(AttrDiffuseScale(), 0.01f, attributes)),
		foamIntensity(new Variant(AttrFoamIntensity(), 1.0f, attributes))
	{
		this->cameraId->setDescription("The optional camera game object id which can be set. E.g. useful if the MinimapComponent is involved, to set the minimap camera, so that ocean is painted correctly on minimap. Can be left of, default is the main active camera.");

		this->deepColour->setDescription("Ocean deep water colour (Vector3 RGB). Example values: Dark stormy ocean: 0.10 0.15 0.40, Natural deep blue ocean: 0.13 0.22 0.57, "
			"Tropical deep water: 0.20 0.30 0.60. Use lower values for darker oceans and higher values for clearer water. Values are multiplied internally by INV_PI.");
		this->deepColour->addUserData(GameObject::AttrActionColorDialog());


		this->shallowColour->setDescription("Ocean shallow water colour (Vector3 RGB). Example values: Cold shallow water: 0.25 0.50 0.80, Natural wave crests: 0.31 0.79 1.41, "
			"Tropical turquoise water: 0.60 1.40 2.20. Use higher and more greenish values than deepColour to get bright waves and foam. Values are multiplied internally by INV_PI.");
		this->shallowColour->addUserData(GameObject::AttrActionColorDialog());

		this->brdf->setDescription("Ocean BRDF model (enum as integer, e.g. BlinnPhong).");

		// ==================== Wave shape & detail ====================

		this->shaderWavesScale->setDescription(
			"Shader-side wave detail scale. Controls the frequency of fine surface detail and normals. "
			"Lower values smooth the surface, higher values add sharper, more detailed ripples.");
		this->shaderWavesScale->setConstraints(0.25f, 4.0f);

		this->wavesIntensity->setDescription(
			"Wave amplitude / height scale. Controls the vertical displacement of the ocean surface. "
			"Higher values create larger waves and stronger silhouettes.");
		this->wavesIntensity->setConstraints(0.0f, 2.0f);

		this->oceanWavesScale->setDescription(
			"Wave size / tiling scale. Controls the overall wavelength of ocean waves. "
			"Lower values create denser, choppier waves, higher values produce large, slow swells.");
		this->oceanWavesScale->setConstraints(0.25f, 4.0f);

		// ==================== Ocean placement & geometry ====================

		this->oceanSize->setDescription(
			"Ocean size in world units. Defines the horizontal extent of the ocean surface (X/Z). "
			"Larger values cover more area but may increase update cost.");

		this->oceanCenter->setDescription(
			"Ocean center position in world space. The ocean surface is generated symmetrically around this point.");

		this->useSkirts->setDescription(
			"Enables skirts on distant ocean cells to hide LOD cracks. "
			"Recommended for above-water views; automatically disabled when underwater.");

		// ==================== Wave animation ====================

		this->waveTimeScale->setDescription(
			"Global wave animation speed multiplier. Scales the time progression of all wave motion. "
			"Values above 1.0 speed up the ocean, lower values slow it down.");
		this->waveTimeScale->setConstraints(0.0f, 3.0f);

		this->waveFrequencyScale->setDescription(
			"Wave frequency scale. Controls how often wave patterns repeat across the surface. "
			"Higher values increase wave density, lower values create broader, smoother patterns.");
		this->waveFrequencyScale->setConstraints(0.25f, 2.5f);

		this->waveChaos->setDescription(
			"Wave chaos factor. Adds time-varying rotation and scale offsets to reduce visible repetition. "
			"Small values add natural variation; high values create chaotic, unstable motion.");
		this->waveChaos->setConstraints(0.0f, 1.0f);

		// ==================== Reflection & shading ====================

		this->reflectionStrength->setDescription(
			"Reflection strength. Controls the intensity of specular environment reflections on the water surface. "
			"Lower values reduce mirror-like appearance.");
		this->reflectionStrength->setConstraints(0.0f, 1.0f);

		this->baseRoughness->setDescription(
			"Base water surface roughness. Controls the sharpness of reflections on calm water areas. "
			"Lower values produce crisp reflections, higher values blur them.");
		this->baseRoughness->setConstraints(0.001f, 1.0f);

		this->foamRoughness->setDescription(
			"Foam roughness. Controls reflection sharpness in foam-covered areas. "
			"Higher values soften highlights and reduce specular intensity on foam.");
		this->foamRoughness->setConstraints(0.001f, 1.0f);

		this->ambientReduction->setDescription(
			"Ambient light reduction factor. Reduces the contribution of ambient lighting on the ocean surface. "
			"Useful to prevent overly bright or flat-looking water.");
		this->ambientReduction->setConstraints(0.0f, 1.0f);

		this->diffuseScale->setDescription(
			"Diffuse lighting scale. Controls the amount of diffuse light applied to the water surface. "
			"Water typically relies more on reflections than diffuse shading.");
		this->diffuseScale->setConstraints(0.0f, 1.0f);

		this->foamIntensity->setDescription(
			"Foam intensity. Controls overall visibility and brightness of foam patterns on the water surface.");
		this->foamIntensity->setConstraints(0.0f, 1.0f);

		this->reflectionTextureName->setDescription("Reflection cubemap texture used for ocean specular reflections (Skies/*.dds).");
		this->setReflectionTextureNames();
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionTextureName")
		{
			this->reflectionTextureName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseSkirts")
		{
			this->useSkirts->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveTimeScale")
		{
			this->waveTimeScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveFrequencyScale")
		{
			this->waveFrequencyScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveChaos")
		{
			this->waveChaos->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionStrength")
		{
			reflectionStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BaseRoughness")
		{
			baseRoughness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.01f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FoamRoughness")
		{
			foamRoughness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AmbientReduction")
		{
			ambientReduction->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseScale")
		{
			diffuseScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.01f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FoamIntensity")
		{
			foamIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
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

		// Populate reflection texture dropdown and apply selection
		this->setReflectionTextureNames();
		this->setReflectionTextureName(this->reflectionTextureName->getListSelectedValue());

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
			auto closureFunction = [this](Ogre::Real renderDt)
			{
				this->ocean->update(renderDt);
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

				// IMPORTANT: must be set before create() because it affects cell geometry
				this->ocean->setUseSkirts(this->useSkirts->getBool());
				Ogre::Vector3 center = this->oceanCenter->getVector3();
				Ogre::Vector2 size = this->oceanSize->getVector2();
				this->ocean->create(center, size);

				// Runtime wave parameters (Ocean side)
				this->ocean->setWavesIntensity(this->wavesIntensity->getReal());
				this->ocean->setWavesScale(this->oceanWavesScale->getReal());

				this->ocean->setWaveTimeScale(this->waveTimeScale->getReal());
				this->ocean->setWaveFrequencyScale(this->waveFrequencyScale->getReal());
				this->ocean->setWaveChaos(this->waveChaos->getReal());

				this->ocean->setBasePixelDimension(32);

				auto* hlmsOcean = static_cast<Ogre::HlmsOcean*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER1));

				hlmsOcean->setOceanDataTextureName("oceanData.dds");
				hlmsOcean->setWeightTextureName("weight.dds");

				hlmsOcean->setAmbientLightMode(this->mapPbsLightModeToOceanMapMode(NOWA::WorkspaceModule::getInstance()->getAmbientLightMode()));
				hlmsOcean->setShadowSettings(this->mapPbsShadowSettingsToOceanShadowSettings(NOWA::WorkspaceModule::getInstance()->getShadowQuality()));

				if (nullptr == this->datablock)
				{
					Ogre::String datablockName = "Ocean_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

					this->datablock = static_cast<Ogre::HlmsOceanDatablock*>(hlmsOcean->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec()));

					this->ocean->setDatablock(this->datablock);

					this->datablock->setDeepColour(this->deepColour->getVector3());
					this->datablock->setShallowColour(this->shallowColour->getVector3());

					this->datablock->setReflectionStrength(reflectionStrength->getReal());
					this->datablock->setBaseRoughness(baseRoughness->getReal());
					this->datablock->setFoamRoughness(foamRoughness->getReal());
					this->datablock->setAmbientReduction(ambientReduction->getReal());
					this->datablock->setDiffuseScale(diffuseScale->getReal());
					this->datablock->setFoamIntensity(foamIntensity->getReal());

					// Transparency will not work:
					// this->datablock->setTransparency(0.5f);

					/*You have THREE choices :

					OPTION A : Live with minor seams(RECOMMENDED)
						- Keep + 1 vertices
						- Use large cells(basePixelDimension = 256)
						- Seams are minimal and realistic
						- Ocean looks good from normal viewing distance

						OPTION B : No transparency
						- Keep + 1 vertices
						- setTransparency(1.0f) = fully opaque
						- No seams because no alpha blending
						- Ocean is solid(like in your original code)

						OPTION C : Advanced mesh solution(COMPLEX)
						- Modify vertex shader to adjust edge vertices
						- Requires deep understanding of mesh generation
						- Not recommended unless you're an expert*/

					/*
					RECOMMENDED SETTINGS FOR BEST RESULTS:

					For Clear Water (minimal borders):
					- setBasePixelDimension(256)
					- setTransparency(0.4f)
					- setBaseRoughness(0.01f)

					For Realistic Ocean (balanced):
					- setBasePixelDimension(128)
					- setTransparency(0.6f)
					- setBaseRoughness(0.02f)

					For Murky/Deep Water:
					- setBasePixelDimension(64)
					- setTransparency(0.85f)
					- setBaseRoughness(0.05f)

					PERFORMANCE NOTE:
					Larger basePixelDimension = fewer cells = better performance
					But also less geometric detail in waves

					VISUAL QUALITY:
					The fresnel effect makes water:
					- More transparent when looking straight down
					- More opaque at grazing angles (horizon)
					This is physically accurate for water!

					*/

					this->datablock->setBrdf(this->mapStringToOceanBrdf(this->brdf->getListSelectedValue()));

					// Shader-side detail scale
					this->datablock->setWavesScale(this->shaderWavesScale->getReal());

					this->setReflectionTextureName(this->reflectionTextureName->getListSelectedValue());
				}

				// Example json material usage:
				/*Ogre::HlmsDatablock* datablock = WorkspaceModule::getInstance()->getHlmsManager()->getDatablock("OceanDefaultMaterial");
				if (nullptr != datablock)
				{
					this->ocean->setDatablock(datablock);
				}*/

				this->ocean->setStatic(false);
				this->gameObjectPtr->setDynamic(true);
				this->gameObjectPtr->getSceneNode()->attachObject(this->ocean);

				this->gameObjectPtr->init(this->ocean);

				this->ocean->update(0.0f);
				// Register after the component has been created
				AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);

				WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
				if (nullptr != workspaceBaseComponent)
				{
					workspaceBaseComponent->setUseOcean(true, this);
				}
			});
		}
	}

	void OceanComponent::setReflectionTextureNames(void)
	{
		if (nullptr == this->reflectionTextureName)
		{
			return;
		}

		const Ogre::String prevSelected = this->reflectionTextureName->getListSelectedValue();

		std::vector<Ogre::String> compatibleSkyNames;
		compatibleSkyNames.reserve(64u);
		compatibleSkyNames.emplace_back("");

		try
		{
			if (Ogre::ResourceGroupManager::getSingleton().resourceGroupExists("Skies"))
			{
				Ogre::StringVectorPtr skyNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Skies", "*.dds");
				if (false == skyNames.isNull())
				{
					for (auto it = skyNames->cbegin(); it != skyNames->cend(); ++it)
						compatibleSkyNames.emplace_back(*it);
				}
			}
		}
		catch (const Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OceanComponent] setReflectionTextureNames exception: " + e.getFullDescription());
		}

		this->reflectionTextureName->getList().clear();
		this->reflectionTextureName->setValue(compatibleSkyNames);

		// Keep selection (even if it is not currently present in the list)
		this->reflectionTextureName->setListSelectedValue(prevSelected);
	}

	void OceanComponent::setReflectionTextureName(const Ogre::String& textureName)
	{
		if (nullptr != this->reflectionTextureName)
		{
			this->reflectionTextureName->setListSelectedValue(textureName);
		}

		// Apply to HLMS (global)
		this->setEnvTexture(textureName);
	}

	void OceanComponent::setEnvTexture(const Ogre::String& envTextureName)
	{
		const Ogre::String textureName = envTextureName;
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setEnvTexture", _1(textureName),
			{
				Ogre::Root* rootLocal = Ogre::Root::getSingletonPtr();
				if (nullptr == rootLocal)
					return;

				auto* hlmsOcean = static_cast<Ogre::HlmsOcean*>(Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_USER1));

				if (hlmsOcean)
				{
					// This will internally call flushRenderables()
					hlmsOcean->setDatablockEnvReflection(this->datablock, textureName);
				}
			});
	}

	Ogre::String OceanComponent::getReflectionTextureName(void) const
	{
		if (nullptr != this->reflectionTextureName)
			return this->reflectionTextureName->getListSelectedValue();
		return "";
	}

	void OceanComponent::actualizeShading(void)
	{
		auto* hlmsOcean = static_cast<Ogre::HlmsOcean*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER1));

		hlmsOcean->setAmbientLightMode(this->mapPbsLightModeToOceanMapMode(NOWA::WorkspaceModule::getInstance()->getAmbientLightMode()));
		hlmsOcean->setShadowSettings(this->mapPbsShadowSettingsToOceanShadowSettings(NOWA::WorkspaceModule::getInstance()->getShadowQuality()));
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

		NOWA::GraphicsModule::DestroyCommand destroyCommand = [ocean, gameObjectPtr, workspaceBaseComponent, appState, datablock]() mutable
		{
			if (nullptr != workspaceBaseComponent)
			{
				if (nullptr != appState && false == appState->bShutdown)
				{
					workspaceBaseComponent->setUseOcean(false, nullptr);
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
					gameObjectPtr->getSceneNode()->detachObject(ocean);
				}

				delete ocean;
				ocean = nullptr;
			}

			if (nullptr != gameObjectPtr)
			{
				gameObjectPtr->movableObject = nullptr;
			}

			if (nullptr != gameObjectPtr && nullptr != gameObjectPtr->sceneManager &&
				nullptr != gameObjectPtr->boundingBoxDraw)
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
		else if (OceanComponent::AttrReflectionTexture() == attribute->getName())
		{
			this->setReflectionTextureName(attribute->getListSelectedValue());
		}
		else if (OceanComponent::AttrShaderWavesScale() == attribute->getName())
		{
			this->setOceanWavesScale(attribute->getReal());
		}
		else if (OceanComponent::AttrWavesIntensity() == attribute->getName())
		{
			this->setWavesIntensity(attribute->getReal());
		}
		else if (OceanComponent::AttrOceanWavesScale() == attribute->getName())
		{
			this->setShaderWavesScale(attribute->getReal());
		}
		else if (OceanComponent::AttrWaveTimeScale() == attribute->getName())
		{
			this->setWaveTimeScale(attribute->getReal());
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
		else if (AttrReflectionStrength() == attribute->getName())
		{
			setReflectionStrength(attribute->getReal());
		}
		else if (AttrBaseRoughness() == attribute->getName())
		{
			setBaseRoughness(attribute->getReal());
		}
		else if (AttrFoamRoughness() == attribute->getName())
		{
			setFoamRoughness(attribute->getReal());
		}
		else if (AttrAmbientReduction() == attribute->getName())
		{
			setAmbientReduction(attribute->getReal());
		}
		else if (AttrDiffuseScale() == attribute->getName())
		{
			setDiffuseScale(attribute->getReal());
		}
		else if (AttrFoamIntensity() == attribute->getName())
		{
			setFoamIntensity(attribute->getReal());
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

		// ReflectionTextureName
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reflectionTextureName->getListSelectedValue())));
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

		// UseSkirts
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseSkirts"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useSkirts->getBool())));
		propertiesXML->append_node(propertyXML);

		// WaveTimeScale
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaveTimeScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waveTimeScale->getReal())));
		propertiesXML->append_node(propertyXML);

		// WaveFrequencyScale
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaveFrequencyScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waveFrequencyScale->getReal())));
		propertiesXML->append_node(propertyXML);

		// WaveChaos
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaveChaos"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waveChaos->getReal())));
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionStrength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reflectionStrength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BaseRoughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->baseRoughness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FoamRoughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamRoughness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AmbientReduction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ambientReduction->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FoamIntensity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamIntensity->getReal())));
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

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

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

	Ogre::HlmsOcean::AmbientLightMode OceanComponent::mapPbsLightModeToOceanMapMode(Ogre::HlmsPbs::AmbientLightMode pbsMode)
	{
		using P = Ogre::HlmsPbs;
		using O = Ogre::HlmsOcean;

		switch (pbsMode)
		{
		case P::AmbientAutoNormal:
			return O::AmbientAuto;
		case P::AmbientFixed:
			return O::AmbientFixed;
		case P::AmbientHemisphereNormal:
			return O::AmbientHemisphere;

			// Ocean doesn't support spherical harmonics.
			// Best fallback is Auto (it will pick Fixed vs Hemisphere based on upper/lower,
			// and disable if both are black).
		case P::AmbientSh:
		case P::AmbientShMonochrome:
			return O::AmbientFixed;
		case P::AmbientNone:
			return O::AmbientNone;

		default:
			return O::AmbientAuto;
		}
	}

	Ogre::HlmsOcean::ShadowFilter OceanComponent::mapPbsShadowSettingsToOceanShadowSettings(Ogre::HlmsPbs::ShadowFilter pbsFilter)
	{
		using P = Ogre::HlmsPbs;
		using O = Ogre::HlmsOcean;

		switch (pbsFilter)
		{
		case P::PCF_2x2: return O::PCF_2x2;
		case P::PCF_3x3: return O::PCF_3x3;
		case P::PCF_4x4: return O::PCF_4x4;

			// Ocean doesn't support 5x5 / 6x6; clamp to best available
		case P::PCF_5x5:
		case P::PCF_6x6:
			return O::PCF_4x4;

			// Ocean doesn't support ESM; fall back to highest PCF quality
		case P::ExponentialShadowMaps:
			return O::PCF_4x4;
		default:
			return O::PCF_3x3;
		}
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
	
	void OceanComponent::setUseSkirts(bool useSkirts)
	{
		this->useSkirts->setValue(useSkirts);

		// Needs rebuild to change geometry, handled by caller (actualizeValue)
		if (nullptr != this->ocean)
		{
			const bool s = useSkirts;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setUseSkirts", _1(s),
			{
				if (nullptr != this->ocean)
				{
					this->ocean->setUseSkirts(s);
				}
			});
		}
	}

	bool OceanComponent::getUseSkirts(void) const
	{
		return this->useSkirts->getBool();
	}

	void OceanComponent::setWaveTimeScale(Ogre::Real timeScale)
	{
		if (timeScale < Ogre::Real(0.0f))
			timeScale = Ogre::Real(0.0f);

		this->waveTimeScale->setValue(timeScale);

		if (nullptr != this->ocean)
		{
			const Ogre::Real s = timeScale;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setWaveTimeScale", _1(s),
			{
				if (nullptr != this->ocean)
				{
					this->ocean->setWaveTimeScale(s);
				}
			});
		}
	}

	Ogre::Real OceanComponent::getWaveTimeScale(void) const
	{
		return this->waveTimeScale->getReal();
	}

	void OceanComponent::setWaveFrequencyScale(Ogre::Real freqScale)
	{
		if (freqScale < Ogre::Real(0.0f))
			freqScale = Ogre::Real(0.0f);

		this->waveFrequencyScale->setValue(freqScale);

		if (nullptr != this->ocean)
		{
			const Ogre::Real s = freqScale;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setWaveFrequencyScale", _1(s),
			{
				if (nullptr != this->ocean)
				{
					this->ocean->setWaveFrequencyScale(s);
				}
			});
		}
	}

	Ogre::Real OceanComponent::getWaveFrequencyScale(void) const
	{
		return this->waveFrequencyScale->getReal();
	}

	void OceanComponent::setWaveChaos(Ogre::Real chaos)
	{
		if (chaos < Ogre::Real(0.0f))
			chaos = Ogre::Real(0.0f);

		this->waveChaos->setValue(chaos);

		if (nullptr != this->ocean)
		{
			const Ogre::Real s = chaos;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setWaveChaos", _1(s),
			{
				if (nullptr != this->ocean)
				{
					this->ocean->setWaveChaos(s);
				}
			});
		}
	}

	Ogre::Real OceanComponent::getWaveChaos(void) const
	{
		return this->waveChaos->getReal();
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

	void OceanComponent::setReflectionStrength(Ogre::Real strength)
	{
		this->reflectionStrength->setValue(strength);

		if (nullptr != this->datablock)
		{
			const Ogre::Real s = strength;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setReflectionStrength", _1(s),
				{
					if (nullptr != this->datablock)
					{
						static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)
							->setReflectionStrength(s);
					}
				});
		}
	}

	Ogre::Real OceanComponent::getReflectionStrength(void) const
	{
		return this->reflectionStrength->getReal();
	}

	void OceanComponent::setBaseRoughness(Ogre::Real roughness)
	{
		this->baseRoughness->setValue(roughness);

		if (nullptr != this->datablock)
		{
			const Ogre::Real r = roughness;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setBaseRoughness", _1(r),
				{
					if (nullptr != this->datablock)
					{
						static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)
							->setBaseRoughness(r);
					}
				});
		}
	}

	Ogre::Real OceanComponent::getBaseRoughness(void) const
	{
		return this->baseRoughness->getReal();
	}

	void OceanComponent::setFoamRoughness(Ogre::Real roughness)
	{
		this->foamRoughness->setValue(roughness);

		if (nullptr != this->datablock)
		{
			const Ogre::Real r = roughness;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setFoamRoughness", _1(r),
				{
					if (nullptr != this->datablock)
					{
						static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)
							->setFoamRoughness(r);
					}
				});
		}
	}

	Ogre::Real OceanComponent::getFoamRoughness(void) const
	{
		return this->foamRoughness->getReal();
	}

	void OceanComponent::setAmbientReduction(Ogre::Real reduction)
	{
		this->ambientReduction->setValue(reduction);

		if (nullptr != this->datablock)
		{
			const Ogre::Real r = reduction;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setAmbientReduction", _1(r),
				{
					if (nullptr != this->datablock)
					{
						static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)
							->setAmbientReduction(r);
					}
				});
		}
	}

	Ogre::Real OceanComponent::getAmbientReduction(void) const
	{
		return this->ambientReduction->getReal();
	}

	void OceanComponent::setDiffuseScale(Ogre::Real scale)
	{
		this->diffuseScale->setValue(scale);

		if (nullptr != this->datablock)
		{
			const Ogre::Real s = scale;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setDiffuseScale", _1(s),
				{
					if (nullptr != this->datablock)
					{
						static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)
							->setDiffuseScale(s);
					}
				});
		}
	}

	Ogre::Real OceanComponent::getDiffuseScale(void) const
	{
		return this->diffuseScale->getReal();
	}

	void OceanComponent::setFoamIntensity(Ogre::Real intensity)
	{
		this->foamIntensity->setValue(intensity);

		if (nullptr != this->datablock)
		{
			const Ogre::Real i = intensity;
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OceanComponent::setFoamIntensity", _1(i),
				{
					if (nullptr != this->datablock)
					{
						static_cast<Ogre::HlmsOceanDatablock*>(this->datablock)
							->setFoamIntensity(i);
					}
				});
		}
	}

	Ogre::Real OceanComponent::getFoamIntensity(void) const
	{
		return this->foamIntensity->getReal();
	}

	unsigned int OceanComponent::geCameraId(void) const
	{
		return this->cameraId->getULong();
	}

	Ogre::Ocean* OceanComponent::getOcean(void) const
	{
		return this->ocean;
	}

	Ogre::HlmsOceanDatablock* OceanComponent::getDatablock(void) const
	{
		return this->datablock;
	}

	bool OceanComponent::isCameraUnderwater(void) const
	{
		if (nullptr != this->ocean)
		{
			return this->ocean->isUnderwater();
		}
		return false;
	}

	bool OceanComponent::isUnderWater(const Ogre::Vector3& position) const
	{
		if (nullptr != this->ocean)
		{
			return this->ocean->isUnderwater(position);
		}
		return false;
	}

	bool OceanComponent::isUnderWater(GameObject* gameObject)
	{
		if (nullptr != this->ocean && nullptr != gameObject)
		{
			return this->ocean->isUnderwater(gameObject->getPosition());
		}
		return false;
	}

}; // namespace end