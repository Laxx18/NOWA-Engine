#ifndef MORPH_ANIMATION_COMPONENT_H
#define MORPH_ANIMATION_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "fparser.hh"

namespace NOWA
{
	/**
	  * @brief		Morph animation component for controlling pose (blend shape) weights via mathematical functions.
	  *				Works with Ogre::Item and Ogre::SubItem pose weights.
	  *				In Ogre-Next V2 poses belong to a sub mesh, hence each entry addresses a sub item index plus a pose index.
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
		 * @brief Sets the number of pose animation entries to control
		 * @param poseAnimationCount The number of pose animation entries
		 */
		void setPoseAnimationCount(unsigned int poseAnimationCount);

		/**
		 * @brief Gets the number of pose animation entries
		 * @return The pose animation entry count
		 */
		unsigned int getPoseAnimationCount(void) const;

		/**
		 * @brief Sets the sub item index the entry at the given position addresses
		 * @param index The entry index
		 * @param subItemIndex The sub item index of the item
		 */
		void setSubItemIndex(unsigned int index, unsigned int subItemIndex);

		/**
		 * @brief Gets the sub item index the entry at the given position addresses
		 * @param index The entry index
		 * @return The sub item index
		 */
		unsigned int getSubItemIndex(unsigned int index) const;

		/**
		 * @brief Sets the pose index the entry at the given position addresses
		 * @param index The entry index
		 * @param poseIndex The pose index inside the addressed sub item
		 */
		void setPoseIndex(unsigned int index, unsigned int poseIndex);

		/**
		 * @brief Gets the pose index the entry at the given position addresses
		 * @param index The entry index
		 * @return The pose index inside the addressed sub item
		 */
		unsigned int getPoseIndex(unsigned int index) const;

		/**
		 * @brief Sets the weight function for the entry at the given index
		 * @param index The entry index
		 * @param weightFunction The mathematical function string, 't' is the time variable (e.g., "(sin(t) + 1) / 2")
		 */
		void setWeightFunction(unsigned int index, const Ogre::String& weightFunction);

		/**
		 * @brief Gets the weight function for the entry at the given index
		 * @param index The entry index
		 * @return The weight function string
		 */
		Ogre::String getWeightFunction(unsigned int index) const;

		/**
		 * @brief Sets the speed multiplier for the entry at the given index
		 * @param index The entry index
		 * @param speed The speed multiplier
		 */
		void setSpeed(unsigned int index, Ogre::Real speed);

		/**
		 * @brief Gets the speed multiplier for the entry at the given index
		 * @param index The entry index
		 * @return The speed multiplier
		 */
		Ogre::Real getSpeed(unsigned int index) const;

		/**
		 * @brief Sets the time offset for the entry at the given index
		 * @param index The entry index
		 * @param timeOffset The time offset value
		 */
		void setTimeOffset(unsigned int index, Ogre::Real timeOffset);

		/**
		 * @brief Gets the time offset for the entry at the given index
		 * @param index The entry index
		 * @return The time offset
		 */
		Ogre::Real getTimeOffset(unsigned int index) const;

		/**
		 * @brief Gets the number of sub items of the item
		 * @return The sub item count
		 */
		unsigned int getSubItemCount(void) const;

		/**
		 * @brief Gets the number of poses of the given sub item
		 * @param subItemIndex The sub item index of the item
		 * @return The pose count
		 */
		unsigned int getPoseCount(unsigned int subItemIndex) const;

		/**
		 * @brief Manually sets the weight of a pose, addressed by sub item index and pose index
		 * @param subItemIndex The sub item index of the item
		 * @param poseIndex The pose index inside that sub item
		 * @param weight The weight value (clamped to 0.0 - 1.0)
		 */
		void setPoseWeight(unsigned int subItemIndex, unsigned int poseIndex, Ogre::Real weight);

		/**
		 * @brief Manually sets the weight of a pose, addressed by sub item index and pose name
		 * @param subItemIndex The sub item index of the item
		 * @param poseName The pose name inside that sub item
		 * @param weight The weight value (clamped to 0.0 - 1.0)
		 */
		void setPoseWeightByName(unsigned int subItemIndex, const Ogre::String& poseName, Ogre::Real weight);

