#include "NOWAPrecompiled.h"
#include "VehicleComponent.h"
#include "PhysicsActiveComponent.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	VehicleComponent::VehicleComponent()
		: GameObjectComponent(),
		engineMass(new Variant(VehicleComponent::AttrEngineMass(), 100.0f, this->attributes)),
		engineRatio(new Variant(VehicleComponent::AttrEngineRatio(), 0.125f, this->attributes)),
		axelSteerAngles(new Variant(VehicleComponent::AttrAxelSteerAngles(), Ogre::Vector2(25.0f, 0.0f), this->attributes)),
		weightDistribution(new Variant(VehicleComponent::AttrWeightDistribution(), 0.6f, this->attributes)),
		clutchFrictionTorque(new Variant(VehicleComponent::AttrClutchFrictionTorque(), 2000.0f, this->attributes)),
		idleTorqueRpm(new Variant(VehicleComponent::AttrIdleTorqueRpm(), Ogre::Vector2(100.0f, 450.0f), this->attributes)),
		peakTorqueRpm(new Variant(VehicleComponent::AttrPeakTorqueRpm(), Ogre::Vector2(500.0f, 3000.0f), this->attributes)),
		peakHpRpm(new Variant(VehicleComponent::AttrPeakHpRpm(), Ogre::Vector2(400.0f, 5200.0f), this->attributes)),
		redlineTorqueRpm(new Variant(VehicleComponent::AttrRedlineTorqueRpm(), 6000.0f, this->attributes)),
		topSpeedKmh(new Variant(VehicleComponent::AttrTopSpeedKmh(), 264.0f, this->attributes)),
		tireCorningStiffness(new Variant(VehicleComponent::AttrTireCorningStiffness(), 4.0f, this->attributes)),
		tireAligningMomentTrail(new Variant(VehicleComponent::AttrTireAligningMomentTrail(), 0.5f, this->attributes)),
		tireSuspensionSpring(new Variant(VehicleComponent::AttrTireSuspensionSpring(), Ogre::Vector3(30000.0f, 700.0f, 0.25f), this->attributes)),
		tireBrakeTorque(new Variant(VehicleComponent::AttrTireBrakeTorque(), 2000.0f, this->attributes)),
		tirePivotOffsetZY(new Variant(VehicleComponent::AttrTirePivotOffsetZY(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		tireGear123(new Variant(VehicleComponent::AttrTireGear123(), Ogre::Vector3(2.66f, 1.78f, 1.30f), this->attributes)),
		tireGear456(new Variant(VehicleComponent::AttrTireGear456(), Ogre::Vector3(1.0f, 0.74f, 0.50f), this->attributes)),
		tireReverseGear(new Variant(VehicleComponent::AttrTireReverseGear(), 2.9f, this->attributes)),
		comYOffset(new Variant(VehicleComponent::AttrComYOffset(), -0.35f, this->attributes)),
		downForceWeightFactors(new Variant(VehicleComponent::AttrDownForceWeightFactors(), Ogre::Vector3(1.0f, 2.0f, 80.0f), this->attributes)),
		wheelHasFender(new Variant(VehicleComponent::AttrWheelHasFender(), true, this->attributes)),
		differentialType(new Variant(VehicleComponent::AttrDifferentialType(), static_cast<unsigned int>(0), this->attributes)),
		tireSuspensionType(new Variant(VehicleComponent::AttrTireSuspensionType(), static_cast<unsigned int>(1), this->attributes)),
		lfTireId(new Variant(VehicleComponent::AttrLFTireId(), static_cast<unsigned long>(0), this->attributes, true)),
		rfTireId(new Variant(VehicleComponent::AttrRFTireId(), static_cast<unsigned long>(0), this->attributes, true)),
		lrTireId(new Variant(VehicleComponent::AttrLRTireId(), static_cast<unsigned long>(0), this->attributes, true)),
		rrTireId(new Variant(VehicleComponent::AttrRRTireId(), static_cast<unsigned long>(0), this->attributes, true))/*,
		vehicle(nullptr)*/
	{
		this->downForceWeightFactors->setDescription("Factor0, factor1, factor-Speed");
		this->axelSteerAngles->setDescription("1 factor is the front steering and second factor the rear steering");
		this->tireSuspensionSpring->setDescription("1 factor is the spring, 2 factor the damping and 3 factor the length");
		this->peakHpRpm->setDescription("Horse power (Hp)");
		this->engineRatio->setDescription("Engine rotor ratio");
		this->tireSuspensionType->setDescription("0 = offroad, 1 = confort, 2 = race, 3 = roller");
	}

	VehicleComponent::~VehicleComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VehicleComponent] Destructor vehicle component for game object: " + this->gameObjectPtr->getName());
#if 0
		if (nullptr != this->vehicle)
		{
			delete this->vehicle;
			this->vehicle = nullptr;
		}
#endif
	}

	bool VehicleComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EngineMass")
		{
			this->engineMass->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EngineRatio")
		{
			this->engineRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AxelSteerAngles")
		{
			this->axelSteerAngles->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WeightDistribution")
		{
			this->weightDistribution->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ClutchFrictionTorque")
		{
			this->clutchFrictionTorque->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IdleTorqueRpm")
		{
			this->idleTorqueRpm->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PeakTorqueRpm")
		{
			this->peakTorqueRpm->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PeakHpRpm")
		{
			this->peakHpRpm->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RedlineTorqueRpm")
		{
			this->redlineTorqueRpm->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TopSpeedKmh")
		{
			this->topSpeedKmh->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireCorningStiffness")
		{
			this->tireCorningStiffness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireAligningMomentTrail")
		{
			this->tireAligningMomentTrail->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireSuspensionSpring")
		{
			this->tireSuspensionSpring->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireBrakeTorque")
		{
			this->tireBrakeTorque->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TirePivotOffsetZY")
		{
			this->tirePivotOffsetZY->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireGear123")
		{
			this->tireGear123->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireGear456")
		{
			this->tireGear456->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireReverseGear")
		{
			this->tireReverseGear->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ComYOffset")
		{
			this->comYOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DownForceWeightFactors")
		{
			this->downForceWeightFactors->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WheelHasFender")
		{
			this->wheelHasFender->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DifferentialType")
		{
			this->differentialType->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireSuspensionType")
		{
			this->tireSuspensionType->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LFTireId")
		{
			this->lfTireId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RFTireId")
		{
			this->rfTireId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LRTireId")
		{
			this->lrTireId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RRTireId")
		{
			this->rrTireId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	bool VehicleComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VehicleComponent] Init vehicle component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}
	
	GameObjectCompPtr VehicleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		VehicleCompPtr clonedCompPtr(boost::make_shared<VehicleComponent>());

		
		// clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedCompPtr->setEngineMass(this->engineMass->getReal());
		clonedCompPtr->setEngineRatio(this->engineRatio->getReal());
		clonedCompPtr->setAxelSteerAngles(this->axelSteerAngles->getVector2());
		clonedCompPtr->setWeightDistribution(this->weightDistribution->getReal());
		clonedCompPtr->setClutchFrictionTorque(this->clutchFrictionTorque->getReal());
		clonedCompPtr->setIdleTorqueRpm(this->idleTorqueRpm->getVector2());
		clonedCompPtr->setPeakTorqueRpm(this->peakTorqueRpm->getVector2());
		clonedCompPtr->setPeakHpRpm(this->peakHpRpm->getVector2());
		clonedCompPtr->setRedlineTorqueRpm(this->redlineTorqueRpm->getReal());
		clonedCompPtr->setTopSpeedKmh(this->topSpeedKmh->getReal());
		clonedCompPtr->setTireCorningStiffness(this->tireCorningStiffness->getReal());
		clonedCompPtr->setTireAligningMomentTrail(this->tireAligningMomentTrail->getReal());
		clonedCompPtr->setTireSuspensionSpring(this->tireSuspensionSpring->getVector3());
		clonedCompPtr->setTireBrakeTorque(this->tireBrakeTorque->getReal());
		clonedCompPtr->setTirePivotOffsetZY(this->tirePivotOffsetZY->getVector2());
		clonedCompPtr->setTireGear123(this->tireGear123->getVector3());
		clonedCompPtr->setTireGear456(this->tireGear456->getVector3());
		clonedCompPtr->setTireReverseGear(this->tireReverseGear->getReal());
		clonedCompPtr->setComYOffset(this->comYOffset->getReal());
		clonedCompPtr->setDownForceWeightFactors(this->downForceWeightFactors->getVector3());
		clonedCompPtr->setWheelHasFender(this->wheelHasFender->getBool());
		clonedCompPtr->setDifferentialType(this->differentialType->getUInt());
		clonedCompPtr->setTireSuspensionType(this->tireSuspensionType->getUInt());
		clonedCompPtr->setLFTireId(this->lfTireId->getULong());
		clonedCompPtr->setRFTireId(this->rfTireId->getULong());
		clonedCompPtr->setLRTireId(this->lrTireId->getULong());
		clonedCompPtr->setRRTireId(this->rrTireId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool VehicleComponent::connect(void)
	{
		bool success = true;

		this->createVehicle();

		return success;
	}

	bool VehicleComponent::disconnect(void)
	{
		return true;
	}

	bool VehicleComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr lfTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->lfTireId->getULong());
		if (nullptr != lfTireGameObjectPtr)
			this->setLFTireId(lfTireGameObjectPtr->getId());
		else
			this->setLFTireId(0);

		GameObjectPtr rfTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->rfTireId->getULong());
		if (nullptr != rfTireGameObjectPtr)
			this->setRFTireId(rfTireGameObjectPtr->getId());
		else
			this->setRFTireId(0);

		GameObjectPtr lrTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->lrTireId->getULong());
		if (nullptr != lrTireGameObjectPtr)
			this->setLRTireId(lrTireGameObjectPtr->getId());
		else
			this->setLRTireId(0);

		GameObjectPtr rrTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->rrTireId->getULong());
		if (nullptr != rrTireGameObjectPtr)
			this->setRRTireId(rrTireGameObjectPtr->getId());
		else
			this->setRRTireId(0);

		return true;
	}

	void VehicleComponent::createVehicle(void)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			delete this->vehicle;
			this->vehicle = nullptr;
		}

		auto& lfTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->lfTireId->getULong());
		if (nullptr == lfTireGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no left front tire for id: "
				+ Ogre::StringConverter::toString(this->lfTireId->getULong()) + " for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto& rfTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->rfTireId->getULong());
		if (nullptr == lfTireGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no right front tire for id: "
				+ Ogre::StringConverter::toString(this->rfTireId->getULong()) + " for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto& lrTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->lrTireId->getULong());
		if (nullptr == lrTireGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no left rear tire for id: "
				+ Ogre::StringConverter::toString(this->lrTireId->getULong()) + " for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto& rrTireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->rrTireId->getULong());
		if (nullptr == rrTireGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no right rear tire for id: "
				+ Ogre::StringConverter::toString(this->rrTireId->getULong()) + " for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto lfTireComponent = NOWA::makeStrongPtr(lfTireGameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr == lfTireComponent)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no left front tire physicsActiveComponent for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto rfTireComponent = NOWA::makeStrongPtr(rfTireGameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr == lfTireComponent)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no right front tire physicsActiveComponent for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto lrTireComponent = NOWA::makeStrongPtr(lrTireGameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr == lfTireComponent)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no left rear tire physicsActiveComponent for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto rrTireComponent = NOWA::makeStrongPtr(rrTireGameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr == lfTireComponent)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no right rear tire physicsActiveComponent for game object: " + this->gameObjectPtr->getName());
			return;
		}

		auto chassisComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr == chassisComponent)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VehicleComponent] Could not create vehicle because there is no chassis physicsActiveComponent for game object: " + this->gameObjectPtr->getName());
			return;
		}

		OgreNewt::CarDefinition carDefinition;
		carDefinition.m_aerodynamicsDownForceWeightCoeffecient0 = this->downForceWeightFactors->getVector3().x;
		carDefinition.m_aerodynamicsDownForceWeightCoeffecient1 = this->downForceWeightFactors->getVector3().y;
		carDefinition.m_aerodynamicsDownForceSpeedFactor = this->downForceWeightFactors->getVector3().z;

		carDefinition.m_chassisYaxisComBias = this->comYOffset->getReal();
		carDefinition.m_cluthFrictionTorque = this->clutchFrictionTorque->getReal();
		carDefinition.m_corneringStiffness = this->tireCorningStiffness->getReal();
		carDefinition.m_differentialType = this->differentialType->getUInt();
		carDefinition.m_engineMass = this->engineMass->getReal();
		carDefinition.m_vehicleMass = chassisComponent->getMass();
		
		carDefinition.m_engineIdleTorque = this->idleTorqueRpm->getVector2().x;
		carDefinition.m_engineRPMAtIdleTorque = this->idleTorqueRpm->getVector2().y;

		carDefinition.m_enginePeakTorque = this->peakTorqueRpm->getVector2().x;
		carDefinition.m_engineRPMAtPeakTorque = this->peakTorqueRpm->getVector2().y;

		carDefinition.m_enginePeakHorsePower = this->peakHpRpm->getVector2().x;
		carDefinition.m_egineRPMAtPeakHorsePower = this->peakHpRpm->getVector2().y;
		
		carDefinition.m_engineRotorRadio = this->engineRatio->getReal();
		carDefinition.m_engineRPMAtRedLine = this->redlineTorqueRpm->getReal();
		carDefinition.m_frontSteeringAngle = this->axelSteerAngles->getVector2().x;
		carDefinition.m_rearSteeringAngle = this->axelSteerAngles->getVector2().y;
		carDefinition.m_tireAligningMomemtTrail = this->tireAligningMomentTrail->getReal();
		carDefinition.m_tireBrakesTorque = this->tireBrakeTorque->getReal();
		carDefinition.m_tireMass = lfTireComponent->getMass();
		carDefinition.m_tirePivotOffset = this->tirePivotOffsetZY->getVector2().x;
		carDefinition.m_tireVerticalOffsetTweak = this->tirePivotOffsetZY->getVector2().y;
		carDefinition.m_tireSuspensionSpringConstant = this->tireSuspensionSpring->getVector3().x;
		carDefinition.m_tireSuspensionDamperConstant = this->tireSuspensionSpring->getVector3().y;
		carDefinition.m_tireSuspensionLength = this->tireSuspensionSpring->getVector3().z;
		unsigned int suspensionType = this->tireSuspensionType->getUInt();
		if (suspensionType > 3) suspensionType = 3;
		carDefinition.m_tireSuspensionType = static_cast<dSuspensionType>(suspensionType);
		carDefinition.m_transmissionGearRatio0 = this->tireGear123->getVector3().x;
		carDefinition.m_transmissionGearRatio1 = this->tireGear123->getVector3().y;
		carDefinition.m_transmissionGearRatio2 = this->tireGear123->getVector3().z;
		carDefinition.m_transmissionGearRatio3 = this->tireGear456->getVector3().x;
		carDefinition.m_transmissionGearRatio4 = this->tireGear456->getVector3().y;
		carDefinition.m_transmissionGearRatio6 = this->tireGear456->getVector3().z;
		carDefinition.m_transmissionRevereGearRatio = this->tireReverseGear->getReal();
		carDefinition.m_vehicleTopSpeed = this->topSpeedKmh->getReal();
		carDefinition.m_vehicleWeightDistribution = this->weightDistribution->getReal();
		carDefinition.m_wheelHasCollisionFenders = true == this->wheelHasFender->getBool() ? 1 : 0;

		this->vehicle = new OgreNewt::Vehicle(carDefinition);
		this->vehicle->createVehicle(this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(), chassisComponent->getBody(),
			lfTireComponent->getBody(), rfTireComponent->getBody(), lrTireComponent->getBody(), rrTireComponent->getBody());
#endif
	}

	void VehicleComponent::update(Ogre::Real dt, bool notSimulating)
	{
#if 0
		if (nullptr != this->vehicle && false == notSimulating)
		{
			this->vehicle->update(dt);
		}
#endif
	}

	void VehicleComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (VehicleComponent::AttrEngineMass() == attribute->getName())
		{
			this->setEngineMass(attribute->getReal());
		}
		else if (VehicleComponent::AttrEngineRatio() == attribute->getName())
		{
			this->setEngineRatio(attribute->getReal());
		}
		else if (VehicleComponent::AttrAxelSteerAngles() == attribute->getName())
		{
			this->setAxelSteerAngles(attribute->getVector2());
		}
		else if (VehicleComponent::AttrWeightDistribution() == attribute->getName())
		{
			this->setWeightDistribution(attribute->getReal());
		}
		else if (VehicleComponent::AttrClutchFrictionTorque() == attribute->getName())
		{
			this->setClutchFrictionTorque(attribute->getReal());
		}
		else if (VehicleComponent::AttrIdleTorqueRpm() == attribute->getName())
		{
			this->setIdleTorqueRpm(attribute->getVector2());
		}
		else if (VehicleComponent::AttrPeakTorqueRpm() == attribute->getName())
		{
			this->setPeakTorqueRpm(attribute->getVector2());
		}
		else if (VehicleComponent::AttrPeakHpRpm() == attribute->getName())
		{
			this->setPeakHpRpm(attribute->getVector2());
		}
		else if (VehicleComponent::AttrTopSpeedKmh() == attribute->getName())
		{
			this->setTopSpeedKmh(attribute->getReal());
		}
		else if (VehicleComponent::AttrTireCorningStiffness() == attribute->getName())
		{
			this->setTireCorningStiffness(attribute->getReal());
		}
		else if (VehicleComponent::AttrTireAligningMomentTrail() == attribute->getName())
		{
			this->setTireAligningMomentTrail(attribute->getReal());
		}
		else if (VehicleComponent::AttrTireSuspensionSpring() == attribute->getName())
		{
			this->setTireSuspensionSpring(attribute->getVector3());
		}
		else if (VehicleComponent::AttrTireBrakeTorque() == attribute->getName())
		{
			this->setTireBrakeTorque(attribute->getReal());
		}
		else if (VehicleComponent::AttrTirePivotOffsetZY() == attribute->getName())
		{
			this->setTirePivotOffsetZY(attribute->getVector2());
		}
		else if (VehicleComponent::AttrTireGear123() == attribute->getName())
		{
			this->setTireGear123(attribute->getVector3());
		}
		else if (VehicleComponent::AttrTireGear456() == attribute->getName())
		{
			this->setTireGear456(attribute->getVector3());
		}
		else if (VehicleComponent::AttrTireReverseGear() == attribute->getName())
		{
			this->setTireReverseGear(attribute->getReal());
		}
		else if (VehicleComponent::AttrComYOffset() == attribute->getName())
		{
			this->setComYOffset(attribute->getReal());
		}
		else if (VehicleComponent::AttrDownForceWeightFactors() == attribute->getName())
		{
			this->setDownForceWeightFactors(attribute->getVector3());
		}
		else if (VehicleComponent::AttrWheelHasFender() == attribute->getName())
		{
			this->setWheelHasFender(attribute->getBool());
		}
		else if (VehicleComponent::AttrDifferentialType() == attribute->getName())
		{
			this->setDifferentialType(attribute->getUInt());
		}
		else if (VehicleComponent::AttrTireSuspensionType() == attribute->getName())
		{
			this->setTireSuspensionType(attribute->getUInt());
		}
		else if (VehicleComponent::AttrLFTireId() == attribute->getName())
		{
			this->setLFTireId(attribute->getULong());
		}
		else if (VehicleComponent::AttrRFTireId() == attribute->getName())
		{
			this->setRFTireId(attribute->getULong());
		}
		else if (VehicleComponent::AttrLRTireId() == attribute->getName())
		{
			this->setLRTireId(attribute->getULong());
		}
		else if (VehicleComponent::AttrRRTireId() == attribute->getName())
		{
			this->setRRTireId(attribute->getULong());
		}
	}

	void VehicleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "EngineMass"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->engineMass->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EngineRatio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->engineRatio->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AxelSteerAngles"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->axelSteerAngles->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WeightDistribution"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->weightDistribution->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ClutchFrictionTorque"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->clutchFrictionTorque->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "IdleTorqueRpm"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->idleTorqueRpm->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PeakTorqueRpm"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->peakTorqueRpm->getVector2())));
		propertiesXML->append_node(propertyXML);


		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PeakHpRpm"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->peakHpRpm->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RedlineTorqueRpm"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->redlineTorqueRpm->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TopSpeedKmh"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->topSpeedKmh->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireCorningStiffness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireCorningStiffness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireAligningMomentTrail"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireAligningMomentTrail->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireSuspensionSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireSuspensionSpring->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireBrakeTorque"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireBrakeTorque->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TirePivotOffsetZY"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tirePivotOffsetZY->getVector2())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireGear123"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireGear123->getVector3())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireGear456"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireGear456->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireReverseGear"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireReverseGear->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ComYOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->comYOffset->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DownForceWeightFactors"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->downForceWeightFactors->getVector3())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WheelHasFender"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wheelHasFender->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DifferentialType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->differentialType->getUInt())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TireSuspensionType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireSuspensionType->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LFTireId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lfTireId->getULong())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RFTireId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rfTireId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LRTireId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lrTireId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RRTireId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rrTireId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String VehicleComponent::getClassName(void) const
	{
		return "VehicleComponent";
	}

	Ogre::String VehicleComponent::getParentClassName(void) const
	{
		return "ProceduralComponent";
	}

	void VehicleComponent::setActivated(bool activated)
	{
		
	}

	void VehicleComponent::setThrottle(Ogre::Real throttle)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_throttle = throttle;
		}
#endif
	}

	void VehicleComponent::setClutchPedal(Ogre::Real clutchPedal)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_clutchPedal = clutchPedal;
		}
#endif
	}

	void VehicleComponent::setSteeringValue(Ogre::Real steeringValue)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_steeringValue = steeringValue;
		}
#endif
	}

	void VehicleComponent::setBrakePedal(Ogre::Real brakePedal)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_brakePedal = brakePedal;
		}
#endif
	}

	void VehicleComponent::setIgnitionKey(bool ignitionKey)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_ignitionKey = true == ignitionKey ? 1 : 0;
		}
#endif
	}

	void VehicleComponent::setHandBrakeValue(Ogre::Real handBrakeValue)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_handBrakeValue = handBrakeValue;
		}
#endif
	}

	void VehicleComponent::setGear(unsigned int gear)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_gear = gear;
		}
