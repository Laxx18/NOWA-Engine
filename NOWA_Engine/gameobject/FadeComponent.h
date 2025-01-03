#ifndef FADE_COMPONENT_H
#define FADE_COMPONENT_H

#include "GameObjectComponent.h"
#include "utilities/Interpolator.h"

namespace NOWA
{
	class FaderLuaProcess;

	class EXPORTED FadeComponent : public GameObjectComponent
	{
	public:
		friend class FaderLuaProcess;

		typedef boost::shared_ptr<NOWA::FadeComponent> FadeCompPtr;
	public:
	
		FadeComponent();

		virtual ~FadeComponent();

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
			return NOWA::getIdFromName("FadeComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "FadeComponent";
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
			return "Usage: Creates a fade effect and its also possible to react in lua script, if the screen fading has completed.";
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

		void setFadeMode(const Ogre::String& fadeMode);

		Ogre::String getFadeMode(void) const;

		void setDurationSec(Ogre::Real duration);

		Ogre::Real getDurationSec(void) const;

		void setEaseFunction(const Ogre::String& easeFunction);

		Ogre::String getEaseFunction(void) const;

		/**
		 * @brief Lua closure function gets called in order to react when the fading has completed.
		 * @param[in] closureFunction The closure function set.
		 */
		void reactOnFadeCompleted(luabind::object closureFunction);

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrFadeMode(void) { return "Fade Mode"; }
		static const Ogre::String AttrDuration(void) { return "Duration"; }
		static const Ogre::String AttrEaseFunction(void) { return "Ease Function"; }
	private:
		Variant* activated;
		Variant* fadeMode;
		Variant* duration;
		Variant* easeFunction;

		Interpolator::EaseFunctions selectedEaseFunction;
		luabind::object fadeCompletedClosureFunction;
		bool bIsInSimulation;
	};

}; //namespace end

#endif