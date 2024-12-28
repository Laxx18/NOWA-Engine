/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
*/

#ifndef ANIMATIONSEQUENCECOMPONENT_H
#define ANIMATIONSEQUENCECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "utilities/SkeletonVisualizer.h"
#include "utilities/AnimationBlender.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	  * @brief		This class represents an animation sequence. Its possible to add several animations which are played in sequence, triggered by transitions.
	  */
	class EXPORTED AnimationSequenceComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<AnimationSequenceComponent> AnimationSequenceCompPtr;
	public:

		AnimationSequenceComponent();

		virtual ~AnimationSequenceComponent();

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

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AnimationSequenceComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "AnimationSequenceComponent";
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
			return "Its possible to add several animations which are played in sequence, triggered by transitions. Requirements: Entity must have animations.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

		/**
		 * @brief Sets the node animation count (how many animations are used to build the sequence).
		 * @param[in] animationCount The animation count to set
		 */
		void setAnimationCount(unsigned int animationCount);

		/**
		 * @brief Gets the animation count
		 * @return animationCount The animation count
		 */
		unsigned int getAnimationCount(void) const;

		void setAnimationName(unsigned int index, const Ogre::String& animationName);

		Ogre::String getAnimationName(unsigned int index) const;

		void setBlendTransition(unsigned int index, const Ogre::String& strBlendTransition);

		Ogre::String getBlendTransition(unsigned int index) const;

		/**
		 * @brief Sets duration how long the animation with the given index should be played until it is switched to another one (if existing).
		 * @param[in] index 		The index for the animation to set the duration
		 * @param[in] timePosition 	The duration in milliseconds
		 */
		void setDuration(unsigned int index, Ogre::Real duration);

		/**
		 * @brief Gets duration in milliseconds how long the animation with the given index is played
		 * @param[in] index 		The index for the animation to get the duration
		 * @return timePosition 	The duration in milliseconds to get for the animation
		 */
		Ogre::Real getDuration(unsigned int index);

		/**
		 * @brief Sets time position at which the animation with the given index should be played.
		 * @param[in] index 		The index, at which animation should be played
		 * @param[in] timePosition 	The time position in milliseconds
		 */
		void setTimePosition(unsigned int index, Ogre::Real timePosition);

		/**
		 * @brief Gets time position in milliseconds for at which the animation with the given index is played
		 * @param[in] index 		The index of animation to get the time position
		 * @return timePosition 	The time position in milliseconds to get for the animation
		 */
		Ogre::Real getTimePosition(unsigned int index);

		void setSpeed(unsigned int index, Ogre::Real animationSpeed);

		Ogre::Real getSpeed(unsigned int index) const;

		void setRepeat(bool animationRepeat);

		bool getRepeat(void) const;

		void setShowSkeleton(bool showSkeleton);

		bool getShowSkeleton(void) const;

		void activateAnimation(void);

		AnimationBlender* getAnimationBlender(void) const;

		Ogre::v1::OldBone* getBone(const Ogre::String& boneName);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrAnimationCount(void) { return "Animation Count"; }
		static const Ogre::String AttrAnimationName(void) { return "Animation Name "; }
		static const Ogre::String AttrBlendTransition(void) { return "Blend Transition "; }
		static const Ogre::String AttrDuration(void) { return "Duration "; }
		static const Ogre::String AttrTimePosition(void) { return "TimePosition "; }
		static const Ogre::String AttrShowSkeleton(void) { return "Show Skeleton"; }
	private:
		void generateAnimationList(void);
		Ogre::String mapBlendingTransitionToString(AnimationBlender::BlendingTransition blendingTransition);
		AnimationBlender::BlendingTransition mapStringToBlendingTransition(const Ogre::String& strBlendingTransition);
		void resetAnimation(void);
	private:
		Variant* activated;
		Variant* animationRepeat;
		Variant* showSkeleton;
		Variant* animationCount;
		std::vector<Variant*> animationNames;
		std::vector<Variant*> animationBlendTransitions;
		std::vector<Variant*> animationDurations; // See: blend(..., duration)
		std::vector<Variant*> animationTimePositions;
		std::vector<Variant*> animationSpeeds;
		std::vector<Ogre::String> availableAnimations;
		Ogre::String name;
		AnimationBlender* animationBlender;
		SkeletonVisualizer* skeletonVisualizer;
		Ogre::Real timePosition;
		bool firstTimeRepeat;
		size_t currentAnimationIndex;
	};

}; // namespace end

#endif

