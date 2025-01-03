#ifndef MESH_DECAL_COMPONENT_H
#define MESH_DECAL_COMPONENT_H

#include "GameObjectComponent.h"
#include "modules/MeshDecalGeneratorModule.h"

namespace NOWA
{
	class EXPORTED MeshDecalComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::MeshDecalComponent> MeshDecalCompPtr;
	public:
	
		MeshDecalComponent();

		virtual ~MeshDecalComponent();

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
			return NOWA::getIdFromName("MeshDecalComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MeshDecalComponent";
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
			return "Usage: Creates a mesh decal.";
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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
		
		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;
		
		virtual bool isActivated(void) const override;
		
		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;
		
		void setTextureName(const Ogre::String& textureName);
		
		Ogre::String getTextureName(void) const;
		
		void setSize(const Ogre::Vector2& size);
		
		Ogre::Vector2 getSize(void) const;

		void setUpdateFrequency(Ogre::Real updateFrequency);

		Ogre::Real getUpdateFrequency(void) const;

		Ogre::ManualObject* getMeshDecal(void) const;

		void setPosition(const Ogre::Vector3& position);

		void setMousePosition(int x, int y);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrTextureName(void) { return "Texture Name"; }
		static const Ogre::String AttrSize(void) { return "Size"; }
		static const Ogre::String AttrUpdateFrequency(void) { return "Update Frequency (sec)"; }
	private:
		void createMeshDecal(const Ogre::Vector3& position);
		void destroyMeshDecal(void);
	private:
		Variant* activated;
		Variant* targetId;
		Variant* textureName;
		Variant* size;
		Variant* updateFrequency;
		Ogre::ManualObject* meshDecal;
		Ogre::Real updateTimer;
		unsigned int categoryId;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		Ogre::ManualObject* decal;
	};

}; //namespace end

#endif