#include "NOWAPrecompiled.h"
#include "InstantRadiosityComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"
#include "InstantRadiosity/OgreInstantRadiosity.h"
#include "OgreIrradianceVolume.h"
#include "OgreForward3D.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreRoot.h"

// Hard dependency
#include "../../LightAreaOfInterestComponent/code/LightAreaOfInterestComponent.cpp"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	InstantRadiosityComponent::InstantRadiosityComponent()
		: GameObjectComponent(),
		name("InstantRadiosityComponent"),
		instantRadiosity(nullptr),
		irradianceVolume(nullptr),
		needsRebuild(false),
		needsVplUpdate(false),
		needsIrradianceVolumeUpdate(false),
		sceneBounds(Ogre::AxisAlignedBox::BOX_NULL),
		activated(new Variant(InstantRadiosityComponent::AttrActivated(), true, this->attributes)),
		numRays(new Variant(InstantRadiosityComponent::AttrNumRays(), static_cast<unsigned int>(128), this->attributes)),
		numRayBounces(new Variant(InstantRadiosityComponent::AttrNumRayBounces(), static_cast<unsigned int>(1), this->attributes)),
		numSpreadIterations(new Variant(InstantRadiosityComponent::AttrNumSpreadIterations(), static_cast<unsigned int>(1), this->attributes)),
		cellSize(new Variant(InstantRadiosityComponent::AttrCellSize(), 1.0f, this->attributes)),
		bias(new Variant(InstantRadiosityComponent::AttrBias(), 0.02f, this->attributes)),
		vplMaxRange(new Variant(InstantRadiosityComponent::AttrVplMaxRange(), 8.0f, this->attributes)),
		vplPowerBoost(new Variant(InstantRadiosityComponent::AttrVplPowerBoost(), 1.0f, this->attributes)),
		vplThreshold(new Variant(InstantRadiosityComponent::AttrVplThreshold(), 0.0005f, this->attributes)),
		vplUseIntensityForMaxRange(new Variant(InstantRadiosityComponent::AttrVplUseIntensityForMaxRange(), false, this->attributes)),
		vplIntensityRangeMultiplier(new Variant(InstantRadiosityComponent::AttrVplIntensityRangeMultiplier(), 100.0f, this->attributes)),
		enableDebugMarkers(new Variant(InstantRadiosityComponent::AttrEnableDebugMarkers(), false, this->attributes)),
		useIrradianceVolume(new Variant(InstantRadiosityComponent::AttrUseIrradianceVolume(), false, this->attributes)),
		irradianceCellSize(new Variant(InstantRadiosityComponent::AttrIrradianceCellSize(), 1.5f, this->attributes)),
		useSceneBoundsAsFallback(new Variant(InstantRadiosityComponent::AttrUseSceneBoundsAsFallback(), true, this->attributes))
	{
		this->numRays->setDescription("Number of rays to cast. Higher = more accurate but slower. Power of 2 recommended.");
		this->numRayBounces->setDescription("Number of ray bounces for indirect light. 0-5 range.");
		this->numRayBounces->setConstraints(0, 5);
		this->numSpreadIterations->setDescription("Number of iterations for VPL clustering. 0-10 range.");
		this->numSpreadIterations->setConstraints(0, 10);
		this->cellSize->setDescription("Cell size for VPL grouping. Smaller = more accurate but slower.");
		this->cellSize->setConstraints(0.001f, 100.0f);
		this->bias->setDescription("Bias pushes VPL placement away from surfaces to reduce light leaking.");
		this->bias->setConstraints(0.0f, 1.0f);
		this->vplMaxRange->setDescription("Maximum range for VPLs. Smaller = faster, less leaking but lower reach.");
		this->vplMaxRange->setConstraints(0.1f, 1000.0f);
		this->vplPowerBoost->setDescription("Power boost multiplier for VPLs.");
		this->vplPowerBoost->setConstraints(0.01f, 100.0f);
		this->vplThreshold->setDescription("Threshold for removing weak VPLs. Higher = better performance.");
		this->vplThreshold->setConstraints(0.0f, 1.0f);
		this->vplUseIntensityForMaxRange->setDescription("Use light intensity to determine VPL max range.");
		this->vplIntensityRangeMultiplier->setDescription("Multiplier when using intensity for max range.");
		this->vplIntensityRangeMultiplier->setConstraints(0.01f, 1000.0f);
		this->enableDebugMarkers->setDescription("Show debug markers for VPL positions.");
		this->useIrradianceVolume->setDescription("Use Irradiance Volume instead of VPLs.");
		this->irradianceCellSize->setDescription("Cell size for Irradiance Volume.");
		this->irradianceCellSize->setConstraints(0.1f, 100.0f);
		this->useSceneBoundsAsFallback->setDescription("Use scene bounds as AoI when no LightAreaOfInterestComponents exist.");
	}

	InstantRadiosityComponent::~InstantRadiosityComponent(void)
	{
	}

	void InstantRadiosityComponent::initialise()
	{
	}

	const Ogre::String& InstantRadiosityComponent::getName() const
	{
		return this->name;
	}

	void InstantRadiosityComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<InstantRadiosityComponent>(InstantRadiosityComponent::getStaticClassId(), InstantRadiosityComponent::getStaticClassName());
	}

	void InstantRadiosityComponent::shutdown()
	{
	}

	void InstantRadiosityComponent::uninstall()
	{
	}

	void InstantRadiosityComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool InstantRadiosityComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumRays")
		{
			this->numRays->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumRayBounces")
		{
			this->numRayBounces->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSpreadIterations")
		{
			this->numSpreadIterations->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CellSize")
		{
			this->cellSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Bias")
		{
			this->bias->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VplMaxRange")
		{
			this->vplMaxRange->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VplPowerBoost")
		{
			this->vplPowerBoost->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VplThreshold")
		{
			this->vplThreshold->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VplUseIntensityForMaxRange")
		{
			this->vplUseIntensityForMaxRange->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VplIntensityRangeMultiplier")
		{
			this->vplIntensityRangeMultiplier->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableDebugMarkers")
		{
			this->enableDebugMarkers->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseIrradianceVolume")
		{
			this->useIrradianceVolume->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IrradianceCellSize")
		{
			this->irradianceCellSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseSceneBoundsAsFallback")
		{
			this->useSceneBoundsAsFallback->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr InstantRadiosityComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr; // Only one instance allowed per scene
	}

	bool InstantRadiosityComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Init component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &InstantRadiosityComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

		if (true == this->activated->getBool())
		{
			this->createInstantRadiosity();
		}

		return true;
	}

	bool InstantRadiosityComponent::connect(void)
	{
		GameObjectComponent::connect();

		if (nullptr != this->instantRadiosity && true == this->activated->getBool())
		{
			this->sceneBounds.setExtents(Core::getSingletonPtr()->getCurrentSceneBoundLeftNear(), Core::getSingletonPtr()->getCurrentSceneBoundRightFar());

			this->collectAreasOfInterest();
			this->needsRebuild = true;
		}

		return true;
	}

	bool InstantRadiosityComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		return true;
	}

	void InstantRadiosityComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &InstantRadiosityComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		this->destroyInstantRadiosity();
	}

	void InstantRadiosityComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != this->instantRadiosity && true == this->activated->getBool())
		{
			if (this->needsRebuild || this->needsVplUpdate || this->needsIrradianceVolumeUpdate)
			{
				auto closureFunction = [this](Ogre::Real renderDt)
				{
					if (this->needsRebuild)
					{
						this->internalBuild();
						this->needsRebuild = false;
						this->needsVplUpdate = false;
						this->needsIrradianceVolumeUpdate = false;
					}
					else if (this->needsVplUpdate)
					{
						this->instantRadiosity->updateExistingVpls();
						this->needsVplUpdate = false;

						if (this->needsIrradianceVolumeUpdate)
						{
							this->internalUpdateIrradianceVolume();
							this->needsIrradianceVolumeUpdate = false;
						}
					}
					else if (this->needsIrradianceVolumeUpdate)
					{
						this->internalUpdateIrradianceVolume();
						this->needsIrradianceVolumeUpdate = false;
					}
				};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
			}
		}
	}

	void InstantRadiosityComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (InstantRadiosityComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (InstantRadiosityComponent::AttrNumRays() == attribute->getName())
		{
			this->setNumRays(attribute->getUInt());
		}
		else if (InstantRadiosityComponent::AttrNumRayBounces() == attribute->getName())
		{
			this->setNumRayBounces(attribute->getUInt());
		}
		else if (InstantRadiosityComponent::AttrNumSpreadIterations() == attribute->getName())
		{
			this->setNumSpreadIterations(attribute->getUInt());
		}
		else if (InstantRadiosityComponent::AttrCellSize() == attribute->getName())
		{
			this->setCellSize(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrBias() == attribute->getName())
		{
			this->setBias(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrVplMaxRange() == attribute->getName())
		{
			this->setVplMaxRange(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrVplPowerBoost() == attribute->getName())
		{
			this->setVplPowerBoost(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrVplThreshold() == attribute->getName())
		{
			this->setVplThreshold(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrVplUseIntensityForMaxRange() == attribute->getName())
		{
			this->setVplUseIntensityForMaxRange(attribute->getBool());
		}
		else if (InstantRadiosityComponent::AttrVplIntensityRangeMultiplier() == attribute->getName())
		{
			this->setVplIntensityRangeMultiplier(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrEnableDebugMarkers() == attribute->getName())
		{
			this->setEnableDebugMarkers(attribute->getBool());
		}
		else if (InstantRadiosityComponent::AttrUseIrradianceVolume() == attribute->getName())
		{
			this->setUseIrradianceVolume(attribute->getBool());
		}
		else if (InstantRadiosityComponent::AttrIrradianceCellSize() == attribute->getName())
		{
			this->setIrradianceCellSize(attribute->getReal());
		}
		else if (InstantRadiosityComponent::AttrUseSceneBoundsAsFallback() == attribute->getName())
		{
			this->setUseSceneBoundsAsFallback(attribute->getBool());
		}
	}

	void InstantRadiosityComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NumRays"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numRays->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NumRayBounces"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numRayBounces->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NumSpreadIterations"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numSpreadIterations->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CellSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cellSize->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Bias"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bias->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VplMaxRange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vplMaxRange->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VplPowerBoost"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vplPowerBoost->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VplThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vplThreshold->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VplUseIntensityForMaxRange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vplUseIntensityForMaxRange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VplIntensityRangeMultiplier"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vplIntensityRangeMultiplier->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableDebugMarkers"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableDebugMarkers->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseIrradianceVolume"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useIrradianceVolume->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "IrradianceCellSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->irradianceCellSize->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseSceneBoundsAsFallback"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useSceneBoundsAsFallback->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String InstantRadiosityComponent::getClassName(void) const
	{
		return "InstantRadiosityComponent";
	}

	Ogre::String InstantRadiosityComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void InstantRadiosityComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == activated && nullptr == this->instantRadiosity)
		{
			this->createInstantRadiosity();
			this->collectAreasOfInterest();
			this->needsRebuild = true;
		}
		else if (false == activated && nullptr != this->instantRadiosity)
		{
			this->destroyInstantRadiosity();
		}
	}

	bool InstantRadiosityComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void InstantRadiosityComponent::setNumRays(unsigned int numRays)
	{
		numRays = std::max<unsigned int>(1, std::min<unsigned int>(32768, numRays));
		this->numRays->setValue(numRays);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mNumRays = static_cast<size_t>(numRays);
			this->needsRebuild = true;
		}
	}

	unsigned int InstantRadiosityComponent::getNumRays(void) const
	{
		return this->numRays->getUInt();
	}

	void InstantRadiosityComponent::setNumRayBounces(unsigned int numRayBounces)
	{
		numRayBounces = std::min<unsigned int>(5, numRayBounces);
		this->numRayBounces->setValue(numRayBounces);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mNumRayBounces = numRayBounces;
			this->needsRebuild = true;
		}
	}

	unsigned int InstantRadiosityComponent::getNumRayBounces(void) const
	{
		return this->numRayBounces->getUInt();
	}

	void InstantRadiosityComponent::setNumSpreadIterations(unsigned int numSpreadIterations)
	{
		numSpreadIterations = std::min<unsigned int>(10, numSpreadIterations);
		this->numSpreadIterations->setValue(numSpreadIterations);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mNumSpreadIterations = numSpreadIterations;
			this->needsRebuild = true;
		}
	}

	unsigned int InstantRadiosityComponent::getNumSpreadIterations(void) const
	{
		return this->numSpreadIterations->getUInt();
	}

	void InstantRadiosityComponent::setCellSize(Ogre::Real cellSize)
	{
		cellSize = std::max(0.001f, cellSize);
		this->cellSize->setValue(cellSize);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mCellSize = cellSize;
			this->needsRebuild = true;
		}
	}

	Ogre::Real InstantRadiosityComponent::getCellSize(void) const
	{
		return this->cellSize->getReal();
	}

	void InstantRadiosityComponent::setBias(Ogre::Real bias)
	{
		bias = Ogre::Math::Clamp(bias, 0.0f, 1.0f);
		this->bias->setValue(bias);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mBias = bias;
			this->needsRebuild = true;
		}
	}

	Ogre::Real InstantRadiosityComponent::getBias(void) const
	{
		return this->bias->getReal();
	}

	void InstantRadiosityComponent::setVplMaxRange(Ogre::Real vplMaxRange)
	{
		this->vplMaxRange->setValue(vplMaxRange);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mVplMaxRange = vplMaxRange;
			this->needsVplUpdate = true;
			this->needsIrradianceVolumeUpdate = true;
		}
	}

	Ogre::Real InstantRadiosityComponent::getVplMaxRange(void) const
	{
		return this->vplMaxRange->getReal();
	}

	void InstantRadiosityComponent::setVplPowerBoost(Ogre::Real vplPowerBoost)
	{
		this->vplPowerBoost->setValue(vplPowerBoost);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mVplPowerBoost = vplPowerBoost;
			this->needsVplUpdate = true;
			this->needsIrradianceVolumeUpdate = true;
		}
	}

	Ogre::Real InstantRadiosityComponent::getVplPowerBoost(void) const
	{
		return this->vplPowerBoost->getReal();
	}

	void InstantRadiosityComponent::setVplThreshold(Ogre::Real vplThreshold)
	{
		this->vplThreshold->setValue(vplThreshold);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mVplThreshold = vplThreshold;
			this->needsVplUpdate = true;
		}
	}

	Ogre::Real InstantRadiosityComponent::getVplThreshold(void) const
	{
		return this->vplThreshold->getReal();
	}

	void InstantRadiosityComponent::setVplUseIntensityForMaxRange(bool useIntensityForMaxRange)
	{
		this->vplUseIntensityForMaxRange->setValue(useIntensityForMaxRange);

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mVplUseIntensityForMaxRange = useIntensityForMaxRange;
			this->needsVplUpdate = true;
			this->needsIrradianceVolumeUpdate = true;
		}
	}

	bool InstantRadiosityComponent::getVplUseIntensityForMaxRange(void) const
	{
		return this->vplUseIntensityForMaxRange->getBool();
	}

	void InstantRadiosityComponent::setVplIntensityRangeMultiplier(Ogre::Real vplIntensityRangeMultiplier)
	{
		vplIntensityRangeMultiplier = std::max(0.01, static_cast<double>(vplIntensityRangeMultiplier));
		this->vplIntensityRangeMultiplier->setValue(static_cast<Ogre::Real>(vplIntensityRangeMultiplier));

		if (nullptr != this->instantRadiosity)
		{
			this->instantRadiosity->mVplIntensityRangeMultiplier = vplIntensityRangeMultiplier;
			this->needsVplUpdate = true;
			this->needsIrradianceVolumeUpdate = true;
		}
	}

	Ogre::Real InstantRadiosityComponent::getVplIntensityRangeMultiplier(void) const
	{
		return this->vplIntensityRangeMultiplier->getReal();
	}

	void InstantRadiosityComponent::setEnableDebugMarkers(bool enableDebugMarkers)
	{
		this->enableDebugMarkers->setValue(enableDebugMarkers);

		if (nullptr != this->instantRadiosity)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, enableDebugMarkers]()
				{
					this->instantRadiosity->setEnableDebugMarkers(enableDebugMarkers);
				};
			NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "InstantRadiosityComponent::setEnableDebugMarkers");
		}
	}

	bool InstantRadiosityComponent::getEnableDebugMarkers(void) const
	{
		return this->enableDebugMarkers->getBool();
	}

	void InstantRadiosityComponent::setUseIrradianceVolume(bool useIrradianceVolume)
	{
		this->useIrradianceVolume->setValue(useIrradianceVolume);

		if (nullptr == this->instantRadiosity)
		{
			return;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this, useIrradianceVolume]()
			{
				Ogre::Root* root = Ogre::Root::getSingletonPtr();
				Ogre::HlmsManager* hlmsManager = root->getHlmsManager();
				Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

				if (true == useIrradianceVolume)
				{
					if (nullptr == this->irradianceVolume)
					{
						this->irradianceVolume = new Ogre::IrradianceVolume(hlmsManager);
					}
					hlmsPbs->setIrradianceVolume(this->irradianceVolume);
					this->instantRadiosity->setUseIrradianceVolume(true);
					this->needsIrradianceVolumeUpdate = true;
				}
				else
				{
					hlmsPbs->setIrradianceVolume(nullptr);
					this->instantRadiosity->setUseIrradianceVolume(false);
				}
			};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "InstantRadiosityComponent::setUseIrradianceVolume");
	}

	bool InstantRadiosityComponent::getUseIrradianceVolume(void) const
	{
		return this->useIrradianceVolume->getBool();
	}

	void InstantRadiosityComponent::setIrradianceCellSize(Ogre::Real irradianceCellSize)
	{
		irradianceCellSize = std::max(0.1f, irradianceCellSize);
		this->irradianceCellSize->setValue(irradianceCellSize);
		this->needsIrradianceVolumeUpdate = true;
	}

	Ogre::Real InstantRadiosityComponent::getIrradianceCellSize(void) const
	{
		return this->irradianceCellSize->getReal();
	}

	void InstantRadiosityComponent::setUseSceneBoundsAsFallback(bool useSceneBoundsAsFallback)
	{
		this->useSceneBoundsAsFallback->setValue(useSceneBoundsAsFallback);
	}

	bool InstantRadiosityComponent::getUseSceneBoundsAsFallback(void) const
	{
		return this->useSceneBoundsAsFallback->getBool();
	}

	void InstantRadiosityComponent::build(void)
	{
		if (nullptr != this->instantRadiosity)
		{
			this->needsRebuild = true;
		}
	}

	void InstantRadiosityComponent::updateExistingVpls(void)
	{
		if (nullptr != this->instantRadiosity)
		{
			this->needsVplUpdate = true;
		}
	}

	void InstantRadiosityComponent::refreshAreasOfInterest(void)
	{
		if (nullptr != this->instantRadiosity)
		{
			this->collectAreasOfInterest();
			this->needsRebuild = true;
		}
	}

	Ogre::InstantRadiosity* InstantRadiosityComponent::getInstantRadiosity(void) const
	{
		return this->instantRadiosity;
	}

	void InstantRadiosityComponent::collectAreasOfInterest(void)
	{
		if (nullptr == this->instantRadiosity)
		{
			return;
		}

		this->instantRadiosity->mAoI.clear();

		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		unsigned int aoiCount = 0;

		for (auto& it : *gameObjects)
		{
			auto lightAoiCompPtr = NOWA::makeStrongPtr(it.second->getComponent<LightAreaOfInterestComponent>());
			if (nullptr != lightAoiCompPtr && lightAoiCompPtr->isActivated())
			{
				Ogre::Vector3 center = lightAoiCompPtr->getCenter();
				Ogre::Vector3 halfSize = lightAoiCompPtr->getHalfSize();
				Ogre::Real sphereRadius = lightAoiCompPtr->getSphereRadius();

				this->instantRadiosity->mAoI.push_back(Ogre::InstantRadiosity::AreaOfInterest(Ogre::Aabb(center, halfSize), sphereRadius));
				aoiCount++;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Added AoI from '" + it.second->getName() + "' at " + Ogre::StringConverter::toString(center));
			}
		}

		if (0 == aoiCount && true == this->useSceneBoundsAsFallback->getBool())
		{
			if (!this->sceneBounds.isNull() && !this->sceneBounds.isInfinite())
			{
				Ogre::Vector3 center = this->sceneBounds.getCenter();
				Ogre::Vector3 halfSize = this->sceneBounds.getHalfSize();
				Ogre::Real sphereRadius = halfSize.length();

				this->instantRadiosity->mAoI.push_back(Ogre::InstantRadiosity::AreaOfInterest(Ogre::Aabb(center, halfSize), sphereRadius));
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Collected " + Ogre::StringConverter::toString(this->instantRadiosity->mAoI.size()) + " Areas of Interest");
	}

	void InstantRadiosityComponent::handleUpdateBounds(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);
		this->sceneBounds.setExtents(castEventData->getCalculatedBounds().first, castEventData->getCalculatedBounds().second);
	}

	void InstantRadiosityComponent::createInstantRadiosity(void)
	{
		if (nullptr != this->instantRadiosity)
		{
			return;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
			{
				Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
				Ogre::Root* root = Ogre::Root::getSingletonPtr();
				Ogre::HlmsManager* hlmsManager = root->getHlmsManager();

				if (nullptr != sceneManager->getForwardPlus())
				{
					sceneManager->getForwardPlus()->setEnableVpls(true);
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
						"[InstantRadiosityComponent] Forward+ is not enabled!");
					return;
				}

				this->instantRadiosity = new Ogre::InstantRadiosity(sceneManager, hlmsManager);

				this->instantRadiosity->mNumRays = static_cast<size_t>(this->numRays->getUInt());
				this->instantRadiosity->mNumRayBounces = this->numRayBounces->getUInt();
				this->instantRadiosity->mNumSpreadIterations = this->numSpreadIterations->getUInt();
				this->instantRadiosity->mCellSize = this->cellSize->getReal();
				this->instantRadiosity->mBias = this->bias->getReal();
				this->instantRadiosity->mVplMaxRange = this->vplMaxRange->getReal();
				this->instantRadiosity->mVplPowerBoost = this->vplPowerBoost->getReal();
				this->instantRadiosity->mVplThreshold = this->vplThreshold->getReal();
				this->instantRadiosity->mVplUseIntensityForMaxRange = this->vplUseIntensityForMaxRange->getBool();
				this->instantRadiosity->mVplIntensityRangeMultiplier = this->vplIntensityRangeMultiplier->getReal();
				this->instantRadiosity->setEnableDebugMarkers(this->enableDebugMarkers->getBool());

				if (true == this->useIrradianceVolume->getBool())
				{
					this->irradianceVolume = new Ogre::IrradianceVolume(hlmsManager);
					Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));
					hlmsPbs->setIrradianceVolume(this->irradianceVolume);
					this->instantRadiosity->setUseIrradianceVolume(true);
				}

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Created InstantRadiosity.");
			};

		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "InstantRadiosityComponent::createInstantRadiosity");
	}

	void InstantRadiosityComponent::destroyInstantRadiosity(void)
	{
		if (nullptr == this->instantRadiosity && nullptr == this->irradianceVolume)
		{
			return;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
			{
				if (nullptr != this->instantRadiosity)
				{
					Ogre::Root* root = Ogre::Root::getSingletonPtr();
					if (nullptr != root)
					{
						Ogre::HlmsManager* hlmsManager = root->getHlmsManager();
						if (nullptr != hlmsManager)
						{
							Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));
							if (nullptr != hlmsPbs)
							{
								hlmsPbs->setIrradianceVolume(nullptr);
							}
						}
					}

					delete this->instantRadiosity;
					this->instantRadiosity = nullptr;
				}

				if (nullptr != this->irradianceVolume)
				{
					delete this->irradianceVolume;
					this->irradianceVolume = nullptr;
				}

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Destroyed InstantRadiosity.");
			};

		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "InstantRadiosityComponent::destroyInstantRadiosity");
	}

	void InstantRadiosityComponent::internalBuild(void)
	{
		if (nullptr == this->instantRadiosity)
		{
			return;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Building...");
		this->instantRadiosity->build();

		if (true == this->useIrradianceVolume->getBool())
		{
			this->internalUpdateIrradianceVolume();
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Build complete.");
	}

	void InstantRadiosityComponent::internalUpdateIrradianceVolume(void)
	{
		if (nullptr == this->instantRadiosity || nullptr == this->irradianceVolume)
		{
			return;
		}

		Ogre::Root* root = Ogre::Root::getSingletonPtr();
		Ogre::HlmsManager* hlmsManager = root->getHlmsManager();
		Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

		if (nullptr == hlmsPbs->getIrradianceVolume())
		{
			return;
		}

		Ogre::Vector3 volumeOrigin;
		Ogre::Real lightMaxPower;
		Ogre::uint32 numBlocksX, numBlocksY, numBlocksZ;
		Ogre::Real cellSize = this->irradianceCellSize->getReal();

		this->instantRadiosity->suggestIrradianceVolumeParameters(
			Ogre::Vector3(cellSize), volumeOrigin, lightMaxPower, numBlocksX, numBlocksY, numBlocksZ);

		this->irradianceVolume->createIrradianceVolumeTexture(numBlocksX, numBlocksY, numBlocksZ);
		this->instantRadiosity->fillIrradianceVolume(
			this->irradianceVolume, Ogre::Vector3(cellSize), volumeOrigin, lightMaxPower, false);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Irradiance volume updated.");
	}

	// Lua registration

	InstantRadiosityComponent* getInstantRadiosityComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<InstantRadiosityComponent>(gameObject->getComponentWithOccurrence<InstantRadiosityComponent>(occurrenceIndex)).get();
	}

	InstantRadiosityComponent* getInstantRadiosityComponent(GameObject* gameObject)
	{
		return makeStrongPtr<InstantRadiosityComponent>(gameObject->getComponent<InstantRadiosityComponent>()).get();
	}

	InstantRadiosityComponent* getInstantRadiosityComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<InstantRadiosityComponent>(gameObject->getComponentFromName<InstantRadiosityComponent>(name)).get();
	}

	void InstantRadiosityComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<InstantRadiosityComponent, GameObjectComponent>("InstantRadiosityComponent")
			.def("setActivated", &InstantRadiosityComponent::setActivated)
			.def("isActivated", &InstantRadiosityComponent::isActivated)
			.def("setNumRays", &InstantRadiosityComponent::setNumRays)
			.def("getNumRays", &InstantRadiosityComponent::getNumRays)
			.def("setNumRayBounces", &InstantRadiosityComponent::setNumRayBounces)
			.def("getNumRayBounces", &InstantRadiosityComponent::getNumRayBounces)
			.def("setNumSpreadIterations", &InstantRadiosityComponent::setNumSpreadIterations)
			.def("getNumSpreadIterations", &InstantRadiosityComponent::getNumSpreadIterations)
			.def("setCellSize", &InstantRadiosityComponent::setCellSize)
			.def("getCellSize", &InstantRadiosityComponent::getCellSize)
			.def("setBias", &InstantRadiosityComponent::setBias)
			.def("getBias", &InstantRadiosityComponent::getBias)
			.def("setVplMaxRange", &InstantRadiosityComponent::setVplMaxRange)
			.def("getVplMaxRange", &InstantRadiosityComponent::getVplMaxRange)
			.def("setVplPowerBoost", &InstantRadiosityComponent::setVplPowerBoost)
			.def("getVplPowerBoost", &InstantRadiosityComponent::getVplPowerBoost)
			.def("setVplThreshold", &InstantRadiosityComponent::setVplThreshold)
			.def("getVplThreshold", &InstantRadiosityComponent::getVplThreshold)
			.def("setVplUseIntensityForMaxRange", &InstantRadiosityComponent::setVplUseIntensityForMaxRange)
			.def("getVplUseIntensityForMaxRange", &InstantRadiosityComponent::getVplUseIntensityForMaxRange)
			.def("setVplIntensityRangeMultiplier", &InstantRadiosityComponent::setVplIntensityRangeMultiplier)
			.def("getVplIntensityRangeMultiplier", &InstantRadiosityComponent::getVplIntensityRangeMultiplier)
			.def("setEnableDebugMarkers", &InstantRadiosityComponent::setEnableDebugMarkers)
			.def("getEnableDebugMarkers", &InstantRadiosityComponent::getEnableDebugMarkers)
			.def("setUseIrradianceVolume", &InstantRadiosityComponent::setUseIrradianceVolume)
			.def("getUseIrradianceVolume", &InstantRadiosityComponent::getUseIrradianceVolume)
			.def("setIrradianceCellSize", &InstantRadiosityComponent::setIrradianceCellSize)
			.def("getIrradianceCellSize", &InstantRadiosityComponent::getIrradianceCellSize)
			.def("setUseSceneBoundsAsFallback", &InstantRadiosityComponent::setUseSceneBoundsAsFallback)
			.def("getUseSceneBoundsAsFallback", &InstantRadiosityComponent::getUseSceneBoundsAsFallback)
			.def("build", &InstantRadiosityComponent::build)
			.def("updateExistingVpls", &InstantRadiosityComponent::updateExistingVpls)
			.def("refreshAreasOfInterest", &InstantRadiosityComponent::refreshAreasOfInterest)
		];

		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "class inherits GameObjectComponent", InstantRadiosityComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setActivated(bool activated)", "Sets whether the component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool isActivated()", "Gets whether the component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setNumRays(unsigned int numRays)", "Sets the number of rays (max 32768).");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "unsigned int getNumRays()", "Gets the number of rays.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setNumRayBounces(unsigned int numRayBounces)", "Sets the number of ray bounces (0-5).");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "unsigned int getNumRayBounces()", "Gets the number of ray bounces.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setCellSize(float cellSize)", "Sets the cell size.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getCellSize()", "Gets the cell size.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setBias(float bias)", "Sets the bias (0.0-1.0).");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getBias()", "Gets the bias.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplMaxRange(float vplMaxRange)", "Sets the VPL max range.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplMaxRange()", "Gets the VPL max range.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplPowerBoost(float vplPowerBoost)", "Sets the VPL power boost.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplPowerBoost()", "Gets the VPL power boost.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplThreshold(float vplThreshold)", "Sets the VPL threshold.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplThreshold()", "Gets the VPL threshold.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setEnableDebugMarkers(bool enableDebugMarkers)", "Sets whether debug markers are shown.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool getEnableDebugMarkers()", "Gets whether debug markers are shown.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setUseIrradianceVolume(bool useIrradianceVolume)", "Sets whether to use irradiance volume.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool getUseIrradianceVolume()", "Gets whether irradiance volume is used.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void build()", "Manually triggers a rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void updateExistingVpls()", "Updates existing VPLs without full rebuild.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void refreshAreasOfInterest()", "Refreshes AoIs from all LightAreaOfInterestComponents.");

		gameObjectClass.def("getInstantRadiosityComponentFromName", &getInstantRadiosityComponentFromName);
		gameObjectClass.def("getInstantRadiosityComponent", (InstantRadiosityComponent * (*)(GameObject*)) & getInstantRadiosityComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InstantRadiosityComponent getInstantRadiosityComponent()", "Gets the component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InstantRadiosityComponent getInstantRadiosityComponentFromName(String name)", "Gets the component by name.");

		gameObjectControllerClass.def("castInstantRadiosityComponent", &GameObjectController::cast<InstantRadiosityComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "InstantRadiosityComponent castInstantRadiosityComponent(InstantRadiosityComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool InstantRadiosityComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		for (auto& it : *gameObjects)
		{
			auto instantRadiosityCompPtr = NOWA::makeStrongPtr(it.second->getComponent<InstantRadiosityComponent>());
			if (nullptr != instantRadiosityCompPtr)
			{
				return false; // Only one allowed per scene
			}
		}
		return true;
	}

}; //namespace end