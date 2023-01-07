#ifndef PHYSICS_TRIGGER_COMPONENT_H
#define PHYSICS_TRIGGER_COMPONENT_H

#include "PhysicsActiveComponent.h"
#include "OgreNewt_World.h"
#include "OgreNewt_TriggerBody.h"

namespace NOWA
{
	class LuaScript;

	class EXPORTED PhysicsTriggerComponent : public PhysicsActiveComponent
	{
	public:
		class PhysicsTriggerCallback : public OgreNewt::TriggerCallback
		{
		public:
			PhysicsTriggerCallback(GameObject* owner, LuaScript* luaScript);

			virtual ~PhysicsTriggerCallback();

			virtual void OnEnter(const OgreNewt::Body* visitor) override;

			virtual void OnInside(const OgreNewt::Body* visitor) override;

			virtual void OnExit(const OgreNewt::Body* visitor) override;

			void setLuaScript(LuaScript* luaScript);

			void setDebugData(bool bDebugData);

			void setCategoryId(unsigned int categoryId);
		private:
			GameObject* owner;
			LuaScript* luaScript;
			bool bDebugData;
			bool onInsideFunctionAvailable;
			unsigned int categoryId;
		};
	public:
		typedef boost::shared_ptr<PhysicsTriggerComponent> PhysicsTriggerCompPtr;
	public:

		PhysicsTriggerComponent();

		virtual ~PhysicsTriggerComponent();

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
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		PhysicsComponent::isMovable
		 */
		virtual bool isMovable(void) const override
		{
			return false;
		}

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		virtual void showDebugData(void) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsTriggerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsTriggerComponent";
		}

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: This component is used to specify an area, which is triggered when another game object enters the erea. "
				   "Requirements: A kind of physics component.";
		}

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override
		{

		}

		virtual void reCreateCollision(void) override;

		void setCategories(const Ogre::String& categories);

		Ogre::String getCategories(void) const;
	public:
		static const Ogre::String AttrCategories(void) { return "Categories"; }
	protected:
		virtual bool createDynamicBody(void);
	private:
		Variant* categories;
		unsigned int categoriesId;
	};

}; //namespace end

#endif