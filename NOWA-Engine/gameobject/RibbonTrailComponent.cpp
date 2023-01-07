#include "NOWAPrecompiled.h"
#include "RibbonTrailComponent.h"
#include "GameObjectController.h"
#include "CameraComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "main/ProcessManager.h"
#include "main/AppStateManager.h"

namespace
{
	bool realToBool(Ogre::Real value)
	{
		if (isnan(value)) return false; // it is not-a-number
		return std::abs(value) > std::numeric_limits<Ogre::Real>::epsilon();
	}

	class SetDataBlockProcess : public NOWA::Process
	{
	public:
		explicit SetDataBlockProcess(Ogre::SceneNode* trackNode, Ogre::v1::RibbonTrail* ribbonTrail, const Ogre::String& datablockName)
			: ribbonTrail(ribbonTrail),
			datablockName(datablockName),
			trackNode(trackNode)
		{

		}
	protected:
		virtual void onInit(void) override
		{
			this->succeed();
			// Attention: Is this necessary, or here id of external node to be tracked?
			if (nullptr != this->trackNode)
			{
				this->ribbonTrail->addNode(this->trackNode);
			}
			this->ribbonTrail->setVisible(true);
			this->ribbonTrail->setDatablock(this->datablockName);

			//https://forums.ogre3d.org/viewtopic.php?t=82797
			//static_cast<Ogre::HlmsUnlitDatablock*>(this->ribbonTrail->getDatablock())->setTextureUvSource(0, 0); //Tex Unit 0 will be sampled using uv0
			//static_cast<Ogre::HlmsUnlitDatablock*>(this->ribbonTrail->getDatablock())->setTextureUvSource(0, 1); //Tex Unit 0 will be sampled using uv1
			//static_cast<Ogre::HlmsUnlitDatablock*>(this->ribbonTrail->getDatablock())->setTextureUvSource(1, 0); //Tex Unit 1 will be sampled using uv0
			//static_cast<Ogre::HlmsUnlitDatablock*>(this->ribbonTrail->getDatablock())->setTextureUvSource(1, 1); //Tex Unit 1 will be sampled using uv1
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	private:
		Ogre::v1::RibbonTrail* ribbonTrail;
		Ogre::String datablockName;
		Ogre::SceneNode* trackNode;
	};
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	RibbonTrailComponent::RibbonTrailComponent()
		: GameObjectComponent(),
		ribbonTrail(nullptr),
		camera(nullptr),
		maxChainElements(new Variant(RibbonTrailComponent::AttrMaxChainElements(), static_cast<unsigned int>(6000), this->attributes)),
		numberOfChains(new Variant(RibbonTrailComponent::AttrNumberOfChains(), static_cast<unsigned int>(0), this->attributes)),
		faceCameraId(new Variant(RibbonTrailComponent::AttrFaceCameraId(), static_cast<unsigned long>(0), this->attributes, true)),
		textureCoordRange(new Variant(RibbonTrailComponent::AttrTextureCoordRange(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		trailLength(new Variant(RibbonTrailComponent::AttrTrailLength(), 400.0f, this->attributes)),
		renderDistance(new Variant(RibbonTrailComponent::AttrRenderDistance(), 1000.0f, this->attributes))
	{
		std::vector<Ogre::String> datablockNames;

		// Go through all types of registered hlms unlit and check if the data block exists and set the data block
		Ogre::Hlms* hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		if (nullptr != hlms)
		{
			for (auto& it = hlms->getDatablockMap().cbegin(); it != hlms->getDatablockMap().cend(); ++it)
			{
				datablockNames.emplace_back(it->second.name);
			}
		}
		std::sort(datablockNames.begin(), datablockNames.end());
		this->datablockName = new Variant(RibbonTrailComponent::AttrDatablockName(), datablockNames, this->attributes);
		this->datablockName->setListSelectedValue("LightRibbonTrail");

		this->faceCameraId->setDescription("When valid camera id is set, ribbon trail will face camera.");

		this->numberOfChains->addUserData(GameObject::AttrActionNeedRefresh());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &RibbonTrailComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());
	}

	RibbonTrailComponent::~RibbonTrailComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RibbonTrailComponent] Destructor ribbon trail component for game object: " + this->gameObjectPtr->getName());
		
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &RibbonTrailComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());
		
