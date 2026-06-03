#include "NOWAPrecompiled.h"
#include "PlanetSunComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PlanetSunComponent::PlanetSunComponent()
        : GameObjectComponent(),
        name("PlanetSunComponent"),
        sun(nullptr),
        postInitDone(false),
        activated(new Variant(PlanetSunComponent::AttrActivated(), true, this->attributes)),
        radius(new Variant(PlanetSunComponent::AttrRadius(), 55.0f, this->attributes)),
        segmentsH(new Variant(PlanetSunComponent::AttrSegmentsH(), static_cast<unsigned int>(32), this->attributes)),
        segmentsV(new Variant(PlanetSunComponent::AttrSegmentsV(), static_cast<unsigned int>(32), this->attributes)),
        datablock(new Variant(PlanetSunComponent::AttrDatablock(), PlanetSunComponent::DefaultMaterialName(), this->attributes)),
        emissiveColour(new Variant(PlanetSunComponent::AttrEmissiveColour(), Ogre::Vector3(1.0f, 0.6f, 0.1f), this->attributes)),
        diffuseColour(new Variant(PlanetSunComponent::AttrDiffuseColour(), Ogre::Vector3(1.0f, 0.5f, 0.05f), this->attributes)),
        roughness(new Variant(PlanetSunComponent::AttrRoughness(), 0.9f, this->attributes)),
        animationSpeed(new Variant(PlanetSunComponent::AttrAnimationSpeed(), 0.5f, this->attributes)),
        plasmaScale(new Variant(PlanetSunComponent::AttrPlasmaScale(), 4.0f, this->attributes))
    {
        this->activated->setDescription("Enable or disable the sun surface animation.");
        this->radius->setDescription("Sun sphere radius in world units. Should be larger than planet terrain + ocean radii to avoid z-fighting.");
        this->segmentsH->setDescription("Horizontal tessellation of the sphere. 32 is sufficient.");
        this->segmentsV->setDescription("Vertical tessellation of the sphere.");
        this->datablock->setDescription("PBS material name. Defaults to PlanetSunDefaultMaterial.");
        this->emissiveColour->setDescription("Emissive tint colour multiplied with the animated plasma texture. Controls the overall sun glow colour.");
        this->diffuseColour->setDescription("Diffuse base colour of the sun surface (visible in shadowed areas or without emissive).");
        this->roughness->setDescription("PBS roughness [0.0, 1.0]. High values (0.9) give a dull matte look suitable for a gaseous surface.");
        this->animationSpeed->setDescription("Speed multiplier for the plasma animation. Higher = faster boiling. Default 0.08.");
        this->plasmaScale->setDescription("Spatial frequency scale of the plasma pattern. Higher = more fine-grained granules. Default 3.5.");

        this->radius->setConstraints(10.0f, 50000.0f);
        this->segmentsH->setConstraints(4u, 512u);
        this->segmentsV->setConstraints(4u, 512u);
        this->roughness->setConstraints(0.001f, 1.0f);
        this->animationSpeed->setConstraints(0.001f, 10.0f);
        this->plasmaScale->setConstraints(0.1f, 20.0f);
    }

    PlanetSunComponent::~PlanetSunComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin interface
    // =========================================================================

    void PlanetSunComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PlanetSunComponent>(PlanetSunComponent::getStaticClassId(), PlanetSunComponent::getStaticClassName());
    }

    void PlanetSunComponent::initialise()
    {
    }
    void PlanetSunComponent::shutdown()
    {
    }
    void PlanetSunComponent::uninstall()
    {
    }

    void PlanetSunComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& PlanetSunComponent::getName() const
    {
        return this->name;
    }

    // =========================================================================
    //  GameObjectComponent interface
    // =========================================================================

    bool PlanetSunComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRadius())
        {
            this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSegmentsH())
        {
            this->segmentsH->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSegmentsV())
        {
            this->segmentsV->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDatablock())
        {
            this->datablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEmissiveColour())
        {
            this->emissiveColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDiffuseColour())
        {
            this->diffuseColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoughness())
        {
            this->roughness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrAnimationSpeed())
        {
            this->animationSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPlasmaScale())
        {
            this->plasmaScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    bool PlanetSunComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetSunComponent] Init component for game object: " + this->gameObjectPtr->getName());

        if (true == this->activated->getBool())
        {
            this->createSun();
        }

        this->postInitDone = true;
        return true;
    }

    GameObjectCompPtr PlanetSunComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PlanetSunComponentPtr cloned(boost::make_shared<PlanetSunComponent>());

        cloned->setActivated(this->activated->getBool());
        cloned->setRadius(this->radius->getReal());
        cloned->setSegmentsH(this->segmentsH->getUInt());
        cloned->setSegmentsV(this->segmentsV->getUInt());
        cloned->emissiveColour->setValue(this->emissiveColour->getVector3());
        cloned->diffuseColour->setValue(this->diffuseColour->getVector3());
        cloned->roughness->setValue(this->roughness->getReal());
        cloned->animationSpeed->setValue(this->animationSpeed->getReal());
        cloned->plasmaScale->setValue(this->plasmaScale->getReal());
        cloned->datablock->setValue(this->datablock->getString());

        clonedGameObjectPtr->addComponent(cloned);
        cloned->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(cloned));
        return cloned;
    }

    bool PlanetSunComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool PlanetSunComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    void PlanetSunComponent::onRemoveComponent(void)
    {
        this->destroySun();
        GameObjectComponent::onRemoveComponent();
    }

    void PlanetSunComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && true == this->activated->getBool() && nullptr != this->sun)
        {
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->sunUpdateClosureId, [this](Ogre::Real renderDt)
            {
               this->sun->update(static_cast<float>(renderDt));
            }, false);
        }
    }

    void PlanetSunComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (PlanetSunComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PlanetSunComponent::AttrRadius() == attribute->getName())
        {
            this->setRadius(attribute->getReal());
        }
        else if (PlanetSunComponent::AttrSegmentsH() == attribute->getName())
        {
            this->segmentsH->setValue(attribute->getUInt());
            if (true == this->postInitDone)
            {
                NOWA::GraphicsModule::RenderCommand cmd = [this]()
                {
                    if (nullptr != this->sun)
                    {
                        this->sun->recreate(this->radius->getReal(), static_cast<int>(this->segmentsH->getUInt()), static_cast<int>(this->segmentsV->getUInt()), this->gameObjectPtr->getSceneNode(), this->datablock->getString());
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setSegmentsH");
            }
        }
        else if (PlanetSunComponent::AttrSegmentsV() == attribute->getName())
        {
            this->segmentsV->setValue(attribute->getUInt());
            if (true == this->postInitDone)
            {
                NOWA::GraphicsModule::RenderCommand cmd = [this]()
                {
                    if (nullptr != this->sun)
                    {
                        this->sun->recreate(this->radius->getReal(), static_cast<int>(this->segmentsH->getUInt()), static_cast<int>(this->segmentsV->getUInt()), this->gameObjectPtr->getSceneNode(), this->datablock->getString());
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setSegmentsV");
            }
        }
        else if (PlanetSunComponent::AttrDatablock() == attribute->getName())
        {
            this->datablock->setValue(attribute->getString());
            if (true == this->postInitDone)
            {
                NOWA::GraphicsModule::RenderCommand cmd = [this]()
                {
                    if (nullptr != this->sun)
                    {
                        this->sun->recreate(this->radius->getReal(), static_cast<int>(this->segmentsH->getUInt()), static_cast<int>(this->segmentsV->getUInt()), this->gameObjectPtr->getSceneNode(), this->datablock->getString());
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setDatablock");
            }
        }
        else if (PlanetSunComponent::AttrEmissiveColour() == attribute->getName())
        {
            this->setEmissiveColour(attribute->getVector3());
        }
        else if (PlanetSunComponent::AttrDiffuseColour() == attribute->getName())
        {
            this->setDiffuseColour(attribute->getVector3());
        }
        else if (PlanetSunComponent::AttrRoughness() == attribute->getName())
        {
            this->setRoughness(attribute->getReal());
        }
        else if (PlanetSunComponent::AttrAnimationSpeed() == attribute->getName())
        {
            this->setAnimationSpeed(attribute->getReal());
        }
        else if (PlanetSunComponent::AttrPlasmaScale() == attribute->getName())
        {
            this->setPlasmaScale(attribute->getReal());
        }
    }

    void PlanetSunComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrRadius().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrSegmentsH().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->segmentsH->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrSegmentsV().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->segmentsV->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrEmissiveColour().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->emissiveColour->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrDiffuseColour().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseColour->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrRoughness().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrAnimationSpeed().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationSpeed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(AttrPlasmaScale().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->plasmaScale->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String PlanetSunComponent::getClassName(void) const
    {
        return "PlanetSunComponent";
    }

    Ogre::String PlanetSunComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    std::optional<NOWA::GameObjectTypeDescriptor> PlanetSunComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = eType::CUSTOM;
        desc.displayName = "Planet Sun";
        desc.meshToDisplay = "Node.mesh";
        desc.needsMeshItem = false;
        desc.forceZeroTransform = false;
        desc.autoComponents = {PlanetSunComponent::getStaticClassName()};
        return desc;
    }

    // =========================================================================
    //  createSun / destroySun
    // =========================================================================

    void PlanetSunComponent::setDynamic(bool dynamic)
    {
        if (nullptr != this->sun)
        {
            this->sun->setDynamic(dynamic);
        }
    }

    void PlanetSunComponent::createSun()
    {
        if (nullptr != this->sun)
        {
            return;
        }

        this->sun = new PlanetSun(this->gameObjectPtr->getName(), this->gameObjectPtr->getSceneManager());

        const float r = this->radius->getReal();
        const int sH = static_cast<int>(this->segmentsH->getUInt());
        const int sV = static_cast<int>(this->segmentsV->getUInt());
        const Ogre::String db = this->datablock->getString();
        const Ogre::Vector3 ec = this->emissiveColour->getVector3();
        const Ogre::Vector3 dc = this->diffuseColour->getVector3();
        const float rou = this->roughness->getReal();
        const float aspd = this->animationSpeed->getReal();
        const float psc = this->plasmaScale->getReal();

        NOWA::GraphicsModule::RenderCommand cmd = [this, r, sH, sV, db, ec, dc, rou, aspd, psc]()
        {
            if (false == this->sun->create(r, sH, sV, this->gameObjectPtr->getSceneNode(), db))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetSunComponent] Failed to create sun for '" + this->gameObjectPtr->getName() + "'.");
                return;
            }

            this->sun->setEmissiveColour(ec);
            this->sun->setDiffuseColour(dc);
            this->sun->setRoughness(rou);
            this->sun->setAnimationSpeed(aspd);
            this->sun->setPlasmaScale(psc);

            this->gameObjectPtr->setDoNotDestroyMovableObject(true);
            this->gameObjectPtr->init(this->sun->getItem());
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::createSun");

        this->sunUpdateClosureId = this->gameObjectPtr->getName() + PlanetSunComponent::getStaticClassName() + "::plasmaUpdate";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->sunUpdateClosureId, [this](Ogre::Real renderDt)
        {
            if (nullptr != this->sun && true == this->activated->getBool())
            {
                this->sun->update(static_cast<float>(renderDt));
            }
        }, false);
    }

    void PlanetSunComponent::destroySun()
    {
        if (nullptr == this->sun)
        {
            return;
        }

        if (false == this->sunUpdateClosureId.empty())
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->sunUpdateClosureId);
            this->sunUpdateClosureId.clear();
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->gameObjectPtr->nullMovableObject();
            this->sun->destroy();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::destroySun");

        delete this->sun;
        this->sun = nullptr;
    }

    // =========================================================================
    //  Setters / Getters
    // =========================================================================

    void PlanetSunComponent::setActivated(bool value)
    {
        this->activated->setValue(value);
        if (false == this->postInitDone)
        {
            return;
        }

        if (true == value && nullptr == this->sun)
        {
            this->createSun();
        }
        else if (false == value && nullptr != this->sun)
        {
            this->destroySun();
        }
    }

    bool PlanetSunComponent::isActivated() const
    {
        return this->activated->getBool();
    }

    void PlanetSunComponent::setRadius(float r)
    {
        this->radius->setValue(r);
        if (false == this->postInitDone || nullptr == this->sun)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this, r]()
        {
            this->gameObjectPtr->nullMovableObject();
            this->sun->recreate(r, static_cast<int>(this->segmentsH->getUInt()), static_cast<int>(this->segmentsV->getUInt()), this->gameObjectPtr->getSceneNode(), this->datablock->getString());
            this->gameObjectPtr->init(this->sun->getItem());
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setRadius");
    }

    float PlanetSunComponent::getRadius() const
    {
        return this->radius->getReal();
    }

    void PlanetSunComponent::setSegmentsH(unsigned int v)
    {
        this->segmentsH->setValue(v);
    }

    unsigned int PlanetSunComponent::getSegmentsH() const
    {
        return this->segmentsH->getUInt();
    }

    void PlanetSunComponent::setSegmentsV(unsigned int v)
    {
        this->segmentsV->setValue(v);
    }

    unsigned int PlanetSunComponent::getSegmentsV() const
    {
        return this->segmentsV->getUInt();
    }

    void PlanetSunComponent::setEmissiveColour(const Ogre::Vector3& colour)
    {
        this->emissiveColour->setValue(colour);
        if (nullptr != this->sun)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, colour]()
            {
                this->sun->setEmissiveColour(colour);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setEmissiveColour");
        }
    }

    Ogre::Vector3 PlanetSunComponent::getEmissiveColour() const
    {
        return this->emissiveColour->getVector3();
    }

    void PlanetSunComponent::setDiffuseColour(const Ogre::Vector3& colour)
    {
        this->diffuseColour->setValue(colour);
        if (nullptr != this->sun)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, colour]()
            {
                this->sun->setDiffuseColour(colour);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setDiffuseColour");
        }
    }

    Ogre::Vector3 PlanetSunComponent::getDiffuseColour() const
    {
        return this->diffuseColour->getVector3();
    }

    void PlanetSunComponent::setRoughness(float r)
    {
        this->roughness->setValue(r);
        if (nullptr != this->sun)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, r]()
            {
                this->sun->setRoughness(r);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetSunComponent::setRoughness");
        }
    }

    float PlanetSunComponent::getRoughness() const
    {
        return this->roughness->getReal();
    }

    void PlanetSunComponent::setAnimationSpeed(float speed)
    {
        this->animationSpeed->setValue(speed);
        if (nullptr != this->sun)
        {
            this->sun->setAnimationSpeed(speed);
        }
    }

    float PlanetSunComponent::getAnimationSpeed() const
    {
        return this->animationSpeed->getReal();
    }

    void PlanetSunComponent::setPlasmaScale(float scale)
    {
        this->plasmaScale->setValue(scale);
        if (nullptr != this->sun)
        {
            this->sun->setPlasmaScale(scale);
        }
    }

    float PlanetSunComponent::getPlasmaScale() const
    {
        return this->plasmaScale->getReal();
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    PlanetSunComponent* getPlanetSunComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<PlanetSunComponent>()).get();
    }

    PlanetSunComponent* getPlanetSunComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<PlanetSunComponent>(name)).get();
    }

    void PlanetSunComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<PlanetSunComponent, GameObjectComponent>("PlanetSunComponent")
                .def("setActivated", &PlanetSunComponent::setActivated)
                .def("isActivated", &PlanetSunComponent::isActivated)
                .def("setRadius", &PlanetSunComponent::setRadius)
                .def("getRadius", &PlanetSunComponent::getRadius)
                .def("setSegmentsH", &PlanetSunComponent::setSegmentsH)
                .def("getSegmentsH", &PlanetSunComponent::getSegmentsH)
                .def("setSegmentsV", &PlanetSunComponent::setSegmentsV)
                .def("getSegmentsV", &PlanetSunComponent::getSegmentsV)
                .def("setEmissiveColour", &PlanetSunComponent::setEmissiveColour)
                .def("getEmissiveColour", &PlanetSunComponent::getEmissiveColour)
                .def("setDiffuseColour", &PlanetSunComponent::setDiffuseColour)
                .def("getDiffuseColour", &PlanetSunComponent::getDiffuseColour)
                .def("setRoughness", &PlanetSunComponent::setRoughness)
                .def("getRoughness", &PlanetSunComponent::getRoughness)
                .def("setAnimationSpeed", &PlanetSunComponent::setAnimationSpeed)
                .def("getAnimationSpeed", &PlanetSunComponent::getAnimationSpeed)
                .def("setPlasmaScale", &PlanetSunComponent::setPlasmaScale)
                .def("getPlasmaScale", &PlanetSunComponent::getPlasmaScale)];

        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "class inherits GameObjectComponent", getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setActivated(bool activated)", "Enables or disables the sun plasma animation.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "bool isActivated()", "Returns true if the sun animation is currently active.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setRadius(float radius)", "Sets the sun sphere radius in world units and fully recreates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "float getRadius()", "Returns the current sun radius in world units.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setSegmentsH(uint segs)", "Sets the horizontal tessellation and recreates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "uint getSegmentsH()", "Returns the current horizontal tessellation segment count.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setSegmentsV(uint segs)", "Sets the vertical tessellation and recreates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "uint getSegmentsV()", "Returns the current vertical tessellation segment count.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setEmissiveColour(Vector3 colour)", "Sets the emissive tint colour multiplied with the animated plasma texture. Controls the sun glow colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "Vector3 getEmissiveColour()", "Returns the current emissive tint colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setDiffuseColour(Vector3 colour)", "Sets the base diffuse colour of the sun surface.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "Vector3 getDiffuseColour()", "Returns the current diffuse colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setRoughness(float roughness)", "Sets PBS roughness [0.0, 1.0]. High values (0.9) give a matte gaseous look.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "float getRoughness()", "Returns the current roughness value.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setAnimationSpeed(float speed)", "Sets the plasma animation speed multiplier. Higher = faster boiling surface.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "float getAnimationSpeed()", "Returns the current animation speed multiplier.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "void setPlasmaScale(float scale)", "Sets the spatial frequency of the plasma pattern. Higher = more fine-grained granules.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetSunComponent", "float getPlasmaScale()", "Returns the current plasma scale value.");

        gameObjectClass.def("getPlanetSunComponent", (PlanetSunComponent * (*)(GameObject*)) & getPlanetSunComponent);
        gameObjectClass.def("getPlanetSunComponentFromName", &getPlanetSunComponentFromName);
        gameObjectControllerClass.def("castPlanetSunComponent", &GameObjectController::cast<PlanetSunComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetSunComponent getPlanetSunComponent()", "Gets the PlanetSunComponent from this GameObject.");
    }

    bool PlanetSunComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto thisCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlanetSunComponent>());
        if (nullptr == thisCompPtr)
        {
            return true;
        }
        return false;
    }

} // namespace NOWA