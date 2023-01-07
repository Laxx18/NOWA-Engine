#include "NOWAPrecompiled.h"
#include "CompositorEffectComponents.h"
#include "CameraComponent.h"
#include "utilities/XMLConverter.h"
#include "WorkspaceComponents.h"
#include "main/Events.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	CompositorEffectBaseComponent::CompositorEffectBaseComponent()
		: GameObjectComponent(),
		effectName("None"),
		isInSimulation(false),
		workspaceBaseComponent(nullptr),
		activated(new Variant(CompositorEffectBaseComponent::AttrActivated(), true, this->attributes)),
		workspaceGameObjectId(new Variant(CompositorEffectBaseComponent::AttrWorkspaceGameObjectId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->activated->setDescription("Activates the effect.");
		// "Bloom, Glass, Old TV, Keyhole, B&W, Embossed, Sharpen Edges, Invert, Posterize, Laplace, Tiling, Old Movie, Radial Blur, ASCII, Halftone, Night Vision, Dither"

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &CompositorEffectBaseComponent::handleWorkspaceComponentDeleted), EventDataDeleteWorkspaceComponent::getStaticEventType());
	}

	CompositorEffectBaseComponent::~CompositorEffectBaseComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectBaseComponent] Destructor compositor base effect component for game object: " + this->gameObjectPtr->getName());
		
		this->workspaceBaseComponent = nullptr;

		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &CompositorEffectBaseComponent::handleWorkspaceComponentDeleted), EventDataDeleteWorkspaceComponent::getStaticEventType());
	}

	bool CompositorEffectBaseComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

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

	bool CompositorEffectBaseComponent::connect(void)
	{
		this->isInSimulation = true;

		GameObjectPtr workspaceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->workspaceGameObjectId->getULong());
		if (nullptr == workspaceGameObjectPtr)
		{
			Ogre::String message = "[CompositorEffectBaseComponent] Could not get game object with workspace component Affected game object: "
				+ this->gameObjectPtr->getName();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

			boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
			return false;
		}

		auto& cameraCompPtr = NOWA::makeStrongPtr(workspaceGameObjectPtr->getComponent<CameraComponent>());
		if (nullptr != cameraCompPtr)
		{
			// If camera is not activated, this compositor effect should not be played
			if (false == cameraCompPtr->isActivated())
			{
				return true;
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectBaseComponent] Could not get prior CameraComponent! Affected game object: " + this->gameObjectPtr->getName());
			return false;
		}

		auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(workspaceGameObjectPtr->getComponent<WorkspaceBaseComponent>());
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
			return false;
		}

		this->setActivated(this->activated->getBool());
		return true;
	}

	bool CompositorEffectBaseComponent::disconnect(void)
	{
		if (nullptr != this->workspaceBaseComponent)
		{
			this->isInSimulation = false;
		
			this->playEffect(this->effectName, false);
		}
		return true;
	}

	void CompositorEffectBaseComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;
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
		// Found the game object
		if (this->workspaceGameObjectId->getULong() == castEventData->getGameObjectId())
		{
			this->workspaceBaseComponent = nullptr;
		}
	}

	void CompositorEffectBaseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("Component" + this->getClassName())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->getClassName())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WorkspaceGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->workspaceGameObjectId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	void CompositorEffectBaseComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == this->isInSimulation)
		{
			this->playEffect(this->effectName, activated);
		}
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

	void CompositorEffectBaseComponent::playEffect(const Ogre::String& effectName, bool enabled)
	{
		if ("none" == effectName || nullptr == this->workspaceBaseComponent || nullptr == this->workspaceBaseComponent->getWorkspace())
			return;

		GameObjectPtr workspaceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->workspaceGameObjectId->getULong());

		auto& cameraCompPtr = NOWA::makeStrongPtr(workspaceGameObjectPtr->getComponent<CameraComponent>());

		// Get the chosen workspace name
		const Ogre::IdString workspaceName(this->workspaceBaseComponent->getWorkspaceName());

		Ogre::String renderingNodeNOWA = this->workspaceBaseComponent->getRenderingNodeName();

		// Disable/Enable the node (it was already instantiated in setupCompositor())
		Ogre::CompositorNode* node = this->workspaceBaseComponent->getWorkspace()->findNode(effectName);
		if (nullptr != node)
		{
			node->setEnabled(enabled);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,"[WorkspaceModule] Could not find effect: " + effectName);
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[WorkspaceModule] Could not find effect: " + effectName, "NOWA");
		}

		// Get workspace definition, to connect everything
		Ogre::CompositorWorkspaceDef* workspaceDef = this->workspaceBaseComponent->getWorkspace()->getCompositorManager()->getWorkspaceDefinition(workspaceName);

		//-------------------------------------------------------------------------------------------
		//
		//  METHOD 2 (the easy way):
		//      Reconstruct the whole connection from scratch based on a copy (be it a cloned,
		//      untouched workspace definition, a custom file, or the very own workspace instance)
		//      but leaving the node we're disabling unplugged.
		//      This method is much safer and easier, the **recommended** way for most usage
		//      scenarios involving toggling compositors on and off frequently. With a few tweaks,
		//      it can easily be adapted to complex compositors too.
		//
		//-------------------------------------------------------------------------------------------
		workspaceDef->clearAllInterNodeConnections();


		Ogre::IdString msaaCompositionId = "NOWAHdrMsaaResolve";
		Ogre::IdString hdrPostProcessingCompositionId;
		if (true == this->workspaceBaseComponent->getUseHdr())
		{
			hdrPostProcessingCompositionId = "NOWAHdrPostprocessingNode";
		}
		// Ogre::IdString finalCompositionId = "NOWAHdrFinalCompositionNode";
		Ogre::IdString finalCompositionId = this->workspaceBaseComponent->getFinalRenderingNodeName();

		const Ogre::CompositorNodeVec& nodes = this->workspaceBaseComponent->getWorkspace()->getNodeSequence();

		Ogre::IdString lastInNode;
		Ogre::CompositorNodeVec::const_iterator it = nodes.begin();
		Ogre::CompositorNodeVec::const_iterator en = nodes.end();

		// Iterate through all effects and add them
		// Note: combinations are possible, so one prior effect is connected to the next effect
		while (it != en)
		{
			Ogre::CompositorNode* outNode = *it;

			if (outNode->getEnabled() && outNode->getName() != finalCompositionId
				&& outNode->getName() != hdrPostProcessingCompositionId
				&& outNode->getName() != msaaCompositionId)
			{
				// Look for the next enabled node we can connect to
				Ogre::CompositorNodeVec::const_iterator it2 = it + 1;
				while (it2 != en && (!(*it2)->getEnabled()
					|| (*it2)->getName() == finalCompositionId
					|| (*it2)->getName() == hdrPostProcessingCompositionId
					|| (*it2)->getName() == msaaCompositionId))
				{
					++it2;
				}

				if (it2 != en)
				{
					lastInNode = (*it2)->getName();
					workspaceDef->connect(outNode->getName(), 0, lastInNode, 0);
					workspaceDef->connect(outNode->getName(), 1, lastInNode, 1);
					// Example:
					// workspaceDef->connect("NOWASkyRenderingNode", 0, "Old Tv", 0);
					// workspaceDef->connect("NOWASkyRenderingNode", 1, "Old Tv", 1);
					// workspaceDef->connect("Old Tv", 0, "Glass", 0);
					// workspaceDef->connect("Old Tv", 1, "Glass", 1);
				}

				it = it2 - 1;
			}

			++it;
		}

		if (lastInNode == Ogre::IdString())
		{
			lastInNode = renderingNodeNOWA;
		}
		if (lastInNode == Ogre::IdString())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceModule] Error: Workspacenode is null!");
		}

		if (true == this->workspaceBaseComponent->getUseHdr())
		{
			// Example without msaa:
			if (1 == this->workspaceBaseComponent->getMSAA())
			{
				// Connect NOWASkyRenderingNode output 2 to NOWAHdrPostprocessingNode input 1
				workspaceDef->connect(renderingNodeNOWA, 2, hdrPostProcessingCompositionId, 1);
				// Connect compositor effect Glass output 0 to NOWAHdrPostprocessingNode input 0
				workspaceDef->connect(lastInNode, 0, hdrPostProcessingCompositionId, 0);
				// Connect RenderWindow to NOWAHdrPostprocessingNode input 2
				// Note: Will produce warning, as exactly this external connection already may exist, but it does not matter, depending on execution order, it may be that its not connected
				workspaceDef->connectExternal(0, hdrPostProcessingCompositionId, 2);
				// Connect NOWAHdrPostprocessingNode output 0 to NOWAHdrFinalCompositionNode input 0
				workspaceDef->connect(hdrPostProcessingCompositionId, 0, finalCompositionId, 0);

				// Example:
				// workspaceDef->connect("NOWASkyRenderingNode", 0, "Glass", 0);
				// workspaceDef->connect("NOWASkyRenderingNode", 1, "Glass", 1);
				// workspaceDef->connect("NOWASkyRenderingNode", 2, "NOWAHdrPostprocessingNode", 1);
				// workspaceDef->connect("Glass", 0, "NOWAHdrPostprocessingNode", 0);
				// workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);
				// workspaceDef->connect("NOWAHdrPostprocessingNode", 0, "NOWAHdrFinalCompositionNode", 0);
			}
			else
			{
				workspaceDef->connect(lastInNode, 0, msaaCompositionId, 0);
				workspaceDef->connect(renderingNodeNOWA, 2, msaaCompositionId, 1);
				workspaceDef->connect(renderingNodeNOWA, 2, hdrPostProcessingCompositionId, 1);
				workspaceDef->connect("NOWAHdrMsaaResolve", 0, hdrPostProcessingCompositionId, 0);
				workspaceDef->connectExternal(0, hdrPostProcessingCompositionId, 2);
				workspaceDef->connect(hdrPostProcessingCompositionId, 0, finalCompositionId, 0);
			}
		}
		else
		{
			workspaceDef->connect(lastInNode, 0, finalCompositionId, 1);
		}

		//Not needed unless we'd called workspaceDef->clearOutputConnections
		//workspaceDef->connectOutput( "NOWAHdrPostprocessingNode", 0 );

		//Now that we're done, tell the instance to update itself.
		this->workspaceBaseComponent->getWorkspace()->reconnectAllNodes();
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

	bool CompositorEffectBloomComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);
		
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
		return true;
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

	void CompositorEffectBloomComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ImageWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->imageWeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BlurWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->blurWeight->getReal())));
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
			this->passes[3]->getFragmentProgramParameters()->setNamedConstant("OriginalImageWeight", imageWeight);
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
			this->passes[3]->getFragmentProgramParameters()->setNamedConstant("BlurWeight", blurWeight);
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

	bool CompositorEffectGlassComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GlassWeight")
		{
			this->setGlassWeight(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectGlassComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CompositorEffectGlassCompPtr clonedCompPtr(boost::make_shared<CompositorEffectGlassComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setGlassWeight(this->glassWeight->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
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

	void CompositorEffectGlassComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GlassWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->glassWeight->getReal())));
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("GlassWeight", glassWeight);
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

	bool CompositorEffectOldTvComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

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

	void CompositorEffectOldTvComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DistortionFrequency"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->distortionFrequency->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DistortionScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->distortionScale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DistortionRoll"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->distortionRoll->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Interference"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->interference->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->frameLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameShape"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->frameShape->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameSharpness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->frameSharpness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Time"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->time->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SinusTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sinusTime->getUInt())));
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("distortionFreq", distortionFrequency);
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("distortionScale", distortionScale);
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("distortionRoll", distortionRoll);
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("interference", interference);
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("frameLimit", frameLimit);
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("frameShape", frameShape);
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("frameSharpness", frameSharpness);
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
			// this->pass->getFragmentProgramParameters()->setNamedConstant("time_0_X", time);
			this->pass->getFragmentProgramParameters()->setNamedConstantFromTime("time_0_X", static_cast<Ogre::Real>(time));
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
			// this->pass->getFragmentProgramParameters()->setNamedConstant("sin_time_0_X", sinusTime);
			this->pass->getFragmentProgramParameters()->setNamedConstantFromTime("sin_time_0_X", static_cast<Ogre::Real>(sinusTime));
		}
	}

	unsigned int CompositorEffectOldTvComponent::getSinusTime(void) const
	{
		return this->sinusTime->getUInt();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CompositorEffectKeyholeComponent::CompositorEffectKeyholeComponent()
		: CompositorEffectBaseComponent(),
		pass(nullptr),
		frameShape(new Variant(CompositorEffectKeyholeComponent::AttrFrameShape(), 0.26f, this->attributes))
	{
		this->effectName = "Keyhole";
	}

	CompositorEffectKeyholeComponent::~CompositorEffectKeyholeComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectKeyholeComponent] Destructor compositor keyhole effect component for game object: " + this->gameObjectPtr->getName());
		this->pass = nullptr;
	}

	bool CompositorEffectKeyholeComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FrameShape")
		{
			this->setFrameShape(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr CompositorEffectKeyholeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CompositorEffectKeyholeCompPtr clonedCompPtr(boost::make_shared<CompositorEffectKeyholeComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setFrameShape(this->frameShape->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CompositorEffectKeyholeComponent::postInit(void)
	{
		bool success = CompositorEffectBaseComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectKeyholeComponent] Init compositor effect keyhole component for game object: " + this->gameObjectPtr->getName());

		Ogre::String materialName0 = "Postprocess/Keyhole";

		this->material = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName0);
		if (true == this->material.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CompositorEffectKeyholeComponent] Could not set: " + this->effectName
				+ " because the material: '" + materialName0 + "' does not exist!");
			return false;
		}

		this->material->load();

		Ogre::Material* material0 = this->material.getPointer();
		this->pass = material0->getTechnique(0)->getPass(0);

		// Set intial value, so that the compositor starts with the loaded values
		this->setFrameShape(this->frameShape->getReal());

		return success;
	}

	void CompositorEffectKeyholeComponent::actualizeValue(Variant* attribute)
	{
		CompositorEffectBaseComponent::actualizeValue(attribute);

		if (CompositorEffectKeyholeComponent::AttrFrameShape() == attribute->getName())
		{
			this->setFrameShape(attribute->getReal());
		}
	}

	void CompositorEffectKeyholeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML  = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FrameShape"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->frameShape->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CompositorEffectKeyholeComponent::getClassName(void) const
	{
		return "CompositorEffectKeyholeComponent";
	}

	Ogre::String CompositorEffectKeyholeComponent::getParentClassName(void) const
	{
		return "CompositorEffectBaseComponent";
	}

	void CompositorEffectKeyholeComponent::setFrameShape(Ogre::Real frameShape)
	{
		this->frameShape->setValue(frameShape);
		if (nullptr != this->pass)
		{
			this->pass->getFragmentProgramParameters()->setNamedConstant("frameShape", frameShape);
		}
	}

	Ogre::Real CompositorEffectKeyholeComponent::getFrameShape(void) const
	{
		return this->frameShape->getReal();
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

	bool CompositorEffectBlackAndWhiteComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

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

	void CompositorEffectBlackAndWhiteComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->color->getVector3())));
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("color", color);
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

	bool CompositorEffectColorComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

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

	void CompositorEffectColorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->color->getVector3())));
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("color", color);
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

	bool CompositorEffectEmbossedComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

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

	void CompositorEffectEmbossedComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Weight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->weight->getReal())));
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("weight", weight);
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

	bool CompositorEffectSharpenEdgesComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CompositorEffectBaseComponent::init(propertyElement, filename);

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

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CompositorEffectSharpenEdgesComponent] Init compositor effect embossed component for game object: " + this->gameObjectPtr->getName());

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

	void CompositorEffectSharpenEdgesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CompositorEffectBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Weight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->weight->getReal())));
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
			this->pass->getFragmentProgramParameters()->setNamedConstant("weight", weight);
		}
	}

	Ogre::Real CompositorEffectSharpenEdgesComponent::getWeight(void) const
	{
		return this->weight->getReal();
	}

}; // namespace end