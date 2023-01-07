#ifndef PHYSICS_RAG_DOLL_COMPONENT_H
#define PHYSICS_RAG_DOLL_COMPONENT_H

#include "PhysicsActiveComponent.h"
#include <tinyxml.h>
#include "JointComponents.h"

namespace NOWA
{
	typedef boost::shared_ptr<JointComponent> JointCompPtr;

	class EXPORTED PhysicsRagDollComponent : public PhysicsActiveComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsRagDollComponent> PhysicsRagDollCompPtr;
	public:

		enum RDState
		{
			INACTIVE = 0,
			ANIMATION,
			RAGDOLLING,
			PARTIAL_RAGDOLLING
		};

		PhysicsRagDollComponent();

		virtual ~PhysicsRagDollComponent();

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

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual Ogre::String getParentParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		virtual bool isMovable(void) const override
		{
			return true;
		}

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsRagDollComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsRagDollComponent";
		}

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: This component is used to create physics rag doll. The ragdoll details are specified in a XML file. "
				"This component has also several states, so that it can also behave like an usual physics active component. "
				"Even a partial rag doll can be created e.g. just using the right arm of the player as rag doll."
				"Requirements: A game object with mesh and animation (*.skeleton) file.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::showDebugData
		*/
		virtual void showDebugData(void) override;
		
		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;

		void inheritVelOmega(Ogre::Vector3 vel, Ogre::Vector3 omega);

		/**
		 * @brief		Sets the rag doll state.
		 * @param[in]	state	The state to set. Possible values are:
		 *				'Inactive': No rag dolling, this component behaves like a physics active component with just one collision body.
		 * 				'Ragdolling': Changes the state to rag doll mode, using the rag doll configuration file
		 * 				'Animation': The collision bodies transform is set to the animated bones
		 */
		void setState(const Ogre::String& state);

		/**
		 * @brief		Gets the rag doll state.
		 * @return		state	The state to get. Possible values are:
		 *				'Inactive': No rag dolling, this component behaves like a physics active component with just one collision body.
		 * 				'Ragdolling': Changes the state to rag doll mode, using the rag doll configuration file
		 * 				'Animation': The collision bodies transform is set to the animated bones
		 */
		Ogre::String getState(void) const;

		/**
		 *  Just For now return NULL
		 */
		virtual OgreNewt::Body* getBody(void) const override;
 
	  /**
		* @brief		Sets the velocity for this game object.
		* @param[in]	velocity	The velocity to set
		*/
		virtual void setVelocity(const Ogre::Vector3& velocity) override;

		/**
		 * @brief		Gets the currently acting velocity.
		 * @return		velocity	The velocity to get
		 */
		virtual Ogre::Vector3 getVelocity(void) const override;

		/**
		 * @brief		Sets the position for this game object.
		 * @param[in]	position	The position to set
		 */
		virtual void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);

		/**
		 * @brief		Sets the position for this game object.
		 * @param[in]	position	The position to set
		 */
		virtual void setPosition(const Ogre::Vector3& position) override;

		/**
		 * @brief		Translates the game object.
		 * @param[in]	relativePosition	The relative position for translation to set
		 */
		virtual void translate(const Ogre::Vector3& relativePosition) override;

		/**
		 * @brief		Gets the game object position.
		 * @return		position	The position to get
		 */
		virtual Ogre::Vector3 getPosition(void) const override;

		/**
		 * @brief		Sets orientation for the game object.
		 * @param[in]	orientation	The orientation to set
		 */
		virtual void setOrientation(const Ogre::Quaternion& orientation) override;

		/**
		 * @brief		Rotates the game object.
		 * @param[in]	relativeRotation	The relative rotation to set
		 */
		virtual void rotate(const Ogre::Quaternion& relativeRotation) override;

		/**
		 * @brief		Gets the game object orientation.
		 * @return		orientation	The orientation to get
		 */
		virtual Ogre::Quaternion getOrientation(void) const override;
		
		void setBoneRotation(const Ogre::String& boneName, const Ogre::Vector3& axis, Ogre::Real degree);

		/**
		 * @brief		Sets all bones to initial transform state.
		 */
		void setInitialState(void);

		void setAnimationEnabled(bool animationEnabled);

		bool isAnimationEnabled(void) const;

		/**
		 * @brief		Sets the bone configuration file.
		 * @param[in]	boneConfigFile	The bone configuration file name for rag dolling to set
		 * @note		The file must be placed in the same folder as the mesh and skeleton file.
		 */
		void setBoneConfigFile(const Ogre::String& boneConfigFile);

		/**
		 * @brief		Gets the bone configuration file name.
		 * @return		boneConfigFile	The bone configuration file name to get
		 */
		Ogre::String getBoneConfigFile(void) const;

		/***************************************Inner bone class*******************************************************/

		/*
		 * @class Main bone class, represents a single bone (rigid body) in the ragdoll.  is linked to a bone in the model too.
		 */
		class EXPORTED RagBone
		{
		public:
			enum BoneShape
			{
				BS_BOX, BS_ELLIPSOID, BS_CYLINDER, BS_CAPSULE, BS_CONE, BS_CONVEXHULL, BS_NULLHULL
			};

			RagBone(const Ogre::String& name, PhysicsRagDollComponent* physicsRagDollComponent, RagBone* parentRagBone, Ogre::v1::OldBone* ogreBone, Ogre::v1::MeshPtr mesh,
				const Ogre::Vector3& pose, RagBone::BoneShape shape, Ogre::Vector3& size, Ogre::Real mass, bool partial, const Ogre::Vector3& offset);

			~RagBone();

			void update(Ogre::Real dt, bool notSimulating = false);

			void setOrientation(const Ogre::Quaternion& orientation);

			void rotate(const Ogre::Quaternion& rotation);

			void setInitialState(void);

			const Ogre::String& getName(void) const;

			// newton body.
			OgreNewt::Body* getBody();

			// get ogre bone
			Ogre::v1::OldBone* getOgreBone(void) const;

			Ogre::SceneNode* getSceneNode(void) const;

			// get parent.
			RagBone* getParentRagBone(void) const;

			Ogre::Vector3 getInitialBonePosition(void);

			Ogre::Quaternion getInitialBoneOrientation(void);

			Ogre::Vector3 getInitialBodyPosition(void);

			Ogre::Quaternion getInitialBodyOrientation(void);

			Ogre::Vector3 getWorldPosition(void);

			Ogre::Quaternion getWorldOrientation(void);

			Ogre::Vector3 getPosition(void) const;

			Ogre::Quaternion getOrientation(void) const;

			void setInitialPose(const Ogre::Vector3& initialPose);

			void attachToNode(void);

			void detachFromNode(void);

			void deleteRagBone(void);

			// get doll.
			PhysicsRagDollComponent* getPhysicsRagDollComponent(void) const;

			const Ogre::Vector3 getRagPose(void);

			void applyPose(const Ogre::Vector3& pose);

			JointCompPtr createJointComponent(const Ogre::String& type);

			JointCompPtr getJointComponent(void) const;

			Ogre::Vector3 getBodySize(void) const;

			void applyRequiredForceForVelocity(const Ogre::Vector3& velocity);

			// Ogre::Vector3 getPosCorrection(void) const;
		private:
			void moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex);
		private:

			// pointer to the doll to which this bone belongs.
			PhysicsRagDollComponent* physicsRagDollComponent;
			Ogre::String name;
			RagBone* parentRagBone;
			Ogre::SceneNode* sceneNode;
			OgreNewt::Body* body;
			Ogre::v1::OldBone* ogreBone;
			OgreNewt::UpVector* upVector;

			// limits.
			Ogre::Real limit1;
			Ogre::Real limit2;

			JointCompPtr jointCompPtr;

			Ogre::Vector3 pose;

			Ogre::Quaternion initialBoneOrientation;
			Ogre::Vector3 initialBonePosition;
			Ogre::Quaternion initialBodyOrientation;
			Ogre::Vector3 initialBodyPosition;
			Ogre::Vector3 initialPose;
			Ogre::Vector3 bodySize;
			Ogre::Vector3 forceForVelocity;
			bool canAddForceForVelocity;
			bool partial;
		};
	public:
		struct RagData
		{
			Ogre::String ragBoneName;
			RagBone* ragBone;
		};

		// std::map<Ogre::String, RagBone*> getRagBones(void) const;
		const std::vector<RagData>& getRagDataList(void);

		RagBone* getRagBone(const Ogre::String& name);

	protected:
		enum JointType
		{
			JT_BALLSOCKET,
			JT_HINGE,
			JT_DOUBLE_HINGE,
			JT_HINGE_ACTUATOR
		};

		bool createRagDoll(const Ogre::String& boneName);

		// recursive function for creating bones.
		bool createRagDollFromXML(TiXmlElement* rootXmlElement, const Ogre::String& boneName);

		// add a joint between 2 bones.
		void joinBones(PhysicsRagDollComponent::JointType type, RagBone* childRagBone, RagBone* parentRagBone, const Ogre::Vector3& pin,
			const Ogre::Degree& minTwistAngle, const Ogre::Degree& maxTwistAngle, const Ogre::Degree& minTwistAngle2, const Ogre::Degree& maxTwistAngle2, 
			const Ogre::Degree& maxConeAngle, Ogre::Real friction, bool useSpring, const Ogre::Vector3& offset);
		
		void internalApplyState(void);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrBonesConfigFile(void) { return "Config. File"; }
		static const Ogre::String AttrState(void) { return "State"; }
	private:
		void applyModelStateToRagdoll(void);

		void applyRagdollStateToModel(void);

		void startRagdolling(void);

		void endRagdolling(void);

		void internalShowDebugData(bool activate);
	protected:
		Variant* activated;
		Variant* boneConfigFile;
		Variant* state;
		
		RDState rdState;
		RDState rdOldState;

		std::vector<RagData> ragDataList;

		JointCompPtr rootJointCompPtr;

		// mesh for this character.
		Ogre::v1::MeshPtr mesh;

		// skeleton instance.
		// Note: V2 Skeleton/Item not ported yet, since its totally different from v1, e.g. there is no animation state etc.
		Ogre::v1::OldSkeletonInstance* skeleton;

		bool animationEnabled;

		Ogre::Vector3 ragdollPositionOffset;
		Ogre::Quaternion ragdollOrientationOffset;
		bool isSimulating;
		Ogre::String partialRagdollBoneName;
		Ogre::String oldPartialRagdollBoneName;

		std::map<Ogre::v1::OldBone*, std::pair<Ogre::v1::OldBone*, Ogre::Vector3>> boneCorrectionMap;
	};

}; //namespace end

#endif