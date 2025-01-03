#ifndef BACKGROUND_SCROLL_COMPONENT_H
#define BACKGROUND_SCROLL_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class WorkspaceBackgroundComponent;

	class EXPORTED BackgroundScrollComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::BackgroundScrollComponent> BackgroundScrollCompPtr;
	public:
	
		BackgroundScrollComponent();

		virtual ~BackgroundScrollComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index);

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::pause
		*/
		virtual void pause(void) override;

		/**
		* @see		GameObjectComponent::resume
		*/
		virtual void resume(void) override;

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
			return NOWA::getIdFromName("BackgroundScrollComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "BackgroundScrollComponent";
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
			return "Usage: Adds background control like scrolling or follow game object. "
				"Requirements: This component can only be placed under a game object with a workspace background component. Up to 9 background can be used.";
		}

		void update(Ogre::Real dt, bool notSimulating);

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;

		void setMoveSpeedX(Ogre::Real moveSpeedX);

		Ogre::Real getMoveSpeedX(void) const;

		void setMoveSpeedY(Ogre::Real moveSpeedY);

		Ogre::Real getMoveSpeedY(void) const;

		void setFollowGameObjectX(bool followGameObjectX);

		bool getFollowGameObjectX(void) const;

		void setFollowGameObjectY(bool followGameObjectY);

		bool getFollowGameObjectY(void) const;

		void setFollowGameObjectZ(bool followGameObjectZ);

		bool getFollowGameObjectZ(void) const;

		void setBackgroundName(const Ogre::String& backgroundName);

		Ogre::String getBackgroundName(void) const;

	public:
		static const Ogre::String AttrActive(void){ return "Active"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrMoveSpeedX(void) { return "Move Speed X"; }
		static const Ogre::String AttrMoveSpeedY(void) { return "Move Speed Y"; }
		static const Ogre::String AttrFollowGameObjectX(void) { return "Follow GameObject X"; }
		static const Ogre::String AttrFollowGameObjectY(void) { return "Follow GameObject Y"; }
		static const Ogre::String AttrFollowGameObjectZ(void) { return "Follow GameObject Z"; }
		static const Ogre::String AttrBackgroundName(void) { return "Background Name"; }
	private:
		bool setupBackground(void);
	private:
		Variant* active;
		Variant* targetId;
		Variant* moveSpeedX;
		Variant* moveSpeedY;
		Variant* followGameObjectX;
		Variant* followGameObjectY;
		Variant* followGameObjectZ;
		Variant* backgroundName;

		Ogre::SceneNode* targetSceneNode;
		Ogre::Vector2 lastPosition;
		Ogre::Vector2 pausedLastPosition;
		bool firstTimePositionSet;
		Ogre::Real xScroll;
		Ogre::Real yScroll;

		WorkspaceBackgroundComponent* workspaceBackgroundComponent;
	};

}; //namespace end

#endif