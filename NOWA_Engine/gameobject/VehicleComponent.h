#ifndef VEHICLE_COMPONENT_H
#define VEHICLE_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreNewt_Vehicle.h"

namespace NOWA
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED VehicleComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<VehicleComponent> VehicleCompPtr;
	public:

		VehicleComponent();

		virtual ~VehicleComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;
		
		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onCloned
		*/
		bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("VehicleComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "VehicleComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: With this component, a physically vehicle is created.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		void setThrottle(Ogre::Real throttle);

		void setClutchPedal(Ogre::Real clutchPedal);

		void setSteeringValue(Ogre::Real steeringValue);

		void setBrakePedal(Ogre::Real brakePedal);

		void setIgnitionKey(bool ignitionKey);

		void setHandBrakeValue(Ogre::Real handBrakeValue);

		void setGear(unsigned int gear);

		void setManualTransmission(bool manualTransmission);
		
		void setLockDifferential(bool lockDifferential);

		void setEngineMass(Ogre::Real engineMass);

		Ogre::Real getEngineMass(void) const;
		
		void setEngineRatio(Ogre::Real engineRatio);

		Ogre::Real getEngineRatio(void) const;
		
		void setAxelSteerAngles(const Ogre::Vector2& axelSteerAngles);

		Ogre::Vector2 getAxelSteerAngles(void) const;
		
		void setWeightDistribution(Ogre::Real weightDistribution);

		Ogre::Real getWeightDistribution(void) const;
		
		void setClutchFrictionTorque(Ogre::Real clutchFrictionTorque);

		Ogre::Real getClutchFrictionTorque(void) const;
		
		void setIdleTorqueRpm(const Ogre::Vector2& idleTorqueRpm);

		Ogre::Vector2 getIdleTorqueRpm(void) const;
		
		void setPeakTorqueRpm(const Ogre::Vector2& peakTorqueRpm);

		Ogre::Vector2 getPeakTorqueRpm(void) const;
		
		void setPeakHpRpm(const Ogre::Vector2& peakHpRpm);

		Ogre::Vector2 getPeakHpRpm(void) const;
		
		void setRedlineTorqueRpm(Ogre::Real redlineTorqueRpm);

		Ogre::Real getRedlineTorqueRpm(void) const;
		
		void setTopSpeedKmh(Ogre::Real topSpeedKmh);

		Ogre::Real getTopSpeedKmh(void) const;
		
		void setTireCorningStiffness(Ogre::Real tireCorningStiffness);

		Ogre::Real getTireCorningStiffness(void) const;
		
		void setTireAligningMomentTrail(Ogre::Real tireAligningMomentTrail);

		Ogre::Real getTireAligningMomentTrail(void) const;
		
		void setTireSuspensionSpring(const Ogre::Vector3& tireSuspensionSpring);

		Ogre::Vector3 getTireSuspensionSpring(void) const;
		
		void setTireBrakeTorque(Ogre::Real tireBrakeTorque);

		Ogre::Real getTireBrakeTorque(void) const;
		
		void setTirePivotOffsetZY(const Ogre::Vector2& tirePivotOffsetZY);

		Ogre::Vector2 getTirePivotOffsetZY(void) const;
		
		void setTireGear123(const Ogre::Vector3& tireGear123);

		Ogre::Vector3 getTireGear123(void) const;
		
		void setTireGear456(const Ogre::Vector3& tireGear456);

		Ogre::Vector3 getTireGear456(void) const;
		
		void setTireReverseGear(Ogre::Real tireReverseGear);

		Ogre::Real getTireReverseGear(void) const;
		
		void setComYOffset(Ogre::Real comYOffset);

		Ogre::Real getComYOffset(void) const;
		
		void setDownForceWeightFactors(const Ogre::Vector3& downForceWeightFactors);

		Ogre::Vector3 getDownForceWeightFactors(void) const;
		
		void setWheelHasFender(bool wheelHasFender);

		bool getWheelHasFender(void) const;
		
		void setDifferentialType(unsigned int differentialType);

		unsigned int getDifferentialType(void) const;
		
		void setTireSuspensionType(unsigned int tireSuspensionType);

		unsigned int getTireSuspensionType(void) const;
		
		void setLFTireId(unsigned long lfTireId);

		unsigned long getLFTireId(void) const;
		
		void setRFTireId(unsigned long rfTireId);

		unsigned long getRFTireId(void) const;
		
		void setLRTireId(unsigned long lrTireId);

		unsigned long getLRTireId(void) const;
		
		void setRRTireId(unsigned long rrTireId);

		unsigned long getRRTireId(void) const;
	private:
		void createVehicle(void);
	public:
		static const Ogre::String AttrEngineMass(void) { return "Engine Mass"; }
		static const Ogre::String AttrEngineRatio(void) { return "Engine Ratio"; }
		static const Ogre::String AttrAxelSteerAngles(void) { return "Axel Steer Angles"; }
		static const Ogre::String AttrWeightDistribution(void) { return "Weight Distribution"; }
		static const Ogre::String AttrClutchFrictionTorque(void) { return "Clutch Friction Torque"; }
		static const Ogre::String AttrIdleTorqueRpm(void) { return "Idle Torque RPM"; }
		static const Ogre::String AttrPeakTorqueRpm(void) { return "Peak Torque RPM"; }
		static const Ogre::String AttrPeakHpRpm(void) { return "Peak HP RPM"; }
		
		static const Ogre::String AttrRedlineTorqueRpm(void) { return "Redline Torque RPM"; }
		static const Ogre::String AttrTopSpeedKmh(void) { return "Top Speed Kmh"; }
		static const Ogre::String AttrTireCorningStiffness(void) { return "Tire Corning Stiffness"; }
		static const Ogre::String AttrTireAligningMomentTrail(void) { return "Tire Aligning Moment Trail"; }
		static const Ogre::String AttrTireSuspensionSpring(void) { return "Tire Suspension S, D, L"; }
		static const Ogre::String AttrTireBrakeTorque(void) { return "Tire Brake Torque"; }
		static const Ogre::String AttrTirePivotOffsetZY(void) { return "Tire Pivot Offset Z,Y"; }
		static const Ogre::String AttrTireGear123(void) { return "Tire Gear 1,2,3"; }
		static const Ogre::String AttrTireGear456(void) { return "Tire Gear 4,5,6"; }
		static const Ogre::String AttrTireReverseGear(void) { return "Tire Reverse Gear"; }
		
		static const Ogre::String AttrComYOffset(void) { return "Com Y Offset"; }
		static const Ogre::String AttrDownForceWeightFactors(void) { return "Downforce Weight Factors"; }
		static const Ogre::String AttrWheelHasFender(void) { return "Wheel has Fender"; }
		static const Ogre::String AttrDifferentialType(void) { return "Differential Type"; }
		static const Ogre::String AttrTireSuspensionType(void) { return "Tire Suspension Type"; }
		static const Ogre::String AttrLFTireId(void) { return "Left Front Tire ID"; }
		static const Ogre::String AttrRFTireId(void) { return "Right Front Tire ID"; }
		static const Ogre::String AttrLRTireId(void) { return "Left Rear Tire ID"; }
		static const Ogre::String AttrRRTireId(void) { return "Right Rear Tire ID"; }
	private:
		Variant* engineMass; // 1
		Variant* engineRatio; // 1
		Variant* axelSteerAngles; // 2
		Variant* weightDistribution; // 1
		Variant* clutchFrictionTorque; // 1
		Variant* idleTorqueRpm; // 2
		Variant* peakTorqueRpm; // 2
		Variant* peakHpRpm; // 2
		Variant* redlineTorqueRpm; // 1
		Variant* topSpeedKmh; // 1
		Variant* tireCorningStiffness; // 1
		Variant* tireAligningMomentTrail; // 1
		Variant* tireSuspensionSpring; // 3
		Variant* tireBrakeTorque; // 1
		Variant* tirePivotOffsetZY; // 2
		Variant* tireGear123; // 3
		Variant* tireGear456; // 3
		Variant* tireReverseGear; // 1
		Variant* comYOffset; // 1
		Variant* downForceWeightFactors; // 3
		Variant* wheelHasFender; // bool
		Variant* differentialType; // 1 list
		Variant* tireSuspensionType; // 1 list
		Variant* lfTireId; // 1
		Variant* rfTireId; // 1
		Variant* lrTireId; // 1
		Variant* rrTireId; // 1

#if 0
		OgreNewt::Vehicle* vehicle;
#endif
	};

}; //namespace end

#endif