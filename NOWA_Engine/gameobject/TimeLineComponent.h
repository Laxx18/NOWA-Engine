#ifndef TIME_LINE_COMPONENT_H
#define TIME_LINE_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	/**
	 * @class 	TimeLineComponent
	 * @brief 	This component can be used in the main game object to add id's of other game objects, that are activated at a specifig time point and deactivated after a duration of time.
	 * @note	The time line propagation starts, as soon as this game object has been connected.
	 *			Requirements: MainGameObject
	 * @details
	 *			// Approach as follows:
	 *  1) Call start function + repeat count start point - 1 + end point for 1 Timepoint
	 *  Example:
	 *  Timepoint count: 3
	 *  startPoint[0] = 2
	 *  repeatCount[0] = 4
	 *  duration[0] = 4
	 *   
	 *  startPoint[1] = 7
	 *  repeatCount[1] = 0
	 *  duration[1] = 4
	 *   
	 *  startPoint[2] = 13
	 *  repeatCount[2] = 3
	 *  duration[2] = 6
	 *  Each time a startpoint or repeat count point is triggered, the timePointStartEventName function for lua is called
	 *  Each time an endpoint is triggered, the timePointEndEventName function for lua is called
	 *  In this example 8x timePointStartEventName and 3x timePointEndEventName
	 *  STx -> timePointStartEventName function
	 *  RCx -> timePointStartEventName function
	 *  EPx -> timePointEndEventName function
	 *  
	 *  sumTimePoints (with repeat counts) = startPoint * 3 + (repeatCount - 1) * 3 + endPoint * 3 = 11 = total points
	 *  
	 *                   D0 = 4                   D1 = 4                             D2 = 6
	 *            __________|__________    __________|__________         _______________|_______________
	 *            |                   |    |                   |         |                             |
	 *           ST0                 EP0  ST1                 EP1       ST2                           EP2
	 *            |   RC0  RC1  RC2   |    |                   |         |        RC0       RC1        |
	 *            |    |    |    |    |    |                   |         |         |         |         |
	 *            |    |    |    |    |    |                   |         |         |         |         | 
	 *  |----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|-> time (sec)
	 *  0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19  20
	 *  
	 *                    R = 4                    R = 0                              R = 3
	 */
	class EXPORTED TimeLineComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::TimeLineComponent> NodeTrackCompPtr;
	public:
	
		TimeLineComponent();

		virtual ~TimeLineComponent();

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
			return NOWA::getIdFromName("TimeLineComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "TimeLineComponent";
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
			return "Usage: This component can be used in the main game object to add id's of other game objects, that are activated at a specifig time point and deactivated after a duration of time. The time line propagation starts, as soon as this game object has been connected."
				"Or if a lua script component does exist an its possible to react on an event at the given time point."
				"Example: Creating a film sequence or a space game, in which enemy space ships are spawned at specific time points. Its also possible set a current time and automatically the nearest time point is determined."
				"Requirements: MainGameObject ";
		}

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
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets the time point count.
		 * @param[in] timePointCount The time point count to set
		 */
		void setTimePointCount(unsigned int timePointCount);

		/**
		 * @brief Gets the time point count
		 * @return timePointCount The time point count
		 */
		unsigned int getTimePointCount(void) const;

		/**
		 * @brief Sets the game object id for the given index in the game object list
		 * @param[in] index The index, the game object id is set in the ordered list.
		 * @param[in] id 	The id of the game object
		 * @note	The order is controlled by the index.
		 */
		void setGameObjectId(unsigned int index, unsigned long id);

		/**
		 * @brief Gets game object id from the given game object index from list
		 * @param[in] index The index, the game object id is set.
		 * @return gameObjectId The game object id for the given index
		 */
		unsigned long getGameObjectId(unsigned int index);

		/**
		 * @brief Sets the start time in seconds after which this game object should be activated from the given index or if a lua script does exist, the corresponding event name is called.
		 * @param[in] index 		The index of the game object
		 * @param[in] timePosition 	The start time in seconds at which the game object should be activated
		 */
		void setStartTime(unsigned int index, Ogre::Real startTime);

		/**
		 * @brief Gets the start time in seconds for the game object with the given index.
		 * @param[in] index 		The index of game object to get the start time
		 * @return timePosition 	The start time in seconds to get for the game object
		 */
		Ogre::Real getStartTime(unsigned int index);

		/**
		 * @brief Sets the duration in seconds, how long the game object should remain activated.
		 * @param[in] index 		The index of the duration
		 * @param[in] duration 		The duration in seconds
		 */
		void setDurationSec(unsigned int index, Ogre::Real duration);

		/**
		 * @brief Gets time duration in seconds
		 * @param[in] index 		The index of the duration
		 * @return duration 		The duration in seconds to get
		 */
		Ogre::Real getDurationSec(unsigned int index);

		/**
		 * @brief Sets the repeat count. That is, how often will the timePointStartEventName be called until the next time point is active. Default value is 0.
		 * @param[in] index 		The index of the repeat count
		 * @param[in] duration 		The repeat count.
		 */
		void setRepeatCount(unsigned int index, unsigned int repeatCount);

		/**
		 * @brief Gets the repeat count. That is, how often will the timePointStartEventName be called until the next time point is active. Default value is 0.
		 * @param[in] index 		The index of the repeat count
		 * @return repeatCount 		The repeat count to get
		 */
		unsigned int getRepeatCount(unsigned int index);

		/**
		 * @brief Sets the lua start event name, in order to react when the time point is met.
		 * @param[in] index 					The index of the start event name
		 * @param[in] timePointStartEventName 	The time point start event name to set.
		 */
		void setTimePointStartEventName(unsigned int index, const Ogre::String& timePointStartEventName);

		/**
		 * @brief Gets the lua start event name, in order to react when the time point is met.
		 * @param[in] index 				The index of get the start event name.
		 * @return timePointStartEventName 	The time point start event name to get.
		 */
		Ogre::String getTimePointStartEventName(unsigned int index);

		/**
		 * @brief Sets the lua end event name, in order to react when the time point is met. Which is start point + duration in seconds.
		 * @param[in] index 					The index of the end event name
		 * @param[in] timePointStartEventName 	The time point end event name to set.
		 */
		void setTimePointEndEventName(unsigned int index, const Ogre::String& timePointEndEventName);

		/**
		 * @brief Gets the lua end event name, in order to react when the time point is met.
		 * @param[in] index 				The index of get the end event name.
		 * @return timePointStartEventName 	The time point end event name to get.
		 */
		Ogre::String getTimePointEndEventName(unsigned int index);

		/**
		 * @brief Sets the current time in seconds.
		 * @param[in] seconds 	The time in seconds to set.
		 * @return	 success	True, if the given time is within the time line duration range, else false
		 * @note	The next time point is determined, and the corresponding game object or lua function (if existing) called.
		 */
		bool setCurrentTimeSec(Ogre::Real seconds);

		/**
		 * @brief Gets the current time in seconds.
		 * @return[in] timeSec 	The current time, since this component is running in seconds.
		 */
		Ogre::Real getCurrentTimeSec(void) const;

		/**
		 * @brief Calculates and gets the maximum time line duration in seconds.
		 * @return[in] timeLineTimeSec 	The maximum time line duration in seconds.
		 * @note		Do not call this to often, because the max time is calculated each time from the scratch, in order to be as flexible as possible. E.g. a time point has been removed during runtime.
		 */
		Ogre::Real getMaxTimeLineDuration(void);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTimePointCount(void) { return "Time Point Count"; }
		static const Ogre::String AttrGameObjectId(void) { return "game object Id "; }
		static const Ogre::String AttrStartTime(void) { return "Start Time "; }
		static const Ogre::String AttrDuration(void) {return "Duration "; }
		static const Ogre::String AttrRepeatCount(void) {return "Repeat Count "; }
		static const Ogre::String AttrTimePointStartEventName(void) { return "Timepoint Start Event Name "; }
		static const Ogre::String AttrTimePointEndEventName(void) { return "Timepoint End Event Name "; }
	private:
		Variant* activated;
		Variant* timePointCount;
		std::vector<Variant*> gameObjectIds;
		std::vector<Variant*> startTimes;
		std::vector<Variant*> durations;
		std::vector<Variant*> repeatCounts;
		std::vector<Variant*> timePointStartEventNames;
		std::vector<Variant*> timePointEndEventNames;
		std::vector<GameObject*> gameObjects;
		std::vector<bool> alreadyActivatedList;
		unsigned int timePointIndex;
		std::vector<unsigned int> globalSubIndex;
		unsigned int totalIndex;
		Ogre::Real timeDt;
		bool callForEndPoint;
	};

}; //namespace end

#endif