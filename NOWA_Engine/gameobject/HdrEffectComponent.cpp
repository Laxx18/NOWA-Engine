#include "NOWAPrecompiled.h"
#include "HdrEffectComponent.h"
#include "utilities/XMLConverter.h"
#include "LightDirectionalComponent.h"
#include "WorkspaceComponents.h"
#include "CameraComponent.h"
#include "main/AppStateManager.h"
#include "main/ProcessManager.h"
#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	HdrEffectComponent::HdrEffectComponent()
		: GameObjectComponent(),
		effectName(new Variant(HdrEffectComponent::AttrEffectName(), { "Bright, sunny day", "Scary Night", "Dark Night", "Dream Night", "Average, slightly hazy day", "Heavy overcast day", "Gibbous moon night", "JJ Abrams style", "Custom" }, this->attributes)),
		skyColor(new Variant(HdrEffectComponent::AttrSkyColor(), Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f, this->attributes)),
		upperHemisphere(new Variant(HdrEffectComponent::AttrUpperHemisphere(), Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f, this->attributes)),
		lowerHemisphere(new Variant(HdrEffectComponent::AttrLowerHemisphere(), Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f, this->attributes)),
		sunPower(new Variant(HdrEffectComponent::AttrSunPower(), 97.0f, this->attributes)),
		exposure(new Variant(HdrEffectComponent::AttrExposure(), 0.0f , this->attributes)),
		minAutoExposure(new Variant(HdrEffectComponent::AttrMinAutoExposure(), -1.0f, this->attributes)),
		maxAutoExposure(new Variant(HdrEffectComponent::AttrMaxAutoExposure(), 2.5f, this->attributes)),
		bloom(new Variant(HdrEffectComponent::AttrBloom(), 5.0f, this->attributes)),
		envMapScale(new Variant(HdrEffectComponent::AttrEnvMapScale(), 16.0f, this->attributes)),
		lightDirectionalComponent(nullptr),
		workspaceBaseComponent(nullptr)
	{
		this->effectName->addUserData(GameObject::AttrActionNeedRefresh());
		this->skyColor->addUserData(GameObject::AttrActionColorDialog());
		this->upperHemisphere->addUserData(GameObject::AttrActionColorDialog());
		this->lowerHemisphere->addUserData(GameObject::AttrActionColorDialog());
		this->sunPower->setConstraints(0.0f, 100.0f);
		this->bloom->setConstraints(0.0001f, 5.0f);

		this->exposure->setDescription("Modifies the HDR Materials for the new exposure parameters. "
			"By default the HDR implementation will try to auto adjust the exposure based on the scene's average luminance. "
			"If left unbounded, even the darkest scenes can look well lit and the brigthest scenes appear too normal. "
			"These parameters are useful to prevent the auto exposure from jumping too much from one extreme to the otherand provide "
			"a consistent experience within the same lighting conditions. (e.g.you may want to change the params when going from indoors to outdoors)"
		    "The smaller the gap between minAutoExposure & maxAutoExposure, the less the auto exposure tries to auto adjust to the scene's lighting conditions. "
		    "The first value is exposure. Valid range is [-4.9; 4.9]. Low values will make the picture darker. Higher values will make the picture brighter.");

		this->minAutoExposure->setDescription("Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure darkens a bright scene. "
			"To prevent that looking at a very bright object makes the rest of the scene really dark, use higher values.");

		this->maxAutoExposure->setDescription("Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure brightens a dark scene. "
			"To prevent that looking at a very dark object makes the rest of the scene really bright, use lower values.");

		this->bloom->setDescription("Controls the bloom intensity. Scale is in lumens / 1024. Valid range is [0.01; 4.9].");

		this->upperHemisphere->setDescription("Ambient color when the surface normal is close to hemisphereDir.");
		this->lowerHemisphere->setDescription("Ambient color when the surface normal is pointing away from hemisphereDir.");
		this->envMapScale->setDescription("Global scale to apply to all environment maps (for relevant Hlms implementations, "
			"like PBS). The value will be stored in upperHemisphere.a. Use 1.0 to disable.");
	}

	HdrEffectComponent::~HdrEffectComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Destructor hdr effect component for game object: " + this->gameObjectPtr->getName());
		
		this->lightDirectionalComponent = nullptr;
		this->workspaceBaseComponent = nullptr;
	}

	bool HdrEffectComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EffectName")
		{
			this->effectName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkyColor")
		{
			this->skyColor->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpperHemisphere")
		{
			this->upperHemisphere->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LowerHemisphere")
		{
			this->lowerHemisphere->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SunPower")
		{
			this->sunPower->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Exposure")
		{
			this->exposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAutoExposure")
		{
			this->minAutoExposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAutoExposure")
		{
			this->maxAutoExposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Bloom")
		{
			this->bloom->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnvMapScale")
		{
			this->envMapScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr HdrEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool HdrEffectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Init hdr effect component for game object: " + this->gameObjectPtr->getName());

		// Get the sun light (directional light for sun power setting)
		GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_LIGHT_ID);

		if (nullptr == lightGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent] Could not find 'SunLight' for this component! Affected game object: " + this->gameObjectPtr->getName());
			return false;
		}

		auto& lightDirectionalCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
		if (nullptr != lightDirectionalCompPtr)
		{
			this->lightDirectionalComponent = lightDirectionalCompPtr.get();
		}

		auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
		if (nullptr != workspaceBaseCompPtr)
		{
			this->workspaceBaseComponent = workspaceBaseCompPtr.get();
			if (false == this->workspaceBaseComponent->getUseHdr())
			{
				this->workspaceBaseComponent->setUseHdr(true);
			}
		}
		
		return true;
	}

	bool HdrEffectComponent::connect(void)
	{
		auto& cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		if (nullptr != cameraCompPtr)
		{
			// Note: If its a camera, e.g. MainCamera and hdr effect shall be set, but there is split screen scenario active, the hdr effect may not be set for a camera, which is not involved in split screen scenario
			if ((true == cameraCompPtr->isActivated() && false == WorkspaceModule::getInstance()->getSplitScreenScenarioActive()) || true == this->workspaceBaseComponent->getInvolvedInSplitScreen())
			{
				// Apply loaded effect
				this->setEffectName(this->effectName->getListSelectedValue());

				this->postApplySunPower();
			}
		}
		return true;
	}

	bool HdrEffectComponent::disconnect(void)
	{
		this->resetShining();
		return true;
	}

	void HdrEffectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (HdrEffectComponent::AttrEffectName() == attribute->getName())
		{
			this->setEffectName(attribute->getListSelectedValue());
		}
		else if (HdrEffectComponent::AttrSkyColor() == attribute->getName())
		{
			this->setSkyColor(attribute->getVector4());
		}
		else if (HdrEffectComponent::AttrUpperHemisphere() == attribute->getName())
		{
			this->setUpperHemisphere(attribute->getVector4());
		}
		else if (HdrEffectComponent::AttrLowerHemisphere() == attribute->getName())
		{
			this->setLowerHemisphere(attribute->getVector4());
		}
		else if (HdrEffectComponent::AttrSunPower() == attribute->getName())
		{
			this->setSunPower(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrExposure() == attribute->getName())
		{
			this->setExposure(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrMinAutoExposure() == attribute->getName())
		{
			this->setMinAutoExposure(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrMaxAutoExposure() == attribute->getName())
		{
			this->setMaxAutoExposure(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrBloom() == attribute->getName())
		{
			this->setBloom(attribute->getReal());
		}
		else if (HdrEffectComponent::AttrEnvMapScale() == attribute->getName())
		{
			this->setEnvMapScale(attribute->getReal());
		}
	}

	void HdrEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EffectName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->effectName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SkyColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skyColor->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UpperHemisphere"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->upperHemisphere->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LowerHemisphere"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lowerHemisphere->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SunPower"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sunPower->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Exposure"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->exposure->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAutoExposure"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAutoExposure->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAutoExposure"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAutoExposure->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Bloom"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bloom->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnvMapScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->envMapScale->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String HdrEffectComponent::getClassName(void) const
	{
		return "HdrEffectComponent";
	}

	Ogre::String HdrEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void HdrEffectComponent::onRemoveComponent(void)
	{
		// Kill all processes if component is removed (e.g. world changed), because below a DelayProcess is involved, which would work with corrupt data.
		NOWA::ProcessManager::getInstance()->clearAllProcesses();

		this->resetShining();
	}

	void HdrEffectComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& lightDirectionalCompPtr = boost::dynamic_pointer_cast<LightDirectionalComponent>(gameObjectCompPtr);
			if (nullptr != lightDirectionalCompPtr)
			{
				this->lightDirectionalComponent = nullptr;
			}
		}
	}

	void HdrEffectComponent::resetShining(void)
	{
		if (true == Core::getSingletonPtr()->getIsWorldBeingDestroyed())
		{
			return;
		}
		// Reset shining and set default values
		if (nullptr != this->lightDirectionalComponent)
		{
			this->lightDirectionalComponent->setPowerScale(3.14159f);
		}
		this->gameObjectPtr->getSceneManager()->setAmbientLight(Ogre::ColourValue(0.03375f, 0.05625f, 0.07875f), Ogre::ColourValue(0.04388f, 0.03291f, 0.02194f, 0.07312f), this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir());
	}

	void HdrEffectComponent::postApplySunPower(void)
	{
		// Strange thing fix:
		// If Loading hdr effect, scene is dark, even if the powerscale is set properly.
		// If setting it afterwards some frames, it does work^^
		if (nullptr != this->lightDirectionalComponent)
		{
			NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.1f));
			// Shows for a specific amount of time
			auto ptrFunction = [this]()
				{
					this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
				};
			NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
			delayProcess->attachChild(closureProcess);
			NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
		}
	}

	void HdrEffectComponent::applyHdrSkyColor(const Ogre::ColourValue& color, Ogre::Real multiplier)
	{
		if (nullptr == this->workspaceBaseComponent || false == this->workspaceBaseComponent->getUseHdr() || nullptr == this->workspaceBaseComponent->getWorkspace())
		{
			this->resetShining();
			return;
		}

		Ogre::CompositorNode* node = this->workspaceBaseComponent->getWorkspace()->findNode(this->workspaceBaseComponent->getRenderingNodeName());

		if (nullptr == node)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "No node '" + this->workspaceBaseComponent->getRenderingNodeName() + "' in provided workspace ", "HdrEffectComponent::applyHdrSkyColor");
		}

		const Ogre::CompositorPassVec passes = node->_getPasses();

		assert(passes.size() >= 1);
		Ogre::CompositorPass* pass = passes[0];

		Ogre::RenderPassDescriptor* renderPassDesc = pass->getRenderPassDesc();
		renderPassDesc->setClearColour(color * multiplier);

		//Set the definition as well, although this isn't strictly necessary.
		Ogre::CompositorManager2* compositorManager = this->workspaceBaseComponent->getWorkspace()->getCompositorManager();
		Ogre::CompositorNodeDef* nodeDef = compositorManager->getNodeDefinitionNonConst(this->workspaceBaseComponent->getRenderingNodeName());

		assert(nodeDef->getNumTargetPasses() >= 1);

		Ogre::CompositorTargetDef* targetDef = nodeDef->getTargetPass(0);
		const Ogre::CompositorPassDefVec& passDefs = targetDef->getCompositorPasses();

		assert(passDefs.size() >= 1);
		Ogre::CompositorPassDef* passDef = passDefs[0];

		passDef->setAllClearColours(color * multiplier);
	}

	void HdrEffectComponent::applyExposure(Ogre::Real exposure, Ogre::Real minAutoExposure, Ogre::Real maxAutoExposure)
	{
		if (nullptr == this->workspaceBaseComponent || false == this->workspaceBaseComponent->getUseHdr() || nullptr == this->workspaceBaseComponent->getWorkspace())
		{
			return;
		}

		assert(minAutoExposure <= maxAutoExposure);

		Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().load("HDR/DownScale03_SumLumEnd", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

		Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
		Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

		const Ogre::Vector3 exposureParams(1024.0f * expf(exposure - 2.0f), 7.5f - maxAutoExposure, 7.5f - minAutoExposure);

		psParams = pass->getFragmentProgramParameters();
		psParams->setNamedConstant("exposure", exposureParams);
	}
	//-----------------------------------------------------------------------------------
	void HdrEffectComponent::applyBloomThreshold(Ogre::Real minThreshold, Ogre::Real fullColorThreshold)
	{
		if (nullptr == this->workspaceBaseComponent || false == this->workspaceBaseComponent->getUseHdr() || nullptr == this->workspaceBaseComponent->getWorkspace())
		{
			return;
		}

		assert(minThreshold < fullColorThreshold);

		Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().load("HDR/BrightPass_Start", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
		Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

		Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
		psParams->setNamedConstant("brightThreshold", Ogre::Vector4(minThreshold, 1.0f / (fullColorThreshold - minThreshold), 0, 0));
	}

	void HdrEffectComponent::setEffectName(const Ogre::String& effectName)
	{
		this->effectName->setListSelectedValue(effectName);

		Ogre::ColourValue oldUpperHemisphere;
		Ogre::ColourValue oldLowerHemisphere;

		if ("Bright, sunny day" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(97.0f);
			this->exposure->setValue(0.0f);
			this->minAutoExposure->setValue(-1.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Scary Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(90.0f);
			this->exposure->setValue(-2.0f);
			this->minAutoExposure->setValue(-3.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(2.674f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Dark Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(30.0f);
			this->exposure->setValue(-2.0f);
			this->minAutoExposure->setValue(-2.5f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(0.1f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Dream Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
			this->sunPower->setValue(97.0f);
			this->exposure->setValue(0.1f);
			this->minAutoExposure->setValue(-1.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(0.001f);
			this->envMapScale->setValue(16.0f);
		}
		else if ("Average, slightly hazy day" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 32.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 3.15f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.0475f);
			this->sunPower->setValue(48.0f);
			this->exposure->setValue(0.0f);
			this->minAutoExposure->setValue(-2.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(8.0f);
		}
		else if ("Heavy overcast day" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.4f, 0.4f, 0.4f, 1.0f) * 4.5f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.4f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.365625f);
			this->sunPower->setValue(6.0625f);
			this->exposure->setValue(0.0f);
			this->minAutoExposure->setValue(-2.5f);
			this->maxAutoExposure->setValue(1.0f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(0.5f);
		}
		else if ("Gibbous moon night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.27f, 0.3f, 0.6f, 1.0f) * 0.01831072f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.50f, 1.0f) * 0.003f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.5f, 0.65f, 1.0f) * 0.00274222f);
			this->sunPower->setValue(0.4f);
			this->exposure->setValue(0.65f);
			this->minAutoExposure->setValue(-2.5f);
			this->maxAutoExposure->setValue(3.0f);
			this->bloom->setValue(5.0f);
			this->envMapScale->setValue(0.0152587890625f);
		}
		else if ("JJ Abrams style" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 6.0f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 0.1125f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 0.073125f);
			this->sunPower->setValue(4.0f);
			this->exposure->setValue(0.5f);
			this->minAutoExposure->setValue(1.0f);
			this->maxAutoExposure->setValue(2.5f);
			this->bloom->setValue(3.0f);
			this->envMapScale->setValue(1.0f);
		}
		else if ("Black Night" == effectName)
		{
			this->skyColor->setValue(Ogre::Vector4(0.27f, 0.3f, 0.6f, 1.0f) * 0.01831072f);
			this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.50f, 1.0f) * 0.003f);
			this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.5f, 0.65f, 1.0f) * 0.00274222f);
			this->sunPower->setValue(0.0f);
			this->exposure->setValue(0.65f);
			this->minAutoExposure->setValue(-2.0f);
			this->maxAutoExposure->setValue(-2.0f);
			this->bloom->setValue(0.01f);
			this->envMapScale->setValue(0.0152587890625f);
		}

		// Apply values
		Ogre::ColourValue skyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);
		this->applyHdrSkyColor(skyColor, 1.0f);
		this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
		this->applyBloomThreshold(std::max(this->bloom->getReal() - 2.0f, 0.0f), std::max(this->bloom->getReal(), 0.01f));

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z, this->lowerHemisphere->getVector4().w);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z, this->upperHemisphere->getVector4().w);

		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());

		if (nullptr != this->lightDirectionalComponent)
		{
			this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
		}
	}

	Ogre::String HdrEffectComponent::getEffectName(void) const
	{
		return this->effectName->getListSelectedValue();
	}

	void HdrEffectComponent::setSkyColor(const Ogre::Vector4& skyColor)
	{
		this->skyColor->setValue(skyColor);
		this->effectName->setListSelectedValue("Custom");
		Ogre::ColourValue tempSkyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);

		this->applyHdrSkyColor(tempSkyColor, 1.0f);
	}

	Ogre::Vector4 HdrEffectComponent::getSkyColor(void) const
	{
		return this->skyColor->getVector4();
	}

	void HdrEffectComponent::setUpperHemisphere(const Ogre::Vector4& upperHemisphere)
	{
		this->upperHemisphere->setValue(upperHemisphere);
		this->effectName->setListSelectedValue("Custom");

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z, this->lowerHemisphere->getVector4().w);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z, this->upperHemisphere->getVector4().w);
		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::Vector4 HdrEffectComponent::getUpperHemisphere(void) const
	{
		return this->upperHemisphere->getVector4();
	}

	void HdrEffectComponent::setLowerHemisphere(const Ogre::Vector4& lowerHemisphere)
	{
		this->lowerHemisphere->setValue(lowerHemisphere);
		this->effectName->setListSelectedValue("Custom");

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z, this->lowerHemisphere->getVector4().w);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z, this->upperHemisphere->getVector4().w);
		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::Vector4 HdrEffectComponent::getLowerHemisphere(void) const
	{
		return this->lowerHemisphere->getVector4();
	}

	void HdrEffectComponent::setSunPower(Ogre::Real sunPower)
	{
		this->sunPower->setValue(sunPower);
		this->effectName->setListSelectedValue("Custom");

		if (nullptr != this->lightDirectionalComponent)
		{
			this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
		}
	}

	Ogre::Real HdrEffectComponent::getSunPower(void) const
	{
		return this->sunPower->getReal();
	}

	void HdrEffectComponent::setExposure(Ogre::Real exposure)
	{
		this->exposure->setValue(exposure);
		this->effectName->setListSelectedValue("Custom");

		this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
	}

	Ogre::Real HdrEffectComponent::getExposure(void) const
	{
		return this->exposure->getReal();
	}

	void HdrEffectComponent::setMinAutoExposure(Ogre::Real minAutoExposure)
	{
		if (minAutoExposure > this->maxAutoExposure->getReal())
			this->maxAutoExposure->setValue(minAutoExposure);
		
		this->minAutoExposure->setValue(minAutoExposure);
		this->effectName->setListSelectedValue("Custom");

		this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
	}

	Ogre::Real HdrEffectComponent::getMinAutoExposure(void) const
	{
		return this->minAutoExposure->getReal();
	}

	void HdrEffectComponent::setMaxAutoExposure(Ogre::Real maxAutoExposure)
	{
		if (maxAutoExposure < this->minAutoExposure->getReal())
			this->minAutoExposure->setValue(maxAutoExposure);
		
		this->maxAutoExposure->setValue(maxAutoExposure);
		this->effectName->setListSelectedValue("Custom");

		this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
	}

	Ogre::Real HdrEffectComponent::getMaxAutoExposure(void) const
	{
		return this->maxAutoExposure->getReal();
	}

	void HdrEffectComponent::setBloom(Ogre::Real bloom)
	{
		this->bloom->setValue(bloom);
		this->effectName->setListSelectedValue("Custom");

		this->applyBloomThreshold(std::max(this->bloom->getReal() - 2.0f, 0.0f), std::max(this->bloom->getReal(), 0.01f));
	}

	Ogre::Real HdrEffectComponent::getBloom(void) const
	{
		return this->bloom->getReal();
	}

	void HdrEffectComponent::setEnvMapScale(Ogre::Real envMapScale)
	{
		if (envMapScale < 0.0f)
			envMapScale = 0.0f;

		this->envMapScale->setValue(envMapScale);
		this->effectName->setListSelectedValue("Custom");

		Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
		Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);
		this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
	}

	Ogre::Real HdrEffectComponent::getEnvMapScale(void) const
	{
		return this->envMapScale->getReal();
	}

}; // namespace end