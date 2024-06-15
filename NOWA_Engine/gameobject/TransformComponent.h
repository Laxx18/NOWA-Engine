#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class PhysicsComponent;

	/*
	 * @brief This component can be used to transform (translate and/or rotate and/or scale) a game object, like a rotating coin etc.
	 */

	class EXPORTED TransformComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::TransformComponent> TransformCompPtr;
	public:

		TransformComponent();

		virtual ~TransformComponent();

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

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("TransformComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "TransformComponent";
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
			return "Usage: This component can be used to transform (translate and/or rotate and/or scale) a game object, like a rotating coin etc.";
		}

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

		virtual bool isActivated(void) const override;

		void setRotationActivated(bool rotationActivated);

		bool isRotationActivated(void) const;
		
		void setRotationAxis(const Ogre::Vector3& rotationAxis);

		Ogre::Vector3 getRotationAxis(void) const;

		void setRotationMin(Ogre::Real rotationMin);

		Ogre::Real getRotationMin(void) const;

		void setRotationMax(Ogre::Real rotationMax);

		Ogre::Real getRotationMax(void) const;

		void setRotationSpeed(Ogre::Real rotationSpeed);

		Ogre::Real getRotationSpeed(void) const;
		
		void setRotationRepeat(bool rotationRepeat);

		bool getRotationRepeat(void) const;
		
		void setRotationDirectionChange(bool rotationDirectionChange);

		bool getRotationDirectionChange(void) const;
		
		void setTranslationActivated(bool rotationActivated);

		bool isTranslationActivated(void) const;
		
		void setTranslationAxis(const Ogre::Vector3& translationAxis);

		Ogre::Vector3 getTranslationAxis(void) const;
		
		void setTranslationMin(Ogre::Real translationMin);

		Ogre::Real getTranslationMin(void) const;

		void setTranslationMax(Ogre::Real translationMax);

		Ogre::Real getTranslationMax(void) const;

		void setTranslationSpeed(Ogre::Real translationSpeed);

		Ogre::Real getTranslationSpeed(void) const;
		
		void setTranslationRepeat(bool translationRepeat);

		bool getTranslationRepeat(void) const;
		
		void setTranslationDirectionChange(bool translationDirectionChange);

		bool getTranslationDirectionChange(void) const;
		
		void setScaleActivated(bool rotationActivated);

		bool isScaleActivated(void) const;
		
		void setScaleAxis(const Ogre::Vector3& scaleAxis);

		Ogre::Vector3 getScaleAxis(void) const;
		
		void setScaleMin(Ogre::Real scaleMin);

		Ogre::Real getScaleMin(void) const;

		void setScaleMax(Ogre::Real scaleMax);

		Ogre::Real getScaleMax(void) const;

		void setScaleSpeed(Ogre::Real scaleSpeed);

		Ogre::Real getScaleSpeed(void) const;
		
		void setScaleRepeat(bool scaleRepeat);

		bool getScaleRepeat(void) const;
		
		void setScaleDirectionChange(bool scaleDirectionChange);

		bool getScaleDirectionChange(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrRotationActivated(void) { return "Rotation Activated"; }
		static const Ogre::String AttrRotationAxis(void) { return "Rotation Axis"; }
		static const Ogre::String AttrRotationMin(void) { return "Rotation Min"; }
		static const Ogre::String AttrRotationMax(void) { return "Rotation Max"; }
		static const Ogre::String AttrRotationSpeed(void) { return "Rotation Speed"; }
		static const Ogre::String AttrRotationRepeat(void) { return "Rotation Repeat"; }
		static const Ogre::String AttrRotationDirectionChange(void) { return "Rotation Dir. Change"; }
		
		static const Ogre::String AttrTranslationActivated(void) { return "Translation Activated"; }
		static const Ogre::String AttrTranslationAxis(void) { return "Translation Axis"; }
		static const Ogre::String AttrTranslationMin(void) { return "Translation Min"; }
		static const Ogre::String AttrTranslationMax(void) { return "Translation Max"; }
		static const Ogre::String AttrTranslationSpeed(void) { return "Translation Speed"; }
		static const Ogre::String AttrTranslationRepeat(void) { return "Translation Repeat"; }
		static const Ogre::String AttrTranslationDirectionChange(void) { return "Translation Dir. Change"; }
		
		static const Ogre::String AttrScaleActivated(void) { return "Scale Activated"; }
		static const Ogre::String AttrScaleAxis(void) { return "Scale Axis"; }
		static const Ogre::String AttrScaleMin(void) { return "Scale Min"; }
		static const Ogre::String AttrScaleMax(void) { return "Scale Max"; }
		static const Ogre::String AttrScaleSpeed(void) { return "Scale Speed"; }
		static const Ogre::String AttrScaleRepeat(void) { return "Scale Repeat"; }
		static const Ogre::String AttrScaleDirectionChange(void) { return "Scale Dir. Change"; }
	private:
		Variant* activated;
		Variant* rotationActivated;
		Variant* rotationAxis;
		Variant* rotationMin;
		Variant* rotationMax;
		Variant* rotationSpeed;
		Variant* rotationRepeat;
		Variant* rotationDirectionChange;
		
		Variant* translationActivated;
		Variant* translationAxis;
		Variant* translationMin;
		Variant* translationMax;
		Variant* translationSpeed;
		Variant* translationRepeat;
		Variant* translationDirectionChange;
		
		Variant* scaleActivated;
		Variant* scaleAxis;
		Variant* scaleMin;
		Variant* scaleMax;
		Variant* scaleSpeed;
		Variant* scaleRepeat;
		Variant* scaleDirectionChange;
		
		Ogre::Real rotationOppositeDir;
		Ogre::Real rotationProgress;
		short rotationRound;
		bool internalRotationDirectionChange;
		
		Ogre::Real translationOppositeDir;
		Ogre::Real translationProgress;
		short translationRound;
		bool internalTranslationDirectionChange;
		
		Ogre::Real scaleOppositeDir;
		Ogre::Real scaleProgress;
		short scaleRound;
		bool internalScaleDirectionChange;

		PhysicsComponent* physicsComponent;
	};

}; //namespace end

#endif