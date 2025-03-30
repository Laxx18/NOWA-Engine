/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef FOLLOWTARGETCOMPONENT_H
#define FOLLOWTARGETCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class PhysicsActiveComponent;
	class PhysicsActiveKinematicComponent;

	/**
	  * @brief		Usage: Flies after the target game object at a given distance and with a bit random height, like a litte helper or jeene.
	  */
	class EXPORTED FollowTargetComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<FollowTargetComponent> FollowTargetComponentPtr;
	public:

		FollowTargetComponent();

		virtual ~FollowTargetComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

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

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setTargetId(unsigned long targetId); 

		unsigned long getTargetId(void) const;

		void setOffsetAngle(Ogre::Real offsetAngle); 

		Ogre::Real getOffsetAngle(void) const;

		void setYOffset(Ogre::Real yOffset); 

		Ogre::Real getYOffset(void) const;

		void setSpringLength(Ogre::Real springLength); 

		Ogre::Real getSpringLength(void) const;

		void setSpringForce(Ogre::Real springForce); 

		Ogre::Real getSpringForce(void) const;

		void setFriction(Ogre::Real friction); 

		Ogre::Real getFriction(void) const;

		void setMaxRandomHeight(Ogre::Real maxRandomHeight); 

		Ogre::Real getMaxRandomHeight(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("FollowTargetComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "FollowTargetComponent";
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
			return "Usage: Flies after the target game object at a given distance and with a bit random height, like a litte helper or jeene.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetId(void) { return "TargetId"; }
		static const Ogre::String AttrOffsetAngle(void) { return "OffsetAngle"; }
		static const Ogre::String AttrYOffset(void) { return "YOffset"; }
		static const Ogre::String AttrSpringLength(void) { return "SpringLength"; }
		static const Ogre::String AttrSpringForce(void) { return "SpringForce"; }
		static const Ogre::String AttrFriction(void) { return "Friction"; }
		static const Ogre::String AttrMaxRandomHeight(void) { return "MaxRandomHeight"; }
	private:
		void handleGameObjectDeleted(EventDataPtr eventData);

		void reset(void);
	private:
		Ogre::String name;

		GameObject* targetGameObject;
		Ogre::Real currentDegree;
		Ogre::Real currentRandomHeight;
		PhysicsActiveComponent* physicsActiveComponent;
		PhysicsActiveKinematicComponent* physicsActiveKinematicComponent;
		Ogre::Vector3 originPosition;
		Ogre::Quaternion originOrientation;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		bool firstTime;

		Variant* activated;
		Variant* targetId;
		Variant* offsetAngle;
		Variant* yOffset;
		Variant* springLength;
		Variant* springForce;
		Variant* friction;
		Variant* maxRandomHeight;
	};

}; // namespace end

#endif

