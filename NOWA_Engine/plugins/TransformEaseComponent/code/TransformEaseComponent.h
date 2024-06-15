/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef TRANSFORMEASECOMPONENT_H
#define TRANSFORMEASECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class PhysicsComponent;

	/**
	  * @brief		This compontent is similiar to the @TransformComponent, but ease functions are used in order to make a smooth movement from a waypoint to another.
	  */
	class EXPORTED TransformEaseComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		enum EaseFunctions
		{
			EaseInSine,
			EaseOutSine,
			EaseInOutSine,
			EaseInQuad,
			EaseOutQuad,
			EaseInOutQuad,
			EaseInCubic,
			EaseOutCubic,
			EaseInOutCubic,
			EaseInQuart,
			EaseOutQuart,
			EaseInOutQuart,
			EaseInQuint,
			EaseOutQuint,
			EaseInOutQuint,
			EaseInExpo,
			EaseOutExpo,
			EaseInOutExpo,
			EaseInCirc,
			EaseOutCirc,
			EaseInOutCirc,
			EaseInBack,
			EaseOutBack,
			EaseInOutBack,
			EaseInElastic,
			EaseOutElastic,
			EaseInOutElastic,
			EaseInBounce,
			EaseOutBounce,
			EaseInOutBounce
		};
	public:
		typedef boost::shared_ptr<TransformEaseComponent> TransformEaseComponentPtr;
	public:

		TransformEaseComponent();

		virtual ~TransformEaseComponent();

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
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("TransformEaseComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "TransformEaseComponent";
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
			return "Usage: My usage text.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	private:
		Ogre::String name;

		void setRotationActivated(bool rotationActivated);

		bool isRotationActivated(void) const;

		void setRotationAxis(const Ogre::Vector3& rotationAxis);

		Ogre::Vector3 getRotationAxis(void) const;

		void setRotationMin(Ogre::Real rotationMin);

		Ogre::Real getRotationMin(void) const;

		void setRotationMax(Ogre::Real rotationMax);

		Ogre::Real getRotationMax(void) const;

		void setRotationDuration(Ogre::Real rotationDuration);

		Ogre::Real getRotationDuration(void) const;

		void setRotationRepeat(bool rotationRepeat);

		bool getRotationRepeat(void) const;

		void setRotationDirectionChange(bool rotationDirectionChange);

		bool getRotationDirectionChange(void) const;

		void setRotationEaseFunction(const Ogre::String& rotationEaseFunction);

		Ogre::String getRotationEaseFunction(void) const;

		void setTranslationActivated(bool rotationActivated);

		bool isTranslationActivated(void) const;

		void setTranslationAxis(const Ogre::Vector3& translationAxis);

		Ogre::Vector3 getTranslationAxis(void) const;

		void setTranslationMin(Ogre::Real translationMin);

		Ogre::Real getTranslationMin(void) const;

		void setTranslationMax(Ogre::Real translationMax);

		Ogre::Real getTranslationMax(void) const;

		void setTranslationDuration(Ogre::Real translationDuration);

		Ogre::Real getTranslationDuration(void) const;

		void setTranslationRepeat(bool translationRepeat);

		bool getTranslationRepeat(void) const;

		void setTranslationDirectionChange(bool translationDirectionChange);

		bool getTranslationDirectionChange(void) const;

		void setTranslationEaseFunction(const Ogre::String& translationEaseFunction);

		Ogre::String getTranslationEaseFunction(void) const;

		void setScaleActivated(bool rotationActivated);

		bool isScaleActivated(void) const;

		void setScaleAxis(const Ogre::Vector3& scaleAxis);

		Ogre::Vector3 getScaleAxis(void) const;

		void setScaleMin(Ogre::Real scaleMin);

		Ogre::Real getScaleMin(void) const;

		void setScaleMax(Ogre::Real scaleMax);

		Ogre::Real getScaleMax(void) const;

		void setScaleDuration(Ogre::Real scaleDuration);

		Ogre::Real getScaleDuration(void) const;

		void setScaleRepeat(bool scaleRepeat);

		bool getScaleRepeat(void) const;

		void setScaleDirectionChange(bool scaleDirectionChange);

		bool getScaleDirectionChange(void) const;

		void setScaleEaseFunction(const Ogre::String& scaleEaseFunction);

		Ogre::String getScaleEaseFunction(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRotationActivated(void) { return "Rotation Activated"; }
		static const Ogre::String AttrRotationAxis(void) { return "Rotation Axis"; }
		static const Ogre::String AttrRotationMin(void) { return "Rotation Min"; }
		static const Ogre::String AttrRotationMax(void) { return "Rotation Max"; }
		static const Ogre::String AttrRotationDuration(void) { return "Rotation Duration Sec"; }
		static const Ogre::String AttrRotationRepeat(void) { return "Rotation Repeat"; }
		static const Ogre::String AttrRotationDirectionChange(void) { return "Rotation Dir. Change"; }
		static const Ogre::String AttrRotationEaseFunction(void) { return "Rotation Ease Function"; }

		static const Ogre::String AttrTranslationActivated(void) { return "Translation Activated"; }
		static const Ogre::String AttrTranslationAxis(void) { return "Translation Axis"; }
		static const Ogre::String AttrTranslationMin(void) { return "Translation Min"; }
		static const Ogre::String AttrTranslationMax(void) { return "Translation Max"; }
		static const Ogre::String AttrTranslationDuration(void) { return "Translation Duration Sec"; }
		static const Ogre::String AttrTranslationRepeat(void) { return "Translation Repeat"; }
		static const Ogre::String AttrTranslationDirectionChange(void) { return "Translation Dir. Change"; }
		static const Ogre::String AttrTranslationEaseFunction(void) { return "Translation Ease Function"; }

		static const Ogre::String AttrScaleActivated(void) { return "Scale Activated"; }
		static const Ogre::String AttrScaleAxis(void) { return "Scale Axis"; }
		static const Ogre::String AttrScaleMin(void) { return "Scale Min"; }
		static const Ogre::String AttrScaleMax(void) { return "Scale Max"; }
		static const Ogre::String AttrScaleDuration(void) { return "Scale Duration Sec"; }
		static const Ogre::String AttrScaleRepeat(void) { return "Scale Repeat"; }
		static const Ogre::String AttrScaleDirectionChange(void) { return "Scale Dir. Change"; }
		static const Ogre::String AttrScaleEaseFunction(void) { return "Scale Ease Function"; }
	private:
		EaseFunctions mapStringToEaseFunctions(const Ogre::String strEaseFunction);

		Ogre::String mapEaseFunctionsToString(EaseFunctions easeFunctions);

		Ogre::Vector3 applyEaseFunction(const Ogre::Vector3& startPosition, const Ogre::Vector3& targetPosition, EaseFunctions easeFunctions, Ogre::Real time);

		Ogre::Real applyEaseFunction(Ogre::Real v1, Ogre::Real v2, EaseFunctions easeFunctions, Ogre::Real time);
	private:
		Variant* activated;
		Variant* rotationActivated;
		Variant* rotationAxis;
		Variant* rotationMin;
		Variant* rotationMax;
		Variant* rotationDuration;
		Variant* rotationRepeat;
		Variant* rotationDirectionChange;
		Variant* rotationEaseFunction;

		Variant* translationActivated;
		Variant* translationAxis;
		Variant* translationMin;
		Variant* translationMax;
		Variant* translationDuration;
		Variant* translationRepeat;
		Variant* translationDirectionChange;
		Variant* translationEaseFunction;

		Variant* scaleActivated;
		Variant* scaleAxis;
		Variant* scaleMin;
		Variant* scaleMax;
		Variant* scaleDuration;
		Variant* scaleRepeat;
		Variant* scaleDirectionChange;
		Variant* scaleEaseFunction;

		Ogre::Real rotationOppositeDir;
		Ogre::Real rotationProgress;
		short rotationRound;
		bool internalRotationDirectionChange;
		EaseFunctions rotationEaseFunctions;
		Ogre::Quaternion oldRotationResult;

		Ogre::Real translationOppositeDir;
		Ogre::Real translationProgress;
		short translationRound;
		bool internalTranslationDirectionChange;
		EaseFunctions translationEaseFunctions;
		Ogre::Vector3 oldTranslationResult;

		Ogre::Real scaleOppositeDir;
		Ogre::Real scaleProgress;
		short scaleRound;
		bool internalScaleDirectionChange;
		EaseFunctions scaleEaseFunctions;
		Ogre::Vector3 oldScaleResult;

		PhysicsComponent* physicsComponent;
	};

}; // namespace end

#endif

