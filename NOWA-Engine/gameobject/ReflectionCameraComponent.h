#ifndef REFLECTION_CAMERA_COMPONENT_H
#define REFLECTION_CAMERA_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED ReflectionCameraComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::ReflectionCameraComponent> ReflectionCameraCompPtr;
	public:
		friend class WorkspaceBaseComponent;

		ReflectionCameraComponent();

		virtual ~ReflectionCameraComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ReflectionCameraComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ReflectionCameraComponent";
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
			return "Usage: Creates a reflection camera that will be used in a workspace with dynamic cubemap reflection.";
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

		void setNearClipDistance(Ogre::Real nearClipDistance);

		Ogre::Real getNearClipDistance(void) const;

		void setFarClipDistance(Ogre::Real farClipDistance);

		Ogre::Real getFarClipDistance(void) const;

		void setOrthographic(bool orthographic);

		bool getIsOrthographic(void) const;

		void setFovy(const Ogre::Degree& fovy);

		Ogre::Degree getFovy(void) const;

		void setFixedYawAxis(bool fixedYawAxis);

		bool getFixedYawAxis(void) const;

		void setCubeTextureSize(Ogre::String& cubeTextureSize);

		Ogre::String getCubeTextureSize(void) const;

		Ogre::Camera* getCamera(void) const;
	public:
		static const Ogre::String AttrNearClipDistance(void) { return "Near Clip Distance"; }
		static const Ogre::String AttrFarClipDistance(void) { return "Far Clip Distance"; }
		static const Ogre::String AttrOrthographic(void) { return "Orthographic"; }
		static const Ogre::String AttrFovy(void) { return "Fovy"; }
		static const Ogre::String AttrFixedYawAxis(void) { return "Fixed Yaw Axis"; }
		static const Ogre::String AttrCubeTextureSize(void) { return "Cube Texture Size"; }
	private:
		void createCamera(void);
	private:
		static bool justCreated;
	private:
		Ogre::Camera* camera;
		Ogre::v1::Entity* dummyEntity;
		Variant* nearClipDistance;
		Variant* farClipDistance;
		Variant* orthographic;
		Variant* fovy;
		Variant* fixedYawAxis;
		Variant* cubeTextureSize;
		
		bool hideEntity;
		WorkspaceBaseComponent* workspaceBaseComponent;
	};

}; //namespace end

#endif