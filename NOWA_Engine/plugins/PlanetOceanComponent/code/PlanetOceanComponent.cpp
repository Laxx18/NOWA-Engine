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
        segmentsH(new Variant(PlanetOceanComponent::AttrSegmentsH(), static_cast<unsigned int>(32), this->attributes)),
        segmentsV(new Variant(PlanetOceanComponent::AttrSegmentsV(), static_cast<unsigned int>(32), this->attributes)),
        datablock(new Variant(PlanetOceanComponent::AttrDatablock(), PlanetOceanComponent::DefaultMaterialName(), this->attributes)),
        deepColour(new Variant(PlanetOceanComponent::AttrDeepColour(), Ogre::Vector3(0.0f, 0.03f, 0.07f), this->attributes)),
        shallowColour(new Variant(PlanetOceanComponent::AttrShallowColour(), Ogre::Vector3(0.0f, 0.12f, 0.18f), this->attributes)),
        roughness(new Variant(PlanetOceanComponent::AttrRoughness(), 0.08f, this->attributes)),
        transparency(new Variant(PlanetOceanComponent::AttrTransparency(), 0.95f, this->attributes)),
        reflectionMap(new Variant(PlanetOceanComponent::AttrReflectionMap(), Ogre::String(""), this->attributes)),
        waveAmplitudeScale(new Variant(PlanetOceanComponent::AttrWaveAmplitudeScale(), 1.0f, this->attributes)),
        wave0Amplitude(new Variant(PlanetOceanComponent::AttrWave0Amplitude(), 0.2f, this->attributes)),
        wave0Frequency(new Variant(PlanetOceanComponent::AttrWave0Frequency(), 2.0f, this->attributes)),
        wave0Speed(new Variant(PlanetOceanComponent::AttrWave0Speed(), 0.8f, this->attributes)),
        wave0Direction(new Variant(PlanetOceanComponent::AttrWave0Direction(), 0.0f, this->attributes)),
        wave1Amplitude(new Variant(PlanetOceanComponent::AttrWave1Amplitude(), 0.05f, this->attributes)),
        wave1Frequency(new Variant(PlanetOceanComponent::AttrWave1Frequency(), 3.0f, this->attributes)),
        wave1Speed(new Variant(PlanetOceanComponent::AttrWave1Speed(), 0.3f, this->attributes)),
        wave1Direction(new Variant(PlanetOceanComponent::AttrWave1Direction(), 1.2f, this->attributes)),
        wave2Amplitude(new Variant(PlanetOceanComponent::AttrWave2Amplitude(), 0.03f, this->attributes)),
        wave2Frequency(new Variant(PlanetOceanComponent::AttrWave2Frequency(), 5.0f, this->attributes)),
        wave2Speed(new Variant(PlanetOceanComponent::AttrWave2Speed(), 0.4f, this->attributes)),
        wave2Direction(new Variant(PlanetOceanComponent::AttrWave2Direction(), 2.7f, this->attributes)),
        wave3Amplitude(new Variant(PlanetOceanComponent::AttrWave3Amplitude(), 0.02f, this->attributes)),
        wave3Frequency(new Variant(PlanetOceanComponent::AttrWave3Frequency(), 8.0f, this->attributes)),
        wave3Speed(new Variant(PlanetOceanComponent::AttrWave3Speed(), 0.6f, this->attributes)),
        wave3Direction(new Variant(PlanetOceanComponent::AttrWave3Direction(), 4.3f, this->attributes))
    {
        this->activated->setDescription("Enable or disable the ocean animation.");
        this->radius->setDescription("Ocean sphere radius. MUST be larger than terrain radius + hill amplitude to avoid "
                                     "terrain hills poking through the ocean surface. Example: terrain radius=50, amplitude=5 -> hills reach 55m, "
                                     "so ocean radius must be at least 56 or higher.");
        this->segmentsH->setDescription("Horizontal tessellation of the sphere. 32 is sufficient for smooth waves.");
        this->segmentsV->setDescription("Vertical tessellation of the sphere.");
        this->datablock->setDescription("PBS material name. Defaults to PlanetOceanDefaultMaterial.");
        this->deepColour->setDescription("Deep ocean diffuse colour.");
        this->shallowColour->setDescription("Shallow water emissive tint blended in at wave crests.");
        this->roughness->setDescription("PBS roughness [0.001, 1]. Uses specular_fresnel workflow. 0.06 = calm ocean. Keep metalness at 0 for water.");
        this->transparency->setDescription("Water transparency [0=opaque, 1=fully transparent].");
        this->reflectionMap->setDescription("Optional cubemap texture name for environment reflections. Leave empty to skip.");
        this->waveAmplitudeScale->setDescription("Global amplitude multiplier for all waves. 0 = flat ocean. Amplitudes scale automatically with planet radius.");
        this->wave0Amplitude->setDescription("Wave 0: crest height in metres on a 50m reference planet. Scales with radius.");
        this->wave0Frequency->setDescription("Wave 0: cycles per hemisphere (radius-independent). 2 = two crests across equator.");
        this->wave0Speed->setDescription("Wave 0: phase speed in radians per second.");
        this->wave0Direction->setDescription("Wave 0: propagation direction angle in radians (0 = +X axis).");
        this->wave1Amplitude->setDescription("Wave 1: crest height in metres on a 50m reference planet.");
        this->wave1Frequency->setDescription("Wave 1: cycles per hemisphere.");
        this->wave1Speed->setDescription("Wave 1: phase speed in radians per second.");
        this->wave1Direction->setDescription("Wave 1: propagation direction angle in radians.");
        this->wave2Amplitude->setDescription("Wave 2: crest height in metres on a 50m reference planet.");
        this->wave2Frequency->setDescription("Wave 2: cycles per hemisphere.");
        this->wave2Speed->setDescription("Wave 2: phase speed in radians per second.");
        this->wave2Direction->setDescription("Wave 2: propagation direction angle in radians.");
        this->wave3Amplitude->setDescription("Wave 3: crest height in metres on a 50m reference planet.");
        this->wave3Frequency->setDescription("Wave 3: cycles per hemisphere.");
        this->wave3Speed->setDescription("Wave 3: phase speed in radians per second.");
        this->wave3Direction->setDescription("Wave 3: propagation direction angle in radians.");

        this->addAttributeFilePathData(this->reflectionMap);
    }

    PlanetOceanComponent::~PlanetOceanComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin interface
    // =========================================================================

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

    // =========================================================================
    //  GameObjectComponent interface
    // =========================================================================

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
        {
            propertyElement = propertyElement->next_sibling("property");
        }
        {
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
        if (false == notSimulating && true == this->activated->getBool())
        {
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->oceanUpdateClosureId, [this](Ogre::Real renderDt)
            {
                if (nullptr != this->ocean && true == this->activated->getBool())
                {
                    this->ocean->update(static_cast<float>(renderDt));
                }
            }, false);
        }
    }

    void PlanetOceanComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (PlanetOceanComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PlanetOceanComponent::AttrRadius() == attribute->getName())
        {
            this->setRadius(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrSegmentsH() == attribute->getName())
        {
            this->segmentsH->setValue(attribute->getUInt());
            if (true == this->postInitDone)
            {
                this->destroyOcean();
                this->createOcean();
            }
        }
        else if (PlanetOceanComponent::AttrSegmentsV() == attribute->getName())
        {
            this->segmentsV->setValue(attribute->getUInt());
            if (true == this->postInitDone)
            {
                this->destroyOcean();
                this->createOcean();
            }
        }
        else if (PlanetOceanComponent::AttrDatablock() == attribute->getName())
        {
            this->datablock->setValue(attribute->getString());
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
        else if (PlanetOceanComponent::AttrWave0Amplitude() == attribute->getName())
        {
            this->setWave0Amplitude(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave0Frequency() == attribute->getName())
        {
            this->setWave0Frequency(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave0Speed() == attribute->getName())
        {
            this->setWave0Speed(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave0Direction() == attribute->getName())
        {
            this->setWave0Direction(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave1Amplitude() == attribute->getName())
        {
            this->setWave1Amplitude(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave1Frequency() == attribute->getName())
        {
            this->setWave1Frequency(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave1Speed() == attribute->getName())
        {
            this->setWave1Speed(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave1Direction() == attribute->getName())
        {
            this->setWave1Direction(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave2Amplitude() == attribute->getName())
        {
            this->setWave2Amplitude(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave2Frequency() == attribute->getName())
        {
            this->setWave2Frequency(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave2Speed() == attribute->getName())
        {
            this->setWave2Speed(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave2Direction() == attribute->getName())
        {
            this->setWave2Direction(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave3Amplitude() == attribute->getName())
        {
            this->setWave3Amplitude(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave3Frequency() == attribute->getName())
        {
            this->setWave3Frequency(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave3Speed() == attribute->getName())
        {
            this->setWave3Speed(attribute->getReal());
        }
        else if (PlanetOceanComponent::AttrWave3Direction() == attribute->getName())
        {
            this->setWave3Direction(attribute->getReal());
        }
    }

    void PlanetOceanComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2=uint, 6=real, 7=string, 9=vector3, 12=bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        auto writeAttr = [&](const char* type, const char* attrName, const Ogre::String& data)
        {
            xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", type));
            propertyXML->append_attribute(doc.allocate_attribute("name", attrName));
            propertyXML->append_attribute(doc.allocate_attribute("data", doc.allocate_string(data.c_str())));
            propertiesXML->append_node(propertyXML);
        };

        writeAttr("12", "Activated", XMLConverter::ConvertString(doc, this->activated->getBool()));
        writeAttr("6", "Radius", XMLConverter::ConvertString(doc, this->radius->getReal()));
        writeAttr("2", "SegmentsH", XMLConverter::ConvertString(doc, this->segmentsH->getUInt()));
        writeAttr("2", "SegmentsV", XMLConverter::ConvertString(doc, this->segmentsV->getUInt()));
        writeAttr("7", "Datablock", XMLConverter::ConvertString(doc, this->datablock->getString()));
        writeAttr("9", "Deep Colour", XMLConverter::ConvertString(doc, this->deepColour->getVector3()));
        writeAttr("9", "Shallow Colour", XMLConverter::ConvertString(doc, this->shallowColour->getVector3()));
        writeAttr("6", "Roughness", XMLConverter::ConvertString(doc, this->roughness->getReal()));
        writeAttr("6", "Transparency", XMLConverter::ConvertString(doc, this->transparency->getReal()));
        writeAttr("7", "Reflection Map", XMLConverter::ConvertString(doc, this->reflectionMap->getString()));
        writeAttr("6", "Wave Amplitude Scale", XMLConverter::ConvertString(doc, this->waveAmplitudeScale->getReal()));
        writeAttr("6", "Wave0 Amplitude", XMLConverter::ConvertString(doc, this->wave0Amplitude->getReal()));
        writeAttr("6", "Wave0 Frequency", XMLConverter::ConvertString(doc, this->wave0Frequency->getReal()));
        writeAttr("6", "Wave0 Speed", XMLConverter::ConvertString(doc, this->wave0Speed->getReal()));
        writeAttr("6", "Wave0 Direction", XMLConverter::ConvertString(doc, this->wave0Direction->getReal()));
        writeAttr("6", "Wave1 Amplitude", XMLConverter::ConvertString(doc, this->wave1Amplitude->getReal()));
        writeAttr("6", "Wave1 Frequency", XMLConverter::ConvertString(doc, this->wave1Frequency->getReal()));
        writeAttr("6", "Wave1 Speed", XMLConverter::ConvertString(doc, this->wave1Speed->getReal()));
        writeAttr("6", "Wave1 Direction", XMLConverter::ConvertString(doc, this->wave1Direction->getReal()));
        writeAttr("6", "Wave2 Amplitude", XMLConverter::ConvertString(doc, this->wave2Amplitude->getReal()));
        writeAttr("6", "Wave2 Frequency", XMLConverter::ConvertString(doc, this->wave2Frequency->getReal()));
        writeAttr("6", "Wave2 Speed", XMLConverter::ConvertString(doc, this->wave2Speed->getReal()));
        writeAttr("6", "Wave2 Direction", XMLConverter::ConvertString(doc, this->wave2Direction->getReal()));
        writeAttr("6", "Wave3 Amplitude", XMLConverter::ConvertString(doc, this->wave3Amplitude->getReal()));
        writeAttr("6", "Wave3 Frequency", XMLConverter::ConvertString(doc, this->wave3Frequency->getReal()));
        writeAttr("6", "Wave3 Speed", XMLConverter::ConvertString(doc, this->wave3Speed->getReal()));
        writeAttr("6", "Wave3 Direction", XMLConverter::ConvertString(doc, this->wave3Direction->getReal()));
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
        if (true == this->postInitDone)
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
            NOWA::GraphicsModule::RenderCommand cmd = [this, colour]()
            {
                this->ocean->setDeepColour(colour);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetOceanComponent::setDeepColour");
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
            NOWA::GraphicsModule::RenderCommand cmd = [this, colour]()
            {
                this->ocean->setShallowColour(colour);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetOceanComponent::setShallowColour");
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
            NOWA::GraphicsModule::RenderCommand cmd = [this, roughness]()
            {
                this->ocean->setRoughness(roughness);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetOceanComponent::setRoughness");
        }
    }

    Ogre::Real PlanetOceanComponent::getRoughness(void) const
    {
        return this->roughness->getReal();
    }

    void PlanetOceanComponent::setTransparency(Ogre::Real transparency)
    {
        this->transparency->setValue(transparency);
        if (nullptr != this->ocean)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, transparency]()
            {
                this->ocean->setTransparency(transparency);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetOceanComponent::setTransparency");
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
            NOWA::GraphicsModule::RenderCommand cmd = [this, cubemapName]()
            {
                this->ocean->setReflectionTextureName(cubemapName);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetOceanComponent::setReflectionMap");
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

    void PlanetOceanComponent::setWave0Amplitude(Ogre::Real v)
    {
        this->wave0Amplitude->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(0);
            p.amplitude = v;
            this->ocean->setWave(0, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave0Amplitude(void) const
    {
        return this->wave0Amplitude->getReal();
    }

    void PlanetOceanComponent::setWave0Frequency(Ogre::Real v)
    {
        this->wave0Frequency->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(0);
            p.frequency = v;
            this->ocean->setWave(0, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave0Frequency(void) const
    {
        return this->wave0Frequency->getReal();
    }

    void PlanetOceanComponent::setWave0Speed(Ogre::Real v)
    {
        this->wave0Speed->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(0);
            p.speed = v;
            this->ocean->setWave(0, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave0Speed(void) const
    {
        return this->wave0Speed->getReal();
    }

    void PlanetOceanComponent::setWave0Direction(Ogre::Real v)
    {
        this->wave0Direction->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(0);
            p.directionAngle = v;
            this->ocean->setWave(0, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave0Direction(void) const
    {
        return this->wave0Direction->getReal();
    }

    void PlanetOceanComponent::setWave1Amplitude(Ogre::Real v)
    {
        this->wave1Amplitude->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(1);
            p.amplitude = v;
            this->ocean->setWave(1, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave1Amplitude(void) const
    {
        return this->wave1Amplitude->getReal();
    }

    void PlanetOceanComponent::setWave1Frequency(Ogre::Real v)
    {
        this->wave1Frequency->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(1);
            p.frequency = v;
            this->ocean->setWave(1, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave1Frequency(void) const
    {
        return this->wave1Frequency->getReal();
    }

    void PlanetOceanComponent::setWave1Speed(Ogre::Real v)
    {
        this->wave1Speed->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(1);
            p.speed = v;
            this->ocean->setWave(1, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave1Speed(void) const
    {
        return this->wave1Speed->getReal();
    }

    void PlanetOceanComponent::setWave1Direction(Ogre::Real v)
    {
        this->wave1Direction->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(1);
            p.directionAngle = v;
            this->ocean->setWave(1, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave1Direction(void) const
    {
        return this->wave1Direction->getReal();
    }

    void PlanetOceanComponent::setWave2Amplitude(Ogre::Real v)
    {
        this->wave2Amplitude->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(2);
            p.amplitude = v;
            this->ocean->setWave(2, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave2Amplitude(void) const
    {
        return this->wave2Amplitude->getReal();
    }

    void PlanetOceanComponent::setWave2Frequency(Ogre::Real v)
    {
        this->wave2Frequency->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(2);
            p.frequency = v;
            this->ocean->setWave(2, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave2Frequency(void) const
    {
        return this->wave2Frequency->getReal();
    }

    void PlanetOceanComponent::setWave2Speed(Ogre::Real v)
    {
        this->wave2Speed->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(2);
            p.speed = v;
            this->ocean->setWave(2, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave2Speed(void) const
    {
        return this->wave2Speed->getReal();
    }

    void PlanetOceanComponent::setWave2Direction(Ogre::Real v)
    {
        this->wave2Direction->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(2);
            p.directionAngle = v;
            this->ocean->setWave(2, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave2Direction(void) const
    {
        return this->wave2Direction->getReal();
    }

    void PlanetOceanComponent::setWave3Amplitude(Ogre::Real v)
    {
        this->wave3Amplitude->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(3);
            p.amplitude = v;
            this->ocean->setWave(3, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave3Amplitude(void) const
    {
        return this->wave3Amplitude->getReal();
    }

    void PlanetOceanComponent::setWave3Frequency(Ogre::Real v)
    {
        this->wave3Frequency->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(3);
            p.frequency = v;
            this->ocean->setWave(3, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave3Frequency(void) const
    {
        return this->wave3Frequency->getReal();
    }

    void PlanetOceanComponent::setWave3Speed(Ogre::Real v)
    {
        this->wave3Speed->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(3);
            p.speed = v;
            this->ocean->setWave(3, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave3Speed(void) const
    {
        return this->wave3Speed->getReal();
    }

    void PlanetOceanComponent::setWave3Direction(Ogre::Real v)
    {
        this->wave3Direction->setValue(v);
        if (nullptr != this->ocean)
        {
            PlanetOcean::WaveParams p = this->ocean->getWave(3);
            p.directionAngle = v;
            this->ocean->setWave(3, p);
        }
    }
    Ogre::Real PlanetOceanComponent::getWave3Direction(void) const
    {
        return this->wave3Direction->getReal();
    }

    PlanetOcean* PlanetOceanComponent::getOcean(void) const
    {
        return this->ocean;
    }

    void PlanetOceanComponent::setDynamic(bool dynamic)
    {
        if (nullptr != this->ocean)
        {
            this->ocean->setDynamic(dynamic);
        }
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
        const Ogre::String db = this->datablock->getString();
        const Ogre::Vector3 dc = this->deepColour->getVector3();
        const Ogre::Vector3 sc = this->shallowColour->getVector3();
        const float rou = this->roughness->getReal();
        const float tra = this->transparency->getReal();
        const Ogre::String ref = this->reflectionMap->getString();

        NOWA::GraphicsModule::RenderCommand renderCmd = [this, r, segH, segV, db, dc, sc, rou, tra, ref]()
        {
            if (false == this->ocean->create(r, segH, segV, this->gameObjectPtr->getSceneNode(), db))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetOceanComponent] Failed to create ocean for '" + this->gameObjectPtr->getName() + "'.");
                return;
            }
            this->ocean->setDeepColour(dc);
            this->ocean->setShallowColour(sc);
            this->ocean->setRoughness(rou);
            this->ocean->setTransparency(tra);
            if (false == ref.empty())
            {
                this->ocean->setReflectionTextureName(ref);
            }

            // Must not be done: PlanetOcean is no own mesh, its belongs to PlanetTerraComponent, so the lines below, would overwrite the item for PlanetTerra and wrong collision hull would be created.
            //// Register ocean item with game object so movableObject is valid.
            //// doNotDestroyMovableObject=true so init() won't destroy the planet item.
            //// It only updates movableObject pointer + flags.
            //this->gameObjectPtr->setDoNotDestroyMovableObject(true);
            //this->gameObjectPtr->init(this->ocean->getItem());
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCmd), "PlanetOceanComponent::createOcean");

        // NOTE: setDoNotDestroyMovableObject moved inside render command above.

        this->oceanUpdateClosureId = this->gameObjectPtr->getName() + PlanetOceanComponent::getStaticClassName() + "::waveUpdate";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->oceanUpdateClosureId, [this](Ogre::Real renderDt)
        {
            if (nullptr != this->ocean && true == this->activated->getBool())
            {
                this->ocean->update(static_cast<float>(renderDt));
            }
        }, false);
    }

    void PlanetOceanComponent::destroyOcean(void)
    {
        if (nullptr == this->ocean)
        {
            return;
        }

        if (false == this->oceanUpdateClosureId.empty())
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->oceanUpdateClosureId);
            this->oceanUpdateClosureId.clear();
        }

        NOWA::GraphicsModule::RenderCommand renderCmd = [this]()
        {
            this->ocean->destroy();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCmd), "PlanetOceanComponent::destroyOcean");

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

    void PlanetOceanComponent::createStaticApiForLua(lua_State* lua,luabind::class_<GameObject>& gameObjectClass,luabind::class_<GameObjectController>& gameObjectControllerClass)
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
                .def("setTransparency", &PlanetOceanComponent::setTransparency)
                .def("getTransparency", &PlanetOceanComponent::getTransparency)
                .def("setReflectionMap", &PlanetOceanComponent::setReflectionMap)
                .def("getReflectionMap", &PlanetOceanComponent::getReflectionMap)
                .def("setWaveAmplitudeScale", &PlanetOceanComponent::setWaveAmplitudeScale)
                .def("getWaveAmplitudeScale", &PlanetOceanComponent::getWaveAmplitudeScale)
                .def("setWave0Amplitude", &PlanetOceanComponent::setWave0Amplitude)
                .def("getWave0Amplitude", &PlanetOceanComponent::getWave0Amplitude)
                .def("setWave0Frequency", &PlanetOceanComponent::setWave0Frequency)
                .def("getWave0Frequency", &PlanetOceanComponent::getWave0Frequency)
                .def("setWave0Speed", &PlanetOceanComponent::setWave0Speed)
                .def("getWave0Speed", &PlanetOceanComponent::getWave0Speed)
                .def("setWave0Direction", &PlanetOceanComponent::setWave0Direction)
                .def("getWave0Direction", &PlanetOceanComponent::getWave0Direction)
                .def("setWave1Amplitude", &PlanetOceanComponent::setWave1Amplitude)
                .def("getWave1Amplitude", &PlanetOceanComponent::getWave1Amplitude)
                .def("setWave1Frequency", &PlanetOceanComponent::setWave1Frequency)
                .def("getWave1Frequency", &PlanetOceanComponent::getWave1Frequency)
                .def("setWave1Speed", &PlanetOceanComponent::setWave1Speed)
                .def("getWave1Speed", &PlanetOceanComponent::getWave1Speed)
                .def("setWave1Direction", &PlanetOceanComponent::setWave1Direction)
                .def("getWave1Direction", &PlanetOceanComponent::getWave1Direction)
                .def("setWave2Amplitude", &PlanetOceanComponent::setWave2Amplitude)
                .def("getWave2Amplitude", &PlanetOceanComponent::getWave2Amplitude)
                .def("setWave2Frequency", &PlanetOceanComponent::setWave2Frequency)
                .def("getWave2Frequency", &PlanetOceanComponent::getWave2Frequency)
                .def("setWave2Speed", &PlanetOceanComponent::setWave2Speed)
                .def("getWave2Speed", &PlanetOceanComponent::getWave2Speed)
                .def("setWave2Direction", &PlanetOceanComponent::setWave2Direction)
                .def("getWave2Direction", &PlanetOceanComponent::getWave2Direction)
                .def("setWave3Amplitude", &PlanetOceanComponent::setWave3Amplitude)
                .def("getWave3Amplitude", &PlanetOceanComponent::getWave3Amplitude)
                .def("setWave3Frequency", &PlanetOceanComponent::setWave3Frequency)
                .def("getWave3Frequency", &PlanetOceanComponent::getWave3Frequency)
                .def("setWave3Speed", &PlanetOceanComponent::setWave3Speed)
                .def("getWave3Speed", &PlanetOceanComponent::getWave3Speed)
                .def("setWave3Direction", &PlanetOceanComponent::setWave3Direction)
                .def("getWave3Direction", &PlanetOceanComponent::getWave3Direction)];

        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "class inherits GameObjectComponent", PlanetOceanComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setActivated(bool activated)", "Enables or disables the ocean animation.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "bool isActivated()", "Gets whether the ocean is active.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setRadius(float radius)", "Sets the ocean sphere radius. Triggers mesh rebuild.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getRadius()", "Gets the ocean sphere radius.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setDeepColour(Vector3 colour)", "Sets the deep ocean diffuse colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "Vector3 getDeepColour()", "Gets the deep ocean colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setShallowColour(Vector3 colour)", "Sets the shallow water emissive tint.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "Vector3 getShallowColour()", "Gets the shallow water colour.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setRoughness(float roughness)", "Sets PBS roughness. Uses specular_fresnel workflow. 0.06 = calm ocean.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getRoughness()", "Gets PBS roughness.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setTransparency(float transparency)", "Sets water transparency [0=opaque, 1=fully transparent].");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getTransparency()", "Gets water transparency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setReflectionMap(String cubemapName)", "Sets optional cubemap for reflections. Empty to skip.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "String getReflectionMap()", "Gets reflection cubemap name.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWaveAmplitudeScale(float scale)", "Global amplitude multiplier. 0 = flat ocean.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWaveAmplitudeScale()", "Gets global wave amplitude scale.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave0Amplitude(float v)", "Wave 0 crest height in metres (calibrated for 50m planet, scales with radius).");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave0Amplitude()", "Gets wave 0 amplitude.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave0Frequency(float v)", "Wave 0 cycles per hemisphere (radius-independent). 2 = two crests across equator.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave0Frequency()", "Gets wave 0 frequency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave0Speed(float v)", "Wave 0 phase speed in radians per second.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave0Speed()", "Gets wave 0 speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave0Direction(float v)", "Wave 0 propagation direction in radians (0 = +X axis).");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave0Direction()", "Gets wave 0 direction.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave1Amplitude(float v)", "Wave 1 crest height in metres.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave1Amplitude()", "Gets wave 1 amplitude.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave1Frequency(float v)", "Wave 1 cycles per hemisphere.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave1Frequency()", "Gets wave 1 frequency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave1Speed(float v)", "Wave 1 phase speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave1Speed()", "Gets wave 1 speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave1Direction(float v)", "Wave 1 propagation direction in radians.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave1Direction()", "Gets wave 1 direction.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave2Amplitude(float v)", "Wave 2 crest height in metres.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave2Amplitude()", "Gets wave 2 amplitude.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave2Frequency(float v)", "Wave 2 cycles per hemisphere.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave2Frequency()", "Gets wave 2 frequency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave2Speed(float v)", "Wave 2 phase speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave2Speed()", "Gets wave 2 speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave2Direction(float v)", "Wave 2 propagation direction in radians.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave2Direction()", "Gets wave 2 direction.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave3Amplitude(float v)", "Wave 3 crest height in metres.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave3Amplitude()", "Gets wave 3 amplitude.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave3Frequency(float v)", "Wave 3 cycles per hemisphere.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave3Frequency()", "Gets wave 3 frequency.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave3Speed(float v)", "Wave 3 phase speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave3Speed()", "Gets wave 3 speed.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "void setWave3Direction(float v)", "Wave 3 propagation direction in radians.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetOceanComponent", "float getWave3Direction()", "Gets wave 3 direction.");

        gameObjectClass.def("getPlanetOceanComponentFromName", &getPlanetOceanComponentFromName);
        gameObjectClass.def("getPlanetOceanComponent", (PlanetOceanComponent * (*)(GameObject*)) & getPlanetOceanComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetOceanComponent getPlanetOceanComponent()", "Gets the PlanetOceanComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetOceanComponent getPlanetOceanComponentFromName(String name)", "Gets the PlanetOceanComponent by name.");

        gameObjectControllerClass.def("castPlanetOceanComponent", &GameObjectController::cast<PlanetOceanComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PlanetOceanComponent castPlanetOceanComponent(PlanetOceanComponent other)", "Casts type for Lua auto completion.");
    }

    bool PlanetOceanComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto thisCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlanetOceanComponent>());
        if (nullptr == thisCompPtr)
        {
            return true;
        }
        return false;
    }

} // namespace NOWA