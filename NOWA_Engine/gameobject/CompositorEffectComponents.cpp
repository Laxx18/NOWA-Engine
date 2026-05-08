#include "NOWAPrecompiled.h"
#include "CompositorEffectComponents.h"
#include "CameraComponent.h"
#include "LightDirectionalComponent.h"
#include "utilities/XMLConverter.h"
#include "WorkspaceComponents.h"
#include "main/Events.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	CompositorEffectBaseComponent::CompositorEffectBaseComponent()
		: GameObjectComponent(),
		effectName("None"),
		workspaceBaseComponent(nullptr),
        camera(nullptr),
		activated(new Variant(CompositorEffectBaseComponent::AttrActivated(), true, this->attributes)),
		workspaceGameObjectId(new Variant(CompositorEffectBaseComponent::AttrWorkspaceGameObjectId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->activated->setDescription("Activates the effect.");
		// "Bloom, Glass, Old TV, B&W, Embossed, Sharpen Edges, Invert, Posterize, Laplace, Tiling, Old Movie, Radial Blur, ASCII, Halftone, Night Vision, Dither"
	}

	CompositorEffectBaseComponent::~CompositorEffectBaseComponent()
	{

	}

	bool CompositorEffectBaseComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &CompositorEffectBaseComponent::handleWorkspaceComponentDeleted), EventDataDeleteWorkspaceComponent::getStaticEventType());

		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->setActivated(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WorkspaceGameObjectId")
		{
			this->workspaceGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool CompositorEffectBaseComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectBaseComponent] Init compositor effect base component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	void CompositorEffectBaseComponent::onRemoveComponent(void)
	{
        GameObjectComponent::onRemoveComponent();

		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &CompositorEffectBaseComponent::handleWorkspaceComponentDeleted), EventDataDeleteWorkspaceComponent::getStaticEventType());
	}

	bool CompositorEffectBaseComponent::connect(void)
	{
        GameObjectComponent::connect();

		return true;
	}

	bool CompositorEffectBaseComponent::disconnect(void)
	{
        GameObjectComponent::disconnect();

		this->camera = nullptr;

		return true;
	}

	void CompositorEffectBaseComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void CompositorEffectBaseComponent::actualizeValue(Variant* attribute)
	{
		if (CompositorEffectBaseComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (CompositorEffectBaseComponent::AttrWorkspaceGameObjectId() == attribute->getName())
		{
			this->setWorkspaceGameObjectId(attribute->getULong());
		}
	}

	void CompositorEffectBaseComponent::handleWorkspaceComponentDeleted(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteWorkspaceComponent> castEventData = boost::static_pointer_cast<EventDataDeleteWorkspaceComponent>(eventData);

		GameProgressModule* gameProgressModule = AppStateManager::getSingletonPtr()->getActiveGameProgressModuleSafe();
        const bool isStalled = AppStateManager::getSingletonPtr()->bStall.load();
        const bool isSceneLoading = (gameProgressModule != nullptr) ? gameProgressModule->bSceneLoading.load() : true;

        if (false == isStalled && false == isSceneLoading)
        {
            // Found the game object
            if (this->workspaceGameObjectId->getULong() == castEventData->getGameObjectId())
            {
                this->workspaceBaseComponent = nullptr;
            }
        }
	}

	void CompositorEffectBaseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Component" + this->getClassName())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getClassName())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WorkspaceGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->workspaceGameObjectId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	void CompositorEffectBaseComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	Ogre::String CompositorEffectBaseComponent::getClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	Ogre::String CompositorEffectBaseComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool CompositorEffectBaseComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void CompositorEffectBaseComponent::setWorkspaceGameObjectId(unsigned long workspaceGameObjectId)
	{
		this->workspaceGameObjectId->setValue(workspaceGameObjectId);
	}

	unsigned long CompositorEffectBaseComponent::getWorkspaceGameObjectId(void) const
	{
		return this->workspaceGameObjectId->getULong();
	}

	void CompositorEffectBaseComponent::enableEffect(const Ogre::String& effectName, bool enabled)
	{
		GameObjectPtr workspaceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->workspaceGameObjectId->getULong());
		if (nullptr == workspaceGameObjectPtr)
		{
			Ogre::String message = "[CompositorEffectBaseComponent] Could not get game object with workspace component Affected game object: "
				+ this->gameObjectPtr->getName();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

			boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
			return;
		}

		auto cameraCompPtr = NOWA::makeStrongPtr(workspaceGameObjectPtr->getComponent<CameraComponent>());
		if (nullptr != cameraCompPtr)
		{
			// If camera is not activated, this compositor effect should not be played
			if (false == cameraCompPtr->isActivated() && false == WorkspaceModule::getInstance()->getSplitScreenScenarioActive())
			{
				return;
			}
            this->camera = cameraCompPtr->getCamera();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBaseComponent] Could not get prior CameraComponent! Affected game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto workspaceBaseCompPtr = NOWA::makeStrongPtr(workspaceGameObjectPtr->getComponent<WorkspaceBaseComponent>());
		if (nullptr != workspaceBaseCompPtr)
		{
			this->workspaceBaseComponent = workspaceBaseCompPtr.get();
		}
		else
		{
			Ogre::String message = "[CompositorEffectBaseComponent] Could not get a workspace base component! Affected game object: "
				+ this->gameObjectPtr->getName();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

			boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
			return;
		}

		if ("none" == effectName || nullptr == this->workspaceBaseComponent || nullptr == this->workspaceBaseComponent->getWorkspace())
		{
			return;
		}

		// Disable/Enable the node (it was already instantiated in setupCompositor())
		Ogre::CompositorNode* node = this->workspaceBaseComponent->getWorkspace()->findNode(effectName);
		if (nullptr != node)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectBaseComponent::enableEffect", _2(node, enabled),
			{
				node->setEnabled(enabled);
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,"[WorkspaceModule] Could not find effect: " + effectName);
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[WorkspaceModule] Could not find effect: " + effectName, "NOWA");
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectBloomComponent::CompositorEffectBloomComponent()
		: CompositorEffectBaseComponent(),
		imageWeight(new Variant(CompositorEffectBloomComponent::AttrImageWeight(), 1.0f, this->attributes)),
		blurWeight(new Variant(CompositorEffectBloomComponent::AttrBlurWeight(), 0.8f, this->attributes))
	{
		this->effectName = "Bloom";
		this->passes = { {nullptr, nullptr, nullptr, nullptr} };
	}

	CompositorEffectBloomComponent::~CompositorEffectBloomComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectBloomComponent] Destructor compositor bloom effect component for game object: " + this->gameObjectPtr->getName());
		this->passes = { {nullptr, nullptr, nullptr, nullptr} };
	}

	bool CompositorEffectBloomComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ImageWeight")
		{
			this->setImageWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlurWeight")
		{
			this->setBlurWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr CompositorEffectBloomComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectBloomComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectBloomComponent] Init compositor effect bloom component for game object: " + this->gameObjectPtr->getName());

		// TODO: Wait queue?

		Ogre::String materialName0 = "Postprocess/BrightPass2";
		Ogre::String materialName1 = "Postprocess/BlurV";
		Ogre::String materialName2 = "Postprocess/BlurH";
		Ogre::String materialName3 = "Postprocess/BloomBlend2";

		this->materials[0] = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->materials[0].isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBloomComponent] Could not set: " + this->effectName 
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		this->materials[1] = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName1);
		if (true == this->materials[1].isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBloomComponent] Could not set: " + this->effectName 
				+ " because the material: '" + materialName1 + "' does not exist!");
			return false;
		}

		this->materials[2] = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName2);
		if (true == this->materials[2].isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBloomComponent] Could not set: " + this->effectName 
				+ " because the material: '" + materialName2 + "' does not exist!");
			return false;
		}

		this->materials[3] = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName3);
		if (true == this->materials[3].isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBloomComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName3 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->materials[0].getPointer();
		this->passes[0] = material0->getTechnique(0)->getPass(0);

		Ogre::Material* material1 = this->materials[1].getPointer();
		this->passes[1] = material1->getTechnique(0)->getPass(0);

		Ogre::Material* material2 = this->materials[2].getPointer();
		this->passes[2] = material2->getTechnique(0)->getPass(0);

		Ogre::Material* material3 = this->materials[3].getPointer();
		this->passes[3] = material3->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setImageWeight(this->imageWeight->getReal());
		this->setBlurWeight(this->blurWeight->getReal());
		
		return success;
	}

	void CompositorEffectBloomComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectBloomComponent::AttrImageWeight() == attribute->getName())
		{
			this->setImageWeight(attribute->getReal());
		}
		else if (CompositorEffectBloomComponent::AttrBlurWeight() == attribute->getName())
		{
			this->setBlurWeight(attribute->getReal());
		}
	}

	void CompositorEffectBloomComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "ImageWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->imageWeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlurWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blurWeight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectBloomComponent::getClassName(void) const
	{
		return "CompositorEffectBloomComponent";
	}

	Ogre::String CompositorEffectBloomComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectBloomComponent::setImageWeight(Ogre::Real imageWeight)
	{
		this->imageWeight->setValue(imageWeight);

		if (nullptr != this->passes[3])
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectBloomComponent::setImageWeight", _1(imageWeight),
			{
				this->passes[3]->getFragmentProgramParameters()->setNamedConstant("OriginalImageWeight", imageWeight);
			});
		}
	}

	Ogre::Real CompositorEffectBloomComponent::getImageWeight(void) const
	{
		return this->imageWeight->getReal();
	}

	void CompositorEffectBloomComponent::setBlurWeight(Ogre::Real blurWeight)
	{
		this->blurWeight->setValue(blurWeight);

		if (nullptr != this->passes[3])
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectBloomComponent::setBlurWeight", _1(blurWeight),
			{
				this->passes[3]->getFragmentProgramParameters()->setNamedConstant("BlurWeight", blurWeight);
			});
		}
	}

	Ogre::Real CompositorEffectBloomComponent::getBlurWeight(void) const
	{
		return this->blurWeight->getReal();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectGlassComponent::CompositorEffectGlassComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		glassWeight(new Variant(CompositorEffectGlassComponent::AttrGlassWeight(), 2.5f, this->attributes))
	{
		this->effectName = "Glass";
	}

	CompositorEffectGlassComponent::~CompositorEffectGlassComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectGlassComponent] Destructor compositor glass effect component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectGlassComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GlassWeight")
		{
			this->setGlassWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectGlassComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectGlassComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectGlassComponent] Init compositor effect glass component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/Glass";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectGlassComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setGlassWeight(this->glassWeight->getReal());

		return success;
	}

	void CompositorEffectGlassComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectGlassComponent::AttrGlassWeight() == attribute->getName())
		{
			this->setGlassWeight(attribute->getReal());
		}
	}

	void CompositorEffectGlassComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "GlassWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->glassWeight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectGlassComponent::getClassName(void) const
	{
		return "CompositorEffectGlassComponent";
	}

	Ogre::String CompositorEffectGlassComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectGlassComponent::setGlassWeight(Ogre::Real glassWeight)
	{
		this->glassWeight->setValue(glassWeight);

		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectGlassComponent::setGlassWeight", _1(glassWeight),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("GlassWeight", glassWeight);
			});
		}
	}

	Ogre::Real CompositorEffectGlassComponent::getGlassWeight(void) const
	{
		return this->glassWeight->getReal();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectOldTvComponent::CompositorEffectOldTvComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		distortionFrequency(new Variant(CompositorEffectOldTvComponent::AttrDistortionFrequency(), 2.7f, this->attributes)),
		distortionScale(new Variant(CompositorEffectOldTvComponent::AttrDistortionScale(), 2.5f, this->attributes)),
		distortionRoll(new Variant(CompositorEffectOldTvComponent::AttrDistortionRoll(), 0.93f, this->attributes)),
		interference(new Variant(CompositorEffectOldTvComponent::AttrInterference(), 0.5f, this->attributes)),
		frameLimit(new Variant(CompositorEffectOldTvComponent::AttrFrameLimit(), 0.4f, this->attributes)),
		frameShape(new Variant(CompositorEffectOldTvComponent::AttrFrameShape(), 0.26f, this->attributes)),
		frameSharpness(new Variant(CompositorEffectOldTvComponent::AttrFrameSharpness(), 6.0f, this->attributes)),
		time(new Variant(CompositorEffectOldTvComponent::AttrTime(), static_cast<unsigned int>(120), this->attributes)),
		sinusTime(new Variant(CompositorEffectOldTvComponent::AttrSinusTime(), static_cast<unsigned int>(120), this->attributes))
	{
		this->effectName = "Old TV";
	}

	CompositorEffectOldTvComponent::~CompositorEffectOldTvComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectOldTvComponent] Destructor compositor old tv effect component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectOldTvComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DistortionFrequency")
		{
			this->setDistortionFrequency(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DistortionScale")
		{
			this->setDistortionScale(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DistortionRoll")
		{
			this->setDistortionRoll(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Interference")
		{
			this->setInterference(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FrameLimit")
		{
			this->setFrameLimit(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FrameShape")
		{
			this->setFrameShape(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FrameSharpness")
		{
			this->setFrameSharpness(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Time")
		{
			this->setTime(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SinusTime")
		{
			this->setSinusTime(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectOldTvComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectOldTvComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectOldTvComponent] Init compositor effect old tv component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/OldTV";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectOldTvComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setDistortionFrequency(this->distortionFrequency->getReal());
		this->setDistortionScale(this->distortionScale->getReal());
		this->setDistortionRoll(this->distortionRoll->getReal());
		this->setInterference(this->interference->getReal());
		this->setFrameLimit(this->frameLimit->getReal());
		this->setFrameShape(this->frameShape->getReal());
		this->setFrameSharpness(this->frameSharpness->getReal());
		this->setTime(this->time->getUInt());
		this->setSinusTime(this->sinusTime->getUInt());

		return success;
	}

	void CompositorEffectOldTvComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectOldTvComponent::AttrDistortionFrequency() == attribute->getName())
		{
			this->setDistortionFrequency(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrDistortionScale() == attribute->getName())
		{
			this->setDistortionScale(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrDistortionRoll() == attribute->getName())
		{
			this->setDistortionRoll(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrInterference() == attribute->getName())
		{
			this->setInterference(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrFrameLimit() == attribute->getName())
		{
			this->setFrameLimit(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrFrameShape() == attribute->getName())
		{
			this->setFrameShape(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrFrameSharpness() == attribute->getName())
		{
			this->setFrameSharpness(attribute->getReal());
		}
		else if (CompositorEffectOldTvComponent::AttrTime() == attribute->getName())
		{
			this->setTime(attribute->getUInt());
		}
		else if (CompositorEffectOldTvComponent::AttrSinusTime() == attribute->getName())
		{
			this->setSinusTime(attribute->getUInt());
		}
	}

	void CompositorEffectOldTvComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "DistortionFrequency"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->distortionFrequency->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DistortionScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->distortionScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DistortionRoll"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->distortionRoll->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Interference"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->interference->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->frameLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameShape"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->frameShape->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameSharpness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->frameSharpness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Time"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->time->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SinusTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sinusTime->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectOldTvComponent::getClassName(void) const
	{
		return "CompositorEffectOldTvComponent";
	}

	Ogre::String CompositorEffectOldTvComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectOldTvComponent::setDistortionFrequency(Ogre::Real distortionFrequency)
	{
		this->distortionFrequency->setValue(distortionFrequency);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setDistortionFrequency", _1(distortionFrequency),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("distortionFreq", distortionFrequency);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getDistortionFrequency(void) const
	{
		return this->distortionFrequency->getReal();
	}

	void CompositorEffectOldTvComponent::setDistortionScale(Ogre::Real distortionScale)
	{
		this->distortionScale->setValue(distortionScale);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setDistortionScale", _1(distortionScale),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("distortionScale", distortionScale);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getDistortionScale(void) const
	{
		return this->distortionScale->getReal();
	}

	void CompositorEffectOldTvComponent::setDistortionRoll(Ogre::Real distortionRoll)
	{
		this->distortionRoll->setValue(distortionRoll);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setDistortionRoll", _1(distortionRoll),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("distortionRoll", distortionRoll);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getDistortionRoll(void) const
	{
		return this->distortionRoll->getReal();
	}

	void CompositorEffectOldTvComponent::setInterference(Ogre::Real interference)
	{
		this->interference->setValue(interference);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setInterference", _1(interference),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("interference", interference);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getInterference(void) const
	{
		return this->interference->getReal();
	}

	void CompositorEffectOldTvComponent::setFrameLimit(Ogre::Real frameLimit)
	{
		this->frameLimit->setValue(frameLimit);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setFrameLimit", _1(frameLimit),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("frameLimit", frameLimit);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getFrameLimit(void) const
	{
		return this->frameLimit->getReal();
	}

	void CompositorEffectOldTvComponent::setFrameShape(Ogre::Real frameShape)
	{
		this->frameShape->setValue(frameShape);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setFrameShape", _1(frameShape),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("frameShape", frameShape);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getFrameShape(void) const
	{
		return this->frameShape->getReal();
	}

	void CompositorEffectOldTvComponent::setFrameSharpness(Ogre::Real frameSharpness)
	{
		this->frameSharpness->setValue(frameSharpness);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setFrameSharpness", _1(frameSharpness),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("frameSharpness", frameSharpness);
			});
		}
	}

	Ogre::Real CompositorEffectOldTvComponent::getFrameSharpness(void) const
	{
		return this->frameSharpness->getReal();
	}

	void CompositorEffectOldTvComponent::setTime(unsigned int time)
	{
		this->time->setValue(time);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setTime", _1(time),
			{
				// this->pass->getFragmentProgramParameters()->setNamedConstant("time_0_X", time);
				this->pass->getFragmentProgramParameters()->setNamedConstantFromTime("time_0_X", static_cast<Ogre::Real>(time));
			});
		}
	}

	unsigned int CompositorEffectOldTvComponent::getTime(void) const
	{
		return this->time->getUInt();
	}

	void CompositorEffectOldTvComponent::setSinusTime(unsigned int sinusTime)
	{
		this->sinusTime->setValue(sinusTime);
		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOldTvComponent::setSinusTime", _1(sinusTime),
			{
				// this->pass->getFragmentProgramParameters()->setNamedConstant("sin_time_0_X", sinusTime);
				this->pass->getFragmentProgramParameters()->setNamedConstantFromTime("sin_time_0_X", static_cast<Ogre::Real>(sinusTime));
			});
		}
	}

	unsigned int CompositorEffectOldTvComponent::getSinusTime(void) const
	{
		return this->sinusTime->getUInt();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectBlackAndWhiteComponent::CompositorEffectBlackAndWhiteComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		color(new Variant(CompositorEffectBlackAndWhiteComponent::AttrColor(), Ogre::Vector3(0.3f, 0.59f, 0.11f), this->attributes))
	{
		this->effectName = "B&W";
		this->color->addUserData(GameObject::AttrActionColorDialog());
	}

	CompositorEffectBlackAndWhiteComponent::~CompositorEffectBlackAndWhiteComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectBlackAndWhiteComponent] Destructor compositor black and white effect component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectBlackAndWhiteComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->setColor(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectBlackAndWhiteComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectBlackAndWhiteComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectBlackAndWhiteComponent] Init compositor effect black and white component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/BlackAndWhite";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBlackAndWhiteComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setColor(this->color->getVector3());

		return success;
	}

	void CompositorEffectBlackAndWhiteComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectBlackAndWhiteComponent::AttrColor() == attribute->getName())
		{
			this->setColor(attribute->getVector3());
		}
	}

	void CompositorEffectBlackAndWhiteComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->color->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectBlackAndWhiteComponent::getClassName(void) const
	{
		return "CompositorEffectBlackAndWhiteComponent";
	}

	Ogre::String CompositorEffectBlackAndWhiteComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectBlackAndWhiteComponent::setColor(const Ogre::Vector3& color)
	{
		this->color->setValue(color);

		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectBlackAndWhiteComponent::setColor", _1(color),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("color", color);
			});
		}
	}

	Ogre::Vector3 CompositorEffectBlackAndWhiteComponent::getColor(void) const
	{
		return this->color->getVector3();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectColorComponent::CompositorEffectColorComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		color(new Variant(CompositorEffectColorComponent::AttrColor(), Ogre::Vector3(0.3f, 0.59f, 0.11f), this->attributes))
	{
		this->effectName = "Color";
		this->color->addUserData(GameObject::AttrActionColorDialog());
	}

	CompositorEffectColorComponent::~CompositorEffectColorComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectColorComponent] Destructor compositor color effect component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectColorComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->setColor(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectColorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectColorComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectColorComponent] Init compositor effect color component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/Color";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectColorComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setColor(this->color->getVector3());

		return success;
	}

	void CompositorEffectColorComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectColorComponent::AttrColor() == attribute->getName())
		{
			this->setColor(attribute->getVector3());
		}
	}

	void CompositorEffectColorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->color->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectColorComponent::getClassName(void) const
	{
		return "CompositorEffectColorComponent";
	}

	Ogre::String CompositorEffectColorComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectColorComponent::setColor(const Ogre::Vector3& color)
	{
		this->color->setValue(color);

		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectColorComponent::setColor", _1(color),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("color", color);
			});
		}
	}

	Ogre::Vector3 CompositorEffectColorComponent::getColor(void) const
	{
		return this->color->getVector3();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectEmbossedComponent::CompositorEffectEmbossedComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		weight(new Variant(CompositorEffectEmbossedComponent::AttrWeight(), 0.3f, this->attributes))
	{
		this->effectName = "Embossed";
	}

	CompositorEffectEmbossedComponent::~CompositorEffectEmbossedComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectEmbossedComponent] Destructor compositor effect embossed component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectEmbossedComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Weight")
		{
			this->setWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectEmbossedComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectEmbossedComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectEmbossedComponent] Init compositor effect embossed component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/Embossed";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectEmbossedComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setWeight(this->weight->getReal());

		return success;
	}

	void CompositorEffectEmbossedComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectEmbossedComponent::AttrWeight() == attribute->getName())
		{
			this->setWeight(attribute->getReal());
		}
	}

	void CompositorEffectEmbossedComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Weight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->weight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectEmbossedComponent::getClassName(void) const
	{
		return "CompositorEffectEmbossedComponent";
	}

	Ogre::String CompositorEffectEmbossedComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectEmbossedComponent::setWeight(const Ogre::Real& weight)
	{
		this->weight->setValue(weight);

		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectEmbossedComponent::setWeight", _1(weight),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("weight", weight);
			});
		}
	}

	Ogre::Real CompositorEffectEmbossedComponent::getWeight(void) const
	{
		return this->weight->getReal();
	}

	// https://forums.ogre3d.org/viewtopic.php?t=34851

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectSharpenEdgesComponent::CompositorEffectSharpenEdgesComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		weight(new Variant(CompositorEffectSharpenEdgesComponent::AttrWeight(), 9.0f, this->attributes))
	{
		this->effectName = "Sharpen Edges";
	}

	CompositorEffectSharpenEdgesComponent::~CompositorEffectSharpenEdgesComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectSharpenEdgesComponent] Destructor compositor effect sharpen edges component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectSharpenEdgesComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Weight")
		{
			this->setWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectSharpenEdgesComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
        return nullptr;
	}

	bool CompositorEffectSharpenEdgesComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectSharpenEdgesComponent] Init compositor effect sharpen edges component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/SharpenEdges";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectSharpenEdgesComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setWeight(this->weight->getReal());

		return success;
	}

	void CompositorEffectSharpenEdgesComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectSharpenEdgesComponent::AttrWeight() == attribute->getName())
		{
			this->setWeight(attribute->getReal());
		}
	}

	void CompositorEffectSharpenEdgesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Weight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->weight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectSharpenEdgesComponent::getClassName(void) const
	{
		return "CompositorEffectSharpenEdgesComponent";
	}

	Ogre::String CompositorEffectSharpenEdgesComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectSharpenEdgesComponent::setWeight(const Ogre::Real& weight)
	{
		this->weight->setValue(weight);

		if (nullptr != this->pass)
		{
			// TODO: Wait?
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectSharpenEdgesComponent::setWeight", _1(weight),
			{
				this->pass->getFragmentProgramParameters()->setNamedConstant("weight", weight);
			});
		}
	}

	Ogre::Real CompositorEffectSharpenEdgesComponent::getWeight(void) const
	{
		return this->weight->getReal();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectCartoonComponent::CompositorEffectCartoonComponent()
		: CompositorEffectBaseComponent(),
        edgePass(nullptr),
        colorPass(nullptr),
        edgeThreshold(new Variant(CompositorEffectCartoonComponent::AttrEdgeThreshold(), 0.8f, this->attributes)), 
        edgeStrength(new Variant(CompositorEffectCartoonComponent::AttrEdgeStrength(), 2.0f, this->attributes)),  
        numBands(new Variant(CompositorEffectCartoonComponent::AttrNumBands(), 10.0f, this->attributes)),  
        saturation(new Variant(CompositorEffectCartoonComponent::AttrSaturation(), 1.2f, this->attributes)),
        edgeDarkness(new Variant(CompositorEffectCartoonComponent::AttrEdgeDarkness(), 0.2f, this->attributes))
    {
        this->effectName = "Cartoon";

        // Describe reasonable value ranges for the editor
        this->edgeThreshold->setDescription("Controls the edge detection sensitivity. Lower values detect more edges.");
        this->edgeStrength->setDescription("Controls the intensity and visibility of the cartoon outlines.");
        this->numBands->setDescription("Controls the number of color shading bands used for the cartoon effect.");
        this->saturation->setDescription("Controls the color saturation of the final cartoon image.");
        this->edgeDarkness->setDescription("Controls how dark the detected outline edges appear.");
    }

    CompositorEffectCartoonComponent::~CompositorEffectCartoonComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectCartoonComponent] Destructor compositor effect cartoon component for game object: " + this->gameObjectPtr->getName());

        this->edgePass = nullptr;
        this->colorPass = nullptr;
    }

    bool CompositorEffectCartoonComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = CompositorEffectBaseComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEdgeThreshold())
        {
            this->setEdgeThreshold(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEdgeStrength())
        {
            this->setEdgeStrength(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrNumBands())
        {
            this->setNumBands(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSaturation())
        {
            this->setSaturation(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEdgeDarkness())
        {
            this->setEdgeDarkness(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr CompositorEffectCartoonComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool CompositorEffectCartoonComponent::postInit(void)
    {
        bool success = CompositorEffectBaseComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectCartoonComponent] Init compositor effect cartoon component for game object: " + this->gameObjectPtr->getName());

        // --- Pass 1: CartoonEdge ---
        const Ogre::String edgeMaterialName = "Postprocess/CartoonEdge";
        this->edgeMaterial = Ogre::MaterialManager::getSingletonPtr()->getByName(edgeMaterialName);

        if (true == this->edgeMaterial.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectCartoonComponent] Could not set: " + this->effectName + " because the material: '" + edgeMaterialName + "' does not exist!");
            return false;
        }

        this->edgePass = this->edgeMaterial->getTechnique(0)->getPass(0);

        // --- Pass 2: CartoonColor ---
        const Ogre::String colorMaterialName = "Postprocess/CartoonColor";
        this->colorMaterial = Ogre::MaterialManager::getSingletonPtr()->getByName(colorMaterialName);

        if (true == this->colorMaterial.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectCartoonComponent] Could not set: " + this->effectName + " because the material: '" + colorMaterialName + "' does not exist!");
            return false;
        }

        this->colorPass = this->colorMaterial->getTechnique(0)->getPass(0);

        // Push all loaded values to the GPU so the compositor starts with the
        // correct state even before the user changes anything.
        this->setEdgeThreshold(this->edgeThreshold->getReal());
        this->setEdgeStrength(this->edgeStrength->getReal());
        this->setNumBands(this->numBands->getReal());
        this->setSaturation(this->saturation->getReal());
        this->setEdgeDarkness(this->edgeDarkness->getReal());

        return success;
    }

    void CompositorEffectCartoonComponent::actualizeValue(Variant* attribute)
    {
        CompositorEffectBaseComponent::actualizeValue(attribute);

        if (CompositorEffectCartoonComponent::AttrEdgeThreshold() == attribute->getName())
        {
            this->setEdgeThreshold(attribute->getReal());
        }
        else if (CompositorEffectCartoonComponent::AttrEdgeStrength() == attribute->getName())
        {
            this->setEdgeStrength(attribute->getReal());
        }
        else if (CompositorEffectCartoonComponent::AttrNumBands() == attribute->getName())
        {
            this->setNumBands(attribute->getReal());
        }
        else if (CompositorEffectCartoonComponent::AttrSaturation() == attribute->getName())
        {
            this->setSaturation(attribute->getReal());
        }
        else if (CompositorEffectCartoonComponent::AttrEdgeDarkness() == attribute->getName())
        {
            this->setEdgeDarkness(attribute->getReal());
        }
    }

    void CompositorEffectCartoonComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // Type codes: 2=int, 6=real, 7=string, 8=vector2, 9=vector3,
        //             10=vector4/quaternion, 12=bool
        CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Edge Threshold"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->edgeThreshold->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Edge Strength"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->edgeStrength->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Num Bands"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numBands->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Saturation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->saturation->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Edge Darkness"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->edgeDarkness->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String CompositorEffectCartoonComponent::getClassName(void) const
    {
        return "CompositorEffectCartoonComponent";
    }

    Ogre::String CompositorEffectCartoonComponent::getParentClassName(void) const
    {
        return "CompositorEffectBaseComponent";
    }

    // -----------------------------------------------------------------------
    // Setters / getters — CartoonEdge pass
    // -----------------------------------------------------------------------

    void CompositorEffectCartoonComponent::setEdgeThreshold(Ogre::Real edgeThreshold)
    {
        this->edgeThreshold->setValue(edgeThreshold);

        if (nullptr != this->edgePass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectCartoonComponent::setEdgeThreshold", _1(edgeThreshold), { this->edgePass->getFragmentProgramParameters()->setNamedConstant("edgeThreshold", edgeThreshold); });
        }
    }

    Ogre::Real CompositorEffectCartoonComponent::getEdgeThreshold(void) const
    {
        return this->edgeThreshold->getReal();
    }

    void CompositorEffectCartoonComponent::setEdgeStrength(Ogre::Real edgeStrength)
    {
        this->edgeStrength->setValue(edgeStrength);

        if (nullptr != this->edgePass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectCartoonComponent::setEdgeStrength", _1(edgeStrength), { this->edgePass->getFragmentProgramParameters()->setNamedConstant("edgeStrength", edgeStrength); });
        }
    }

    Ogre::Real CompositorEffectCartoonComponent::getEdgeStrength(void) const
    {
        return this->edgeStrength->getReal();
    }

    // -----------------------------------------------------------------------
    // Setters / getters — CartoonColor pass
    // -----------------------------------------------------------------------

    void CompositorEffectCartoonComponent::setNumBands(Ogre::Real numBands)
    {
        this->numBands->setValue(numBands);

        if (nullptr != this->colorPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectCartoonComponent::setNumBands", _1(numBands), { this->colorPass->getFragmentProgramParameters()->setNamedConstant("numBands", numBands); });
        }
    }

    Ogre::Real CompositorEffectCartoonComponent::getNumBands(void) const
    {
        return this->numBands->getReal();
    }

    void CompositorEffectCartoonComponent::setSaturation(Ogre::Real saturation)
    {
        this->saturation->setValue(saturation);

        if (nullptr != this->colorPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectCartoonComponent::setSaturation", _1(saturation), { this->colorPass->getFragmentProgramParameters()->setNamedConstant("saturation", saturation); });
        }
    }

    Ogre::Real CompositorEffectCartoonComponent::getSaturation(void) const
    {
        return this->saturation->getReal();
    }

    void CompositorEffectCartoonComponent::setEdgeDarkness(Ogre::Real edgeDarkness)
    {
        this->edgeDarkness->setValue(edgeDarkness);

        if (nullptr != this->colorPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectCartoonComponent::setEdgeDarkness", _1(edgeDarkness), { this->colorPass->getFragmentProgramParameters()->setNamedConstant("edgeDarkness", edgeDarkness); });
        }
    }

    Ogre::Real CompositorEffectCartoonComponent::getEdgeDarkness(void) const
    {
        return this->edgeDarkness->getReal();
    }

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectVolumetricLightComponent::CompositorEffectVolumetricLightComponent() :
        CompositorEffectBaseComponent(),
        sunMaskPass(nullptr),
        radialBlurPass(nullptr),
        blendPass(nullptr),
        sunLight(nullptr),
        lightId(new Variant(CompositorEffectVolumetricLightComponent::AttrLightId(), static_cast<unsigned long>(0), this->attributes)),
        godRayStrength(new Variant(CompositorEffectVolumetricLightComponent::AttrGodRayStrength(), 1.0f, this->attributes)),
        sunThreshold(new Variant(CompositorEffectVolumetricLightComponent::AttrSunThreshold(), 0.8f, this->attributes)),
        sunRadius(new Variant(CompositorEffectVolumetricLightComponent::AttrSunRadius(), 0.4f, this->attributes)),
        decay(new Variant(CompositorEffectVolumetricLightComponent::AttrDecay(), 0.97f, this->attributes)),
        density(new Variant(CompositorEffectVolumetricLightComponent::AttrDensity(), 0.8f, this->attributes)),
        exposure(new Variant(CompositorEffectVolumetricLightComponent::AttrExposure(), 0.25f, this->attributes)),
        tint(new Variant(CompositorEffectVolumetricLightComponent::AttrTint(), Ogre::Vector3(1.0f, 0.9f, 0.7f), this->attributes))
    {
        this->effectName = "Volumetric Light";

        // Editor slider ranges
        this->godRayStrength->setConstraints(0.0f, 3.0f);
        this->sunThreshold->setConstraints(0.0f, 1.0f);
        this->sunRadius->setConstraints(0.05f, 1.5f);
        this->decay->setConstraints(0.90f, 1.0f);
        this->density->setConstraints(0.1f, 1.0f);
        this->exposure->setConstraints(0.01f, 2.0f);

		this->godRayStrength->setDescription("Controls the overall intensity of the volumetric light and god ray effect.");
        this->sunThreshold->setDescription("Defines the brightness threshold required for light scattering contribution.");
        this->sunRadius->setDescription("Controls the apparent size of the light source used for the god ray effect.");
        this->decay->setDescription("Controls how quickly volumetric light fades over distance.");
        this->density->setDescription("Controls the sampling density of the volumetric light scattering effect.");
        this->exposure->setDescription("Controls the exposure multiplier applied to the volumetric lighting.");
    }

    CompositorEffectVolumetricLightComponent::~CompositorEffectVolumetricLightComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectVolumetricLightComponent] Destructor for game object: " + this->gameObjectPtr->getName());

        this->sunMaskPass = nullptr;
        this->radialBlurPass = nullptr;
        this->blendPass = nullptr;
        this->sunLight = nullptr;
    }

    bool CompositorEffectVolumetricLightComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = CompositorEffectBaseComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLightId())
        {
            this->lightId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGodRayStrength())
        {
            this->setGodRayStrength(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSunThreshold())
        {
            this->setSunThreshold(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSunRadius())
        {
            this->setSunRadius(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDecay())
        {
            this->setDecay(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDensity())
        {
            this->setDensity(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrExposure())
        {
            this->setExposure(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrTint())
        {
            this->setTint(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr CompositorEffectVolumetricLightComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool CompositorEffectVolumetricLightComponent::connect(void)
    {
        CompositorEffectBaseComponent::connect();

        return true;
    }

    bool CompositorEffectVolumetricLightComponent::disconnect(void)
    {
        CompositorEffectBaseComponent::disconnect();

        return true;
    }

    bool CompositorEffectVolumetricLightComponent::postInit(void)
    {
        bool success = CompositorEffectBaseComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectVolumetricLightComponent] Init for game object: " + this->gameObjectPtr->getName());

        // ----------------------------------------------------------------
        // Resolve the three materials / passes
        // ----------------------------------------------------------------
        auto loadPass = [this](const Ogre::String& matName, Ogre::MaterialPtr& matPtr, Ogre::Pass*& passPtr) -> bool
        {
            matPtr = Ogre::MaterialManager::getSingletonPtr()->getByName(matName);
            if (matPtr.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectVolumetricLightComponent] Material not found: '" + matName + "'");
                return false;
            }
            passPtr = matPtr->getTechnique(0)->getPass(0);
            return true;
        };

        if (!loadPass("Postprocess/VolumetricLightSunMask", this->sunMaskMaterial, this->sunMaskPass))
        {
            return false;
        }
        if (!loadPass("Postprocess/VolumetricLightRadialBlur", this->radialBlurMaterial, this->radialBlurPass))
        {
            return false;
        }
        if (!loadPass("Postprocess/VolumetricLightBlend", this->blendMaterial, this->blendPass))
        {
            return false;
        }

        // ----------------------------------------------------------------
        // Resolve directional light
        // ----------------------------------------------------------------
        this->resolveLight();

        // ----------------------------------------------------------------
        // Push initial values to GPU
        // ----------------------------------------------------------------
        this->setGodRayStrength(this->godRayStrength->getReal());
        this->setSunThreshold(this->sunThreshold->getReal());
        this->setSunRadius(this->sunRadius->getReal());
        this->setDecay(this->decay->getReal());
        this->setDensity(this->density->getReal());
        this->setExposure(this->exposure->getReal());
        this->setTint(this->tint->getVector3());

        return success;
    }

    void CompositorEffectVolumetricLightComponent::onRemoveComponent(void)
    {
        CompositorEffectBaseComponent::onRemoveComponent();

        // Remove the per-frame render-thread closure so it does not hold a dangling 'this'
        Ogre::String closureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);
    }

    void CompositorEffectVolumetricLightComponent::actualizeValue(Variant* attribute)
    {
        CompositorEffectBaseComponent::actualizeValue(attribute);

        const Ogre::String& name = attribute->getName();

        if (name == AttrLightId())
        {
            this->setLightId(attribute->getULong());
        }
        else if (name == AttrGodRayStrength())
        {
            this->setGodRayStrength(attribute->getReal());
        }
        else if (name == AttrSunThreshold())
        {
            this->setSunThreshold(attribute->getReal());
        }
        else if (name == AttrSunRadius())
        {
            this->setSunRadius(attribute->getReal());
        }
        else if (name == AttrDecay())
        {
            this->setDecay(attribute->getReal());
        }
        else if (name == AttrDensity())
        {
            this->setDensity(attribute->getReal());
        }
        else if (name == AttrExposure())
        {
            this->setExposure(attribute->getReal());
        }
        else if (name == AttrTint())
        {
            this->setTint(attribute->getVector3());
        }
    }

    void CompositorEffectVolumetricLightComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // Type codes: 2=int, 6=real, 7=string, 8=vector2, 9=vector3, 10=vector4, 12=bool
        CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

        auto writeReal = [&](const char* name, Ogre::Real value)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "6"));
            p->append_attribute(doc.allocate_attribute("name", name));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(p);
        };

        // Light ID (unsigned long, type 2)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "2"));
            p->append_attribute(doc.allocate_attribute("name", "Light Id"));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lightId->getULong())));
            propertiesXML->append_node(p);
        }

        writeReal("God Ray Strength", this->godRayStrength->getReal());
        writeReal("Sun Threshold", this->sunThreshold->getReal());
        writeReal("Sun Radius", this->sunRadius->getReal());
        writeReal("Decay", this->decay->getReal());
        writeReal("Density", this->density->getReal());
        writeReal("Exposure", this->exposure->getReal());

        // Tint (vector3, type 9)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "9"));
            p->append_attribute(doc.allocate_attribute("name", "Tint"));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tint->getVector3())));
            propertiesXML->append_node(p);
        }
    }

    Ogre::String CompositorEffectVolumetricLightComponent::getClassName(void) const
    {
        return "CompositorEffectVolumetricLightComponent";
    }

    Ogre::String CompositorEffectVolumetricLightComponent::getParentClassName(void) const
    {
        return "CompositorEffectBaseComponent";
    }

    // -----------------------------------------------------------------------
    // Per-frame update — project sun to screen space
    // -----------------------------------------------------------------------

    void CompositorEffectVolumetricLightComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }

        if (nullptr == this->sunLight)
        {
            return;
        }
        if (nullptr == this->sunMaskPass)
        {
            return;
        }
        if (nullptr == this->radialBlurPass)
        {
            return;
        }
        if (nullptr == this->workspaceBaseComponent)
        {
            return;
        }

		if (nullptr == this->camera)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectVolumetricLightComponent] Could not find camera, hence effect cannot be enabled for game object: " + this->gameObjectPtr->getName());
            return;
        }

        // Capture pointers by value for the render-thread closure
        Ogre::Pass* maskPass = this->sunMaskPass;
        Ogre::Pass* blurPass = this->radialBlurPass;
        Ogre::Light* light = this->sunLight;
        Ogre::Camera* camera = this->camera;

        auto closureFunction = [maskPass, blurPass, light, camera](Ogre::Real /*renderDt*/)
        {
            // The sun direction points FROM the sun TOWARD the scene.
            // To find the sun's position, we go in the OPPOSITE direction.
            Ogre::Vector3 sunDir = light->getDerivedDirectionUpdated().normalisedCopy();

            // Place a virtual sun point very far away in the anti-light direction
            Ogre::Vector3 sunWorldPos = camera->getDerivedPosition() + (-sunDir) * 100000.0f;

            // Build view-projection matrix.
            // getProjectionMatrixWithRSDepth accounts for D3D/GL depth-range differences.
            Ogre::Matrix4 viewProj = camera->getProjectionMatrixWithRSDepth() * camera->getViewMatrix();

            Ogre::Vector4 clipPos = viewProj * Ogre::Vector4(sunWorldPos.x, sunWorldPos.y, sunWorldPos.z, 1.0f);

            Ogre::Vector2 sunScreenPos;

            if (clipPos.w > 0.0001f)
            {
                // Standard NDC → UV conversion.
                // Y is flipped: NDC +1 = top of screen = UV 0 in Ogre's convention.
                sunScreenPos.x = (clipPos.x / clipPos.w) * 0.5f + 0.5f;
                sunScreenPos.y = 1.0f - ((clipPos.y / clipPos.w) * 0.5f + 0.5f);
            }
            else
            {
                // Sun is behind the camera (clipPos.w <= 0).
                // clipPos.xy still encodes a 2D screen-space direction — use it
                // to park sunScreenPos just past the nearest screen edge (0.55).
                // The sun mask will produce no emitters anyway since bright sky
                // is not visible when looking away from the sun.
                Ogre::Vector2 screenDir(clipPos.x, -clipPos.y);
                const float len = screenDir.length();
                if (len > 0.0001f)
                {
                    screenDir /= len;
                    sunScreenPos = Ogre::Vector2(0.5f + screenDir.x * 0.55f, 0.5f + screenDir.y * 0.55f);
                }
                else
                {
                    sunScreenPos = Ogre::Vector2(-1.0f, -1.0f);
                }
            }

            // Push the updated position to both passes that need it
            maskPass->getFragmentProgramParameters()->setNamedConstant("sunScreenPos", sunScreenPos);
            blurPass->getFragmentProgramParameters()->setNamedConstant("sunScreenPos", sunScreenPos);
        };

        Ogre::String closureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closureFunction, false);
    }

    // -----------------------------------------------------------------------
    // Light resolution
    // -----------------------------------------------------------------------

    void CompositorEffectVolumetricLightComponent::resolveLight(void)
    {
        this->sunLight = nullptr;

        unsigned long id = this->lightId->getULong();

        // Fallback to the main scene light when no ID is specified
        if (id == 0)
        {
            id = GameObjectController::MAIN_LIGHT_ID;
        }

        GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);

        if (nullptr == lightGameObjectPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectVolumetricLightComponent] Could not find game object "
                                                                                "for light id: " +
                                                                                    Ogre::StringConverter::toString(id) + "  game object: " + this->gameObjectPtr->getName());
            return;
        }

        auto lightCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());

        if (nullptr != lightCompPtr)
        {
            this->sunLight = lightCompPtr->getOgreLight();
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[CompositorEffectVolumetricLightComponent] Game object with id: " + Ogre::StringConverter::toString(id) + " has no LightDirectionalComponent. Game object: " + this->gameObjectPtr->getName());
        }
    }

    // -----------------------------------------------------------------------
    // Setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectVolumetricLightComponent::setLightId(unsigned long lightId)
    {
        this->lightId->setValue(lightId);
        // Re-resolve light at runtime (e.g. user changes the ID in NOWA-Design)
        this->resolveLight();
    }

    unsigned long CompositorEffectVolumetricLightComponent::getLightId(void) const
    {
        return this->lightId->getULong();
    }

    // --- Sun Mask pass ---

    void CompositorEffectVolumetricLightComponent::setSunThreshold(Ogre::Real sunThreshold)
    {
        this->sunThreshold->setValue(sunThreshold);
        if (nullptr != this->sunMaskPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setSunThreshold", _1(sunThreshold), { this->sunMaskPass->getFragmentProgramParameters()->setNamedConstant("sunThreshold", sunThreshold); });
        }
    }

    Ogre::Real CompositorEffectVolumetricLightComponent::getSunThreshold(void) const
    {
        return this->sunThreshold->getReal();
    }

    void CompositorEffectVolumetricLightComponent::setSunRadius(Ogre::Real sunRadius)
    {
        this->sunRadius->setValue(sunRadius);
        if (nullptr != this->sunMaskPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setSunRadius", _1(sunRadius), { this->sunMaskPass->getFragmentProgramParameters()->setNamedConstant("sunRadius", sunRadius); });
        }
    }

    Ogre::Real CompositorEffectVolumetricLightComponent::getSunRadius(void) const
    {
        return this->sunRadius->getReal();
    }

    // --- Radial Blur pass ---

    void CompositorEffectVolumetricLightComponent::setDecay(Ogre::Real decay)
    {
        this->decay->setValue(decay);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setDecay", _1(decay), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("decay", decay); });
        }
    }

    Ogre::Real CompositorEffectVolumetricLightComponent::getDecay(void) const
    {
        return this->decay->getReal();
    }

    void CompositorEffectVolumetricLightComponent::setDensity(Ogre::Real density)
    {
        this->density->setValue(density);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setDensity", _1(density), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("density", density); });
        }
    }

    Ogre::Real CompositorEffectVolumetricLightComponent::getDensity(void) const
    {
        return this->density->getReal();
    }

    void CompositorEffectVolumetricLightComponent::setExposure(Ogre::Real exposure)
    {
        this->exposure->setValue(exposure);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setExposure", _1(exposure), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("exposure", exposure); });
        }
    }

    Ogre::Real CompositorEffectVolumetricLightComponent::getExposure(void) const
    {
        return this->exposure->getReal();
    }

    // --- Blend pass ---

    void CompositorEffectVolumetricLightComponent::setGodRayStrength(Ogre::Real godRayStrength)
    {
        this->godRayStrength->setValue(godRayStrength);
        if (nullptr != this->blendPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setGodRayStrength", _1(godRayStrength), { this->blendPass->getFragmentProgramParameters()->setNamedConstant("godRayStrength", godRayStrength); });
        }
    }

    Ogre::Real CompositorEffectVolumetricLightComponent::getGodRayStrength(void) const
    {
        return this->godRayStrength->getReal();
    }

    void CompositorEffectVolumetricLightComponent::setTint(const Ogre::Vector3& tint)
    {
        this->tint->setValue(tint);
        if (nullptr != this->blendPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectVolumetricLightComponent::setTint", _1(tint), { this->blendPass->getFragmentProgramParameters()->setNamedConstant("tint", tint); });
        }
    }

    Ogre::Vector3 CompositorEffectVolumetricLightComponent::getTint(void) const
    {
        return this->tint->getVector3();
    }

	CompositorEffectVolumetricLightComponent* getCompositorEffectVolumetricLightComponent(GameObject* gameObject)
    {
        return makeStrongPtr<CompositorEffectVolumetricLightComponent>(gameObject->getComponent<CompositorEffectVolumetricLightComponent>()).get();
    }

    CompositorEffectVolumetricLightComponent* getCompositorEffectVolumetricLightComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<CompositorEffectVolumetricLightComponent>(gameObject->getComponentFromName<CompositorEffectVolumetricLightComponent>(name)).get();
    }

    void CompositorEffectVolumetricLightComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)
		[
			luabind::class_<CompositorEffectVolumetricLightComponent, GameObjectComponent>("CompositorEffectVolumetricLightComponent")
            .def("setLightId", &CompositorEffectVolumetricLightComponent::setLightId)
            .def("getLightId", &CompositorEffectVolumetricLightComponent::getLightId)
            .def("setGodRayStrength", &CompositorEffectVolumetricLightComponent::setGodRayStrength)
            .def("getGodRayStrength", &CompositorEffectVolumetricLightComponent::getGodRayStrength)
            .def("setSunThreshold", &CompositorEffectVolumetricLightComponent::setSunThreshold)
            .def("getSunThreshold", &CompositorEffectVolumetricLightComponent::getSunThreshold)
            .def("setSunRadius", &CompositorEffectVolumetricLightComponent::setSunRadius)
            .def("getSunRadius", &CompositorEffectVolumetricLightComponent::getSunRadius)
            .def("setDecay", &CompositorEffectVolumetricLightComponent::setDecay)
            .def("getDecay", &CompositorEffectVolumetricLightComponent::getDecay)
            .def("setDensity", &CompositorEffectVolumetricLightComponent::setDensity)
            .def("getDensity", &CompositorEffectVolumetricLightComponent::getDensity)
            .def("setExposure", &CompositorEffectVolumetricLightComponent::setExposure)
            .def("getExposure", &CompositorEffectVolumetricLightComponent::getExposure)
            .def("setTint", &CompositorEffectVolumetricLightComponent::setTint)
            .def("getTint", &CompositorEffectVolumetricLightComponent::getTint)
		];

        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "class inherits GameObjectComponent", getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setLightId(string lightId)", "Sets the light game object id used as the volumetric light source.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "string getLightId()", "Gets the current volumetric light source game object id.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setGodRayStrength(float strength)", "Sets the overall intensity of the god ray effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "float getGodRayStrength()", "Gets the overall god ray intensity.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setSunThreshold(float threshold)", "Sets the brightness threshold for light scattering contribution.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "float getSunThreshold()", "Gets the sun brightness threshold.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setSunRadius(float radius)", "Sets the apparent screen space radius of the light source.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "float getSunRadius()", "Gets the apparent light source radius.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setDecay(float decay)", "Sets how quickly light scattering fades over distance.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "float getDecay()", "Gets the light scattering decay factor.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setDensity(float density)", "Sets the sample density used for volumetric light scattering.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "float getDensity()", "Gets the volumetric light sample density.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setExposure(float exposure)", "Sets the exposure multiplier of the volumetric light effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "float getExposure()", "Gets the volumetric light exposure multiplier.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "void setTint(Vector3 tint)", "Sets the RGB tint color of the volumetric light effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectVolumetricLightComponent", "Vector3 getTint()", "Gets the RGB tint color.");

        gameObjectClass.def("getCompositorEffectVolumetricLightComponent", &getCompositorEffectVolumetricLightComponent);
        gameObjectClass.def("getCompositorEffectVolumetricLightComponentFromName", &getCompositorEffectVolumetricLightComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectVolumetricLightComponent getCompositorEffectVolumetricLightComponent()", "Gets the CompositorEffectVolumetricLightComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectVolumetricLightComponent getCompositorEffectVolumetricLightComponentFromName(string name)", "Gets the CompositorEffectVolumetricLightComponent by name.");

        gameObjectControllerClass.def("castCompositorEffectVolumetricLightComponent", &GameObjectController::cast<CompositorEffectVolumetricLightComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CompositorEffectVolumetricLightComponent castCompositorEffectVolumetricLightComponent(CompositorEffectVolumetricLightComponent other)",
            "Casts for Lua auto completion.");
    }

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectFogComponent::CompositorEffectFogComponent() :
        CompositorEffectBaseComponent(),
        fogPass(nullptr),
        depthFogDensity(new Variant(CompositorEffectFogComponent::AttrDepthFogDensity(), 0.008f, this->attributes)),
        depthFogStart(new Variant(CompositorEffectFogComponent::AttrDepthFogStart(), 20.0f, this->attributes)),
        heightFogDensity(new Variant(CompositorEffectFogComponent::AttrHeightFogDensity(), 0.06f, this->attributes)),
        heightFogStart(new Variant(CompositorEffectFogComponent::AttrHeightFogStart(), 0.0f, this->attributes)),
        heightFogEnd(new Variant(CompositorEffectFogComponent::AttrHeightFogEnd(), 40.0f, this->attributes)),
        fogColor(new Variant(CompositorEffectFogComponent::AttrFogColor(), Ogre::Vector3(0.7f, 0.75f, 0.8f), this->attributes)),
        fogSkyBlend(new Variant(CompositorEffectFogComponent::AttrFogSkyBlend(), 0.3f, this->attributes))
    {
        this->effectName = "Fog";

        this->depthFogDensity->setConstraints(0.0f, 0.5f);
        this->depthFogStart->setConstraints(0.0f, 5000.0f);
        this->heightFogDensity->setConstraints(0.0f, 1.0f);
        this->heightFogStart->setConstraints(-1000.0f, 1000.0f);
        this->heightFogEnd->setConstraints(-1000.0f, 1000.0f);
        this->fogSkyBlend->setConstraints(0.0f, 1.0f);

		this->depthFogDensity->setDescription("Controls the intensity of distance based fog. Higher values create thicker atmospheric haze.");
        this->depthFogStart->setDescription("Defines the camera distance where depth fog begins to appear.");
        this->heightFogDensity->setDescription("Controls the intensity of height based fog. Higher values create denser ground or valley fog.");

        this->heightFogStart->setDescription("Defines the world space height where height fog starts.");
        this->heightFogEnd->setDescription("Defines the world space height where height fog fully fades out.");
        this->fogSkyBlend->setDescription("Controls how much the fog color blends with the sky color. 0 uses only the fog color, 1 fully blends with the sky.");
    }

    CompositorEffectFogComponent::~CompositorEffectFogComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectFogComponent] Destructor for game object: " + this->gameObjectPtr->getName());

        this->fogPass = nullptr;
    }

    bool CompositorEffectFogComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = CompositorEffectBaseComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDepthFogDensity())
        {
            this->setDepthFogDensity(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDepthFogStart())
        {
            this->setDepthFogStart(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHeightFogDensity())
        {
            this->setHeightFogDensity(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHeightFogStart())
        {
            this->setHeightFogStart(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHeightFogEnd())
        {
            this->setHeightFogEnd(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFogColor())
        {
            this->setFogColor(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFogSkyBlend())
        {
            this->setFogSkyBlend(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr CompositorEffectFogComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool CompositorEffectFogComponent::postInit(void)
    {
        bool success = CompositorEffectBaseComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectFogComponent] Init for game object: " + this->gameObjectPtr->getName());

        const Ogre::String materialName = "Postprocess/Fog";
        this->fogMaterial = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName);

        if (this->fogMaterial.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectFogComponent] Material not found: '" + materialName + "'");
            return false;
        }

        return success;
    }

    void CompositorEffectFogComponent::onRemoveComponent(void)
    {
        CompositorEffectBaseComponent::onRemoveComponent();

        Ogre::String closureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);
    }

    bool CompositorEffectFogComponent::connect(void)
    {
        CompositorEffectBaseComponent::connect();

		this->fogPass = this->fogMaterial->getTechnique(0)->getPass(0);

        // Push static camera params immediately — projectionParams and
        // farClipDistance don't change per frame (unless camera changes).
        // invViewProjMatrix is pushed per-frame in update().
        if (nullptr != this->workspaceBaseComponent)
        {
            if (nullptr == this->camera)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectFogComponent] Could not find camera, hence effect cannot be enabled for game object: " + this->gameObjectPtr->getName());
                return true;
            }

            Ogre::Vector2 projAB = this->camera->getProjectionParamsAB();
            projAB.y /= this->camera->getFarClipDistance();

            Ogre::Pass* pass = this->fogPass;
            Ogre::Camera* camera = this->camera;
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::postInit_projParams", _3(projAB, pass, camera), {
                auto* params = pass->getFragmentProgramParameters().get();
                params->setNamedConstant("projectionParams", projAB);
                params->setNamedConstant("farClipDistance", camera->getFarClipDistance());
            });
        }

        // Push all tunable values
        this->setDepthFogDensity(this->depthFogDensity->getReal());
        this->setDepthFogStart(this->depthFogStart->getReal());
        this->setHeightFogDensity(this->heightFogDensity->getReal());
        this->setHeightFogStart(this->heightFogStart->getReal());
        this->setHeightFogEnd(this->heightFogEnd->getReal());
        this->setFogColor(this->fogColor->getVector3());
        this->setFogSkyBlend(this->fogSkyBlend->getReal());

        return true;
    }

	
    bool CompositorEffectFogComponent::disconnect(void)
    {
        CompositorEffectBaseComponent::disconnect();
        return true;
    }

    void CompositorEffectFogComponent::actualizeValue(Variant* attribute)
    {
        CompositorEffectBaseComponent::actualizeValue(attribute);

        const Ogre::String& name = attribute->getName();

        if (name == AttrDepthFogDensity())
        {
            this->setDepthFogDensity(attribute->getReal());
        }
        else if (name == AttrDepthFogStart())
        {
            this->setDepthFogStart(attribute->getReal());
        }
        else if (name == AttrHeightFogDensity())
        {
            this->setHeightFogDensity(attribute->getReal());
        }
        else if (name == AttrHeightFogStart())
        {
            this->setHeightFogStart(attribute->getReal());
        }
        else if (name == AttrHeightFogEnd())
        {
            this->setHeightFogEnd(attribute->getReal());
        }
        else if (name == AttrFogColor())
        {
            this->setFogColor(attribute->getVector3());
        }
        else if (name == AttrFogSkyBlend())
        {
            this->setFogSkyBlend(attribute->getReal());
        }
    }

    void CompositorEffectFogComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

        auto writeReal = [&](const char* name, Ogre::Real value)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "6"));
            p->append_attribute(doc.allocate_attribute("name", name));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(p);
        };

        auto writeVec3 = [&](const char* name, const Ogre::Vector3& value)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "9"));
            p->append_attribute(doc.allocate_attribute("name", name));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(p);
        };

        writeReal("Depth Fog Density", this->depthFogDensity->getReal());
        writeReal("Depth Fog Start", this->depthFogStart->getReal());
        writeReal("Height Fog Density", this->heightFogDensity->getReal());
        writeReal("Height Fog Start", this->heightFogStart->getReal());
        writeReal("Height Fog End", this->heightFogEnd->getReal());
        writeVec3("Fog Color", this->fogColor->getVector3());
        writeReal("Fog Sky Blend", this->fogSkyBlend->getReal());
    }

    Ogre::String CompositorEffectFogComponent::getClassName(void) const
    {
        return "CompositorEffectFogComponent";
    }

    Ogre::String CompositorEffectFogComponent::getParentClassName(void) const
    {
        return "CompositorEffectBaseComponent";
    }

    // -----------------------------------------------------------------------
    // Per-frame update — push invViewProjMatrix to shader
    // -----------------------------------------------------------------------

    void CompositorEffectFogComponent::update(Ogre::Real dt, bool notSimulating)
    {
		if (true == notSimulating)
		{
            return;
		}

        if (nullptr == this->fogPass)
        {
            return;
        }
        if (nullptr == this->workspaceBaseComponent)
        {
            return;
        }
        if (nullptr == this->camera)
        {
            return;
        }

        Ogre::Pass* pass = this->fogPass;
        Ogre::Camera* camera = this->camera;

        auto closureFunction = [pass, camera](Ogre::Real /*renderDt*/)
        {
            // Inverse view-projection: world pos = invVP * ndcPos
            Ogre::Matrix4 invViewProj = (camera->getProjectionMatrixWithRSDepth() * camera->getViewMatrix()).inverse();

            pass->getFragmentProgramParameters()->setNamedConstant("invViewProjMatrix", invViewProj);
        };

        Ogre::String closureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closureFunction, false);
    }

    // -----------------------------------------------------------------------
    // Setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectFogComponent::setDepthFogDensity(Ogre::Real density)
    {
        this->depthFogDensity->setValue(density);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setDepthFogDensity", _1(density), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("depthFogDensity", density); });
        }
    }

    Ogre::Real CompositorEffectFogComponent::getDepthFogDensity(void) const
    {
        return this->depthFogDensity->getReal();
    }

    void CompositorEffectFogComponent::setDepthFogStart(Ogre::Real start)
    {
        this->depthFogStart->setValue(start);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setDepthFogStart", _1(start), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("depthFogStart", start); });
        }
    }

    Ogre::Real CompositorEffectFogComponent::getDepthFogStart(void) const
    {
        return this->depthFogStart->getReal();
    }

    void CompositorEffectFogComponent::setHeightFogDensity(Ogre::Real density)
    {
        this->heightFogDensity->setValue(density);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setHeightFogDensity", _1(density), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("heightFogDensity", density); });
        }
    }

    Ogre::Real CompositorEffectFogComponent::getHeightFogDensity(void) const
    {
        return this->heightFogDensity->getReal();
    }

    void CompositorEffectFogComponent::setHeightFogStart(Ogre::Real worldY)
    {
        this->heightFogStart->setValue(worldY);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setHeightFogStart", _1(worldY), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("heightFogStart", worldY); });
        }
    }

    Ogre::Real CompositorEffectFogComponent::getHeightFogStart(void) const
    {
        return this->heightFogStart->getReal();
    }

    void CompositorEffectFogComponent::setHeightFogEnd(Ogre::Real worldY)
    {
        this->heightFogEnd->setValue(worldY);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setHeightFogEnd", _1(worldY), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("heightFogEnd", worldY); });
        }
    }

    Ogre::Real CompositorEffectFogComponent::getHeightFogEnd(void) const
    {
        return this->heightFogEnd->getReal();
    }

    void CompositorEffectFogComponent::setFogColor(const Ogre::Vector3& color)
    {
        this->fogColor->setValue(color);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setFogColor", _1(color), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("fogColor", color); });
        }
    }

    Ogre::Vector3 CompositorEffectFogComponent::getFogColor(void) const
    {
        return this->fogColor->getVector3();
    }

    void CompositorEffectFogComponent::setFogSkyBlend(Ogre::Real blend)
    {
        this->fogSkyBlend->setValue(blend);
        if (nullptr != this->fogPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectFogComponent::setFogSkyBlend", _1(blend), { this->fogPass->getFragmentProgramParameters()->setNamedConstant("fogSkyBlend", blend); });
        }
    }

    Ogre::Real CompositorEffectFogComponent::getFogSkyBlend(void) const
    {
        return this->fogSkyBlend->getReal();
    }

	CompositorEffectFogComponent* getCompositorEffectFogComponent(GameObject* gameObject)
    {
        return makeStrongPtr<CompositorEffectFogComponent>(gameObject->getComponent<CompositorEffectFogComponent>()).get();
    }

    CompositorEffectFogComponent* getCompositorEffectFogComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<CompositorEffectFogComponent>(gameObject->getComponentFromName<CompositorEffectFogComponent>(name)).get();
    }

    void CompositorEffectFogComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)
		[
			luabind::class_<CompositorEffectFogComponent, GameObjectComponent>("CompositorEffectFogComponent")
            // Depth fog
            .def("setDepthFogDensity", &CompositorEffectFogComponent::setDepthFogDensity)
            .def("getDepthFogDensity", &CompositorEffectFogComponent::getDepthFogDensity)
            .def("setDepthFogStart", &CompositorEffectFogComponent::setDepthFogStart)
            .def("getDepthFogStart", &CompositorEffectFogComponent::getDepthFogStart)
            // Height fog
            .def("setHeightFogDensity", &CompositorEffectFogComponent::setHeightFogDensity)
            .def("getHeightFogDensity", &CompositorEffectFogComponent::getHeightFogDensity)
            .def("setHeightFogStart", &CompositorEffectFogComponent::setHeightFogStart)
            .def("getHeightFogStart", &CompositorEffectFogComponent::getHeightFogStart)
            .def("setHeightFogEnd", &CompositorEffectFogComponent::setHeightFogEnd)
            .def("getHeightFogEnd", &CompositorEffectFogComponent::getHeightFogEnd)
            // Shared
            .def("setFogColor", &CompositorEffectFogComponent::setFogColor)
            .def("getFogColor", &CompositorEffectFogComponent::getFogColor)
            .def("setFogSkyBlend", &CompositorEffectFogComponent::setFogSkyBlend)
            .def("getFogSkyBlend", &CompositorEffectFogComponent::getFogSkyBlend)
		];

        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "class inherits GameObjectComponent", getStaticInfoText());

        // Depth fog
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setDepthFogDensity(float density)", "Sets the density/intensity of the distance based fog effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "float getDepthFogDensity()", "Gets the current distance fog density.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setDepthFogStart(float start)", "Sets the world distance at which depth fog begins.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "float getDepthFogStart()", "Gets the depth fog start distance.");

        // Height fog
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setHeightFogDensity(float density)", "Sets the density/intensity of the height based fog effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "float getHeightFogDensity()", "Gets the current height fog density.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setHeightFogStart(float startHeight)", "Sets the world height where height fog begins.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "float getHeightFogStart()", "Gets the height fog start height.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setHeightFogEnd(float endHeight)", "Sets the world height where height fog fully fades out.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "float getHeightFogEnd()", "Gets the height fog end height.");

        // Shared fog settings
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setFogColor(Vector3 color)", "Sets the fog RGB color.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "Vector3 getFogColor()", "Gets the current fog RGB color.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "void setFogSkyBlend(float blend)",
            "Sets how much the fog blends with the sky color. "
            "0 = only fog color, 1 = fully blended with sky.");

        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectFogComponent", "float getFogSkyBlend()", "Gets the sky blend factor.");

        // GameObject bindings
        gameObjectClass.def("getCompositorEffectFogComponent", &getCompositorEffectFogComponent);
        gameObjectClass.def("getCompositorEffectFogComponentFromName", &getCompositorEffectFogComponentFromName);
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectFogComponent getCompositorEffectFogComponent()", "Gets the CompositorEffectFogComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectFogComponent getCompositorEffectFogComponentFromName(string name)", "Gets the CompositorEffectFogComponent by name.");

        // Cast helper
        gameObjectControllerClass.def("castCompositorEffectFogComponent", &GameObjectController::cast<CompositorEffectFogComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CompositorEffectFogComponent castCompositorEffectFogComponent(CompositorEffectFogComponent other)", "Casts for Lua auto completion.");
    }

	///////////////////////////////////////////////////////////////////////////////////////////////

    CompositorEffectLightShaftsComponent::CompositorEffectLightShaftsComponent() :
        CompositorEffectBaseComponent(),
        occlusionMaskPass(nullptr),
        radialBlurPass(nullptr),
        blendPass(nullptr),
        sunLight(nullptr),
        lightId(new Variant(CompositorEffectLightShaftsComponent::AttrLightId(), static_cast<unsigned long>(0), this->attributes)),
        occlusionDepthThreshold(new Variant(CompositorEffectLightShaftsComponent::AttrOcclusionDepthThreshold(), 0.9999f, this->attributes)),
        sunRadius(new Variant(CompositorEffectLightShaftsComponent::AttrSunRadius(), 0.5f, this->attributes)),
        brightnessThreshold(new Variant(CompositorEffectLightShaftsComponent::AttrBrightnessThreshold(), 0.6f, this->attributes)),
        decay(new Variant(CompositorEffectLightShaftsComponent::AttrDecay(), 0.96f, this->attributes)),
        density(new Variant(CompositorEffectLightShaftsComponent::AttrDensity(), 0.9f, this->attributes)),
        exposure(new Variant(CompositorEffectLightShaftsComponent::AttrExposure(), 0.3f, this->attributes)),
        shaftSharpness(new Variant(CompositorEffectLightShaftsComponent::AttrShaftSharpness(), 1.5f, this->attributes)),
        shaftStrength(new Variant(CompositorEffectLightShaftsComponent::AttrShaftStrength(), 1.0f, this->attributes)),
        tint(new Variant(CompositorEffectLightShaftsComponent::AttrTint(), Ogre::Vector3(1.0f, 0.92f, 0.75f), this->attributes))
    {
        this->effectName = "Light Shafts";

        this->occlusionDepthThreshold->setConstraints(0.99f, 1.0f);
        this->sunRadius->setConstraints(0.05f, 1.5f);
        this->brightnessThreshold->setConstraints(0.0f, 1.0f);
        this->decay->setConstraints(0.90f, 1.0f);
        this->density->setConstraints(0.1f, 1.0f);
        this->exposure->setConstraints(0.01f, 2.0f);
        this->shaftSharpness->setConstraints(0.5f, 4.0f);
        this->shaftStrength->setConstraints(0.0f, 3.0f);

        this->occlusionDepthThreshold->setDescription("Controls the depth threshold used to determine light occlusion. Higher values reduce shaft visibility behind geometry.");
        this->sunRadius->setDescription("Controls the apparent size of the light source used for the shaft effect.");
        this->brightnessThreshold->setDescription("Defines the minimum brightness required for light shaft generation.");
        this->decay->setDescription("Controls how quickly the light shafts fade over distance.");
        this->density->setDescription("Controls the sampling density of the light shaft effect.");
        this->exposure->setDescription("Controls the exposure multiplier applied to the light shafts.");
        this->shaftSharpness->setDescription("Controls how sharp or blurred the light shafts appear.");
        this->shaftStrength->setDescription("Controls the final intensity of the blended light shaft effect.");
    }

    CompositorEffectLightShaftsComponent::~CompositorEffectLightShaftsComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectLightShaftsComponent] Destructor for game object: " + this->gameObjectPtr->getName());

        this->occlusionMaskPass = nullptr;
        this->radialBlurPass = nullptr;
        this->blendPass = nullptr;
        this->sunLight = nullptr;
    }

    bool CompositorEffectLightShaftsComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = CompositorEffectBaseComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLightId())
        {
            this->lightId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOcclusionDepthThreshold())
        {
            this->setOcclusionDepthThreshold(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSunRadius())
        {
            this->setSunRadius(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrightnessThreshold())
        {
            this->setBrightnessThreshold(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDecay())
        {
            this->setDecay(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDensity())
        {
            this->setDensity(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrExposure())
        {
            this->setExposure(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShaftSharpness())
        {
            this->setShaftSharpness(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShaftStrength())
        {
            this->setShaftStrength(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrTint())
        {
            this->setTint(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr CompositorEffectLightShaftsComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool CompositorEffectLightShaftsComponent::postInit(void)
    {
        bool success = CompositorEffectBaseComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectLightShaftsComponent] Init for game object: " + this->gameObjectPtr->getName());

        auto loadPass = [this](const Ogre::String& matName, Ogre::MaterialPtr& matPtr, Ogre::Pass*& passPtr) -> bool
        {
            matPtr = Ogre::MaterialManager::getSingletonPtr()->getByName(matName);
            if (matPtr.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectLightShaftsComponent] Material not found: '" + matName + "'");
                return false;
            }
            passPtr = matPtr->getTechnique(0)->getPass(0);
            return true;
        };

        if (!loadPass("Postprocess/LightShaftsOcclusionMask", this->occlusionMaskMaterial, this->occlusionMaskPass))
        {
            return false;
        }
        if (!loadPass("Postprocess/LightShaftsRadialBlur", this->radialBlurMaterial, this->radialBlurPass))
        {
            return false;
        }
        if (!loadPass("Postprocess/LightShaftsBlend", this->blendMaterial, this->blendPass))
        {
            return false;
        }

        // Push initial GPU state
        this->setOcclusionDepthThreshold(this->occlusionDepthThreshold->getReal());
        this->setSunRadius(this->sunRadius->getReal());
        this->setBrightnessThreshold(this->brightnessThreshold->getReal());
        this->setDecay(this->decay->getReal());
        this->setDensity(this->density->getReal());
        this->setExposure(this->exposure->getReal());
        this->setShaftSharpness(this->shaftSharpness->getReal());
        this->setShaftStrength(this->shaftStrength->getReal());
        this->setTint(this->tint->getVector3());

        return success;
    }

    bool CompositorEffectLightShaftsComponent::connect(void)
    {
        CompositorEffectBaseComponent::connect();

        this->resolveLight();

        return true;
    }

    bool CompositorEffectLightShaftsComponent::disconnect(void)
    {
        CompositorEffectBaseComponent::disconnect();

        return true;
    }

    void CompositorEffectLightShaftsComponent::onRemoveComponent(void)
    {
        CompositorEffectBaseComponent::onRemoveComponent();

        Ogre::String closureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);
    }

    void CompositorEffectLightShaftsComponent::actualizeValue(Variant* attribute)
    {
        CompositorEffectBaseComponent::actualizeValue(attribute);

        const Ogre::String& name = attribute->getName();

        if (name == AttrLightId())
        {
            this->setLightId(attribute->getULong());
        }
        else if (name == AttrOcclusionDepthThreshold())
        {
            this->setOcclusionDepthThreshold(attribute->getReal());
        }
        else if (name == AttrSunRadius())
        {
            this->setSunRadius(attribute->getReal());
        }
        else if (name == AttrBrightnessThreshold())
        {
            this->setBrightnessThreshold(attribute->getReal());
        }
        else if (name == AttrDecay())
        {
            this->setDecay(attribute->getReal());
        }
        else if (name == AttrDensity())
        {
            this->setDensity(attribute->getReal());
        }
        else if (name == AttrExposure())
        {
            this->setExposure(attribute->getReal());
        }
        else if (name == AttrShaftSharpness())
        {
            this->setShaftSharpness(attribute->getReal());
        }
        else if (name == AttrShaftStrength())
        {
            this->setShaftStrength(attribute->getReal());
        }
        else if (name == AttrTint())
        {
            this->setTint(attribute->getVector3());
        }
    }

    void CompositorEffectLightShaftsComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

        auto writeULong = [&](const char* name, unsigned long value)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "2"));
            p->append_attribute(doc.allocate_attribute("name", name));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(p);
        };
        auto writeReal = [&](const char* name, Ogre::Real value)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "6"));
            p->append_attribute(doc.allocate_attribute("name", name));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(p);
        };
        auto writeVec3 = [&](const char* name, const Ogre::Vector3& value)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "9"));
            p->append_attribute(doc.allocate_attribute("name", name));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(p);
        };

        writeULong("Light Id", this->lightId->getULong());
        writeReal("Occlusion Depth Threshold", this->occlusionDepthThreshold->getReal());
        writeReal("Sun Radius", this->sunRadius->getReal());
        writeReal("Brightness Threshold", this->brightnessThreshold->getReal());
        writeReal("Decay", this->decay->getReal());
        writeReal("Density", this->density->getReal());
        writeReal("Exposure", this->exposure->getReal());
        writeReal("Shaft Sharpness", this->shaftSharpness->getReal());
        writeReal("Shaft Strength", this->shaftStrength->getReal());
        writeVec3("Tint", this->tint->getVector3());
    }

    Ogre::String CompositorEffectLightShaftsComponent::getClassName(void) const
    {
        return "CompositorEffectLightShaftsComponent";
    }

    Ogre::String CompositorEffectLightShaftsComponent::getParentClassName(void) const
    {
        return "CompositorEffectBaseComponent";
    }

    // -----------------------------------------------------------------------
    // Per-frame update — identical sun projection logic as VolumetricLight
    // -----------------------------------------------------------------------

    void CompositorEffectLightShaftsComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }
        if (nullptr == this->occlusionMaskPass)
        {
            return;
        }
        if (nullptr == this->workspaceBaseComponent)
        {
            return;
        }
        if (nullptr == this->camera)
        {
            return;
        }

        Ogre::Pass* maskPass = this->occlusionMaskPass;
        Ogre::Pass* blurPass = this->radialBlurPass;
        Ogre::Light* light = this->sunLight;
        Ogre::Camera* camera = this->camera;

        auto closureFunction = [maskPass, blurPass, light, camera](Ogre::Real /*renderDt*/)
        {
            Ogre::Vector3 sunDir = light->getDerivedDirectionUpdated().normalisedCopy();
            Ogre::Vector3 sunWorldPos = camera->getDerivedPosition() + (-sunDir) * 100000.0f;

            Ogre::Matrix4 viewProj = camera->getProjectionMatrixWithRSDepth() * camera->getViewMatrix();

            Ogre::Vector4 clipPos = viewProj * Ogre::Vector4(sunWorldPos.x, sunWorldPos.y, sunWorldPos.z, 1.0f);

            Ogre::Vector2 sunScreenPos;

            if (clipPos.w > 0.0001f)
            {
                // Sun is in front of the camera — standard perspective divide
                sunScreenPos.x = (clipPos.x / clipPos.w) * 0.5f + 0.5f;
                sunScreenPos.y = 1.0f - ((clipPos.y / clipPos.w) * 0.5f + 0.5f);
            }
            else
            {
                // Sun is behind the camera (clipPos.w <= 0).
                // We cannot do a perspective divide, but clipPos.xy still encodes
                // a 2D screen-space direction toward where the sun "would be".
                // Push sunScreenPos just past the nearest screen edge (0.55 = 10%
                // outside [0,1]) so the radial blur still marches in the correct
                // direction but terminates quickly.  No shafts appear because
                // the occlusion mask is all-black when looking away from the sun.
                Ogre::Vector2 screenDir(clipPos.x, -clipPos.y);
                const float len = screenDir.length();
                if (len > 0.0001f)
                {
                    screenDir /= len;
                    // 0.55 places the virtual sun just outside the nearest edge
                    sunScreenPos = Ogre::Vector2(0.5f + screenDir.x * 0.55f, 0.5f + screenDir.y * 0.55f);
                }
                else
                {
                    // Directly overhead or directly behind — park it off-screen
                    sunScreenPos = Ogre::Vector2(-1.0f, -1.0f);
                }
            }

            maskPass->getFragmentProgramParameters()->setNamedConstant("sunScreenPos", sunScreenPos);
            blurPass->getFragmentProgramParameters()->setNamedConstant("sunScreenPos", sunScreenPos);
        };

        Ogre::String closureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closureFunction, false);
    }

    // -----------------------------------------------------------------------
    // Light resolution — identical to VolumetricLight
    // -----------------------------------------------------------------------

    void CompositorEffectLightShaftsComponent::resolveLight(void)
    {
        this->sunLight = nullptr;

        unsigned long id = this->lightId->getULong();
        if (id == 0)
        {
            id = GameObjectController::MAIN_LIGHT_ID;
        }

        GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);

        if (nullptr == lightGameObjectPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[CompositorEffectLightShaftsComponent] Could not find game object for light id: " + Ogre::StringConverter::toString(id) + "  game object: " + this->gameObjectPtr->getName());
            return;
        }

        auto lightCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());

        if (nullptr != lightCompPtr)
        {
            this->sunLight = lightCompPtr->getOgreLight();
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectLightShaftsComponent] No LightDirectionalComponent on id: " + Ogre::StringConverter::toString(id));
        }
    }

    // -----------------------------------------------------------------------
    // Setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectLightShaftsComponent::setLightId(unsigned long lightId)
    {
        this->lightId->setValue(lightId);
        this->resolveLight();
    }
    unsigned long CompositorEffectLightShaftsComponent::getLightId(void) const
    {
        return this->lightId->getULong();
    }

    // -----------------------------------------------------------------------
    // Pass 1 — Occlusion Mask setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectLightShaftsComponent::setOcclusionDepthThreshold(Ogre::Real threshold)
    {
        this->occlusionDepthThreshold->setValue(threshold);
        if (nullptr != this->occlusionMaskPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setOcclusionDepthThreshold", _1(threshold),
                { this->occlusionMaskPass->getFragmentProgramParameters()->setNamedConstant("occlusionDepthThreshold", threshold); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getOcclusionDepthThreshold(void) const
    {
        return this->occlusionDepthThreshold->getReal();
    }

    void CompositorEffectLightShaftsComponent::setSunRadius(Ogre::Real radius)
    {
        this->sunRadius->setValue(radius);
        if (nullptr != this->occlusionMaskPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setSunRadius", _1(radius), { this->occlusionMaskPass->getFragmentProgramParameters()->setNamedConstant("sunRadius", radius); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getSunRadius(void) const
    {
        return this->sunRadius->getReal();
    }

    void CompositorEffectLightShaftsComponent::setBrightnessThreshold(Ogre::Real threshold)
    {
        this->brightnessThreshold->setValue(threshold);
        if (nullptr != this->occlusionMaskPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setBrightnessThreshold", _1(threshold), { this->occlusionMaskPass->getFragmentProgramParameters()->setNamedConstant("brightnessThreshold", threshold); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getBrightnessThreshold(void) const
    {
        return this->brightnessThreshold->getReal();
    }

    // -----------------------------------------------------------------------
    // Pass 2 — Radial Blur setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectLightShaftsComponent::setDecay(Ogre::Real decay)
    {
        this->decay->setValue(decay);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setDecay", _1(decay), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("decay", decay); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getDecay(void) const
    {
        return this->decay->getReal();
    }

    void CompositorEffectLightShaftsComponent::setDensity(Ogre::Real density)
    {
        this->density->setValue(density);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setDensity", _1(density), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("density", density); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getDensity(void) const
    {
        return this->density->getReal();
    }

    void CompositorEffectLightShaftsComponent::setExposure(Ogre::Real exposure)
    {
        this->exposure->setValue(exposure);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setExposure", _1(exposure), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("exposure", exposure); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getExposure(void) const
    {
        return this->exposure->getReal();
    }

    void CompositorEffectLightShaftsComponent::setShaftSharpness(Ogre::Real sharpness)
    {
        this->shaftSharpness->setValue(sharpness);
        if (nullptr != this->radialBlurPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setShaftSharpness", _1(sharpness), { this->radialBlurPass->getFragmentProgramParameters()->setNamedConstant("shaftSharpness", sharpness); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getShaftSharpness(void) const
    {
        return this->shaftSharpness->getReal();
    }

    // -----------------------------------------------------------------------
    // Pass 3 — Blend setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectLightShaftsComponent::setShaftStrength(Ogre::Real strength)
    {
        this->shaftStrength->setValue(strength);
        if (nullptr != this->blendPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setShaftStrength", _1(strength), { this->blendPass->getFragmentProgramParameters()->setNamedConstant("shaftStrength", strength); });
        }
    }

    Ogre::Real CompositorEffectLightShaftsComponent::getShaftStrength(void) const
    {
        return this->shaftStrength->getReal();
    }

    void CompositorEffectLightShaftsComponent::setTint(const Ogre::Vector3& tint)
    {
        this->tint->setValue(tint);
        if (nullptr != this->blendPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectLightShaftsComponent::setTint", _1(tint), { this->blendPass->getFragmentProgramParameters()->setNamedConstant("tint", tint); });
        }
    }

    Ogre::Vector3 CompositorEffectLightShaftsComponent::getTint(void) const
    {
        return this->tint->getVector3();
    }

    CompositorEffectLightShaftsComponent* getCompositorEffectLightShaftsComponent(GameObject* gameObject)
    {
        return makeStrongPtr<CompositorEffectLightShaftsComponent>(gameObject->getComponent<CompositorEffectLightShaftsComponent>()).get();
    }

    CompositorEffectLightShaftsComponent* getCompositorEffectLightShaftsComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<CompositorEffectLightShaftsComponent>(gameObject->getComponentFromName<CompositorEffectLightShaftsComponent>(name)).get();
    }

    void CompositorEffectLightShaftsComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)
        [
            luabind::class_<CompositorEffectLightShaftsComponent, GameObjectComponent>("CompositorEffectLightShaftsComponent")

            .def("setLightId", &CompositorEffectLightShaftsComponent::setLightId)
            .def("getLightId", &CompositorEffectLightShaftsComponent::getLightId)

            // Pass 1
            .def("setOcclusionDepthThreshold", &CompositorEffectLightShaftsComponent::setOcclusionDepthThreshold)
            .def("getOcclusionDepthThreshold", &CompositorEffectLightShaftsComponent::getOcclusionDepthThreshold)
            .def("setSunRadius", &CompositorEffectLightShaftsComponent::setSunRadius)
            .def("getSunRadius", &CompositorEffectLightShaftsComponent::getSunRadius)
            .def("setBrightnessThreshold", &CompositorEffectLightShaftsComponent::setBrightnessThreshold)
            .def("getBrightnessThreshold", &CompositorEffectLightShaftsComponent::getBrightnessThreshold)

            // Pass 2
            .def("setDecay", &CompositorEffectLightShaftsComponent::setDecay)
            .def("getDecay", &CompositorEffectLightShaftsComponent::getDecay)
            .def("setDensity", &CompositorEffectLightShaftsComponent::setDensity)
            .def("getDensity", &CompositorEffectLightShaftsComponent::getDensity)
            .def("setExposure", &CompositorEffectLightShaftsComponent::setExposure)
            .def("getExposure", &CompositorEffectLightShaftsComponent::getExposure)
            .def("setShaftSharpness", &CompositorEffectLightShaftsComponent::setShaftSharpness)
            .def("getShaftSharpness", &CompositorEffectLightShaftsComponent::getShaftSharpness)

            // Pass 3
            .def("setShaftStrength", &CompositorEffectLightShaftsComponent::setShaftStrength)
            .def("getShaftStrength", &CompositorEffectLightShaftsComponent::getShaftStrength)

            .def("setTint", &CompositorEffectLightShaftsComponent::setTint)
            .def("getTint", &CompositorEffectLightShaftsComponent::getTint)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "class inherits GameObjectComponent", getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setLightId(string lightId)", "Sets the light game object id used as the light shaft source.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "string getLightId()", "Gets the current light shaft source game object id.");

        // Pass 1
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setOcclusionDepthThreshold(float threshold)", "Sets the depth threshold used for occlusion masking.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getOcclusionDepthThreshold()", "Gets the occlusion depth threshold.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setSunRadius(float radius)", "Sets the apparent size of the light source.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getSunRadius()", "Gets the apparent light source size.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setBrightnessThreshold(float threshold)", "Sets the minimum brightness required for shaft generation.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getBrightnessThreshold()", "Gets the brightness threshold.");

        // Pass 2
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setDecay(float decay)", "Controls how quickly the light shafts fade over distance.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getDecay()", "Gets the shaft decay factor.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setDensity(float density)", "Controls the sampling density of the light shaft effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getDensity()", "Gets the shaft sampling density.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setExposure(float exposure)", "Controls the exposure multiplier of the light shafts.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getExposure()", "Gets the shaft exposure multiplier.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setShaftSharpness(float sharpness)", "Controls how sharp or blurred the light shafts appear.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getShaftSharpness()", "Gets the shaft sharpness factor.");

        // Pass 3
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setShaftStrength(float strength)", "Controls the final intensity of the blended light shafts.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "float getShaftStrength()", "Gets the final shaft intensity.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "void setTint(Vector3 tint)", "Sets the RGB tint color of the light shafts.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectLightShaftsComponent", "Vector3 getTint()", "Gets the RGB tint color.");

        gameObjectClass.def("getCompositorEffectLightShaftsComponent", &getCompositorEffectLightShaftsComponent);
        gameObjectClass.def("getCompositorEffectLightShaftsComponentFromName", &getCompositorEffectLightShaftsComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectLightShaftsComponent getCompositorEffectLightShaftsComponent()", "Gets the CompositorEffectLightShaftsComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectLightShaftsComponent getCompositorEffectLightShaftsComponentFromName(string name)", "Gets the CompositorEffectLightShaftsComponent by name.");
        gameObjectControllerClass.def("castCompositorEffectLightShaftsComponent", &GameObjectController::cast<CompositorEffectLightShaftsComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CompositorEffectLightShaftsComponent castCompositorEffectLightShaftsComponent(CompositorEffectLightShaftsComponent other)", "Casts for Lua auto completion.");
    }

	///////////////////////////////////////////////////////////////////////////////////////////////

    CompositorEffectDepthOfFieldComponent::CompositorEffectDepthOfFieldComponent() :
        CompositorEffectBaseComponent(),
        cocPass(nullptr),
        blurPass0(nullptr),
        blurPass1(nullptr),
        blurPass2(nullptr),
        blendPass(nullptr),
        dofType(new Variant(CompositorEffectDepthOfFieldComponent::AttrDofType(), std::vector<Ogre::String>({"Depth Of Field Gaussian", "Depth Of Field Hex Bokeh"}), this->attributes)),
        focusGameObjectId(new Variant(CompositorEffectDepthOfFieldComponent::AttrFocusGameObjectId(), static_cast<unsigned long>(0), this->attributes)),
        focusDistance(new Variant(CompositorEffectDepthOfFieldComponent::AttrFocusDistance(), 20.0f, this->attributes)),
        nearBlurRange(new Variant(CompositorEffectDepthOfFieldComponent::AttrNearBlurRange(), 5.0f, this->attributes)),
        farBlurRange(new Variant(CompositorEffectDepthOfFieldComponent::AttrFarBlurRange(), 30.0f, this->attributes)),
        blurRadius(new Variant(CompositorEffectDepthOfFieldComponent::AttrBlurRadius(), 0.012f, this->attributes)),
        blendStrength(new Variant(CompositorEffectDepthOfFieldComponent::AttrBlendStrength(), 1.0f, this->attributes))
    {
        this->effectName = "Depth Of Field Gaussian";

        this->focusDistance->setConstraints(0.1f, 10000.0f);
        this->nearBlurRange->setConstraints(0.1f, 500.0f);
        this->farBlurRange->setConstraints(0.1f, 5000.0f);
        this->blurRadius->setConstraints(0.001f, 0.1f);
        this->blendStrength->setConstraints(0.0f, 1.0f);

        this->focusDistance->setDescription("Defines the camera distance where the image remains perfectly sharp.");
        this->nearBlurRange->setDescription("Controls how quickly objects in front of the focus distance become blurred.");
        this->farBlurRange->setDescription("Controls how quickly objects behind the focus distance become blurred.");
        this->blurRadius->setDescription("Controls the blur kernel radius used for out of focus image regions.");
        this->blendStrength->setDescription("Controls the final blending intensity between sharp and blurred image areas.");
    }

    CompositorEffectDepthOfFieldComponent::~CompositorEffectDepthOfFieldComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectDepthOfFieldComponent] Destructor for game object: " + this->gameObjectPtr->getName());

        this->cocPass = nullptr;
        this->blurPass0 = nullptr;
        this->blurPass1 = nullptr;
        this->blurPass2 = nullptr;
        this->blendPass = nullptr;
    }

    bool CompositorEffectDepthOfFieldComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = CompositorEffectBaseComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDofType())
        {
            this->setDofType(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFocusGameObjectId())
        {
            this->focusGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFocusDistance())
        {
            this->setFocusDistance(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrNearBlurRange())
        {
            this->setNearBlurRange(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFarBlurRange())
        {
            this->setFarBlurRange(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBlurRadius())
        {
            this->setBlurRadius(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBlendStrength())
        {
            this->setBlendStrength(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr CompositorEffectDepthOfFieldComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool CompositorEffectDepthOfFieldComponent::postInit(void)
    {
        bool success = CompositorEffectBaseComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectDepthOfFieldComponent] Init for game object: " + this->gameObjectPtr->getName());

        if (!this->resolvePasses())
        {
            return false;
        }

        // Push all tunable values
        this->setFocusDistance(this->focusDistance->getReal());
        this->setNearBlurRange(this->nearBlurRange->getReal());
        this->setFarBlurRange(this->farBlurRange->getReal());
        this->setBlurRadius(this->blurRadius->getReal());
        this->setBlendStrength(this->blendStrength->getReal());

        return success;
    }

    bool CompositorEffectDepthOfFieldComponent::connect(void)
    {
        CompositorEffectBaseComponent::connect();

        // Push camera-derived projection params once — only change if clip planes change
        if (nullptr != this->workspaceBaseComponent)
        {
            if (nullptr == this->camera)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectDepthOfFieldComponent] Could not find camera, hence effect cannot be enabled for game object: " + this->gameObjectPtr->getName());
                return true;
            }

            Ogre::Vector2 projAB = this->camera->getProjectionParamsAB();
            projAB.y /= this->camera->getFarClipDistance();
            const float farClip = this->camera->getFarClipDistance();
  

            Ogre::Pass* coc = this->cocPass;
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectDepthOfFieldComponent::postInit_proj", _3(projAB, farClip, coc), {
                auto* p = coc->getFragmentProgramParameters().get();
                p->setNamedConstant("projectionParams", projAB);
                p->setNamedConstant("farClipDistance", farClip);
            });
        }

        return true;
    }

    bool CompositorEffectDepthOfFieldComponent::disconnect(void)
    {
        CompositorEffectBaseComponent::disconnect();

        return true;
    }

    void CompositorEffectDepthOfFieldComponent::onRemoveComponent(void)
    {
        CompositorEffectBaseComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
    }

    void CompositorEffectDepthOfFieldComponent::actualizeValue(Variant* attribute)
    {
        CompositorEffectBaseComponent::actualizeValue(attribute);

        const Ogre::String& name = attribute->getName();

        if (name == AttrDofType())
        {
            this->setDofType(attribute->getListSelectedValue());
        }
        else if (name == AttrFocusGameObjectId())
        {
            this->setFocusGameObjectId(attribute->getULong());
        }
        else if (name == AttrFocusDistance())
        {
            this->setFocusDistance(attribute->getReal());
        }
        else if (name == AttrNearBlurRange())
        {
            this->setNearBlurRange(attribute->getReal());
        }
        else if (name == AttrFarBlurRange())
        {
            this->setFarBlurRange(attribute->getReal());
        }
        else if (name == AttrBlurRadius())
        {
            this->setBlurRadius(attribute->getReal());
        }
        else if (name == AttrBlendStrength())
        {
            this->setBlendStrength(attribute->getReal());
        }
    }

    void CompositorEffectDepthOfFieldComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

        auto writeString = [&](const char* n, const Ogre::String& v)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "7"));
            p->append_attribute(doc.allocate_attribute("name", n));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(p);
        };
        auto writeInt = [&](const char* n, int v)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "2"));
            p->append_attribute(doc.allocate_attribute("name", n));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(p);
        };
        auto writeULong = [&](const char* n, unsigned long v)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "2"));
            p->append_attribute(doc.allocate_attribute("name", n));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(p);
        };
        auto writeReal = [&](const char* n, Ogre::Real v)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "6"));
            p->append_attribute(doc.allocate_attribute("name", n));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(p);
        };

        writeString("Dof Type", this->dofType->getListSelectedValue());
        writeULong("Focus Game Object Id", this->focusGameObjectId->getULong());
        writeReal("Focus Distance", this->focusDistance->getReal());
        writeReal("Near Blur Range", this->nearBlurRange->getReal());
        writeReal("Far Blur Range", this->farBlurRange->getReal());
        writeReal("Blur Radius", this->blurRadius->getReal());
        writeReal("Blend Strength", this->blendStrength->getReal());
    }

    Ogre::String CompositorEffectDepthOfFieldComponent::getClassName(void) const
    {
        return "CompositorEffectDepthOfFieldComponent";
    }

    Ogre::String CompositorEffectDepthOfFieldComponent::getParentClassName(void) const
    {
        return "CompositorEffectBaseComponent";
    }

    // -----------------------------------------------------------------------
    // Per-frame update — focus tracking + depth params
    // -----------------------------------------------------------------------

    void CompositorEffectDepthOfFieldComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }
        if (nullptr == this->cocPass)
        {
            return;
        }
        if (nullptr == this->workspaceBaseComponent)
        {
            return;
        }
        if (nullptr == this->camera)
        {
            return;
        }

        // Resolve auto-focus distance if a game object is tracked
        Ogre::Real currentFocusDist = this->focusDistance->getReal();

        unsigned long trackId = this->focusGameObjectId->getULong();
        if (trackId != 0)
        {
            GameObjectPtr focusObj = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(trackId);

            if (nullptr != focusObj && nullptr != this->workspaceBaseComponent)
            {
                Ogre::Vector3 diff = focusObj->getSceneNode()->_getDerivedPosition() - this->camera->getDerivedPosition();
                currentFocusDist = diff.length();
            }
        }

        Ogre::Pass* coc = this->cocPass;
        const Ogre::Real dist = currentFocusDist;
        Ogre::Camera* camera = this->camera;

        auto closureFunction = [coc, camera, dist](Ogre::Real)
        {
            Ogre::Vector2 projAB = camera->getProjectionParamsAB();
            projAB.y /= camera->getFarClipDistance();

            auto* params = coc->getFragmentProgramParameters().get();
            params->setNamedConstant("projectionParams", projAB);
            params->setNamedConstant("farClipDistance", camera->getFarClipDistance());
            params->setNamedConstant("focusDistance", dist);
        };

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
    }

    // -----------------------------------------------------------------------
    // Pass resolution
    // -----------------------------------------------------------------------

    bool CompositorEffectDepthOfFieldComponent::resolvePasses(void)
    {
        this->cocPass = nullptr;
        this->blurPass0 = nullptr;
        this->blurPass1 = nullptr;
        this->blurPass2 = nullptr;
        this->blendPass = nullptr;

        Ogre::String type = this->dofType->getListSelectedValue();

        auto loadPass = [this](const Ogre::String& mat, Ogre::MaterialPtr& ptr, Ogre::Pass*& pass) -> bool
        {
            ptr = Ogre::MaterialManager::getSingletonPtr()->getByName(mat);
            if (ptr.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectDepthOfFieldComponent] Material not found: '" + mat + "'");
                return false;
            }
            pass = ptr->getTechnique(0)->getPass(0);
            return true;
        };
        if (type == "Depth Of Field Gaussian")
        {
            if (!loadPass("Postprocess/DOFGaussianCoC", this->cocMaterial, this->cocPass))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFGaussianBlurH", this->blurMaterial0, this->blurPass0))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFGaussianBlurV", this->blurMaterial1, this->blurPass1))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFGaussianBlend", this->blendMaterial, this->blendPass))
            {
                return false;
            }
            this->blurPass2 = nullptr; // unused for Gaussian
        }
        else // HexBokeh
        {
            if (!loadPass("Postprocess/DOFHexBokehCoC", this->cocMaterial, this->cocPass))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFHexBokehDir0", this->blurMaterial0, this->blurPass0))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFHexBokehDir1", this->blurMaterial1, this->blurPass1))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFHexBokehDir2", this->blurMaterial2, this->blurPass2))
            {
                return false;
            }
            if (!loadPass("Postprocess/DOFHexBokehBlend", this->blendMaterial, this->blendPass))
            {
                return false;
            }
        }

        return true;
    }

    // -----------------------------------------------------------------------
    // Setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectDepthOfFieldComponent::setDofType(const Ogre::String& dofType)
    {
        this->dofType->setListSelectedValue(dofType);

        // Update the node name so enableEffect() targets the correct node
        this->effectName = dofType;

        // Re-resolve material pointers for the new type
        if (nullptr != this->cocPass) // i.e., postInit has already run
        {
            this->resolvePasses();
            this->setFocusDistance(this->focusDistance->getReal());
            this->setNearBlurRange(this->nearBlurRange->getReal());
            this->setFarBlurRange(this->farBlurRange->getReal());
            this->setBlurRadius(this->blurRadius->getReal());
            this->setBlendStrength(this->blendStrength->getReal());
        }
    }

    Ogre::String CompositorEffectDepthOfFieldComponent::getDofType(void) const
    {
        return this->dofType->getListSelectedValue();
    }

    void CompositorEffectDepthOfFieldComponent::setFocusGameObjectId(unsigned long id)
    {
        this->focusGameObjectId->setValue(id);
    }
    unsigned long CompositorEffectDepthOfFieldComponent::getFocusGameObjectId(void) const
    {
        return this->focusGameObjectId->getULong();
    }

    // CoC pass setters — push to both Gaussian CoC and Hex CoC materials

    void CompositorEffectDepthOfFieldComponent::setFocusDistance(Ogre::Real dist)
    {
        this->focusDistance->setValue(dist);
        if (nullptr != this->cocPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectDepthOfFieldComponent::setFocusDistance", _1(dist), { this->cocPass->getFragmentProgramParameters()->setNamedConstant("focusDistance", dist); });
        }
    }
    Ogre::Real CompositorEffectDepthOfFieldComponent::getFocusDistance(void) const
    {
        return this->focusDistance->getReal();
    }

    void CompositorEffectDepthOfFieldComponent::setNearBlurRange(Ogre::Real range)
    {
        this->nearBlurRange->setValue(range);
        if (nullptr != this->cocPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectDepthOfFieldComponent::setNearBlurRange", _1(range), { this->cocPass->getFragmentProgramParameters()->setNamedConstant("nearBlurRange", range); });
        }
    }
    Ogre::Real CompositorEffectDepthOfFieldComponent::getNearBlurRange(void) const
    {
        return this->nearBlurRange->getReal();
    }

    void CompositorEffectDepthOfFieldComponent::setFarBlurRange(Ogre::Real range)
    {
        this->farBlurRange->setValue(range);
        if (nullptr != this->cocPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectDepthOfFieldComponent::setFarBlurRange", _1(range), { this->cocPass->getFragmentProgramParameters()->setNamedConstant("farBlurRange", range); });
        }
    }
    Ogre::Real CompositorEffectDepthOfFieldComponent::getFarBlurRange(void) const
    {
        return this->farBlurRange->getReal();
    }

    // Blur pass setters — fan out to all active blur passes

    void CompositorEffectDepthOfFieldComponent::setBlurRadius(Ogre::Real radius)
    {
        this->blurRadius->setValue(radius);

        if (nullptr != this->blurPass0)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectDepthOfFieldComponent::setBlurRadius", _1(radius),
            {
                if (this->blurPass0)
                {
                    this->blurPass0->getFragmentProgramParameters()->setNamedConstant("blurRadius", radius);
                }

                if (this->blurPass1)
                {
                    this->blurPass1->getFragmentProgramParameters()->setNamedConstant("blurRadius", radius);
                }

                if (this->blurPass2)
                {
                    this->blurPass2->getFragmentProgramParameters()->setNamedConstant("blurRadius", radius);
                }
            });
        }
    }
    Ogre::Real CompositorEffectDepthOfFieldComponent::getBlurRadius(void) const
    {
        return this->blurRadius->getReal();
    }

    void CompositorEffectDepthOfFieldComponent::setBlendStrength(Ogre::Real strength)
    {
        this->blendStrength->setValue(strength);
        if (nullptr != this->blendPass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectDepthOfFieldComponent::setBlendStrength", _1(strength), { this->blendPass->getFragmentProgramParameters()->setNamedConstant("blendStrength", strength); });
        }
    }
    Ogre::Real CompositorEffectDepthOfFieldComponent::getBlendStrength(void) const
    {
        return this->blendStrength->getReal();
    }

    CompositorEffectDepthOfFieldComponent* getCompositorEffectDepthOfFieldComponent(GameObject* gameObject)
    {
        return makeStrongPtr<CompositorEffectDepthOfFieldComponent>(gameObject->getComponent<CompositorEffectDepthOfFieldComponent>()).get();
    }

    CompositorEffectDepthOfFieldComponent* getCompositorEffectDepthOfFieldComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<CompositorEffectDepthOfFieldComponent>(gameObject->getComponentFromName<CompositorEffectDepthOfFieldComponent>(name)).get();
    }

    void CompositorEffectDepthOfFieldComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)
        [
            luabind::class_<CompositorEffectDepthOfFieldComponent, GameObjectComponent>("CompositorEffectDepthOfFieldComponent")
            .def("setDofType", &CompositorEffectDepthOfFieldComponent::setDofType)
            .def("getDofType", &CompositorEffectDepthOfFieldComponent::getDofType)
            .def("setFocusGameObjectId", &CompositorEffectDepthOfFieldComponent::setFocusGameObjectId)
            .def("getFocusGameObjectId", &CompositorEffectDepthOfFieldComponent::getFocusGameObjectId)
            .def("setFocusDistance", &CompositorEffectDepthOfFieldComponent::setFocusDistance)
            .def("getFocusDistance", &CompositorEffectDepthOfFieldComponent::getFocusDistance)
            .def("setNearBlurRange", &CompositorEffectDepthOfFieldComponent::setNearBlurRange)
            .def("getNearBlurRange", &CompositorEffectDepthOfFieldComponent::getNearBlurRange)
            .def("setFarBlurRange", &CompositorEffectDepthOfFieldComponent::setFarBlurRange)
            .def("getFarBlurRange", &CompositorEffectDepthOfFieldComponent::getFarBlurRange)
            .def("setBlurRadius", &CompositorEffectDepthOfFieldComponent::setBlurRadius)
            .def("getBlurRadius", &CompositorEffectDepthOfFieldComponent::getBlurRadius)
            .def("setBlendStrength", &CompositorEffectDepthOfFieldComponent::setBlendStrength)
            .def("getBlendStrength", &CompositorEffectDepthOfFieldComponent::getBlendStrength)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "class inherits GameObjectComponent", getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setDofType(string dofType)", "Sets the depth of field mode or algorithm type.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "string getDofType()", "Gets the current depth of field mode.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setFocusGameObjectId(string id)", "Sets the game object id used as the dynamic focus target.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "string getFocusGameObjectId()", "Gets the current focus target game object id.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setFocusDistance(float distance)", "Sets the camera distance where the image remains sharp.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "float getFocusDistance()", "Gets the focus distance.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setNearBlurRange(float range)", "Sets the blur transition range in front of the focus distance.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "float getNearBlurRange()", "Gets the near blur transition range.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setFarBlurRange(float range)", "Sets the blur transition range behind the focus distance.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "float getFarBlurRange()", "Gets the far blur transition range.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setBlurRadius(float radius)", "Sets the blur kernel radius used for out of focus areas.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "float getBlurRadius()", "Gets the blur kernel radius.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "void setBlendStrength(float strength)", "Controls the blending intensity between sharp and blurred image regions.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectDepthOfFieldComponent", "float getBlendStrength()", "Gets the final blur blend strength.");

        gameObjectClass.def("getCompositorEffectDepthOfFieldComponent", &getCompositorEffectDepthOfFieldComponent);
        gameObjectClass.def("getCompositorEffectDepthOfFieldComponentFromName", &getCompositorEffectDepthOfFieldComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectDepthOfFieldComponent getCompositorEffectDepthOfFieldComponent()", "Gets the CompositorEffectDepthOfFieldComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectDepthOfFieldComponent getCompositorEffectDepthOfFieldComponentFromName(string name)", "Gets the CompositorEffectDepthOfFieldComponent by name.");
        gameObjectControllerClass.def("castCompositorEffectDepthOfFieldComponent", &GameObjectController::cast<CompositorEffectDepthOfFieldComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CompositorEffectDepthOfFieldComponent castCompositorEffectDepthOfFieldComponent(CompositorEffectDepthOfFieldComponent other)", "Casts for Lua auto completion.");
    }

	///////////////////////////////////////////////////////////////////////////////////////////////

    CompositorEffectOutlineComponent::CompositorEffectOutlineComponent() :
        CompositorEffectBaseComponent(),
        outlinePass(nullptr),
        outlineColor(new Variant(CompositorEffectOutlineComponent::AttrOutlineColor(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes)),
        outlineThickness(new Variant(CompositorEffectOutlineComponent::AttrOutlineThickness(), 1.0f, this->attributes)),
        depthThreshold(new Variant(CompositorEffectOutlineComponent::AttrDepthThreshold(), 0.001f, this->attributes)),
        outlineStrength(new Variant(CompositorEffectOutlineComponent::AttrOutlineStrength(), 1.0f, this->attributes))
    {
        this->effectName = "Outline";

        this->outlineThickness->setConstraints(0.5f, 5.0f);
        this->depthThreshold->setConstraints(0.0001f, 0.05f);
        this->outlineStrength->setConstraints(0.0f, 1.0f);

        this->outlineThickness->setDescription("Controls the thickness of the geometry outlines in pixels.");
        this->depthThreshold->setDescription("Defines the minimum depth difference required to detect an outline edge. Lower values detect more small geometry details.");
        this->outlineStrength->setDescription("Controls the final opacity and intensity of the outline effect.");
    }

    CompositorEffectOutlineComponent::~CompositorEffectOutlineComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectOutlineComponent] Destructor for game object: " + this->gameObjectPtr->getName());

        this->outlinePass = nullptr;
    }

    bool CompositorEffectOutlineComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = CompositorEffectBaseComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOutlineColor())
        {
            this->setOutlineColor(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOutlineThickness())
        {
            this->setOutlineThickness(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDepthThreshold())
        {
            this->setDepthThreshold(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOutlineStrength())
        {
            this->setOutlineStrength(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr CompositorEffectOutlineComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        CompositorEffectOutlineCompPtr clonedCompPtr(boost::make_shared<CompositorEffectOutlineComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setOutlineColor(this->outlineColor->getVector3());
        clonedCompPtr->setOutlineThickness(this->outlineThickness->getReal());
        clonedCompPtr->setDepthThreshold(this->depthThreshold->getReal());
        clonedCompPtr->setOutlineStrength(this->outlineStrength->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool CompositorEffectOutlineComponent::postInit(void)
    {
        bool success = CompositorEffectBaseComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectOutlineComponent] Init for game object: " + this->gameObjectPtr->getName());

        const Ogre::String matName = "Postprocess/Outline";
        this->outlineMaterial = Ogre::MaterialManager::getSingletonPtr()->getByName(matName);

        if (this->outlineMaterial.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectOutlineComponent] Material not found: '" + matName + "'");
            return false;
        }

        this->outlinePass = this->outlineMaterial->getTechnique(0)->getPass(0);

        // Push tunable values
        this->setOutlineColor(this->outlineColor->getVector3());
        this->setOutlineThickness(this->outlineThickness->getReal());
        this->setDepthThreshold(this->depthThreshold->getReal());
        this->setOutlineStrength(this->outlineStrength->getReal());

        return success;
    }

    void CompositorEffectOutlineComponent::onRemoveComponent(void)
    {
        CompositorEffectBaseComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
    }

    bool CompositorEffectOutlineComponent::connect(void)
    {
        CompositorEffectBaseComponent::connect();

        // Push camera projection params once
        if (nullptr != this->workspaceBaseComponent)
        {
            if (nullptr == this->camera)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectOutlineComponent] Could not find camera, hence effect cannot be enabled for game object: " + this->gameObjectPtr->getName());
                return true;
            }

            Ogre::Vector2 projAB = this->camera->getProjectionParamsAB();
            projAB.y /= this->camera->getFarClipDistance();
            const float farClip = this->camera->getFarClipDistance();

            Ogre::Pass* pass = this->outlinePass;
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOutlineComponent::postInit_proj", _3(pass, projAB, farClip), {
                auto* p = pass->getFragmentProgramParameters().get();
                p->setNamedConstant("projectionParams", projAB);
                p->setNamedConstant("farClipDistance", farClip);
            });
        }

        return true;
    }

    bool CompositorEffectOutlineComponent::disconnect(void)
    {
        CompositorEffectBaseComponent::disconnect();

        return true;
    }

    void CompositorEffectOutlineComponent::actualizeValue(Variant* attribute)
    {
        CompositorEffectBaseComponent::actualizeValue(attribute);

        const Ogre::String& name = attribute->getName();

        if (name == AttrOutlineColor())
        {
            this->setOutlineColor(attribute->getVector3());
        }
        else if (name == AttrOutlineThickness())
        {
            this->setOutlineThickness(attribute->getReal());
        }
        else if (name == AttrDepthThreshold())
        {
            this->setDepthThreshold(attribute->getReal());
        }
        else if (name == AttrOutlineStrength())
        {
            this->setOutlineStrength(attribute->getReal());
        }
    }

    void CompositorEffectOutlineComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        CompositorEffectBaseComponent::writeXML(propertiesXML, doc);

        auto writeReal = [&](const char* n, Ogre::Real v)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "6"));
            p->append_attribute(doc.allocate_attribute("name", n));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(p);
        };
        auto writeVec3 = [&](const char* n, const Ogre::Vector3& v)
        {
            xml_node<>* p = doc.allocate_node(node_element, "property");
            p->append_attribute(doc.allocate_attribute("type", "9"));
            p->append_attribute(doc.allocate_attribute("name", n));
            p->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(p);
        };

        writeVec3("Outline Color", this->outlineColor->getVector3());
        writeReal("Outline Thickness", this->outlineThickness->getReal());
        writeReal("Depth Threshold", this->depthThreshold->getReal());
        writeReal("Outline Strength", this->outlineStrength->getReal());
    }

    Ogre::String CompositorEffectOutlineComponent::getClassName(void) const
    {
        return "CompositorEffectOutlineComponent";
    }

    Ogre::String CompositorEffectOutlineComponent::getParentClassName(void) const
    {
        return "CompositorEffectBaseComponent";
    }

    void CompositorEffectOutlineComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }
        if (nullptr == this->outlinePass)
        {
            return;
        }
        if (nullptr == this->workspaceBaseComponent)
        {
            return;
        }
        if (nullptr == this->camera)
        {
            return;
        }

        Ogre::Pass* pass = this->outlinePass;
        Ogre::Camera* camera = this->camera;

        auto closureFunction = [pass, camera](Ogre::Real)
        {
            Ogre::Vector2 projAB = camera->getProjectionParamsAB();
            projAB.y /= camera->getFarClipDistance();

            auto* p = pass->getFragmentProgramParameters().get();
            p->setNamedConstant("projectionParams", projAB);
            p->setNamedConstant("farClipDistance", camera->getFarClipDistance());
        };

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
    }

    // -----------------------------------------------------------------------
    // Setters / getters
    // -----------------------------------------------------------------------

    void CompositorEffectOutlineComponent::setOutlineColor(const Ogre::Vector3& color)
    {
        this->outlineColor->setValue(color);
        if (nullptr != this->outlinePass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOutlineComponent::setOutlineColor", _1(color), { this->outlinePass->getFragmentProgramParameters()->setNamedConstant("outlineColor", color); });
        }
    }

    Ogre::Vector3 CompositorEffectOutlineComponent::getOutlineColor(void) const
    {
        return this->outlineColor->getVector3();
    }

    void CompositorEffectOutlineComponent::setOutlineThickness(Ogre::Real thickness)
    {
        this->outlineThickness->setValue(thickness);
        if (nullptr != this->outlinePass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOutlineComponent::setOutlineThickness", _1(thickness), { this->outlinePass->getFragmentProgramParameters()->setNamedConstant("outlineThickness", thickness); });
        }
    }

    Ogre::Real CompositorEffectOutlineComponent::getOutlineThickness(void) const
    {
        return this->outlineThickness->getReal();
    }

    void CompositorEffectOutlineComponent::setDepthThreshold(Ogre::Real threshold)
    {
        this->depthThreshold->setValue(threshold);
        if (nullptr != this->outlinePass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOutlineComponent::setDepthThreshold", _1(threshold), { this->outlinePass->getFragmentProgramParameters()->setNamedConstant("depthThreshold", threshold); });
        }
    }

    Ogre::Real CompositorEffectOutlineComponent::getDepthThreshold(void) const
    {
        return this->depthThreshold->getReal();
    }

    void CompositorEffectOutlineComponent::setOutlineStrength(Ogre::Real strength)
    {
        this->outlineStrength->setValue(strength);
        if (nullptr != this->outlinePass)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CompositorEffectOutlineComponent::setOutlineStrength", _1(strength), { this->outlinePass->getFragmentProgramParameters()->setNamedConstant("outlineStrength", strength); });
        }
    }

    Ogre::Real CompositorEffectOutlineComponent::getOutlineStrength(void) const
    {
        return this->outlineStrength->getReal();
    }

    CompositorEffectOutlineComponent* getCompositorEffectOutlineComponent(GameObject* gameObject)
    {
        return makeStrongPtr<CompositorEffectOutlineComponent>(gameObject->getComponent<CompositorEffectOutlineComponent>()).get();
    }

    CompositorEffectOutlineComponent* getCompositorEffectOutlineComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<CompositorEffectOutlineComponent>(gameObject->getComponentFromName<CompositorEffectOutlineComponent>(name)).get();
    }

    void CompositorEffectOutlineComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)
        [
            luabind::class_<CompositorEffectOutlineComponent, GameObjectComponent>("CompositorEffectOutlineComponent")
            .def("setOutlineColor", &CompositorEffectOutlineComponent::setOutlineColor)
            .def("getOutlineColor", &CompositorEffectOutlineComponent::getOutlineColor)
            .def("setOutlineThickness", &CompositorEffectOutlineComponent::setOutlineThickness)
            .def("getOutlineThickness", &CompositorEffectOutlineComponent::getOutlineThickness)
            .def("setDepthThreshold", &CompositorEffectOutlineComponent::setDepthThreshold)
            .def("getDepthThreshold", &CompositorEffectOutlineComponent::getDepthThreshold)
            .def("setOutlineStrength", &CompositorEffectOutlineComponent::setOutlineStrength)
            .def("getOutlineStrength", &CompositorEffectOutlineComponent::getOutlineStrength)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "class inherits GameObjectComponent", getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "void setOutlineColor(Vector3 color)", "Sets the RGB color of the outline strokes.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "Vector3 getOutlineColor()", "Gets the RGB outline color.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "void setOutlineThickness(float thickness)",
            "Sets the Sobel kernel step size in pixels. "
            "Lower values create thin outlines, higher values thicker outlines.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "float getOutlineThickness()", "Gets the outline thickness.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "void setDepthThreshold(float threshold)",
            "Sets the minimum depth gradient required to detect an edge. "
            "Lower values detect more geometry details.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "float getDepthThreshold()", "Gets the depth edge detection threshold.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "void setOutlineStrength(float strength)", "Controls the final opacity and intensity of the outline effect.");
        LuaScriptApi::getInstance()->addClassToCollection("CompositorEffectOutlineComponent", "float getOutlineStrength()", "Gets the final outline intensity.");

        gameObjectClass.def("getCompositorEffectOutlineComponent", &getCompositorEffectOutlineComponent);
        gameObjectClass.def("getCompositorEffectOutlineComponentFromName", &getCompositorEffectOutlineComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectOutlineComponent getCompositorEffectOutlineComponent()", "Gets the CompositorEffectOutlineComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CompositorEffectOutlineComponent getCompositorEffectOutlineComponentFromName(string name)", "Gets the CompositorEffectOutlineComponent by name.");
        gameObjectControllerClass.def("castCompositorEffectOutlineComponent", &GameObjectController::cast<CompositorEffectOutlineComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CompositorEffectOutlineComponent castCompositorEffectOutlineComponent(CompositorEffectOutlineComponent other)", "Casts for Lua auto completion.");
    }

	///////////////////////////////////////////////////////////////////////////////////////////////

}; // namespace end