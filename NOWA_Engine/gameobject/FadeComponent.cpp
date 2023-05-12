#include "NOWAPrecompiled.h"
#include "FadeComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/FaderProcess.h"
#include "modules/LuaScript.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	class EXPORTED FaderLuaProcess : public FaderProcess
	{
	public:
		explicit FaderLuaProcess(FaderProcess::FadeOperation fadeOperation, Ogre::Real duration, class FadeComponent* fadeComponent)
			: FaderProcess(fadeOperation, duration),
			fadeComponent(fadeComponent)
		{
		
		}

	protected:
		virtual void finished(void) override
		{
			FaderProcess::finished();
			if (nullptr != this->fadeComponent->getOwner()->getLuaScript())
			{
				if (this->fadeComponent->fadeCompletedClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->fadeComponent->fadeCompletedClosureFunction);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnFadeCompleted' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}

			}
		}
	private:
		FadeComponent* fadeComponent;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	FadeComponent::FadeComponent()
		: GameObjectComponent(),
		activated(new Variant(FadeComponent::AttrActivated(), true, this->attributes)),
		fadeMode(new Variant(FadeComponent::AttrFadeMode(), { Ogre::String("FadeIn"), Ogre::String("FadeOut") }, this->attributes)),
		duration(new Variant(FadeComponent::AttrDuration(), 5.0f, this->attributes))
	{
	}

	FadeComponent::~FadeComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FadeComponent] Destructor fog component for game object: " + this->gameObjectPtr->getName());
	}

	bool FadeComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FadeMode")
		{
			this->fadeMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration")
		{
			this->duration->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr FadeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return GameObjectCompPtr();
	}

	bool FadeComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FadeComponent] Init fog component for game object: " + this->gameObjectPtr->getName());

		
		return true;
	}

	void FadeComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;
	}

	bool FadeComponent::connect(void)
	{
		
		return true;
	}

	bool FadeComponent::disconnect(void)
	{
		
		return true;
	}

	void FadeComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (FadeComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (FadeComponent::AttrFadeMode() == attribute->getName())
		{
			this->setFadeMode(attribute->getListSelectedValue());
		}
		else if (FadeComponent::AttrDuration() == attribute->getName())
		{
			this->setDurationSec(attribute->getReal());
		}
	}

	void FadeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FadeMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Duration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->duration->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void FadeComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == this->isInSimulation)
		{
			if (true == activated)
			{
				FaderProcess::FadeOperation fadeMode = FaderProcess::FadeOperation::FADE_NONE;

				if ("FadeIn" == this->fadeMode->getListSelectedValue())
				{
					fadeMode = FaderProcess::FadeOperation::FADE_IN;
				}
				else if ("FadeOut" == this->fadeMode->getListSelectedValue())
				{
					fadeMode = FaderProcess::FadeOperation::FADE_OUT;
				}
		
				ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderLuaProcess(fadeMode, this->duration->getReal(), this)));
			}
		}
	}

	bool FadeComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void FadeComponent::setFadeMode(const Ogre::String& fadeMode)
	{
		this->fadeMode->setListSelectedValue(fadeMode);
	}

	Ogre::String FadeComponent::getFadeMode(void) const
	{
		return this->fadeMode->getListSelectedValue();
	}

	void FadeComponent::setDurationSec(Ogre::Real duration)
	{
		if (duration < 0.0f)
			duration = 1.0f;
		this->duration->setValue(duration);
	}

	Ogre::Real FadeComponent::getDurationSec(void) const
	{
		return this->duration->getReal();
	}

	void FadeComponent::reactOnFadeCompleted(luabind::object closureFunction)
	{
		this->fadeCompletedClosureFunction = closureFunction;
	}

	Ogre::String FadeComponent::getClassName(void) const
	{
		return "FadeComponent";
	}

	Ogre::String FadeComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

}; // namespace end