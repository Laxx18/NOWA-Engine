#include "NOWAPrecompiled.h"
#include "AttributeEffectComponent.h"
#include "physicsActiveComponent.h"
#include "GameObjectController.h"
#include "LuaScriptComponent.h"
#include "utilities/XMLConverter.h"
#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AttributeEffectComponent::AttributeEffectObserver::AttributeEffectObserver(luabind::object closureFunction, bool oneTime)
		: IAttributeEffectObserver(),
		closureFunction(closureFunction),
		oneTime(oneTime)
	{
		
	}

	AttributeEffectComponent::AttributeEffectObserver::~AttributeEffectObserver()
	{

	}

	void AttributeEffectComponent::AttributeEffectObserver::onEffect(const Ogre::Vector3& functionResult)
	{
		if (this->closureFunction.is_valid())
		{
			try
			{
				luabind::call_function<void>(this->closureFunction);
			}
			catch (luabind::error& error)
			{
				luabind::object errorMsg(luabind::from_stack(error.state(), -1));
				std::stringstream msg;
				msg << errorMsg;

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnEffect' Error: " + Ogre::String(error.what())
					+ " details: " + msg.str());
			}
		}
	}

	bool AttributeEffectComponent::AttributeEffectObserver::shouldReactOneTime(void) const
	{
		return this->oneTime;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	AttributeEffectComponent::AttributeEffectComponent()
		: GameObjectComponent(),
		oppositeDir(1.0f),
		progress(0.0f),
		round(0),
		attributeEffect(nullptr),
		oldAttributeEffect(nullptr),
		targetComponent(nullptr),
		notificationValue(0.0f),
		attributeEffectObserver(nullptr),
		activated(new Variant(AttributeEffectComponent::AttrActivated(), true, this->attributes)),
		attributeName(new Variant(AttributeEffectComponent::AttrAttributeName(), "", this->attributes)),
		xFunction(new Variant(AttributeEffectComponent::AttrXFunction(), "cos(t)", this->attributes)),
		yFunction(new Variant(AttributeEffectComponent::AttrYFunction(), "sin(t)", this->attributes)),
		zFunction(new Variant(AttributeEffectComponent::AttrZFunction(), "0", this->attributes)),
		minLength(new Variant(AttributeEffectComponent::AttrMinLength(), "0", this->attributes)),
		maxLength(new Variant(AttributeEffectComponent::AttrMaxLength(), "2 * PI", this->attributes)),
		speed(new Variant(AttributeEffectComponent::AttrSpeed(), 1.0f, this->attributes)),
		directionChange(new Variant(AttributeEffectComponent::AttrDirectionChange(), false, this->attributes)),
		repeat(new Variant(AttributeEffectComponent::AttrRepeat(), true, this->attributes))
	{
		this->internalDirectionChange = this->directionChange->getBool();
		this->attributeName->setDescription("Sets the attribute name, that should be manipulated by a mathematical function. It will be used on the next prior component or game object.");
	}

	AttributeEffectComponent::~AttributeEffectComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributeEffectComponent] Destructor attribute effect component for game object: " + this->gameObjectPtr->getName());
		
		if (nullptr != this->oldAttributeEffect)
		{
			delete this->oldAttributeEffect;
			this->oldAttributeEffect = nullptr;
		}
		if (nullptr != this->attributeEffectObserver)
		{
			delete this->attributeEffectObserver;
			this->attributeEffectObserver = nullptr;
		}
	}

	bool AttributeEffectComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttributeName")
		{
			this->attributeName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "XFunction")
		{
			this->xFunction->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "YFunction")
		{
			this->yFunction->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ZFunction")
		{
			this->zFunction->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinLength")
		{
			this->minLength->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxLength")
		{
			this->maxLength->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed")
		{
			this->speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr AttributeEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AttributeEffectCompPtr clonedCompPtr(boost::make_shared<AttributeEffectComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setAttributeName(this->attributeName->getString());
		clonedCompPtr->setXFunction(this->xFunction->getString());
		clonedCompPtr->setYFunction(this->yFunction->getString());
		clonedCompPtr->setZFunction(this->zFunction->getString());
		clonedCompPtr->setMinLength(this->minLength->getString());
		clonedCompPtr->setMaxLength(this->maxLength->getString());
		clonedCompPtr->setSpeed(this->speed->getReal());
		clonedCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedCompPtr->setRepeat(this->repeat->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AttributeEffectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributeEffectComponent] Init attribute effect component for game object: " + this->gameObjectPtr->getName());

		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}
	
	bool AttributeEffectComponent::connect(void)
	{
		this->parseMathematicalFunction();
		
		this->progress = 0.0f;
		this->internalDirectionChange = this->directionChange->getBool();
		this->round = 0;
		oppositeDir = 1.0f;
		
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			// Working here with shared_ptrs is evil, because of bidirectional referencing
			auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
			// Seek for this component and go to the previous one to process
			if (component == this)
			{
				if (i > 0)
				{
					// Search for the attribute in the next prior component
					component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i - 1)).get();
					// std::vector<std::pair<Ogre::String, Variant*>>& getAttributes(void);
					for (size_t j = 0; j < component->getAttributes().size(); j++)
					{
						if (this->attributeName->getString() == component->getAttributes()[j].first)
						{
							this->attributeEffect = component->getAttributes()[j].second;
							// Also create an old attribute effect with the old data, in order to reset later the value
							this->oldAttributeEffect = this->attributeEffect->clone();
							this->targetComponent = component;
							break;
						}
					}
					break;
				}
				else
				{
					// If there is no prior component, search directly in game object for the attribute
					for (size_t j = 0; j < this->gameObjectPtr->getAttributes().size(); j++)
					{
						if (this->attributeName->getString() == gameObjectPtr->getAttributes()[j].first)
						{
							this->attributeEffect = gameObjectPtr->getAttributes()[j].second;
							// Also create an old attribute effect with the old data, in order to reset later the value
							this->oldAttributeEffect = this->attributeEffect->clone();
							break;
						}
					}
					break;
				}
			}
		}

		if (nullptr == this->targetComponent)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "AttributeEffectComponent: Could not add effect, because no prior component has the attribute name: '" 
				+ this->attributeName->getString() + "'. Important: It always just works for the next prior component! So place it beyond the the be manipulated component!");
		}
		
		return true;
	}

	bool AttributeEffectComponent::disconnect(void)
	{
		// Reset the old value
		if (nullptr != this->oldAttributeEffect)
		{
			if (nullptr != this->targetComponent)
				this->targetComponent->actualizeValue(this->oldAttributeEffect);
			else
				this->gameObjectPtr->actualizeValue(this->oldAttributeEffect);
		}

		this->attributeEffect = nullptr;
		this->targetComponent = nullptr;

		if (nullptr != this->oldAttributeEffect)
		{
			delete this->oldAttributeEffect;
			this->oldAttributeEffect = nullptr;
		}
		return true;
	}
	
	void AttributeEffectComponent::onOtherComponentRemoved(unsigned int index)
	{
		GameObjectComponent::onOtherComponentRemoved(index);
		
		int thisIndex = this->gameObjectPtr->getIndexFromComponent(this);
		// Check if the prior component has been removed
		if (index == thisIndex - 1)
		{
			this->attributeEffect = nullptr;
			this->targetComponent = nullptr;
		}
	}

	void AttributeEffectComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (false == this->activated->getBool())
			{
				return;
			}

			// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow")->setCaption("Progress: " + Ogre::StringConverter::toString(this->progress));
			
			if (Ogre::Math::RealEqual(this->oppositeDir, 1.0f))
			{
				this->progress += this->speed->getReal() * dt;

				double varX[] = { 0 };
				Ogre::Real maxLengthResult = static_cast<Ogre::Real>(this->functionParser[4].Eval(varX));

				if (this->progress >= maxLengthResult)
				{
					// this->progress = 0.0f;
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from +  to -");
					if (true == this->internalDirectionChange)
					{
						this->oppositeDir *= -1.0f;
					}
					this->round++;
				}
			}
			else
			{
				this->progress -= this->speed->getReal() * dt;

				double varX[] = { 0 };
				Ogre::Real minLengthResult = static_cast<Ogre::Real>(this->functionParser[3].Eval(varX));

				if (this->progress <= minLengthResult)
				{
					this->progress = 0.0f;
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
					if (true == this->internalDirectionChange)
					{
						this->oppositeDir *= -1.0f;
					}
					this->round++;
				}
			}
			// if the value took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
			// also take the progress into account
			if (this->round == 2 && this->progress >= 0.0f)
			{
				// if repeat is of, only change the direction one time, to get back to its origin and leave
				if (false == this->repeat->getBool())
				{
					this->internalDirectionChange = false;
				}

				// No notification value specified, call at the end of effect
				if (this->notificationValue == 0.0f && nullptr != this->attributeEffectObserver)
				{
					this->attributeEffectObserver->onEffect(Ogre::Vector3::ZERO);
					if (true == this->attributeEffectObserver->shouldReactOneTime())
					{
						delete this->attributeEffectObserver;
						this->attributeEffectObserver = nullptr;
					}
				}
			}
			// when repeat is on, proceed
			// when direction change is on, proceed
			if (true == this->repeat->getBool() || true == this->internalDirectionChange || 0 == this->round)
			{
				// set the x, y, z variable, which is the movement speed with positive or negative direction
				double varX[] = { this->progress };
				double varY[] = { this->progress };
				double varZ[] = { this->progress };
				// evauluate the function for x, y, z, to get the results
				Ogre::Vector3 mathFunction;
				mathFunction.x = static_cast<Ogre::Real>(this->functionParser[0].Eval(varX));
				mathFunction.y = static_cast<Ogre::Real>(this->functionParser[1].Eval(varY));
				mathFunction.z = static_cast<Ogre::Real>(this->functionParser[2].Eval(varZ));

				if (true == this->bShowDebugData)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "AttributeEffectComponent: Debug: x,y,z = " + Ogre::StringConverter::toString(this->progress) +
																						" f(x) = " + Ogre::StringConverter::toString(mathFunction.x) +
																						", f(y) = " + Ogre::StringConverter::toString(mathFunction.y) +
																						", f(z) = " + Ogre::StringConverter::toString(mathFunction.z));
				}
				 
				if (nullptr != this->attributeEffect)
				{
					bool compatibleType = false;
					if (this->attributeEffect->getType() == Variant::VAR_BOOL)
					{
						this->attributeEffect->setValue(static_cast<bool>(mathFunction.x));
						compatibleType = true;
					}
					else if (this->attributeEffect->getType() == Variant::VAR_INT)
					{
						this->attributeEffect->setValue(static_cast<bool>(mathFunction.x));
						compatibleType = true;
					}
					else if (this->attributeEffect->getType() == Variant::VAR_UINT)
					{
						this->attributeEffect->setValue(static_cast<unsigned int>(mathFunction.x));
						compatibleType = true;
					}
					else if (this->attributeEffect->getType() == Variant::VAR_REAL)
					{
						this->attributeEffect->setValue(static_cast<Ogre::Real>(mathFunction.x));
						compatibleType = true;
					}
					else if (this->attributeEffect->getType() == Variant::VAR_ULONG)
					{
						this->attributeEffect->setValue(static_cast<unsigned long>(mathFunction.x));
						compatibleType = true;
					}
					else if (this->attributeEffect->getType() == Variant::VAR_VEC2)
					{
						this->attributeEffect->setValue(Ogre::Vector2(mathFunction.x, mathFunction.y));
						compatibleType = true;
					}
					else if (this->attributeEffect->getType() == Variant::VAR_VEC3)
					{
						this->attributeEffect->setValue(mathFunction);
						compatibleType = true;
					}
					if (true == compatibleType)
					{
						// Set the new value
						if (nullptr != this->targetComponent)
							this->targetComponent->actualizeValue(this->attributeEffect);
						else
							this->gameObjectPtr->actualizeValue(this->attributeEffect);

						// If there is an attribute effect observer, notify at the specified notification value
						if (nullptr != this->attributeEffectObserver)
						{
							if (this->notificationValue != 0.0f)
							{
								if (Ogre::Math::RealEqual(this->notificationValue, this->progress, dt))
								{
									this->attributeEffectObserver->onEffect(mathFunction);
								}
							}
						}
					}
				}
			}
		}
	}

	void AttributeEffectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (AttributeEffectComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AttributeEffectComponent::AttrAttributeName() == attribute->getName())
		{
			this->setAttributeName(attribute->getString());
		}
		else if (AttributeEffectComponent::AttrXFunction() == attribute->getName())
		{
			this->setXFunction(attribute->getString());
		}
		else if (AttributeEffectComponent::AttrYFunction() == attribute->getName())
		{
			this->setYFunction(attribute->getString());
		}
		else if (AttributeEffectComponent::AttrZFunction() == attribute->getName())
		{
			this->setZFunction(attribute->getString());
		}
		else if (AttributeEffectComponent::AttrMinLength() == attribute->getName())
		{
			this->setMinLength(attribute->getString());
		}
		else if (AttributeEffectComponent::AttrMaxLength() == attribute->getName())
		{
			this->setMaxLength(attribute->getString());
		}
		else if (AttributeEffectComponent::AttrSpeed() == attribute->getName())
		{
			this->setSpeed(attribute->getReal());
		}
		else if (AttributeEffectComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (AttributeEffectComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
	}

	void AttributeEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttributeName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attributeName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "XFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->xFunction->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "YFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->yFunction->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ZFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->zFunction->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minLength->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxLength->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AttributeEffectComponent::getClassName(void) const
	{
		return "AttributeEffectComponent";
	}

	Ogre::String AttributeEffectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AttributeEffectComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (false == activated)
		{
			// If set to false, reset values: This is necessary if an effect should be played several times
			this->progress = 0.0f;
			this->internalDirectionChange = this->directionChange->getBool();
			this->round = 0;
			oppositeDir = 1.0f;
		}
	}

	bool AttributeEffectComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}
	
	void AttributeEffectComponent::setAttributeName(const Ogre::String& attributeName)
	{
		this->attributeName->setValue(attributeName);
	}
	
	Ogre::String AttributeEffectComponent::getAttributeName(void) const
	{
		return this->attributeName->getString();
	}
	
	void AttributeEffectComponent::parseMathematicalFunction(void)
	{
		// add the pi constant
		for (int i = 0; i < 5; ++i)
		{
			this->functionParser[i].AddConstant("PI", 3.1415926535897932);
		}
		// parse the function for x, y, z
		int resX = this->functionParser[0].Parse(this->xFunction->getString(), "t");
		if (resX > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for x: "
				+ Ogre::String(this->functionParser[0].ErrorMsg()) + " Note: Variable 't' must be used, for game object: " + this->gameObjectPtr->getName());
		}
