#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H

#include "GameObjectComponent.h"
#include "utilities/SkeletonVisualizer.h"
#include "utilities/AnimationBlender.h"

namespace NOWA
{
	class EXPORTED AnimationComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<AnimationComponent> AnimationCompPtr;
	public:
		AnimationComponent();

		virtual ~AnimationComponent();

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
			return NOWA::getIdFromName("AnimationComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AnimationComponent";
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
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Play one animation. Requirements: Entity must have a skeleton with animations.";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual bool isActivated(void) const override;

		void setAnimationName(const Ogre::String& animationName);

		const Ogre::String getAnimationName(void) const;

		void setSpeed(Ogre::Real animationSpeed);

		Ogre::Real getSpeed(void) const;

		void setRepeat(bool animationRepeat);

		bool getRepeat(void) const;

		void setShowSkeleton(bool showSkeleton);

		bool getShowSkeleton(void) const;

		void activateAnimation(void);

		bool isComplete(void) const;
		
		AnimationBlender* getAnimationBlender(void) const;
		
		Ogre::v1::OldBone* getBone(const Ogre::String& boneName);

		Ogre::Vector3 getLocalToWorldPosition(Ogre::v1::OldBone* bone);

		Ogre::Quaternion getLocalToWorldOrientation(Ogre::v1::OldBone* bone);
		
		// These methods are just for lua and are not stored for an editor
		
		void setTimePosition(Ogre::Real timePosition);
		
		Ogre::Real getTimePosition(void) const;
		
		Ogre::Real getLength(void) const;
		
		void setWeight(Ogre::Real weight);
		
		Ogre::Real getWeight(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrName(void) { return "Animation Name"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrShowSkeleton(void) { return "Show Skeleton"; }
	private:
		void resetAnimation(void);
		void createAnimationBlender(void);
	protected:
		Variant* activated;
		Variant* animationName;
		Variant* animationSpeed;
		Variant* animationRepeat;
		Variant* showSkeleton;
		AnimationBlender* animationBlender;
		SkeletonVisualizer* skeletonVisualizer;
		bool isInSimulation;
	};

}; //namespace end

#endif