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

    namespace
    {
        const Ogre::String ATMOSPHERE_COMPONENT_CLASS_NAME = "AtmosphereComponent";

        // ====================================================================================
        // Diagnostics for the shared lighting state. DELIBERATELY file local: temporary
        // troubleshooting scaffolding for the "who writes the directional light and the scene
        // ambient last" question, nothing of it is declared in the header. Deleting this block
        // plus its call sites removes the instrumentation completely. It reads the component
        // through its PUBLIC getters only, which is why it needs no access to internals.
        // ====================================================================================
        void logLightingState(const Ogre::String& context, const Ogre::String& gameObjectName, HdrEffectComponent* component, Ogre::SceneManager* sceneManager, Ogre::Real powerScale)
        {
            if (nullptr == component)
            {
                return;
            }

            Ogre::String description;

            if (nullptr == sceneManager)
            {
                description = "<no scene manager>";
            }
            else
            {
                const Ogre::ColourValue upperHemisphere = sceneManager->getAmbientLightUpperHemisphere();
                const Ogre::ColourValue lowerHemisphere = sceneManager->getAmbientLightLowerHemisphere();

                // Same read out as the probe in AtmosphereComponent.cpp, so both sides of the
                // conflict produce comparable lines in one and the same log. The envmap scale
                // has no getter of its own - it lives in upperHemisphere.a.
                description = "powerScale=" + Ogre::StringConverter::toString(powerScale) + " envMapScale=" + Ogre::StringConverter::toString(upperHemisphere.a) +
                              " upper=" + Ogre::StringConverter::toString(Ogre::Vector3(upperHemisphere.r, upperHemisphere.g, upperHemisphere.b)) +
                              " lower=" + Ogre::StringConverter::toString(Ogre::Vector3(lowerHemisphere.r, lowerHemisphere.g, lowerHemisphere.b));
            }

            description += " | want: sunPower=" + Ogre::StringConverter::toString(component->getSunPower()) + " envMapScale=" + Ogre::StringConverter::toString(component->getEnvMapScale()) +
                           " exposure=" + Ogre::StringConverter::toString(component->getExposure()) + " minAuto=" + Ogre::StringConverter::toString(component->getMinAutoExposure()) +
                           " maxAuto=" + Ogre::StringConverter::toString(component->getMaxAutoExposure()) + " effect=" + component->getEffectName() +
                           " | atmosphereOwnsLighting=" + Ogre::String(component->isAtmosphereOwningLighting() ? "true" : "false");

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][LIGHTING] " + context + " for '" + gameObjectName + "': " + description);
        }

        /**
         * Makes sure a compositor material is actually LOADED before anybody writes shader
         * constants into it, and reports what really arrived.
         *
         * Ogre::MaterialManager::getByName() hands out the material in whatever state it happens
         * to be in. A material parsed from script but never loaded has no compiled GPU program
         * yet, and the named constant map lives in the GPU program - so
         * GpuProgramParameters::setNamedConstant() has nothing to write into and the value is
         * silently dropped. The existing "Applied exposure: ..." line is printed unconditionally
         * and therefore says nothing about whether the write landed.
         *
         * @return True if the material carries a resolvable named constant of that name.
         */
        bool prepareCompositorMaterial(const Ogre::MaterialPtr& material, const Ogre::String& materialName, const Ogre::String& constantName)
        {
            if (true == material.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][MATERIAL] '" + materialName + "' not found at all.");
                return false;
            }

            const bool wasLoaded = material->isLoaded();
            if (false == wasLoaded)
            {
                material->load();
            }

            Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            const bool constantResolvable = (nullptr != psParams->_findNamedConstantDefinition(constantName));

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][MATERIAL] '" + materialName + "' wasLoaded=" + Ogre::String(wasLoaded ? "true" : "false") +
                                                                                    " isLoadedNow=" + Ogre::String(material->isLoaded() ? "true" : "false") + " hasFragmentProgram=" + Ogre::String(pass->hasFragmentProgram() ? "true" : "false") +
                                                                                    " '" + constantName + "' resolvable=" + Ogre::String(constantResolvable ? "true" : "false"));

            return constantResolvable;
        }

        /**
         * Reads a named constant back out of the pass after it was written, so the log shows the
         * value that is actually in the parameter block rather than the value somebody intended
         * to put there.
         */
        void logNamedConstantReadback(Ogre::Pass* pass, const Ogre::String& materialName, const Ogre::String& constantName, unsigned int componentCount)
        {
            if (nullptr == pass)
            {
                return;
            }

            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            const Ogre::GpuConstantDefinition* definition = psParams->_findNamedConstantDefinition(constantName);

            if (nullptr == definition)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][MATERIAL] READBACK '" + materialName + "' / '" + constantName + "': constant does NOT exist, the write was silently dropped.");
                return;
            }

            const float* values = psParams->getFloatPointer(definition->physicalIndex);
            Ogre::String readback;
            for (unsigned int i = 0; i < componentCount; i++)
            {
                readback += (0u == i ? "" : ", ") + Ogre::StringConverter::toString(values[i]);
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][MATERIAL] READBACK '" + materialName + "' / '" + constantName + "' = " + readback);
        }
    }

    HdrEffectComponent::HdrEffectComponent() :
        GameObjectComponent(),
        name("HdrEffectComponent"),
        effectName(new Variant(HdrEffectComponent::AttrEffectName(),
            {"Bright, sunny day", "Scary Night", "Dark Night", "Dream Night", "Average, slightly hazy day", "Heavy overcast day", "Gibbous moon night", "JJ Abrams style", "Black Night", "Golden Hour", "Dawn", "Dusk", "Stormy", "Underwater",
                "Alien World", "Foggy Morning", "Foggy Day", "Desert Noon", "Arctic Day", "Neon Night", "Volcanic", "Moonless Night", "Custom"},
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
        isApplyingPreset(false),
        oldLightPowerScale(3.14159f),
        hasConnectLightingSnapshot(false),
        connectOldLightPowerScale(3.14159f),
        connectOldAmbientUpperHemisphere(Ogre::ColourValue::Black),
        connectOldAmbientLowerHemisphere(Ogre::ColourValue::Black),
        connectOldAmbientHemisphereDir(Ogre::Vector3::UNIT_Y),
        connectOldEnvMapScale(1.0f)
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

    /**
     * Ownership rule for the two pieces of GLOBAL lighting state.
     *
     * The directional light's power scale and SceneManager's ambient light (upper and lower
     * hemisphere plus the envmap scale packed into upperHemisphere.a) exist exactly once per
     * scene, and two components write them.
     *
     * Ogre::AtmosphereNpr::syncToLight(), reached from setPreset() and therefore from
     * updatePreset(), writes the light direction, the diffuse and specular colour, the power
     * scale from Preset::linkedLightPower and the ambient light from
     * Preset::linkedSceneAmbientUpper/LowerPower and Preset::envmapScale. AtmosphereComponent
     * drives that from a tracked closure, so it happens on EVERY render frame.
     * HdrEffectComponent wrote the same state once per connect() and lost it again one frame
     * later - that is the "HDR only works on the second connect" symptom, and the difference
     * between the first and later runs was only the timing of that race.
     *
     * The atmosphere wins by design: it simulates the time of day and its presets carry a
     * complete lighting description per time slot. HdrEffectComponent is then strictly a TONE
     * MAPPING component - exposure, auto exposure clamp, bloom threshold and the compositor sky
     * colour - and does not touch light or ambient at all. Its SunPower, UpperHemisphere,
     * LowerHemisphere and EnvMapScale values are still stored and still saved, they simply stay
     * inert as long as an activated atmosphere is present.
     */
    bool HdrEffectComponent::isAtmosphereOwningLighting(void) const
    {
        GameObjectController* gameObjectController = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController();
        if (nullptr == gameObjectController)
        {
            return false;
        }

        // Looked up by class name on purpose: AtmosphereComponent is a plugin and must not be
        // included from here, and the state it owns is global anyway - an atmosphere on ANY game
        // object claims it, not only one sharing this camera. In this scene MainCamera and
        // GameCamera share the same MAIN_LIGHT, so a per game object check would let the other
        // camera's HdrEffectComponent keep fighting the atmosphere.
        std::vector<GameObjectPtr> gameObjects = gameObjectController->getGameObjectsFromComponent(ATMOSPHERE_COMPONENT_CLASS_NAME);

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            if (nullptr == gameObjects[i])
            {
                continue;
            }

            auto atmosphereCompPtr = NOWA::makeStrongPtr(gameObjects[i]->getComponentFromName<GameObjectComponent>(ATMOSPHERE_COMPONENT_CLASS_NAME));
            if (nullptr != atmosphereCompPtr && true == atmosphereCompPtr->isActivated())
            {
                return true;
            }
        }

        return false;
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

        // this->postApplySunPower();

        return true;
    }

    bool HdrEffectComponent::connect(void)
    {
        GameObjectComponent::connect();

        logLightingState("connect() ENTER", this->gameObjectPtr->getName(), this, this->gameObjectPtr->getSceneManager(), (nullptr != this->lightDirectionalComponent) ? this->lightDirectionalComponent->getPowerScale() : -1.0f);

        auto cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
        if (nullptr != cameraCompPtr)
        {
            // Note: If its a camera, e.g. MainCamera and hdr effect shall be set, but there is split screen scenario active, the hdr effect may not be set for a camera, which is not involved in split screen scenario
            if ((true == cameraCompPtr->isActivated() && false == AppStateManager::getSingletonPtr()->getWorkspaceModule()->getSplitScreenScenarioActive()) || true == this->workspaceBaseComponent->getInvolvedInSplitScreen())
            {
                // FIX/FEATURE: snapshot whatever lighting was in effect right
                // before HDR touches anything - directional light power scale is
                // GLOBAL (there is only one directional light for the whole
                // scene), so if HDR overrides it (e.g. to 200 for a bright
                // preset) and that override is never reverted, the scene can get
                // saved with the blown-out value baked in, making the next load
                // start already overexposed. disconnect() -> resetShining() uses
                // this snapshot to put everything back exactly as it was.
                // No snapshot and no restore when the atmosphere owns the lighting: nothing is
                // overridden, so there would be nothing to put back. Leaving
                // hasConnectLightingSnapshot at false also keeps resetShining() out of the way,
                // which would otherwise fight the atmosphere on every disconnect().
                if (false == isAtmosphereOwningLighting() && nullptr != this->lightDirectionalComponent)
                {
                    this->connectOldLightPowerScale = this->lightDirectionalComponent->getPowerScale();
                }

                Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
                if (nullptr != sceneManager)
                {
                    this->connectOldAmbientUpperHemisphere = sceneManager->getAmbientLightUpperHemisphere();
                    this->connectOldAmbientLowerHemisphere = sceneManager->getAmbientLightLowerHemisphere();
                    this->connectOldAmbientHemisphereDir = sceneManager->getAmbientLightHemisphereDir();
                    // SceneManager has no dedicated getter for the envmap scale - it is stored
                    // in the alpha channel of the upper hemisphere colour (see the doc comment on
                    // SceneManager::setAmbientLight). This snapshot used to be commented out, so
                    // resetShining() restored whatever connectOldEnvMapScale happened to be
                    // initialised to instead of the value that was actually active.
                    this->connectOldEnvMapScale = this->connectOldAmbientUpperHemisphere.a;
                }
                this->hasConnectLightingSnapshot = (false == isAtmosphereOwningLighting());

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[HdrEffectComponent] connect() -> calling applyCurrentValues (DIRECT) for '" + this->gameObjectPtr->getName() + "', snapshot powerScale=" + Ogre::StringConverter::toString(this->connectOldLightPowerScale));

                this->postApplySunPower();

                this->applyCurrentValues();

                // Apply loaded effect
                this->setEffectName(this->effectName->getListSelectedValue());

                logLightingState("connect() EXIT (applied)", this->gameObjectPtr->getName(), this, this->gameObjectPtr->getSceneManager(), (nullptr != this->lightDirectionalComponent) ? this->lightDirectionalComponent->getPowerScale() : -1.0f);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][LIGHTING] connect() SKIPPED for '" + this->gameObjectPtr->getName() + "': camera not activated or not involved in the split screen scenario.");
            }
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent][LIGHTING] connect() SKIPPED for '" + this->gameObjectPtr->getName() + "': no CameraComponent on this game object.");
        }
        return true;
    }

    bool HdrEffectComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        logLightingState("disconnect() ENTER", this->gameObjectPtr->getName(), this, this->gameObjectPtr->getSceneManager(), (nullptr != this->lightDirectionalComponent) ? this->lightDirectionalComponent->getPowerScale() : -1.0f);

        this->resetShining();

        logLightingState("disconnect() EXIT", this->gameObjectPtr->getName(), this, this->gameObjectPtr->getSceneManager(), (nullptr != this->lightDirectionalComponent) ? this->lightDirectionalComponent->getPowerScale() : -1.0f);
        return true;
    }

    void HdrEffectComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &HdrEffectComponent::handleHdrActivated), NOWA::EventDataHdrActivated::getStaticEventType());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] Destructor hdr effect component for game object: " + this->gameObjectPtr->getName());

        // FIX: resetShining() needs this->lightDirectionalComponent (and the
        // scene manager) to still be valid to actually restore the saved
        // snapshot - it used to run AFTER both were already nulled below,
        // which meant the light power scale never got restored on removal.
        this->resetShining();

        this->lightDirectionalComponent = nullptr;
        this->workspaceBaseComponent = nullptr;
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
        boost::shared_ptr<EventDataHdrActivated> castEventData = boost::static_pointer_cast<EventDataHdrActivated>(eventData);

        // FIX: this condition was inverted - it early-returned exactly when the
        // event DID concern this game object's workspace (IDs equal), and only
        // fell through to applyCurrentValues() when the IDs did NOT match (i.e.
        // for a workspace belonging to some other game object entirely). With a
        // single HDR workspace/camera in the scene, that meant this corrective
        // re-apply - meant to fix up the luminance measurement window once the
        // viewport rect is finalized - never ran at all on the first connect(),
        // leaving auto-exposure to react to a wrong/degenerate measurement
        // region and render the scene too dark. A second connect() only looked
        // right because by then the viewport was already correctly sized from
        // the first run, so connect()'s own direct applyCurrentValues() call was
        // sufficient without needing this event-triggered correction.
        if (castEventData->getGameObjectId() != this->gameObjectPtr->getId())
        {
            return;
        }

        // FIX: this listener is registered once in postInit(), which runs at
        // scene LOAD time - independent of whether connect() (simulation start)
        // has ever run. WorkspaceBaseComponent::setViewportRect() can fire this
        // event purely from editor/initial workspace setup, before Play is ever
        // pressed. Without this guard, applyCurrentValues() applied the HDR
        // preset (e.g. power scale 200) immediately on load - exactly the thing
        // connect() is supposed to be the sole gatekeeper for.
        if (false == this->bConnected)
        {
            return;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] handleHdrActivated -> calling applyCurrentValues (EVENT-triggered) for '" + this->gameObjectPtr->getName() + "'");

        // The workspace has just been (re)created. Any apply calls made before
        // this point early-returned because the workspace did not exist yet,
        // hence the CURRENT values must be (re)applied now - never hardcoded
        // reset defaults, which would silently neutralize the active preset.
        logLightingState("handleHdrActivated() (EVENT-triggered)", this->gameObjectPtr->getName(), this, this->gameObjectPtr->getSceneManager(), (nullptr != this->lightDirectionalComponent) ? this->lightDirectionalComponent->getPowerScale() : -1.0f);

        this->applyCurrentValues();
    }

    void HdrEffectComponent::applyCurrentValues(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] applyCurrentValues ENTER for '" + this->gameObjectPtr->getName() +
                                                                               "', workspace ready=" + Ogre::String((nullptr != this->workspaceBaseComponent && nullptr != this->workspaceBaseComponent->getWorkspace()) ? "true" : "false") +
                                                                               ", hasConnectLightingSnapshot=" + Ogre::String(this->hasConnectLightingSnapshot ? "true" : "false"));

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

        // Track the active viewport region for the luminance measurement,
        // so editor letterboxing or custom viewports do not poison the auto exposure.
        if (nullptr != this->workspaceBaseComponent)
        {
            this->applyLuminanceViewportRect(this->workspaceBaseComponent->getViewportRect());
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[HdrEffectComponent] applyCurrentValues EXIT for '" + this->gameObjectPtr->getName() + "', hasConnectLightingSnapshot=" + Ogre::String(this->hasConnectLightingSnapshot ? "true" : "false"));
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
        if (true == Core::getSingletonPtr()->getIsSceneBeingDestroyed())
        {
            return;
        }

        // FIX: this used to be entirely #if-0'd out, and even enabled it only
        // ever wrote hardcoded fallback values - never the lighting that was
        // actually active before connect() applied HDR. That meant an HDR
        // override (e.g. directional light power scale 200 for a bright
        // preset) never got reverted on disconnect, since the directional
        // light is global (only one for the whole scene) - the blown-out
        // state could even get saved into the scene file and reappear
        // immediately blown out on next load.
        if (false == this->hasConnectLightingSnapshot)
        {
            // connect() never actually applied HDR for this component (e.g.
            // its camera was inactive, or split-screen didn't involve it) -
            // nothing was overridden, so there is nothing to restore.
            return;
        }

        if (nullptr != this->lightDirectionalComponent)
        {
            Ogre::Real restorePowerScale = this->connectOldLightPowerScale;
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, restorePowerScale]()
            {
                this->lightDirectionalComponent->setPowerScale(restorePowerScale);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "HdrEffectComponent::resetShining");
        }

        if (nullptr != this->gameObjectPtr)
        {
            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
            if (nullptr != sceneManager)
            {
                // Through the render thread like every other lighting write in this component.
                // Writing the ambient light straight from the logic thread races against
                // AtmosphereComponent's per frame closure, which writes the very same state.
                const Ogre::ColourValue restoreUpper = this->connectOldAmbientUpperHemisphere;
                const Ogre::ColourValue restoreLower = this->connectOldAmbientLowerHemisphere;
                const Ogre::Vector3 restoreDir = this->connectOldAmbientHemisphereDir;
                const Ogre::Real restoreEnvMapScale = this->connectOldEnvMapScale;

                NOWA::GraphicsModule::RenderCommand ambientRdCmd = [sceneManager, restoreUpper, restoreLower, restoreDir, restoreEnvMapScale]()
                {
                    sceneManager->setAmbientLight(restoreUpper, restoreLower, restoreDir, restoreEnvMapScale);
                };
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(ambientRdCmd), "HdrEffectComponent::resetShining");
            }
        }

        this->hasConnectLightingSnapshot = false;
    }

    void HdrEffectComponent::postApplySunPower(void)
    {
        // The null check below is worthless as long as the pointer is dereferenced above it.
        // onOtherComponentRemoved() sets lightDirectionalComponent to nullptr, and postInit()
        // leaves it null when the scene has no LightDirectionalComponent on the main light.
        if (nullptr == this->lightDirectionalComponent)
        {
            return;
        }

        if (true == isAtmosphereOwningLighting())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] postApplySunPower skipped for '" + this->gameObjectPtr->getName() + "': an activated AtmosphereComponent owns the directional light.");
            return;
        }

        this->oldLightPowerScale = this->lightDirectionalComponent->getPowerScale();
        // The presets are calibrated like the Ogre HDR sample: powerScale is
        // lumens / 1024, e.g. 97 = direct sunlight (~100.000 lumens).
        // NO multiplication by PI here, and setEffectName must not do it either,
        // otherwise the last writer wins and the sun power differs by factor 3.14
        // depending on call order.
        {
            Ogre::Real targetPowerScale = this->sunPower->getReal();
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[HdrEffectComponent] postApplySunPower for '" + this->gameObjectPtr->getName() + "': " + Ogre::StringConverter::toString(this->oldLightPowerScale) + " -> " + Ogre::StringConverter::toString(targetPowerScale));

            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, targetPowerScale]
            {
                this->lightDirectionalComponent->setPowerScale(targetPowerScale);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HdrEffectComponent] postApplySunPower (render thread) actual powerScale now: " + Ogre::StringConverter::toString(this->lightDirectionalComponent->getPowerScale()));
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::postApplySunPower");
        }
    }

    void HdrEffectComponent::applyHdrSkyColor(const Ogre::ColourValue& color, Ogre::Real multiplier)
    {
        // FIX: this used to call this->resetShining() when the workspace wasn't
        // ready yet (e.g. on the very first connect(), before WorkspaceBaseComponent
        // has finished (re)creating its Ogre::CompositorWorkspace). That was
        // harmless back when resetShining() was dead code (#if 0'd out), but
        // resetShining() now does REAL work - it restores the connect-time
        // lighting snapshot and clears hasConnectLightingSnapshot. Calling it
        // here mid-applyCurrentValues() (itself called from connect(), right
        // after the snapshot was taken and the bright values were just set)
        // immediately undid the just-applied brightness and desynced the
        // snapshot flag - the root cause of "first connect looks dark/wrong,
        // second connect looks right" depending on exactly when the workspace
        // became available. Simply skip sky-colour application this call - it
        // gets correctly re-applied once handleHdrActivated() fires after the
        // workspace truly exists.
        if (nullptr == this->workspaceBaseComponent || false == this->workspaceBaseComponent->getUseHdr() || nullptr == this->workspaceBaseComponent->getWorkspace())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[HdrEffectComponent] applyHdrSkyColor: workspace not ready yet for game object '" + this->gameObjectPtr->getName() + "' - skipping sky colour for now, will be re-applied once the workspace exists.");
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
                prepareCompositorMaterial(skyMaterial, "NOWASkyPostprocess", "hdrSkyPower");

                Ogre::Pass* skyPass = skyMaterial->getTechnique(0)->getPass(0);
                Ogre::GpuProgramParametersSharedPtr psParams = skyPass->getFragmentProgramParameters();

                // 0.372 = luminance of the demo base sky colour (0.2, 0.4, 0.6).
                // skyBoxCompensation corrects for the inherent brightness of the
                // cubemap texels relative to that base: a cubemap whose sky region
                // averages ~0.7 luminance needs ~0.5 to land on the preset target.

                const Ogre::ColourValue hdrSkyColor = color * multiplier;
                // Play with this value:
                const Ogre::Real skyBoxCompensation = 0.5f;
                Ogre::Real hdrSkyPower = (0.2125f * hdrSkyColor.r + 0.7154f * hdrSkyColor.g + 0.0721f * hdrSkyColor.b) / 0.372f * skyBoxCompensation;

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

        // Force the material to be loaded first. Without a compiled GPU program there is no named
        // constant map, so setNamedConstant() below writes into nothing and the exposure silently
        // keeps the value from the material script - which looks exactly like "HDR only works on
        // the second run", because by the second run the material has been loaded by the first.
        prepareCompositorMaterial(material, "HDR/DownScale03_SumLumEnd", "exposure");

        Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

        // 1024 = HDR calibration factor (-10 stops), e is the correct base, NOT 2.
        // 1024 * exp(0 - 2) = 138.5833, which matches the material default.
        const Ogre::Vector3 exposureParams(1024.0f * expf(exposure - 2.0f), 7.5f - maxAutoExposure, 7.5f - minAutoExposure);

        NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, pass, exposureParams]
        {
            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            psParams->setNamedConstant("exposure", exposureParams);

            logNamedConstantReadback(pass, "HDR/DownScale03_SumLumEnd", "exposure", 3u);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "HdrEffectComponent::applyExposure");

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

        prepareCompositorMaterial(material, "HDR/BrightPass_Start", "brightThreshold");

        Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

        const Ogre::Vector4 brightThreshold(minThreshold, 1.0f / (fullThreshold - minThreshold), 0.0f, 0.0f);

        NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, pass, brightThreshold]
        {
            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            psParams->setNamedConstant("brightThreshold", brightThreshold);

            logNamedConstantReadback(pass, "HDR/BrightPass_Start", "brightThreshold", 2u);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "HdrEffectComponent::applyBloomThreshold");
    }

    void HdrEffectComponent::applyLuminanceViewportRect(const Ogre::Vector4& viewportRect)
    {
        // Restricts the HDR luminance measurement to the region of rt0 that
        // actually contains the rendered scene. In NOWA-Design the 3D view is
        // letterboxed inside the render window (editor panels above and below),
        // and averaging the black bars into the luminance poisons the auto
        // exposure: the chain concludes 'dark frame' and brightens to the clamp
        // limit, washing out the whole image.
        Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName("HDR/DownScale01_SumLumStart", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        if (true == material.isNull())
        {
            Ogre::LogManager::getSingleton().logMessage("[HdrEffectComponent] ERROR: HDR/DownScale01_SumLumStart material not found!");
            return;
        }

        Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, pass, viewportRect]
        {
            Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
            if (nullptr != psParams->_findNamedConstantDefinition("viewportRect"))
            {
                psParams->setNamedConstant("viewportRect", viewportRect);
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[HdrEffectComponent] DownScale01_SumLumStart has no 'viewportRect' uniform, luminance measurement will include letterbox bars!");
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "HdrEffectComponent::applyLuminanceViewportRect");
    }

    void HdrEffectComponent::setEffectName(const Ogre::String& effectName)
    {
        this->effectName->setListSelectedValue(effectName);

        if (nullptr != this->lightDirectionalComponent)
        {
            this->oldLightPowerScale = this->lightDirectionalComponent->getPowerScale();
        }

        // ====================================================================
        // Preset calibration philosophy (post aspect-ratio fix):
        //
        // With auto exposure working correctly, the ABSOLUTE sky/sun/ambient
        // brightness mostly affects mood and which surfaces catch light. What
        // controls the final on-screen brightness is the exposure bias plus the
        // min/max auto exposure CLAMP.
        //
        // Day presets:   moderate symmetric clamp so the view adapts freely but
        //                cannot wash out. Use 'exposure' for fine brightness.
        // Night presets: NARROW clamp pinned low (min approx equals max) so the
        //                look stays fixed and dark. Auto exposure must NOT be
        //                allowed to brighten a night back into visibility,
        //                otherwise the night looks like an underexposed day.
        //
        // Sun power is raw (lumens / 1024), NO multiplication by PI anywhere.
        // ====================================================================

        if ("Bright, sunny day" == effectName)
        {
            // Perfect summer day - intense sun, clear blue sky. Adapts.
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 60.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 4.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.925f);
            this->sunPower->setValue(200.0f);
            this->exposure->setValue(-0.3f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.0f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(16.0f);
        }
        else if ("Scary Night" == effectName)
        {
            this->skyColor->setValue(Ogre::Vector4(0.02f, 0.03f, 0.08f, 1.0f) * 0.15f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.05f, 0.08f, 0.15f, 1.0f) * 0.15f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.02f, 0.04f, 0.08f, 1.0f) * 0.1f);
            this->sunPower->setValue(8.0f);
            this->exposure->setValue(-0.5f);
            this->minAutoExposure->setValue(-1.2f);
            this->maxAutoExposure->setValue(-0.6f);
            this->bloom->setValue(0.5f);
            this->envMapScale->setValue(0.1f);
        }
        else if ("Dark Night" == effectName)
        {
            this->skyColor->setValue(Ogre::Vector4(0.01f, 0.01f, 0.03f, 1.0f) * 0.05f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.02f, 0.03f, 0.06f, 1.0f) * 0.08f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.01f, 0.02f, 0.04f, 1.0f) * 0.05f);
            this->sunPower->setValue(5.0f);
            this->exposure->setValue(-0.8f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(-0.9f);
            this->bloom->setValue(0.2f);
            this->envMapScale->setValue(0.02f);
        }
        else if ("Dream Night" == effectName)
        {
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.25f, 0.4f, 1.0f) * 3.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.35f, 0.5f, 1.0f) * 0.8f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.25f, 0.3f, 0.45f, 1.0f) * 0.5f);
            this->sunPower->setValue(1.0f);
            this->exposure->setValue(-0.4f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.0f);
            this->bloom->setValue(0.0f);
            this->envMapScale->setValue(4.0f);
        }
        else if ("Average, slightly hazy day" == effectName)
        {
            // Overcast but decent lighting - soft shadows. Adapts.
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 32.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 3.15f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 2.0475f);
            this->sunPower->setValue(48.0f);
            this->exposure->setValue(-0.2f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.2f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(8.0f);
        }
        else if ("Heavy overcast day" == effectName)
        {
            // Thick cloud cover - dull, flat. Adapts narrowly.
            this->skyColor->setValue(Ogre::Vector4(0.4f, 0.4f, 0.4f, 1.0f) * 4.5f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.4f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.5f, 1.0f) * 0.365625f);
            this->sunPower->setValue(20.0f);
            this->exposure->setValue(-0.2f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(0.8f);
            this->bloom->setValue(4.0f);
            this->envMapScale->setValue(0.5f);
        }
        else if ("Gibbous moon night" == effectName)
        {
            // Bright moonlit night - you can see, but it reads as night. Fixed-ish.
            this->skyColor->setValue(Ogre::Vector4(0.3f, 0.35f, 0.6f, 1.0f) * 2.5f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.35f, 0.4f, 0.65f, 1.0f) * 0.6f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.35f, 0.5f, 1.0f) * 0.35f);
            this->sunPower->setValue(2.5f);
            this->exposure->setValue(-1.0f);
            this->minAutoExposure->setValue(-2.0f);
            this->maxAutoExposure->setValue(1.0f);
            this->bloom->setValue(2.5f);
            this->envMapScale->setValue(2.0f);
        }
        else if ("JJ Abrams style" == effectName)
        {
            // Cinematic lens flare style - dramatic. Adapts, bright-biased.
            this->skyColor->setValue(Ogre::Vector4(0.2f, 0.4f, 0.6f, 1.0f) * 6.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.50f, 0.7f, 1.0f) * 0.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.6f, 0.45f, 0.3f, 1.0f) * 0.3f);
            this->sunPower->setValue(8.0f);
            this->exposure->setValue(0.2f);
            this->minAutoExposure->setValue(-0.5f);
            this->maxAutoExposure->setValue(1.5f);
            this->bloom->setValue(4.0f);
            this->envMapScale->setValue(1.0f);
        }
        else if ("Black Night" == effectName)
        {
            // Near-complete darkness - new moon. Fixed, deliberately minimal.
            this->skyColor->setValue(Ogre::Vector4(0.005f, 0.005f, 0.01f, 1.0f) * 0.02f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.01f, 0.01f, 0.02f, 1.0f) * 0.005f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.005f, 0.005f, 0.01f, 1.0f) * 0.002f);
            this->sunPower->setValue(3.1f);
            this->exposure->setValue(-2.5f);
            this->minAutoExposure->setValue(-3.5f);
            this->maxAutoExposure->setValue(-3.2f);
            this->bloom->setValue(0.01f);
            this->envMapScale->setValue(0.001f);
        }
        else if ("Golden Hour" == effectName)
        {
            // Sunset/sunrise magic hour - warm, dramatic. Adapts.
            this->skyColor->setValue(Ogre::Vector4(0.8f, 0.5f, 0.3f, 1.0f) * 35.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.9f, 0.7f, 0.5f, 1.0f) * 3.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.7f, 0.5f, 0.4f, 1.0f) * 2.2f);
            this->sunPower->setValue(45.0f);
            this->exposure->setValue(-0.1f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.2f);
            this->bloom->setValue(6.0f);
            this->envMapScale->setValue(12.0f);
        }
        else if ("Dawn" == effectName)
        {
            // Early morning before sunrise - cool blues to warmth. Adapts.
            this->skyColor->setValue(Ogre::Vector4(0.4f, 0.5f, 0.7f, 1.0f) * 8.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.6f, 0.8f, 1.0f) * 1.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.7f, 0.6f, 0.5f, 1.0f) * 0.8f);
            this->sunPower->setValue(18.0f);
            this->exposure->setValue(-0.5f);
            this->minAutoExposure->setValue(-1.8f);
            this->maxAutoExposure->setValue(0.8f);
            this->bloom->setValue(0.1f);
            this->envMapScale->setValue(5.0f);
        }
        else if ("Dusk" == effectName)
        {
            // Late evening twilight - purple/blue. Adapts darkly.
            this->skyColor->setValue(Ogre::Vector4(0.3f, 0.35f, 0.6f, 1.0f) * 2.5f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.35f, 0.4f, 0.65f, 1.0f) * 0.6f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.35f, 0.5f, 1.0f) * 0.35f);
            this->sunPower->setValue(2.5f);
            this->exposure->setValue(-1.0f);
            this->minAutoExposure->setValue(-1.0f);
            this->maxAutoExposure->setValue(4.0f);
            this->bloom->setValue(2.5f);
            this->envMapScale->setValue(2.0f);
        }
        else if ("Stormy" == effectName)
        {
            // Storm approaching - dark, dramatic, greenish. Adapts darkly.
            this->skyColor->setValue(Ogre::Vector4(0.25f, 0.3f, 0.25f, 1.0f) * 2.5f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.35f, 0.4f, 0.35f, 1.0f) * 0.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.25f, 0.3f, 0.25f, 1.0f) * 0.3f);
            this->sunPower->setValue(10.0f);
            this->exposure->setValue(0.8f);
            this->minAutoExposure->setValue(-2.0f);
            this->maxAutoExposure->setValue(1.0f);
            this->bloom->setValue(1.5f);
            this->envMapScale->setValue(0.3f);
        }
        else if ("Underwater" == effectName)
        {
            // Submerged - blue/green. Adapts.
            this->skyColor->setValue(Ogre::Vector4(0.1f, 0.3f, 0.4f, 1.0f) * 8.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.15f, 0.35f, 0.5f, 1.0f) * 1.8f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.05f, 0.15f, 0.25f, 1.0f) * 0.9f);
            this->sunPower->setValue(25.0f);
            this->exposure->setValue(-0.3f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(3.0f);
            this->bloom->setValue(0.01f);
            this->envMapScale->setValue(4.0f);
        }
        else if ("Alien World" == effectName)
        {
            // Sci-fi exoplanet - purple/magenta. Adapts.
            // this->skyColor->setValue(Ogre::Vector4(0.6f, 0.2f, 0.7f, 1.0f) * 40.0f);
            this->skyColor->setValue(Ogre::Vector4(2.66f, 1.14f, 3.04f, 3.8f));
            this->upperHemisphere->setValue(Ogre::Vector4(0.7f, 0.3f, 0.8f, 1.0f) * 3.8f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.5f, 0.6f, 1.0f) * 2.5f);
            this->sunPower->setValue(85.0f);
            this->exposure->setValue(-0.2f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.3f);
            this->bloom->setValue(5.5f);
            this->envMapScale->setValue(14.0f);
        }
        else if ("Foggy Morning" == effectName)
        {
            // Dense fog - muted, low contrast. Adapts narrowly.
            this->skyColor->setValue(Ogre::Vector4(0.8f, 0.8f, 0.65f, 1.0f) * 12.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.55f, 0.55f, 0.6f, 1.0f) * 2.0f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.55f, 1.0f) * 1.6f);
            this->sunPower->setValue(50.0f);
            this->exposure->setValue(1.0f);
            this->minAutoExposure->setValue(2.0f);
            this->maxAutoExposure->setValue(2.0f);
            this->bloom->setValue(3.8f);
            this->envMapScale->setValue(3.0f);
        }
        else if ("Foggy Day" == effectName)
        {
            // Dense fog - muted, low contrast. Adapts narrowly.
            this->skyColor->setValue(Ogre::Vector4(0.6f, 0.6f, 0.65f, 1.0f) * 12.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.55f, 0.55f, 0.6f, 1.0f) * 2.0f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.5f, 0.5f, 0.55f, 1.0f) * 1.6f);
            this->sunPower->setValue(22.0f);
            this->exposure->setValue(1.0f);
            this->minAutoExposure->setValue(2.0f);
            this->maxAutoExposure->setValue(2.0f);
            this->bloom->setValue(0.0f);
            this->envMapScale->setValue(3.0f);
        }
        else if ("Desert Noon" == effectName)
        {
            // Harsh desert midday - very bright. Adapts, bright-clamped low to tame it.
            this->skyColor->setValue(Ogre::Vector4(0.5f, 0.6f, 0.8f, 1.0f) * 85.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.8f, 0.75f, 0.7f, 1.0f) * 6.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.85f, 0.7f, 0.6f, 1.0f) * 5.2f);
            this->sunPower->setValue(120.0f);
            this->exposure->setValue(0.1f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(0.8f);
            this->bloom->setValue(6.0f);
            this->envMapScale->setValue(18.0f);
        }
        else if ("Arctic Day" == effectName)
        {
            // Polar - bright snow reflection, cool. Adapts, clamped to avoid glare blowout.
            this->skyColor->setValue(Ogre::Vector4(0.6f, 0.7f, 0.9f, 1.0f) * 45.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.7f, 0.8f, 0.95f, 1.0f) * 4.2f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.85f, 0.9f, 0.95f, 1.0f) * 4.8f);
            this->sunPower->setValue(75.0f);
            this->exposure->setValue(-0.4f);
            this->minAutoExposure->setValue(1.5f);
            this->maxAutoExposure->setValue(0.8f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(15.0f);
        }
        else if ("Neon Night" == effectName)
        {
            // Cyberpunk city night - colorful artificial light. Fixed-ish, dark.
            this->skyColor->setValue(Ogre::Vector4(0.15f, 0.1f, 0.25f, 1.0f) * 0.8f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.3f, 0.2f, 0.4f, 1.0f) * 0.4f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.4f, 0.15f, 0.3f, 1.0f) * 0.6f);
            this->sunPower->setValue(1.5f);
            this->exposure->setValue(-1.0f);
            this->minAutoExposure->setValue(-1.8f);
            this->maxAutoExposure->setValue(3.0f);
            this->bloom->setValue(4.5f);
            this->envMapScale->setValue(3.0f);
        }
        else if ("Volcanic" == effectName)
        {
            // Near active volcano - red/orange glow, ash. Adapts darkly.
            this->skyColor->setValue(Ogre::Vector4(0.4f, 0.25f, 0.2f, 1.0f) * 8.0f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.5f, 0.3f, 0.25f, 1.0f) * 1.5f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.8f, 0.3f, 0.2f, 1.0f) * 2.5f);
            this->sunPower->setValue(15.0f);
            this->exposure->setValue(-0.3f);
            this->minAutoExposure->setValue(-1.5f);
            this->maxAutoExposure->setValue(1.0f);
            this->bloom->setValue(5.0f);
            this->envMapScale->setValue(6.0f);
        }
        else if ("Moonless Night" == effectName)
        {
            // Clear night, no moon - stars only. Fixed, deepest visible dark.
            this->skyColor->setValue(Ogre::Vector4(0.03f, 0.04f, 0.12f, 1.0f) * 0.08f);
            this->upperHemisphere->setValue(Ogre::Vector4(0.06f, 0.08f, 0.15f, 1.0f) * 0.018f);
            this->lowerHemisphere->setValue(Ogre::Vector4(0.03f, 0.05f, 0.1f, 1.0f) * 0.01f);
            this->sunPower->setValue(0.15f);
            this->exposure->setValue(-2.2f);
            this->minAutoExposure->setValue(-3.0f);
            this->maxAutoExposure->setValue(-2.7f);
            this->bloom->setValue(0.8f);
            this->envMapScale->setValue(0.08f);
        }

        // Apply values
        Ogre::ColourValue skyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);
        this->applyHdrSkyColor(skyColor, 1.0f);
        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());
        this->applyBloomThreshold(std::max(this->bloom->getReal() - 2.0f, 0.0f), std::max(this->bloom->getReal(), 0.01f));

        if (false == isAtmosphereOwningLighting())
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this]
            {
                Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
                Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);

                this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());

                if (nullptr != this->lightDirectionalComponent)
                {
                    this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::setEffectName");
        }
    }

    void HdrEffectComponent::refreshAllParameters()
    {
        if (nullptr != this->lightDirectionalComponent)
        {
            this->oldLightPowerScale = this->lightDirectionalComponent->getPowerScale();
        }

        // Force re-application of all parameters
        this->applyExposure(this->exposure->getReal(), this->minAutoExposure->getReal(), this->maxAutoExposure->getReal());

        this->applyBloomThreshold(std::max(this->bloom->getReal() - 2.0f, 0.0f), std::max(this->bloom->getReal(), 0.01f));

        Ogre::ColourValue skyColor(this->skyColor->getVector4().x, this->skyColor->getVector4().y, this->skyColor->getVector4().z, this->skyColor->getVector4().w);
        this->applyHdrSkyColor(skyColor, 1.0f);

        // Force ambient light update - unless the atmosphere owns it.
        if (true == isAtmosphereOwningLighting())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this]
        {
            Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
            Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);

            this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());

            if (nullptr != this->lightDirectionalComponent)
            {
                this->lightDirectionalComponent->setPowerScale(this->sunPower->getReal());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::setEnvMapScale");
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

        // Value is stored and saved either way, it is only INERT while the atmosphere owns the
        // ambient light.
        if (false == isAtmosphereOwningLighting())
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, upperHemisphere]
            {
                Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
                Ogre::ColourValue ambUpperHemisphere(upperHemisphere.x, upperHemisphere.y, upperHemisphere.z);
                this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::setUpperHemisphere");
        }
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

        if (false == isAtmosphereOwningLighting())
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, lowerHemisphere]
            {
                Ogre::ColourValue ambLowerHemisphere(lowerHemisphere.x, lowerHemisphere.y, lowerHemisphere.z);
                Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);
                this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), this->envMapScale->getReal());
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::setLowerHemisphere");
        }
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

        if (false == isAtmosphereOwningLighting())
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, sunPower]
            {
                if (nullptr != this->lightDirectionalComponent)
                {
                    this->lightDirectionalComponent->setPowerScale(sunPower);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::setSunPower");
        }
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

        if (false == isAtmosphereOwningLighting())
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, envMapScale]
            {
                Ogre::ColourValue ambLowerHemisphere(this->lowerHemisphere->getVector4().x, this->lowerHemisphere->getVector4().y, this->lowerHemisphere->getVector4().z);
                Ogre::ColourValue ambUpperHemisphere(this->upperHemisphere->getVector4().x, this->upperHemisphere->getVector4().y, this->upperHemisphere->getVector4().z);
                this->gameObjectPtr->getSceneManager()->setAmbientLight(ambUpperHemisphere, ambLowerHemisphere, this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir(), envMapScale);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(oceanRdCmd), "HdrEffectComponent::setEnvMapScale");
        }
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