// Attention: X is used, test this
		int resY = this->functionParser[1].Parse(this->yFunction->getString(), "t");
		if (resY > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for y: "
															+ Ogre::String(this->functionParser[0].ErrorMsg()) + " Note: Variable 't' must be used, for game object: " + this->gameObjectPtr->getName());
		}

		int resZ = this->functionParser[2].Parse(this->zFunction->getString(), "t");
		if (resZ > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for z: "
															+ Ogre::String(this->functionParser[0].ErrorMsg()) + " Note: Variable 't' must be used, for game object: " + this->gameObjectPtr->getName());
		}

		int resMinLength = this->functionParser[3].Parse(this->minLength->getString(), "t");
		if (resMinLength > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for min length: "
															+ Ogre::String(this->functionParser[0].ErrorMsg()) + " Note: Variable 't' must be used, for game object: " + this->gameObjectPtr->getName());
		}

		int resMaxLength = this->functionParser[4].Parse(this->maxLength->getString(), "t");
		if (resMaxLength > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for max length: "
															+ Ogre::String(this->functionParser[0].ErrorMsg()) + " Note: Variable 't' must be used, for game object: " + this->gameObjectPtr->getName());
		}
	}

	void AttributeEffectComponent::setXFunction(const Ogre::String& xFunction)
	{
		this->xFunction->setValue(xFunction);
		int resX = this->functionParser[0].Parse(xFunction, "t");
		if (resX > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for x: "
				+ Ogre::String(this->functionParser[0].ErrorMsg()) + " Note: Variable 't' must be used.");
		}
	}

	void AttributeEffectComponent::setYFunction(const Ogre::String& yFunction)
	{
		this->yFunction->setValue(yFunction);
// Attention: X is used, test this
		int resY = this->functionParser[1].Parse(yFunction, "t");
		if (resY > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for y: "
				+ Ogre::String(this->functionParser[1].ErrorMsg()) + " Note: Variable 't' must be used.");
		}
	}

	void AttributeEffectComponent::setZFunction(const Ogre::String& zFunction)
	{
		this->zFunction->setValue(zFunction);
// Attention: X is used, test this
		int resZ = this->functionParser[2].Parse(zFunction, "t");
		if (resZ > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttributeEffectComponent] Mathematical function parse error for z: "
				+ Ogre::String(this->functionParser[2].ErrorMsg()) + " Note: Variable 't' must be used.");
		}
	}

	Ogre::String AttributeEffectComponent::getXFunction(void) const
	{
		return this->xFunction->getString();
	}

	Ogre::String AttributeEffectComponent::getYFunction(void) const
	{
		return this->yFunction->getString();
	}

	Ogre::String AttributeEffectComponent::getZFunction(void) const
	{
		return this->zFunction->getString();
	}
	
	void AttributeEffectComponent::setMinLength(const Ogre::String& minLength)
	{
		this->minLength->setValue(minLength);
	}
	
	Ogre::String AttributeEffectComponent::getMinLength(void) const
	{
		return this->minLength->getString();
	}

	void AttributeEffectComponent::setMaxLength(const Ogre::String& maxLength)
	{
		this->maxLength->setValue(maxLength);
	}

	Ogre::String AttributeEffectComponent::getMaxLength(void) const
	{
		return this->maxLength->getString();
	}
	
	void AttributeEffectComponent::setSpeed(Ogre::Real speed)
	{
		this->speed->setValue(speed);
	}
	
	Ogre::Real AttributeEffectComponent::getSpeed(void) const
	{
		return this->speed->getReal();
	}
	
	void AttributeEffectComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
		this->internalDirectionChange = directionChange;
	}

	bool AttributeEffectComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	void AttributeEffectComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool AttributeEffectComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void AttributeEffectComponent::reactOnEndOfEffect(luabind::object closureFunction, Ogre::Real notificationValue, bool oneTime)
	{
		if (nullptr == this->attributeEffectObserver)
		{
			this->attributeEffectObserver = new AttributeEffectObserver(closureFunction, oneTime);
			this->reactOnEndOfEffect(closureFunction, 0.0f, oneTime);
			this->notificationValue = notificationValue;
		}
		else
		{
			// Here later on the existing attributeEffectObserver, call setters?
		}

		if (true == oneTime)
		{
			if (nullptr != this->attributeEffectObserver)
			{
				delete this->attributeEffectObserver;
				this->attributeEffectObserver = nullptr;
			}
			this->notificationValue = 0.0f;
		}
	}

	void AttributeEffectComponent::reactOnEndOfEffect(luabind::object closureFunction, bool oneTime)
	{
		this->reactOnEndOfEffect(closureFunction, 0.0f, oneTime);
	}

}; // namespace end