#ifndef MOVE_MATH_FUNCTION_COMPONENT_H
#define MOVE_MATH_FUNCTION_COMPONENT_H

#include "GameObjectComponent.h"
#include <fparser.hh>
#include "utilities/LuaObserver.h"

namespace NOWA
{
	class PhysicsActiveComponent;
	class PhysicsActiveKinematicComponent;
	
	class EXPORTED MoveMathFunctionComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::MoveMathFunctionComponent> MoveMathFunctionCompPtr;
	public:

		MoveMathFunctionComponent();

		virtual ~MoveMathFunctionComponent();

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
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index);

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
			return NOWA::getIdFromName("MoveMathFunctionComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MoveMathFunctionComponent";
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
			return "Usage: Creates a mathematical function for physics active component movement.\n"
				"Example:\n"
				"XFunction0 = 20 - cos(t) * 20\n"
				"YFunction0 = 0\n"
				"ZFunction0 = sin(t) * 20\n"
				"MinLength0 = 0\n"
				"MaxLength0 = 2 * PI\n"
				"Speed0 = 5\n"
				"XFunction1 = -20 - cos(t) * -20\n"
				"YFunction1 = 0\n"
				"ZFunction1 = sin(t) * 10\n"
				"MinLength1 = 0\n"
				"MaxLength1 = 2 * PI\n"
				"Speed1 = 2\n\n"
				"Will move via functions0 the game object in a circle in x, z of radius 20 and when finished will move via functions1 the other direction an ellipse, forming a strange infinite curve.\n"
				"Note: 20 - ... * 20 should be used to offset the origin, so that the game object will not warp, but remains as origin.\n"
				"Other useful functions:\n"
				"XFunction0 = 10 - cos(t) * 10; YFunction0 = 1; ZFunction0 = 10 + sin(t) * 10 (screw function)"
				"Note: Functions behave different when using a kind of PhysicsActiveComponent, because not the absolute position can be taken, but the relative velocity or omega force must be used.";
		}

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

		/**
		 * @brief Sets the function count.
		 * @param[in] functionCount The function count to set
		 */
		void setFunctionCount(unsigned int functionCount);

		/**
		 * @brief Gets the function count
		 * @return functionCount The function count
		 */
		unsigned int getFunctionCount(void) const;

		void setAutoOrientate(bool autoOrientate);

		bool getAutoOrientate(void) const;

		/**
		* @brief			Sets the function for the x component of the move function vector
		* @param[in]		xFunction		The mathematical function for the x component of the vector
		* @note				Some example function:
		*					X-Function: cos(x * 0.1); Y-Function: sin(x * 0.1); Z-Function: 0
		*					X-Function: x * 0.01; Y-Function: 0; Z-Function: sin(x * 0.5); Speed: 20
		*/
		void setXFunction(unsigned int index, const Ogre::String& xFunction);

		Ogre::String getXFunction(unsigned int index) const;

		void setYFunction(unsigned int index, const Ogre::String& yFunction);

		Ogre::String getYFunction(unsigned int index) const;

		void setZFunction(unsigned int index, const Ogre::String& zFunction);

		Ogre::String getZFunction(unsigned int index) const;
		
		void setSpeed(unsigned int index, Ogre::Real speed);
		
		Ogre::Real getSpeed(unsigned int index) const;

		void setRotationAxis(unsigned int index, const Ogre::Vector3 rotationAxis);
		
		Ogre::Vector3 getRotationAxis(unsigned int index) const;
		
		void setMinLength(unsigned int index, const Ogre::String& minLength);
		
		Ogre::String getMinLength(unsigned int index) const;

		void setMaxLength(unsigned int index, const Ogre::String& maxLength);

		Ogre::String getMaxLength(unsigned int index) const;
		
		void setDirectionChange(bool directionChange);

		bool getDirectionChange(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void reactOnFunctionFinished(luabind::object closureFunction);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrDirectionChange(void) {return "Direction Change"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrAutoOrientate(void) { return "Auto Orientate"; }
		static const Ogre::String AttrFunctionCount(void) { return "Function Count"; }
		static const Ogre::String AttrXFunction(void) { return "X-Function "; }
		static const Ogre::String AttrYFunction(void) { return "Y-Function "; }
		static const Ogre::String AttrZFunction(void) { return "Z-Function "; }
		static const Ogre::String AttrMinLength(void) { return "Min Length "; }
		static const Ogre::String AttrMaxLength(void) { return "Max Length "; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrRotationAxis(void) { return "Rotation Axis"; }
	private:
		bool parseMathematicalFunction(void);
	private:
		Variant* activated;
		Variant* directionChange;
		Variant* repeat;
		Variant* autoOrientate;
		Variant* functionCount;
		std::vector<Variant*> xFunctions;
		std::vector<Variant*> yFunctions;
		std::vector<Variant*> zFunctions;
		std::vector<Variant*> minLengths;
		std::vector<Variant*> maxLengths;
		std::vector<Variant*> speeds;
		std::vector<Variant*> rotationAxes;

		Ogre::Real oppositeDir;
		Ogre::Real progress;
		bool internalDirectionChange;
		short round;
		size_t totalIndex;
		std::vector<FunctionParser> functionParser;
		PhysicsActiveComponent* physicsActiveComponent;
		PhysicsActiveKinematicComponent* physicsActiveKinematicComponent;
		Ogre::Vector3 originPosition;
		Ogre::Quaternion originOrientation;
		Ogre::Vector3 oldPosition;
		Ogre::Vector3 mathFunction;
		bool firstTime;
		IPathGoalObserver* pathGoalObserver;
	};

}; //namespace end

#endif