		/**
		 * @brief Gets the last weight this component wrote for the given pose
		 * @param subItemIndex The sub item index of the item
		 * @param poseIndex The pose index inside that sub item
		 * @return The last written weight, 0.0 if this component never wrote that pose
		 */
		Ogre::Real getPoseWeight(unsigned int subItemIndex, unsigned int poseIndex) const;

		/**
		 * @brief Resets the time accumulator to zero
		 */
		void resetTime(void);

		/**
		 * @brief Gets the current time accumulator value
		 * @return The accumulated time
		 */
		Ogre::Real getAccumulatedTime(void) const;

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
			return "Usage: Controls morph (blend shape) pose weights on an Ogre::Item. "
				"Use mathematical functions to animate pose weights over time. "
				"Requirements: A game object with an Ogre::Item whose mesh carries at least one pose. "

				"A pose defines vertex offsets from the original mesh position. For example: "
				"A face mesh could have poses like a smile (moves mouth vertices up) or closed eyes (moves eyelid vertices down). "
				"A chest could have a pose that moves the lid vertices to the open position. "

				"Poses must be baked into the mesh file during export from Blender/Maya/3DS Max. They are called: "
				"Shape Keys in Blender, Blend Shapes in Maya, Morph Targets in 3DS Max. "

				"In Ogre-Next V2 a pose belongs to a sub mesh, not to the whole mesh. Therefore each entry addresses "
				"a sub item index plus a pose index inside that sub item. Use the log output of this component to see "
				"how many poses each sub item actually has after the mesh has been converted to v2. "

				"The weight function uses 't' as the time variable, where t = accumulated time * speed + time offset. "
				"Example: '(sin(t) + 1) / 2' oscillates the weight between 0 and 1.";
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
		static const Ogre::String AttrPoseAnimationCount(void)
		{
			return "Pose Animation Count";
		}
		static const Ogre::String AttrSubItemIndex(void)
		{
			return "Sub Item Index ";
		}
		static const Ogre::String AttrPoseIndex(void)
		{
			return "Pose Index ";
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
		 * @brief Parses all mathematical functions for the pose weights
		 * @return True if all functions parsed successfully
		 */
		bool parseMathematicalFunctions(void);

		/**
		 * @brief Collects the item and the pose count of each of its sub items
		 */
		void initializePoseData(void);

		/**
		 * @brief Creates the variant attributes for the pose animation entries
		 * @param prevIndex The previous count (for dynamic attribute creation)
		 */
		void createPoseAnimationAttributes(unsigned int prevIndex);

		/**
		 * @brief Removes pose animation attributes beyond the current count
		 * @param count The target count
		 */
		void removePoseAnimationAttributes(unsigned int count);

		/**
		 * @brief Evaluates all weight functions for the current accumulator and writes the results to the sub items.
		 *		  Must only be called on the render thread.
		 */
		void applyPoseWeights(void);

		/**
		 * @brief Sets all pose weights this component ever wrote back to zero.
		 *		  Must only be called on the render thread.
		 */
		void clearPoseWeights(void);

		/**
		 * @brief Builds the tracked closure id for this component instance
		 * @return The closure id
		 */
		Ogre::String getClosureId(void) const;

	private:
		Ogre::String name;

		Variant* activated;
		Variant* poseAnimationCount;

		std::vector<Variant*> subItemIndices;
		std::vector<Variant*> poseIndices;
		std::vector<Variant*> weightFunctions;
		std::vector<Variant*> speeds;
		std::vector<Variant*> timeOffsets;

		std::vector<FunctionParser> functionParsers;
		Ogre::Real accumulator;

		Ogre::Item* item;

		// Pose count per sub item index, filled in initializePoseData
		std::vector<unsigned int> poseCountsPerSubItem;

		// Last weight written per (sub item index, pose index), so getPoseWeight
		// does not have to read back from the render thread
		std::map<std::pair<unsigned int, unsigned int>, Ogre::Real> currentPoseWeights;
	};

}; // namespace end

#endif