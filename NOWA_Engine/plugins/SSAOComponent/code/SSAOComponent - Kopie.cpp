#include "NOWAPrecompiled.h"
#include "SSAOComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreBitwise.h"
#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SSAOComponent::SSAOComponent()
		: GameObjectComponent(),
		name("SSAOComponent"),
		passSSAO(nullptr),
		passApplySSAO(nullptr),
		cameraComponent(nullptr),
		activated(new Variant(SSAOComponent::AttrActivated(), true, this->attributes)),
		kernelRadius(new Variant(SSAOComponent::AttrKernelRadius(), 1.0f, this->attributes)),
		powerScale(new Variant(SSAOComponent::AttrPowerScale(), 1.5f, this->attributes))
	{
		this->activated->setDescription("If activated, shader will taking place.");
		this->kernelRadius->setConstraints(0.5f, 6.0f);
		this->powerScale->setConstraints(1.0f, 6.0f);
	}

	SSAOComponent::~SSAOComponent(void)
	{

	}

	void SSAOComponent::initialise()
	{

	}

	const Ogre::String& SSAOComponent::getName() const
	{
		return this->name;
	}

	void SSAOComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SSAOComponent>(SSAOComponent::getStaticClassId(), SSAOComponent::getStaticClassName());
	}

	void SSAOComponent::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void SSAOComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void SSAOComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool SSAOComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "KernelRadius")
		{
			this->kernelRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PowerScale")
		{
			this->powerScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr SSAOComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool SSAOComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SSAOComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->setActivated(this->activated->getBool());

		return true;
	}

	bool SSAOComponent::connect(void)
	{

		return true;
	}

	bool SSAOComponent::disconnect(void)
	{

		return true;
	}

	bool SSAOComponent::onCloned(void)
	{

		return true;
	}

	void SSAOComponent::onRemoveComponent(void)
	{

	}

	void SSAOComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		auto workspaceCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
		if (nullptr == cameraCompPtr)
		{
			this->cameraComponent = nullptr;
			this->setActivated(false);
		}
	}

	void SSAOComponent::onOtherComponentAdded(unsigned int index)
	{

	}

	void SSAOComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (true == this->activated->getBool())
			{
				Ogre::GpuProgramParametersSharedPtr psParams = this->passSSAO->getFragmentProgramParameters();
				Ogre::Camera* camera = this->cameraComponent->getCamera();
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
				// We don't render to render window directly, thus we need to get the projection
				// matrix with phone orientation disable when calculating SSAO
				camera->setOrientationMode(Ogre::OR_DEGREE_0);
#endif
				psParams->setNamedConstant("projection", camera->getProjectionMatrix());
				psParams->setNamedConstant("kernelRadius", this->kernelRadius->getReal());

				Ogre::GpuProgramParametersSharedPtr psParamsApply = this->passApplySSAO->getFragmentProgramParameters();
				psParamsApply->setNamedConstant("powerScale", this->powerScale->getReal());
			}
		}
	}

	void SSAOComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SSAOComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if(SSAOComponent::AttrKernelRadius() == attribute->getName())
		{
			this->setKernelRadius(attribute->getReal());
		}
		else if (SSAOComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
	}

	void SSAOComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "KernelRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->kernelRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PowerScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->powerScale->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String SSAOComponent::getClassName(void) const
	{
		return "SSAOComponent";
	}

	Ogre::String SSAOComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SSAOComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			this->createSSAOTexture();
		}
	}

	bool SSAOComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SSAOComponent::setKernelRadius(Ogre::Real kernelRadius)
	{
		this->kernelRadius->setValue(kernelRadius);
	}

	Ogre::Real SSAOComponent::getKernelRadius(void) const
	{
		return this->kernelRadius->getReal();
	}

	void SSAOComponent::setPowerScale(Ogre::Real powerScale)
	{
		this->powerScale->setValue(powerScale);
	}

	Ogre::Real SSAOComponent::getPowerScale(void) const
	{
		return this->powerScale->getReal();
	}

	// Lua registration part

	SSAOComponent* getSSAOComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SSAOComponent>(gameObject->getComponentWithOccurrence<SSAOComponent>(occurrenceIndex)).get();
	}

	SSAOComponent* getSSAOComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SSAOComponent>(gameObject->getComponent<SSAOComponent>()).get();
	}

	SSAOComponent* getSSAOComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SSAOComponent>(gameObject->getComponentFromName<SSAOComponent>(name)).get();
	}

	void SSAOComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		
	}

	void SSAOComponent::createSSAOTexture(void)
	{
		auto cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SSAOComponent] Cannot create ssao shader, because there is no camera component for game object: " + this->gameObjectPtr->getName());
			return;
		}
		this->cameraComponent = cameraCompPtr.get();

		//We need to create SSAO kernel samples and noise texture
		//Generate kernel samples first
		float kernelSamples[64][4];
		for (size_t i = 0; i < 64u; ++i)
		{
			Ogre::Vector3 sample = Ogre::Vector3(Ogre::Math::RangeRandom(-1.0f, 1.0f), Ogre::Math::RangeRandom(-1.0f, 1.0f), Ogre::Math::RangeRandom(0.0f, 1.0f));

			sample.normalise();

			float scale = (float)i / 64.0f;
			scale = Ogre::Math::lerp(0.3f, 1.0f, scale * scale);
			sample = sample * scale;

			kernelSamples[i][0] = sample.x;
			kernelSamples[i][1] = sample.y;
			kernelSamples[i][2] = sample.z;
			kernelSamples[i][3] = 1.0f;
		}

		//Next generate noise texture
		Ogre::Root* root = Core::getSingletonPtr()->getOgreRoot();
		Ogre::TextureGpuManager* textureManager = root->getRenderSystem()->getTextureGpuManager();

		Ogre::TextureGpu* noiseTexture = nullptr;

		if (false == textureManager->hasTextureResource("noiseTextureSSAO", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
		{
			noiseTexture = textureManager->createTexture("noiseTextureSSAO", Ogre::GpuPageOutStrategy::SaveToSystemRam, 0, Ogre::TextureTypes::Type2D);
			noiseTexture->setResolution(2u, 2u);
			noiseTexture->setPixelFormat(Ogre::PFG_RGBA8_SNORM);
			noiseTexture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
			noiseTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);

			Ogre::StagingTexture* stagingTexture = textureManager->getStagingTexture(2u, 2u, 1u, 1u, Ogre::PFG_RGBA8_SNORM);
			stagingTexture->startMapRegion();
			Ogre::TextureBox texBox = stagingTexture->mapRegion(2u, 2u, 1u, 1u, Ogre::PFG_RGBA8_SNORM);

			for (size_t j = 0; j < texBox.height; ++j)
			{
				for (size_t i = 0; i < texBox.width; ++i)
				{
					Ogre::Vector3 noise = Ogre::Vector3(Ogre::Math::RangeRandom(-1.0f, 1.0f), Ogre::Math::RangeRandom(-1.0f, 1.0f), 0.0f);
					noise.normalise();

					Ogre::uint8* pixelData = reinterpret_cast<Ogre::uint8*>(texBox.at(i, j, 0));
					pixelData[0] = Ogre::Bitwise::floatToSnorm8(noise.x);
					pixelData[1] = Ogre::Bitwise::floatToSnorm8(noise.y);
					pixelData[2] = Ogre::Bitwise::floatToSnorm8(noise.z);
					pixelData[3] = Ogre::Bitwise::floatToSnorm8(1.0f);
				}
			}

			stagingTexture->stopMapRegion();
			stagingTexture->upload(texBox, noiseTexture, 0, 0, 0);
			textureManager->removeStagingTexture(stagingTexture);
			stagingTexture = 0;
		}
		else
		{
			noiseTexture = textureManager->findTextureNoThrow("noiseTextureSSAO");
		}

		//---------------------------------------------------------------------------------
		//Get GpuProgramParametersSharedPtr to set uniforms that we need
		Ogre::MaterialPtr material = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/HS", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

		Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
		this->passSSAO = pass;
		Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

		//Lets set uniforms for shader
		//Set texture uniform for noise
		Ogre::TextureUnitState* noiseTextureState = pass->getTextureUnitState("noiseTextureSSAO");
		noiseTextureState->setTexture(noiseTexture);

		//Reconstruct position from depth. Position is needed in SSAO
		//We need to set the parameters based on camera to the
		//shader so that the un-projection works as expected
		Ogre::Vector2 projectionAB = this->cameraComponent->getCamera()->getProjectionParamsAB();
		//The division will keep "linearDepth" in the shader in the [0; 1] range.
		projectionAB.y /= this->cameraComponent->getFarClipDistance();
		psParams->setNamedConstant("projectionParams", projectionAB);

		//Set other uniforms
		psParams->setNamedConstant("kernelRadius", this->kernelRadius->getReal());

		psParams->setNamedConstant("noiseScale", Ogre::Vector2((Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() * 0.5f) / 2.0f,
								   (Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() * 0.5f) / 2.0f));
		psParams->setNamedConstant("invKernelSize", 1.0f / 64.0f);
		psParams->setNamedConstant("sampleDirs", (float*)kernelSamples, 64, 4);

		//Set blur shader uniforms
		Ogre::MaterialPtr materialBlurH = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/BlurH", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

		Ogre::Pass* passBlurH = materialBlurH->getTechnique(0)->getPass(0);
		Ogre::GpuProgramParametersSharedPtr psParamsBlurH = passBlurH->getFragmentProgramParameters();
		psParamsBlurH->setNamedConstant("projectionParams", projectionAB);

		Ogre::MaterialPtr materialBlurV = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/BlurV", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

		Ogre::Pass* passBlurV = materialBlurV->getTechnique(0)->getPass(0);
		Ogre::GpuProgramParametersSharedPtr psParamsBlurV = passBlurV->getFragmentProgramParameters();
		psParamsBlurV->setNamedConstant("projectionParams", projectionAB);

		//Set apply shader uniforms
		Ogre::MaterialPtr materialApply = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/Apply", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

		Ogre::Pass* passApply = materialApply->getTechnique(0)->getPass(0);
		this->passApplySSAO = passApply;
		Ogre::GpuProgramParametersSharedPtr psParamsApply = passApply->getFragmentProgramParameters();
		psParamsApply->setNamedConstant("powerScale", this->powerScale->getReal());
	}

	bool SSAOComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto ssaoCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<SSAOComponent>());
		auto cameraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
		// Constraints: Can only be placed under a camera object and only once
		if (nullptr != cameraCompPtr && nullptr == ssaoCompPtr)
		{
			return true;
		}
		return false;
	}

}; //namespace end