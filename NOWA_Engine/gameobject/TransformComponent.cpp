#include "NOWAPrecompiled.h"
#include "TransformComponent.h"
#include "GameObjectController.h"
#include "PhysicsComponent.h"
#include "utilities/XMLConverter.h"
#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	TransformComponent::TransformComponent()
		: GameObjectComponent(),
		rotationOppositeDir(1.0f),
		rotationProgress(0.0f),
		rotationRound(0),
		translationOppositeDir(1.0f),
		translationProgress(0.0f),
		translationRound(0),
		scaleOppositeDir(1.0f),
		scaleProgress(0.0f),
		scaleRound(0),
		physicsComponent(nullptr),
		activated(new Variant(TransformComponent::AttrActivated(), true, this->attributes)),
		rotationActivated(new Variant(TransformComponent::AttrRotationActivated(), true, this->attributes)),
		rotationAxis(new Variant(TransformComponent::AttrRotationAxis(), Ogre::Vector3(0.0f, 1.0f, 0.0f), this->attributes)),
		rotationMin(new Variant(TransformComponent::AttrRotationMin(), 0.0f, this->attributes)),
		rotationMax(new Variant(TransformComponent::AttrRotationMax(), 180.0f, this->attributes)),
		rotationSpeed(new Variant(TransformComponent::AttrRotationSpeed(), 1.0f, this->attributes)),
		rotationRepeat(new Variant(TransformComponent::AttrRotationRepeat(), true, this->attributes)),
		rotationDirectionChange(new Variant(TransformComponent::AttrRotationDirectionChange(), true, this->attributes)),
		
		translationActivated(new Variant(TransformComponent::AttrTranslationActivated(), false, this->attributes)),
		translationAxis(new Variant(TransformComponent::AttrTranslationAxis(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes)),
		translationMin(new Variant(TransformComponent::AttrTranslationMin(), 0.0f, this->attributes)),
		translationMax(new Variant(TransformComponent::AttrTranslationMax(), 10.0f, this->attributes)),
		translationSpeed(new Variant(TransformComponent::AttrTranslationSpeed(), 1.0f, this->attributes)),
		translationRepeat(new Variant(TransformComponent::AttrTranslationRepeat(), true, this->attributes)),
		translationDirectionChange(new Variant(TransformComponent::AttrTranslationDirectionChange(), true, this->attributes)),
		
		scaleActivated(new Variant(TransformComponent::AttrScaleActivated(), false, this->attributes)),
		scaleAxis(new Variant(TransformComponent::AttrScaleAxis(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes)),
		scaleMin(new Variant(TransformComponent::AttrScaleMin(), 0.5f, this->attributes)),
		scaleMax(new Variant(TransformComponent::AttrScaleMax(), 10.0f, this->attributes)),
		scaleSpeed(new Variant(TransformComponent::AttrScaleSpeed(), 0.1f, this->attributes)),
		scaleRepeat(new Variant(TransformComponent::AttrScaleRepeat(), true, this->attributes)),
		scaleDirectionChange(new Variant(TransformComponent::AttrScaleDirectionChange(), true, this->attributes))
	{
		this->internalRotationDirectionChange = this->rotationDirectionChange->getBool();
		this->internalTranslationDirectionChange = this->translationDirectionChange->getBool();
		this->internalScaleDirectionChange = this->scaleDirectionChange->getBool();
	}

	TransformComponent::~TransformComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TransformComponent] Destructor transform component for game object: " + this->gameObjectPtr->getName());
		
	}

	bool TransformComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->setRotationSpeed(XMLConverter::getAttribReal(propertyElement, "data"));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TranslationSpeed")
		{
			this->setTranslationSpeed(XMLConverter::getAttribReal(propertyElement, "data"));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleSpeed")
		{
			this->setScaleSpeed(XMLConverter::getAttribReal(propertyElement, "data"));
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
		
		return true;
	}

	GameObjectCompPtr TransformComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TransformCompPtr clonedCompPtr(boost::make_shared<TransformComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRotationActivated(this->rotationActivated->getBool());
		clonedCompPtr->setRotationAxis(this->rotationAxis->getVector3());
		clonedCompPtr->setRotationMin(this->rotationMin->getReal());
		clonedCompPtr->setRotationMax(this->rotationMax->getReal());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setRotationRepeat(this->rotationRepeat->getBool());
		clonedCompPtr->setRotationDirectionChange(this->rotationDirectionChange->getBool());
		
		clonedCompPtr->setTranslationActivated(this->translationActivated->getBool());
		clonedCompPtr->setTranslationAxis(this->translationAxis->getVector3());
		clonedCompPtr->setTranslationMin(this->translationMin->getReal());
		clonedCompPtr->setTranslationMax(this->translationMax->getReal());
		clonedCompPtr->setTranslationSpeed(this->translationSpeed->getReal());
		clonedCompPtr->setTranslationRepeat(this->translationRepeat->getBool());
		clonedCompPtr->setTranslationDirectionChange(this->translationDirectionChange->getBool());
		
		clonedCompPtr->setScaleActivated(this->scaleActivated->getBool());
		clonedCompPtr->setScaleAxis(this->scaleAxis->getVector3());
		clonedCompPtr->setScaleMin(this->scaleMin->getReal());
		clonedCompPtr->setScaleMax(this->scaleMax->getReal());
		clonedCompPtr->setScaleSpeed(this->scaleSpeed->getReal());
		clonedCompPtr->setScaleRepeat(this->scaleRepeat->getBool());
		clonedCompPtr->setScaleDirectionChange(this->scaleDirectionChange->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool TransformComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TransformComponent] Init move math function component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}
	
	bool TransformComponent::connect(void)
	{
		this->rotationProgress = 0.0f;
		this->internalRotationDirectionChange = this->rotationDirectionChange->getBool();
		this->rotationRound = 0;
		this->rotationOppositeDir = 1.0f;
		
		this->translationProgress = 0.0f;
		this->internalTranslationDirectionChange = this->translationDirectionChange->getBool();
		this->translationRound = 0;
		this->translationOppositeDir = 1.0f;
		
		this->scaleProgress = 0.0f;
		this->internalScaleDirectionChange = this->scaleDirectionChange->getBool();
		this->scaleRound = 0;
		this->scaleOppositeDir = 1.0f;
		
		auto& physicsCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>());
		if (nullptr != physicsCompPtr)
		 	this->physicsComponent = physicsCompPtr.get();
		
		return true;
	}

	void TransformComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			///////////////////////////////////////////// Rotation ////////////////////////////////////////////////////////////////
			if (true == this->rotationActivated->getBool())
			{
				if (Ogre::Math::RealEqual(this->rotationOppositeDir, 1.0f))
				{
					this->rotationProgress += this->rotationSpeed->getReal() * dt;
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "rot progress: " + Ogre::StringConverter::toString(this->rotationProgress) +  " rot max: " 
					// 	+ Ogre::StringConverter::toString(this->rotationMax->getReal()));
					if (this->rotationProgress >= this->rotationMax->getReal())
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from +  to -");
						if (true == this->internalRotationDirectionChange)
						{
							this->rotationOppositeDir *= -1.0f;
						}
						this->rotationRound++;
					}
				}
				else
				{
					this->rotationProgress -= this->rotationSpeed->getReal() * dt;
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "rot progress: " + Ogre::StringConverter::toString(this->rotationProgress) + " min max: "
					// 	+ Ogre::StringConverter::toString(this->rotationMin->getReal()));
					if (this->rotationProgress <= this->rotationMin->getReal())
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
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
					// if repeat is of, only change the direction one time, to get back to its origin and leave
					if (false == this->rotationRepeat->getBool())
					{
						this->internalRotationDirectionChange = false;
					}
				}
				// when repeat is on, proceed
				// when direction change is on, proceed
				if (true == this->rotationRepeat->getBool() || true == this->internalRotationDirectionChange || 0 == this->rotationRound)
				{
					if (nullptr == this->physicsComponent)
						this->gameObjectPtr->getSceneNode()->rotate(Ogre::Quaternion(Ogre::Degree(this->rotationSpeed->getReal() * this->rotationOppositeDir), this->rotationAxis->getVector3()));
					else
						this->physicsComponent->rotate(Ogre::Quaternion(Ogre::Degree(this->rotationSpeed->getReal() * this->rotationOppositeDir), this->rotationAxis->getVector3()));
				}
			}

			///////////////////////////////////////////// Translation ////////////////////////////////////////////////////////////////
			if (true == this->translationActivated->getBool())
			{
				if (Ogre::Math::RealEqual(this->translationOppositeDir, 1.0f))
				{
					this->translationProgress += this->translationSpeed->getReal() * dt;
					if (this->translationProgress >= this->translationMax->getReal())
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from +  to -");
						if (true == this->internalTranslationDirectionChange)
						{
							this->translationOppositeDir *= -1.0f;
						}
						this->translationRound++;
					}
				}
				else
				{
					this->translationProgress -= this->translationSpeed->getReal() * dt;
					if (this->translationProgress <= this->translationMin->getReal())
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
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
					if (nullptr == this->physicsComponent)
					{
						// Do not forget dt, because its no physics
						this->gameObjectPtr->getSceneNode()->translate(this->translationAxis->getVector3() * (this->translationSpeed->getReal() * this->translationOppositeDir) * dt);
					}
					else
					{
						this->physicsComponent->translate(this->translationAxis->getVector3() * (this->translationSpeed->getReal() * this->translationOppositeDir));
					}
				}
			}
			///////////////////////////////////////////// Scale ////////////////////////////////////////////////////////////////
			if (true == this->scaleActivated->getBool())
			{
				if (Ogre::Math::RealEqual(this->scaleOppositeDir, 1.0f))
				{
					this->scaleProgress += this->scaleSpeed->getReal() * dt;
					if (this->scaleProgress >= this->scaleMax->getReal())
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from +  to -");
						if (true == this->internalScaleDirectionChange)
						{
							this->scaleOppositeDir *= -1.0f;
						}
						this->scaleRound++;
					}
				}
				else
				{
					this->scaleProgress -= this->scaleSpeed->getReal() * dt;
					if (this->scaleProgress <= this->scaleMin->getReal())
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
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
					if (nullptr == this->physicsComponent)
					{
						// Do not forget dt, because its no physics
						// scale relative directly does not work, Ogre bug??
						this->gameObjectPtr->getSceneNode()->setScale(this->gameObjectPtr->getSceneNode()->getScale() + (this->scaleAxis->getVector3() * this->scaleSpeed->getReal() * this->scaleOppositeDir * dt));
					}
					else
					{
						this->physicsComponent->setScale(this->gameObjectPtr->getSceneNode()->getScale() + (this->scaleAxis->getVector3() * this->scaleSpeed->getReal() * this->scaleOppositeDir));
					}
				}
			}
		}
	}

	void TransformComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TransformComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (TransformComponent::AttrRotationActivated() == attribute->getName())
		{
			this->setRotationActivated(attribute->getBool());
		}
		else if (TransformComponent::AttrRotationAxis() == attribute->getName())
		{
			this->setRotationAxis(attribute->getVector3());
		}
		else if (TransformComponent::AttrRotationMin() == attribute->getName())
		{
			this->setRotationMin(attribute->getReal());
		}
		else if (TransformComponent::AttrRotationMax() == attribute->getName())
		{
			this->setRotationMax(attribute->getReal());
		}
		else if (TransformComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (TransformComponent::AttrRotationRepeat() == attribute->getName())
		{
			this->setRotationRepeat(attribute->getBool());
		}
		else if (TransformComponent::AttrRotationDirectionChange() == attribute->getName())
		{
			this->setRotationDirectionChange(attribute->getBool());
		}
		
		else if (TransformComponent::AttrTranslationActivated() == attribute->getName())
		{
			this->setTranslationActivated(attribute->getBool());
		}
		else if (TransformComponent::AttrTranslationAxis() == attribute->getName())
		{
			this->setTranslationAxis(attribute->getVector3());
		}
		else if (TransformComponent::AttrTranslationMin() == attribute->getName())
		{
			this->setTranslationMin(attribute->getReal());
		}
		else if (TransformComponent::AttrTranslationMax() == attribute->getName())
		{
			this->setTranslationMax(attribute->getReal());
		}
		else if (TransformComponent::AttrTranslationSpeed() == attribute->getName())
		{
			this->setTranslationSpeed(attribute->getReal());
		}
		else if (TransformComponent::AttrTranslationRepeat() == attribute->getName())
		{
			this->setTranslationRepeat(attribute->getBool());
		}
		else if (TransformComponent::AttrTranslationDirectionChange() == attribute->getName())
		{
			this->setTranslationDirectionChange(attribute->getBool());
		}
		
		else if (TransformComponent::AttrScaleActivated() == attribute->getName())
		{
			this->setScaleActivated(attribute->getBool());
		}
		else if (TransformComponent::AttrScaleAxis() == attribute->getName())
		{
			this->setScaleAxis(attribute->getVector3());
		}
		else if (TransformComponent::AttrScaleMin() == attribute->getName())
		{
			this->setScaleMin(attribute->getReal());
		}
		else if (TransformComponent::AttrScaleMax() == attribute->getName())
		{
			this->setScaleMax(attribute->getReal());
		}
		else if (TransformComponent::AttrScaleSpeed() == attribute->getName())
		{
			this->setScaleSpeed(attribute->getReal());
		}
		else if (TransformComponent::AttrScaleRepeat() == attribute->getName())
		{
			this->setScaleRepeat(attribute->getBool());
		}
		else if (TransformComponent::AttrScaleDirectionChange() == attribute->getName())
		{
			this->setScaleDirectionChange(attribute->getBool());
		}
	}

	void TransformComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationActivated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationMin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationMin->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationMax"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationMax->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationDirectionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationActivated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationMin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationMin->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationMax"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationMax->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TranslationDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->translationDirectionChange->getBool())));
		propertiesXML->append_node(propertyXML);
		
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleActivated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleActivated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleMin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleMin->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleMax"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleMax->getReal())));
		propertiesXML->append_node(propertyXML);
	
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleRepeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scaleDirectionChange->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void TransformComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool TransformComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::String TransformComponent::getClassName(void) const
	{
		return "TransformComponent";
	}

	Ogre::String TransformComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TransformComponent::setRotationActivated(bool rotationActivated)
	{
		this->rotationActivated->setValue(rotationActivated);
	}

	bool TransformComponent::isRotationActivated(void) const
	{
		return this->rotationActivated->getBool();
	}
	
	void TransformComponent::setRotationAxis(const Ogre::Vector3& rotationAxis)
	{
		this->rotationAxis->setValue(rotationAxis);
	}
	
	Ogre::Vector3 TransformComponent::getRotationAxis(void) const
	{
		return this->rotationAxis->getVector3();
	}
	
	void TransformComponent::setRotationMin(Ogre::Real rotationMin)
	{
		if (rotationMin > this->rotationMax->getReal())
			rotationMin = this->rotationMax->getReal();
		this->rotationMin->setValue(rotationMin);
	}
	
	Ogre::Real TransformComponent::getRotationMin(void) const
	{
		return this->rotationMin->getReal();
	}
	
	void TransformComponent::setRotationMax(Ogre::Real rotationMax)
	{
		if (rotationMax < this->rotationMin->getReal())
			rotationMax = this->rotationMin->getReal();
		this->rotationMax->setValue(rotationMax);
	}
	
	Ogre::Real TransformComponent::getRotationMax(void) const
	{
		return this->rotationMax->getReal();
	}
	
	void TransformComponent::setRotationSpeed(Ogre::Real rotationSpeed)
	{
		this->rotationSpeed->setValue(rotationSpeed);
	}
	
	Ogre::Real TransformComponent::getRotationSpeed(void) const
	{
		return this->rotationSpeed->getReal();
	}
	
	void TransformComponent::setRotationRepeat(bool rotationRepeat)
	{
		this->rotationRepeat->setValue(rotationRepeat);
	}
	
	bool TransformComponent::getRotationRepeat(void) const
	{
		return this->rotationRepeat->getBool();
	}
		
	void TransformComponent::setRotationDirectionChange(bool rotationDirectionChange)
	{
		this->rotationDirectionChange->setValue(rotationDirectionChange);
		this->internalRotationDirectionChange = rotationDirectionChange;
	}
	
	bool TransformComponent::getRotationDirectionChange(void) const
	{
		return this->rotationDirectionChange->getBool();
	}
	
	
	void TransformComponent::setTranslationActivated(bool translationActivated)
	{
		this->translationActivated->setValue(translationActivated);
	}

	bool TransformComponent::isTranslationActivated(void) const
	{
		return this->translationActivated->getBool();
	}
		
	void TransformComponent::setTranslationAxis(const Ogre::Vector3& translationAxis)
	{
		this->translationAxis->setValue(translationAxis);
	}
	
	Ogre::Vector3 TransformComponent::getTranslationAxis(void) const
	{
		return this->translationAxis->getVector3();
	}
	
	void TransformComponent::setTranslationMin(Ogre::Real translationMin)
	{
		if (translationMin > this->translationMax->getReal())
			translationMin = this->translationMax->getReal();
		this->translationMin->setValue(translationMin);
	}
	
	Ogre::Real TransformComponent::getTranslationMin(void) const
	{
		return this->translationMin->getReal();
	}
	
	void TransformComponent::setTranslationMax(Ogre::Real translationMax)
	{
		if (translationMax < this->translationMin->getReal())
			translationMax = this->translationMin->getReal();
		this->translationMax->setValue(translationMax);
	}
	
	Ogre::Real TransformComponent::getTranslationMax(void) const
	{
		return this->translationMax->getReal();
	}
	
	void TransformComponent::setTranslationSpeed(Ogre::Real translationSpeed)
	{
		this->translationSpeed->setValue(translationSpeed);
	}
	
	Ogre::Real TransformComponent::getTranslationSpeed(void) const
	{
		return this->translationSpeed->getReal();
	}
		
	void TransformComponent::setTranslationRepeat(bool translationRepeat)
	{
		this->translationRepeat->setValue(translationRepeat);
	}
	
	bool TransformComponent::getTranslationRepeat(void) const
	{
		return this->translationRepeat->getBool();
	}
		
	void TransformComponent::setTranslationDirectionChange(bool translationDirectionChange)
	{
		this->translationDirectionChange->setValue(translationDirectionChange);
		this->internalTranslationDirectionChange = translationDirectionChange;
	}
	
	bool TransformComponent::getTranslationDirectionChange(void) const
	{
		return this->translationDirectionChange->getBool();
	}
	
	
	void TransformComponent::setScaleActivated(bool scaleActivated)
	{
		this->scaleActivated->setValue(scaleActivated);
	}

	bool TransformComponent::isScaleActivated(void) const
	{
		return this->translationActivated->getBool();
	}
	
	void TransformComponent::setScaleAxis(const Ogre::Vector3& scaleAxis)
	{
		this->scaleAxis->setValue(scaleAxis);
	}
	
	Ogre::Vector3 TransformComponent::getScaleAxis(void) const
	{
		return this->scaleAxis->getVector3();
	}
	
	void TransformComponent::setScaleMin(Ogre::Real scaleMin)
	{
		if (scaleMin > this->scaleMax->getReal())
			scaleMin = this->scaleMax->getReal();
		this->scaleMin->setValue(scaleMin);
	}
	
	Ogre::Real TransformComponent::getScaleMin(void) const
	{
		return this->scaleMin->getReal();
	}
	
	void TransformComponent::setScaleMax(Ogre::Real scaleMax)
	{
		if (scaleMax < this->scaleMin->getReal())
			scaleMax = this->scaleMin->getReal();
		this->scaleMax->setValue(scaleMax);
	}
	
	Ogre::Real TransformComponent::getScaleMax(void) const
	{
		return this->scaleMax->getReal();
	}
	
	void TransformComponent::setScaleSpeed(Ogre::Real scaleSpeed)
	{
		if (scaleSpeed > this->scaleMin->getReal())
		{
			scaleSpeed = 0.1f;
		}
		this->scaleSpeed->setValue(scaleSpeed);
	}
	
	Ogre::Real TransformComponent::getScaleSpeed(void) const
	{
		return this->scaleSpeed->getReal();
	}
		
	void TransformComponent::setScaleRepeat(bool scaleRepeat)
	{
		this->scaleRepeat->setValue(scaleRepeat);
	}
	
	bool TransformComponent::getScaleRepeat(void) const
	{
		return this->scaleRepeat->getBool();
	}
		
	void TransformComponent::setScaleDirectionChange(bool scaleDirectionChange)
	{
		this->scaleDirectionChange->setValue(scaleDirectionChange);
		this->internalScaleDirectionChange = scaleDirectionChange;
	}
	
	bool TransformComponent::getScaleDirectionChange(void) const
	{
		return this->scaleDirectionChange->getBool();
	}

}; // namespace end