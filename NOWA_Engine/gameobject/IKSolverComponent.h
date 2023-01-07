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

#ifndef IK_SOLVER_COMPONENT_H
#define IK_SOLVER_COMPONENT_H

#include "GameObjectComponent.h"

struct ik_solver_t;
struct ik_node_t;

namespace NOWA
{
	class IKEffectorComponent;
	
	class EXPORTED IKSolverComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::IKSolverComponent> IKSolverCompPtr;
	public:

	enum Algorithm
    {
        ONE_BONE = 0,
        TWO_BONE,
        FABRIK
        /* not implemented yet
        MSD,
        JACOBIAN_INVERSE,
        JACOBIAN_TRANSPOSE*/
    };

    enum Option
    {
        /*!
         * @brief Should be enabled if your model uses skinning or if you are
         * generally interested in correct joint rotations. Has a minor
         * performance impact.
         *
         * When enabled, final joint rotations are calculated as a post
         * processing step. If you are using IK on a model with skinning, you will
         * want to enable this or it will look wrong. If you disable this, then
         * you will get a slight performance boost (less calculations are required)
         * but only the node positions are updated. This can be useful for scene
         * IK (perhaps a chain of platforms, where each platform should retain its
         * initial world rotation?)
         */
        JOINT_ROTATIONS = 0x01,

        /*!
         * @brief When enabled, the effector will try to match the target's
         * rotation as well as the effectors position. When disabled, the target
         * node will reach the effector with any rotation necessary.
         *
         * If the target position goes out of range of the effector then the
         * rotation will no longer be matched. The chain will try to reach out to
         * reach the target position, even if it means rotating towards it.
         */
        TARGET_ROTATIONS = 0x02,

        /*!
         * When the solver is first initialized, it will copy the positions
         * and rotations of the current Urho3D scene graph into an internal
         * structure. This is referred to as the "original pose" and will by
         * default never change for the duration of the solver's life cycle.
         * When the solver is destroyed, the original pose is applied back to
         * Urho3D's scene graph so the nodes are restored to whatever they were
         * before the solver was created.
         *
         * By enabling UPDATE_ORIGINAL_POSE, the original pose will be updated
         * right before solving to reflect the current Urho3D scene graph. As
         * a consequence, there will no longer be an original pose to restore
         * when the solver is destroyed.
         *
         * When disabled, the original pose will remain unmodified. The original
         * pose is set when the solver is first created. You can manually update the
         * original pose at any time by calling UpdateInitialPose().
         */
        UPDATE_ORIGINAL_POSE = 0x04,

        /*!
         * @brief Should be enabled if you are using IK on an animated model,
         * along with disabling USE_ORIGINAL_POSE.
         *
         * The "active pose" has two purposes: The solver uses it as the
         * initial tree to derive a solution from, and at the same time uses it
         * to store the solution into. Thus, the typical solving process is:
         *   1) The active pose needs to be updated to reflect a preferred
         *      initial condition (such as the current frame of animation)
         *   2) Call Solve()
         *   3) The active pose now holds the solution, so it must be applied
         *      back to the Urho3D scene graph.
         *
         * When enabled, the active pose is updated right before solving to
         * reflect the current state of the Urho3D scene graph.
         *
         * When disabled, the active pose will simply remain as it was since
         * the last time Solve() was called.
         *
         * @note This option conflicts with USE_ORIGINAL_POSE. Make sure to
         * disable USE_ORIGINAL_POSE if you enable this feature.
         */
        UPDATE_ACTIVE_POSE = 0x08,

        /*!
         * @brief Choose between using the original pose or the active pose as
         * a basis for a solution.
         *
         * When enabled, the solver will copy the original pose
         * (see UPDATE_ORIGINAL_POSE) into the active pose before solving (and
         * thus use the original pose as a basis for a solution).
         *
         * @note This option conflicts with UPDATE_ACTIVE_POSE. If you enable
         * this feature, make sure to disable UPDATE_ACTIVE_POSE.
         *
         * If both UPDATE_ACTIVE_POSE and USE_ORIGINAL_POSE are disabled, then
         * the solver will use the previously solved tree as a basis for the new
         * calculation. The result is a more "continuous" solution that unfolds
         * over time. This can be useful if you want to simulate chains or
         * something similar.
         */
        USE_ORIGINAL_POSE = 0x10,

        /*!
         * Due to the somewhat unfortunate performance impacts, the solver
         * does not enable constraints by default. Enabling constraints causes
         * the solver's tree to be written to and from Urho3D's scene graph every
         * iteration, while calling ApplyConstraints(). Disabling constraints means
         * ApplyConstraints() is never called.
         */
        CONSTRAINTS = 0x20,

