//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Modified and adapted by Lukas Kalinowski for NOWA-Engine

#ifndef IK_EFFECTOR_COMPONENT_H
#define IK_EFFECTOR_COMPONENT_H

#include "GameObjectComponent.h"

struct ik_solver_t;
struct ik_node_t;
using ik_node_t = struct ik_node_t;

namespace NOWA
{
	class EXPORTED IKEffectorComponent : public GameObjectComponent
	{
	public:
		friend class IKSolverComponent;
		
		typedef boost::shared_ptr<NOWA::IKEffectorComponent> IKEffectorCompPtr;
	public:
		enum Option
		{

			/**
			 * If you set the effector weight (see SetWeight()) to a value in
			 * between 0 and 1, the default behaviour is to linearly interpolate
			 * the effector's target position. If the solved tree and the initial
			 * tree are far apart, this can look very strange, especially if you
			 * are controlling limbs on a character that are designed to rotation.
			 * Enabling this causes a rotational based interpolation (nlerp) around
			 * the chain's base node and makes transitions look much more natural.
			 */
			WEIGHT_NLERP = 0x01,

			/**
			 * By default the end effector node will retain its global orientation,
			 * even after solving. By enabling this feature, the node will instead
			 * "rotate with" its parent node.
			 */
			INHERIT_PARENT_ROTATION = 0x02
		};

		IKEffectorComponent();

		virtual ~IKEffectorComponent();

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
		virtual void onRemoveComponent(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("IKEffectorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "IKEffectorComponent";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override;

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
		
		bool isActivated(void) const;
	   
	// Attention: BoneComponent is required, to get a specifig bone like NodeComponent
		
		/**
		 * @brief The position of the target node provides the target position of
		 * the effector node.
		 *
		 * The IK chain is solved such that the node to which this component is
		 * attached to will try to move to the position of the target node.
		 * @param targetNode A scene node that acts as a target. Specifying NULL
		 * will erase the target and cause the solver to ignore this chain.
		 * @note You will get very strange behaviour if you specify a target node
		 * that is part the IK chain being solved for (circular dependency). Don't
		 * do that.
		 */
		void setTargetBoneName(const Ogre::String& targetBoneName);
		
		Ogre::String getTargetBoneName(void) const;

		/// Sets the number of segments that will be affected. 0 Means all nodes between this effector and the next IKSolver.
		void setChainLength(unsigned int chainLength);
		
		 /// Returns the number of segments that will be affected by this effector. 0 Means all nodes between this effector and the next IKSolver.
		unsigned int getChainLength(void) const;
		
		void setWeightNLerp(bool weightNLerp);
		
		bool getWeightNLerp(void) const;

		void setInheritParentRotation(bool inheritParentRotation);
		
		bool getInheritParentRotation(void) const;
		
		/**
		 * @brief Sets how much influence the effector has on the solution.
		 *
		 * You can use this value to smoothly transition between a solved pose and
		 * an initial pose  For instance, lifting a foot off of the ground or
		 * letting go of an object.
		 */
		void setWeight(Ogre::Real weight);
		
		 /// How strongly the effector affects the solution.
		Ogre::Real getWeight(void) const;

		/**
		 * @brief Sets how much influence the target rotation should have on the
		 * solution. A value of 1 means to match the target rotation exactly, if
		 * possible. A value of 0 means to not match it at all.
		 * @note The solver must have target rotation enabled for this to have
		 * any effect. See IKSolver::Feature::TARGET_ROTATIONS.
		 */
		void setRotationWeight(Ogre::Real rotationWeight);
		
		Ogre::Real getRotationWeight(void);

		/**
		 * @brief A factor with which to control the target rotation influence of
		 * the next segments down the chain.
		 *
		 * For example, if this is set to 0.5 and the rotation weight is set to
		 * 1.0, then the first segment will match the target rotation exactly, the
		 * next segment will match it only 50%, the next segment 25%, the next
		 * 12.5%, etc. This parameter makes long chains look more natural when
		 * matching a target rotation.
		 */
		void setRotationDecay(Ogre::Real rotationDecay);
		
		/// Retrieves the rotation decay factor. See SetRotationDecay() for info.
		Ogre::Real getRotationDecay(void) const;
		
		void setStiffness(Ogre::Real stiffness);
		
		Ogre::Real getStiffness(void) const;
 
		Ogre::Real getStretchness(void) const;
		
		void setStretchness(Ogre::Real stretchness);
		
		Ogre::Vector2 getLengthConstraints() const;
		
		void setLengthConstraints(const Ogre::Vector2& lengthConstraints);
		
		Ogre::v1::OldBone* getTargetBone(void) const;
	private:
		/// Intended to be used only by IKSolverComponent
		void setIKSolverComponent(boost::weak_ptr<IKSolverComponent> weakIKSolverComponent);
		/// Sets the current target position. If the effector has a target node then this will have no effect.
		void setTargetPosition(const Ogre::Vector3& targetPosition);
		/// Returns the current target position in world space.
		Ogre::Vector3 getTargetPosition(void) const;
		/// Sets the current target rotation. If the effector has a target node then this will have no effect.
		void setTargetRotation(const Ogre::Quaternion& targetRotation);
		/// Gets the current target rotation in world space.
		Ogre::Quaternion getTargetRotation(void) const;
		/// Intended to be used only by IKSolver
		void setIKEffectorNode(ik_node_t* effectorNode);
		/// Intended to be used by IKSolver. Copies the positions/rotations of the target node into the effector
		void updateTargetBonePosition(void);
	public:
		// static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetBoneName(void) { return "Target Bone Name"; }
		static const Ogre::String AttrChainLength(void) { return "Chain Length"; }
		static const Ogre::String AttrWeight(void) { return "Weight"; }
		static const Ogre::String AttrRotationWeight(void) { return "Rotation Weight"; }
		static const Ogre::String AttrRotationDecay(void) { return "Rotation Decay"; }
		static const Ogre::String AttrWeightNLerp(void) { return "Weight NLerp"; }
		static const Ogre::String AttrInheritParentRotation(void) { return "Inherit Parent Rotation"; }
		static const Ogre::String AttrStiffness(void) { return "Stiffness"; }
		static const Ogre::String AttrStrechness(void) { return "Stretchness"; }
		static const Ogre::String AttrLengthConstraints(void) { return "Length Constraints"; }
	private:
		Ogre::v1::OldBone* targetBone;
		Ogre::Vector3 targetPosition;
		Ogre::Quaternion targetRotation;
		ik_node_t* effectorNode;
		unsigned options;
		boost::shared_ptr<IKSolverComponent> ikSolverCompPtr;

		Variant* targetBoneName;
		Variant* chainLength;
		Variant* weight;
		Variant* rotationWeight;
		Variant* rotationDecay;
		Variant* weightNLerp;
		Variant* inheritParentRotation;
		Variant* stiffness;
		Variant* stretchness;
		Variant* lengthConstraints; // Vector2
	};

}; //namespace end

#endif
