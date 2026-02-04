#ifndef MORPH_ANIMATION_COMPONENT_H
#define MORPH_ANIMATION_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "fparser.hh"

namespace Ogre
{
	namespace v1
	{
		class Entity;
		class Mesh;
		class SubEntity;
		class Skeleton;
		class Pose;
		class VertexPoseKeyFrame;
	}
}

namespace NOWA
{
	/**
	  * @brief		Morph animation component for controlling pose/blend shape animations via mathematical functions.
	  *				Works with Ogre::v1::Entity and supports creating, querying, and animating poses.
	  *				Can optionally be associated with a specific bone.
	  */
	class EXPORTED MorphAnimationComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<MorphAnimationComponent> MorphAnimationComponentPtr;
	public:

		MorphAnimationComponent();

		virtual ~MorphAnimationComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		*/
		virtual void initialise() override;

		/**
		* @see		Ogre::Plugin::shutdown
		*/
		virtual void shutdown() override;

		/**
		* @see		Ogre::Plugin::uninstall
		*/
		virtual void uninstall() override;

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;

		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

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

		/**
		* @see		GameObjectComponent::update
		*/
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
		 * @brief Sets whether the morph animation is activated
		 * @param activated True to activate, false to deactivate
		 */
		void setActivated(bool activated);

		/**
		 * @brief Gets whether the morph animation is activated
		 * @return True if activated
		 */
		bool getActivated(void) const;

		/**
		 * @brief Sets the bone name to associate with (optional)
		 * @param boneName The bone name, empty for no bone association
		 */
		void setBoneName(const Ogre::String& boneName);

		/**
		 * @brief Gets the associated bone name
		 * @return The bone name
		 */
		Ogre::String getBoneName(void) const;

		/**
		 * @brief Sets the number of pose animations to control
		 * @param poseAnimationCount The number of pose animations
		 */
		void setPoseAnimationCount(unsigned int poseAnimationCount);

		/**
		 * @brief Gets the number of pose animations
		 * @return The pose animation count
		 */
		unsigned int getPoseAnimationCount(void) const;

		/**
		 * @brief Sets the pose name at the given index
		 * @param index The pose index
		 * @param poseName The pose name to set
		 */
		void setPoseName(unsigned int index, const Ogre::String& poseName);

		/**
		 * @brief Gets the pose name at the given index
		 * @param index The pose index
		 * @return The pose name
		 */
		Ogre::String getPoseName(unsigned int index) const;

		/**
		 * @brief Sets the weight function for the pose at the given index
		 * @param index The pose index
		 * @param weightFunction The mathematical function string (e.g., "(sin(t) + 1) / 2")
		 */
		void setWeightFunction(unsigned int index, const Ogre::String& weightFunction);

		/**
		 * @brief Gets the weight function for the pose at the given index
		 * @param index The pose index
		 * @return The weight function string
		 */
		Ogre::String getWeightFunction(unsigned int index) const;

		/**
		 * @brief Sets the speed multiplier for the pose at the given index
		 * @param index The pose index
		 * @param speed The speed multiplier
		 */
		void setSpeed(unsigned int index, Ogre::Real speed);

		/**
		 * @brief Gets the speed multiplier for the pose at the given index
		 * @param index The pose index
		 * @return The speed multiplier
		 */
		Ogre::Real getSpeed(unsigned int index) const;

		/**
		 * @brief Sets the time offset for the pose at the given index
		 * @param index The pose index
		 * @param timeOffset The time offset value
		 */
		void setTimeOffset(unsigned int index, Ogre::Real timeOffset);

		/**
		 * @brief Gets the time offset for the pose at the given index
		 * @param index The pose index
		 * @return The time offset
		 */
		Ogre::Real getTimeOffset(unsigned int index) const;

		/**
		 * @brief Gets all available pose names from the mesh
		 * @return Vector of pose name strings
		 */
		std::vector<Ogre::String> getAvailablePoseNames(void) const;

		/**
		 * @brief Gets all available bone names from the skeleton
		 * @return Vector of bone name strings
		 */
		std::vector<Ogre::String> getAvailableBoneNames(void) const;

		/**
		 * @brief Manually sets the weight for a pose by name (via animation state)
		 * @param poseName The pose name
		 * @param weight The weight value (typically 0.0 to 1.0)
		 */
		void setPoseWeight(const Ogre::String& poseName, Ogre::Real weight);

		/**
		 * @brief Gets the current weight for a pose by name
		 * @param poseName The pose name
		 * @return The current weight
		 */
		Ogre::Real getPoseWeight(const Ogre::String& poseName) const;

		/**
		 * @brief Resets the time accumulator to zero
		 */
		void resetTime(void);

		/**
		 * @brief Gets the current time accumulator value
		 * @return The accumulated time
		 */
		Ogre::Real getAccumulatedTime(void) const;

