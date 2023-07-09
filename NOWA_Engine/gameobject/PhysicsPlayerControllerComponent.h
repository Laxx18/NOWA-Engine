#ifndef PHYSICS_PLAYER_CONTROLLER_COMPONENT_H
#define PHYSICS_PLAYER_CONTROLLER_COMPONENT_H

#include "PhysicsActiveComponent.h"
#include "OgreNewt_PlayerControllerBody.h"

namespace NOWA
{
	class PlayerContact;

	class EXPORTED PhysicsPlayerControllerComponent : public PhysicsActiveComponent
	{
	public:
		class PhysicsPlayerCallback : public OgreNewt::PlayerCallback
		{
		public:
			PhysicsPlayerCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onContactFrictionFunctionName,
				const Ogre::String& onContactFunctionName);

			virtual ~PhysicsPlayerCallback();

			virtual Ogre::Real onContactFriction(const OgreNewt::PlayerControllerBody* visitor, const Ogre::Vector3& position, const Ogre::Vector3& normal, int contactId, const OgreNewt::Body* other) override;

			virtual void onContact(const OgreNewt::PlayerControllerBody* visitor, const Ogre::Vector3& position, const Ogre::Vector3& normal, Ogre::Real penetration, int contactId, const OgreNewt::Body* other) override;
			
			void setLuaScript(LuaScript* luaScript);

			void setCallbackFunctions(const Ogre::String& onContactFrictionFunctionName, const Ogre::String& onContactFunctionName);

		private:
			GameObject* owner;
			LuaScript* luaScript;
			OgreNewt::World* ogreNewt;
			Ogre::String onContactFrictionFunctionName;
			Ogre::String onContactFunctionName;
			PlayerContact* playerFrictionContact;
			PlayerContact* playerContact;
		};
	public:
		typedef boost::shared_ptr<PhysicsPlayerControllerComponent> PhysicsPlayerControllerCompPtr;
	public:

		PhysicsPlayerControllerComponent();

		virtual ~PhysicsPlayerControllerComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual bool connect(void) override;

		virtual bool disconnect(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual Ogre::String getParentParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		virtual bool isMovable(void) const override
		{
			return true;
		}

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsPlayerControllerComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsPlayerControllerComponent";
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
			return "Usage: A physics player controller component with jump/stairs up going functionality. Note: @PhysicMaterialComponent for collision detection cannot be used, as this controller is kinematic. "
				"But it has two own collision detection functions, which can be used in lua: @onContactFriction(gameObject0, gameObject1, playerContact) and @onContact(gameObject0, gameObject1, playerContact).";
		}

		///! set the characters velocity, the -Speed-values can be negative, sideSpeed positive means move to the right, heading is in absolute space
		void move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngleRad);

		void setHeading(const Ogre::Radian& headingAngleRad);

		void stop(void);

		void toggleCrouch(void);

		void jump(void);

		void setDefaultDirection(const Ogre::Vector3& defaultDirection);

		virtual void setMass(Ogre::Real mass) override;

		virtual void setVelocity(const Ogre::Vector3& velocity) override;

		virtual Ogre::Vector3 getVelocity(void) const override;

		virtual void setOrientation(const Ogre::Quaternion& orientation) override;

		virtual void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z) override;

		virtual void setPosition(const Ogre::Vector3& position) override;

		virtual void setScale(const Ogre::Vector3& scale) override;

		virtual void actualizeValue(Variant* attribute) override;

		void setFrame(const Ogre::Quaternion& frame);

		Ogre::Quaternion getFrame(void) const;

		virtual void setCollisionPosition(const Ogre::Vector3& collisionPosition) override;

		void setRadius(Ogre::Real radius);

		Ogre::Real getRadius(void) const;

		void setHeight(Ogre::Real height);

		Ogre::Real getHeight(void) const;

		void setStepHeight(Ogre::Real stepHeight);

		Ogre::Real getStepHeight(void) const;

		Ogre::Vector3 getUpDirection(void) const;

		bool isInFreeFall(void) const;

		bool isOnFloor(void) const;

		bool isCrouching(void) const;

		virtual void setSpeed(Ogre::Real speed) override;

		void setJumpSpeed(Ogre::Real jumpSpeed);

		Ogre::Real getJumpSpeed(void) const;

		Ogre::Real getForwardSpeed(void) const;

		Ogre::Real getSideSpeed(void) const;

		Ogre::Real getHeading(void) const;

		/**
		 * @brief Sets the lua function name, to react when a player controller has friction with another game object below.
		 * @param[in]	onEnterFunctionName		The function name to set
		 */
		void setOnContactFrictionFunctionName(const Ogre::String& onContactFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnContactFrictionFunctionName(void) const;

		/**
		 * @brief Sets the lua function name, to react when a player controller collided with another game object.
		 * @param[in]	onContactFunctionName		The function name to set
		 */
		void setOnContactFunctionName(const Ogre::String& onContactFunctionName);

		/**
		 * @brief Gets the lua function name.
		 * @return lua function name to get
		 */
		Ogre::String getOnContactFunctionName(void) const;
	public:
		static const Ogre::String AttrRadius(void) { return "Radius"; }
		static const Ogre::String AttrHeight(void) { return "Height"; }
		static const Ogre::String AttrStepHeight(void) { return "Step Height"; }
		static const Ogre::String AttrJumpSpeed(void) { return "Jump Speed"; }
		static const Ogre::String AttrOnContactFrictionFunctionName(void) { return "On Contact Friction Function Name"; }
		static const Ogre::String AttrOnContactFunctionName(void) { return "On Contact Function Name"; }
	protected:
		virtual bool createDynamicBody(void);
	private:
		void handleTranslateFinished(NOWA::EventDataPtr eventData);

		void handleRotateFinished(NOWA::EventDataPtr eventData);

		void handleDefaultDirectionChanged(NOWA::EventDataPtr eventData);

		void adjustScale(void);
	protected:
		OgreNewt::PlayerController* playerController;
		Variant* radius;
		Variant* height;
		Variant* stepHeight;
		// Note: walk speed comes from PAC
		Variant* jumpSpeed;
		Variant* onContactFrictionFunctionName;
		Variant* onContactFunctionName;

		bool newlyCreated;
	};

	//////////////////////////////////////////////////////////////////////////////////

	class EXPORTED PlayerContact
	{
	public:
		friend class PhysicsPlayerCallback;
		friend class PhysicsPlayerControllerComponent::PhysicsPlayerCallback;

	public:
		PlayerContact();

		~PlayerContact();

		Ogre::Vector3 getPosition(void) const;

		Ogre::Vector3 getNormal(void) const;

		Ogre::Real getPenetration(void) const;

		void setResultFriction(Ogre::Real resultFriction);

		Ogre::Real getResultFriction(void) const;

	private:
		Ogre::Vector3 position;
		Ogre::Vector3 normal;
		Ogre::Real penetration;
		Ogre::Real resultFriction;
	};

}; //namespace end

#endif