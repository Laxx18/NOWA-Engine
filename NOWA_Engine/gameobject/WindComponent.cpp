#include "NOWAPrecompiled.h"

#include "GameObjectController.h"
#include "shader/HlmsWind.h"
#include "WindComponent.h"
#include "main/AppStateManager.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    WindComponent::WindComponent() :
        GameObjectComponent(),
        windStrength(new Variant(WindComponent::AttrWindStrength(), 0.1f, this->attributes)),
        windDirection(new Variant(WindComponent::AttrWindDirection(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes)),
        windFrequency(new Variant(WindComponent::AttrWindFrequency(), 0.2f, this->attributes))
    {
        this->windStrength->setConstraints(0.0f, 5.0f);
        this->windFrequency->setConstraints(0.1f, 10.0f);

        this->windStrength->setDescription("Overall wind displacement scale applied to all Wind HLMS objects. "
                                           "0 = no movement. 0.5 = gentle breeze. 2.0+ = strong storm. Typical range: 0..2.");
        this->windDirection->setDescription("Dominant wind direction in world space (automatically normalized). "
                                            "(1,0,0) = east wind, (-1,0,0) = west, (0,0,1) = south. "
                                            "The Y component has no effect on ground-level vegetation.");
        this->windFrequency->setDescription("Speed at which the 3D Perlin noise field animates (turbulence speed). "
                                            "1.0 = default. Higher values = more rapid fluttering between blades. Typical range: 0.1..5.0.");
    }

    WindComponent::~WindComponent()
    {
    }

    bool WindComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WindStrength")
        {
            this->windStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WindDirection")
        {
            this->windDirection->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WindFrequency")
        {
            this->windFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    GameObjectCompPtr WindComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool WindComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WindComponent] Init wind component for game object: " + this->gameObjectPtr->getName());

        // Apply initial values to HlmsWind immediately so the shader
        // uses correct parameters from the first frame.
        this->applyToHlmsWind();
        return true;
    }

    void WindComponent::onRemoveComponent(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WindComponent] Remove wind component for game object: " + this->gameObjectPtr->getName());

        // Reset wind to neutral so removing the component stops wind effects.
        GraphicsModule::RenderCommand renderCommand = []()
        {
            HlmsWind* hlmsWind = dynamic_cast<HlmsWind*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER0));
            if (nullptr != hlmsWind)
            {
                hlmsWind->setWindStrength(0.0f);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "WindComponent::onRemoveComponent::reset");
    }

    bool WindComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool WindComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    void WindComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Wind parameters are set once when changed (not per-frame).
        // Time advancement is handled by WorkspaceBaseComponent::update via hlmsWind->addTime().
        // Nothing to do here unless animated wind (gusts) is desired in the future.
    }

    void WindComponent::applyToHlmsWind(void)
    {
        const Ogre::Real strength = this->windStrength->getReal();
        const Ogre::Vector3 dir = this->windDirection->getVector3();
        const Ogre::Real frequency = this->windFrequency->getReal();

        GraphicsModule::RenderCommand renderCommand = [strength, dir, frequency]()
        {
            HlmsWind* hlmsWind = dynamic_cast<HlmsWind*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER0));
            if (nullptr != hlmsWind)
            {
                hlmsWind->setWindStrength(static_cast<float>(strength));
                hlmsWind->setWindDirection(dir);
                hlmsWind->setWindFrequency(static_cast<float>(frequency));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "WindComponent::applyToHlmsWind");
    }

    void WindComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (WindComponent::AttrWindStrength() == attribute->getName())
        {
            this->setWindStrength(attribute->getReal());
        }
        else if (WindComponent::AttrWindDirection() == attribute->getName())
        {
            this->setWindDirection(attribute->getVector3());
        }
        else if (WindComponent::AttrWindFrequency() == attribute->getName())
        {
            this->setWindFrequency(attribute->getReal());
        }
    }

    void WindComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WindStrength"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windStrength->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WindDirection"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windDirection->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WindFrequency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windFrequency->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String WindComponent::getClassName(void) const
    {
        return "WindComponent";
    }

    Ogre::String WindComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void WindComponent::setWindStrength(Ogre::Real strength)
    {
        this->windStrength->setValue(strength);
        this->applyToHlmsWind();
    }

    Ogre::Real WindComponent::getWindStrength(void) const
    {
        return this->windStrength->getReal();
    }

    void WindComponent::setWindDirection(const Ogre::Vector3& direction)
    {
        // Normalize before storing so NOWA-Design shows the true normalized value.
        Ogre::Vector3 normalized = direction;
        if (normalized.squaredLength() > 0.0f)
        {
            normalized.normalise();
        }
        this->windDirection->setValue(normalized);
        this->applyToHlmsWind();
    }

    Ogre::Vector3 WindComponent::getWindDirection(void) const
    {
        return this->windDirection->getVector3();
    }

    void WindComponent::setWindFrequency(Ogre::Real frequency)
    {
        this->windFrequency->setValue(frequency);
        this->applyToHlmsWind();
    }

    Ogre::Real WindComponent::getWindFrequency(void) const
    {
        return this->windFrequency->getReal();
    }

    // ------------------------------------------------------------
    // Lua free functions
    // ------------------------------------------------------------
    WindComponent* getWindComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<WindComponent>(gameObject->getComponentWithOccurrence<WindComponent>(occurrenceIndex)).get();
    }

    WindComponent* getWindComponent(GameObject* gameObject)
    {
        return makeStrongPtr<WindComponent>(gameObject->getComponent<WindComponent>()).get();
    }

    WindComponent* getWindComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<WindComponent>(gameObject->getComponentFromName<WindComponent>(name)).get();
    }

    void WindComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<WindComponent, GameObjectComponent>("WindComponent")
                .def("setActivated", &WindComponent::setActivated)
                .def("isActivated", &WindComponent::isActivated)

                .def("setWindStrength", &WindComponent::setWindStrength)
                .def("getWindStrength", &WindComponent::getWindStrength)

                .def("setWindDirection", &WindComponent::setWindDirection)
                .def("getWindDirection", &WindComponent::getWindDirection)

                .def("setWindFrequency", &WindComponent::setWindFrequency)
                .def("getWindFrequency", &WindComponent::getWindFrequency)];

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "class inherits GameObjectComponent", WindComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "void setActivated(bool activated)", "Sets whether this component is active. If deactivated, wind parameters are frozen at their current values.");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "bool isActivated()", "Gets whether this component is active.");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "void setWindStrength(float strength)",
            "Sets the overall wind displacement strength applied to all Wind HLMS objects. "
            "0 = no movement. 0.5 = gentle breeze. 2.0 = strong storm. Typical range: 0..2. "
            "Example: windComp:setWindStrength(1.5) -- strong wind");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "float getWindStrength()", "Gets the current wind strength. Returns a value in the range 0..5.");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "void setWindDirection(Vector3 direction)",
            "Sets the dominant world-space wind direction (automatically normalized). "
            "(1,0,0) = east, (-1,0,0) = west, (0,0,1) = south, (0,0,-1) = north. "
            "The Y component has no visual effect on ground-level vegetation. "
            "Example: windComp:setWindDirection(Vector3(1, 0, 0.5)) -- east-southeast wind");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "Vector3 getWindDirection()", "Gets the current normalized wind direction vector.");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "void setWindFrequency(float frequency)",
            "Sets the animation speed of the 3D Perlin noise turbulence field. "
            "1.0 = default neutral speed. Higher values produce rapid fluttering. "
            "Typical range: 0.1 (very slow, lazy sway) to 5.0 (rapid gusty turbulence). "
            "Example: windComp:setWindFrequency(2.0) -- faster turbulence");

        LuaScriptApi::getInstance()->addClassToCollection("WindComponent", "float getWindFrequency()", "Gets the current wind frequency (turbulence animation speed).");

        gameObjectClass.def("getWindComponentFromName", &getWindComponentFromName);
        gameObjectClass.def("getWindComponent", (WindComponent * (*)(GameObject*)) & getWindComponent);
        // If it is desired to create several of this components for one game object
        gameObjectClass.def("getWindComponent2", (WindComponent * (*)(GameObject*, unsigned int)) & getWindComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WindComponent getWindComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurrence index, since a game object may have this component several times.");

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WindComponent getWindComponent()", "Gets the component. Use this if the game object has this component only once.");

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WindComponent getWindComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castWindComponent", &GameObjectController::cast<WindComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "WindComponent castWindComponent(WindComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end