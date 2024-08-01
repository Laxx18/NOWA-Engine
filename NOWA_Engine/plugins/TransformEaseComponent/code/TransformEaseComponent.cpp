#include "NOWAPrecompiled.h"
#include "TransformEaseComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	TransformEaseComponent::TransformEaseComponent()
		: GameObjectComponent(),
		rotationOppositeDir(1.0f),
		rotationProgress(0.0f),
		rotationRound(0),
		rotationEaseFunctions(Interpolator::EaseInSine),
		oldRotationResult(Ogre::Quaternion::IDENTITY),
		translationOppositeDir(1.0f),
		translationProgress(0.0f),
		translationRound(0),
		translationEaseFunctions(Interpolator::EaseInSine),
		oldTranslationResult(Ogre::Vector3::ZERO),
		scaleOppositeDir(1.0f),
		scaleProgress(0.0f),
		scaleRound(0),
		oldScaleResult(Ogre::Vector3::ZERO),
		physicsComponent(nullptr),
		scaleEaseFunctions(Interpolator::EaseInSine),
		activated(new Variant(TransformEaseComponent::AttrActivated(), true, this->attributes)),
		rotationActivated(new Variant(TransformEaseComponent::AttrRotationActivated(), true, this->attributes)),
		rotationAxis(new Variant(TransformEaseComponent::AttrRotationAxis(), Ogre::Vector3(0.0f, 1.0f, 0.0f), this->attributes)),
		rotationMin(new Variant(TransformEaseComponent::AttrRotationMin(), 0.0f, this->attributes)),
		rotationMax(new Variant(TransformEaseComponent::AttrRotationMax(), 180.0f, this->attributes)),
		rotationDuration(new Variant(TransformEaseComponent::AttrRotationDuration(), 1.0f, this->attributes)),
		rotationRepeat(new Variant(TransformEaseComponent::AttrRotationRepeat(), true, this->attributes)),
		rotationDirectionChange(new Variant(TransformEaseComponent::AttrRotationDirectionChange(), true, this->attributes)),
		rotationEaseFunction(new Variant(TransformEaseComponent::AttrRotationEaseFunction(), Interpolator::getInstance()->getAllEaseFunctionNames(), this->attributes)),

		translationActivated(new Variant(TransformEaseComponent::AttrTranslationActivated(), false, this->attributes)),
		translationAxis(new Variant(TransformEaseComponent::AttrTranslationAxis(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes)),
		translationMin(new Variant(TransformEaseComponent::AttrTranslationMin(), 0.0f, this->attributes)),
		translationMax(new Variant(TransformEaseComponent::AttrTranslationMax(), 10.0f, this->attributes)),
		translationDuration(new Variant(TransformEaseComponent::AttrTranslationDuration(), 1.0f, this->attributes)),
		translationRepeat(new Variant(TransformEaseComponent::AttrTranslationRepeat(), true, this->attributes)),
		translationDirectionChange(new Variant(TransformEaseComponent::AttrTranslationDirectionChange(), true, this->attributes)),
		translationEaseFunction(new Variant(TransformEaseComponent::AttrTranslationEaseFunction(), Interpolator::getInstance()->getAllEaseFunctionNames(), this->attributes)),

		scaleActivated(new Variant(TransformEaseComponent::AttrScaleActivated(), false, this->attributes)),
		scaleAxis(new Variant(TransformEaseComponent::AttrScaleAxis(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes)),
		scaleMin(new Variant(TransformEaseComponent::AttrScaleMin(), 0.5f, this->attributes)),
		scaleMax(new Variant(TransformEaseComponent::AttrScaleMax(), 10.0f, this->attributes)),
		scaleDuration(new Variant(TransformEaseComponent::AttrScaleDuration(), 2.0f, this->attributes)),
		scaleRepeat(new Variant(TransformEaseComponent::AttrScaleRepeat(), true, this->attributes)),
		scaleDirectionChange(new Variant(TransformEaseComponent::AttrScaleDirectionChange(), true, this->attributes)),
		scaleEaseFunction(new Variant(TransformEaseComponent::AttrScaleEaseFunction(), Interpolator::getInstance()->getAllEaseFunctionNames(), this->attributes))
	{
		this->internalRotationDirectionChange = this->rotationDirectionChange->getBool();
		this->internalTranslationDirectionChange = this->translationDirectionChange->getBool();
		this->internalScaleDirectionChange = this->scaleDirectionChange->getBool();

		// Duration einbauen! Wenn Wert erreicht, muss Objekt am Ziel sein!
		// D.h. Duration abhängig von Duration und easeFunction

		/*
		const duration = 750; // Animation duration in milliseconds
		 let animationStart = 0;
		 function animate(time: number) {
		   requestAnimationFrame(animate);

			 // reset start time for animation
			 if (pointsData.resetAnimationStart)
			 {
			   animationStart = time;
			   pointsData.resetAnimationStart = false;
			 }
			 // calculate animation progress
			 const t = (time - animationStart) / duration;
			 if (t >= 1)
			 {
			   animationStart = 0;
			   pointsData.playing = false;
			 }
			 // easing
			 pointsData.t = pointsData.origin + easeInOutQuad(t) * (pointsData.target - pointsData.origin);

		* */

	}

	TransformEaseComponent::~TransformEaseComponent(void)
	{
		
	}

	const Ogre::String& TransformEaseComponent::getName() const
	{
		return this->name;
	}

	void TransformEaseComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<TransformEaseComponent>(TransformEaseComponent::getStaticClassId(), TransformEaseComponent::getStaticClassName());
	}
	
	void TransformEaseComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool TransformEaseComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationActivated")
		{
			this->setRotationActivated(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationAxis")
		{
			this->setRotationAxis(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationMin")
		{
			this->setRotationMin(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationMax")
		{
			this->setRotationMax(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationDuration")
		{
			this->setRotationDuration(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationRepeat")
		{
			this->setRotationRepeat(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationDirectionChange")
		{
			this->setRotationDirectionChange(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationEaseFunction")
		{
			this->setRotationEaseFunction(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationActivated")
		{
			this->setTranslationActivated(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationAxis")
		{
			this->setTranslationAxis(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationMin")
		{
			this->setTranslationMin(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationMax")
		{
			this->setTranslationMax(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationDuration")
		{
			this->setTranslationDuration(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationRepeat")
		{
			this->setTranslationRepeat(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationDirectionChange")
		{
			this->setTranslationDirectionChange(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationEaseFunction")
		{
			this->setTranslationEaseFunction(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleActivated")
		{
			this->setScaleActivated(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleAxis")
		{
			this->setScaleAxis(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleMin")
		{
			this->setScaleMin(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleMax")
		{
			this->setScaleMax(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleDuration")
		{
			this->setScaleDuration(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleRepeat")
		{
			this->setScaleRepeat(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleDirectionChange")
		{
			this->setScaleDirectionChange(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleEaseFunction")
		{
			this->setScaleEaseFunction(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr TransformEaseComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TransformEaseComponentPtr clonedCompPtr(boost::make_shared<TransformEaseComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRotationActivated(this->rotationActivated->getBool());
		clonedCompPtr->setRotationAxis(this->rotationAxis->getVector3());
		clonedCompPtr->setRotationMin(this->rotationMin->getReal());
		clonedCompPtr->setRotationMax(this->rotationMax->getReal());
		clonedCompPtr->setRotationDuration(this->rotationDuration->getReal());
		clonedCompPtr->setRotationRepeat(this->rotationRepeat->getBool());
		clonedCompPtr->setRotationDirectionChange(this->rotationDirectionChange->getBool());
		clonedCompPtr->setRotationEaseFunction(this->rotationEaseFunction->getListSelectedValue());

		clonedCompPtr->setTranslationActivated(this->translationActivated->getBool());
		clonedCompPtr->setTranslationAxis(this->translationAxis->getVector3());
		clonedCompPtr->setTranslationMin(this->translationMin->getReal());
		clonedCompPtr->setTranslationMax(this->translationMax->getReal());
		clonedCompPtr->setTranslationDuration(this->translationDuration->getReal());
		clonedCompPtr->setTranslationRepeat(this->translationRepeat->getBool());
		clonedCompPtr->setTranslationDirectionChange(this->translationDirectionChange->getBool());
		clonedCompPtr->setTranslationEaseFunction(this->translationEaseFunction->getListSelectedValue());

		clonedCompPtr->setScaleActivated(this->scaleActivated->getBool());
		clonedCompPtr->setScaleAxis(this->scaleAxis->getVector3());
		clonedCompPtr->setScaleMin(this->scaleMin->getReal());
		clonedCompPtr->setScaleMax(this->scaleMax->getReal());
		clonedCompPtr->setScaleDuration(this->scaleDuration->getReal());
		clonedCompPtr->setScaleRepeat(this->scaleRepeat->getBool());
		clonedCompPtr->setScaleDirectionChange(this->scaleDirectionChange->getBool());
		clonedCompPtr->setScaleEaseFunction(this->scaleEaseFunction->getListSelectedValue());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool TransformEaseComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TransformEaseComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool TransformEaseComponent::connect(void)
	{
		this->rotationProgress = 0.0f;
		this->internalRotationDirectionChange = this->rotationDirectionChange->getBool();
		this->rotationRound = 0;
		this->rotationOppositeDir = 1.0f;
		this->oldRotationResult = this->gameObjectPtr->getOrientation();

		this->translationProgress = 0.0f;
		this->internalTranslationDirectionChange = this->translationDirectionChange->getBool();
		this->translationRound = 0;
		this->translationOppositeDir = 1.0f;
		// this->oldTranslationResult = Ogre::Vector3::ZERO;
		this->oldTranslationResult = this->gameObjectPtr->getPosition();

		this->scaleProgress = 0.0f;
		this->internalScaleDirectionChange = this->scaleDirectionChange->getBool();
		this->scaleRound = 0;
		this->scaleOppositeDir = 1.0f;
		this->oldScaleResult = this->gameObjectPtr->getScale();

		auto& physicsCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>());
		if (nullptr != physicsCompPtr)
		{
			this->physicsComponent = physicsCompPtr.get();
		}

		return true;
	}

	bool TransformEaseComponent::disconnect(void)
	{
		
		return true;
	}

	bool TransformEaseComponent::onCloned(void)
	{
		
		return true;
	}

	void TransformEaseComponent::onRemoveComponent(void)
	{
		
	}
	
	void TransformEaseComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void TransformEaseComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void TransformEaseComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			///////////////////////////////////////////// Rotation ////////////////////////////////////////////////////////////////
			if (true == this->rotationActivated->getBool())
			{
				if (Ogre::Math::RealEqual(this->rotationOppositeDir, 1.0f))
				{
					this->rotationProgress += dt;
					if (this->rotationProgress >= this->rotationDuration->getReal())
					{
						if (true == this->internalRotationDirectionChange)
						{
							this->rotationOppositeDir *= -1.0f;
						}
						this->rotationRound++;
					}
				}
				else
				{
					this->rotationProgress -= dt;
					if (this->rotationProgress <= 0.0f)
					{
						if (true == this->internalRotationDirectionChange)
						{
							this->rotationOppositeDir *= -1.0f;
						}
						this->rotationRound++;
					}
				}
				// if the translation took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
				// also take the progress into account, the translation started at zero and should stop at zero
				if (this->rotationRound == 2 && this->rotationProgress >= 0.0f)
				{
					// if repeat is off, only change the direction one time, to get back to its origin and leave
					if (false == this->rotationRepeat->getBool())
					{
						this->internalRotationDirectionChange = false;
					}
				}
				// when repeat is on, proceed
				// when direction change is on, proceed
				if (true == this->rotationRepeat->getBool() || true == this->internalRotationDirectionChange || 0 == this->rotationRound)
				{
					Ogre::Real t = this->rotationProgress / this->rotationDuration->getReal();
					Ogre::Real resultValue = Interpolator::getInstance()->applyEaseFunction(this->rotationMin->getReal(), this->rotationMax->getReal(), this->rotationEaseFunctions, t);

					if (nullptr == this->physicsComponent)
						this->gameObjectPtr->getSceneNode()->setOrientation(this->oldRotationResult * Ogre::Quaternion(Ogre::Degree(resultValue), this->rotationAxis->getVector3()));
					else
						this->physicsComponent->setOrientation(this->oldRotationResult * Ogre::Quaternion(Ogre::Degree(resultValue), this->rotationAxis->getVector3()));
				}
			}

			///////////////////////////////////////////// Translation ////////////////////////////////////////////////////////////////
			if (true == this->translationActivated->getBool())
			{
				if (Ogre::Math::RealEqual(this->translationOppositeDir, 1.0f))
				{
					this->translationProgress += dt;

					if (this->translationProgress >= this->translationDuration->getReal())
					{
						if (true == this->internalTranslationDirectionChange)
						{
							this->translationOppositeDir *= -1.0f;
						}
						this->translationRound++;
					}
				}
				else
				{
					this->translationProgress -= dt;

					if (this->translationProgress <= 0.0f)
					{
						if (true == this->internalTranslationDirectionChange)
						{
							this->translationOppositeDir *= -1.0f;
						}
						this->translationRound++;
					}
				}
				// if the translation took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
				// also take the progress into account, the translation started at zero and should stop at zero
				if (this->translationRound == 2 && this->translationProgress >= 0.0f)
				{
					// if repeat is of, only change the direction one time, to get back to its origin and leave
					if (false == this->translationRepeat->getBool())
					{
						this->internalTranslationDirectionChange = false;
					}
				}
				// when repeat is on, proceed
				// when direction change is on, proceed
				if (true == this->translationRepeat->getBool() || true == this->internalTranslationDirectionChange || 0 == this->translationRound)
				{
					Ogre::Real t = this->translationProgress / this->translationDuration->getReal();
					Ogre::Vector3 resultVec = Interpolator::getInstance()->applyEaseFunction(this->translationAxis->getVector3() * this->translationMin->getReal(), this->translationAxis->getVector3() * this->translationMax->getReal(), this->translationEaseFunctions, t);

					if (nullptr == this->physicsComponent)
					{
						 this->gameObjectPtr->getSceneNode()->setPosition(this->oldTranslationResult + resultVec);
					}
					else
					{
						this->physicsComponent->setPosition(this->oldTranslationResult + resultVec);
					}

					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "transProgress: " + Ogre::StringConverter::toString(this->translationProgress) + " time: " + Ogre::StringConverter::toString(t) + " pos: "
							// 										+ Ogre::StringConverter::toString(this->gameObjectPtr->getPosition()) + " resultVec: " + Ogre::StringConverter::toString(resultVec));
				}
			}
			///////////////////////////////////////////// Scale ////////////////////////////////////////////////////////////////
			if (true == this->scaleActivated->getBool())
			{
				if (Ogre::Math::RealEqual(this->scaleOppositeDir, 1.0f))
				{
					this->scaleProgress += dt;
					if (this->scaleProgress >= this->scaleDuration->getReal())
					{
						if (true == this->internalScaleDirectionChange)
						{
							this->scaleOppositeDir *= -1.0f;
						}
						this->scaleRound++;
					}
				}
				else
				{
					this->scaleProgress -= dt;
					if (this->scaleProgress <= 0.0f)
					{
						if (true == this->internalScaleDirectionChange)
						{
							this->scaleOppositeDir *= -1.0f;
						}
						this->scaleRound++;
					}
				}
				// if the translation took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
				// also take the progress into account, the translation started at zero and should stop at zero
				if (this->scaleRound == 2 && this->scaleProgress >= 0.0f)
				{
					// if repeat is of, only change the direction one time, to get back to its origin and leave
					if (false == this->scaleRepeat->getBool())
					{
						this->internalScaleDirectionChange = false;
					}
				}
				// when repeat is on, proceed
				// when direction change is on, proceed
				if (true == this->scaleRepeat->getBool() || true == this->internalScaleDirectionChange || 0 == this->scaleRound)
				{
					Ogre::Real t = this->scaleProgress / this->scaleDuration->getReal();
					Ogre::Vector3 resultVec = Interpolator::getInstance()->applyEaseFunction(this->scaleAxis->getVector3() * this->scaleMin->getReal(), this->scaleAxis->getVector3() * this->scaleMax->getReal(), this->scaleEaseFunctions, t);

					if (nullptr == this->physicsComponent)
					{
						this->gameObjectPtr->getSceneNode()->setScale(this->oldScaleResult + resultVec);
					}
					else
					{
						this->physicsComponent->setScale(this->oldScaleResult + resultVec);
					}
				}
			}
		}
	}

	void TransformEaseComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TransformEaseComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrRotationActivated() == attribute->getName())
		{
			this->setRotationActivated(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrRotationAxis() == attribute->getName())
		{
			this->setRotationAxis(attribute->getVector3());
		}
		else if (TransformEaseComponent::AttrRotationMin() == attribute->getName())
		{
			this->setRotationMin(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrRotationMax() == attribute->getName())
		{
			this->setRotationMax(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrRotationDuration() == attribute->getName())
		{
			this->setRotationDuration(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrRotationRepeat() == attribute->getName())
		{
			this->setRotationRepeat(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrRotationDirectionChange() == attribute->getName())
		{
			this->setRotationDirectionChange(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrRotationEaseFunction() == attribute->getName())
		{
			this->setRotationEaseFunction(attribute->getListSelectedValue());
		}
		else if (TransformEaseComponent::AttrTranslationActivated() == attribute->getName())
		{
			this->setTranslationActivated(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrTranslationAxis() == attribute->getName())
		{
			this->setTranslationAxis(attribute->getVector3());
		}
		else if (TransformEaseComponent::AttrTranslationMin() == attribute->getName())
		{
			this->setTranslationMin(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrTranslationMax() == attribute->getName())
		{
			this->setTranslationMax(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrTranslationDuration() == attribute->getName())
		{
			this->setTranslationDuration(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrTranslationRepeat() == attribute->getName())
		{
			this->setTranslationRepeat(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrTranslationDirectionChange() == attribute->getName())
		{
			this->setTranslationDirectionChange(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrTranslationEaseFunction() == attribute->getName())
		{
			this->setTranslationEaseFunction(attribute->getListSelectedValue());
		}

		else if (TransformEaseComponent::AttrScaleActivated() == attribute->getName())
		{
			this->setScaleActivated(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrScaleAxis() == attribute->getName())
		{
			this->setScaleAxis(attribute->getVector3());
		}
		else if (TransformEaseComponent::AttrScaleMin() == attribute->getName())
		{
			this->setScaleMin(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrScaleMax() == attribute->getName())
		{
			this->setScaleMax(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrScaleDuration() == attribute->getName())
		{
			this->setScaleDuration(attribute->getReal());
		}
		else if (TransformEaseComponent::AttrScaleRepeat() == attribute->getName())
		{
			this->setScaleRepeat(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrScaleDirectionChange() == attribute->getName())
		{
			this->setScaleDirectionChange(attribute->getBool());
		}
		else if (TransformEaseComponent::AttrScaleEaseFunction() == attribute->getName())
		{
			this->setScaleEaseFunction(attribute->getListSelectedValue());
		}
	}

	void TransformEaseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationActivated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationMin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationMin->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationMax"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationMax->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationDuration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationDuration->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationDirectionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationEaseFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationEaseFunction->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationActivated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationMin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationMin->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationMax"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationMax->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationDuration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationDuration->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationDirectionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationEaseFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->translationEaseFunction->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);



		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleActivated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleMin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleMin->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleMax"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleMax->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleDuration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleDuration->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleDirectionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleEaseFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleEaseFunction->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	void TransformEaseComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool TransformEaseComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::String TransformEaseComponent::getClassName(void) const
	{
		return "TransformEaseComponent";
	}

	Ogre::String TransformEaseComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TransformEaseComponent::setRotationActivated(bool rotationActivated)
	{
		this->rotationActivated->setValue(rotationActivated);
	}

	bool TransformEaseComponent::isRotationActivated(void) const
	{
		return this->rotationActivated->getBool();
	}

	void TransformEaseComponent::setRotationAxis(const Ogre::Vector3& rotationAxis)
	{
		this->rotationAxis->setValue(rotationAxis);
	}

	Ogre::Vector3 TransformEaseComponent::getRotationAxis(void) const
	{
		return this->rotationAxis->getVector3();
	}

	void TransformEaseComponent::setRotationMin(Ogre::Real rotationMin)
	{
		if (rotationMin > this->rotationMax->getReal())
			rotationMin = this->rotationMax->getReal();
		this->rotationMin->setValue(rotationMin);
	}

	Ogre::Real TransformEaseComponent::getRotationMin(void) const
	{
		return this->rotationMin->getReal();
	}

	void TransformEaseComponent::setRotationMax(Ogre::Real rotationMax)
	{
		if (rotationMax < this->rotationMin->getReal())
			rotationMax = this->rotationMin->getReal();
		this->rotationMax->setValue(rotationMax);
	}

	Ogre::Real TransformEaseComponent::getRotationMax(void) const
	{
		return this->rotationMax->getReal();
	}

	void TransformEaseComponent::setRotationDuration(Ogre::Real rotationDuration)
	{
		this->rotationDuration->setValue(rotationDuration);
	}

	Ogre::Real TransformEaseComponent::getRotationDuration(void) const
	{
		return this->rotationDuration->getReal();
	}

	void TransformEaseComponent::setRotationRepeat(bool rotationRepeat)
	{
		this->rotationRepeat->setValue(rotationRepeat);
	}

	bool TransformEaseComponent::getRotationRepeat(void) const
	{
		return this->rotationRepeat->getBool();
	}

	void TransformEaseComponent::setRotationDirectionChange(bool rotationDirectionChange)
	{
		this->rotationDirectionChange->setValue(rotationDirectionChange);
		this->internalRotationDirectionChange = rotationDirectionChange;
	}

	bool TransformEaseComponent::getRotationDirectionChange(void) const
	{
		return this->rotationDirectionChange->getBool();
	}

	void TransformEaseComponent::setRotationEaseFunction(const Ogre::String& rotationEaseFunction)
	{
		this->rotationEaseFunction->setListSelectedValue(rotationEaseFunction);
		this->rotationEaseFunctions = Interpolator::getInstance()->mapStringToEaseFunctions(rotationEaseFunction);
	}

	Ogre::String TransformEaseComponent::getRotationEaseFunction(void) const
	{
		return this->rotationEaseFunction->getListSelectedValue();
	}

	void TransformEaseComponent::setTranslationActivated(bool translationActivated)
	{
		this->translationActivated->setValue(translationActivated);
	}

	bool TransformEaseComponent::isTranslationActivated(void) const
	{
		return this->translationActivated->getBool();
	}

	void TransformEaseComponent::setTranslationAxis(const Ogre::Vector3& translationAxis)
	{
		this->translationAxis->setValue(translationAxis);
	}

	Ogre::Vector3 TransformEaseComponent::getTranslationAxis(void) const
	{
		return this->translationAxis->getVector3();
	}

	void TransformEaseComponent::setTranslationMin(Ogre::Real translationMin)
	{
		if (translationMin > this->translationMax->getReal())
			translationMin = this->translationMax->getReal();
		this->translationMin->setValue(translationMin);
	}

	Ogre::Real TransformEaseComponent::getTranslationMin(void) const
	{
		return this->translationMin->getReal();
	}

	void TransformEaseComponent::setTranslationMax(Ogre::Real translationMax)
	{
		if (translationMax < this->translationMin->getReal())
			translationMax = this->translationMin->getReal();
		this->translationMax->setValue(translationMax);
	}

	Ogre::Real TransformEaseComponent::getTranslationMax(void) const
	{
		return this->translationMax->getReal();
	}

	void TransformEaseComponent::setTranslationDuration(Ogre::Real translationDuration)
	{
		this->translationDuration->setValue(translationDuration);
	}

	Ogre::Real TransformEaseComponent::getTranslationDuration(void) const
	{
		return this->translationDuration->getReal();
	}

	void TransformEaseComponent::setTranslationRepeat(bool translationRepeat)
	{
		this->translationRepeat->setValue(translationRepeat);
	}

	bool TransformEaseComponent::getTranslationRepeat(void) const
	{
		return this->translationRepeat->getBool();
	}

	void TransformEaseComponent::setTranslationDirectionChange(bool translationDirectionChange)
	{
		this->translationDirectionChange->setValue(translationDirectionChange);
		this->internalTranslationDirectionChange = translationDirectionChange;
	}

	bool TransformEaseComponent::getTranslationDirectionChange(void) const
	{
		return this->translationDirectionChange->getBool();
	}

	void TransformEaseComponent::setTranslationEaseFunction(const Ogre::String& translationEaseFunction)
	{
		this->translationEaseFunction->setListSelectedValue(translationEaseFunction);
		this->translationEaseFunctions = Interpolator::getInstance()->mapStringToEaseFunctions(translationEaseFunction);
	}

	Ogre::String TransformEaseComponent::getTranslationEaseFunction(void) const
	{
		return this->translationEaseFunction->getListSelectedValue();
	}

	void TransformEaseComponent::setScaleActivated(bool scaleActivated)
	{
		this->scaleActivated->setValue(scaleActivated);
	}

	bool TransformEaseComponent::isScaleActivated(void) const
	{
		return this->translationActivated->getBool();
	}

	void TransformEaseComponent::setScaleAxis(const Ogre::Vector3& scaleAxis)
	{
		this->scaleAxis->setValue(scaleAxis);
	}

	Ogre::Vector3 TransformEaseComponent::getScaleAxis(void) const
	{
		return this->scaleAxis->getVector3();
	}

	void TransformEaseComponent::setScaleMin(Ogre::Real scaleMin)
	{
		if (scaleMin > this->scaleMax->getReal())
			scaleMin = this->scaleMax->getReal();
		this->scaleMin->setValue(scaleMin);
	}

	Ogre::Real TransformEaseComponent::getScaleMin(void) const
	{
		return this->scaleMin->getReal();
	}

	void TransformEaseComponent::setScaleMax(Ogre::Real scaleMax)
	{
		if (scaleMax < this->scaleMin->getReal())
			scaleMax = this->scaleMin->getReal();
		this->scaleMax->setValue(scaleMax);
	}

	Ogre::Real TransformEaseComponent::getScaleMax(void) const
	{
		return this->scaleMax->getReal();
	}

	void TransformEaseComponent::setScaleDuration(Ogre::Real scaleDuration)
	{
		// WTF?
		/*if (scaleDuration > this->scaleMin->getReal())
		{
			scaleDuration = 0.1f;
		}*/
		this->scaleDuration->setValue(scaleDuration);
	}

	Ogre::Real TransformEaseComponent::getScaleDuration(void) const
	{
		return this->scaleDuration->getReal();
	}

	void TransformEaseComponent::setScaleRepeat(bool scaleRepeat)
	{
		this->scaleRepeat->setValue(scaleRepeat);
	}

	bool TransformEaseComponent::getScaleRepeat(void) const
	{
		return this->scaleRepeat->getBool();
	}

	void TransformEaseComponent::setScaleDirectionChange(bool scaleDirectionChange)
	{
		this->scaleDirectionChange->setValue(scaleDirectionChange);
		this->internalScaleDirectionChange = scaleDirectionChange;
	}

	bool TransformEaseComponent::getScaleDirectionChange(void) const
	{
		return this->scaleDirectionChange->getBool();
	}

	void TransformEaseComponent::setScaleEaseFunction(const Ogre::String& scaleEaseFunction)
	{
		this->scaleEaseFunction->setListSelectedValue(scaleEaseFunction);
		this->scaleEaseFunctions = Interpolator::getInstance()->mapStringToEaseFunctions(scaleEaseFunction);
	}

	Ogre::String TransformEaseComponent::getScaleEaseFunction(void) const
	{
		return this->scaleEaseFunction->getListSelectedValue();
	}

	// Lua registration part

	TransformEaseComponent* getTransformEaseComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TransformEaseComponent>(gameObject->getComponentWithOccurrence<TransformEaseComponent>(occurrenceIndex)).get();
	}

	TransformEaseComponent* getTransformEaseComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TransformEaseComponent>(gameObject->getComponent<TransformEaseComponent>()).get();
	}

	TransformEaseComponent* getTransformEaseComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TransformEaseComponent>(gameObject->getComponentFromName<TransformEaseComponent>(name)).get();
	}

	void TransformEaseComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<TransformEaseComponent, GameObjectComponent>("TransformEaseComponent")
			.def("setActivated", &TransformEaseComponent::setActivated)
			.def("isActivated", &TransformEaseComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("TransformEaseComponent", "class inherits GameObjectComponent", TransformEaseComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("TransformEaseComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformEaseComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getTransformEaseComponentFromName", &getTransformEaseComponentFromName);
		gameObjectClass.def("getTransformEaseComponent", (TransformEaseComponent * (*)(GameObject*)) & getTransformEaseComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getTransformEaseComponent2", (TransformEaseComponent * (*)(GameObject*, unsigned int)) & getTransformEaseComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TransformEaseComponent getTransformEaseComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TransformEaseComponent getTransformEaseComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TransformEaseComponent getTransformEaseComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castTransformEaseComponent", &GameObjectController::cast<TransformEaseComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "TransformEaseComponent castTransformEaseComponent(TransformEaseComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool TransformEaseComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end