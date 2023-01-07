#ifndef NODE_TRACK_COMPONENT_H
#define NODE_TRACK_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	/**
	 * @class 	NodeTrackComponent
	 * @brief 	This component can be used to move the owner game object at a specific path created by node components.
	 *			Info: Only one node track component can be added for the owner game object.
	 *			Example: A bee flying around or a light flying around an object.
	 *			Requirements: None
	 */
	class EXPORTED NodeTrackComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::NodeTrackComponent> NodeTrackCompPtr;
	public:
	
		NodeTrackComponent();

		virtual ~NodeTrackComponent();

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
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("NodeTrackComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "NodeTrackComponent";
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
			return "Usage: This Component is used for camera tracking in conjunction with several NodeComponents acting as waypoints.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets the node track count (how many nodes are used for the tracking).
		 * @param[in] nodeTrackCount The node track count to set
		 */
		void setNodeTrackCount(unsigned int nodeTrackCount);

		/**
		 * @brief Gets the node track count
		 * @return nodeTrackCount The node track count
		 */
		unsigned int getNodeTrackCount(void) const;

		/**
		 * @brief Sets the node track id for the given index in the node track list with @nodeTrackCount elements
		 * @param[in] index The index, the node track id is set in the ordered list.
		 * @param[in] id 	The id of the game object with the node component
		 * @note	The order is controlled by the index, from which node to which node this game object will be tracked.
		 */
		void setNodeTrackId(unsigned int index, unsigned long id);

		/**
		 * @brief Gets node track id from the given node track index from list
		 * @param[in] index The index, the node track id is set.
		 * @return nodeTrackCount The node track count
		 */
		unsigned long getNodeTrackId(unsigned int index);

		/**
		 * @brief Sets time position in milliseconds after which this game object should be tracked at the node from the given index.
		 * @param[in] index 		The index, at which node after x-milliseconds the game object should arrive
		 * @param[in] timePosition 	The time position in milliseconds at which the game object should arrive at the node with the given index
		 */
		void setTimePosition(unsigned int index, Ogre::Real timePosition);

		/**
		 * @brief Gets time position in milliseconds for the node with the given index.
		 * @param[in] index 		The index of node to get the time position
		 * @return timePosition 	The time position in milliseconds to get for the node
		 */
		Ogre::Real getTimePosition(unsigned int index);

		/**
		 * @brief Sets the curve interpolation mode how the game object will be moved. 
		 * @param[in] interpolationMode The interpolation mode to set
		 * @note	Possible values are: "Spline", "Linear"
		 */
		void setInterpolationMode(const Ogre::String& interpolationMode);

		/**
		 * @brief Gets the curve interpolation mode how the game object is moved. 
		 * @return interpolationMode The interpolation mode to get
		 * @note	Possible values are: "Spline", "Linear"
		 */
		Ogre::String getInterpolationMode(void) const;

		/**
		 * @brief Sets the rotation mode how the game object will be rotated during movement. 
		 * @param[in] rotationMode The rotation mode to set
		 * @note	Possible values are: "Linear", "Spherical"
		 */
		void setRotationMode(const Ogre::String& rotationMode);

		/**
		 * @brief Gets the rotation mode how the game object is rotated during movement. 
		 * @return interpolationMode The interpolation mode to get
		 * @note	Possible values are: "Spline", "Linear"
		 */
		Ogre::String getRotationMode(void) const;

		/**
		 * @brief Gets Ogre v1 animation pointer to control the animation directly.
		 * @return animation The animation pointer to get
		 */
		Ogre::v1::Animation* getAnimation(void) const;

		/**
		 * @brief Gets Ogre v1 animation track pointer to control the animation track directly.
		 * @return animationTrack The animation track pointer to get
		 */
		Ogre::v1::NodeAnimationTrack* getAnimationTrack(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrNodeTrackCount(void) { return "Count"; }
		static const Ogre::String AttrNodeTrackId(void) { return "Node Track Id "; }
		static const Ogre::String AttrTimePosition(void) { return "Time Position "; }
		static const Ogre::String AttrInterpolationMode(void) {return "Interpolation Mode"; }
		static const Ogre::String AttrRotationMode(void) { return "Rotation Mode"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
	private:
		Variant* activated;
		Variant* nodeTrackCount;
		std::vector<Variant*> nodeTrackIds;
		std::vector<Variant*> timePositions;
		Variant* interpolationMode;
		Variant* rotationMode;
		Variant* repeat;
		Ogre::v1::Animation* animation;
		Ogre::v1::NodeAnimationTrack* animationTrack;
		Ogre::v1::AnimationState* animationState;
	};

}; //namespace end

#endif