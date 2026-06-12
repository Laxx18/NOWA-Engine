#include "NOWAPrecompiled.h"
#include "HdrEffectComponent.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/LightDirectionalComponent.h"
#include "gameobject/WorkspaceComponents.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/EventManager.h"
#include "main/ProcessManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"

#include "OgreAbiUtils.h"
#include "OgreBitwise.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    HdrEffectComponent::HdrEffectComponent() :
        GameObjectComponent(),
        name("HdrEffectComponent"),
        effectName(new Variant(HdrEffectComponent::AttrEffectName(),
            {"Bright, sunny day", "Scary Night", "Dark Night", "Dream Night", "Average, slightly hazy day", "Heavy overcast day", "Gibbous moon night", "JJ Abrams style", "Black Night", "Golden Hour", "Dawn", "Dusk", "Stormy", "Underwater",
                "Alien World", "Foggy Morning", "Desert Noon", "Arctic Day", "Neon Night", "Volcanic", "Moonless Night", "Custom"},
            this->attributes)),
        skyColor(new Variant(HdrEffectComponent::AttrSkyColor(), Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f, this->attributes)),
        upperHemisphere(new Variant(HdrEffectComponent::AttrUpperHemisphere(), Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f, this->attributes)),
        lowerHemisphere(new Variant(HdrEffectComponent::AttrLowerHemisphere(), Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f, this->attributes)),
        sunPower(new Variant(HdrEffectComponent::AttrSunPower(), 97.0f, this->attributes)),
        exposure(new Variant(HdrEffectComponent::AttrExposure(), 0.0f, this->attributes)),
        minAutoExposure(new Variant(HdrEffectComponent::AttrMinAutoExposure(), -1.0f, this->attributes)),
        maxAutoExposure(new Variant(HdrEffectComponent::AttrMaxAutoExposure(), 2.5f, this->attributes)),
        bloom(new Variant(HdrEffectComponent::AttrBloom(), 5.0f, this->attributes)),
        envMapScale(new Variant(HdrEffectComponent::AttrEnvMapScale(), 16.0f, this->attributes)),
        lightDirectionalComponent(nullptr),
        workspaceBaseComponent(nullptr),
        isApplyingPreset(false)
    {
        this->effectName->addUserData(GameObject::AttrActionNeedRefresh());
        this->skyColor->addUserData(GameObject::AttrActionColorDialog());
        this->upperHemisphere->addUserData(GameObject::AttrActionColorDialog());
        this->lowerHemisphere->addUserData(GameObject::AttrActionColorDialog());
        this->sunPower->setConstraints(0.0f, 200.0f);
        this->bloom->setConstraints(0.0001f, 5.0f);

        this->exposure->setDescription("Modifies the HDR Materials for the new exposure parameters. "
                                       "By default the HDR implementation will try to auto adjust the exposure based on the scene's average luminance. "
                                       "If left unbounded, even the darkest scenes can look well lit and the brigthest scenes appear too normal. "
                                       "These parameters are useful to prevent the auto exposure from jumping too much from one extreme to the otherand provide "
                                       "a consistent experience within the same lighting conditions. (e.g.you may want to change the params when going from indoors to outdoors)"
                                       "The smaller the gap between minAutoExposure & maxAutoExposure, the less the auto exposure tries to auto adjust to the scene's lighting conditions. "
                                       "The first value is exposure. Valid range is [-4.9; 4.9]. Low values will make the picture darker. Higher values will make the picture brighter.");

        this->minAutoExposure->setDescription("Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure darkens a bright scene. "
                                              "To prevent that looking at a very bright object makes the rest of the scene really dark, use higher values.");

        this->maxAutoExposure->setDescription("Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure brightens a dark scene. "
                                              "To prevent that looking at a very dark object makes the rest of the scene really bright, use lower values.");

        this->bloom->setDescription("Controls the bloom intensity. Scale is in lumens / 1024. Valid range is [0.01; 4.9].");

        this->upperHemisphere->setDescription("Ambient color when the surface normal is close to hemisphereDir.");
        this->lowerHemisphere->setDescription("Ambient color when the surface normal is pointing away from hemisphereDir.");
        this->envMapScale->setDescription("Global scale to apply to all environment maps (for relevant Hlms implementations, "
                                          "like PBS). The value will be stored in upperHemisphere.a. Use 1.0 to disable.");
    }

    HdrEffectComponent::~HdrEffectComponent(void)
    {
    }

    void HdrEffectComponent::initialise()
    {
    }

    const Ogre::String& HdrEffectComponent::getName() const
    {
        return this->name;
    }

    void HdrEffectComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<HdrEffectComponent>(HdrEffectComponent::getStaticClassId(), HdrEffectComponent::getStaticClassName());
    }

    void HdrEffectComponent::shutdown()
    {
        // Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
    }

    void HdrEffectComponent::uninstall()
    {
        // Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
    }

    void HdrEffectComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool HdrEffectComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EffectName")
        {
            this->effectName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkyColor")
        {
            this->skyColor->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpperHemisphere")
        {
            this->upperHemisphere->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LowerHemisphere")
        {
            this->lowerHemisphere->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SunPower")
        {
            this->sunPower->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Exposure")
        {
            this->exposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAutoExposure")
        {
            this->minAutoExposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAutoExposure")
        {
            this->maxAutoExposure->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Bloom")
        {
            this->bloom->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnvMapScale")
        {
            this->envMapScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    GameObjectCompPtr HdrEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool HdrEffectComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Init hdr effect component for game object: " + this->gameObjectPtr->getName());

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &HdrEffectComponent::handleHdrActivated), NOWA::EventDataHdrActivated::getStaticEventType());

        // Get the sun light (directional light for sun power setting)
        GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_LIGHT_ID);

        if (nullptr == lightGameObjectPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent] Could not find 'SunLight' for this component! Affected game object: " + this->gameObjectPtr->getName());
            return false;
        }

        auto lightDirectionalCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
        if (nullptr != lightDirectionalCompPtr)
        {
            this->lightDirectionalComponent = lightDirectionalCompPtr.get();
        }

        auto workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
        if (nullptr != workspaceBaseCompPtr)
        {
            this->workspaceBaseComponent = workspaceBaseCompPtr.get();
            if (false == this->workspaceBaseComponent->getUseHdr())
            {
                this->workspaceBaseComponent->setUseHdr(true);
            }
        }

        this->postApplySunPower();

        return true;
    }

    bool HdrEffectComponent::connect(void)
    {
        GameObjectComponent::connect();

        auto cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
        if (nullptr != cameraCompPtr)
        {
            // Note: If its a camera, e.g. MainCamera and hdr effect shall be set, but there is split screen scenario active, the hdr effect may not be set for a camera, which is not involved in split screen scenario
            if ((true == cameraCompPtr->isActivated() && false == WorkspaceModule::getInstance()->getSplitScreenScenarioActive()) || true == this->workspaceBaseComponent->getInvolvedInSplitScreen())
            {
                // Apply loaded effect
                this->setEffectName(this->effectName->getListSelectedValue());

                this->postApplySunPower();
            }
        }
        return true;
    }

    bool HdrEffectComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        this->resetShining();
        return true;
    }

    void HdrEffectComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &HdrEffectComponent::handleHdrActivated), NOWA::EventDataHdrActivated::getStaticEventType());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Destructor hdr effect component for game object: " + this->gameObjectPtr->getName());

        this->lightDirectionalComponent = nullptr;
        this->workspaceBaseComponent = nullptr;

        this->resetShining();
    }

    void HdrEffectComponent::onOtherComponentRemoved(unsigned int index)
    {
        auto gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
        if (nullptr != gameObjectCompPtr)
        {
            auto lightDirectionalCompPtr = boost::dynamic_pointer_cast<LightDirectionalComponent>(gameObjectCompPtr);
            if (nullptr != lightDirectionalCompPtr)
            {
                this->lightDirectionalComponent = nullptr;
            }
        }
    }

    void HdrEffectComponent::handleHdrActivated(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataInputDeviceOccupied> castEventData = boost::static_pointer_cast<EventDataInputDeviceOccupied>(eventData);

        // Only react if the event concerns this game object's workspace
        if (castEventData->getGameObjectId() != this->gameObjectPtr->getId())
        {
            return;
        }

        // The workspace has just been (re)created. Any apply calls made before
        // this point early-returned because the workspace did not exist yet,
        // hence the CURRENT values must be (re)applied now - never hardcoded
        // reset defaults, which would silently neutralize the active preset.
        this->applyCurrentValues();
    }

    void HdrEffectComponent::applyCurrentValues(void)
    {
        // The skyColor variant stores the PREMULTIPLIED colour (e.g. 12 24 36
        // for bright sunny day = base 0.2 0.4 0.6 times 60) and keeps the
        // original preset multiplier in w for display purposes. The rgb part
        // already contains the multiplier, hence pass 1.0 here:
        // luminance(12, 24, 36) / 0.372 = 60 = correct hdrSkyPower.
        const Ogre::Vector4 tempSkyColor = this->skyColor->getVector4();
        this->applyHdrSkyColor(Ogre::ColourValue(tempSkyColor.x, tempSkyColor.y, tempSkyColor.z), 1.0f);

        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());

        // Demo mapping: minThreshold = bloom - 2, fullThreshold = bloom
        Ogre::Real fullThreshold = this->bloom->getReal();
        if (fullThreshold < 0.01f)
        {
            fullThreshold = 0.01f;
        }

        Ogre::Real minThreshold = fullThreshold - 2.0f;
        if (minThreshold < 0.01f)
        {
            minThreshold = 0.01f;
        }

        this->applyBloomThreshold(minThreshold, fullThreshold);

        this->postApplySunPower();
    }

    void HdrEffectComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (HdrEffectComponent::AttrEffectName() == attribute->getName())
        {
            this->setEffectName(attribute->getListSelectedValue());
        }
        else if (HdrEffectComponent::AttrSkyColor() == attribute->getName())
        {
            this->setSkyColor(attribute->getVector4());
        }
        else if (HdrEffectComponent::AttrUpperHemisphere() == attribute->getName())
        {
            this->setUpperHemisphere(attribute->getVector4());
        }
        else if (HdrEffectComponent::AttrLowerHemisphere() == attribute->getName())
        {
            this->setLowerHemisphere(attribute->getVector4());
        }
        else if (HdrEffectComponent::AttrSunPower() == attribute->getName())
        {
            this->setSunPower(attribute->getReal());
        }
        else if (HdrEffectComponent::AttrExposure() == attribute->getName())
        {
            this->setExposure(attribute->getReal());
        }
        else if (HdrEffectComponent::AttrMinAutoExposure() == attribute->getName())
        {
            this->setMinAutoExposure(attribute->getReal());
        }
        else if (HdrEffectComponent::AttrMaxAutoExposure() == attribute->getName())
        {
            this->setMaxAutoExposure(attribute->getReal());
        }
        else if (HdrEffectComponent::AttrBloom() == attribute->getName())
        {
            this->setBloom(attribute->getReal());
        }
        else if (HdrEffectComponent::AttrEnvMapScale() == attribute->getName())
        {
            this->setEnvMapScale(attribute->getReal());
        }
    }

    void HdrEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "EffectName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->effectName->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SkyColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skyColor->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UpperHemisphere"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->upperHemisphere->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "LowerHemisphere"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lowerHemisphere->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SunPower"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sunPower->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Exposure"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->exposure->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MinAutoExposure"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAutoExposure->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAutoExposure"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAutoExposure->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Bloom"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bloom->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "EnvMapScale"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->envMapScale->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String HdrEffectComponent::getClassName(void) const
    {
        return "HdrEffectComponent";
    }

    Ogre::String HdrEffectComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void HdrEffectComponent::resetShining(void)
    {
#if 0
		if (true == Core::getSingletonPtr()->getIsSceneBeingDestroyed())
		{
			return;
		}
		// Reset shining and set default values
		if (nullptr != this->lightDirectionalComponent)
		{
			ENQUEUE_RENDER_COMMAND("HdrEffectComponent::resetShining",
				{
					this->lightDirectionalComponent->setPowerScale(3.14159f);
				});
		}
		this->gameObjectPtr->getSceneManager()->setAmbientLight(Ogre::ColourValue(0.03375f, 0.05625f, 0.07875f), Ogre::ColourValue(0.04388f, 0.03291f, 0.02194f, 0.07312f), this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir());
#endif
    }

    void HdrEffectComponent::postApplySunPower(void)
    {
        // The presets are calibrated like the Ogre HDR sample: powerScale is
        // lumens / 1024, e.g. 97 = direct sunlight (~100.000 lumens).
        // NO multiplication by PI here, and setEffectName must not do it either,
        // otherwise the last writer wins and the sun power differs by factor 3.14
        // depending on call order.
        if (nullptr != this->lightDirectionalComponent)
        {
            ENQUEUE_RENDER_COMMAND("HdrEffectComponent::postApplySunPower", { this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal()); });
        }
    }

    void HdrEffectComponent::applyHdrSkyColor(const Ogre::ColourValue& color, Ogre::Real multiplier)
    {
        if (nullptr == this->workspaceBaseComponent || false == this->workspaceBaseComponent->getUseHdr() || nullptr == this->workspaceBaseComponent->getWorkspace())
        {
            this->resetShining();
            return;
        }

        Ogre::CompositorNode* node = this->workspaceBaseComponent->getWorkspace()->findNode(this->workspaceBaseComponent->getRenderingNodeName());

        if (nullptr == node)
        {
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "No node '" + this->workspaceBaseComponent->getRenderingNodeName() + "' in provided workspace ", "HdrEffectComponent::applyHdrSkyColor");
        }

        // The clear colour only matters where the sky quad does not draw.
        // The sky quad itself must be scaled into HDR range via hdrSkyPower,
        // otherwise an LDR skybox (luminance ~0.3) against HDR lit geometry
        // (luminance ~18 with sun power 97) creates an uncompressable 60:1
        // ratio: ground burns to white, sky sinks to black.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, color, multiplier, node]()
        {
            const Ogre::CompositorPassVec passes = node->_getPasses();

            assert(passes.size() >= 1);
            Ogre::CompositorPass* pass = passes[0];

            Ogre::RenderPassDescriptor* renderPassDesc = pass->getRenderPassDesc();
            renderPassDesc->setClearColour(color * multiplier);

            Ogre::CompositorManager2* compositorManager = this->workspaceBaseComponent->getWorkspace()->getCompositorManager();
            Ogre::CompositorNodeDef* nodeDef = compositorManager->getNodeDefinitionNonConst(this->workspaceBaseComponent->getRenderingNodeName());

            assert(nodeDef->getNumTargetPasses() >= 1);

            Ogre::CompositorTargetDef* targetDef = nodeDef->getTargetPass(0);
            const Ogre::CompositorPassDefVec& passDefs = targetDef->getCompositorPasses();

            assert(passDefs.size() >= 1);
            Ogre::CompositorPassDef* passDef = passDefs[0];

            passDef->setAllClearColours(color * multiplier);

            // Scale the sky quad into HDR range. The factor is the luminance of the
            // preset sky colour relative to the luminance of the demo base sky colour
            // (0.2, 0.4, 0.6), whose luminance is 0.372. For "Bright, sunny day" this
            // yields exactly the preset multiplier 60, for night presets values << 1.
            Ogre::MaterialPtr skyMaterial = Ogre::MaterialManager::getSingleton().getByName("NOWASkyPostprocess", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            if (false == skyMaterial.isNull())
            {
                Ogre::Pass* skyPass = skyMaterial->getTechnique(0)->getPass(0);
                Ogre::GpuProgramParametersSharedPtr psParams = skyPass->getFragmentProgramParameters();

                // 0.372 = luminance of the demo base sky colour (0.2, 0.4, 0.6).
                // skyBoxCompensation corrects for the inherent brightness of the
                // cubemap texels relative to that base: a cubemap whose sky region
                // averages ~0.7 luminance needs ~0.5 to land on the preset target.

                const Ogre::ColourValue hdrSkyColor = color * multiplier;
                // Play with this value:
                const Ogre::Real skyBoxCompensation = 0.05f;
                Ogre::Real hdrSkyPower = (0.2125f * hdrSkyColor.r + 0.7154f * hdrSkyColor.g + 0.0721f * hdrSkyColor.b) / 0.372f * skyBoxCompensation;
                
                // Ogre::Real hdrSkyPower = (0.2125f * hdrSkyColor.r + 0.7154f * hdrSkyColor.g + 0.0721f * hdrSkyColor.b) / 0.372f;
                if (hdrSkyPower < 0.001f)
                {
                    hdrSkyPower = 0.001f;
                }

                if (nullptr != psParams->_findNamedConstantDefinition("hdrSkyPower"))
                {
                    psParams->setNamedConstant("hdrSkyPower", hdrSkyPower);
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent] NOWASkyPostprocess has no 'hdrSkyPower' uniform, sky will stay LDR and break HDR exposure!");
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "HdrEffectComponent::applyHdrSkyColor");
    }

    void HdrEffectComponent::applyExposure(Ogre::Real exposure, Ogre::Real minAutoExposure, Ogre::Real maxAutoExposure)
    {
        // Contract with DownScale03_SumLumEnd_ps:
        //   newLum = exposure.x / exp( clamp( fLumAvg, exposure.y, exposure.z ) )
        // fLumAvg is the average LOG luminance of the frame, hence y and z are
        // clamp bounds in log space and must be packed as 7.5 - autoExposure.
        // Note the inversion: maxAutoExposure goes into y, minAutoExposure into z,
        // because a LOWER clamped log luminance means a BRIGHTER final image.
        // Material default float3 138.5833 5 10 corresponds to
        // exposure = 0, maxAutoExposure = 2.5, minAutoExposure = -2.5.
        if (minAutoExposure > maxAutoExposure)
        {
            Ogre::Real temp = minAutoExposure;
            minAutoExposure = maxAutoExposure;
            maxAutoExposure = temp;
        }

        Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName("HDR/DownScale03_SumLumEnd", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        if (true == material.isNull())
        {
            Ogre::LogManager::getSingleton().logMessage("[HdrEffectComponent] ERROR: HDR/DownScale03_SumLumEnd material not found!");
            return;
        }

        Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

        // 1024 = HDR calibration factor (-10 stops), e is the correct base, NOT 2.
        // 1024 * exp(0 - 2) = 138.5833, which matches the material default.
        const Ogre::Vector3 exposureParams(1024.0f * expf(exposure - 2.0f), 7.5f - maxAutoExposure, 7.5f - minAutoExposure);

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("HdrEffectComponent::applyExposure", _2(pass, exposureParams), {
            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            psParams->setNamedConstant("exposure", exposureParams);
        });

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL,
            "[HdrEffectComponent] Applied exposure: " + Ogre::StringConverter::toString(exposureParams.x) + ", " + Ogre::StringConverter::toString(exposureParams.y) + ", " + Ogre::StringConverter::toString(exposureParams.z));
    }

    void HdrEffectComponent::applyBloomThreshold(Ogre::Real minThreshold, Ogre::Real fullThreshold)
    {
        // Contract with BrightPass_Start_ps:
        //   brightThreshold.x = low threshold, colour below is 0
        //   brightThreshold.y = 1 / (full - min), the smoothstep slope
        // Material default float2 3.0 0.5 corresponds to min 3.0, full 5.0.
        if (fullThreshold <= minThreshold)
        {
            fullThreshold = minThreshold + 0.01f;
        }

        Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName("HDR/BrightPass_Start", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        if (true == material.isNull())
        {
            Ogre::LogManager::getSingleton().logMessage("[HdrEffectComponent] ERROR: HDR/BrightPass_Start material not found!");
            return;
        }

        Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

        const Ogre::Vector4 brightThreshold(minThreshold, 1.0f / (fullThreshold - minThreshold), 0.0f, 0.0f);

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("HdrEffectComponent::applyBloomThreshold", _2(pass, brightThreshold), {
            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            psParams->setNamedConstant("brightThreshold", brightThreshold);
        });
    }

    void HdrEffectComponent::setEffectName(const Ogre::String& effectName)
    {
        this->effectName->setListSelectedValue(effectName);

        if ("Bright, sunny day" == effectName)
        {
            // Perfect summer day - intense sun, clear blue sky
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
            this->sunPower->setValue(97.0f); // × PI in render command
            this->exposure->setValue(0.0f);
            this->minAutoExposure->setValue(-1.0f);
            this->maxAutoExposure->setValue(2.5f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(16.0f);
        }
        else if ("Scary Night" == effectName)
        {
            // ACTUALLY DARK scary night - horror movie atmosphere
            this->skyColor->setValue(Ogre::Vector4(0.02f, 0.03f, 0.08f, 1.0f) * 0.15f);         // Almost black sky
            this->upperHemisphere->setValue(Ogre::Vector4(0.05f, 0.08f, 0.15f, 1.0f) * 0.025f); // Barely visible ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.02f, 0.04f, 0.08f, 1.0f) * 0.015f); // Nearly zero
            this->sunPower->setValue(0.08f);                                                    // Extremely weak moonlight (×π ≈ 0.25)
            this->exposure->setValue(-2.5f);                                                    // Start very dark
            this->minAutoExposure->setValue(-4.5f);                                             // Allow extreme darkness
            this->maxAutoExposure->setValue(-1.0f);                                             // Don't brighten much
            this->bloom->setValue(0.5f);                                                        // Minimal glow
            this->envMapScale->setValue(0.1f);
        }
        else if ("Dark Night" == effectName)
        {
            // Pitch black - can barely see anything
            this->skyColor->setValue(Ogre::Vector4(0.01f, 0.01f, 0.03f, 1.0f) * 0.05f);         // Nearly zero
            this->upperHemisphere->setValue(Ogre::Vector4(0.02f, 0.03f, 0.06f, 1.0f) * 0.01f);  // Almost nothing
            this->lowerHemisphere->setValue(Ogre::Vector4(0.01f, 0.02f, 0.04f, 1.0f) * 0.005f); // Zero ambient
            this->sunPower->setValue(0.02f);                                                    // Starlight only (×π ≈ 0.063)
            this->exposure->setValue(-3.0f);                                                    // Very dark start
            this->minAutoExposure->setValue(-4.9f);                                             // Maximum darkness allowed
            this->maxAutoExposure->setValue(-2.0f);                                             // Keep it dark
            this->bloom->setValue(0.2f);                                                        // Almost no bloom
            this->envMapScale->setValue(0.02f);
        }
        else if ("Dream Night" == effectName)
        {
            // Surreal, ethereal moonlit scene - magical atmosphere
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.25f, 0.4f, 1.0f) * 3.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.35f, 0.5f, 1.0f) * 0.8f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.25f, 0.3f, 0.45f, 1.0f) * 0.5f);
            this->sunPower->setValue(3.0f); // (×π ≈ 9.42)
            this->exposure->setValue(0.2f);
            this->minAutoExposure->setValue(-0.5f);
            this->maxAutoExposure->setValue(1.8f);
            this->bloom->setValue(4.0f); // Strong dreamy bloom
            this->envMapScale->setValue(4.0f);
        }
        else if ("Average, slightly hazy day" == effectName)
        {
            // Overcast but still decent lighting - soft shadows
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 32.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 3.15f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.0475f);
            this->sunPower->setValue(48.0f); // (×π ≈ 150.8)
            this->exposure->setValue(0.0f);
            this->minAutoExposure->setValue(-2.0f);
            this->maxAutoExposure->setValue(2.5f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(8.0f);
        }
        else if ("Heavy overcast day" == effectName)
        {
            // Thick cloud cover - dull, flat lighting
            this->skyColor->setValue(Ogre::Vector4(0.4f, 0.4f, 0.4f, 1.0f) * 4.5f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.4f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.365625f);
            this->sunPower->setValue(6.0625f); // (×π ≈ 19.05)
            this->exposure->setValue(0.0f);
            this->minAutoExposure->setValue(-2.5f);
            this->maxAutoExposure->setValue(1.0f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(0.5f);
        }
        else if ("Gibbous moon night" == effectName)
        {
            // Bright moonlit night - you can see clearly
            this->skyColor->setValue(Ogre::Vector4(0.27f, 0.3f, 0.6f, 1.0f) * 0.01831072f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.50f, 1.0f) * 0.003f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.5f, 0.65f, 1.0f) * 0.00274222f);
            this->sunPower->setValue(0.4f); // (×π ≈ 1.26)
            this->exposure->setValue(0.65f);
            this->minAutoExposure->setValue(-2.5f);
            this->maxAutoExposure->setValue(3.0f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(0.0152587890625f);
        }
        else if ("JJ Abrams style" == effectName)
        {
            // Cinematic lens flare style - dramatic light shafts
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 6.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 0.1125f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 0.073125f);
            this->sunPower->setValue(4.0f); // (×π ≈ 12.57)
            this->exposure->setValue(0.5f);
            this->minAutoExposure->setValue(1.0f);
            this->maxAutoExposure->setValue(2.5f);
            this->bloom->setValue(3.0f);
            this->envMapScale->setValue(1.0f);
        }
        else if ("Black Night" == effectName)
        {
            // Complete darkness - new moon, no lights
            this->skyColor->setValue(Ogre::Vector4(0.005f, 0.005f, 0.01f, 1.0f) * 0.02f);         // Absolute minimum
            this->upperHemisphere->setValue(Ogre::Vector4(0.01f, 0.01f, 0.02f, 1.0f) * 0.005f);   // Nearly zero
            this->lowerHemisphere->setValue(Ogre::Vector4(0.005f, 0.005f, 0.01f, 1.0f) * 0.002f); // Zero
            this->sunPower->setValue(0.0f);                                                       // No light source
            this->exposure->setValue(-4.0f);                                                      // Maximum darkness
            this->minAutoExposure->setValue(-4.9f);                                               // Keep it pitch black
            this->maxAutoExposure->setValue(-3.5f);                                               // No brightening
            this->bloom->setValue(0.01f);
            this->envMapScale->setValue(0.001f);
        }
        // ========================================================================
        // NEW PRESETS START HERE
        // ========================================================================
        else if ("Golden Hour" == effectName)
        {
            // Sunset/sunrise magic hour - warm, dramatic, photographer's dream
            this->skyColor->setValue(Ogre::Vector4(0.8f, 0.5f, 0.3f, 1.0f) * 35.0f);       // Warm orange
            this->upperHemisphere->setValue(Ogre::Vector4(0.9f, 0.7f, 0.5f, 1.0f) * 3.5f); // Golden ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.7f, 0.5f, 0.4f, 1.0f) * 2.2f); // Warm shadows
            this->sunPower->setValue(45.0f);                                               // (×π ≈ 141.4) Soft sun near horizon
            this->exposure->setValue(0.2f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(2.0f);
            this->bloom->setValue(6.0f); // Strong bloom for that golden glow
            this->envMapScale->setValue(12.0f);
        }
        else if ("Dawn" == effectName)
        {
            // Early morning before sunrise - cool blues transitioning to warmth
            this->skyColor->setValue(Ogre::Vector4(0.4f, 0.5f, 0.7f, 1.0f) * 8.0f);        // Cool blue
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.6f, 0.8f, 1.0f) * 1.5f); // Cool upper
            this->lowerHemisphere->setValue(Ogre::Vector4(0.7f, 0.6f, 0.5f, 1.0f) * 0.8f); // Warmer ground
            this->sunPower->setValue(18.0f);                                               // (×π ≈ 56.5) Rising sun, still weak
            this->exposure->setValue(-0.3f);
            this->minAutoExposure->setValue(-2.0f);
            this->maxAutoExposure->setValue(1.5f);
            this->bloom->setValue(3.5f);
            this->envMapScale->setValue(5.0f);
        }
        else if ("Dusk" == effectName)
        {
            // Late evening, sun just set - purple/blue twilight
            this->skyColor->setValue(Ogre::Vector4(0.3f, 0.35f, 0.6f, 1.0f) * 2.5f);         // Deep blue
            this->upperHemisphere->setValue(Ogre::Vector4(0.35f, 0.4f, 0.65f, 1.0f) * 0.6f); // Cool ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.35f, 0.5f, 1.0f) * 0.35f); // Purplish ground
            this->sunPower->setValue(1.2f);                                                  // (×π ≈ 3.77) Last light remnants
            this->exposure->setValue(-0.8f);
            this->minAutoExposure->setValue(-3.0f);
            this->maxAutoExposure->setValue(0.8f);
            this->bloom->setValue(2.5f);
            this->envMapScale->setValue(2.0f);
        }
        else if ("Stormy" == effectName)
        {
            // Storm approaching - dark, dramatic, greenish tint
            this->skyColor->setValue(Ogre::Vector4(0.25f, 0.3f, 0.25f, 1.0f) * 2.5f);        // Greenish dark
            this->upperHemisphere->setValue(Ogre::Vector4(0.35f, 0.4f, 0.35f, 1.0f) * 0.5f); // Very dim
            this->lowerHemisphere->setValue(Ogre::Vector4(0.25f, 0.3f, 0.25f, 1.0f) * 0.3f); // Dark ground
            this->sunPower->setValue(3.0f);                                                  // (×π ≈ 9.42) Blocked sun
            this->exposure->setValue(-0.5f);
            this->minAutoExposure->setValue(-3.0f);
            this->maxAutoExposure->setValue(0.5f);
            this->bloom->setValue(1.5f);
            this->envMapScale->setValue(0.3f);
        }
        else if ("Underwater" == effectName)
        {
            // Submerged - blue/green caustics, dimmed light
            this->skyColor->setValue(Ogre::Vector4(0.1f, 0.3f, 0.4f, 1.0f) * 8.0f);           // Deep blue-green
            this->upperHemisphere->setValue(Ogre::Vector4(0.15f, 0.35f, 0.5f, 1.0f) * 1.8f);  // Blue ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.05f, 0.15f, 0.25f, 1.0f) * 0.9f); // Darker depths
            this->sunPower->setValue(25.0f);                                                  // (×π ≈ 78.5) Filtered sunlight
            this->exposure->setValue(0.0f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.8f);
            this->bloom->setValue(2.5f); // Caustics glow
            this->envMapScale->setValue(4.0f);
        }
        else if ("Alien World" == effectName)
        {
            // Sci-fi exoplanet - purple/magenta atmosphere
            this->skyColor->setValue(Ogre::Vector4(0.6f, 0.2f, 0.7f, 1.0f) * 40.0f);       // Purple sky
            this->upperHemisphere->setValue(Ogre::Vector4(0.7f, 0.3f, 0.8f, 1.0f) * 3.8f); // Magenta ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.5f, 0.6f, 1.0f) * 2.5f); // Cooler ground
            this->sunPower->setValue(85.0f);                                               // (×π ≈ 267.0) Alien sun
            this->exposure->setValue(0.1f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(2.3f);
            this->bloom->setValue(5.5f); // Otherworldly glow
            this->envMapScale->setValue(14.0f);
        }
        else if ("Foggy Morning" == effectName)
        {
            // Dense fog - muted colors, low contrast, mysterious
            this->skyColor->setValue(Ogre::Vector4(0.6f, 0.6f, 0.65f, 1.0f) * 12.0f);        // Neutral gray
            this->upperHemisphere->setValue(Ogre::Vector4(0.55f, 0.55f, 0.6f, 1.0f) * 2.0f); // Soft ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.55f, 1.0f) * 1.6f);  // Similar below
            this->sunPower->setValue(22.0f);                                                 // (×π ≈ 69.1) Diffused sun through fog
            this->exposure->setValue(0.1f);
            this->minAutoExposure->setValue(-1.2f);
            this->maxAutoExposure->setValue(1.8f);
            this->bloom->setValue(3.8f); // Fog scattering glow
            this->envMapScale->setValue(3.0f);
        }
        else if ("Desert Noon" == effectName)
        {
            // Harsh desert midday - extremely bright, heat haze
            this->skyColor->setValue(Ogre::Vector4(0.5f, 0.6f, 0.8f, 1.0f) * 85.0f);        // Intense bright sky
            this->upperHemisphere->setValue(Ogre::Vector4(0.8f, 0.75f, 0.7f, 1.0f) * 6.5f); // Hot ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.85f, 0.7f, 0.6f, 1.0f) * 5.2f); // Sandy reflection
            this->sunPower->setValue(120.0f);                                               // (×π ≈ 377.0) Extremely bright sun
            this->exposure->setValue(-0.5f);                                                // Compensate for brightness
            this->minAutoExposure->setValue(-2.0f);
            this->maxAutoExposure->setValue(1.5f);
            this->bloom->setValue(6.5f); // Heat haze bloom
            this->envMapScale->setValue(18.0f);
        }
        else if ("Arctic Day" == effectName)
        {
            // Polar environment - bright from snow reflection, cool tones
            this->skyColor->setValue(Ogre::Vector4(0.6f, 0.7f, 0.9f, 1.0f) * 45.0f);         // Bright cool sky
            this->upperHemisphere->setValue(Ogre::Vector4(0.7f, 0.8f, 0.95f, 1.0f) * 4.2f);  // Cool ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.85f, 0.9f, 0.95f, 1.0f) * 4.8f); // Snow reflection (higher than upper!)
            this->sunPower->setValue(75.0f);                                                 // (×π ≈ 235.6) Bright but low angle sun
            this->exposure->setValue(-0.3f);
            this->minAutoExposure->setValue(-1.8f);
            this->maxAutoExposure->setValue(2.0f);
            this->bloom->setValue(5.5f); // Snow glare
            this->envMapScale->setValue(15.0f);
        }
        else if ("Neon Night" == effectName)
        {
            // Cyberpunk city night - artificial lights, colorful
            this->skyColor->setValue(Ogre::Vector4(0.15f, 0.1f, 0.25f, 1.0f) * 0.8f);       // Dark purple sky
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.2f, 0.4f, 1.0f) * 0.4f);  // Purple ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.15f, 0.3f, 1.0f) * 0.6f); // Neon glow from ground
            this->sunPower->setValue(0.5f);                                                 // (×π ≈ 1.57) Weak moonlight
            this->exposure->setValue(-0.5f);
            this->minAutoExposure->setValue(-2.5f);
            this->maxAutoExposure->setValue(1.2f);
            this->bloom->setValue(4.5f); // Strong bloom for neon glow
            this->envMapScale->setValue(3.0f);
        }
        else if ("Volcanic" == effectName)
        {
            // Near active volcano - red/orange glow from lava, ash in air
            this->skyColor->setValue(Ogre::Vector4(0.4f, 0.25f, 0.2f, 1.0f) * 8.0f);        // Ashy red-brown
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.3f, 0.25f, 1.0f) * 1.5f); // Warm dark ambient
            this->lowerHemisphere->setValue(Ogre::Vector4(0.8f, 0.3f, 0.2f, 1.0f) * 2.5f);  // Lava glow from below (higher!)
            this->sunPower->setValue(15.0f);                                                // (×π ≈ 47.1) Sun through ash
            this->exposure->setValue(0.1f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(2.0f);
            this->bloom->setValue(5.0f); // Lava glow
            this->envMapScale->setValue(6.0f);
        }
        else if ("Moonless Night" == effectName)
        {
            // Clear night sky, no moon - darker than Scary Night but stars visible
            this->skyColor->setValue(Ogre::Vector4(0.03f, 0.04f, 0.12f, 1.0f) * 0.08f);         // Very dark blue
            this->upperHemisphere->setValue(Ogre::Vector4(0.06f, 0.08f, 0.15f, 1.0f) * 0.018f); // Minimal starlight
            this->lowerHemisphere->setValue(Ogre::Vector4(0.03f, 0.05f, 0.1f, 1.0f) * 0.01f);   // Nearly black
            this->sunPower->setValue(0.05f);                                                    // (×π ≈ 0.157) Starlight only
            this->exposure->setValue(-2.8f);
            this->minAutoExposure->setValue(-4.7f);
            this->maxAutoExposure->setValue(-1.5f);
            this->bloom->setValue(0.8f); // Star twinkle
            this->envMapScale->setValue(0.08f);
        }

        // Apply values (same as your original code)
        Ogre::ColourValue skyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);
        this->applyHdrSkyColor(skyColor, 1.0f);
        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
        this->applyBloomThreshold(std::max(this->bloom->getReal() - 2.0f, 0.0f), std::max(this->bloom->getReal(), 0.01f));

        ENQUEUE_RENDER_COMMAND("HdrEffectComponent::setEffectName", {
            Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
            Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);

            this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());

            if (nullptr != this->lightDirectionalComponent)
            {
                this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
            }
        });
    }

    // ============================================================================
    // UPDATE YOUR VARIANT DEFINITION IN CONSTRUCTOR
    // ============================================================================
    /*
    effectName(new Variant(HdrEffectComponent::AttrEffectName(),
        {
            "Bright, sunny day",
            "Scary Night",           // NOW ACTUALLY DARK!
            "Dark Night",            // NOW EVEN DARKER!
            "Dream Night",
            "Average, slightly hazy day",
            "Heavy overcast day",
            "Gibbous moon night",
            "JJ Abrams style",
            "Black Night",           // Pitch black
            "Golden Hour",           // NEW: Sunset/sunrise
            "Dawn",                  // NEW: Early morning
            "Dusk",                  // NEW: Evening twilight
            "Stormy",                // NEW: Storm weather
            "Underwater",            // NEW: Submerged look
            "Alien World",           // NEW: Purple sci-fi
            "Foggy Morning",         // NEW: Dense fog
            "Desert Noon",           // NEW: Harsh desert
            "Arctic Day",            // NEW: Snowy bright
            "Neon Night",            // NEW: Cyberpunk city
            "Volcanic",              // NEW: Lava glow
            "Moonless Night"         // NEW: Stars only
        },
        this->attributes))
    */

    // ============================================================================
    // PRESET COMPARISON TABLE
    // ============================================================================
    /*
    Preset Name         | Sun×π  | Sky Mult | Exposure | Min/Max Auto  | Mood Description
    --------------------|--------|----------|----------|---------------|-------------------
    Bright, sunny day   | 304.8  | 60       | 0.0      | -1.0 / 2.5    | Perfect summer
    Scary Night         | 0.25   | 0.15     | -2.5     | -4.5 / -1.0   | HORROR DARK
    Dark Night          | 0.063  | 0.05     | -3.0     | -4.9 / -2.0   | PITCH BLACK
    Dream Night         | 9.42   | 3.0      | 0.2      | -0.5 / 1.8    | Surreal ethereal
    Avg. hazy day       | 150.8  | 32       | 0.0      | -2.0 / 2.5    | Overcast normal
    Heavy overcast      | 19.1   | 4.5      | 0.0      | -2.5 / 1.0    | Very cloudy
    Gibbous moon        | 1.26   | 0.018    | 0.65     | -2.5 / 3.0    | Bright moonlit
    JJ Abrams style     | 12.57  | 6.0      | 0.5      | 1.0 / 2.5     | Lens flare drama
    Black Night         | 0.0    | 0.02     | -4.0     | -4.9 / -3.5   | Complete darkness
    Golden Hour         | 141.4  | 35       | 0.2      | -1.5 / 2.0    | Sunset magic
    Dawn                | 56.5   | 8.0      | -0.3     | -2.0 / 1.5    | Early morning
    Dusk                | 3.77   | 2.5      | -0.8     | -3.0 / 0.8    | Evening twilight
    Stormy              | 9.42   | 2.5      | -0.5     | -3.0 / 0.5    | Dark storm
    Underwater          | 78.5   | 8.0      | 0.0      | -1.5 / 1.8    | Submerged blue
    Alien World         | 267.0  | 40       | 0.1      | -1.5 / 2.3    | Sci-fi purple
    Foggy Morning       | 69.1   | 12       | 0.1      | -1.2 / 1.8    | Dense fog
    Desert Noon         | 377.0  | 85       | -0.5     | -2.0 / 1.5    | Extreme bright
    Arctic Day          | 235.6  | 45       | -0.3     | -1.8 / 2.0    | Snowy bright
    Neon Night          | 1.57   | 0.8      | -0.5     | -2.5 / 1.2    | Cyberpunk city
    Volcanic            | 47.1   | 8.0      | 0.1      | -1.5 / 2.0    | Lava glow
    Moonless Night      | 0.157  | 0.08     | -2.8     | -4.7 / -1.5   | Stars only
    */

    void HdrEffectComponent::refreshAllParameters()
    {
        // Force re-application of all parameters
        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());

        this->applyBloomThreshold(std::max(this->bloom->getReal() - 2.0f, 0.0f), std::max(this->bloom->getReal(), 0.01f));

        Ogre::ColourValue skyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);
        this->applyHdrSkyColor(skyColor, 1.0f);

        // Force ambient light update
        ENQUEUE_RENDER_COMMAND("HdrEffectComponent::refreshAllParameters", {
            Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
            Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);

            this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());

            if (nullptr != this->lightDirectionalComponent)
            {
                this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal() * Ogre::Math::PI);
            }
        });
    }

    Ogre::String HdrEffectComponent::getEffectName(void) const
    {
        return this->effectName->getListSelectedValue();
    }

    void HdrEffectComponent::setSkyColor(const Ogre::Vector4& skyColor)
    {
        this->skyColor->setValue(skyColor);

        // Only switch to "Custom" if NOT applying a preset
        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        Ogre::ColourValue color(skyColor.x, skyColor.y, skyColor.z, skyColor.w);
        this->applyHdrSkyColor(color, 1.0f);
    }

    Ogre::Vector4 HdrEffectComponent::getSkyColor(void) const
    {
        return this->skyColor->getVector4();
    }

    void HdrEffectComponent::setUpperHemisphere(const Ogre::Vector4& upperHemisphere)
    {
        this->upperHemisphere->setValue(upperHemisphere);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        ENQUEUE_RENDER_COMMAND_MULTI("HdrEffectComponent::setUpperHemisphere", _1(upperHemisphere), {
            Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
            Ogre::ColourValue ambUpperHemisphere(upperHemisphere.x, upperHemisphere.y, upperHemisphere.z);
            this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
        });
    }

    Ogre::Vector4 HdrEffectComponent::getUpperHemisphere(void) const
    {
        return this->upperHemisphere->getVector4();
    }

    void HdrEffectComponent::setLowerHemisphere(const Ogre::Vector4& lowerHemisphere)
    {
        this->lowerHemisphere->setValue(lowerHemisphere);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        ENQUEUE_RENDER_COMMAND_MULTI("HdrEffectComponent::setLowerHemisphere", _1(lowerHemisphere), {
            Ogre::ColourValue ambLowerHemisphere(lowerHemisphere.x, lowerHemisphere.y, lowerHemisphere.z);
            Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);
            this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
        });
    }

    Ogre::Vector4 HdrEffectComponent::getLowerHemisphere(void) const
    {
        return this->lowerHemisphere->getVector4();
    }

    void HdrEffectComponent::setSunPower(Ogre::Real sunPower)
    {
        this->sunPower->setValue(sunPower);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        ENQUEUE_RENDER_COMMAND_MULTI("HdrEffectComponent::setSunPower", _1(sunPower), {
            if (nullptr != this->lightDirectionalComponent)
            {
                // MULTIPLY BY PI HERE
                this->lightDirectionalComponent->setPowerScale(sunPower * Ogre::Math::PI);
            }
        });
    }

    Ogre::Real HdrEffectComponent::getSunPower(void) const
    {
        return this->sunPower->getReal();
    }

    void HdrEffectComponent::setExposure(Ogre::Real exposure)
    {
        this->exposure->setValue(exposure);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
    }

    Ogre::Real HdrEffectComponent::getExposure(void) const
    {
        return this->exposure->getReal();
    }

    void HdrEffectComponent::setMinAutoExposure(Ogre::Real minAutoExposure)
    {
        this->minAutoExposure->setValue(minAutoExposure);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
    }

    Ogre::Real HdrEffectComponent::getMinAutoExposure(void) const
    {
        return this->minAutoExposure->getReal();
    }

    void HdrEffectComponent::setMaxAutoExposure(Ogre::Real maxAutoExposure)
    {
        this->maxAutoExposure->setValue(maxAutoExposure);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
    }

    Ogre::Real HdrEffectComponent::getMaxAutoExposure(void) const
    {
        return this->maxAutoExposure->getReal();
    }

    void HdrEffectComponent::setBloom(Ogre::Real bloom)
    {
        this->bloom->setValue(bloom);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        this->applyBloomThreshold(std::max(bloom - 2.0f, 0.0f), std::max(bloom, 0.01f));
    }

    Ogre::Real HdrEffectComponent::getBloom(void) const
    {
        return this->bloom->getReal();
    }

    void HdrEffectComponent::setEnvMapScale(Ogre::Real envMapScale)
    {
        this->envMapScale->setValue(envMapScale);

        if (!this->isApplyingPreset && this->effectName->getListSelectedValue() != "Custom")
        {
            this->effectName->setListSelectedValue("Custom");
        }

        ENQUEUE_RENDER_COMMAND_MULTI("HdrEffectComponent::setEnvMapScale", _1(envMapScale), {
            Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
            Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);
            this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), envMapScale);
        });
    }

    Ogre::Real HdrEffectComponent::getEnvMapScale(void) const
    {
        return this->envMapScale->getReal();
    }

    bool HdrEffectComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto hdrEffectCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<HdrEffectComponent>());
        auto workspaceBaseCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<WorkspaceBaseComponent>());
        // Constraints: Can only be placed under an object with WorkspaceBaseComponent and only once
        if (nullptr != workspaceBaseCompPtr && nullptr == hdrEffectCompPtr)
        {
            return true;
        }
        return false;
    }

    // Lua registration part

    HdrEffectComponent* getHdrEffectComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<HdrEffectComponent>(gameObject->getComponentWithOccurrence<HdrEffectComponent>(occurrenceIndex)).get();
    }

    HdrEffectComponent* getHdrEffectComponent(GameObject* gameObject)
    {
        return makeStrongPtr<HdrEffectComponent>(gameObject->getComponent<HdrEffectComponent>()).get();
    }

    HdrEffectComponent* getHdrEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<HdrEffectComponent>(gameObject->getComponentFromName<HdrEffectComponent>(name)).get();
    }

    void HdrEffectComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<HdrEffectComponent, GameObjectComponent>("HdrEffectComponent")
                .def("setEffectName", &HdrEffectComponent::setEffectName)
                .def("getEffectName", &HdrEffectComponent::getEffectName)
                .def("setSkyColor", &HdrEffectComponent::setSkyColor)
                .def("getSkyColor", &HdrEffectComponent::getSkyColor)
                .def("setUpperHemisphere", &HdrEffectComponent::setUpperHemisphere)
                .def("getUpperHemisphere", &HdrEffectComponent::getUpperHemisphere)
                .def("setLowerHemisphere", &HdrEffectComponent::setLowerHemisphere)
                .def("getLowerHemisphere", &HdrEffectComponent::getLowerHemisphere)
                .def("setSunPower", &HdrEffectComponent::setSunPower)
                .def("getSunPower", &HdrEffectComponent::getSunPower)
                .def("setExposure", &HdrEffectComponent::setExposure)
                .def("getExposure", &HdrEffectComponent::getExposure)
                .def("setMinAutoExposure", &HdrEffectComponent::setMinAutoExposure)
                .def("getMinAutoExposure", &HdrEffectComponent::getMinAutoExposure)
                .def("setMaxAutoExposure", &HdrEffectComponent::setMaxAutoExposure)
                .def("getMaxAutoExposure", &HdrEffectComponent::getMaxAutoExposure)
                .def("setBloom", &HdrEffectComponent::setBloom)
                .def("getBloom", &HdrEffectComponent::getBloom)
                .def("setEnvMapScale", &HdrEffectComponent::setEnvMapScale)
                .def("getEnvMapScale", &HdrEffectComponent::getEnvMapScale)];

        gameObjectClass.def("getHdrEffectComponentFromName", &getHdrEffectComponentFromName);
        gameObjectClass.def("getHdrEffectComponent", (HdrEffectComponent * (*)(GameObject*)) & getHdrEffectComponent);

        LuaScriptApi::getInstance()->LuaScriptApi::getInstance()->addClassToCollection("GameObject", "HdrEffectComponent getHdrEffectComponent()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->LuaScriptApi::getInstance()->addClassToCollection("GameObject", "HdrEffectComponent getHdrEffectComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castHdrEffectComponent", &GameObjectController::cast<HdrEffectComponent>);
        LuaScriptApi::getInstance()->LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "HdrEffectComponent castHdrEffectComponent(HdrEffectComponent other)", "Casts an incoming type from function for lua auto completion.");

        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "class inherits GameObjectComponent", HdrEffectComponent::getStaticInfoText());
        // LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "String getClassName()", "Gets the class name of this component as string.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setEffectName(string effectName)",
            "Sets the hdr effect name. Possible values are: "
            "'Bright, sunny day', 'Scary Night', 'Average, slightly hazy day', 'Heavy overcast day', 'Gibbous moon night', 'JJ Abrams style', 'Custom'"
            "Note: If any value is manipulated manually, the effect name will be set to 'Custom'.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "string getEffectName()",
            "Gets currently set effect name. Possible values are : "
            "'Bright, sunny day', 'Scary Night', 'Average, slightly hazy day', 'Heavy overcast day', 'Gibbous moon night', 'JJ Abrams style', 'Custom'"
            "Note: If any value is manipulated manually, the effect name will be set to 'Custom'.");

        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setSkyColor(Vector3 skyColor)", "Sets sky color.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "Vector3 getSkyColor()", "Gets the sky color.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setUpperHemisphere(Vector3 upperHemisphere)", "Sets the ambient color when the surface normal is close to hemisphereDir.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "Vector3 getUpperHemisphere()", "Gets the upper hemisphere color.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setLowerHemisphere(Vector3 lowerHemisphere)", "Sets the ambient color when the surface normal is pointing away from hemisphereDir.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "Vector3 getLowerHemisphere()", "Gets the sky color.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setSunPower(float sunPower)", "Sets the sun power scale. Note: This is applied on the 'SunLight'.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "float getSunPower()", "Gets the sun power scale.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setExposure(float exposure)",
            "Modifies the HDR Materials for the new exposure parameters. "
            "By default the HDR implementation will try to auto adjust the exposure based on the scene's average luminance. "
            "If left unbounded, even the darkest scenes can look well lit and the brigthest scenes appear too normal. "
            "These parameters are useful to prevent the auto exposure from jumping too much from one extreme to the otherand provide "
            "a consistent experience within the same lighting conditions. (e.g.you may want to change the params when going from indoors to outdoors)"
            "The smaller the gap between minAutoExposure & maxAutoExposure, the less the auto exposure tries to auto adjust to the scene's lighting conditions. "
            "The first value is exposure. Valid range is [-4.9; 4.9]. Low values will make the picture darker. Higher values will make the picture brighter.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "float getExposure()", "Gets the exposure.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setMinAutoExposure(float minAutoExposure)",
            "Sets the min auto exposure. Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure darkens a bright scene. "
            "To prevent that looking at a very bright object makes the rest of the scene really dark, use higher values.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "float getMinAutoExposure()", "Gets the min auto exposure.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setMaxAutoExposure(float maxAutoExposure)",
            "Sets max auto exposure. Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure brightens a dark scene. "
            "To prevent that looking at a very dark object makes the rest of the scene really bright, use lower values.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "float getMaxAutoExposure()", "Gets max auto exposure.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setBloom(float bloom)", "Sets the bloom intensity. Scale is in lumens / 1024. Valid range is [0.01; 4.9].");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "float getBloom()", "Gets bloom intensity.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "void setEnvMapScale(float envMapScale)",
            "Sets enivornmental scale. Its a global scale that applies to all environment maps (for relevant Hlms implementations, "
            "like PBS). The value will be stored in upperHemisphere.a. Use 1.0 to disable.");
        LuaScriptApi::getInstance()->addClassToCollection("HdrEffectComponent", "float getEnvMapScale()", "Gets the environmental map scale.");
    }

}; // namespace end