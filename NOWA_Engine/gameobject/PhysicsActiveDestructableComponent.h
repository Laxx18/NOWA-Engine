#ifndef PHYSICS_ACTIVE_DESTRUCTABLE_COMPONENT_H
#define PHYSICS_ACTIVE_DESTRUCTABLE_COMPONENT_H

#include "PhysicsActiveComponent.h"

namespace NOWA
{
	class JointComponent;
	typedef boost::shared_ptr<JointComponent> JointCompPtr;
	
	class EXPORTED PhysicsActiveDestructableComponent : public PhysicsActiveComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsActiveDestructableComponent> PhysicsActiveDestructableCompPtr;
	public:

		PhysicsActiveDestructableComponent();

		virtual ~PhysicsActiveDestructableComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual Ogre::String getParentParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual bool isMovable(void) const override
		{
			return true;
		}

		virtual void setOrientation(const Ogre::Quaternion& orientation);

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		virtual void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z) override;

		virtual void setPosition(const Ogre::Vector3& position) override;

		virtual void addImpulse(const Ogre::Vector3& deltaVector) override;

		virtual void translate(const Ogre::Vector3& relativePosition) override;

		virtual void setVelocity(const Ogre::Vector3& velocity) override;

		virtual void setActivated(bool activated) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsActiveDestructableComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsActiveDestructableComponent";
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
			return "Usage: This Component transforms the given game object mesh into pieces with an external tool and glues the parts together for destruction, which happens when a force or torque exceeds a critical factor.";
		}

		void setBreakForce(Ogre::Real breakForce);

		void setBreakTorque(Ogre::Real breakTorque);

		void setDensity(Ogre::Real density);

		void setMotionLess(bool motionless);

	protected:

		virtual bool createDynamicBody(void);

		class EXPORTED SplitPart
		{
		private:
			PhysicsActiveDestructableComponent* physicsActiveDestructableComponent;
		public:
			SplitPart(PhysicsActiveDestructableComponent* physicsActiveDestructableComponent, const Ogre::String& meshName, Ogre::SceneManager* sceneManager);

			~SplitPart();

			OgreNewt::Body* getBody();

			Ogre::Vector3 getPartPosition(void) const;

			void setBreakForce(Ogre::Real breakForce);

			Ogre::Real getBreakForce(void) const;

			void setBreakTorque(Ogre::Real breakTorque);

			Ogre::Real getBreakTorque(void) const;

			void addJointComponent(JointCompPtr jointCompPtr);

			std::vector<JointCompPtr>& getJointComponents(void);

			void setMotionLess(bool motionless);

			bool isMotionLess(void) const;

			Ogre::SceneNode* getSceneNode(void) const;

			void splitPartMoveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex);
		private:
			Ogre::SceneManager* sceneManager;
			Ogre::v1::Entity* entity;
			Ogre::Item* item;
			Ogre::SceneNode* sceneNode;
			OgreNewt::Body* splitPartBody;
			Ogre::Vector3 partPosition;
			Ogre::Real breakForce;
			Ogre::Real breakTorque;
			std::vector<JointCompPtr> jointComponents;
			bool motionless;
		};

		SplitPart* getOrCreatePart(const Ogre::String &meshName);

		void createJoint(SplitPart* part1, SplitPart* part2, const Ogre::Vector3& jointPosition, Ogre::Real breakForce, Ogre::Real breakTorque);
	public:
		static const Ogre::String AttrBreakForce(void) { return "Break Force"; }
		static const Ogre::String AttrBreakTorque(void) { return "Break Torque"; }
		static const Ogre::String AttrDensity(void) { return "Density"; }
		static const Ogre::String AttrMeshMotionless(void) { return "Mesh Motionless"; }
		static const Ogre::String AttrSplitMesh(void) { return "Split Mesh"; }
	private:
		Ogre::DataStreamPtr execSplitMesh(void);
	protected:
		Variant* breakForce;
		Variant* breakTorque;
		Variant* density;
		Variant* motionless;
		Variant* splitMeshButton;
		Ogre::String meshSplitConfigFile;
		std::unordered_map<Ogre::String, SplitPart*> parts;
		unsigned int jointCount;
		unsigned int partsCount;
		std::set<JointCompPtr> delayedDeleteJointComponentList;
		
		bool firstTimeBroken;

	};

}; //namespace end

#endif