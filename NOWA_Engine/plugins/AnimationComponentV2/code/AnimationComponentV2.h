/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef ANIMATIONCOMPONENTV2_H
#define ANIMATIONCOMPONENTV2_H

#include "gameobject/GameObjectComponent.h"
#include "utilities/AnimationBlenderV2.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace Ogre
{
	class SkeletonAnimation;
	class Bone;
}

namespace NOWA
{

	/**
	  * @brief		Play one animation with Ogre Item (v2). Requirements: Item must have a skeleton with animations.
	  */
	class EXPORTED AnimationComponentV2 : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<AnimationComponentV2> AnimationComponentV2Ptr;
	public:

		AnimationComponentV2();

		virtual ~AnimationComponentV2();

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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

		void setAnimationName(const Ogre::String& animationName);

		const Ogre::String getAnimationName(void) const;

		void setSpeed(Ogre::Real animationSpeed);

		Ogre::Real getSpeed(void) const;

		void setRepeat(bool animationRepeat);

		bool getRepeat(void) const;

		void activateAnimation(void);

		bool isComplete(void) const;

		AnimationBlenderV2* getAnimationBlender(void) const;

		Ogre::Bone* getBone(const Ogre::String& boneName);

		Ogre::Vector3 getLocalToWorldPosition(Ogre::Bone* bone);

		Ogre::Quaternion getLocalToWorldOrientation(Ogre::Bone* bone);

		// These methods are just for lua and are not stored for an editor

		void setTimePosition(Ogre::Real timePosition);

		Ogre::Real getTimePosition(void) const;

		Ogre::Real getLength(void) const;

		void setWeight(Ogre::Real weight);

		Ogre::Real getWeight(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AnimationComponentV2");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "AnimationComponentV2";
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
			return "Usage: Play one animation with Ogre Item (v2). Requirements: Item must have a skeleton with animations.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrName(void) { return "Animation Name"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
	private:
		void resetAnimation(void);
		void createAnimationBlender(void);
	protected:
		Ogre::String name;
		Ogre::SkeletonInstance* skeleton;
		bool isInSimulation;
		Variant* activated;
		Variant* animationName;
		Variant* animationSpeed;
		Variant* animationRepeat;
		AnimationBlenderV2* animationBlender;
	};

}; // namespace end

#endif
