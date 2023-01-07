#ifndef BILLBOARD_COMPONENT_H
#define BILLBOARD_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED BillboardComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::BillboardComponent> BillboardCompPtr;
	public:
	
		BillboardComponent();

		virtual ~BillboardComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("BillboardComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "BillboardComponent";
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
			return "Usage: Creates billboard effect.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void setActivated(bool activated) override;

		/**
		* @brief Gets whether the component behaviour is activated or not.
		* @return activated True, if the components behaviour is activated, else false
		*/
		virtual bool isActivated(void) const override;

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