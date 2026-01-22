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
		: CompositorEffectBaseComponent(),
		name("SSAOComponent"),
		passSSAO(nullptr),
		passApplySSAO(nullptr),
		cameraComponent(nullptr),
		passBlurH(nullptr),
		passBlurV(nullptr),
		kernelRadius(new Variant(SSAOComponent::AttrKernelRadius(), 1.0f, this->attributes)),
		powerScale(new Variant(SSAOComponent::AttrPowerScale(), 3.0f, this->attributes))
	{
		this->kernelRadius->setConstraints(0.5f, 6.0f);
		this->powerScale->setConstraints(1.0f, 6.0f);

		this->effectName = "SSAO";
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

	bool SSAOComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

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

		return success;
	}

	GameObjectCompPtr SSAOComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool SSAOComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SSAOComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->initializeSSAOShaders();

		auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
		if (nullptr != workspaceBaseCompPtr)
		{
			this->workspaceBaseComponent = workspaceBaseCompPtr.get();
			if (false == this->workspaceBaseComponent->getUseSSAO())
			{
				this->workspaceBaseComponent->setUseSSAO(true);
				this->workspaceBaseComponent->setUseDepthTexture(true);
			}
		}

		return true;
	}

	bool SSAOComponent::connect(void)
	{
		GameObjectComponent::connect();

		return true;
	}

	bool SSAOComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		return true;
	}

	void SSAOComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
	}

	void SSAOComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			// Only update if we have valid shader passes
			if (nullptr == this->passSSAO || nullptr == this->passApplySSAO)
			{
				return;
			}

			auto closureFunction = [this](Ogre::Real renderDt)
				{
					// Update kernel radius
					Ogre::GpuProgramParametersSharedPtr psParams = this->passSSAO->getFragmentProgramParameters();
					psParams->setNamedConstant("kernelRadius", this->kernelRadius->getReal());

					// Update power scale
					Ogre::GpuProgramParametersSharedPtr psParamsApply = this->passApplySSAO->getFragmentProgramParameters();
					psParamsApply->setNamedConstant("powerScale", this->powerScale->getReal());
				};

			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
		}
	}

	void SSAOComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if(SSAOComponent::AttrKernelRadius() == attribute->getName())
		{
			this->setKernelRadius(attribute->getReal());
		}
		else if (SSAOComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
	}

	void SSAOComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "KernelRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->kernelRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PowerScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->powerScale->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String SSAOComponent::getClassName(void) const
	{
		return "SSAOComponent";
	}

	Ogre::String SSAOComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
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

	void SSAOComponent::initializeSSAOShaders(void)
	{
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
			{
				// Get the SSAO material passes - these should already be loaded by the workspace
				Ogre::MaterialPtr material = std::static_pointer_cast<Ogre::Material>(
					Ogre::MaterialManager::getSingleton().getByName("SSAO/HS", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

				if (material.isNull())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SSAOComponent] SSAO/HS material not found!");
					return;
				}

				this->passSSAO = material->getTechnique(0)->getPass(0);

				// Get blur passes
				Ogre::MaterialPtr materialBlurH = std::static_pointer_cast<Ogre::Material>(
					Ogre::MaterialManager::getSingleton().getByName("SSAO/BlurH", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));
				if (!materialBlurH.isNull())
				{
					this->passBlurH = materialBlurH->getTechnique(0)->getPass(0);
				}

				Ogre::MaterialPtr materialBlurV = std::static_pointer_cast<Ogre::Material>(
					Ogre::MaterialManager::getSingleton().getByName("SSAO/BlurV", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));
				if (!materialBlurV.isNull())
				{
					this->passBlurV = materialBlurV->getTechnique(0)->getPass(0);
				}

				// Get apply pass
				Ogre::MaterialPtr materialApply = std::static_pointer_cast<Ogre::Material>(
					Ogre::MaterialManager::getSingleton().getByName("SSAO/Apply", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

				if (materialApply.isNull())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SSAOComponent] SSAO/Apply material not found!");
					return;
				}

				this->passApplySSAO = materialApply->getTechnique(0)->getPass(0);

				// Set initial parameter values
				if (this->passSSAO)
				{
					Ogre::GpuProgramParametersSharedPtr psParams = this->passSSAO->getFragmentProgramParameters();
					psParams->setNamedConstant("kernelRadius", this->kernelRadius->getReal());
				}

				if (this->passApplySSAO)
				{
					Ogre::GpuProgramParametersSharedPtr psParamsApply = this->passApplySSAO->getFragmentProgramParameters();
					psParamsApply->setNamedConstant("powerScale", this->powerScale->getReal());
				}

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SSAOComponent] SSAO shaders initialized successfully");
			};

		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SSAOComponent::initializeSSAOShaders");
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