		/**
		 * @brief Creates a new pose on the mesh
		 * @param poseName The name for the new pose
		 * @param target The target: 0 = shared geometry, 1+ = submesh index
		 * @return True if pose was created successfully
		 */
		bool createPose(const Ogre::String& poseName, unsigned short target = 0);

		/**
		 * @brief Adds a vertex offset to an existing pose
		 * @param poseName The pose name
		 * @param vertexIndex The vertex index to modify
		 * @param offset The position offset vector
		 * @param normalOffset Optional normal offset vector (can be Vector3::ZERO)
		 * @return True if vertex was added successfully
		 */
		bool addPoseVertex(const Ogre::String& poseName, size_t vertexIndex,
			const Ogre::Vector3& offset, const Ogre::Vector3& normalOffset = Ogre::Vector3::ZERO);

		/**
		 * @brief Removes a pose from the mesh
		 * @param poseName The pose name to remove
		 * @return True if pose was removed successfully
		 */
		bool removePose(const Ogre::String& poseName);

		/**
		 * @brief Gets the number of poses on the mesh
		 * @return The pose count
		 */
		size_t getMeshPoseCount(void) const;

		/**
		 * @brief Gets a pose by index
		 * @param index The pose index
		 * @return The pose pointer or nullptr
		 */
		Ogre::v1::Pose* getPoseByIndex(size_t index) const;

		/**
		 * @brief Gets a pose by name
		 * @param poseName The pose name
		 * @return The pose pointer or nullptr
		 */
		Ogre::v1::Pose* getPoseByName(const Ogre::String& poseName) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MorphAnimationComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "MorphAnimationComponent";
		}

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Controls morph/pose animations on v1 meshes with blend shapes. "
				"Use mathematical functions to animate pose weights over time. "
				"Can also create new poses at runtime. "
				"Requirements: v1::Entity with mesh.";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrActivated(void)
		{
			return "Activated";
		}
		static const Ogre::String AttrBoneName(void)
		{
			return "Bone Name";
		}
		static const Ogre::String AttrPoseAnimationCount(void)
		{
			return "Pose Animation Count";
		}
		static const Ogre::String AttrPoseName(void)
		{
			return "Pose Name ";
		}
		static const Ogre::String AttrWeightFunction(void)
		{
			return "Weight Function ";
		}
		static const Ogre::String AttrSpeed(void)
		{
			return "Speed ";
		}
		static const Ogre::String AttrTimeOffset(void)
		{
			return "Time Offset ";
		}

	private:
		/**
		 * @brief Parses all mathematical functions for pose weights
		 * @return True if all functions parsed successfully
		 */
		bool parseMathematicalFunctions(void);

		/**
		 * @brief Initializes the pose data from the mesh
		 */
		void initializePoseData(void);

		/**
		 * @brief Initializes the bone data from the skeleton
		 */
		void initializeBoneData(void);

		/**
		 * @brief Creates the variant attributes for pose animations
		 * @param prevIndex The previous count (for dynamic attribute creation)
		 */
		void createPoseAnimationAttributes(unsigned int prevIndex);

		/**
		 * @brief Removes pose animation attributes beyond the current count
		 * @param count The target count
		 */
		void removePoseAnimationAttributes(unsigned int count);

		/**
		 * @brief Sets up the manual pose animation for blending
		 */
		void setupPoseAnimation(void);

		/**
		 * @brief Updates pose weights in the vertex pose key frame
		 */
		void updatePoseKeyFrame(void);

	private:
		Ogre::String name;

		Variant* activated;
		Variant* boneName;
		Variant* poseAnimationCount;

		std::vector<Variant*> poseNames;
		std::vector<Variant*> weightFunctions;
		std::vector<Variant*> speeds;
		std::vector<Variant*> timeOffsets;

		std::vector<FunctionParser> functionParsers;
		Ogre::Real accumulator;

		Ogre::v1::Entity* entity;
		Ogre::v1::Mesh* mesh;
		Ogre::v1::Skeleton* skeleton;

		std::vector<Ogre::String> availablePoseNames;
		std::vector<Ogre::String> availableBoneNames;

		// For manual pose animation
		Ogre::v1::AnimationState* poseAnimationState;
		Ogre::v1::VertexPoseKeyFrame* poseKeyFrame;

		// Store current pose weights
		std::map<Ogre::String, Ogre::Real> currentPoseWeights;
	};

	// Lua helper functions
	MorphAnimationComponent* getMorphAnimationComponent(GameObject* gameObject, unsigned int occurrenceIndex);
	MorphAnimationComponent* getMorphAnimationComponent(GameObject* gameObject);
	MorphAnimationComponent* getMorphAnimationComponentFromName(GameObject* gameObject, const Ogre::String& name);

}; // namespace end

#endif