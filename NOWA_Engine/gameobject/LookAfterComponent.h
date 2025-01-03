#ifndef LOOK_AFTER_COMPONENTS_H
#define LOOK_AFTER_COMPONENTS_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED LookAfterComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<LookAfterComponent> LookAfterCompPtr;
	public:

		LookAfterComponent();

		virtual ~LookAfterComponent();

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
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;
		
		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("LookAfterComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LookAfterComponent";
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
			return "Usage: Lets the game object's bone head look after a target. "
				   "Requirements: An AnimationComponent";
		}

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

		void setDefaultOrientation(const Ogre::Vector3& defaultOrientation);

		Ogre::Vector3 getDefaultOrientation(void) const;

		void setHeadBoneName(const Ogre::String& headBoneName);

		Ogre::String getHeadBoneName(void) const;

		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;

		void setLookSpeed(Ogre::Real lookSpeed);

		Ogre::Real getLookSpeed(void) const;
		
		void setMaxPitch(Ogre::Real maxPitch);

		Ogre::Real getMaxPitch(void) const;
		
		void setMaxYaw(Ogre::Real maxYaw);

		Ogre::Real getMaxYaw(void) const;
		
		Ogre::SceneNode* getTargetSceneNode(void) const;
		
		Ogre::v1::OldBone* getHeadBone(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrDefaultOrientation(void) { return "Default Orientation"; }
		static const Ogre::String AttrHeadBoneName(void) { return "Head Bone Name"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrLookSpeed(void) { return "Look Speed"; }
		static const Ogre::String AttrMaxPitch(void) { return "Max. Pitch"; }
		static const Ogre::String AttrMaxYaw(void) { return "Max. Yaw"; }
	protected:
		Variant* activated;
		Variant* defaultOrientation;
		Variant* headBoneName;
		Variant* targetId;
		Variant* lookSpeed; 
		Variant* maxPitch;
		Variant* maxYaw;
		Ogre::SceneNode* targetSceneNode;
		Ogre::v1::OldBone* headBone;
	};

}; //namespace end

#endif