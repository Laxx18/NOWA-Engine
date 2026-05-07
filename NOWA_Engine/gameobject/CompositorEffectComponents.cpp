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
		CompositorEffectBloomCompPtr clonedCompPtr(boost::make_shared<CompositorEffectBloomComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setImageWeight(this->imageWeight->getReal());
		clonedCompPtr->setBlurWeight(this->blurWeight->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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
#if 0
		CompositorEffectGlassCompPtr clonedCompPtr(boost::make_shared<CompositorEffectGlassComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setGlassWeight(this->glassWeight->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
#else
		return nullptr;
#endif
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
		CompositorEffectOldTvCompPtr clonedCompPtr(boost::make_shared<CompositorEffectOldTvComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setDistortionFrequency(this->distortionFrequency->getReal());
		clonedCompPtr->setDistortionScale(this->distortionScale->getReal());
		clonedCompPtr->setDistortionRoll(this->distortionRoll->getReal());
		clonedCompPtr->setInterference(this->interference->getReal());
		clonedCompPtr->setFrameLimit(this->frameLimit->getReal());
		clonedCompPtr->setFrameShape(this->frameShape->getReal());
		clonedCompPtr->setFrameSharpness(this->frameSharpness->getReal());
		clonedCompPtr->setTime(this->time->getUInt());
		clonedCompPtr->setSinusTime(this->sinusTime->getUInt());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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
		CompositorEffectBlackAndWhiteCompPtr clonedCompPtr(boost::make_shared<CompositorEffectBlackAndWhiteComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setColor(this->color->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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
		CompositorEffectColorCompPtr clonedCompPtr(boost::make_shared<CompositorEffectColorComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setColor(this->color->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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
		CompositorEffectEmbossedCompPtr clonedCompPtr(boost::make_shared<CompositorEffectEmbossedComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setWeight(this->weight->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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
		CompositorEffectSharpenEdgesCompPtr clonedCompPtr(boost::make_shared<CompositorEffectSharpenEdgesComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setWeight(this->weight->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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
        CompositorEffectCartoonCompPtr clonedCompPtr(boost::make_shared<CompositorEffectCartoonComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setEdgeThreshold(this->edgeThreshold->getReal());
        clonedCompPtr->setEdgeStrength(this->edgeStrength->getReal());
        clonedCompPtr->setNumBands(this->numBands->getReal());
        clonedCompPtr->setSaturation(this->saturation->getReal());
        clonedCompPtr->setEdgeDarkness(this->edgeDarkness->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
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
        CompositorEffectVolumetricLightCompPtr clonedCompPtr(boost::make_shared<CompositorEffectVolumetricLightComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setLightId(this->lightId->getULong());
        clonedCompPtr->setGodRayStrength(this->godRayStrength->getReal());
        clonedCompPtr->setSunThreshold(this->sunThreshold->getReal());
        clonedCompPtr->setSunRadius(this->sunRadius->getReal());
        clonedCompPtr->setDecay(this->decay->getReal());
        clonedCompPtr->setDensity(this->density->getReal());
        clonedCompPtr->setExposure(this->exposure->getReal());
        clonedCompPtr->setTint(this->tint->getVector3());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
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
        WorkspaceBaseComponent* workspaceBaseComponent = this->workspaceBaseComponent;

        auto closureFunction = [maskPass, blurPass, light, camera, workspaceBaseComponent](Ogre::Real /*renderDt*/)
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
                // Sun is behind the camera — push it to a far edge so rays
                // still point in roughly the right direction rather than
                // collapsing to screen centre.
                Ogre::Vector2 edge(clipPos.x, -clipPos.y);
                float len = edge.length();
                if (len > 0.0001f)
                {
                    edge /= len;
                    sunScreenPos = Ogre::Vector2(0.5f + edge.x * 2.0f, 0.5f + edge.y * 2.0f);
                }
                else
                {
                    // Completely overhead or directly behind — arbitrary fallback
                    sunScreenPos = Ogre::Vector2(0.5f, 0.5f);
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
        CompositorEffectFogCompPtr clonedCompPtr(boost::make_shared<CompositorEffectFogComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setDepthFogDensity(this->depthFogDensity->getReal());
        clonedCompPtr->setDepthFogStart(this->depthFogStart->getReal());
        clonedCompPtr->setHeightFogDensity(this->heightFogDensity->getReal());
        clonedCompPtr->setHeightFogStart(this->heightFogStart->getReal());
        clonedCompPtr->setHeightFogEnd(this->heightFogEnd->getReal());
        clonedCompPtr->setFogColor(this->fogColor->getVector3());
        clonedCompPtr->setFogSkyBlend(this->fogSkyBlend->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
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

}; // namespace end