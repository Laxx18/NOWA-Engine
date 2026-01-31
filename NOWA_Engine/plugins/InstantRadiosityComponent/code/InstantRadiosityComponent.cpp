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
		irradianceCellSize(new Variant(InstantRadiosityComponent::AttrIrradianceCellSize(), 1.5f, this->attributes))
	{
		// Set constraints for properties
		this->numRays->setDescription("Number of rays to cast. Higher values are more accurate but slower. Power of 2 recommended.");
		this->numRayBounces->setDescription("Number of ray bounces for indirect light. 0-5 range.");
		this->numRayBounces->setConstraints(0, 5);
		this->numSpreadIterations->setDescription("Number of iterations for VPL clustering/spreading. 0-10 range.");
		this->numSpreadIterations->setConstraints(0, 10);
		this->cellSize->setDescription("Cell/cluster size for VPL grouping. Smaller = more accurate but slower.");
		this->cellSize->setConstraints(0.001f, 100.0f);
		this->bias->setDescription("Bias pushes VPL placement away from surfaces to reduce light leaking.");
		this->bias->setConstraints(0.0f, 1.0f);
		this->vplMaxRange->setDescription("Maximum range for VPLs. Smaller = faster and less leaking but lower reach.");
		this->vplMaxRange->setConstraints(0.1f, 1000.0f);
		this->vplPowerBoost->setDescription("Power boost multiplier for VPLs.");
		this->vplPowerBoost->setConstraints(0.01f, 100.0f);
		this->vplThreshold->setDescription("Threshold for removing weak VPLs. Higher = better performance.");
		this->vplThreshold->setConstraints(0.0f, 1.0f);
		this->vplUseIntensityForMaxRange->setDescription("Use light intensity to determine VPL max range.");
		this->vplIntensityRangeMultiplier->setDescription("Multiplier when using intensity for max range.");
		this->vplIntensityRangeMultiplier->setConstraints(0.01f, 1000.0f);
		this->enableDebugMarkers->setDescription("Show debug markers for VPL positions.");
		this->useIrradianceVolume->setDescription("Use Irradiance Volume instead of VPLs. Better performance but less accurate.");
		this->irradianceCellSize->setDescription("Cell size for Irradiance Volume.");
		this->irradianceCellSize->setConstraints(0.1f, 100.0f);
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
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void InstantRadiosityComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
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

		return success;
	}

	GameObjectCompPtr InstantRadiosityComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Only one instance allowed per scene
		return nullptr;
	}

	bool InstantRadiosityComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Init component for game object: " + this->gameObjectPtr->getName());

		if (true == this->activated->getBool())
		{
			this->createInstantRadiosity();
		}

		return true;
	}

	bool InstantRadiosityComponent::connect(void)
	{
		GameObjectComponent::connect();

		// Perform initial build after scene is loaded
		if (nullptr != this->instantRadiosity && true == this->activated->getBool())
		{
			this->internalBuild();
		}

		return true;
	}

	bool InstantRadiosityComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		return true;
	}

	void InstantRadiosityComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		this->destroyInstantRadiosity();
	}

	void InstantRadiosityComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Handle deferred updates
		if (nullptr != this->instantRadiosity && true == this->activated->getBool())
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
				}

				if (this->needsIrradianceVolumeUpdate)
				{
					this->updateIrradianceVolume();
					this->needsIrradianceVolumeUpdate = false;
				}
			};
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
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
	}

	void InstantRadiosityComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		// Clamp to valid range (max 32768 as per Ogre sample)
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
			this->instantRadiosity->setEnableDebugMarkers(enableDebugMarkers);
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

	Ogre::InstantRadiosity* InstantRadiosityComponent::getInstantRadiosity(void) const
	{
		return this->instantRadiosity;
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

			// Enable VPLs in Forward+ (required by InstantRadiosity)
			if (nullptr != sceneManager->getForwardPlus())
			{
				sceneManager->getForwardPlus()->setEnableVpls(true);
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
					"[InstantRadiosityComponent] Forward+ is not enabled! InstantRadiosity requires Forward+ with VPLs.");
				return;
			}

			this->instantRadiosity = new Ogre::InstantRadiosity(sceneManager, hlmsManager);

			// Apply all settings
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
			// this->instantRadiosity->mAoI.push_back()

			// Create irradiance volume if needed
			if (true == this->useIrradianceVolume->getBool())
			{
				this->irradianceVolume = new Ogre::IrradianceVolume(hlmsManager);
				Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));
				hlmsPbs->setIrradianceVolume(this->irradianceVolume);
				this->instantRadiosity->setUseIrradianceVolume(true);
			}

		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "InstantRadiosityComponent::createInstantRadiosity");


		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Created InstantRadiosity for scene.");
	}

	void InstantRadiosityComponent::destroyInstantRadiosity(void)
	{
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			if (nullptr != this->instantRadiosity)
			{
				// Disable irradiance volume in HlmsPbs
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
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "InstantRadiosityComponent::destroyInstantRadiosity");

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Destroyed InstantRadiosity.");
	}

	void InstantRadiosityComponent::internalBuild(void)
	{
		if (nullptr == this->instantRadiosity)
		{
			return;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Building InstantRadiosity...");

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{	
			this->instantRadiosity->build();

			if (true == this->useIrradianceVolume->getBool())
			{
				this->updateIrradianceVolume();
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "InstantRadiosityComponent::internalBuild");


		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] InstantRadiosity build complete.");
	}

	void InstantRadiosityComponent::updateIrradianceVolume(void)
	{
		// Threadsafe from the outside
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

		this->instantRadiosity->suggestIrradianceVolumeParameters(Ogre::Vector3(cellSize), volumeOrigin, lightMaxPower, numBlocksX, numBlocksY, numBlocksZ);

		this->irradianceVolume->createIrradianceVolumeTexture(numBlocksX, numBlocksY, numBlocksZ);

		this->instantRadiosity->fillIrradianceVolume(this->irradianceVolume, Ogre::Vector3(cellSize), volumeOrigin, lightMaxPower, false);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InstantRadiosityComponent] Irradiance volume updated.");
	}

	// Lua registration part

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
			.def("build", &InstantRadiosityComponent::build)
			.def("updateExistingVpls", &InstantRadiosityComponent::updateExistingVpls)
		];

		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "class inherits GameObjectComponent", InstantRadiosityComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setActivated(bool activated)", "Sets whether the component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool isActivated()", "Gets whether the component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setNumRays(unsigned int numRays)", "Sets the number of rays to cast (max 32768, power of 2 recommended).");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "unsigned int getNumRays()", "Gets the number of rays.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setNumRayBounces(unsigned int numRayBounces)", "Sets the number of ray bounces (0-5).");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "unsigned int getNumRayBounces()", "Gets the number of ray bounces.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setNumSpreadIterations(unsigned int numSpreadIterations)", "Sets the number of spread iterations for VPL clustering (0-10).");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "unsigned int getNumSpreadIterations()", "Gets the number of spread iterations.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setCellSize(float cellSize)", "Sets the cell/cluster size. Smaller = more accurate but slower.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getCellSize()", "Gets the cell size.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setBias(float bias)", "Sets the bias for VPL placement (0.0-1.0). Helps reduce light leaking.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getBias()", "Gets the bias value.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplMaxRange(float vplMaxRange)", "Sets the maximum range for VPLs.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplMaxRange()", "Gets the VPL maximum range.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplPowerBoost(float vplPowerBoost)", "Sets the power boost multiplier for VPLs.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplPowerBoost()", "Gets the VPL power boost.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplThreshold(float vplThreshold)", "Sets the threshold for removing weak VPLs.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplThreshold()", "Gets the VPL threshold.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplUseIntensityForMaxRange(bool useIntensityForMaxRange)", "Sets whether to use intensity for max range calculation.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool getVplUseIntensityForMaxRange()", "Gets whether intensity is used for max range.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setVplIntensityRangeMultiplier(float vplIntensityRangeMultiplier)", "Sets the intensity range multiplier.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getVplIntensityRangeMultiplier()", "Gets the VPL intensity range multiplier.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setEnableDebugMarkers(bool enableDebugMarkers)", "Sets whether to show debug markers for VPLs.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool getEnableDebugMarkers()", "Gets whether debug markers are enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setUseIrradianceVolume(bool useIrradianceVolume)", "Sets whether to use Irradiance Volume instead of VPLs.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "bool getUseIrradianceVolume()", "Gets whether irradiance volume is used.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void setIrradianceCellSize(float irradianceCellSize)", "Sets the irradiance volume cell size.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "float getIrradianceCellSize()", "Gets the irradiance volume cell size.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void build()", "Manually triggers a rebuild of the IR data. Call after scene changes.");
		LuaScriptApi::getInstance()->addClassToCollection("InstantRadiosityComponent", "void updateExistingVpls()", "Updates existing VPLs without full rebuild. Use for parameter changes.");

		gameObjectClass.def("getInstantRadiosityComponentFromName", &getInstantRadiosityComponentFromName);
		gameObjectClass.def("getInstantRadiosityComponent", (InstantRadiosityComponent * (*)(GameObject*)) & getInstantRadiosityComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InstantRadiosityComponent getInstantRadiosityComponent()", "Gets the component. This can be used if the game object just has one InstantRadiosityComponent.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InstantRadiosityComponent getInstantRadiosityComponentFromName(String name)", "Gets the component by name.");

		gameObjectControllerClass.def("castInstantRadiosityComponent", &GameObjectController::cast<InstantRadiosityComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "InstantRadiosityComponent castInstantRadiosityComponent(InstantRadiosityComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool InstantRadiosityComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Only allow one InstantRadiosityComponent per scene (check all game objects)
		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		for (auto& it : *gameObjects)
		{
			auto instantRadiosityCompPtr = NOWA::makeStrongPtr(it.second->getComponent<InstantRadiosityComponent>());
			if (nullptr != instantRadiosityCompPtr)
			{
				// Already exists in scene
				return false;
			}
		}
		return true;
	}

}; //namespace end