        /*!
         * Mostly exists because of the editor. When enabled, the solver
         * will be invoked automatically for you. If you need to do additional
         * calculations before being able to set the effector target data, you will
         * want to disable this and call Solve() manually.
         */
        AUTO_SOLVE = 0x40
    };
		
		IKSolverComponent();

		virtual ~IKSolverComponent();

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
			return NOWA::getIdFromName("IKSolverComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "IKSolverComponent";
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
		
		/**
		 * @brief For the effectors to work, an IKSolver needs to be attached to one of
		 * the parent bones. Typically, the solver should be placed as close as
		 * possible to the effectors for optimal performance. E.g. when solving
		 * the legs only, the solver can be placed at the spine.
		 */
		void setTargetBoneName(const Ogre::String& targetBoneName);
		
		Ogre::String getTargetBoneName(void) const;

		/**
		 * @brief Selects the solver algorithm. Default is FABRIK. Note that this
		 * may not be the most efficient algorithm available. The specialized
		 * solvers will be a lot faster.
		 *
		 * The currently supported solvers are listed below.
		 *   + **FABRIK**: This is a fairly new and highly efficient inverse
		 *     kinematic solving algorithm. It requires the least iterations to
		 *     reach its goal, it does not suffer from singularities (nearly no
		 *     violent snapping about), and it always converges.
		 *   + **2 Bone**: A specialized solver optimized for 2 bone problems (such
		 *     as a human leg)
		 *   + **1 Bone**: A specialized solver optimized for 1 bone problems (such
		 *     as a look-at target, e.g. eyes or a head)
		 */
		void setAlgorithm(const Ogre::String& algorithm);
		
		Ogre::String getAlgorithm(void) const;
		
		void setJointRotations(bool jointRotations);
		
		bool getJointRotations(void) const;
		
		void setTargetRotations(bool targetRotations);
		
		bool getTargetRotations(void) const;
		
		void setUpdateOriginalPose(bool updateOriginalPose);
		
		bool getUpdateOriginalPose(void) const;
		
		void setUpdateActivePose(bool updateActivePose);
		
		bool getUpdateActivePose(void) const;
		
		void setUseOriginalPose(bool useOriginalPose);
		
		bool getUseOriginalPose(void) const;
		
		void setConstraints(bool constraints);
		
		bool getConstraints(void) const;
		
		void setAutoSolve(bool autoSolve);
		
		bool getAutoSolve(void) const;
		
		/**
		 * @brief Sets the maximum number of iterations the solver is allowed to
		 * perform before applying the result.
		 *
		 * Depending on the algorithm, you may want higher or lower values.
		 * FABRIK looks decent after only 10 iterations, whereas Jacobian based
		 * methods often require more than a 100.
		 *
		 * The default value is 20.
		 *
		 * @note Most algorithms have a convergence criteria at which the solver
		 * will stop iterating, so most of the time the maximum number of
		 * iterations isn't even reached.
		 *
		 * @param[in] iterations Number of iterations. Must be greater than 0. Higher
		 * values yield more accurate results, but at the cost of performance.
		 */
		void setMaxIterations(unsigned int maxIterations);
		
		unsigned int getMaxIterations(void) const;
		
		/**
		 * @brief Sets the distance at which the effector is "close enough" to the
		 * target node, at which point the algorithm will stop iterating.
		 *
		 * @param tolerance The distance to set. Smaller values yield more accurate
		 * results, but at the cost of more iterations. Generally you'll want to
		 * specify a number that is about 1/100th to 1/1000th of the total size of
		 * the IK chain, e.g. if your human character has a leg that is 1 Urho3D
		 * unit long, a good starting tolerance would be 0.01.
		 */
		void setTolerance(Ogre::Real tolerance);
		
		Ogre::Real getTolerance(void) const;
		
		Ogre::v1::OldBone* getTargetBone(void) const;
		
			ik_solver_t*  getSolver(void) const;
		
		/*!
		 * @brief Updates the solver's internal data structures, which is required
		 * whenever the tree is modified in any way (e.g. adding or removing nodes,
		 * adding or removing effectors, etc.).
		 * @note This gets called  automatically for you in Solve().
		 */
		void rebuildChainTrees(void);

		/*!
		 * @brief Unusual, but if you have a tree with translational motions such
		 * that the distances between nodes changes (perhaps a slider?), you can
		 * call this to recalculate the segment lengths after assigning new
		 * positions to the nodes.
		 * @note This function gets called by RebuildData() and by extension in
		 * Solve().
		 */
		void recalculateSegmentLengths(void);

		/*!
		 * @brief Skinned models require joint rotations to be calculated so
		 * skinning works correctly. This is automatically enabled by default with
		 * the feature flag JOINT_ROTATIONS.
		 */
		void calculateJointRotations(void);

		/*!
		 * @brief Invokes the solver. The solution is applied back to the scene
		 * graph automatically.
		 * @note By default this is called automatically for you if the feature
		 * flag AUTO_SOLVE is set. For more complex IK problems you can disable
		 * that flag and call Solve() in response to E_SCENEDRAWABLEUPDATEFINISHED.
		 * This is right after the animations have been applied.
		 */
		void solve(void);

		/*!
		 * Copies the original pose into the scene graph. This will reset the pose
		 * to whatever state it had when the IKSolver component was first created,
		 * or, if the original pose was updated since then (for example if
		 * Feature::UPDATE_ORIGINAL_POSE is set), will reset it to that state.
		 */
		void applyOriginalPoseToScene(void);

		/*!
		 * Copies the current scene graph data into the solvers original pose. You
		 * generally won't need to call this, because it gets called for you
		 * automatically if Feature::UPDATE_ORIGINAL_POSE is set.
		 */
		void applySceneToOriginalPose(void);

		/*!
		 * Copies the solvers current active pose into the scene graph. You
		 * generally won't need to call this because it gets called for you
		 * automatically in Solve(). This is used to apply the solution back to the
		 * scene graph.
		 */
		void applyActivePoseToScene(void);

		/*!
		 * Copies the current scene graph data into the solvers active pose. You
		 * generally won't need to call this because it gets called for you
		 * automatically if Feature::UPDATE_ACTIVE_POSE is set.
		 */
		void applySceneToActivePose(void);

		/*!
		 * Copies the solvers original pose into the solvers active pose. This is
		 * used in Solve() automatically if Feature::USE_ORIGINAL_POSE is set.
		 */
		void applyOriginalPoseToActivePose(void);
	public:
		// static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetBoneName(void) { return "Target Bone Name"; }
		static const Ogre::String AttrAlgorithm(void) { return "Algorithm"; }
		static const Ogre::String AttrJointRotations(void) { return "Joint Rotations"; }
		static const Ogre::String AttrTargetRotations(void) { return "Target Rotations"; }
		static const Ogre::String AttrUpdateOriginalPose(void) { return "Update Original Pose"; }
		static const Ogre::String AttrUpdateActivePose(void) { return "Update Active Pose"; }
		static const Ogre::String AttrUseOriginalPose(void) { return "Use Original Pose"; }
		static const Ogre::String AttrConstraints(void) { return "Constraints"; }
		static const Ogre::String AttrAutoSolve(void) { return "Auto Solve"; }
		static const Ogre::String AttrMaxIterations(void) { return "Max. Iterations"; }
		static const Ogre::String AttrTolerance(void) { return "Tolerance"; }
	private:
		/// Indicates that the internal structures of the IK library need to be updated. See the documentation of ik_solver_rebuild_chain_trees() for more info on when this happens.
		void markChainsNeedUpdating(void);
		/// Indicates that the tree structure has changed in some way and needs updating (nodes added or removed, components added or removed)
		void markTreeNeedsRebuild(void);
		/// Returns false if calling Solve() would cause the IK library to abort. Urho3D's error handling philosophy is to log an error and continue, not crash.
		bool isSolverTreeValid(void) const;

		/// Creates the ik library node and sets the current rotation/position and user data correctly.
		ik_node_t* createIKNodeFromOgreBone(const Ogre::v1::OldBone* bone);

		/// Destroys the solver's tree
		void destroyTree(void);
		/// Builds the solver's tree to match the scene graph's tree. If a tree already exists, it is first destroyed
		void rebuildTree(void);
		/// Builds a chain of nodes up to the node of the specified effector component.
		bool buildTreeToEffector(IKEffector* effector);
		/*!
		 * Checks if the specified component is 1) attached to a node that is below
		 * the one we are attached to and 2) isn't in the subtree of a child solver.
		 * @note This will return false if the component is attached to our root
		 * node, because in that case the solver can't do anything to it, it's in
		 * the hands of a parent solver (if it exists).
		 */
		// bool ComponentIsInOurSubtree(Component* component) const;
	private:
		Ogre::v1::OldBone* targetBone;
		std::vector<IKEffectorComponent*> ikEffectorComponents;
		ik_solver_t* solver;
		unsigned options;
		bool chainTreesNeedUpdating;
		bool treeNeedsRebuild;
		bool solverTreeValid;

		Variant* targetBoneName;
		Variant* algorithm;
		Variant* jointRotations;
		Variant* targetRotations;
		Variant* updateOriginalPose;
		Variant* updateActivePose;
		Variant* useOriginalPose;
		Variant* constraints;
		Variant* autoSolve;
		Variant* maxIterations;
		Variant* tolerance;
	};

}; //namespace end

#endif