		if (nullptr != this->ribbonTrail)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->ribbonTrail);
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->ribbonTrail);
			this->ribbonTrail = nullptr;
		}
	}
	
	void RibbonTrailComponent::handleRemoveCamera(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataRemoveCamera> castEventData = boost::static_pointer_cast<EventDataRemoveCamera>(eventData);
		if (this->camera == castEventData->getCamera())
		{
			// Set camera to null, if it has been removed by the camera component
			this->camera = nullptr;
			this->faceCameraId->setValue(static_cast<unsigned long>(0));
			if (nullptr != this->ribbonTrail)
				this->ribbonTrail->setFaceCamera(false, Ogre::Vector3::ZERO);
		}
	}

	bool RibbonTrailComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DatablockName")
		{
			this->datablockName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxChainElements")
		{
			this->maxChainElements->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumberOfChains")
		{
			this->numberOfChains->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->colorChanges.size() < this->numberOfChains->getUInt())
		{
			this->colorChanges.resize(this->numberOfChains->getUInt());
			this->widthChanges.resize(this->numberOfChains->getUInt());
			this->initialColors.resize(this->numberOfChains->getUInt());
			this->initialWidths.resize(this->numberOfChains->getUInt());
		}

		for (unsigned int i = 0; i < this->numberOfChains->getUInt(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ColorChange" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->colorChanges[i])
				{
					this->colorChanges[i] = new Variant(RibbonTrailComponent::AttrColorChange() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->colorChanges[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WidthChange" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->widthChanges[i])
				{
					this->widthChanges[i] = new Variant(RibbonTrailComponent::AttrWidthChange() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
				}
				else
				{
					this->widthChanges[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}

			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InitialColor" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->initialColors[i])
				{
					this->initialColors[i] = new Variant(RibbonTrailComponent::AttrInitialColor() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->initialColors[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InitialWidth" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->initialWidths[i])
				{
					this->initialWidths[i] = new Variant(RibbonTrailComponent::AttrInitialWidth() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
				}
				else
				{
					this->initialWidths[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->initialWidths[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FaceCameraId")
		{
			this->faceCameraId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureCoordRange")
		{
			this->textureCoordRange->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrailLength")
		{
			this->trailLength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RenderDistance")
		{
			this->renderDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr RibbonTrailComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		RibbonTrailCompPtr clonedCompPtr(boost::make_shared<RibbonTrailComponent>());

		
		clonedCompPtr->setMaxChainElements(this->maxChainElements->getUInt());
		clonedCompPtr->setNumberOfChains(this->numberOfChains->getUInt());
		
		for (unsigned int i = 0; i < this->numberOfChains->getUInt(); i++)
		{
			clonedCompPtr->setColorChange(i, this->colorChanges[i]->getVector3());
			clonedCompPtr->setWidthChange(i, this->widthChanges[i]->getReal());
			clonedCompPtr->setInitialColor(i, this->initialColors[i]->getVector3());
			clonedCompPtr->setInitialWidth(i, this->initialWidths[i]->getReal());
		}
		
		clonedCompPtr->setFaceCameraId(this->faceCameraId->getULong());
		clonedCompPtr->setTexCoordChange(this->textureCoordRange->getVector2());
		clonedCompPtr->setTrailLength(this->trailLength->getReal());
		clonedCompPtr->setRenderDistance(this->renderDistance->getReal());
		clonedCompPtr->setDatablockName(this->datablockName->getListSelectedValue());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool RibbonTrailComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RibbonTrailComponent] Init ribbon trail component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->setFaceCameraId(this->faceCameraId->getULong());
		this->createRibbonTrail();
		return true;
	}
	
	bool RibbonTrailComponent::connect(void)
	{
		this->setFaceCameraId(this->faceCameraId->getULong());
		
		return true;
	}

	void RibbonTrailComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->_timeUpdate(dt);
			
			if (nullptr != this->camera)
			{
				this->ribbonTrail->setFaceCamera(true, this->camera->getDirection());
			}
		}
	}

	void RibbonTrailComponent::createRibbonTrail(void)
	{
		if (nullptr == this->ribbonTrail)
		{
			// Create a ribbon trail that our lights will leave behind
			Ogre::NameValuePairList params;
			params["numberOfChains"] = this->numberOfChains->getUInt();
			params["maxElements"] = this->maxChainElements->getUInt();
			this->ribbonTrail = (Ogre::v1::RibbonTrail*)this->gameObjectPtr->getSceneManager()->createMovableObject("RibbonTrail", 
				&this->gameObjectPtr->getSceneManager()->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), &params);

			// this->ribbonTrail = this->gameObjectPtr->getSceneManager()->createRibbonTrail();
			
			// this->ribbonTrail->setRenderQueueGroup(NOWA::RENDER_QUEUE_PARTICLE_STUFF);
			this->ribbonTrail->setRenderQueueGroup(NOWA::RENDER_QUEUE_LEGACY);

			auto data = DeployResourceModule::getInstance()->getPathAndResourceGroupFromDatablock(this->datablockName->getListSelectedValue(), Ogre::HlmsTypes::HLMS_UNLIT);

			DeployResourceModule::getInstance()->tagResource(this->datablockName->getListSelectedValue(), data.first, data.second);
			// this->ribbonTrail->setDatablockOrMaterialName(this->datablockName->getListSelectedValue(), "Unlit");
			// this->ribbonTrail->setDatablockOrMaterialName("Examples/LightRibbonTrail", "General"); // does not work anymore because of fixed pipeline
			// Must be set to false, because unlit hlms is involved
			// this->ribbonTrail->setCastShadows(false);
			this->ribbonTrail->setQueryFlags(0);
			this->ribbonTrail->setDynamic(true);
			
			this->ribbonTrail->setUseVertexColours(true);
			this->ribbonTrail->setUseTextureCoords(true);
			// this->ribbonTrail->setName("RibbonTrail");
			

			this->ribbonTrail->setMaxChainElements(static_cast<unsigned int>(this->maxChainElements->getUInt()));
			this->ribbonTrail->setNumberOfChains(static_cast<unsigned int>(this->numberOfChains->getUInt()));
			 
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->colorChanges.size()); i++)
			{
				this->setColorChange(i, this->colorChanges[i]->getVector3());
				this->setWidthChange(i, this->widthChanges[i]->getReal());
				this->setInitialColor(i, this->initialColors[i]->getVector3());
				this->setInitialWidth(i, this->initialWidths[i]->getReal());
			}

			this->ribbonTrail->setOtherTextureCoordRange(this->textureCoordRange->getVector2().x, this->textureCoordRange->getVector2().y);
			this->ribbonTrail->setTrailLength(this->trailLength->getReal());
			this->ribbonTrail->setRenderingDistance(this->renderDistance->getReal());

			// this->ribbonTrail->setDatablock(this->datablockName->getListSelectedValue());


			this->gameObjectPtr->getSceneNode()->attachObject(this->ribbonTrail);

// Attention: will never be destroyed
			Ogre::SceneNode* trailNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
			// this->ribbonTrail->addNode(trailNode);

			NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.1f));
			delayProcess->attachChild(NOWA::ProcessPtr(new SetDataBlockProcess(trailNode, this->ribbonTrail, this->datablockName->getListSelectedValue())));
			NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

			// https://forums.ogre3d.org/viewtopic.php?t=91597
			
			/*Ogre::v1::BillboardChain * chain = this->gameObjectPtr->getSceneManager()->createBillboardChain();
			chain->setDynamic(true);
			chain->setNumberOfChains(1);
			chain->setUseVertexColours(true);
			chain->setMaxChainElements(5);
			chain->setDatablockOrMaterialName(this->datablockName->getListSelectedValue(), "Unlit");
			
			for (int i = 0; i < 5; ++i)
				chain->addChainElement(0, Ogre::v1::BillboardChain::Element(Ogre::Vector3(0.5 * i, 0, 0), 0.1, 0.2 * i, Ogre::ColourValue(1, 0, 0, 1), Ogre::Quaternion()));
			
			this->gameObjectPtr->getSceneNode()->attachObject(chain);
			chain->setVisible(true);*/
		}
	}

	void RibbonTrailComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (RibbonTrailComponent::AttrDatablockName() == attribute->getName())
		{
			this->setDatablockName(attribute->getListSelectedValue());
		}
		else if (RibbonTrailComponent::AttrMaxChainElements() == attribute->getName())
		{
			this->setMaxChainElements(attribute->getUInt());
		}
		else if (RibbonTrailComponent::AttrNumberOfChains() == attribute->getName())
		{
			this->setNumberOfChains(attribute->getUInt());
		}
		else if (RibbonTrailComponent::AttrFaceCameraId() == attribute->getName())
		{
			this->setFaceCameraId(attribute->getULong());
		}
		else if (RibbonTrailComponent::AttrTextureCoordRange() == attribute->getName())
		{
			this->setTexCoordChange(attribute->getVector2());
		}
		else if (RibbonTrailComponent::AttrTrailLength() == attribute->getName())
		{
			this->setTrailLength(attribute->getReal());
		}
		else if (RibbonTrailComponent::AttrRenderDistance() == attribute->getName())
		{
			this->setRenderDistance(attribute->getReal());
		}
		else
		{
			for (unsigned int i = 0; i < this->numberOfChains->getUInt(); i++)
			{
				if (RibbonTrailComponent::AttrColorChange() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setColorChange(i, attribute->getVector3());
				}
				else if (RibbonTrailComponent::AttrWidthChange() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setWidthChange(i, attribute->getReal());
				}
				else if (RibbonTrailComponent::AttrInitialColor() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setInitialColor(i, attribute->getVector3());
				}
				else if (RibbonTrailComponent::AttrInitialWidth() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setInitialWidth(i, attribute->getReal());
				}
			}
		}
		
		// else if (RibbonTrailComponent::AttrCastShadows() == attribute->getName())
		// {
		// 	this->setCastShadows(attribute->getBool());
		// }
	}

	void RibbonTrailComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "DatablockName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->datablockName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxChainElements"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->maxChainElements->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NumberOfChains"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numberOfChains->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		for (size_t i = 0; i < this->numberOfChains->getUInt(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("ColorChange" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->colorChanges[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("WidthChange" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->widthChanges[i]->getReal())));
			propertiesXML->append_node(propertyXML);
			
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("InitialColor" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->initialColors[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("InitialWidth" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->initialWidths[i]->getReal())));
			propertiesXML->append_node(propertyXML);
		}
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FaceCameraId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->faceCameraId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextureCoordRange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->textureCoordRange->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TrailLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->trailLength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RenderDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->renderDistance->getReal())));
		propertiesXML->append_node(propertyXML);
		
		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);*/
	}

	void RibbonTrailComponent::setActivated(bool activated)
	{
		
	}

	Ogre::String RibbonTrailComponent::getClassName(void) const
	{
		return "RibbonTrailComponent";
	}

	Ogre::String RibbonTrailComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void RibbonTrailComponent::setDatablockName(const Ogre::String& datablockName)
	{
		this->datablockName->setListSelectedValue(datablockName);
		if (nullptr != this->ribbonTrail)
		{
			// this->ribbonTrail->setDatablock(this->datablockName->getListSelectedValue());
			// this->ribbonTrail->setDatablock(Ogre::Root::getSingleton().getHlmsManager()->getDatablock(datablockName));
			// this->ribbonTrail->setDatablockOrMaterialName(this->datablockName->getListSelectedValue(), "Unlit");
			// this->ribbonTrail->setDatablock(LightRibbonTrail"); // does not work anymore because of fixed pipeline

			// Attention: will never be destroyed
			Ogre::SceneNode* trailNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();

			NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.1f));
			//// creates the delay process and changes the application state at another tick. Note, this is necessary
			//// because changing the application state destroys all game objects and its components from the current state.
			//// so changing the state directly inside a component would create a mess, since everything will be destroyed
			//// and the game object map in update loop becomes invalid while its iterating
			delayProcess->attachChild(NOWA::ProcessPtr(new SetDataBlockProcess(trailNode, this->ribbonTrail, this->datablockName->getListSelectedValue())));
			NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
		}
	}

	Ogre::String RibbonTrailComponent::getDatablockName(void) const
	{
		return this->datablockName->getListSelectedValue();
	}
	
	void RibbonTrailComponent::setMaxChainElements(unsigned int maxChainElements)
	{
		this->maxChainElements->setValue(maxChainElements);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setMaxChainElements(maxChainElements);
		}
	}

	unsigned int RibbonTrailComponent::getMaxChainElements(void) const
	{
		return this->maxChainElements->getUInt();
	}

	void RibbonTrailComponent::setNumberOfChains(unsigned int numberOfChains)
	{
		this->numberOfChains->setValue(numberOfChains);
		unsigned int oldSize = static_cast<unsigned int>(this->colorChanges.size());

		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setNumberOfChains(numberOfChains);
		}

		if (numberOfChains > oldSize)
		{
			// Resize the waypoints array for count
			this->colorChanges.resize(numberOfChains);
			this->widthChanges.resize(numberOfChains);
			this->initialColors.resize(numberOfChains);
			this->initialWidths.resize(numberOfChains);

			for (unsigned int i = oldSize; i < this->numberOfChains->getUInt(); i++)
			{
				this->colorChanges[i] = new Variant(RibbonTrailComponent::AttrColorChange() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.5f, 0.5f, 0.5f), this->attributes);
				this->widthChanges[i] = new Variant(RibbonTrailComponent::AttrWidthChange() + Ogre::StringConverter::toString(i), 0, this->attributes);
				this->initialColors[i] = new Variant(RibbonTrailComponent::AttrInitialColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.0f, 1.0f, 0.8f), this->attributes);
				this->initialWidths[i] = new Variant(RibbonTrailComponent::AttrInitialWidth() + Ogre::StringConverter::toString(i), 0.5f, this->attributes);
				this->initialWidths[i]->addUserData(GameObject::AttrActionSeparator());

				this->setColorChange(i, this->colorChanges[i]->getVector3());
				this->setWidthChange(i, this->widthChanges[i]->getReal());
				this->setInitialColor(i, this->initialColors[i]->getVector3());
				this->setInitialWidth(i, this->initialWidths[i]->getReal());
			}
		}
		else if (numberOfChains < oldSize)
		{
			this->eraseVariants(this->colorChanges, numberOfChains);
			this->eraseVariants(this->widthChanges, numberOfChains);
			this->eraseVariants(this->initialColors, numberOfChains);
			this->eraseVariants(this->initialWidths, numberOfChains);
		}
	}

	unsigned int RibbonTrailComponent::getNumberOfChains(void) const
	{
		return this->numberOfChains->getUInt();
	}

	void RibbonTrailComponent::setColorChange(unsigned int chainIndex, const Ogre::Vector3& valuePerSecond)
	{
		if (chainIndex > this->colorChanges.size())
			chainIndex = static_cast<unsigned int>(this->colorChanges.size()) - 1;
		
		this->colorChanges[chainIndex]->setValue(valuePerSecond);
		if (nullptr != this->ribbonTrail)
		{
			// What is with alpha?
			this->ribbonTrail->setColourChange(chainIndex, valuePerSecond.x, valuePerSecond.y, valuePerSecond.z, 1.0f);
		}
	}

	Ogre::Vector3 RibbonTrailComponent::getColorChange(unsigned int chainIndex) const
	{
		if (index > this->colorChanges.size())
			return Ogre::Vector3::ZERO;
		return this->colorChanges[index]->getVector3();
	}

	void RibbonTrailComponent::setWidthChange(unsigned int chainIndex, Ogre::Real widthDeltaPerSecond)
	{
		if (chainIndex > this->widthChanges.size())
			chainIndex = static_cast<unsigned int>(this->widthChanges.size()) - 1;
		
		this->widthChanges[chainIndex]->setValue(widthDeltaPerSecond);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setWidthChange(chainIndex, widthDeltaPerSecond);
		}
	}

	Ogre::Real RibbonTrailComponent::getWidthChange(unsigned int chainIndex) const
	{
		if (index > this->widthChanges.size())
			return 0.0f;
		return this->widthChanges[index]->getReal();
	}

	void RibbonTrailComponent::setInitialColor(unsigned int chainIndex, const Ogre::Vector3& color)
	{
		if (chainIndex > this->initialColors.size())
			chainIndex = static_cast<unsigned int>(this->initialColors.size()) - 1;
		
		this->initialColors[chainIndex]->setValue(color);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setInitialColour(chainIndex, Ogre::ColourValue(color.x, color.y, color.z, 1.0f));
		}
	}

	Ogre::Vector3 RibbonTrailComponent::getInitialColor(unsigned int chainIndex) const
	{
		if (index > this->initialColors.size())
			return Ogre::Vector3::ZERO;
		return this->initialColors[index]->getVector3();
	}

	void RibbonTrailComponent::setInitialWidth(unsigned int chainIndex, Ogre::Real width)
	{
		if (chainIndex > this->initialWidths.size())
			chainIndex = static_cast<unsigned int>(this->initialWidths.size()) - 1;
		
		this->initialWidths[chainIndex]->setValue(width);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setInitialWidth(chainIndex, width);
		}
	}

	Ogre::Real RibbonTrailComponent::getInitialWidth(unsigned int chainIndex) const
	{
		if (index > this->initialWidths.size())
			return 0.0f;
		return this->initialWidths[index]->getReal();
	}
	
	void RibbonTrailComponent::setFaceCameraId(unsigned long faceCameraId)
	{
		this->faceCameraId->setValue(faceCameraId);
		auto& cameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(faceCameraId);
		if (nullptr != cameraGameObject)
		{
			auto cameraCompPtr = NOWA::makeStrongPtr(cameraGameObject->getComponent<CameraComponent>());
			if (nullptr != cameraCompPtr)
			{
				this->camera = cameraCompPtr->getCamera();
			}
		}
	}
	
	unsigned long RibbonTrailComponent::getFaceCameraId(void) const
	{
		return this->faceCameraId->getULong();
	}

	void RibbonTrailComponent::setTexCoordChange(const Ogre::Vector2& texCoordChange)
	{
		this->textureCoordRange->setValue(texCoordChange);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setOtherTextureCoordRange(texCoordChange.x, texCoordChange.y);
		}
	}

	Ogre::Vector2 RibbonTrailComponent::getTexCoordChange(void) const
	{
		return this->textureCoordRange->getVector2();
	}

	void RibbonTrailComponent::setTrailLength(Ogre::Real trailLength)
	{
		this->trailLength->setValue(trailLength);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setTrailLength(trailLength);
		}
	}

	Ogre::Real RibbonTrailComponent::getTrailLength(void) const
	{
		return this->trailLength->getReal();
	}

	void RibbonTrailComponent::setRenderDistance(Ogre::Real renderDistance)
	{
		this->renderDistance->setValue(renderDistance);
		if (nullptr != this->ribbonTrail)
		{
			this->ribbonTrail->setRenderingDistance(renderDistance);
		}
	}

	Ogre::Real RibbonTrailComponent::getRenderDistance(void) const
	{
		return this->renderDistance->getReal();
	}

	/*void RibbonTrailComponent::setDirection(const Ogre::Vector3& direction)
	{
		this->direction->setValue(direction);
		if (nullptr != this->light)
		{
			this->light->setDirection(this->direction->getVector3());
		}
	}

	Ogre::Vector3 RibbonTrailComponent::getDirection(void) const
	{
		return this->direction->getVector3();
	}

	void RibbonTrailComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			this->light->setCastShadows(this->castShadows->getBool());
		}
	}

	bool RibbonTrailComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}*/

	Ogre::v1::RibbonTrail* RibbonTrailComponent::getRibbonTrail(void) const
	{
		return this->ribbonTrail;
	}

}; // namespace end