#include "NOWAPrecompiled.h"
#include "PlanetSurfaceComponent.h"

#include "gameobject/GameObjectFactory.h"
#include "gameobject/GameObjectController.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    //  Constructor / Destructor
    // =========================================================================

    PlanetSurfaceComponent::PlanetSurfaceComponent() : GameObjectComponent(), name("PlanetSurfaceComponent"), targetPlanetId(new Variant(PlanetSurfaceComponent::AttrTargetPlanetId(), static_cast<unsigned long>(0ul), this->attributes))
    {
        this->targetPlanetId->setDescription("Id of the planet or moon (managed by UniversumComponent) that this object is placed on. "
                                             "Set this to the planet's Id shown in the editor Properties panel. "
                                             "UniversumComponent hides and repositions this object automatically: hidden while the "
                                             "planet is orbiting, shown and correctly placed when the player is on the surface.");
    }

    PlanetSurfaceComponent::~PlanetSurfaceComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin
    // =========================================================================

    void PlanetSurfaceComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PlanetSurfaceComponent>(PlanetSurfaceComponent::getStaticClassId(), PlanetSurfaceComponent::getStaticClassName());
    }

    void PlanetSurfaceComponent::initialise()
    {
    }

    void PlanetSurfaceComponent::shutdown()
    {
    }

    void PlanetSurfaceComponent::uninstall()
    {
    }

    void PlanetSurfaceComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& PlanetSurfaceComponent::getName() const
    {
        return this->name;
    }

    // =========================================================================
    //  GameObjectComponent lifecycle
    // =========================================================================

    Ogre::String PlanetSurfaceComponent::getClassName(void) const
    {
        return "PlanetSurfaceComponent";
    }

    Ogre::String PlanetSurfaceComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    bool PlanetSurfaceComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrTargetPlanetId())
        {
            this->targetPlanetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    bool PlanetSurfaceComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[PlanetSurfaceComponent] Init component for game object: " + this->gameObjectPtr->getName() + " target planet id=" + Ogre::StringConverter::toString(this->targetPlanetId->getULong()));
        return true;
    }

    void PlanetSurfaceComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (PlanetSurfaceComponent::AttrTargetPlanetId() == attribute->getName())
        {
            this->setTargetPlanetId(attribute->getULong());
        }
    }

    void PlanetSurfaceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = uint/ulong
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Target Planet Id"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetPlanetId->getULong())));
        propertiesXML->append_node(propertyXML);
    }

    GameObjectCompPtr PlanetSurfaceComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PlanetSurfaceCompPtr clonedCompPtr(boost::make_shared<PlanetSurfaceComponent>());

        clonedCompPtr->targetPlanetId->setValue(this->targetPlanetId->getULong());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool PlanetSurfaceComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Only one PlanetSurfaceComponent per game object.
        auto existing = NOWA::makeStrongPtr(gameObject->getComponent<PlanetSurfaceComponent>());
        if (nullptr == existing)
        {
            return true;
        }
        return false;
    }

    // =========================================================================
    //  Getter / Setter
    // =========================================================================

    void PlanetSurfaceComponent::setTargetPlanetId(unsigned long id)
    {
        this->targetPlanetId->setValue(id);
    }

    unsigned long PlanetSurfaceComponent::getTargetPlanetId(void) const
    {
        return this->targetPlanetId->getULong();
    }

    // =========================================================================
    //  Type descriptor
    // =========================================================================

    std::optional<NOWA::GameObjectTypeDescriptor> PlanetSurfaceComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = eType::CUSTOM;
        desc.displayName = "PlanetSurface";
        desc.meshToDisplay = "Node.mesh";
        desc.needsMeshItem = false;
        desc.forceZeroTransform = false;
        desc.autoComponents = {PlanetSurfaceComponent::getStaticClassName()};
        return desc;
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    PlanetSurfaceComponent* getPlanetSurfaceComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<PlanetSurfaceComponent>(gameObject->getComponentWithOccurrence<PlanetSurfaceComponent>(occurrenceIndex)).get();
    }

    PlanetSurfaceComponent* getPlanetSurfaceComponent(GameObject* gameObject)
    {
        return makeStrongPtr<PlanetSurfaceComponent>(gameObject->getComponent<PlanetSurfaceComponent>()).get();
    }

    PlanetSurfaceComponent* getPlanetSurfaceComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<PlanetSurfaceComponent>(gameObject->getComponentFromName<PlanetSurfaceComponent>(name)).get();
    }

    void PlanetSurfaceComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<PlanetSurfaceComponent, GameObjectComponent>("PlanetSurfaceComponent").def("setTargetPlanetId", &PlanetSurfaceComponent::setTargetPlanetId).def("getTargetPlanetId", &PlanetSurfaceComponent::getTargetPlanetId)];

        LuaScriptApi::getInstance()->addClassToCollection("PlanetSurfaceComponent", "class inherits GameObjectComponent", PlanetSurfaceComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSurfaceComponent", "void setTargetPlanetId(uint id)", "Sets the Id of the planet or moon this object is placed on.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSurfaceComponent", "uint getTargetPlanetId()", "Gets the target planet Id.");

        gameObjectClass.def("getPlanetSurfaceComponentFromName", &getPlanetSurfaceComponentFromName);
        gameObjectClass.def("getPlanetSurfaceComponent", (PlanetSurfaceComponent * (*)(GameObject*)) & getPlanetSurfaceComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetSurfaceComponent getPlanetSurfaceComponent()", "Gets the PlanetSurfaceComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetSurfaceComponent getPlanetSurfaceComponentFromName(String name)", "Gets the PlanetSurfaceComponent by name.");

        gameObjectControllerClass.def("castPlanetSurfaceComponent", &GameObjectController::cast<PlanetSurfaceComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PlanetSurfaceComponent castPlanetSurfaceComponent(PlanetSurfaceComponent other)", "Casts type for Lua auto completion.");
    }

} // namespace NOWA