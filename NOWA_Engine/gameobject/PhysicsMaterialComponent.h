#ifndef PHYSICS_MATERIAL_COMPONENT_H
#define PHYSICS_MATERIAL_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class ConveyorContactCallback;
	class GenericContactCallback;
	class LuaScript;

	class EXPORTED PhysicsMaterialComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::PhysicsMaterialComponent> PhysicsMaterialCompPtr;
	public:
	
		PhysicsMaterialComponent();

		virtual ~PhysicsMaterialComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsMaterialComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsMaterialComponent";
		}

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: With this component physics effects like friction, possibility on reacting when two physically active game objects have collided etc. It glues two game object categories together. "
				   "Note: Several of this components can be created.\n"
					"Attention: Physics material collision do only work for the following constellations:\n"
					"Category1						Category2							Result\n"
					"PhysicsActiveComponent			PhysicsActiveComponent				Working\n"
					"PhysicsArtifactComponent		PhysicsActiveComponent				Working\n"
					"PhysicsActiveComponent			PhysicsKinematicComponent			Working\n"
					"PhysicsArtifactComponent		PhysicsKinematicComponent			Not Working\n"
					"PhysicsKinematicComponent		PhysicsKinematicComponent			Not Working\n"
					"The two last constellations do not work, because no force is taking place, use PhysicsTriggerComponent, in order to detect collisions of this ones!";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override { };

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setCategory1(const Ogre::String& category1);

		Ogre::String getCategory1(void) const;

		void setCategory2(const Ogre::String& category2);

		Ogre::String getCategory2(void) const;

		void setFriction(const Ogre::Vector2& friction);

		Ogre::Vector2 getFriction(void) const;

		void setSoftness(Ogre::Real softness);

		Ogre::Real getSoftness(void) const;

		void setElasticity(Ogre::Real elasticity);

		Ogre::Real getElasticity(void) const;

		void setSurfaceThickness(Ogre::Real surfaceThickness);

		Ogre::Real getSurfaceThickness(void) const;

		void setCollideable(bool collideable);

		bool getCollideable(void) const;

		void setContactBehavior(const Ogre::String& contactBehavior);

		Ogre::String getContactBehavior(void) const;

		void setContactSpeed(Ogre::Real contactSpeed);

		Ogre::Real getContactSpeed(void) const;

		void setContactDirection(const Ogre::Vector3 contactDirection);

		Ogre::Vector3 getContactDirection(void) const;

		void setOverlapFunctionName(const Ogre::String& overlapFunctionName);

		void setContactFunctionName(const Ogre::String& contactFunctionName);

		void setContactOnceFunctionName(const Ogre::String& contactOnceFunctionName);

		void setContactScratchFunctionName(const Ogre::String& contactScratchFunctionName);

		/*
		* @brief Sets the physics contact callback script file to react in script at the moment two game objects with physics components collide.
		* @param[in]	contactScriptFilePathName	The physics contact callback script file
		* @note			The file must have the following functions implemented and the module name must be called like the lua file without the lua extension. E.g. 'Contact.lua':
		*	module("Contact", package.seeall);
		*	function onAABBOverlap(gameObject0, gameObject1)
		*	end
		*
		*	function onContact(gameObject0, gameObject1, contact)
		*	end
		*
		*	function onContactOnce(gameObject0, gameObject1, contact)
		*	end
		*/
	public:
		static const Ogre::String AttrCategory1(void) { return "Category 1"; }
		static const Ogre::String AttrCategory2(void) { return "Category 2"; }
		static const Ogre::String AttrFriction(void) { return "Friction"; }
		static const Ogre::String AttrSoftness(void) { return "Softness"; }
		static const Ogre::String AttrElasticity(void) { return "Elasticity"; }
		static const Ogre::String AttrSurfaceThickness(void) { return "Surface Thickness"; }
		static const Ogre::String AttrCollideable(void) { return "Colideable"; }
		static const Ogre::String AttrContactBehavior(void) { return "Contact Behavior"; }
		static const Ogre::String AttrContactSpeed(void) { return "Contact Speed"; }
		static const Ogre::String AttrContactDirection(void) { return "Contact Direction"; }
		static const Ogre::String AttrOverlapFunctionName(void) { return "Overlap Function Name"; }
		static const Ogre::String AttrContactFunctionName(void) { return "Contact Function Name"; }
		static const Ogre::String AttrContactOnceFunctionName(void) { return "Contact Once Function Name"; }
		static const Ogre::String AttrContactScratchFunctionName(void) { return "Contact Scratch Function Name"; }
	private:
		void createMaterialPair(void);
	private:
		Variant* category1;
		Variant* category2;
		Variant* friction;
		Variant* softness;
		Variant* elasticity;
		Variant* surfaceThickness;
		Variant* collideable;
		Variant* contactBehavior;
		Variant* contactSpeed;
		Variant* contactDirection;
		Variant* overlapFunctionName;
		Variant* contactFunctionName;
		Variant* contactOnceFunctionName;
		Variant* contactScratchFunctionName;

		OgreNewt::World* ogreNewt;
		OgreNewt::MaterialPair* materialPair;
		ConveyorContactCallback* conveyorContactCallback;
		GenericContactCallback* genericContactCallback;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	class ConveyorContactCallback : public OgreNewt::ContactCallback
	{
	public:
		ConveyorContactCallback(Ogre::Real speed, const Ogre::Vector3& direction, int conveyorCategoryId, bool forPlayer);
		~ConveyorContactCallback();

		int onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex);

		void contactsProcess(OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex);

		void setContactSpeed(Ogre::Real contactSpeed);

		void setContactDirection(const Ogre::Vector3& contactDirection);
	private:
		int conveyorCategoryId;
		Ogre::Real speed;
		Ogre::Vector3 direction;
		bool forPlayer;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	class GenericContactCallback : public OgreNewt::ContactCallback
	{
	public:
		GenericContactCallback(LuaScript* luaScript, int firstObjectId, const Ogre::String& overlapFunctionName, 
			const Ogre::String& contactFunctionName, const Ogre::String& contactOnceFunctionName, const Ogre::String& contactOnceScratchName);

		~GenericContactCallback();

		int onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex) override;

		void contactsProcess(OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex) override;
	private:
		int firstObjectId;
		Ogre::Real lastNormalSpeed;
		GameObject* gameObject0;
		GameObject* gameObject1;
		OgreNewt::Body* body0;
		OgreNewt::Body* body1;
		LuaScript* luaScript;
		Ogre::String overlapFunctionName;
		Ogre::String contactFunctionName;
		Ogre::String contactOnceFunctionName;
		Ogre::String contactScratchFunctionName;
	};

}; //namespace end

#endif