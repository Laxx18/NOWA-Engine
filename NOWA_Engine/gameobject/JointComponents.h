#ifndef JOINT_COMPONENTS_H
#define JOINT_COMPONENTS_H

#include "GameObjectComponent.h"
#include "OgreNewt.h"
#include <fparser.hh>

#include "main/Events.h"

namespace NOWA
{
	/*******************************JointComponent*******************************/

	// See: http://www.motionboutique.com/files/newton2/documentation/
	// For some nice video example, what joints should be possible

	class EXPORTED JointComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<JointComponent> JointCompPtr;
		friend class GameObjectController;
	public:
		JointComponent(void);

		virtual ~JointComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		* @see		GameObjectComponent::onOtherComponentRemoved
		*/
		void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false) override
		{

		}

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		/**
		* @brief		Actualizes the value for the given attribute
		* @param[in]	attribute	The attribute to trigger the actualization of a value of a component
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath);

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO);

		virtual void forceShowDebugData(bool activate) { };

		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		/**
		* @brief	Gets the unique id of this joint handler instance
		* @return	id		The id of this joint handler instance
		*/
		unsigned long getId(void) const;

		void setPredecessorId(unsigned long predecessorId);

		unsigned long getPredecessorId(void) const;

		void connectPredecessorCompPtr(boost::weak_ptr<JointComponent> weakJointPredecessorCompPtr);

		void connectPredecessorId(unsigned long predecessorId);

		void setTargetId(unsigned long targetId);

		void connectTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;

		void setType(const Ogre::String& type);

		Ogre::String getType(void) const;

		OgreNewt::Joint* getJoint(void) const;

		void setBody(OgreNewt::Body* body);

		OgreNewt::Body* getBody(void) const;

		Ogre::Vector3 getJointPosition(void) const;

		/*
		 * @brief Sets the maximum percentage of the constraint force that will be applied to the constraint.
		 * @param[in] stiffness	The stiffness value to set. It must be a values between 0.0 and 1.0, the default is 0.9.
		 * @note This function will override the default stiffness value set after a call to NewtonUserJointAddLinearRow or NewtonUserJointAddAngleRow. 
		 *		The row stiffness is the percentage of the constraint force that will be applied to the rigid bodies. 
		 *		Ideally the value should be 1.0 (100% stiff) but dues to numerical integration error this could be the joint a little unstable, and lower values are preferred.
		 *		Other detailed notes:
		 *		Constraint keep bodies together by calculating the exact force necessary to cancel the relative acceleration between one or more common points fixed in the two bodies. 
		 *		The problem is that when the bodies drift apart due to numerical integration inaccuracies, 
		 *		the reaction force work to pull eliminated the error but at the expense of adding extra energy to the system, 
		 *		does violating the rule that constraint forces must be work less. 
		 *		This is a inevitable situation and the only think we can do is to minimize the effect of the extra energy by dampening the force by some amount. 
		 *		In essence the stiffness coefficient tell Newton calculate the precise reaction force by only apply a fraction of it to the joint point. 
		 *		And value of 1.0 will apply the exact force, and a value of zero will apply only 10 percent.
		 *		
		 *		The stiffness is set to a all around value that work well for most situation, however the application can play with these parameter to make finals adjustment. 
		 *		A high value will make the joint stronger but more prompt to vibration of instability; a low value will make the joint more stable but weaker.
		 */
		void setStiffness(Ogre::Real stiffness);

		Ogre::Real getStiffness(void) const;

		///*
		// * @brief Sets the minimum friction value the solver is allow to apply to the joint.
		// * @param[in] minimumFriction	The minimum friction value to set. It must be a negative value between 0.0 and -INFINITY.
		// */
		//void setMinimumFriction(Ogre::Real minimumFriction);

		///*
		// * @brief Sets the maximum friction value the solver is allow to apply to the joint.
		// * @param[in] maximumFriction	The maximum friction value to set. It must be a positive value between 0.0 and INFINITY.
		// */
		//void setMaximumFriction(Ogre::Real maximumFriction);

		/**
		* @brief		Sets whether this joint's body should collide with its predecessor joint body
		* @param[in]	enable	true, if the joint should collide, else false
		*/
		void setJointRecursiveCollisionEnabled(bool enable);

		bool getJointRecursiveCollisionEnabled(void) const;

		/**
		* @brief		Sets in which ratio the first body is influenced by the mass of the second body and vice versa.
		* @param[in]	scaleBody0	The first body mass scale radio
		* @param[in]	scaleBody1	The second body mass scale radio
		*/
		void setBodyMassScale(const Ogre::Vector2& bodyMassScale);

		Ogre::Vector2 getBodyMassScale(void) const;

		void releaseJoint(bool resetPredecessorAndTarget = false);

		void deleteJointDelegate(EventDataPtr eventData);

		unsigned long getPriorId(void) const;

		void setSceneManager(Ogre::SceneManager* sceneManager);
	public:
		// Attribute constants
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrId(void) { return "Id"; }
		static const Ogre::String AttrPredecessorId(void) { return "Predecessor Id"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrType(void) { return "Type"; }
		// static const Ogre::String AttrPosition(void) { return "Position"; }
		static const Ogre::String AttrRecursiveCollision(void) { return "Recursive Col."; }
		static const Ogre::String AttrStiffness(void) { return "Stiffness"; }
		static const Ogre::String AttrBodyMassScale(void) { return "Body mass scale"; }
	protected:
		void setJointPosition(const Ogre::Vector3& jointPosition);
		void internalSetPriorId(unsigned long priorId);
		void internalShowDebugData(bool activate, unsigned short type, const Ogre::Vector3& value1, const Ogre::Vector3& value2);
		void applyStiffness(void);
	protected:
		Variant* activated;
		Variant* id;
		Variant* predecessorId;
		Variant* targetId;
		Variant* type;
		Variant* jointRecursiveCollision;
		Variant* stiffness;
		Variant* bodyMassScale;
		unsigned long priorId;
		// Debug data
		Ogre::SceneNode* debugGeometryNode;
		Ogre::v1::Entity* debugGeometryEntity;
		Ogre::SceneNode* debugGeometryNode2;
		Ogre::v1::Entity* debugGeometryEntity2;
		Ogre::SceneManager* sceneManager;
		bool useCustomStiffness;
		bool jointAlreadyCreated;
		OgreNewt::Joint* joint;
		OgreNewt::Body* body;
		JointCompPtr predecessorJointCompPtr;
		JointCompPtr targetJointCompPtr;
		Ogre::Vector3 jointPosition;

	};

	/*******************************JointHingeComponent*******************************/

	class EXPORTED JointHingeComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointHingeComponent> JointHingeCompPtr;
	public:
		JointHingeComponent(void);

		virtual ~JointHingeComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointHingeComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointHingeComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;
	
		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;

		void setLimitsEnabled(bool enableLimits);

		bool getLimitsEnabled(void) const;

		void setMinMaxAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit);
		
		Ogre::Degree getMinAngleLimit(void) const;
		
		Ogre::Degree getMaxAngleLimit(void) const;

		void setTorgue(Ogre::Real torgue);

		Ogre::Real getTorgue(void) const;

		void setBreakForce(Ogre::Real breakForce);

		Ogre::Real getBreakForce(void) const;

		void setBreakTorque(Ogre::Real breakTorque);

		Ogre::Real getBreakTorque(void) const;

		void setSpring(bool asSpringDamper, bool massIndependent);

		void setSpring(bool asSpringDamper, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD);

		std::tuple<bool, bool, Ogre::Real, Ogre::Real, Ogre::Real> getSpring(void) const;

		void setFriction(Ogre::Real friction);

		Ogre::Real getFriction(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrLimits(void) { return "Limits"; }
		static const Ogre::String AttrMinAngleLimit(void) { return "Min Angle"; }
		static const Ogre::String AttrMaxAngleLimit(void) { return "Max Angle"; }
		static const Ogre::String AttrTorque(void) { return "Torque"; }
		static const Ogre::String AttrBreakForce(void) { return "Break Force"; }
		static const Ogre::String AttrBreakTorque(void) { return "Break Torque"; }
		static const Ogre::String AttrAsSpringDamper(void) { return "As Spring Damper"; }
		static const Ogre::String AttrMassIndependent(void) { return "Mass Independent"; }
		static const Ogre::String AttrSpringK(void) { return "SpringK"; }
		static const Ogre::String AttrSpringD(void) { return "SpringD"; }
		static const Ogre::String AttrSpringDamperRelaxation(void) { return "Damper Relaxation"; }
		static const Ogre::String AttrFriction(void) { return "Friction"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* enableLimits;
		Variant* minAngleLimit;
		Variant* maxAngleLimit;
		Variant* torque;
		Variant* breakForce;
		Variant* breakTorque;
		Variant* asSpringDamper;
		Variant* massIndependent;
		Variant* springK;
		Variant* springD;
		Variant* springDamperRelaxation;
		Variant* friction;
	};
	
	/*******************************JointHingeActuatorComponent*******************************/

	class EXPORTED JointHingeActuatorComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointHingeActuatorComponent> JointHingeActuatorCompPtr;
	public:
		JointHingeActuatorComponent(void);

		virtual ~JointHingeActuatorComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointHingeActuatorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointHingeActuatorComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		virtual void setActivated(bool activated) override;

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;
	
		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;
		
		void setTargetAngle(const Ogre::Degree& targetAngle);
		
		Ogre::Degree getTargetAngle(void) const;
		
		void setAngleRate(const Ogre::Degree& angleRate);
		
		Ogre::Degree getAngleRate(void) const;

		void setMinAngleLimit(const Ogre::Degree& minAngleLimit);

		Ogre::Degree getMinAngleLimit(void) const;
		
		void setMaxAngleLimit(const Ogre::Degree& maxAngleLimit);

		Ogre::Degree getMaxAngleLimit(void) const;

		void setMaxTorque(Ogre::Real maxTorque);

		Ogre::Real getMaxTorque(void) const;
		
		void setDirectionChange(bool directionChange);
		
		bool getDirectionChange(void) const;
		
		void setRepeat(bool repeat);
		
		bool getRepeat(void) const;

		void setSpring(bool asSpringDamper, bool massIndependent);

		void setSpring(bool asSpringDamper, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD);

		std::tuple<bool, bool, Ogre::Real, Ogre::Real, Ogre::Real> getSpring(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrTargetAngle(void) { return "Target Angle"; }
		static const Ogre::String AttrAngleRate(void) { return "Angle Rate"; }
		static const Ogre::String AttrMinAngleLimit(void) { return "Min Angle Limit"; }
		static const Ogre::String AttrMaxAngleLimit(void) { return "Max Angle Limit"; }
		static const Ogre::String AttrMaxTorque(void) { return "Max Torque"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrAsSpringDamper(void) { return "As Spring Damper"; }
		static const Ogre::String AttrMassIndependent(void) { return "Mass Independent"; }
		static const Ogre::String AttrSpringK(void) { return "SpringK"; }
		static const Ogre::String AttrSpringD(void) { return "SpringD"; }
		static const Ogre::String AttrSpringDamperRelaxation(void) { return "Damper Relaxation"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* targetAngle;
		Variant* angleRate;
		Variant* minAngleLimit;
		Variant* maxAngleLimit;
		Variant* maxTorque;
		Variant* directionChange;
		Variant* repeat;
		short round;
		bool internalDirectionChange;
		Ogre::Real oppositeDir;
		Variant* asSpringDamper;
		Variant* massIndependent;
		Variant* springK;
		Variant* springD;
		Variant* springDamperRelaxation;
	};

	/*******************************JointBallAndSocketComponent*******************************/

	class EXPORTED JointBallAndSocketComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointBallAndSocketComponent> JointBallAndSocketCompPtr;
	public:
		JointBallAndSocketComponent(void);

		virtual ~JointBallAndSocketComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointBallAndSocketComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointBallAndSocketComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setTwistLimitsEnabled(bool enableLimits);

		bool getTwistLimitsEnabled(void) const;

		void setConeLimitsEnabled(bool enableLimits);

		bool getConeLimitsEnabled(void) const;

		void setMinMaxConeAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit, const Ogre::Degree& maxConeLimit);
		
		Ogre::Degree getMinAngleLimit(void) const;
		
		Ogre::Degree getMaxAngleLimit(void) const;
		
		Ogre::Degree getMaxConeLimit(void) const;

		void setTwistFriction(Ogre::Real twistFriction);

		Ogre::Real getTwistFriction(void) const;

		void setConeFriction(Ogre::Real coneFriction);

		Ogre::Real getConeFriction(void) const;

		// void setTwistSpringDamper(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrEnableTwistLimits(void) { return "Enable Twist Limits"; }
		static const Ogre::String AttrMinAngleLimit(void) { return "Min Angle"; }
		static const Ogre::String AttrMaxAngleLimit(void) { return "Max Angle"; }
		static const Ogre::String AttrEnableConeLimits(void) { return "Enable Cone Limits"; }
		static const Ogre::String AttrMaxConeLimit(void) { return "Max Cone Angle"; }
		static const Ogre::String AttrTwistFriction(void) { return "Twist Friction"; }
		static const Ogre::String AttrConeFriction(void) { return "Cone Friction"; }
	protected:
		Variant* anchorPosition;
		Variant* enableTwistLimits;
		Variant* enableConeLimits;
		Variant* minAngleLimit;
		Variant* maxAngleLimit;
		Variant* maxConeLimit;
		Variant* twistFriction;
		Variant* coneFriction;
	};
	
	/*******************************PointToPointComponent*******************************/

	class EXPORTED JointPointToPointComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointPointToPointComponent> JointPointToPointCompPtr;
	public:
		JointPointToPointComponent(void);

		virtual ~JointPointToPointComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointPointToPointComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointPointToPointComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition1(const Ogre::Vector3& anchorPosition1);

		Ogre::Vector3 getAnchorPosition1(void) const;

		void setAnchorPosition2(const Ogre::Vector3& anchorPosition2);

		Ogre::Vector3 getAnchorPosition2(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition1(void) { return "Anchor Position1"; }
		static const Ogre::String AttrAnchorPosition2(void) { return "Anchor Position2"; }
	protected:
		Variant* anchorPosition1;
		Variant* anchorPosition2;
		Ogre::Vector3 jointPosition2;
	};
	
	/*******************************JointControlledBallAndSocketComponent*******************************/

	//class EXPORTED JointControlledBallAndSocketComponent : public JointComponent
	//{
	//public:
	//	typedef boost::shared_ptr<JointControlledBallAndSocketComponent> JointControlledBallAndSocketCompPtr;
	//public:
	//	JointControlledBallAndSocketComponent(void);

	//	virtual ~JointControlledBallAndSocketComponent() override;

	//	virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

	//	virtual bool postInit(void) override;

	//	virtual bool connect(void) override;

	//	virtual bool disconnect(void) override;

	//	virtual Ogre::String getClassName(void) const override;

	//	virtual Ogre::String getParentClassName(void) const override;

	//	virtual void update(Ogre::Real dt, bool notSimulating = false) override;

	//	virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

	//	virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

	//	virtual void actualizeValue(Variant* attribute) override;

	//	virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

	//	static unsigned int getStaticClassId(void)
	//	{
	//		return NOWA::getIdFromName("JointControlledBallAndSocketComponent");
	//	}

	//	static Ogre::String getStaticClassName(void)
	//	{
	//		return "JointControlledBallAndSocketComponent";
	//	}

	/**
	* @see  GameObjectComponent::createStaticApiForLua
	*/
	// static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

	/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
	// static Ogre::String getStaticInfoText(void)
	// {
	// 	return "Requirements: A kind of physics component must exist.";
	// }

	//	void setAnchorPosition(const Ogre::Vector3& anchorPosition);

	//	Ogre::Vector3 getAnchorPosition(void) const;
	//	
	//	void setAngleVelocity(Ogre::Real angleVelocity);

	//	Ogre::Real getAngleVelocity(void) const;

	//	void setPitchYawRollAngle(const Ogre::Degree& pitchAngle, const Ogre::Degree& yawAngle, const Ogre::Degree& rollAngle);

	//	std::tuple<Ogre::Degree, Ogre::Degree, Ogre::Degree> getPitchYawRollAngle(void) const;

	//public:
	//	// Attribute constants
	//	static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
	//	static const Ogre::String AttrAngleVelocity(void) { return "Angle Velocity"; }
	//	static const Ogre::String AttrPitchAngle(void) { return "Pitch Angle"; }
	//	static const Ogre::String AttrYawAngle(void) { return "Yaw Angle"; }
	//	static const Ogre::String AttrRollAngle(void) { return "Roll Angle"; }
	//protected:
	//	Variant* anchorPosition;
	//	Variant* pitchAngle;
	//	Variant* yawAngle;
	//	Variant* rollAngle;
	//	Variant* angleVelocity;
	//};
	
	/*******************************RagDollMotorDofComponent*******************************/

	//class EXPORTED RagDollMotorDofComponent : public JointComponent
	//{
	//public:
	//	typedef boost::shared_ptr<RagDollMotorDofComponent> RagDollMotorDofCompPtr;
	//public:
	//	RagDollMotorDofComponent(void);

	//	virtual ~RagDollMotorDofComponent() override;

	//	virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

	//	virtual bool postInit(void) override;

	//	virtual bool connect(void) override;

	//	virtual bool disconnect(void) override;

	//	virtual Ogre::String getClassName(void) const override;

	//	virtual Ogre::String getParentClassName(void) const override;

	//	virtual void update(Ogre::Real dt, bool notSimulating = false) override;

	//	virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

	//	virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

	//	virtual void actualizeValue(Variant* attribute) override;

	//	virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

	//	static unsigned int getStaticClassId(void)
	//	{
	//		return NOWA::getIdFromName("RagDollMotorDofComponent");
	//	}

	//	static Ogre::String getStaticClassName(void)
	//	{
	//		return "JointBallAndSocketComponent";
	//	}



	/**
	 * @see  GameObjectComponent::createStaticApiForLua
	 */
	// static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

	/**
	* @see	GameObjectComponent::getStaticInfoText
	*/
	// static Ogre::String getStaticInfoText(void)
	// {
	// 	return "Requirements: A kind of physics component must exist.";
	// }

	//	void setAnchorPosition(const Ogre::Vector3& anchorPosition);

	//	Ogre::Vector3 getAnchorPosition(void) const;

	//	void setMinMaxConeAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit, const Ogre::Degree& maxConeLimit);

	//	std::tuple<Ogre::Degree, Ogre::Degree, Ogre::Degree> getMinMaxConeAngleLimit(void) const;
	//	
	//	void setDofCount(unsigned int dofCount);
	//	
	//	unsigned int getDofCount(void) const;
	//	
	//	void setMotorOn(bool on);
	//	
	//	bool isMotorOn(void) const;
	//	
	//	void setTorque(Ogre::Real torque);
	//	
	//	Ogre::Real getTorque(void) const;

	//public:
	//	// Attribute constants
	//	static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
	//	static const Ogre::String AttrMinAngleLimit(void) { return "Min Angle"; }
	//	static const Ogre::String AttrMaxAngleLimit(void) { return "Max Angle"; }
	//	static const Ogre::String AttrMaxConeLimit(void) { return "Max Cone Angle"; }
	//	static const Ogre::String AttrDofCount(void) { return "DOF Count"; }
	//	static const Ogre::String AttrMotorOn(void) { return "Motor On"; }
	//	static const Ogre::String AttrTorque(void) { return "Torque"; }
	//protected:
	//	Variant* anchorPosition;
	//	Variant* minAngleLimit;
	//	Variant* maxAngleLimit;
	//	Variant* maxConeLimit;
	//	Variant* dofCount;
	//	Variant* motorOn;
	//	Variant* torque;
	//};

	/*******************************JointPinComponent*******************************/

	class EXPORTED JointPinComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointPinComponent> JointPinCompPtr;
	public:
		JointPinComponent(void);

		virtual ~JointPinComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointPinComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointPinComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;
	public:
		static const Ogre::String AttrPin(void) { return "Pin"; }
	protected:
		Variant* pin;
	};
	
	/*******************************JointPlaneComponent*******************************/

	class EXPORTED JointPlaneComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointPlaneComponent> JointPlaneCompPtr;
	public:
		JointPlaneComponent(void);

		virtual ~JointPlaneComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointPlaneComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointPlaneComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}
		
		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setNormal(const Ogre::Vector3& normal);

		Ogre::Vector3 getNormal(void) const;
	public:
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrNormal(void) { return "Normal"; }
	protected:
		Variant* normal;
		Variant* anchorPosition;
	};

	/*******************************JointSpringComponent*******************************/

	class EXPORTED JointSpringComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointSpringComponent> JointSpringCompPtr;
	public:
		JointSpringComponent(void);

		virtual ~JointSpringComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;
		
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointSpringComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointSpringComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setSpringStrength(Ogre::Real springStrength);

		Ogre::Real getSpringStrength(void) const;

		void setShowLine(bool showLine);

		bool getShowLine(void) const;

		void setAnchorOffsetPosition(const Ogre::Vector3& anchorOffsetPosition);

		const Ogre::Vector3 getAnchorOffsetPosition(void);

		void setSpringOffsetPosition(const Ogre::Vector3& springOffsetPosition);

		const Ogre::Vector3 getSpringOffsetPosition(void);

		void releaseSpring(void);

		void drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition);
	public:
		// Attribute constants
		static const Ogre::String AttrSpringStrength(void) { return "Spring Strength"; }
		static const Ogre::String AttrAnchorOffsetPosition(void) { return "Anchor Offset Pos"; }
		static const Ogre::String AttrSpringOffsetPosition(void) { return "Spring Offset Pos"; }
		static const Ogre::String AttrShowLine(void) { return "Show Line"; }
		static const Ogre::String AttrMaxConeLimit(void) { return "Max Cone Angle"; }
		static const Ogre::String AttrStiffness(void) { return "Stiffness"; }
	private:
		void createLine(void);
	protected:
		Variant* springStrength;
		Variant* anchorOffsetPosition;
		Variant* springOffsetPosition;
		Variant* showLine;
		OgreNewt::Body* predecessorBody;
		Ogre::SceneNode* dragLineNode;
		Ogre::ManualObject* dragLineObject;
	};

	/*******************************JointAttractorComponent*******************************/

	class EXPORTED JointAttractorComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointAttractorComponent> JointAttractorCompPtr;
	public:
		JointAttractorComponent(void);

		virtual ~JointAttractorComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointAttractorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointAttractorComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setMagneticStrength(Ogre::Real magneticStrength);

		Ogre::Real getMagneticStrength(void) const;

		void setAttractionDistance(Ogre::Real attractionDistance);

		Ogre::Real getAttractionDistance(void) const;

		void setRepellerCategory(const Ogre::String& repellerCategory);

		const Ogre::String getRepellerCategory(void);
	public:
		// Attribute constants
		static const Ogre::String AttrMagneticStrength(void) { return "Magnetic Strength"; }
		static const Ogre::String AttrAttractionDistance(void) { return "Attraction Distance"; }
		static const Ogre::String AttrRepellerCategory(void) { return "Repeller Category"; }
	protected:
		Variant* magneticStrength;
		Variant* attractionDistance;
		Variant* repellerCategory;
	};

	/*******************************JointCorkScrewComponent*******************************/

	class EXPORTED JointCorkScrewComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointCorkScrewComponent> JointCorkScrewCompPtr;
	public:
		JointCorkScrewComponent(void);

		virtual ~JointCorkScrewComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointCorkScrewComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointCorkScrewComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setLinearLimitsEnabled(bool enableLinearLimits);

		bool getLinearLimitsEnabled(void) const;

		void setAngleLimitsEnabled(bool enableAngleLimits);

		bool getAngleLimitsEnabled(void) const;

		void setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance);
		
		Ogre::Real getMinStopDistance(void) const;
		
		Ogre::Real getMaxStopDistance(void) const;

		void setMinMaxAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit);
		
		Ogre::Degree getMinAngleLimit(void) const;
		
		Ogre::Degree getMaxAngleLimit(void) const;
		
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrLinearLimits(void) { return "Linear Limits"; }
		static const Ogre::String AttrAngleLimits(void) { return "Angle Limits"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrMinAngleLimit(void) { return "Min Angle Angle"; }
		static const Ogre::String AttrMaxAngleLimit(void) { return "Max Angluar Angle"; }
	protected:
		Variant* anchorPosition;
		Variant* enableLinearLimits;
		Variant* enableAngleLimits;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* minAngleLimit;
		Variant* maxAngleLimit;
	};

	/*******************************JointPassiveSliderComponent*******************************/

	class EXPORTED JointPassiveSliderComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointPassiveSliderComponent> JointPassiveSliderCompPtr;
	public:
		JointPassiveSliderComponent(void);

		virtual ~JointPassiveSliderComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointPassiveSliderComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointPassiveSliderComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;

		void setLimitsEnabled(bool enableLimits);

		bool getLimitsEnabled(void) const;

		void setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance);
		
		Ogre::Real getMinStopDistance(void) const;
		
		Ogre::Real getMaxStopDistance(void) const;

		void setSpring(bool asSpringDamper);

		void setSpring(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD);

		std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> getSpring(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrLimits(void) { return "Limits"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrAsSpringDamper(void) { return "As Spring Damper"; }
		static const Ogre::String AttrSpringK(void) { return "SpringK"; }
		static const Ogre::String AttrSpringD(void) { return "SpringD"; }
		static const Ogre::String AttrSpringDamperRelaxation(void) { return "Damper Relaxation"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* enableLimits;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* asSpringDamper;
		Variant* springK;
		Variant* springD;
		Variant* springDamperRelaxation;
	};
	
	/*******************************JointSliderActuatorComponent*******************************/

	class EXPORTED JointSliderActuatorComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointSliderActuatorComponent> JointSliderActuatorCompPtr;
	public:
		JointSliderActuatorComponent();

		virtual ~JointSliderActuatorComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;
		
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointSliderActuatorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointSliderActuatorComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		virtual void setActivated(bool activated) override;

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;
	
		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;
		
		void setTargetPosition(Ogre::Real targetPosition);
		
		Ogre::Real getTargetPosition(void) const;
		
		void setLinearRate(Ogre::Real linearRate);
		
		Ogre::Real getLinearRate(void) const;
		
		void setMinStopDistance(Ogre::Real minStopDistance);
		
		Ogre::Real getMinStopDistance(void) const;
		
		void setMaxStopDistance(Ogre::Real maxStopDistance);
		
		Ogre::Real getMaxStopDistance(void) const;
		
		void setMinForce(Ogre::Real force);
		
		Ogre::Real getMinForce(void) const;
		
		void setMaxForce(Ogre::Real force);
		
		Ogre::Real getMaxForce(void) const;
		
		void setDirectionChange(bool directionChange);
		
		bool getDirectionChange(void) const;
		
		void setRepeat(bool repeat);
		
		bool getRepeat(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrTargetPosition(void) { return "Target Position"; }
		static const Ogre::String AttrLinearRate(void) { return "Linear Rate"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrMinForce(void) { return "Min Force"; }
		static const Ogre::String AttrMaxForce(void) { return "Max Force"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* targetPosition;
		Variant* linearRate;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* minForce;
		Variant* maxForce;
		Variant* directionChange;
		Variant* repeat;
		short round;
		bool internalDirectionChange;
		Ogre::Real oppositeDir;
	};

	/*******************************JointSlidingContactComponent*******************************/

	class EXPORTED JointSlidingContactComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointSlidingContactComponent> JointSlidingContactCompPtr;
	public:
		JointSlidingContactComponent(void);

		virtual ~JointSlidingContactComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointSlidingContactComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointSlidingContactComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;

		void setLinearLimitsEnabled(bool enableLinearLimits);

		bool getLinearLimitsEnabled(void) const;

		void setAngleLimitsEnabled(bool enableAngleLimits);

		bool getAngleLimitsEnabled(void) const;

		void setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance);
		
		Ogre::Real getMinStopDistance(void) const;
		
		Ogre::Real getMaxStopDistance(void) const;

		void setMinMaxAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit);
		
		Ogre::Real getMinAngleLimit(void) const;
		
		Ogre::Real getMaxAngleLimit(void) const;

		void setSpring(bool asSpringDamper);

		void setSpring(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD);

		std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> getSpring(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrLinearLimits(void) { return "Linear Limits"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrAngleLimits(void) { return "Angle Limits"; }
		static const Ogre::String AttrMinAngleLimit(void) { return "Min Angle"; }
		static const Ogre::String AttrMaxAngleLimit(void) { return "Max Angle"; }
		static const Ogre::String AttrAsSpringDamper(void) { return "As Spring Damper"; }
		static const Ogre::String AttrSpringK(void) { return "SpringK"; }
		static const Ogre::String AttrSpringD(void) { return "SpringD"; }
		static const Ogre::String AttrSpringDamperRelaxation(void) { return "Damper Relaxation"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* enableLinearLimits;
		Variant* enableAngleLimits;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* minAngleLimit;
		Variant* maxAngleLimit;
		Variant* asSpringDamper;
		Variant* springK;
		Variant* springD;
		Variant* springDamperRelaxation;
	};

	/*******************************JointActiveSliderComponent*******************************/

	class EXPORTED JointActiveSliderComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointActiveSliderComponent> JointActiveSliderCompPtr;
	public:
		JointActiveSliderComponent(void);

		virtual ~JointActiveSliderComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointActiveSliderComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointActiveSliderComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;
		
		void setLimitsEnabled(bool enableLimits);

		bool getLimitsEnabled(void) const;

		void setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance);
		
		Ogre::Real getMinStopDistance(void) const;
		
		Ogre::Real getMaxStopDistance(void) const;

		int getCurrentDirection(void) const;

		void setMoveSpeed(Ogre::Real moveSpeed);

		Ogre::Real getMoveSpeed(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;
	public:
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrLimits(void) { return "Limits"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrMoveSpeed(void) { return "Move Speed"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* enableLimits;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* moveSpeed;
		Variant* repeat;
		Variant* directionChange;
	};

	/*******************************JointMathSliderComponent*******************************/

	class EXPORTED JointMathSliderComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointMathSliderComponent> JointMathSliderCompPtr;
	public:
		JointMathSliderComponent(void);

		virtual ~JointMathSliderComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointMathSliderComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointMathSliderComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setLimitsEnabled(bool enableLimits);

		bool getLimitsEnabled(void) const;

		void setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance);
		
		Ogre::Real getMinStopDistance(void) const;
		
		Ogre::Real getMaxStopDistance(void) const;

		void setMoveSpeed(Ogre::Real moveSpeed);

		Ogre::Real getMoveSpeed(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;

		void setFunctionX(const Ogre::String& mx);

		Ogre::String getFunctionX(void) const;

		void setFunctionY(const Ogre::String& my);

		Ogre::String getFunctionY(void) const;

		void setFunctionZ(const Ogre::String& mz);

		Ogre::String getFunctionZ(void) const;

	public:
		static const Ogre::String AttrFunctionX(void) { return "Function X"; }
		static const Ogre::String AttrFunctionY(void) { return "Function Y"; }
		static const Ogre::String AttrFunctionZ(void) { return "Function Z"; }
		static const Ogre::String AttrLimits(void) { return "Limits"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrMoveSpeed(void) { return "Move Speed"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrDirectionChange(void) { return "Direction Change"; }
	private:
		void parseMathematicalFunction(void);
	protected:
		Variant* mx;
		Variant* my;
		Variant* mz;
		Variant* enableLimits;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* moveSpeed;
		Variant* repeat;
		Variant* directionChange;
		FunctionParser functionParser[3];
		Ogre::Real oppositeDir;
		Ogre::Real progress;
		short round;
	};

	/*******************************JointKinematicComponent*******************************/

	class EXPORTED JointKinematicComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointKinematicComponent> JointKinematicCompPtr;
	public:
		JointKinematicComponent(void);

		virtual ~JointKinematicComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointKinematicComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointKinematicComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setPickingMode(short mode);

		short getPickingMode(void) const;

		void setMaxLinearAngleFriction(Ogre::Real maxLinearFriction, Ogre::Real maxAngleFriction);

		std::tuple<Ogre::Real, Ogre::Real> getMaxLinearAngleFriction(void) const;

		void setTargetPosition(const Ogre::Vector3& targetPosition);

		Ogre::Vector3 getTargetPosition(void) const;

		void setTargetRotation(const Ogre::Quaternion& targetRotation);

		void setTargetPositionRotation(const Ogre::Vector3& targetPosition, const Ogre::Quaternion& targetRotation);

		Ogre::Quaternion getTargetRotation(void) const;

		void setMaxSpeed(Ogre::Real speedInMetersPerSeconds);

		Ogre::Real getMaxSpeed(void) const;

		void setMaxOmega(const Ogre::Real& speedInDegreesPerSeconds);

		Ogre::Real getMaxOmega(void) const;

		void backToOrigin(void);

	public:
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrMode(void) { return "Mode"; }
		static const Ogre::String AttrMaxLinearFriction(void) { return "Max. Linear Friction"; }
		static const Ogre::String AttrMaxAngleFriction(void) { return "Max. Angle Friction"; }
		static const Ogre::String AttrMaxSpeed(void) { return "Max. Speed"; }
		static const Ogre::String AttrMaxOmega(void) { return "Max. Omega"; }
		static const Ogre::String AttrTargetPosition(void) { return "Target Position"; }
		static const Ogre::String AttrTargetRotation(void) { return "Target Rotation"; }
	protected:
		Variant* anchorPosition;
		Variant* mode;
		Variant* maxLinearFriction;
		Variant* maxAngleFriction;
		Variant* maxSpeed;
		Variant* maxOmega;
		Variant* targetPosition;
		Variant* targetRotation;
		Ogre::Vector3 originPosition;
		Ogre::Quaternion originRotation;
		Ogre::Vector3 gravity;
	};

	/*******************************JointTargetTransformComponent*******************************/

	class EXPORTED JointTargetTransformComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointTargetTransformComponent> JointTargetTransformCompPtr;
	public:
		JointTargetTransformComponent(void);

		virtual ~JointTargetTransformComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		void setOffsetOrientation(const Ogre::Quaternion& offsetOrientation);

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointTargetTransformComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointTargetTransformComponent";
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
			return "Requirements: A kind of physics component must exist and a predecessor joint component. This joint will synchronize the transform of the predecessor joint.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

	public:
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
	protected:
		Variant* anchorPosition;
		Ogre::Vector3 offsetPosition;
		Ogre::Quaternion offsetOrientation;
	};

	/*******************************JointPathFollowComponent*******************************/

	class EXPORTED JointPathFollowComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointPathFollowComponent> JointPathFollowCompPtr;
	public:
		JointPathFollowComponent(void);

		virtual ~JointPathFollowComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointPathFollowComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointPathFollowComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		virtual void setActivated(bool activated) override;

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setWaypointsCount(unsigned int waypointsCount);

		unsigned int getWaypointsCount(void) const;

		void setWaypointId(unsigned long id, unsigned int index);

		unsigned long getWaypointId(unsigned int index);

	public:
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrWaypointsCount(void) { return "Waypoints Count"; }
		static const Ogre::String AttrWaypoint(void) { return "Waypoint Id "; }
	private:
		void createLines(void);

		void drawLines(void);

		void destroyLines(void);
	protected:
		Variant* anchorPosition;
		Variant* waypointsCount;
		std::vector<Variant*> waypoints;

		std::vector<Ogre::Vector3> knots;
		Ogre::SceneNode* lineNode;
		Ogre::ManualObject* lineObjects;
	};

	/*******************************JointDryRollingFrictionComponent*******************************/

	class EXPORTED JointDryRollingFrictionComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointDryRollingFrictionComponent> JointDryRollingFrictionCompPtr;
	public:
		JointDryRollingFrictionComponent(void);

		virtual ~JointDryRollingFrictionComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointDryRollingFrictionComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointDryRollingFrictionComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setRadius(Ogre::Real radius);

		Ogre::Real getRadius(void) const;

		void setRollingFrictionCoefficient(Ogre::Real rollingFrictionCoefficient);

		Ogre::Real getRollingFrictionCoefficient(void) const;
	public:
		static const Ogre::String AttrRadius(void) { return "Radius"; }
		static const Ogre::String AttrFrictionCoefficient(void) { return "Friction Coeff."; }
	protected:
		Variant* radius;
		Variant* rollingFrictionCoefficient;
	};

	/*******************************JointGearComponent*******************************/

	class EXPORTED JointGearComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointGearComponent> JointGearCompPtr;
	public:
		JointGearComponent(void);

		virtual ~JointGearComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointGearComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointGearComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setGearRatio(Ogre::Real gearRatio);

		Ogre::Real getGearRatio(void) const;
	public:
		static const Ogre::String AttrGearRatio(void) { return "Gear Ratio"; }
	protected:
		Variant* gearRatio;
	};
	
	/*******************************JointRackAndPinionComponent*******************************/

	class EXPORTED JointRackAndPinionComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointRackAndPinionComponent> JointRackAndPinionCompPtr;
	public:
		JointRackAndPinionComponent(void);

		virtual ~JointRackAndPinionComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointRackAndPinionComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointRackAndPinionComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setGearRatio(Ogre::Real gearRatio);

		Ogre::Real getGearRatio(void) const;
	public:
		static const Ogre::String AttrGearRatio(void) { return "Gear Ratio"; }
	protected:
		Variant* gearRatio;
	};

	/*******************************JointWormGearComponent*******************************/

	class EXPORTED JointWormGearComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointWormGearComponent> JointWormGearCompPtr;
	public:
		JointWormGearComponent(void);

		virtual ~JointWormGearComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointWormGearComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointWormGearComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setGearRatio(Ogre::Real gearRatio);

		Ogre::Real getGearRatio(void) const;
		
		void setSlideRatio(Ogre::Real slideRatio);

		Ogre::Real getSlideRatio(void) const;
	public:
		static const Ogre::String AttrGearRatio(void) { return "Gear Ratio"; }
		static const Ogre::String AttrSlideRatio(void) { return "Slide Ratio"; }
	protected:
		Variant* gearRatio;
		Variant* slideRatio;
	};

	/*******************************JointPulleyComponent*******************************/

	class EXPORTED JointPulleyComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointPulleyComponent> JointPulleyCompPtr;
	public:
		JointPulleyComponent(void);

		virtual ~JointPulleyComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointPulleyComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointPulleyComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setPulleyRatio(Ogre::Real pulleyRatio);

		Ogre::Real getPulleyRatio(void) const;
	public:
		static const Ogre::String AttrPulleyRatio(void) { return "Pulley Ratio"; }
	protected:
		Variant* pulleyRatio;
	};

	/*******************************JointUniversalComponent*******************************/

	class EXPORTED JointUniversalComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointUniversalComponent> JointUniversalCompPtr;
	public:
		JointUniversalComponent(void);

		virtual ~JointUniversalComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointUniversalComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointUniversalComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;

		void setLimits0Enabled(bool enableLimits0);

		bool getLimits0Enabled(void) const;

		void setMinMaxAngleLimit0(const Ogre::Degree& minAngleLimit0, const Ogre::Degree& maxAngleLimit0);
		
		Ogre::Degree getMinAngleLimit0(void) const;
		
		Ogre::Degree getMaxAngleLimit0(void) const;

		void setLimits1Enabled(bool enableLimits1);

		bool getLimits1Enabled(void) const;

		void setMinMaxAngleLimit1(const Ogre::Degree& minAngleLimit1, const Ogre::Degree& maxAngleLimit1);
		
		Ogre::Degree getMinAngleLimit1(void) const;
		
		Ogre::Degree getMaxAngleLimit1(void) const;

		void setMotorEnabled(bool enableMotor0);

		bool getMotorEnabled(void) const;

		void setMotorSpeed(Ogre::Real motorSpeed);

		Ogre::Real getMotorSpeed(void) const;

		void setFriction0(Ogre::Real frictionTorque);

		Ogre::Real getFriction0(void) const;

		void setFriction1(Ogre::Real frictionTorque);

		Ogre::Real getFriction1(void) const;

		void setSpring0(bool asSpringDamper);

		void setSpring0(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD);

		std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> getSpring0(void) const;

		void setSpring1(bool asSpringDamper);

		void setSpring1(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD);

		std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> getSpring1(void) const;

	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrLimits0(void) { return "Limits 0"; }
		static const Ogre::String AttrMinAngleLimit0(void) { return "Min Angle 0"; }
		static const Ogre::String AttrMaxAngleLimit0(void) { return "Max Angle 0"; }
		static const Ogre::String AttrLimits1(void) { return "Limits 1"; }
		static const Ogre::String AttrMinAngleLimit1(void) { return "Min Angle 1"; }
		static const Ogre::String AttrMaxAngleLimit1(void) { return "Max Angle 1"; }
		static const Ogre::String AttrEnableMotor(void) { return "Enable Motor"; }
		static const Ogre::String AttrMotorSpeed(void) { return "Motor Speed"; }
		static const Ogre::String AttrAsSpringDamper0(void) { return "As Spring Damper 0"; }
		static const Ogre::String AttrSpringK0(void) { return "SpringK 0"; }
		static const Ogre::String AttrSpringD0(void) { return "SpringD 0"; }
		static const Ogre::String AttrSpringDamperRelaxation0(void) { return "Damper Relaxation 0"; }
		static const Ogre::String AttrAsSpringDamper1(void) { return "As Spring Damper 1"; }
		static const Ogre::String AttrSpringK1(void) { return "SpringK 1"; }
		static const Ogre::String AttrSpringD1(void) { return "SpringD 1"; }
		static const Ogre::String AttrSpringDamperRelaxation1(void) { return "Damper Relaxation 1"; }
		static const Ogre::String AttrFriction0(void) { return "Friction 0"; }
		static const Ogre::String AttrFriction1(void) { return "Friction 1"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* enableLimits0;
		Variant* minAngleLimit0;
		Variant* maxAngleLimit0;
		Variant* enableLimits1;
		Variant* minAngleLimit1;
		Variant* maxAngleLimit1;
		Variant* enableMotor;
		Variant* motorSpeed;
		Variant* asSpringDamper0;
		Variant* springK0;
		Variant* springD0;
		Variant* springDamperRelaxation0;
		Variant* asSpringDamper1;
		Variant* springK1;
		Variant* springD1;
		Variant* springDamperRelaxation1;
		Variant* friction0;
		Variant* friction1;
	};
	
	/*******************************JointUniversalActuatorComponent*******************************/

	class EXPORTED JointUniversalActuatorComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointUniversalActuatorComponent> JointUniversalActuatorCompPtr;
	public:
		JointUniversalActuatorComponent(void);

		virtual ~JointUniversalActuatorComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointUniversalActuatorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointUniversalActuatorComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		virtual void setActivated(bool activated) override;

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;
		
		void setTargetAngle0(const Ogre::Degree& targetAngle0);
		
		Ogre::Degree getTargetAngle0(void) const;
		
		void setAngleRate0(const Ogre::Degree& angleRate0);
		
		Ogre::Degree getAngleRate0(void) const;

		void setMinAngleLimit0(const Ogre::Degree& minAngleLimit0);

		Ogre::Degree getMinAngleLimit0(void) const;
		
		void setMaxAngleLimit0(const Ogre::Degree& maxAngleLimit0);

		Ogre::Degree getMaxAngleLimit0(void) const;

		void setMaxTorque0(Ogre::Real maxTorque0);

		Ogre::Real getMaxTorque0(void) const;
		
		void setDirectionChange0(bool directionChange0);
		
		bool getDirectionChange0(void) const;
		
		void setRepeat0(bool repeat0);
		
		bool getRepeat0(void) const;
		
		void setTargetAngle1(const Ogre::Degree& targetAngle1);
		
		Ogre::Degree getTargetAngle1(void) const;
		
		void setAngleRate1(const Ogre::Degree& angleRate1);
		
		Ogre::Degree getAngleRate1(void) const;

		void setMinAngleLimit1(const Ogre::Degree& minAngleLimit1);

		Ogre::Degree getMinAngleLimit1(void) const;
		
		void setMaxAngleLimit1(const Ogre::Degree& maxAngleLimit1);

		Ogre::Degree getMaxAngleLimit1(void) const;

		void setMaxTorque1(Ogre::Real maxTorque1);

		Ogre::Real getMaxTorque1(void) const;
		
		void setDirectionChange1(bool directionChange1);
		
		bool getDirectionChange1(void) const;
		
		void setRepeat1(bool repeat1);
		
		bool getRepeat1(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrTargetAngle0(void) { return "Target Angle 0"; }
		static const Ogre::String AttrAngleRate0(void) { return "Angle Rate 0"; }
		static const Ogre::String AttrMinAngleLimit0(void) { return "Min Angle Limit 0"; }
		static const Ogre::String AttrMaxAngleLimit0(void) { return "Max Angle Limit 0"; }
		static const Ogre::String AttrMaxTorque0(void) { return "Max Torque 0"; }
		static const Ogre::String AttrDirectionChange0(void) { return "Direction Change 0"; }
		static const Ogre::String AttrRepeat0(void) { return "Repeat 0"; }
		static const Ogre::String AttrTargetAngle1(void) { return "Target Angle 1"; }
		static const Ogre::String AttrAngleRate1(void) { return "Angle Rate 1"; }
		static const Ogre::String AttrMinAngleLimit1(void) { return "Min Angle Limit 1"; }
		static const Ogre::String AttrMaxAngleLimit1(void) { return "Max Angle Limit 1"; }
		static const Ogre::String AttrMaxTorque1(void) { return "Max Torque 1"; }
		static const Ogre::String AttrDirectionChange1(void) { return "Direction Change 1"; }
		static const Ogre::String AttrRepeat1(void) { return "Repeat 1"; }
	protected:
		Variant* anchorPosition;
		Variant* targetAngle0;
		Variant* angleRate0;
		Variant* minAngleLimit0;
		Variant* maxAngleLimit0;
		Variant* maxTorque0;
		Variant* directionChange0;
		Variant* repeat0;
		short round0;
		bool internalDirectionChange0;
		Ogre::Real oppositeDir0;
		Variant* targetAngle1;
		Variant* angleRate1;
		Variant* minAngleLimit1;
		Variant* maxAngleLimit1;
		Variant* maxTorque1;
		Variant* directionChange1;
		Variant* repeat1;
		short round1;
		bool internalDirectionChange1;
		Ogre::Real oppositeDir1;
	};
	
	/*******************************Joint6DofComponent*******************************/

	class EXPORTED Joint6DofComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<Joint6DofComponent> Joint6DofCompPtr;
	public:
		Joint6DofComponent(void);

		virtual ~Joint6DofComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;
		
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("Joint6DofComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "Joint6DofComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition0(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition0(void) const;

		void setAnchorPosition1(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition1(void) const;

		void setMinMaxStopDistance(const Ogre::Vector3& minStopDistance, const Ogre::Vector3& maxStopDistance);
		
		Ogre::Vector3 getMinStopDistance(void) const;
		
		Ogre::Vector3 getMaxStopDistance(void) const;

		void setMinMaxYawAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit);
		
		Ogre::Degree getMinYawAngleLimit(void) const;
		
		Ogre::Degree getMaxYawAngleLimit(void) const;

		void setMinMaxPitchAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit);
		
		Ogre::Degree getMinPitchAngleLimit(void) const;
		
		Ogre::Degree getMaxPitchAngleLimit(void) const;

		void setMinMaxRollAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit);
		
		Ogre::Degree getMinRollAngleLimit(void) const;
		
		Ogre::Degree getMaxRollAngleLimit(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition0(void) { return "Anchor Position 0"; }
		static const Ogre::String AttrAnchorPosition1(void) { return "Anchor Position 1"; }
		static const Ogre::String AttrMinStopDistance(void) { return "Min Stop Distance"; }
		static const Ogre::String AttrMaxStopDistance(void) { return "Max Stop Distance"; }
		static const Ogre::String AttrMinYawAngleLimit(void) { return "Min Yaw Angle"; }
		static const Ogre::String AttrMaxYawAngleLimit(void) { return "Max Yaw Angle"; }
		static const Ogre::String AttrMinPitchAngleLimit(void) { return "Min Pitch Angle"; }
		static const Ogre::String AttrMaxPitchAngleLimit(void) { return "Max Pitch Angle"; }
		static const Ogre::String AttrMinRollAngleLimit(void) { return "Min Roll Angle"; }
		static const Ogre::String AttrMaxRollAngleLimit(void) { return "Max Roll Angle"; }
	protected:
		Variant* anchorPosition0;
		Variant* anchorPosition1;
		Variant* minStopDistance;
		Variant* maxStopDistance;
		Variant* minYawAngleLimit;
		Variant* maxYawAngleLimit;
		Variant* minPitchAngleLimit;
		Variant* maxPitchAngleLimit;
		Variant* minRollAngleLimit;
		Variant* maxRollAngleLimit;
	};

	/*******************************JointMotorComponent*******************************/

	class EXPORTED JointMotorComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointMotorComponent> JointMotorCompPtr;
	public:
		JointMotorComponent(void);

		virtual ~JointMotorComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointMotorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointMotorComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}
	
		void setPin0(const Ogre::Vector3& pin0);

		Ogre::Vector3 getPin0(void) const;

		void setPin1(const Ogre::Vector3& pin1);

		Ogre::Vector3 getPin1(void) const;

		void setSpeed0(Ogre::Real speed0);

		Ogre::Real getSpeed0(void) const;

		void setSpeed1(Ogre::Real speed1);

		Ogre::Real getSpeed1(void) const;

		void setTorgue0(Ogre::Real torgue0);

		Ogre::Real getTorgue0(void) const;

		void setTorgue1(Ogre::Real torgue1);

		Ogre::Real getTorgue1(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrPin0(void) { return "Pin0"; }
		static const Ogre::String AttrPin1(void) { return "Pin1"; }
		static const Ogre::String AttrSpeed0(void) { return "Speed 0"; }
		static const Ogre::String AttrSpeed1(void) { return "Speed 1"; }
		static const Ogre::String AttrTorque0(void) { return "Torque 0"; }
		static const Ogre::String AttrTorque1(void) { return "Torque 1"; }
	protected:
		Variant* pin0;
		Variant* pin1;
		Variant* speed0;
		Variant* speed1;
		Variant* torgue0;
		Variant* torgue1;
	};

	/*******************************JointWheelComponent*******************************/

	class EXPORTED JointWheelComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointWheelComponent> JointWheelCompPtr;
	public:
		JointWheelComponent(void);

		virtual ~JointWheelComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;
		
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void forceShowDebugData(bool activate) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointWheelComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointWheelComponent";
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
			return "Requirements: A kind of physics component must exist.";
		}

		void setAnchorPosition(const Ogre::Vector3& anchorPosition);

		Ogre::Vector3 getAnchorPosition(void) const;

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;

		Ogre::Real getSteeringAngle(void) const;

		void setTargetSteeringAngle(const Ogre::Degree& targetSteeringAngle);

		Ogre::Degree getTargetSteeringAngle(void) const;

		void setSteeringRate(Ogre::Real steeringRate);

		Ogre::Real getSteeringRate(void) const;
	public:
		// Attribute constants
		static const Ogre::String AttrAnchorPosition(void) { return "Anchor Position"; }
		static const Ogre::String AttrPin(void) { return "Pin"; }
		static const Ogre::String AttrTargetSteeringAngle(void) { return "Target Steering Angle"; }
		static const Ogre::String AttrSteeringRate(void) { return "Steering Rate"; }
	protected:
		Variant* anchorPosition;
		Variant* pin;
		Variant* targetSteeringAngle;
		Variant* steeringRate;
	};

	/*******************************JointFlexyPipeHandleComponent*******************************/

	class EXPORTED JointFlexyPipeHandleComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointFlexyPipeHandleComponent> JointFlexyPipeHandleCompPtr;
	public:
		JointFlexyPipeHandleComponent(void);

		virtual ~JointFlexyPipeHandleComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointFlexyPipeHandleComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointFlexyPipeHandleComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics component must exist.";
		}

		void setPin(const Ogre::Vector3& pin);

		Ogre::Vector3 getPin(void) const;

		void setVelocity(const Ogre::Vector3& velocity, Ogre::Real dt);

		void setOmega(const Ogre::Vector3& omega, Ogre::Real dt);
	public:
		static const Ogre::String AttrPin(void) { return "Pin"; }
	protected:
		Variant* pin;
	};

	/*******************************JointFlexyPipeSpinnerComponent*******************************/

	class EXPORTED JointFlexyPipeSpinnerComponent : public JointComponent
	{
	public:
		typedef boost::shared_ptr<JointFlexyPipeSpinnerComponent> JointFlexyPipeSpinnerCompPtr;
	public:
		JointFlexyPipeSpinnerComponent(void);

		virtual ~JointFlexyPipeSpinnerComponent() override;

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool createJoint(const Ogre::Vector3& customJointPosition = Ogre::Vector3::ZERO) override;

		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("JointFlexyPipeSpinnerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "JointFlexyPipeSpinnerComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController)
		{
		}

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A kind of physics component must exist.";
		}
	};
	
}; // namespace end

#endif