#endif
	}

	void VehicleComponent::setManualTransmission(bool manualTransmission)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_manualTransmission = true == manualTransmission ? 1 : 0;
		}
#endif
	}

	void VehicleComponent::setLockDifferential(bool lockDifferential)
	{
#if 0
		if (nullptr != this->vehicle)
		{
			this->vehicle->getDriverInput().m_lockDifferential = true == lockDifferential ? 1 : 0;
		}
#endif
	}

	void VehicleComponent::setEngineMass(Ogre::Real engineMass)
	{
		this->engineMass->setValue(engineMass);
	}
	
	Ogre::Real VehicleComponent::getEngineMass(void) const
	{
		return this->engineMass->getReal();
	}
	
	void VehicleComponent::setEngineRatio(Ogre::Real engineRatio)
	{
		this->engineRatio->setValue(engineRatio);
	}
	
	Ogre::Real VehicleComponent::getEngineRatio(void) const
	{
		return this->engineRatio->getReal();
	}
	
	void VehicleComponent::setAxelSteerAngles(const Ogre::Vector2& axelSteerAngles)
	{
		this->axelSteerAngles->setValue(axelSteerAngles);
	}
	
	Ogre::Vector2 VehicleComponent::getAxelSteerAngles(void) const
	{
		return this->axelSteerAngles->getVector2();
	}
		
	void VehicleComponent::setWeightDistribution(Ogre::Real weightDistribution)
	{
		this->weightDistribution->setValue(weightDistribution);
	}
	
	Ogre::Real VehicleComponent::getWeightDistribution(void) const
	{
		return this->weightDistribution->getReal();
	}
		
	void VehicleComponent::setClutchFrictionTorque(Ogre::Real clutchFrictionTorque)
	{
		this->clutchFrictionTorque->setValue(clutchFrictionTorque);
	}
	
	Ogre::Real VehicleComponent::getClutchFrictionTorque(void) const
	{
		return this->clutchFrictionTorque->getReal();
	}
		
	void VehicleComponent::setIdleTorqueRpm(const Ogre::Vector2& idleTorqueRpm)
	{
		this->idleTorqueRpm->setValue(idleTorqueRpm);
	}
	
	Ogre::Vector2 VehicleComponent::getIdleTorqueRpm(void) const
	{
		return this->idleTorqueRpm->getVector2();
	}
		
	void VehicleComponent::setPeakTorqueRpm(const Ogre::Vector2& peakTorqueRpm)
	{
		this->peakTorqueRpm->setValue(peakTorqueRpm);
	}
	
	Ogre::Vector2 VehicleComponent::getPeakTorqueRpm(void) const
	{
		return this->peakTorqueRpm->getVector2();
	}
		
	void VehicleComponent::setPeakHpRpm(const Ogre::Vector2& peakHpRpm)
	{
		this->peakHpRpm->setValue(peakHpRpm);
	}
	
	Ogre::Vector2 VehicleComponent::getPeakHpRpm(void) const
	{
		return this->peakHpRpm->getVector2();
	}
		
	void VehicleComponent::setRedlineTorqueRpm(Ogre::Real redlineTorqueRpm)
	{
		this->redlineTorqueRpm->setValue(redlineTorqueRpm);
	}
	
	Ogre::Real VehicleComponent::getRedlineTorqueRpm(void) const
	{
		return this->redlineTorqueRpm->getReal();
	}
		
	void VehicleComponent::setTopSpeedKmh(Ogre::Real topSpeedKmh)
	{
		this->topSpeedKmh->setValue(topSpeedKmh);
	}
	
	Ogre::Real VehicleComponent::getTopSpeedKmh(void) const
	{
		return this->topSpeedKmh->getReal();
	}
		
	void VehicleComponent::setTireCorningStiffness(Ogre::Real tireCorningStiffness)
	{
		this->tireCorningStiffness->setValue(tireCorningStiffness);
	}
	
	Ogre::Real VehicleComponent::getTireCorningStiffness(void) const
	{
		return this->tireCorningStiffness->getReal();
	}
		
	void VehicleComponent::setTireAligningMomentTrail(Ogre::Real tireAligningMomentTrail)
	{
		this->tireAligningMomentTrail->setValue(tireAligningMomentTrail);
	}
	
	Ogre::Real VehicleComponent::getTireAligningMomentTrail(void) const
	{
		return this->tireAligningMomentTrail->getReal();
	}
		
	void VehicleComponent::setTireSuspensionSpring(const Ogre::Vector3& tireSuspensionSpring)
	{
		this->tireSuspensionSpring->setValue(tireSuspensionSpring);
	}
	
	Ogre::Vector3 VehicleComponent::getTireSuspensionSpring(void) const
	{
		return this->tireSuspensionSpring->getVector3();
	}
		
	void VehicleComponent::setTireBrakeTorque(Ogre::Real tireBrakeTorque)
	{
		this->tireBrakeTorque->setValue(tireBrakeTorque);
	}
	
	Ogre::Real VehicleComponent::getTireBrakeTorque(void) const
	{
		return this->tireBrakeTorque->getReal();
	}
		
	void VehicleComponent::setTirePivotOffsetZY(const Ogre::Vector2& tirePivotOffsetZY)
	{
		this->tirePivotOffsetZY->setValue(tirePivotOffsetZY);
	}
	
	Ogre::Vector2 VehicleComponent::getTirePivotOffsetZY(void) const
	{
		return this->tirePivotOffsetZY->getVector2();
	}
		
	void VehicleComponent::setTireGear123(const Ogre::Vector3& tireGear123)
	{
		this->tireGear123->setValue(tireGear123);
	}
	
	Ogre::Vector3 VehicleComponent::getTireGear123(void) const
	{
		return this->tireGear123->getVector3();
	}
		
	void VehicleComponent::setTireGear456(const Ogre::Vector3& tireGear456)
	{
		this->tireGear456->setValue(tireGear456);
	}
	
	Ogre::Vector3 VehicleComponent::getTireGear456(void) const
	{
		return this->tireGear456->getVector3();
	}
		
	void VehicleComponent::setTireReverseGear(Ogre::Real tireReverseGear)
	{
		this->tireReverseGear->setValue(tireReverseGear);
	}
	
	Ogre::Real VehicleComponent::getTireReverseGear(void) const
	{
		return this->tireReverseGear->getReal();
	}
		
	void VehicleComponent::setComYOffset(Ogre::Real comYOffset)
	{
		this->comYOffset->setValue(comYOffset);
	}
	
	Ogre::Real VehicleComponent::getComYOffset(void) const
	{
		return this->comYOffset->getReal();
	}
		
	void VehicleComponent::setDownForceWeightFactors(const Ogre::Vector3& downForceWeightFactors)
	{
		this->downForceWeightFactors->setValue(downForceWeightFactors);
	}
	
	Ogre::Vector3 VehicleComponent::getDownForceWeightFactors(void) const
	{
		return this->downForceWeightFactors->getVector3();
	}
		
	void VehicleComponent::setWheelHasFender(bool wheelHasFender)
	{
		this->wheelHasFender->setValue(wheelHasFender);
	}
	
	bool VehicleComponent::getWheelHasFender(void) const
	{
		return this->wheelHasFender->getBool();
	}
		
	void VehicleComponent::setDifferentialType(unsigned int differentialType)
	{
		this->differentialType->setValue(differentialType);
	}
	
	unsigned int VehicleComponent::getDifferentialType(void) const
	{
		return this->differentialType->getUInt();
	}
		
	void VehicleComponent::setTireSuspensionType(unsigned int tireSuspensionType)
	{
		this->tireSuspensionType->setValue(tireSuspensionType);
	}
	
	unsigned int VehicleComponent::getTireSuspensionType(void) const
	{
		return this->tireSuspensionType->getUInt();
	}
		
	void VehicleComponent::setLFTireId(unsigned long lfTireId)
	{
		this->lfTireId->setValue(lfTireId);
	}
	
	unsigned long VehicleComponent::getLFTireId(void) const
	{
		return this->lfTireId->getULong();
	}
		
	void VehicleComponent::setRFTireId(unsigned long rfTireId)
	{
		this->rfTireId->setValue(rfTireId);
	}
	
	unsigned long VehicleComponent::getRFTireId(void) const
	{
		return this->rfTireId->getULong();
	}
		
	void VehicleComponent::setLRTireId(unsigned long lrTireId)
	{
		this->lrTireId->setValue(lrTireId);
	}
	
	unsigned long VehicleComponent::getLRTireId(void) const
	{
		return this->lrTireId->getULong();
	}
		
	void VehicleComponent::setRRTireId(unsigned long rrTireId)
	{
		this->rrTireId->setValue(rrTireId);
	}
	
	unsigned long VehicleComponent::getRRTireId(void) const
	{
		return this->rrTireId->getULong();
	}
	
}; // namespace end