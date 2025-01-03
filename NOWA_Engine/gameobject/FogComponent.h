#ifndef FOG_COMPONENT_H
#define FOG_COMPONENT_H

#include "GameObjectComponent.h"
#include "shader/HlmsFog.h"

namespace NOWA
{
	class EXPORTED FogComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::FogComponent> FogCompPtr;
	public:
	
		FogComponent();

		virtual ~FogComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

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
			return NOWA::getIdFromName("FogComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "FogComponent";
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
			return "Usage: Creates fog.";
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

		void setFogMode(const Ogre::String& fogMode);

		Ogre::String getFogMode(void) const;

		void setColor(const Ogre::Vector3& color);

		Ogre::Vector3 getColor(void) const;

		void setExpDensity(Ogre::Real expDensity);

		Ogre::Real getExpDensity(void) const;

		void setLinearStart(Ogre::Real linearStart);

		Ogre::Real getLinearStart(void) const;

		void setLinearEnd(Ogre::Real linearEnd);

		Ogre::Real getLinearEnd(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrFogMode(void) { return "Fog Mode"; }
		static const Ogre::String AttrColor(void) { return "Color"; }
		static const Ogre::String AttrExpDensity(void) { return "Exponential Density"; }
		static const Ogre::String AttrLinearStart(void) { return "Linear Start"; }
		static const Ogre::String AttrLinearEnd(void) { return "Linear End"; }
	private:
		void createFog(void);
	private:
		HlmsFogListener* fogListener;
		Variant* activated;
		Variant* fogMode;
		Variant* color;
		Variant* expDensity;
		Variant* linearStart;
		Variant* linearEnd;
	};

}; //namespace end

#endif