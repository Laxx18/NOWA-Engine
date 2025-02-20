/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/
#ifndef BILLBOARD_COMPONENT_H
#define BILLBOARD_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	  * @brief		Usage: Creates a billboard effect, which also can be customized.
	  */
	class EXPORTED BillboardComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:

		typedef boost::shared_ptr<BillboardComponent> BillboardCompPtr;
	public:
	
		BillboardComponent();

		virtual ~BillboardComponent();

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
		* @brief Gets whether the component behaviour is activated or not.
		* @return activated True, if the components behaviour is activated, else false
		*/
		virtual bool isActivated(void) const override;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("BillboardComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "BillboardComponent";
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
			return "Usage: Creates a billboard effect, which also can be customized.";
		}

		/**
		* @see	GameObjectComponent::createStaticApiForLua
		*/
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		void setDatablockName(const Ogre::String& datablockName);

		Ogre::String getDatablockName(void) const;

		void setPosition(const Ogre::Vector3& position);

		Ogre::Vector3 getPosition(void) const;

		void setOrigin(Ogre::v1::BillboardOrigin origin);

		Ogre::v1::BillboardOrigin getOrigin(void);

		void setDimensions(const Ogre::Vector2& dimensions);

		Ogre::Vector2 getDimensions(void) const;

		void setColor(const Ogre::Vector3& color);

		Ogre::Vector3 getColor(void) const;

		void setRotationType(Ogre::v1::BillboardRotationType rotationType);

		Ogre::v1::BillboardRotationType getRotationType(void);

		void setType(Ogre::v1::BillboardType type);

		Ogre::v1::BillboardType getType(void);

		void setRenderDistance(Ogre::Real renderDistance);

		Ogre::Real getRenderDistance(void) const;

		Ogre::v1::BillboardSet* getBillboard(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrDatablockName(void) { return "Datablock Name"; }
		static const Ogre::String AttrPosition(void) { return "Position"; }
		static const Ogre::String AttrOrigin(void) { return "Origin"; }
		static const Ogre::String AttrDimensions(void) { return "Dimensions"; }
		static const Ogre::String AttrColor(void) { return "Color"; }
		static const Ogre::String AttrRotationType(void) { return "Rotation Type"; }
		static const Ogre::String AttrType(void) { return "Type"; }
		static const Ogre::String AttrRenderDistance(void) { return "Render Distance"; }
	private:
		void createBillboard(void);

		Ogre::String mapOriginToString(Ogre::v1::BillboardOrigin origin);
		Ogre::v1::BillboardOrigin mapStringToOrigin(const Ogre::String& strOrigin);

		Ogre::String mapRotationTypeToString(Ogre::v1::BillboardRotationType rotationType);
		Ogre::v1::BillboardRotationType mapStringToRotationType(const Ogre::String& strRotationType);

		Ogre::String mapTypeToString(Ogre::v1::BillboardType type);
		Ogre::v1::BillboardType mapStringToType(const Ogre::String& strType);
	private:
		Ogre::String name;
		Ogre::v1::BillboardSet* billboardSet;
		Ogre::v1::Billboard* billboard;

		Variant* activated;
		Variant* datablockName;
		Variant* position;
		Variant* origin;
		Variant* dimensions;
		Variant* color;
		Variant* rotationType;
		Variant* type;
		Variant* renderDistance;
	};

}; //namespace end

#endif