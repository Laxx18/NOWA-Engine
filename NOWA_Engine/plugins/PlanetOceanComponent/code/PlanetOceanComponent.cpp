#include "NOWAPrecompiled.h"
#include "PlanetOceanComponent.h"
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

    PlanetOceanComponent::PlanetOceanComponent() :
        GameObjectComponent(),
        name("PlanetOceanComponent"),
        ocean(nullptr),
        postInitDone(false),
        activated(new Variant(PlanetOceanComponent::AttrActivated(), true, this->attributes)),
        radius(new Variant(PlanetOceanComponent::AttrRadius(), 52.0f, this->attributes)),
        segmentsH(new Variant(PlanetOceanComponent::AttrSegmentsH(), static_cast<unsigned int>(64), this->attributes)),
        segmentsV(new Variant(PlanetOceanComponent::AttrSegmentsV(), static_cast<unsigned int>(64), this->attributes)),
        datablock(new Variant(PlanetOceanComponent::AttrDatablock(), PlanetOceanComponent::DefaultMaterialName(), this->attributes)),
        deepColour(new Variant(PlanetOceanComponent::AttrDeepColour(), Ogre::Vector3(0.0f, 0.03f, 0.07f), this->attributes)),
        shallowColour(new Variant(PlanetOceanComponent::AttrShallowColour(), Ogre::Vector3(0.0f, 0.12f, 0.18f), this->attributes)),
        roughness(new Variant(PlanetOceanComponent::AttrRoughness(), 0.04f, this->attributes)),
        metalness(new Variant(PlanetOceanComponent::AttrMetalness(), 0.0f, this->attributes)),
        transparency(new Variant(PlanetOceanComponent::AttrTransparency(), 0.7f, this->attributes)),
        reflectionMap(new Variant(PlanetOceanComponent::AttrReflectionMap(), Ogre::String(""), this->attributes)),
        waveAmplitudeScale(new Variant(PlanetOceanComponent::AttrWaveAmplitudeScale(), 1.0f, this->attributes)),
        wave0Amplitude(new Variant(PlanetOceanComponent::AttrWave0Amplitude(), 0.4f, this->attributes)),
        wave0Frequency(new Variant(PlanetOceanComponent::AttrWave0Frequency(), 0.25f, this->attributes)),
        wave0Speed(new Variant(PlanetOceanComponent::AttrWave0Speed(), 1.2f, this->attributes)),
        wave0Direction(new Variant(PlanetOceanComponent::AttrWave0Direction(), 0.0f, this->attributes)),
        wave1Amplitude(new Variant(PlanetOceanComponent::AttrWave1Amplitude(), 0.25f, this->attributes)),
        wave1Frequency(new Variant(PlanetOceanComponent::AttrWave1Frequency(), 0.4f, this->attributes)),
        wave1Speed(new Variant(PlanetOceanComponent::AttrWave1Speed(), 0.9f, this->attributes)),
        wave1Direction(new Variant(PlanetOceanComponent::AttrWave1Direction(), 1.2f, this->attributes)),
        wave2Amplitude(new Variant(PlanetOceanComponent::AttrWave2Amplitude(), 0.15f, this->attributes)),
        wave2Frequency(new Variant(PlanetOceanComponent::AttrWave2Frequency(), 0.6f, this->attributes)),
        wave2Speed(new Variant(PlanetOceanComponent::AttrWave2Speed(), 1.6f, this->attributes)),
        wave2Direction(new Variant(PlanetOceanComponent::AttrWave2Direction(), 2.7f, this->attributes)),
        wave3Amplitude(new Variant(PlanetOceanComponent::AttrWave3Amplitude(), 0.1f, this->attributes)),
        wave3Frequency(new Variant(PlanetOceanComponent::AttrWave3Frequency(), 0.9f, this->attributes)),
        wave3Speed(new Variant(PlanetOceanComponent::AttrWave3Speed(), 2.0f, this->attributes)),
        wave3Direction(new Variant(PlanetOceanComponent::AttrWave3Direction(), 4.3f, this->attributes))
    {
        this->activated->setDescription("Enable or disable the ocean animation.");
        this->radius->setDescription("Ocean sphere radius. Should be slightly larger than the terrain sphere beneath it.");
        this->segmentsH->setDescription("Horizontal tessellation of the sphere. Higher values give smoother waves.");
        this->segmentsV->setDescription("Vertical tessellation of the sphere.");
        this->datablock->setDescription("PBS material name. Defaults to PlanetOceanDefaultMaterial.");
        this->deepColour->setDescription("Deep ocean diffuse colour (dark blue for abyssal depth).");
        this->shallowColour->setDescription("Shallow water colour blended in at wave crests.");
        this->roughness->setDescription("PBS roughness [0.001, 1]. Low values produce mirror-like reflections.");
        this->metalness->setDescription("PBS metalness [0, 1]. Usually 0 for water.");
        this->transparency->setDescription("Water transparency [0=opaque, 1=fully transparent].");
        this->reflectionMap->setDescription("Optional cubemap texture name for environment reflections. Leave empty to skip.");
        this->waveAmplitudeScale->setDescription("Global amplitude multiplier for all waves. 0 = flat ocean.");
        this->wave0Amplitude->setDescription("Wave 0: height of wave crests in world units.");
        this->wave0Frequency->setDescription("Wave 0: spatial frequency (cycles per metre).");
        this->wave0Speed->setDescription("Wave 0: phase speed in metres per second.");
        this->wave0Direction->setDescription("Wave 0: propagation direction angle in radians (0 = +X axis).");
        this->wave1Amplitude->setDescription("Wave 1: height of wave crests in world units.");
        this->wave1Frequency->setDescription("Wave 1: spatial frequency.");
        this->wave1Speed->setDescription("Wave 1: phase speed.");
        this->wave1Direction->setDescription("Wave 1: propagation direction angle in radians.");
        this->wave2Amplitude->setDescription("Wave 2: height of wave crests in world units.");
        this->wave2Frequency->setDescription("Wave 2: spatial frequency.");
        this->wave2Speed->setDescription("Wave 2: phase speed.");
        this->wave2Direction->setDescription("Wave 2: propagation direction angle in radians.");
        this->wave3Amplitude->setDescription("Wave 3: height of wave crests in world units.");
        this->wave3Frequency->setDescription("Wave 3: spatial frequency.");
        this->wave3Speed->setDescription("Wave 3: phase speed.");
        this->wave3Direction->setDescription("Wave 3: propagation direction angle in radians.");

        this->addAttributeFilePathData(this->reflectionMap);
    }

    PlanetOceanComponent::~PlanetOceanComponent()
    {
    }

    void PlanetOceanComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PlanetOceanComponent>(PlanetOceanComponent::getStaticClassId(), PlanetOceanComponent::getStaticClassName());
    }

    void PlanetOceanComponent::initialise()
    {
    }

    void PlanetOceanComponent::shutdown()
    {
    }

    void PlanetOceanComponent::uninstall()
    {
    }

    void PlanetOceanComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& PlanetOceanComponent::getName() const
    {
        return this->name;
    }

    bool PlanetOceanComponent::init(rapidxml::xml_node<>*& propertyElement)
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDeepColour())
        {
            this->deepColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShallowColour())
        {
            this->shallowColour->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoughness())
        {
            this->roughness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMetalness())
        {
            this->metalness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrTransparency())
        {
            this->transparency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrReflectionMap())
        {
            this->reflectionMap->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWaveAmplitudeScale())
        {
            this->waveAmplitudeScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave0Amplitude())
        {
            this->wave0Amplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave0Frequency())
        {
            this->wave0Frequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave0Speed())
        {
            this->wave0Speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave0Direction())
        {
            this->wave0Direction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave1Amplitude())
        {
            this->wave1Amplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave1Frequency())
        {
            this->wave1Frequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave1Speed())
        {
            this->wave1Speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave1Direction())
        {
            this->wave1Direction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave2Amplitude())
        {
            this->wave2Amplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave2Frequency())
        {
            this->wave2Frequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave2Speed())
        {
            this->wave2Speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave2Direction())
        {
            this->wave2Direction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave3Amplitude())
        {
            this->wave3Amplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave3Frequency())
        {
            this->wave3Frequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave3Speed())
        {
            this->wave3Speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWave3Direction())
        {
            this->wave3Direction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    bool PlanetOceanComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetOceanComponent] Init component for game object: " + this->gameObjectPtr->getName());

        if (true == this->activated->getBool())
        {
            this->createOcean();
        }

        this->postInitDone = true;
        return true;
    }

    bool PlanetOceanComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool PlanetOceanComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    void PlanetOceanComponent::onRemoveComponent(void)
    {
        this->destroyOcean();
        GameObjectComponent::onRemoveComponent();
    }

    void PlanetOceanComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (nullptr == this->ocean)
        {
            return;
        }
        if (false == this->activated->getBool())
        {
            return;
        }
        if (true == notSimulating)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, dt]()
        {
            this->ocean->update(static_cast<float>(dt));
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::update");
    }

    void PlanetOceanComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (PlanetOceanComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PlanetOceanComponent::AttrRadius() == attribute->getName() || PlanetOceanComponent::AttrSegmentsH() == attribute->getName() || PlanetOceanComponent::AttrSegmentsV() == attribute->getName() ||
                 PlanetOceanComponent::AttrDatablock() == attribute->getName())
        {
            if (true == this->postInitDone)
            {
                this->destroyOcean();
                this->createOcean();
            }
        }
        else if (PlanetOceanComponent::AttrDeepColour() == attribute->getName())
        {
            this->setDeepColour(attribute->getVector3());
        }
        else if (PlanetOceanComponent::AttrShallowColour() == attribute->getName())
        {
            this->setShallowColour(attribute->getVector3());
        }
        else if (PlanetOceanComponent::AttrRoughness() == attribute->getName())
        {
            this->setRoughness(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrMetalness() == attribute->getName())
        {
            this->setMetalness(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrTransparency() == attribute->getName())
        {
            this->setTransparency(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrReflectionMap() == attribute->getName())
        {
            this->setReflectionMap(attribute->getString());
        }
        else if (PlanetOceanComponent::AttrWaveAmplitudeScale() == attribute->getName())
        {
            this->setWaveAmplitudeScale(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave0Amplitude() == attribute->getName() || PlanetOceanComponent::AttrWave0Frequency() == attribute->getName() || PlanetOceanComponent::AttrWave0Speed() == attribute->getName() ||
                 PlanetOceanComponent::AttrWave0Direction() == attribute->getName())
        {
            this->applyWaveParam(0);
        }
        else if (PlanetOceanComponent::AttrWave1Amplitude() == attribute->getName() || PlanetOceanComponent::AttrWave1Frequency() == attribute->getName() || PlanetOceanComponent::AttrWave1Speed() == attribute->getName() ||
                 PlanetOceanComponent::AttrWave1Direction() == attribute->getName())
        {
            this->applyWaveParam(1);
        }
        else if (PlanetOceanComponent::AttrWave2Amplitude() == attribute->getName() || PlanetOceanComponent::AttrWave2Frequency() == attribute->getName() || PlanetOceanComponent::AttrWave2Speed() == attribute->getName() ||
                 PlanetOceanComponent::AttrWave2Direction() == attribute->getName())
        {
            this->applyWaveParam(2);
        }
        else if (PlanetOceanComponent::AttrWave3Amplitude() == attribute->getName() || PlanetOceanComponent::AttrWave3Frequency() == attribute->getName() || PlanetOceanComponent::AttrWave3Speed() == attribute->getName() ||
                 PlanetOceanComponent::AttrWave3Direction() == attribute->getName())
        {
            this->applyWaveParam(3);
        }
    }

    void PlanetOceanComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int, 6 = real, 7 = string, 9 = vector3, 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Radius"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SegmentsH"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->segmentsH->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SegmentsV"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->segmentsV->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Datablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Deep Colour"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->deepColour->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Shallow Colour"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shallowColour->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Transparency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->transparency->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Reflection Map"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reflectionMap->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave Amplitude Scale"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waveAmplitudeScale->getReal())));
        propertiesXML->append_node(propertyXML);

        // Wave 0
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave0 Amplitude"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave0Amplitude->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave0 Frequency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave0Frequency->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave0 Speed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave0Speed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave0 Direction"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave0Direction->getReal())));
        propertiesXML->append_node(propertyXML);

        // Wave 1
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave1 Amplitude"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave1Amplitude->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave1 Frequency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave1Frequency->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave1 Speed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave1Speed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave1 Direction"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave1Direction->getReal())));
        propertiesXML->append_node(propertyXML);

        // Wave 2
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave2 Amplitude"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave2Amplitude->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave2 Frequency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave2Frequency->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave2 Speed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave2Speed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave2 Direction"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave2Direction->getReal())));
        propertiesXML->append_node(propertyXML);

        // Wave 3
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave3 Amplitude"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave3Amplitude->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave3 Frequency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave3Frequency->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave3 Speed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave3Speed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wave3 Direction"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wave3Direction->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String PlanetOceanComponent::getClassName(void) const
    {
        return "PlanetOceanComponent";
    }

    Ogre::String PlanetOceanComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    GameObjectCompPtr PlanetOceanComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PlanetOceanComponentPtr clonedCompPtr(boost::make_shared<PlanetOceanComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->radius->setValue(this->radius->getReal());
        clonedCompPtr->segmentsH->setValue(this->segmentsH->getUInt());
        clonedCompPtr->segmentsV->setValue(this->segmentsV->getUInt());
        clonedCompPtr->datablock->setValue(this->datablock->getString());
        clonedCompPtr->deepColour->setValue(this->deepColour->getVector3());
        clonedCompPtr->shallowColour->setValue(this->shallowColour->getVector3());
        clonedCompPtr->roughness->setValue(this->roughness->getReal());
        clonedCompPtr->metalness->setValue(this->metalness->getReal());
        clonedCompPtr->transparency->setValue(this->transparency->getReal());
        clonedCompPtr->reflectionMap->setValue(this->reflectionMap->getString());
        clonedCompPtr->waveAmplitudeScale->setValue(this->waveAmplitudeScale->getReal());
        clonedCompPtr->wave0Amplitude->setValue(this->wave0Amplitude->getReal());
        clonedCompPtr->wave0Frequency->setValue(this->wave0Frequency->getReal());
        clonedCompPtr->wave0Speed->setValue(this->wave0Speed->getReal());
        clonedCompPtr->wave0Direction->setValue(this->wave0Direction->getReal());
        clonedCompPtr->wave1Amplitude->setValue(this->wave1Amplitude->getReal());
        clonedCompPtr->wave1Frequency->setValue(this->wave1Frequency->getReal());
        clonedCompPtr->wave1Speed->setValue(this->wave1Speed->getReal());
        clonedCompPtr->wave1Direction->setValue(this->wave1Direction->getReal());
        clonedCompPtr->wave2Amplitude->setValue(this->wave2Amplitude->getReal());
        clonedCompPtr->wave2Frequency->setValue(this->wave2Frequency->getReal());
        clonedCompPtr->wave2Speed->setValue(this->wave2Speed->getReal());
        clonedCompPtr->wave2Direction->setValue(this->wave2Direction->getReal());
        clonedCompPtr->wave3Amplitude->setValue(this->wave3Amplitude->getReal());
        clonedCompPtr->wave3Frequency->setValue(this->wave3Frequency->getReal());
        clonedCompPtr->wave3Speed->setValue(this->wave3Speed->getReal());
        clonedCompPtr->wave3Direction->setValue(this->wave3Direction->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    // =========================================================================
    //  Setters / Getters
    // =========================================================================

    void PlanetOceanComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated && nullptr == this->ocean && true == this->postInitDone)
        {
            this->createOcean();
        }
        else if (false == activated && nullptr != this->ocean)
        {
            this->destroyOcean();
        }
    }

    bool PlanetOceanComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void PlanetOceanComponent::setRadius(Ogre::Real radius)
    {
        this->radius->setValue(radius);
        if (nullptr != this->ocean && true == this->postInitDone)
        {
            this->destroyOcean();
            this->createOcean();
        }
    }

    Ogre::Real PlanetOceanComponent::getRadius(void) const
    {
        return this->radius->getReal();
    }

    void PlanetOceanComponent::setDeepColour(const Ogre::Vector3& colour)
    {
        this->deepColour->setValue(colour);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, colour]()
            {
                this->ocean->setDeepColour(colour);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::setDeepColour");
        }
    }

    Ogre::Vector3 PlanetOceanComponent::getDeepColour(void) const
    {
        return this->deepColour->getVector3();
    }

    void PlanetOceanComponent::setShallowColour(const Ogre::Vector3& colour)
    {
        this->shallowColour->setValue(colour);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, colour]()
            {
                this->ocean->setShallowColour(colour);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::setShallowColour");
        }
    }

    Ogre::Vector3 PlanetOceanComponent::getShallowColour(void) const
    {
        return this->shallowColour->getVector3();
    }

    void PlanetOceanComponent::setRoughness(Ogre::Real roughness)
    {
        this->roughness->setValue(roughness);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, roughness]()
            {
                this->ocean->setRoughness(roughness);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::setRoughness");
        }
    }

    Ogre::Real PlanetOceanComponent::getRoughness(void) const
    {
        return this->roughness->getReal();
    }

    void PlanetOceanComponent::setMetalness(Ogre::Real metalness)
    {
        this->metalness->setValue(metalness);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, metalness]()
            {
                Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->ocean->getItem()->getSubItem(0)->getDatablock());
                if (nullptr != db)
                {
                    db->setMetalness(metalness);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::setMetalness");
        }
    }

    Ogre::Real PlanetOceanComponent::getMetalness(void) const
    {
        return this->metalness->getReal();
    }

    void PlanetOceanComponent::setTransparency(Ogre::Real transparency)
    {
        this->transparency->setValue(transparency);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, transparency]()
            {
                this->ocean->setTransparency(transparency);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::setTransparency");
        }
    }

    Ogre::Real PlanetOceanComponent::getTransparency(void) const
    {
        return this->transparency->getReal();
    }

    void PlanetOceanComponent::setReflectionMap(const Ogre::String& cubemapName)
    {
        this->reflectionMap->setValue(cubemapName);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, cubemapName]()
            {
                this->ocean->setReflectionTextureName(cubemapName);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::setReflectionMap");
        }
    }

    Ogre::String PlanetOceanComponent::getReflectionMap(void) const
    {
        return this->reflectionMap->getString();
    }

    void PlanetOceanComponent::setWaveAmplitudeScale(Ogre::Real scale)
    {
        this->waveAmplitudeScale->setValue(scale);
        if (nullptr != this->ocean)
        {
            this->ocean->setWaveAmplitudeScale(scale);
        }
    }

    Ogre::Real PlanetOceanComponent::getWaveAmplitudeScale(void) const
    {
        return this->waveAmplitudeScale->getReal();
    }

    PlanetOcean* PlanetOceanComponent::getOcean(void) const
    {
        return this->ocean;
    }

    // =========================================================================
    //  Private helpers
    // =========================================================================

    void PlanetOceanComponent::createOcean(void)
    {
        if (nullptr != this->ocean)
        {
            return;
        }

        this->ocean = new PlanetOcean(this->gameObjectPtr->getName(), this->gameObjectPtr->getSceneManager());

        for (int w = 0; w < 4; ++w)
        {
            this->applyWaveParam(w);
        }
        this->ocean->setWaveAmplitudeScale(this->waveAmplitudeScale->getReal());

        const float r = this->radius->getReal();
        const int segH = static_cast<int>(this->segmentsH->getUInt());
        const int segV = static_cast<int>(this->segmentsV->getUInt());
        const Ogre::String dbName = this->datablock->getString();
        const Ogre::Vector3 dc = this->deepColour->getVector3();
        const Ogre::Vector3 sc = this->shallowColour->getVector3();
        const float rou = this->roughness->getReal();
        const float met = this->metalness->getReal();
        const float tra = this->transparency->getReal();
        const Ogre::String refMap = this->reflectionMap->getString();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, r, segH, segV, dbName, dc, sc, rou, met, tra, refMap]()
        {
            if (false == this->ocean->create(r, segH, segV, this->gameObjectPtr->getSceneNode(), dbName))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetOceanComponent] Failed to create ocean for '" + this->gameObjectPtr->getName() + "'.");
                return;
            }

            this->ocean->setDeepColour(dc);
            this->ocean->setShallowColour(sc);
            this->ocean->setRoughness(rou);
            this->ocean->setTransparency(tra);

            Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->ocean->getItem()->getSubItem(0)->getDatablock());
            if (nullptr != db)
            {
                db->setMetalness(met);
            }

            if (false == refMap.empty())
            {
                this->ocean->setReflectionTextureName(refMap);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::createOcean");

        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
    }

    void PlanetOceanComponent::destroyOcean(void)
    {
        if (nullptr == this->ocean)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->ocean->destroy();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetOceanComponent::destroyOcean");

        delete this->ocean;
        this->ocean = nullptr;
    }

    void PlanetOceanComponent::applyWaveParam(int waveIndex)
    {
        if (nullptr == this->ocean)
        {
            return;
        }

        PlanetOcean::WaveParams p = this->ocean->getWave(waveIndex);

        if (waveIndex == 0)
        {
            p.amplitude = this->wave0Amplitude->getReal();
            p.frequency = this->wave0Frequency->getReal();
            p.speed = this->wave0Speed->getReal();
            p.directionAngle = this->wave0Direction->getReal();
        }
        else if (waveIndex == 1)
        {
            p.amplitude = this->wave1Amplitude->getReal();
            p.frequency = this->wave1Frequency->getReal();
            p.speed = this->wave1Speed->getReal();
            p.directionAngle = this->wave1Direction->getReal();
        }
        else if (waveIndex == 2)
        {
            p.amplitude = this->wave2Amplitude->getReal();
            p.frequency = this->wave2Frequency->getReal();
            p.speed = this->wave2Speed->getReal();
            p.directionAngle = this->wave2Direction->getReal();
        }
        else if (waveIndex == 3)
        {
            p.amplitude = this->wave3Amplitude->getReal();
            p.frequency = this->wave3Frequency->getReal();
            p.speed = this->wave3Speed->getReal();
            p.directionAngle = this->wave3Direction->getReal();
        }

        this->ocean->setWave(waveIndex, p);
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    PlanetOceanComponent* getPlanetOceanComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<PlanetOceanComponent>(gameObject->getComponentWithOccurrence<PlanetOceanComponent>(occurrenceIndex)).get();
    }

    PlanetOceanComponent* getPlanetOceanComponent(GameObject* gameObject)
    {
        return makeStrongPtr<PlanetOceanComponent>(gameObject->getComponent<PlanetOceanComponent>()).get();
    }

    PlanetOceanComponent* getPlanetOceanComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<PlanetOceanComponent>(gameObject->getComponentFromName<PlanetOceanComponent>(name)).get();
    }

    void PlanetOceanComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<PlanetOceanComponent, GameObjectComponent>("PlanetOceanComponent")
                .def("setActivated", &PlanetOceanComponent::setActivated)
                .def("isActivated", &PlanetOceanComponent::isActivated)
                .def("setRadius", &PlanetOceanComponent::setRadius)
                .def("getRadius", &PlanetOceanComponent::getRadius)
                .def("setDeepColour", &PlanetOceanComponent::setDeepColour)
                .def("getDeepColour", &PlanetOceanComponent::getDeepColour)
                .def("setShallowColour", &PlanetOceanComponent::setShallowColour)
                .def("getShallowColour", &PlanetOceanComponent::getShallowColour)
                .def("setRoughness", &PlanetOceanComponent::setRoughness)
                .def("getRoughness", &PlanetOceanComponent::getRoughness)
                .def("setMetalness", &PlanetOceanComponent::setMetalness)
                .def("getMetalness", &PlanetOceanComponent::getMetalness)
                .def("setTransparency", &PlanetOceanComponent::setTransparency)
                .def("getTransparency", &PlanetOceanComponent::getTransparency)
                .def("setReflectionMap", &PlanetOceanComponent::setReflectionMap)
                .def("getReflectionMap", &PlanetOceanComponent::getReflectionMap)
                .def("setWaveAmplitudeScale", &PlanetOceanComponent::setWaveAmplitudeScale)
                .def("getWaveAmplitudeScale", &PlanetOceanComponent::getWaveAmplitudeScale)];

        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "class inherits GameObjectComponent", PlanetOceanComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setActivated(bool activated)", "Enables or disables the ocean animation.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "bool isActivated()", "Gets whether the ocean is active.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setRadius(float radius)", "Sets the ocean sphere radius.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getRadius()", "Gets the ocean sphere radius.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setDeepColour(Vector3 colour)", "Sets the deep ocean colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "Vector3 getDeepColour()", "Gets the deep ocean colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setShallowColour(Vector3 colour)", "Sets the shallow water colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "Vector3 getShallowColour()", "Gets the shallow water colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setRoughness(float roughness)", "Sets the PBS roughness.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getRoughness()", "Gets the PBS roughness.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setTransparency(float transparency)", "Sets the water transparency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getTransparency()", "Gets the water transparency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWaveAmplitudeScale(float scale)", "Sets the global wave amplitude scale.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWaveAmplitudeScale()", "Gets the global wave amplitude scale.");

        gameObjectClass.def("getPlanetOceanComponentFromName", &getPlanetOceanComponentFromName);
        gameObjectClass.def("getPlanetOceanComponent", (PlanetOceanComponent * (*)(GameObject*)) & getPlanetOceanComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetOceanComponent getPlanetOceanComponent()", "Gets the PlanetOceanComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetOceanComponent getPlanetOceanComponentFromName(String name)", "Gets the PlanetOceanComponent by name.");

        gameObjectControllerClass.def("castPlanetOceanComponent", &GameObjectController::cast<PlanetOceanComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PlanetOceanComponent castPlanetOceanComponent(PlanetOceanComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    bool PlanetOceanComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return true;
    }

} // namespace NOWA