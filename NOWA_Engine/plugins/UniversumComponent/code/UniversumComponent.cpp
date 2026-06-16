#include "NOWAPrecompiled.h"
#include "UniversumComponent.h"

#include "../../AreaOfInterestComponent/code/AreaOfInterestComponent.h"
#include "../../AtmosphereComponent/code/AtmosphereComponent.h"
#include "../../PlanetOceanComponent/code/PlanetOceanComponent.h"
#include "../../PlanetSunComponent/code/PlanetSunComponent.h"
#include "../../PlanetSurfaceComponent/code/PlanetSurfaceComponent.h"
#include "../../PlanetTerraComponent/code/PlanetTerraComponent.h"
#include "../../ProceduralPlanetComponent/code/ProceduralPlanetComponent.h"
#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "gameobject/DatablockPbsComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/InputDeviceComponent.h"
#include "gameobject/LightDirectionalComponent.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsActiveKinematicComponent.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "gameobject/PhysicsComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "modules/WorkspaceModule.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreCamera.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    UniversumComponent::PlanetOrbitObserver::PlanetOrbitObserver(UniversumComponent* owner, unsigned long planetId) : owner(owner), planetId(planetId)
    {
    }

    void UniversumComponent::PlanetOrbitObserver::onEnter(GameObject* gameObject)
    {
        if (gameObject->getId() != this->owner->getPlayerGameObjectId())
        {
            return;
        }

        this->owner->activeTriggerOverlaps.insert(this->planetId);

        if (false == this->owner->getAutoPauseOrbit())
        {
            this->owner->callPlanetEnteredFunction(this->planetId, gameObject->getId());
            return;
        }

        // Never interrupt an active landing/takeoff sequence.
        // The ship is mid-transition and AOI overlaps are transient.
        LandingState currentState = this->owner->getLandingState();
        if (currentState == LandingState::LANDING || currentState == LandingState::LANDED || currentState == LandingState::TAKING_OFF)
        {
            return;
        }

        // If already paused on a DIFFERENT body, resume it first
        unsigned long currentlyPaused = this->owner->getCurrentlyPausedBodyId();
        if (currentlyPaused != 0ul && currentlyPaused != this->planetId)
        {
            this->owner->resumePlanetOrbit(currentlyPaused, gameObject->getId());
        }

        this->owner->pausePlanetOrbit(this->planetId, gameObject->getId());
    }

    void UniversumComponent::PlanetOrbitObserver::onLeave(GameObject* gameObject)
    {
        if (gameObject->getId() != this->owner->getPlayerGameObjectId())
        {
            return;
        }

        this->owner->activeTriggerOverlaps.erase(this->planetId);

        if (false == this->owner->getAutoPauseOrbit())
        {
            this->owner->callPlanetLeftFunction(this->planetId, gameObject->getId());
            return;
        }

        // Never interrupt an active landing/takeoff sequence.
        // When taking off from moon, the ship leaves the moon AOI — but we must
        // NOT resume/reset the landing state machine. The TAKING_OFF state itself
        // transitions to APPROACHING when clearance altitude is reached.
        LandingState currentState = this->owner->getLandingState();
        if (currentState == LandingState::LANDING || currentState == LandingState::LANDED || currentState == LandingState::TAKING_OFF)
        {
            return;
        }

        this->owner->resumePlanetOrbit(this->planetId, gameObject->getId());

        // Only re-pause another body if we are fully in NONE state —
        // never auto-pause during any active state transition.
        if (this->owner->getLandingState() == LandingState::NONE)
        {
            unsigned long fallbackId = this->owner->getInnermostOverlapId();
            if (fallbackId != 0ul)
            {
                this->owner->pausePlanetOrbit(fallbackId, gameObject->getId());
            }
        }
    }

    // =========================================================================
    //  Constructor / Destructor
    // =========================================================================

    UniversumComponent::UniversumComponent() :
        GameObjectComponent(),
        name("UniversumComponent"),
        randomSeed(new Variant(UniversumComponent::AttrRandomSeed(), static_cast<unsigned int>(12345u), this->attributes)),
        solarSystemCount(new Variant(UniversumComponent::AttrSolarSystemCount(), static_cast<unsigned int>(1u), this->attributes)),
        solarSystemDistanceMin(new Variant(UniversumComponent::AttrSolarSystemDistanceMin(), 500.0f, this->attributes)),
        solarSystemDistanceMax(new Variant(UniversumComponent::AttrSolarSystemDistanceMax(), 2000.0f, this->attributes)),
        planetsPerSystemMin(new Variant(UniversumComponent::AttrPlanetsPerSystemMin(), static_cast<unsigned int>(2u), this->attributes)),
        planetsPerSystemMax(new Variant(UniversumComponent::AttrPlanetsPerSystemMax(), static_cast<unsigned int>(6u), this->attributes)),
        useMoons(new Variant(UniversumComponent::AttrUseMoons(), true, this->attributes)),
        moonsPerPlanetMin(new Variant(UniversumComponent::AttrMoonsPerPlanetMin(), static_cast<unsigned int>(0u), this->attributes)),
        moonsPerPlanetMax(new Variant(UniversumComponent::AttrMoonsPerPlanetMax(), static_cast<unsigned int>(3u), this->attributes)),
        useMotion(new Variant(UniversumComponent::AttrUseMotion(), true, this->attributes)),
        useOcean(new Variant(UniversumComponent::AttrUseOcean(), true, this->attributes)),
        oceanProbability(new Variant(UniversumComponent::AttrOceanProbability(), 0.4f, this->attributes)),
        sunRadius(new Variant(UniversumComponent::AttrSunRadius(), 25.0f, this->attributes)),
        planetRadiusMin(new Variant(UniversumComponent::AttrPlanetRadiusMin(), 8.0f, this->attributes)),
        planetRadiusMax(new Variant(UniversumComponent::AttrPlanetRadiusMax(), 30.0f, this->attributes)),
        moonRadius(new Variant(UniversumComponent::AttrMoonRadius(), 5.0f, this->attributes)),
        orbitalDistanceMin(new Variant(UniversumComponent::AttrOrbitalDistanceMin(), 160.0f, this->attributes)),
        orbitalDistanceMax(new Variant(UniversumComponent::AttrOrbitalDistanceMax(), 600.0f, this->attributes)),
        moonOrbitalDistanceMin(new Variant(UniversumComponent::AttrMoonOrbitalDistanceMin(), 80.0f, this->attributes)),
        moonOrbitalDistanceMax(new Variant(UniversumComponent::AttrMoonOrbitalDistanceMax(), 200.0f, this->attributes)),
        orbitalSpeedMin(new Variant(UniversumComponent::AttrOrbitalSpeedMin(), 0.02f, this->attributes)),
        orbitalSpeedMax(new Variant(UniversumComponent::AttrOrbitalSpeedMax(), 0.15f, this->attributes)),
        axialSpeedMin(new Variant(UniversumComponent::AttrAxialSpeedMin(), 0.01f, this->attributes)),
        axialSpeedMax(new Variant(UniversumComponent::AttrAxialSpeedMax(), 0.05f, this->attributes)),
        playerGameObjectId(new Variant(UniversumComponent::AttrPlayerGameObjectId(), static_cast<unsigned long>(0ul), this->attributes)),
        cameraGameObjectId(new Variant(UniversumComponent::AttrCameraGameObjectId(), static_cast<unsigned long>(GameObjectController::MAIN_CAMERA_ID), this->attributes)),
        sunLightGameObjectId(new Variant(UniversumComponent::AttrSunLightGameObjectId(), static_cast<unsigned long>(GameObjectController::MAIN_LIGHT_ID), this->attributes)),
        farClipSpace(new Variant(UniversumComponent::AttrFarClipSpace(), 1000000.0f, this->attributes)),
        farClipSurface(new Variant(UniversumComponent::AttrFarClipSurface(), 5000.0f, this->attributes)),
        farClipTransitionSpeed(new Variant(UniversumComponent::AttrFarClipTransitionSpeed(), 0.5f, this->attributes)),
        scale(new Variant(UniversumComponent::AttrScale(), 1.0f, this->attributes)),
        autoPauseOrbit(new Variant(UniversumComponent::AttrAutoPauseOrbit(), true, this->attributes)),
        generate(new Variant(UniversumComponent::AttrGenerate(), Ogre::String("Generate"), this->attributes)),
        elapsedTime(0.0f),
        currentFarClip(1000000.0f),
        postInitDone(false),
        cachedSunLight(nullptr),
        currentLightColor(1.0f, 0.95f, 0.80f),
        currentLightPower(3.14159f),
        playerOnSurface(false),
        firstLightFrame(true),
        loadedFromFile(false),
        cachedAtmosphereComponent(nullptr),
        landingState(LandingState::NONE),
        landingBodyCentre(Ogre::Vector3::ZERO),
        landingBodyRadius(0.0f),
        landedOnBodyId(0ul),
        landingAltitudeThreshold(60.0f),
        takeoffClearanceAltitude(90.0f),
        takeoffSpeed(15.0f),
        landingInputLocked(false),
        landingFired(false),
        landingDebugTimer(0.0f),
        fakeLightElapsed(0.0f),
        fakeLightAxialSpeed(0.0f),
        shadowsConfiguredForSurface(false),
        landingContactActive(false),
        landingContactTimer(0.0f),
        currentAmbientUpper(0.03f, 0.03f, 0.04f),
        currentAmbientLower(0.03f, 0.03f, 0.04f),
        landingRayQuery(nullptr),
        resolvedLandingTargetValid(false)
    {
        this->generate->setDescription("Runs the full generation pipeline and applies the result to the "
                                       "sibling PlanetTerraComponent. The planet must already exist.");
        this->generate->addUserData(GameObject::AttrActionExec());
        this->generate->addUserData(GameObject::AttrActionNeedRefresh());
        this->generate->addUserData(GameObject::AttrActionExecId(), UniversumComponent::ActionGenerate());

        this->randomSeed->setDescription("Seed for reproducible universe generation. Change to explore different configurations.");
        this->solarSystemCount->setDescription("Number of solar systems to generate.");
        this->solarSystemDistanceMin->setDescription("Minimum world-unit distance between solar system centres.");
        this->solarSystemDistanceMax->setDescription("Maximum world-unit distance between solar system centres.");
        this->planetsPerSystemMin->setDescription("Minimum number of planets per solar system.");
        this->planetsPerSystemMax->setDescription("Maximum number of planets per solar system.");
        this->useMoons->setDescription("Whether planets can have moons.");
        this->moonsPerPlanetMin->setDescription("Minimum moons per planet (when Use Moons is checked).");
        this->moonsPerPlanetMax->setDescription("Maximum moons per planet.");
        this->useMotion->setDescription("Enable orbital motion. Planets and moons orbit their parents. "
                                        "PhysicsArtifactComponent is used for collision; setDynamic(true/false) "
                                        "switches between moving and static modes.");
        this->useOcean->setDescription("Allow planets to receive a PlanetOceanComponent based on Ocean Probability.");
        this->oceanProbability->setDescription("Probability [0,1] that a planet gets an ocean.");
        this->sunRadius->setDescription("Radius of the sun sphere in world units.");
        this->planetRadiusMin->setDescription("Minimum planet radius in world units.");
        this->planetRadiusMax->setDescription("Maximum planet radius in world units.");
        this->moonRadius->setDescription("Fixed moon radius in world units.");
        this->orbitalDistanceMin->setDescription("Minimum orbital radius of planets from their sun.");
        this->orbitalDistanceMax->setDescription("Maximum orbital radius of planets from their sun.");
        this->moonOrbitalDistanceMin->setDescription("Minimum orbital radius of moons from their planet.");
        this->moonOrbitalDistanceMax->setDescription("Maximum orbital radius of moons from their planet.");
        this->orbitalSpeedMin->setDescription("Minimum orbital angular speed in radians per second.");
        this->orbitalSpeedMax->setDescription("Maximum orbital angular speed in radians per second.");
        this->axialSpeedMin->setDescription("Minimum axial (self-rotation) angular speed in radians per second.");
        this->axialSpeedMax->setDescription("Maximum axial angular speed in radians per second.");
        this->playerGameObjectId->setDescription("Optional: Id of the player GameObject. Used for camera far-clip adaptation.");
        this->cameraGameObjectId->setDescription("Optional: Id of the camera GameObject whose far-clip distance is adapted.");
        this->sunLightGameObjectId->setDescription("Id of the scene directional light GameObject to steer as the sun light. "
                                                   "Defaults to MAIN_LIGHT_ID (the mandatory SunLight every scene has). "
                                                   "Only the nearest solar system to the camera drives this light at any time.");
        this->farClipSpace->setDescription("Camera far clip distance when the player is in open space.");
        this->farClipSurface->setDescription("Camera far clip distance when the player is on or near a planet surface.");
        this->farClipTransitionSpeed->setDescription("Lerp speed for far clip transitions between space and surface values.");
        this->scale->setDescription("Universe size scale. 1 = smallest units are ~10m. Each step multiplies all radii and distances by 10. "
                                    "scale=1: sun~25m, planets 8-30m. scale=2: sun~250m, planets 80-300m.");
        this->autoPauseOrbit->setDescription("When enabled, orbital motion of a planet or moon is automatically paused when "
                                             "any game object enters its AreaOfInterest zone, and resumed when it leaves. "
                                             "This freezes the surface geometry so buildings and props with TreeCollision "
                                             "remain physically valid while the player is on the surface. "
                                             "Day/night is faked via directional light rotation during the pause.");
    }

    UniversumComponent::~UniversumComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin
    // =========================================================================

    void UniversumComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<UniversumComponent>(UniversumComponent::getStaticClassId(), UniversumComponent::getStaticClassName());
    }

    void UniversumComponent::initialise()
    {
    }

    void UniversumComponent::shutdown()
    {
    }

    void UniversumComponent::uninstall()
    {
    }

    void UniversumComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& UniversumComponent::getName() const
    {
        return this->name;
    }

    // =========================================================================
    //  GameObjectComponent lifecycle
    // =========================================================================

    bool UniversumComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = GameObjectComponent::init(propertyElement);

        // Prevents duplicates when readXML fires multiple times in one reload cycle.
        this->solarSystems.clear();
        this->ownedGameObjectIds.clear();

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRandomSeed())
        {
            this->randomSeed->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSolarSystemCount())
        {
            this->solarSystemCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSolarSystemDistanceMin())
        {
            this->solarSystemDistanceMin->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSolarSystemDistanceMax())
        {
            this->solarSystemDistanceMax->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPlanetsPerSystemMin())
        {
            this->planetsPerSystemMin->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPlanetsPerSystemMax())
        {
            this->planetsPerSystemMax->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrUseMoons())
        {
            this->useMoons->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMoonsPerPlanetMin())
        {
            this->moonsPerPlanetMin->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMoonsPerPlanetMax())
        {
            this->moonsPerPlanetMax->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrUseMotion())
        {
            this->useMotion->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrUseOcean())
        {
            this->useOcean->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOceanProbability())
        {
            this->oceanProbability->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSunRadius())
        {
            this->sunRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPlanetRadiusMin())
        {
            this->planetRadiusMin->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPlanetRadiusMax())
        {
            this->planetRadiusMax->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMoonRadius())
        {
            this->moonRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOrbitalDistanceMin())
        {
            this->orbitalDistanceMin->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOrbitalDistanceMax())
        {
            this->orbitalDistanceMax->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMoonOrbitalDistanceMin())
        {
            this->moonOrbitalDistanceMin->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMoonOrbitalDistanceMax())
        {
            this->moonOrbitalDistanceMax->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOrbitalSpeedMin())
        {
            this->orbitalSpeedMin->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOrbitalSpeedMax())
        {
            this->orbitalSpeedMax->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrAxialSpeedMin())
        {
            this->axialSpeedMin->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrAxialSpeedMax())
        {
            this->axialSpeedMax->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPlayerGameObjectId())
        {
            this->playerGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCameraGameObjectId())
        {
            this->cameraGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSunLightGameObjectId())
        {
            this->sunLightGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFarClipSpace())
        {
            this->farClipSpace->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFarClipSurface())
        {
            this->farClipSurface->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFarClipTransitionSpeed())
        {
            this->farClipTransitionSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrScale())
        {
            this->scale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrAutoPauseOrbit())
        {
            this->autoPauseOrbit->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // ---- Restore solar systems from saved scene -------------------------
        // If saved system count > 0, rebuild solarSystems so orbital motion and
        // observer attachment work on reload without requiring Generate.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Saved System Count")
        {
            const unsigned int savedSystemCount = XMLConverter::getAttribUnsignedInt(propertyElement, "data");
            propertyElement = propertyElement->next_sibling("property");

            for (unsigned int si = 0u; si < savedSystemCount; ++si)
            {
                const Ogre::String sp = "Sys" + Ogre::StringConverter::toString(si) + "_";
                SolarSystem system;
                system.sunLightColor = Ogre::Vector3(1.0f, 0.95f, 0.80f);
                system.sunLightPower = 3.14159f;

                auto readUL = [&](const Ogre::String& key, unsigned long& out)
                {
                    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                    {
                        out = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
                        propertyElement = propertyElement->next_sibling("property");
                    }
                };
                auto readUInt = [&](const Ogre::String& key, unsigned int& out)
                {
                    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                    {
                        out = XMLConverter::getAttribUnsignedInt(propertyElement, "data");
                        propertyElement = propertyElement->next_sibling("property");
                    }
                };
                auto readReal = [&](const Ogre::String& key, float& out)
                {
                    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                    {
                        out = XMLConverter::getAttribReal(propertyElement, "data");
                        propertyElement = propertyElement->next_sibling("property");
                    }
                };

                readUL(sp + "SunId", system.sunId);
                // Sun must be in ownedGameObjectIds so destroyUniverse() can delete it
                // on the next Generate click. Without this, the old sun is never
                // cleaned up and a second sun is created at the same position.
                if (0ul != system.sunId)
                {
                    this->ownedGameObjectIds.push_back(system.sunId);
                }
                // Restore per-system star color and intensity saved at generation time.
                Ogre::Vector3 lightColor(1.0f, 0.95f, 0.80f);
                float lightPower = 3.14159f;
                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == sp + "LightColor")
                {
                    lightColor = XMLConverter::getAttribVector3(propertyElement, "data");
                    propertyElement = propertyElement->next_sibling("property");
                }
                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == sp + "LightPower")
                {
                    lightPower = XMLConverter::getAttribReal(propertyElement, "data");
                    propertyElement = propertyElement->next_sibling("property");
                }
                system.sunLightColor = lightColor;
                system.sunLightPower = lightPower;

                unsigned int planetCount = 0u;
                readUInt(sp + "PlanetCount", planetCount);

                for (unsigned int pi = 0u; pi < planetCount; ++pi)
                {
                    const Ogre::String pp = sp + "P" + Ogre::StringConverter::toString(pi) + "_";
                    OrbitalBody planet;
                    planet.orbitalElapsed = 0.0f;
                    planet.axialElapsed = 0.0f;
                    planet.orbitalPaused = false;
                    planet.currentPosition = Ogre::Vector3::ZERO;

                    readUL(pp + "Id", planet.gameObjectId);
                    readReal(pp + "OrbR", planet.orbitalRadius);
                    readReal(pp + "OrbS", planet.orbitalSpeed);
                    readReal(pp + "OrbT", planet.orbitalTilt);
                    readReal(pp + "Phase", planet.phaseOffset);
                    readReal(pp + "AxS", planet.axialSpeed);
                    readReal(pp + "Grav", planet.gravityStrength);

                    // Recompute spawn position from phase offset (t=0 state).
                    GameObjectPtr sunGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(system.sunId);
                    if (nullptr != sunGo)
                    {
                        Ogre::Vector3 sunPos = sunGo->getPosition();
                        const float angle = planet.phaseOffset;
                        planet.currentPosition = Ogre::Vector3(sunPos.x + std::cos(angle) * planet.orbitalRadius, sunPos.y + std::sin(angle * planet.orbitalTilt) * planet.orbitalRadius * 0.1f, sunPos.z + std::sin(angle) * planet.orbitalRadius);
                    }

                    // Restore this body to the ownedGameObjectIds list.
                    this->ownedGameObjectIds.push_back(planet.gameObjectId);
                    system.planets.push_back(planet);
                }

                unsigned int moonCount = 0u;
                readUInt(sp + "MoonCount", moonCount);

                for (unsigned int mi = 0u; mi < moonCount; ++mi)
                {
                    const Ogre::String mp = sp + "M" + Ogre::StringConverter::toString(mi) + "_";
                    unsigned int parentIdx = 0u;
                    readUInt(mp + "Parent", parentIdx);

                    OrbitalBody moon;
                    moon.orbitalElapsed = 0.0f;
                    moon.axialElapsed = 0.0f;
                    moon.orbitalPaused = false;
                    moon.currentPosition = Ogre::Vector3::ZERO;

                    readUL(mp + "Id", moon.gameObjectId);
                    readReal(mp + "OrbR", moon.orbitalRadius);
                    readReal(mp + "OrbS", moon.orbitalSpeed);
                    readReal(mp + "OrbT", moon.orbitalTilt);
                    readReal(mp + "Phase", moon.phaseOffset);
                    readReal(mp + "AxS", moon.axialSpeed);
                    readReal(mp + "Grav", moon.gravityStrength);

                    if (parentIdx < system.planets.size())
                    {
                        const Ogre::Vector3& parentPos = system.planets[parentIdx].currentPosition;
                        const float angle = moon.phaseOffset;
                        moon.currentPosition = Ogre::Vector3(parentPos.x + std::cos(angle) * moon.orbitalRadius, parentPos.y + std::sin(angle * moon.orbitalTilt) * moon.orbitalRadius * 0.1f, parentPos.z + std::sin(angle) * moon.orbitalRadius);
                    }

                    this->ownedGameObjectIds.push_back(moon.gameObjectId);
                    system.moons.push_back(std::make_pair(static_cast<size_t>(parentIdx), moon));
                }

                this->solarSystems.push_back(system);
            }

            // Skip "Owned GO Ids" (already rebuilt above).
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Owned GO Ids")
            {
                propertyElement = propertyElement->next_sibling("property");
            }

            if (false == this->solarSystems.empty())
            {
                this->loadedFromFile = true;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Restored " + Ogre::StringConverter::toString(savedSystemCount) + " solar system(s) from scene XML.");
            }
        }

        return success;
    }

    bool UniversumComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Init component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &UniversumComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        this->currentFarClip = this->farClipSpace->getReal();
        this->postInitDone = true;

        this->landingRayQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), NOWA::GameObjectController::ALL_CATEGORIES_ID);
        this->landingRayQuery->setSortByDistance(true);

        // If the universe was loaded from a saved scene, planet GOs already exist in the
        // scene. Re-attach PlanetOrbitObserver to each planet's AreaOfInterestComponent
        // so auto-pause works without the user clicking Generate again.
        return true;
    }

    bool UniversumComponent::connect(void)
    {
        GameObjectComponent::connect();
        this->elapsedTime = 0.0f;

        // Cache the scene directional light raw pointer.
        // Default is MAIN_LIGHT_ID -- the mandatory SunLight every scene has.
        // The user can point sunLightGameObjectId at a different light if needed.
        this->cachedSunLight = nullptr;
        GameObjectPtr lightGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sunLightGameObjectId->getULong());
        if (nullptr != lightGo)
        {
            auto lightComp = NOWA::makeStrongPtr(lightGo->getComponent<LightDirectionalComponent>());
            if (nullptr != lightComp)
            {
                this->cachedSunLight = lightComp->getOgreLight();
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Cached scene directional light from '" + lightGo->getName() + "'.");
            }
        }

        if (nullptr == this->cachedSunLight)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[UniversumComponent] No directional light found for sunLightGameObjectId=" + Ogre::StringConverter::toString(this->sunLightGameObjectId->getULong()) + " -- sun lighting will not be updated.");
        }

        // Start in space mode -- disable PSSM shadows until player lands on a planet.
        // With farClipSpace=1,000,000 the PSSM splits are useless (split 0 covers ~8km).
        // Shadows are re-enabled with tight split distances when pausePlanetOrbit fires.
        if (nullptr != this->cachedSunLight && false == this->playerOnSurface)
        {
            Ogre::Light* light = this->cachedSunLight;
            NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
            {
                light->setCastShadows(false);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(shadowCmd), "UniversumComponent::connect::disableShadows");
        }

        // Ensure all planet/moon GOs and their sub-component items (PlanetTerra, PlanetOcean,
        // PlanetSun) are consistently SCENE_STATIC at simulation start. On scene load,
        // sub-component items may default to SCENE_DYNAMIC while the SceneNode is already
        // SCENE_STATIC (saved state), causing "movableobject is static while node is not" crash.
        if (false == this->solarSystems.empty())
        {
            this->registerAndHideSurfaceObjects();
        }

        // If the universe was loaded from a saved scene, planet GOs already exist in the
        // scene. Re-attach PlanetOrbitObserver to each planet's AreaOfInterestComponent

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[UniversumComponent] Restored from scene -- re-attaching observers for " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->solarSystems.size())) + " solar system(s).");

        // Planets/moons were always SCENE_STATIC -- nothing to reset.
        // They were set to SCENE_DYNAMIC in connect() for orbital motion.
        for (SolarSystem& system : this->solarSystems)
        {
            // No cast shadows for sun, else its projected on nearby moons, planets and looks ugly
            this->setBodyCastShadows(system.sunId, false, "UniversumComponent::connect::sunNoCastShadows");

            for (OrbitalBody& planet : system.planets)
            {
                GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                if (nullptr != planetGo)
                {
                    if (false == planet.orbitalPaused)
                    {
                        this->setPlanetDynamic(planetGo, true);
                    }

                    auto aoiComp = NOWA::makeStrongPtr(planetGo->getComponent<AreaOfInterestComponent>());
                    if (nullptr != aoiComp)
                    {
                        PlanetOrbitObserver* obs = new PlanetOrbitObserver(this, planet.gameObjectId);
                        this->planetObservers.push_back(obs);
                        aoiComp->attachTriggerObserver(obs);
                    }
                }
            }
            for (auto& moonPair : system.moons)
            {
                OrbitalBody& moon = moonPair.second;
                GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moon.gameObjectId);
                if (nullptr != moonGo)
                {
                    if (false == moonPair.second.orbitalPaused)
                    {
                        this->setPlanetDynamic(moonGo, true);
                    }

                    auto aoiComp = NOWA::makeStrongPtr(moonGo->getComponent<AreaOfInterestComponent>());
                    if (nullptr != aoiComp)
                    {
                        PlanetOrbitObserver* obs = new PlanetOrbitObserver(this, moon.gameObjectId);
                        this->planetObservers.push_back(obs);
                        aoiComp->attachTriggerObserver(obs);
                    }
                }
            }
        }

        // Fire camera speed event so DesignState sets the editor camera speed correctly,
        // matching what generateUniverse() would have posted.
        // Formula mirrors generateUniverse: max orbital dist * 0.1 / 10s.
        const float scaleFactor = std::pow(10.0f, this->scale->getReal());
        const float approxMaxDist = this->orbitalDistanceMax->getReal() * scaleFactor;
        const float suggestedSpeed = std::max(10.0f, approxMaxDist * 0.1f);
        boost::shared_ptr<EventDataFeedback> speedEvent(boost::make_shared<EventDataFeedback>(true, "[UNIVERSUM_CAMSPEED:" + Ogre::StringConverter::toString(suggestedSpeed) + "]"));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(speedEvent);

        return true;
    }

    bool UniversumComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        this->reset();

        return true;
    }

    void UniversumComponent::reset(void)
    {
        this->cachedAtmosphereComponent = nullptr;

        // During destruction just unregister without touching physics or visibility.
        // restoreAllSurfaceObjects calls setActivated(true) which hangs if Newton is gone.
        if (AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            this->unregisterSurfaceObjects();
        }
        else
        {
            this->restoreAllSurfaceObjects();
        }

        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                if (nullptr != planetGo)
                {
                    auto aoiComp = NOWA::makeStrongPtr(planetGo->getComponent<AreaOfInterestComponent>());
                    if (nullptr != aoiComp)
                    {
                        aoiComp->attachTriggerObserver(nullptr);
                    }
                }
            }
            for (auto& moonPair : system.moons)
            {
                GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moonPair.second.gameObjectId);
                if (nullptr != moonGo)
                {
                    auto aoiComp = NOWA::makeStrongPtr(moonGo->getComponent<AreaOfInterestComponent>());
                    if (nullptr != aoiComp)
                    {
                        aoiComp->attachTriggerObserver(nullptr);
                    }
                }
            }
        }

        // Delete any observers from a previous connect() cycle before creating new ones.
        // disconnect() nullifies the AOI pointer but does not delete the objects.
        // Without this cleanup planetObservers grows by N on every play/stop cycle.
        for (PlanetOrbitObserver* obs : this->planetObservers)
        {
            delete obs;
        }
        this->planetObservers.clear();

        // Restore all surface objects to original world positions and make visible.
        this->restoreAllSurfaceObjects();

        {
            const unsigned long shipGOId = this->playerGameObjectId->getULong();
            if (0ul != shipGOId)
            {
                GameObjectPtr shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(shipGOId);
                if (nullptr != shipGo)
                {
                    auto actComp = NOWA::makeStrongPtr(shipGo->getComponent<PhysicsActiveComponent>());
                    if (nullptr != actComp)
                    {
                        actComp->removeCppContactCallback();
                    }
                }
            }
            this->landingContactActive = false;
            this->landingContactTimer = 0.0f;
        }

        // Restore shadow casting -- editor and next simulation start from a clean state.
        if (nullptr != this->cachedSunLight)
        {
            if (false == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
            {
                Ogre::Light* light = this->cachedSunLight;

                NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
                {
                    light->setCastShadows(true);
                };
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(shadowCmd), "UniversumComponent::disconnect::restoreShadows");
            }
        }

        this->playerOnSurface = false;
        this->firstLightFrame = true;
        if (true == this->landingInputLocked)
        {
            this->setPlayerInputLock(false);
            this->landingInputLocked = false;
        }
        this->landingState = LandingState::NONE;
        this->landedOnBodyId = 0ul;
        this->landingFired = false;
        this->landingDebugTimer = 0.0f;
        this->fakeLightAxialSpeed = 0.0f;
        this->fakeLightElapsed = 0.0f;
        this->shadowsConfiguredForSurface = false;
        this->landingContactActive = false;
        this->landingContactTimer = 0.0f;
        this->currentAmbientUpper = Ogre::Vector3(0.03f, 0.03f, 0.04f);
        this->currentAmbientLower = Ogre::Vector3(0.03f, 0.03f, 0.04f);
        this->elapsedTime = 0.0f;
        this->resolvedLandingTargetValid = false;

        // When simulation stops, the undo system resets all planet/moon SceneNodes back
        // to their spawn positions (t=0 state). We must reset the per-body timers to match,
        // otherwise on the next play the first update() computes a huge position delta
        // and the bodies jump.
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                planet.orbitalElapsed = 0.0f;
                planet.axialElapsed = 0.0f;
                planet.orbitalPaused = false;

                // Recompute currentPosition at t=0 so the first update() delta is near zero.
                // This must match what generateSolarSystem computed at spawn.
                const float angle = planet.phaseOffset;
                planet.currentPosition =
                    Ogre::Vector3(system.position.x + std::cos(angle) * planet.orbitalRadius, system.position.y + std::sin(angle * planet.orbitalTilt) * planet.orbitalRadius * 0.1f, system.position.z + std::sin(angle) * planet.orbitalRadius);
            }

            for (auto& moonPair : system.moons)
            {
                OrbitalBody& moon = moonPair.second;
                moon.orbitalElapsed = 0.0f;
                moon.axialElapsed = 0.0f;
                moon.orbitalPaused = false;

                // Moon spawn position is relative to its parent planet's spawn position.
                const size_t parentIdx = moonPair.first;
                if (parentIdx < system.planets.size())
                {
                    const Ogre::Vector3& parentSpawnPos = system.planets[parentIdx].currentPosition;
                    const float angle = moon.phaseOffset;
                    moon.currentPosition =
                        Ogre::Vector3(parentSpawnPos.x + std::cos(angle) * moon.orbitalRadius, parentSpawnPos.y + std::sin(angle * moon.orbitalTilt) * moon.orbitalRadius * 0.1f, parentSpawnPos.z + std::sin(angle) * moon.orbitalRadius);
                }
            }
        }
    }

    void UniversumComponent::onRemoveComponent(void)
    {
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &UniversumComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        this->destroyUniverse();
        GameObjectComponent::onRemoveComponent();
    }

    void UniversumComponent::applyShipMovement(GameObjectPtr shipGo, const Ogre::Vector3& velocity, const Ogre::Quaternion& targetOrient, float orientStrength)
    {
        // Kinematic bodies use direct velocity setters -- no force accumulation.
        auto kinComp = NOWA::makeStrongPtr(shipGo->getComponent<PhysicsActiveKinematicComponent>());
        if (nullptr != kinComp)
        {
            kinComp->setVelocity(velocity);
            if (orientStrength > 0.0f)
            {
                kinComp->setOmegaVelocityRotateTo(targetOrient, Ogre::Vector3::UNIT_SCALE, orientStrength);
            }
            return;
        }
        // Dynamic bodies use force-based setters.
        auto actComp = NOWA::makeStrongPtr(shipGo->getComponent<PhysicsActiveComponent>());
        if (nullptr != actComp)
        {
            actComp->applyRequiredForceForVelocity(velocity);
            if (orientStrength > 0.0f)
            {
                actComp->applyOmegaForceRotateTo(targetOrient, Ogre::Vector3::UNIT_SCALE, orientStrength);
            }
            return;
        }
        // No physics component: approximate by direct node manipulation.
        // This branch is a safety fallback and won't look smooth.
        if (velocity.squaredLength() > 0.0f)
        {
            shipGo->setAttributePosition(shipGo->getPosition() + velocity * 0.016f);
        }
        if (orientStrength > 0.0f)
        {
            Ogre::Quaternion cur = shipGo->getOrientation();
            Ogre::Quaternion blended = Ogre::Quaternion::Slerp(std::min(orientStrength * 0.016f, 1.0f), cur, targetOrient, true);
            shipGo->setAttributeOrientation(blended);
        }
    }

    void UniversumComponent::setPlayerInputLock(bool locked)
    {
        const unsigned long playerGOId = this->playerGameObjectId->getULong();
        if (0ul == playerGOId)
        {
            return;
        }
        GameObjectPtr shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(playerGOId);
        if (nullptr == shipGo)
        {
            return;
        }
        auto inputComp = NOWA::makeStrongPtr(shipGo->getComponent<InputDeviceComponent>());
        if (nullptr != inputComp)
        {
            inputComp->lockDevice(locked);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Input device " + Ogre::String(locked ? "locked" : "unlocked") + " for GO id=" + Ogre::StringConverter::toString(playerGOId));
        }
    }

    // =========================================================================
    // updateLandingStateMachine -- called each frame from update()
    //
    // APPROACHING -> LANDING  when distToSurface <= landingAltitudeThreshold
    // LANDING     -> LANDED   when ship is close and aligned to surface normal
    // LANDED      -> (holds)  waiting for requestTakeoff()
    // TAKING_OFF  -> APPROACHING when distToSurface > takeoffClearanceAltitude
    // =========================================================================
    void UniversumComponent::updateLandingStateMachine(Ogre::Real dt)
    {
        if (this->landingState == LandingState::NONE)
        {
            return;
        }
        const unsigned long playerGOId = this->playerGameObjectId->getULong();
        if (0ul == playerGOId)
        {
            return;
        }
        GameObjectPtr shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(playerGOId);
        if (nullptr == shipGo)
        {
            return;
        }

        // Keep landingBodyCentre in sync with the paused body GO
        if (0ul != this->landedOnBodyId)
        {
            GameObjectPtr bodyGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->landedOnBodyId);
            if (nullptr != bodyGo)
            {
                this->landingBodyCentre = bodyGo->getPosition();
            }
        }

        Ogre::Vector3 shipPos = shipGo->getPosition();
        Ogre::Vector3 toCenter = this->landingBodyCentre - shipPos;
        float distFromCenter = toCenter.length();
        Ogre::Vector3 surfaceNormal = (distFromCenter > 1e-4f) ? (-toCenter / distFromCenter) : Ogre::Vector3::UNIT_Y;
        float distToSurface = distFromCenter - this->landingBodyRadius;
        Ogre::Quaternion currentOrient = shipGo->getOrientation();
        Ogre::Quaternion targetOrient = MathHelper::getInstance()->computeLandingOrientation(currentOrient, surfaceNormal, shipGo->getDefaultDirection());

        if (this->landingState == LandingState::APPROACHING)
        {
            if (false == this->landingFired && distToSurface <= this->landingAltitudeThreshold)
            {
                this->landingFired = true;
                this->callLandingFunction(this->landedOnBodyId, playerGOId);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] reactOnLanding fired, distToSurface=" + Ogre::StringConverter::toString(distToSurface));
            }
            if (true == this->landingFired && distToSurface > this->landingAltitudeThreshold * 2.0f)
            {
                this->landingFired = false;
            }
            return;
        }

        if (this->landingState == LandingState::LANDING)
        {
            const float settleHeight = 1.0f;

            // Resolve flat landing target once at the start of the descent
            if (false == this->resolvedLandingTargetValid)
            {
                Ogre::Vector3 flatSpot;
                if (this->findFlatLandingSpot(shipPos, surfaceNormal, this->landingBodyCentre, this->landingBodyRadius, shipGo, flatSpot))
                {
                    Ogre::Vector3 flatNormal = (flatSpot - this->landingBodyCentre).normalisedCopy();
                    float flatSpotDistFromCentre = (flatSpot - this->landingBodyCentre).length();
                    this->resolvedLandingTarget = flatSpot + flatNormal * settleHeight;
                    this->resolvedLandingTargetValid = true;

                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[UniversumComponent] LANDING resolvedTarget=" + Ogre::StringConverter::toString(this->resolvedLandingTarget) + " flatSpot=" + Ogre::StringConverter::toString(flatSpot) + " flatSpotDistFromCentre=" +
                            Ogre::StringConverter::toString(flatSpotDistFromCentre) + " bodyRadius=" + Ogre::StringConverter::toString(this->landingBodyRadius) + " bodyCentre=" + Ogre::StringConverter::toString(this->landingBodyCentre) +
                            " shipPos=" + Ogre::StringConverter::toString(shipPos) + " distFromCenter=" + Ogre::StringConverter::toString(distFromCenter) + " distToSurface=" + Ogre::StringConverter::toString(distToSurface));
                }
                else
                {
                    this->resolvedLandingTarget = this->landingBodyCentre + surfaceNormal * (this->landingBodyRadius + settleHeight);
                    this->resolvedLandingTargetValid = true;
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] LANDING fallback resolvedTarget=" + Ogre::StringConverter::toString(this->resolvedLandingTarget) +
                                                                                           " bodyCentre=" + Ogre::StringConverter::toString(this->landingBodyCentre) + " bodyRadius=" + Ogre::StringConverter::toString(this->landingBodyRadius) +
                                                                                           " surfaceNormal=" + Ogre::StringConverter::toString(surfaceNormal) + " shipPos=" + Ogre::StringConverter::toString(shipPos) +
                                                                                           " distFromCenter=" + Ogre::StringConverter::toString(distFromCenter) + " distToSurface=" + Ogre::StringConverter::toString(distToSurface));
                }
            }

            Ogre::Vector3 toTarget = this->resolvedLandingTarget - shipPos;
            float distToTarget = toTarget.length();

            Ogre::Vector3 targetNormal = (this->resolvedLandingTarget - this->landingBodyCentre).normalisedCopy();
            Ogre::Quaternion targetOrientAtSpot = MathHelper::getInstance()->computeLandingOrientation(currentOrient, targetNormal, shipGo->getDefaultDirection());

            // Check landing completion FIRST — before applying any velocity
            const bool contactLanded = this->landingContactActive;
            const bool distTargetLanded = (distToTarget <= settleHeight * 2.0f);

            this->landingDebugTimer += dt;
            if (this->landingDebugTimer >= 1.0f)
            {
                this->landingDebugTimer = 0.0f;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[UniversumComponent] LANDING tick:"
                    " shipPos=" +
                        Ogre::StringConverter::toString(shipPos) + " resolvedTarget=" + Ogre::StringConverter::toString(this->resolvedLandingTarget) + " distToTarget=" + Ogre::StringConverter::toString(distToTarget) +
                        " distToSurface=" + Ogre::StringConverter::toString(distToSurface) + " distFromCenter=" + Ogre::StringConverter::toString(distFromCenter) + " bodyRadius=" + Ogre::StringConverter::toString(this->landingBodyRadius) +
                        " bodyCentre=" + Ogre::StringConverter::toString(this->landingBodyCentre) + " contactLanded=" + Ogre::StringConverter::toString(contactLanded) + " distTargetLanded=" + Ogre::StringConverter::toString(distTargetLanded));
            }

            if (contactLanded || distTargetLanded)
            {
                this->applyShipMovement(shipGo, Ogre::Vector3::ZERO, targetOrientAtSpot, 0.0f);
                this->landingState = LandingState::LANDED;
                this->landingDebugTimer = 0.0f;
                this->landingContactActive = false;
                this->resolvedLandingTargetValid = false;
                this->setPlayerInputLock(false);
                this->landingInputLocked = false;
                this->callLandedFunction(this->landedOnBodyId, playerGOId);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] LANDED on body id=" + Ogre::StringConverter::toString(this->landedOnBodyId) + " shipPos=" + Ogre::StringConverter::toString(shipPos) +
                                                                                       " resolvedTarget=" + Ogre::StringConverter::toString(this->resolvedLandingTarget) + " distToTarget=" + Ogre::StringConverter::toString(distToTarget) +
                                                                                       " distToSurface=" + Ogre::StringConverter::toString(distToSurface) + " distFromCenter=" + Ogre::StringConverter::toString(distFromCenter) +
                                                                                       " bodyRadius=" + Ogre::StringConverter::toString(this->landingBodyRadius) + " contact=" + Ogre::StringConverter::toString(contactLanded) +
                                                                                       " distTarget=" + Ogre::StringConverter::toString(distTargetLanded));
                return;
            }

            // Apply descent velocity only when not yet at target
            Ogre::Vector3 descentVelocity = Ogre::Vector3::ZERO;
            if (distToTarget > 0.05f && distToTarget > settleHeight * 2.0f)
            {
                const float maxDescentSpeed = 30.0f;
                const float minDescentSpeed = 5.0f;
                float speed = std::max(minDescentSpeed, std::min(distToTarget * 0.8f, maxDescentSpeed));
                descentVelocity = toTarget.normalisedCopy() * speed;
            }
            else
            {
                descentVelocity = -targetNormal * 2.0f;
            }

            this->applyShipMovement(shipGo, descentVelocity, targetOrientAtSpot, 5.0f);
            this->landingContactActive = false;
            return;
        }

        if (this->landingState == LandingState::LANDED)
        {
            this->applyShipMovement(shipGo, Ogre::Vector3::ZERO, targetOrient, 0.0f);
            return;
        }

        if (this->landingState == LandingState::TAKING_OFF)
        {
            this->applyShipMovement(shipGo, surfaceNormal * this->takeoffSpeed, targetOrient, 1.0f);
            if (distToSurface > this->takeoffClearanceAltitude)
            {
                this->landingState = LandingState::APPROACHING;
                this->landingFired = false;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Takeoff clearance reached, state=APPROACHING");
            }
        }
    }

    void UniversumComponent::requestTakeoff(void)
    {
        if (this->landingState != LandingState::LANDED)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] requestTakeoff ignored -- not in LANDED state (current=" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->landingState)) + ")");
            return;
        }
        this->landingState = LandingState::TAKING_OFF;
        this->setPlayerInputLock(false);
        this->landingInputLocked = false;

        {
            const unsigned long shipGOId = this->playerGameObjectId->getULong();
            if (0ul != shipGOId)
            {
                GameObjectPtr shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(shipGOId);
                if (nullptr != shipGo)
                {
                    auto actComp = NOWA::makeStrongPtr(shipGo->getComponent<PhysicsActiveComponent>());
                    if (nullptr != actComp)
                    {
                        actComp->removeCppContactCallback();

                        // Snap the ship upright on takeoff: preserve only yaw,
                        // zero out roll and pitch that may have accumulated during landing.
#if 0
                        Ogre::Quaternion currentOrient = actComp->getOrientation();

                        // Extract forward from current orientation and flatten onto world XZ plane
                        Ogre::Vector3 defaultDir = shipGo->getDefaultDirection();
                        Ogre::Vector3 forward = currentOrient * defaultDir;
                        forward.y = 0.0f;

                        if (forward.squaredLength() < 1e-4f)
                        {
                            forward = defaultDir;
                            forward.y = 0.0f;
                            if (forward.squaredLength() < 1e-4f)
                            {
                                forward = Ogre::Vector3::UNIT_Z;
                            }
                        }
                        forward.normalise();

                        const Ogre::Vector3 worldUp = Ogre::Vector3::UNIT_Y;
                        Ogre::Vector3 right = forward.crossProduct(worldUp);
                        right.normalise();
                        Ogre::Vector3 cleanUp = right.crossProduct(forward);
                        cleanUp.normalise();

                        // Map the three axes back to X/Y/Z columns based on the ship's default direction.
                        // Column 0 = X axis (right), Column 1 = Y axis (up), Column 2 = Z axis (forward in Ogre).
                        // We need to place our computed axes into the slots that match the default direction.
                        Ogre::Vector3 axisX, axisY, axisZ;

                        if (Ogre::Math::Abs(defaultDir.dotProduct(Ogre::Vector3::UNIT_Z)) > 0.9f)
                        {
                            // Default direction is +Z or -Z
                            axisX = right;
                            axisY = cleanUp;
                            axisZ = forward * defaultDir.dotProduct(Ogre::Vector3::UNIT_Z); // preserve sign
                        }
                        else if (Ogre::Math::Abs(defaultDir.dotProduct(Ogre::Vector3::UNIT_X)) > 0.9f)
                        {
                            // Default direction is +X or -X
                            axisZ = right;
                            axisY = cleanUp;
                            axisX = forward * defaultDir.dotProduct(Ogre::Vector3::UNIT_X); // preserve sign
                        }
                        else
                        {
                            // Fallback: treat as Z
                            axisX = right;
                            axisY = cleanUp;
                            axisZ = forward;
                        }

                        Ogre::Matrix3 mat;
                        mat.SetColumn(0, axisX);
                        mat.SetColumn(1, axisY);
                        mat.SetColumn(2, axisZ);
                        Ogre::Quaternion uprightOrient;
                        uprightOrient.FromRotationMatrix(mat);

                        actComp->setOrientation(uprightOrient);
#else

                        Ogre::Quaternion currentOrient = actComp->getOrientation();
                        Ogre::Vector3 defaultDir = shipGo->getDefaultDirection();

                        // Get actual gravity direction — on a planet surface this points toward the planet
                        // center, not necessarily -Y
                        Ogre::Vector3 gravityDir = actComp->getGravityDirection();
                        if (gravityDir.isZeroLength())
                        {
                            gravityDir = Ogre::Vector3::NEGATIVE_UNIT_Y;
                        }

                        // World up = opposite of gravity
                        Ogre::Vector3 worldUp = -gravityDir.normalisedCopy();

                        // Extract the ship's current forward in world space
                        Ogre::Vector3 forward = currentOrient * defaultDir;

                        // Project onto the plane perpendicular to worldUp — strips roll AND pitch
                        forward = forward - forward.dotProduct(worldUp) * worldUp;

                        if (forward.squaredLength() < 1e-4f)
                        {
                            // Ship pointing straight up/down — try using current right axis projected
                            Ogre::Vector3 right = currentOrient * Ogre::Vector3::UNIT_X;
                            forward = right - right.dotProduct(worldUp) * worldUp;
                            if (forward.squaredLength() < 1e-4f)
                            {
                                // Last resort — pick any stable reference
                                Ogre::Vector3 worldRef = (Ogre::Math::Abs(worldUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
                                forward = worldRef - worldRef.dotProduct(worldUp) * worldRef;
                            }
                        }
                        forward.normalise();

                        // Build clean orthogonal frame
                        Ogre::Vector3 right = forward.crossProduct(worldUp);
                        right.normalise();
                        Ogre::Vector3 cleanUp = right.crossProduct(forward);
                        cleanUp.normalise();

                        // Place axes into the correct matrix columns for this ship's default direction
                        Ogre::Vector3 axisX, axisY, axisZ;

                        if (Ogre::Math::Abs(defaultDir.dotProduct(Ogre::Vector3::UNIT_Z)) > 0.9f)
                        {
                            axisX = right;
                            axisY = cleanUp;
                            axisZ = forward * defaultDir.dotProduct(Ogre::Vector3::UNIT_Z);
                        }
                        else if (Ogre::Math::Abs(defaultDir.dotProduct(Ogre::Vector3::UNIT_X)) > 0.9f)
                        {
                            axisZ = right;
                            axisY = cleanUp;
                            axisX = forward * defaultDir.dotProduct(Ogre::Vector3::UNIT_X);
                        }
                        else
                        {
                            axisX = right;
                            axisY = cleanUp;
                            axisZ = forward;
                        }

                        Ogre::Matrix3 mat;
                        mat.SetColumn(0, axisX);
                        mat.SetColumn(1, axisY);
                        mat.SetColumn(2, axisZ);
                        Ogre::Quaternion uprightOrient;
                        uprightOrient.FromRotationMatrix(mat);
                        actComp->setOrientation(uprightOrient);
#endif
                    }
                }
            }
            this->landingContactActive = false;
            this->landingContactTimer = 0.0f;
            this->resolvedLandingTargetValid = false;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Takeoff requested on body id=" + Ogre::StringConverter::toString(this->landedOnBodyId));
    }

    void UniversumComponent::requestLanding(void)
    {
        /*if (this->landingState != LandingState::APPROACHING)
        {
            return;
        }*/
        this->landingState = LandingState::LANDING;
        this->setPlayerInputLock(true);
        this->landingInputLocked = true;
        this->landingFired = false;
        this->landingContactActive = false;
        this->landingContactTimer = 0.0f;

        // Fire the reactOnLanding callback immediately so Lua can show
        // the approach fade-in regardless of whether APPROACHING threshold was reached.
        if (false == this->landingFired)
        {
            this->landingFired = true;
            const unsigned long playerGOId = this->playerGameObjectId->getULong();
            this->callLandingFunction(this->landedOnBodyId, playerGOId);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] reactOnLanding fired from requestLanding for body id=" + Ogre::StringConverter::toString(this->landedOnBodyId));
        }

        // Register contact callback on the ship so any collision with the
        // planet/moon surface sets the landingContactActive flag.
        const unsigned long shipGOId = this->playerGameObjectId->getULong();
        if (0ul != shipGOId)
        {
            GameObjectPtr shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(shipGOId);
            if (nullptr != shipGo)
            {
                auto actComp = NOWA::makeStrongPtr(shipGo->getComponent<PhysicsActiveComponent>());
                if (nullptr != actComp)
                {
                    const unsigned long bodyId = this->landedOnBodyId;
                    actComp->setCppContactCallback(
                        [this, bodyId](GameObjectPtr otherGo, const OgreNewt::ContactSnapshot&)
                        {
                            if (nullptr != otherGo && otherGo->getId() == bodyId)
                            {
                                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                                    "[UniversumComponent] contactCallback fired for body id=" + Ogre::StringConverter::toString(bodyId) + " landingState=" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->landingState)));
                                this->landingContactActive = true;
                            }
                        });
                }
            }
        }
    }

    void UniversumComponent::callLandedFunction(unsigned long bodyId, unsigned long shipId)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->landedClosureFunction.is_valid())
        {
            return;
        }

        NOWA::AppStateManager::LogicCommand cmd = [this, bodyId, shipId]()
        {
            try
            {
                auto ctrl = AppStateManager::getSingletonPtr()->getGameObjectController();
                // Pass raw pointers — never GameObjectPtr to Lua (shared_ptr lifetime bug)
                auto bodyGameObjectPtr = ctrl->getGameObjectFromId(bodyId);
                auto shipGameObjectPtr = ctrl->getGameObjectFromId(shipId);

                GameObject* bodyGameObject = nullptr;
                GameObject* shipGameObject = nullptr;

                if (nullptr != bodyGameObjectPtr)
                {
                    bodyGameObject = bodyGameObjectPtr.get();
                }

                if (nullptr != shipGameObjectPtr)
                {
                    shipGameObject = shipGameObjectPtr.get();
                }

                luabind::call_function<void>(this->landedClosureFunction, bodyGameObject, shipGameObject);
            }
            catch (luabind::error& e)
            {
                luabind::object errMsg(luabind::from_stack(e.state(), -1));
                std::stringstream ss;
                ss << errMsg;
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Lua error in reactOnLanded: " + Ogre::String(e.what()) + " details: " + ss.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
    }

    void UniversumComponent::callLandingFunction(unsigned long bodyId, unsigned long shipId)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->landingClosureFunction.is_valid())
        {
            return;
        }

        NOWA::AppStateManager::LogicCommand cmd = [this, bodyId, shipId]()
        {
            try
            {
                auto ctrl = AppStateManager::getSingletonPtr()->getGameObjectController();
                // Pass raw pointers — never GameObjectPtr to Lua (shared_ptr lifetime bug)
                auto bodyGameObjectPtr = ctrl->getGameObjectFromId(bodyId);
                auto shipGameObjectPtr = ctrl->getGameObjectFromId(shipId);

                GameObject* bodyGameObject = nullptr;
                GameObject* shipGameObject = nullptr;

                if (nullptr != bodyGameObjectPtr)
                {
                    bodyGameObject = bodyGameObjectPtr.get();
                }

                if (nullptr != shipGameObjectPtr)
                {
                    shipGameObject = shipGameObjectPtr.get();
                }

                luabind::call_function<void>(this->landingClosureFunction, bodyGameObject, shipGameObject);
            }
            catch (luabind::error& e)
            {
                luabind::object errMsg(luabind::from_stack(e.state(), -1));
                std::stringstream ss;
                ss << errMsg;
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Lua error in reactOnLanding: " + Ogre::String(e.what()) + " details: " + ss.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
    }

    void UniversumComponent::snapMoonToOrbit(OrbitalBody& moon, const OrbitalBody& parentPlanet)
    {
        Ogre::Vector3 offset = moon.currentPosition - parentPlanet.currentPosition;

        if (std::abs(moon.orbitalSpeed) > 1e-6f)
        {
            float newAngle = std::atan2(offset.z, offset.x);
            moon.orbitalElapsed = (newAngle - moon.phaseOffset) / moon.orbitalSpeed;
        }

        float angle = moon.orbitalElapsed * moon.orbitalSpeed + moon.phaseOffset;
        moon.currentPosition = Ogre::Vector3(parentPlanet.currentPosition.x + std::cos(angle) * moon.orbitalRadius, parentPlanet.currentPosition.y + std::sin(angle * moon.orbitalTilt) * moon.orbitalRadius * 0.1f,
            parentPlanet.currentPosition.z + std::sin(angle) * moon.orbitalRadius);
    }

    void UniversumComponent::snapPlanetToOrbit(OrbitalBody& planet, const Ogre::Vector3& sunPos)
    {
        // Re-derive orbitalElapsed from the planet's current world position so
        // the orbit resumes from exactly where the planet was frozen, with no jump.
        // Mirrors snapMoonToOrbit but the reference point is the sun, not a parent.
        Ogre::Vector3 offset = planet.currentPosition - sunPos;
        if (std::abs(planet.orbitalSpeed) > 1e-6f)
        {
            float newAngle = std::atan2(offset.z, offset.x);
            planet.orbitalElapsed = (newAngle - planet.phaseOffset) / planet.orbitalSpeed;
        }
        // Recompute currentPosition from the formula for floating-point consistency.
        float angle = planet.orbitalElapsed * planet.orbitalSpeed + planet.phaseOffset;
        planet.currentPosition = Ogre::Vector3(sunPos.x + std::cos(angle) * planet.orbitalRadius, sunPos.y + std::sin(angle * planet.orbitalTilt) * planet.orbitalRadius * 0.1f, sunPos.z + std::sin(angle) * planet.orbitalRadius);
    }

    // ============================================================
    // Refactored -- duplications removed, helpers extracted.
    // Logic is identical to the original; only structure changed.
    // ============================================================

    // ---------------------------------------------------------------
    // Private helpers -- add declarations to the .h
    // ---------------------------------------------------------------

    // Restore PSSM lambda to compositor defaults (space mode).
    void UniversumComponent::restoreSpaceShadowSettings()
    {
        const Ogre::String& shadowNodeName = WorkspaceModule::getInstance()->shadowNodeName;
        Ogre::CompositorShadowNodeDef* shadowDef = Ogre::Root::getSingleton().getCompositorManager2()->getShadowNodeDefinitionNonConst(shadowNodeName);
        if (nullptr == shadowDef)
        {
            return;
        }
        const bool isEsm = (shadowNodeName.find("ESM") != Ogre::String::npos);
        const float defaultLambda = isEsm ? 0.70f : 0.95f;
        const size_t numDefs = shadowDef->getNumShadowTextureDefinitions();
        for (size_t si = 0u; si < numDefs; ++si)
        {
            shadowDef->getShadowTextureDefinitionNonConst(si)->pssmLambda = defaultLambda;
        }
    }

    // Set shadow casting on any body GO's movable, dispatched to render thread.
    void UniversumComponent::setBodyCastShadows(unsigned long bodyGoId, bool castShadows, const Ogre::String& tag)
    {
        GameObjectPtr bodyGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(bodyGoId);
        if (nullptr == bodyGo)
        {
            return;
        }

        Ogre::MovableObject* movable = bodyGo->getMovableObject();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[setBodyCastShadows] bodyGoId=" + Ogre::StringConverter::toString(bodyGoId) + " movable=" + (movable ? movable->getName() : "NULL") + " castShadows=" + Ogre::StringConverter::toString(castShadows));

        if (nullptr == movable)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand cmd = [movable, castShadows]
        {
            movable->setCastShadows(castShadows);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), Ogre::String(tag).c_str());
    }

    // Shared setup called from both planet and moon branches of pausePlanetOrbit.
    void UniversumComponent::setupLandingState(OrbitalBody& body, unsigned long bodyGoId,
        float axialSpeedOverride, // planet.axialSpeed or moon.orbitalSpeed
        unsigned long playerGoId)
    {
        this->playerOnSurface = true;
        this->landedOnBodyId = bodyGoId;
        this->landingBodyCentre = body.currentPosition;
        this->landingBodyRadius = 0.0f;
        this->landingFired = false;
        this->landingDebugTimer = 0.0f;
        this->fakeLightAxialSpeed = axialSpeedOverride;
        this->fakeLightElapsed = 0.0f;
        this->landingState = LandingState::APPROACHING;

        GameObjectPtr bodyGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(bodyGoId);
        if (nullptr != bodyGo)
        {
            auto terraComp = NOWA::makeStrongPtr(bodyGo->getComponent<PlanetTerraComponent>());
            if (nullptr != terraComp)
            {
                // Use computedMaxRadius which equals base sphere radius + max(heightData[i]).
                // This is the actual outermost vertex of the terrain mesh after procedural
                // generation and any brush edits. Using just getRadius() (the base sphere)
                // means distToSurface is too large by the hill amplitude, causing the
                // approach threshold to fire far too late (ship visually grazes hilltops
                // while we still report hundreds of units of altitude).
                this->landingBodyRadius = terraComp->getComputedMaxRadius();
            }

            // Scale the approach threshold with body size so the Land button fires
            // at a comfortable altitude regardless of planet/moon size.
            // Rule: max(80, radius * 0.04) capped at 500. For radius=2630 -> ~105 m.
            this->landingAltitudeThreshold = Ogre::Math::Clamp(this->landingBodyRadius * 0.04f, 80.0f, 500.0f);

            // Takeoff clearance = 1.5x approach threshold.
            this->takeoffClearanceAltitude = this->landingAltitudeThreshold * 1.5f;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] setupLandingState bodyId=" + Ogre::StringConverter::toString(bodyGoId) + " computedMaxRadius=" +
                                                                                   Ogre::StringConverter::toString(this->landingBodyRadius) + " landingAltitudeThreshold=" + Ogre::StringConverter::toString(this->landingAltitudeThreshold));
        }

        this->currentFarClip = this->farClipSurface->getReal();

        // Enable shadow casting on the directional light so the ship shadow
        // is visible on the moon surface.
        if (nullptr != this->cachedSunLight)
        {
            Ogre::Light* light = this->cachedSunLight;
            NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
            {
                const Ogre::String& shadowNodeName = WorkspaceModule::getInstance()->shadowNodeName;
                Ogre::CompositorShadowNodeDef* shadowDef = Ogre::Root::getSingleton().getCompositorManager2()->getShadowNodeDefinitionNonConst(shadowNodeName);
                if (nullptr != shadowDef)
                {
                    const size_t numDefs = shadowDef->getNumShadowTextureDefinitions();
                    for (size_t si = 0u; si < numDefs; ++si)
                    {
                        shadowDef->getShadowTextureDefinitionNonConst(si)->pssmLambda = 0.95f;
                    }
                }

                light->setCastShadows(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(shadowCmd), "UniversumComponent::setupLandingState::enableShadows");
        }

        // Disable shadow CASTING on the moon mesh only.
        // The moon still RECEIVES shadows (ship shadow falls on it correctly).
        // Without this, the moon sphere projects its circular silhouette onto
        // its own surface via the PSSM shadow map = the black hole artifact.
        this->setBodyCastShadows(bodyGoId, false, "UniversumComponent::setupLandingState::bodyCastShadowsFalse");
    }

    // Shared teardown called from both planet and moon branches of resumePlanetOrbit.
    // Returns true if no body is still paused (fully back in space).
    bool UniversumComponent::teardownLandingState(unsigned long planetGameObjectId, unsigned long gameObjectId, OrbitalBody& body, bool isHideSurface, SolarSystem& system)
    {
        body.orbitalPaused = false;

        this->shadowsConfiguredForSurface = false;
        this->restoreSpaceShadowSettings();
        this->setBodyCastShadows(this->landedOnBodyId, true, "UniversumComponent::teardownLandingState::bodyCastShadowsTrue");

        if (isHideSurface)
        {
            this->hideSurfaceObjects(body);
        }
        this->callPlanetLeftFunction(planetGameObjectId, gameObjectId);

        // Check if any other body is still paused.
        bool anyPaused = false;
        for (const OrbitalBody& p : system.planets)
        {
            if (true == p.orbitalPaused)
            {
                anyPaused = true;
                break;
            }
        }
        if (false == anyPaused)
        {
            for (const auto& mp : system.moons)
            {
                if (true == mp.second.orbitalPaused)
                {
                    anyPaused = true;
                    break;
                }
            }
        }
        this->playerOnSurface = anyPaused;

        if (false == anyPaused)
        {
            this->firstLightFrame = true;
            this->currentFarClip = this->farClipSpace->getReal();

            if (nullptr != this->cachedSunLight)
            {
                Ogre::Light* light = this->cachedSunLight;
                NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
                {
                    light->setCastShadows(false);
                };
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(shadowCmd), "UniversumComponent::teardownLandingState::disableShadows");
            }

            this->restoreSpaceShadowSettings();

            if (nullptr != this->cachedAtmosphereComponent)
            {
                this->cachedAtmosphereComponent->setActivated(false);
                this->cachedAtmosphereComponent->setExternalLightMode(false);
                this->cachedAtmosphereComponent = nullptr;
            }
        }

        return !anyPaused;
    }

    UniversumComponent::LandingState UniversumComponent::getLandingState(void) const
    {
        return this->landingState;
    }

    bool UniversumComponent::findFlatLandingSpot(const Ogre::Vector3& shipPos, const Ogre::Vector3& surfaceNormal, const Ogre::Vector3& bodyCentre, Ogre::Real bodyRadius, GameObjectPtr shipGo, Ogre::Vector3& outTarget)
    {
        GameObjectPtr bodyGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->landedOnBodyId);
        if (nullptr == bodyGo)
        {
            return false;
        }

        auto terraComp = NOWA::makeStrongPtr(bodyGo->getComponent<PlanetTerraComponent>());
        if (nullptr == terraComp)
        {
            return false;
        }

        const Ogre::Vector3 outwardDir = (shipPos - bodyCentre).normalisedCopy();

        const float searchAngles[] = {20.0f, 35.0f, 60.0f};
        const int numAngles = static_cast<int>(sizeof(searchAngles) / sizeof(searchAngles[0]));

        Ogre::Vector3 bestWorldPos = Ogre::Vector3::ZERO;
        Ogre::Vector3 bestWorldNormal = surfaceNormal;
        bool found = false;

        for (int ai = 0; ai < numAngles && false == found; ++ai)
        {
            found = terraComp->findFlatLandingVertex(outwardDir, searchAngles[ai], bestWorldPos, bestWorldNormal);
            if (found)
            {
                float distFromCentre = (bestWorldPos - bodyCentre).length();
                if (distFromCentre < bodyRadius * 0.9f)
                {
                    // Vertex is inside the planet sphere — valley or transform issue.
                    // Fall back to radial projection on the nominal sphere.
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] findFlatLandingSpot: vertex inside sphere "
                                                                                       "(dist=" +
                                                                                           Ogre::StringConverter::toString(distFromCentre) + " vs radius=" + Ogre::StringConverter::toString(bodyRadius) + "), using radial projection");
                    bestWorldPos = bodyCentre + outwardDir * bodyRadius;
                    bestWorldNormal = outwardDir;
                }
                else
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] findFlatLandingSpot: vertex found at angle=" + Ogre::StringConverter::toString(searchAngles[ai]) +
                                                                                           " pos=" + Ogre::StringConverter::toString(bestWorldPos) + " normal=" + Ogre::StringConverter::toString(bestWorldNormal));
                }
            }
        }

        if (false == found)
        {
            // Absolute fallback: radial projection onto nominal sphere.
            bestWorldPos = bodyCentre + outwardDir * bodyRadius;
            bestWorldNormal = outwardDir;
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] findFlatLandingSpot: no vertex in 60-deg cone, using radial projection");
        }

        outTarget = bestWorldPos;
        return true; // always returns a valid position
    }

    void UniversumComponent::pausePlanetOrbit(unsigned long planetGameObjectId, unsigned long gameObjectId)
    {
        const unsigned long playerGoId = gameObjectId;

        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                if (planet.gameObjectId != planetGameObjectId)
                {
                    continue;
                }

                planet.orbitalPaused = true;
                this->setupLandingState(planet, planetGameObjectId, planet.axialSpeed, playerGoId);
                this->showSurfaceObjects(planet);
                this->callPlanetEnteredFunction(planetGameObjectId, gameObjectId);

                {
                    GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                    if (nullptr != planetGo)
                    {
                        this->setPlanetDynamic(planetGo, false);
                    }
                }

                // Also pause all moons orbiting this planet so they cannot
                // collide with the ship/player or exit their AOI unexpectedly
                // while the player is on the surface.
                for (auto& moonPair : system.moons)
                {
                    // moonPair.first is the parent planet index; find which index this planet is.
                    // We match by iterating planets to find the index.
                    size_t planetIdx = 0u;
                    bool foundIdx = false;
                    for (size_t pi = 0u; pi < system.planets.size(); ++pi)
                    {
                        if (system.planets[pi].gameObjectId == planetGameObjectId)
                        {
                            planetIdx = pi;
                            foundIdx = true;
                            break;
                        }
                    }
                    if (false == foundIdx || moonPair.first != planetIdx)
                    {
                        continue;
                    }
                    if (false == moonPair.second.orbitalPaused)
                    {
                        moonPair.second.orbitalPaused = true;
                        GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moonPair.second.gameObjectId);
                        if (nullptr != moonGo)
                        {
                            this->setPlanetDynamic(moonGo, false);
                        }
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Moon id=" + Ogre::StringConverter::toString(moonPair.second.gameObjectId) + " also paused while player is on parent planet.");
                    }
                }

                // Disable shadow casting on all distant bodies — their geometry
                // blows out the PSSM frustum even with small farClip on the surface
                for (SolarSystem& sys : this->solarSystems)
                {
                    for (OrbitalBody& otherPlanet : sys.planets)
                    {
                        if (otherPlanet.gameObjectId != planetGameObjectId)
                        {
                            this->setBodyCastShadows(otherPlanet.gameObjectId, false, "UniversumComponent::pausePlanetOrbit::disableDistantShadows");
                        }
                    }
                    for (auto& moonPair : sys.moons)
                    {
                        if (moonPair.second.gameObjectId != planetGameObjectId)
                        {
                            this->setBodyCastShadows(moonPair.second.gameObjectId, false, "UniversumComponent::pausePlanetOrbit::disableDistantShadows");
                        }
                    }
                }

                // Enable shadows with constrained far distance for surface mode
                if (nullptr != this->cachedSunLight)
                {
                    Ogre::Light* light = this->cachedSunLight;
                    Ogre::Real surfaceFarClip = this->farClipSurface->getReal();
                    NOWA::GraphicsModule::RenderCommand cmd = [light, surfaceFarClip]()
                    {
                        light->setCastShadows(true);
                        light->setShadowFarDistance(surfaceFarClip);
                    };
                    NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "UniversumComponent::pausePlanetOrbit::enableShadows");
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[UniversumComponent] Orbital motion paused for planet id=" + Ogre::StringConverter::toString(planetGameObjectId) + " -- landing state=APPROACHING, bodyRadius=" + Ogre::StringConverter::toString(this->landingBodyRadius));
                return;
            }

            for (auto& moonPair : system.moons)
            {
                if (moonPair.second.gameObjectId != planetGameObjectId)
                {
                    continue;
                }

                OrbitalBody& moon = moonPair.second;
                moon.orbitalPaused = true;
                this->setupLandingState(moon, planetGameObjectId, moon.axialSpeed, playerGoId);
                this->showSurfaceObjects(moon);
                this->callPlanetEnteredFunction(planetGameObjectId, gameObjectId);

                {
                    GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                    if (nullptr != moonGo)
                    {
                        this->setPlanetDynamic(moonGo, false);
                    }
                }

                // Also pause the parent planet (and all sibling moons of this moon) so
                // they cannot collide with the frozen moon while the player is on its surface.
                const size_t parentIdx = moonPair.first;
                if (parentIdx < system.planets.size())
                {
                    OrbitalBody& parentPlanet = system.planets[parentIdx];
                    if (false == parentPlanet.orbitalPaused)
                    {
                        parentPlanet.orbitalPaused = true;
                        GameObjectPtr parentGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(parentPlanet.gameObjectId);
                        if (nullptr != parentGo)
                        {
                            this->setPlanetDynamic(parentGo, false);
                        }
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Parent planet id=" + Ogre::StringConverter::toString(parentPlanet.gameObjectId) + " also paused while player is on moon.");
                    }

                    // Pause all sibling moons of this parent as well
                    for (auto& siblingPair : system.moons)
                    {
                        if (siblingPair.first == parentIdx && siblingPair.second.gameObjectId != planetGameObjectId)
                        {
                            if (false == siblingPair.second.orbitalPaused)
                            {
                                siblingPair.second.orbitalPaused = true;
                                GameObjectPtr siblingGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(siblingPair.second.gameObjectId);
                                if (nullptr != siblingGo)
                                {
                                    this->setPlanetDynamic(siblingGo, false);
                                }
                                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Sibling moon id=" + Ogre::StringConverter::toString(siblingPair.second.gameObjectId) + " also paused while player is on moon.");
                            }
                        }
                    }
                }

                // Disable shadow casting on all distant bodies
                for (SolarSystem& sys : this->solarSystems)
                {
                    for (OrbitalBody& otherPlanet : sys.planets)
                    {
                        this->setBodyCastShadows(otherPlanet.gameObjectId, false, "UniversumComponent::pausePlanetOrbit::disableDistantShadows");
                    }
                    for (auto& otherMoonPair : sys.moons)
                    {
                        if (otherMoonPair.second.gameObjectId != planetGameObjectId)
                        {
                            this->setBodyCastShadows(otherMoonPair.second.gameObjectId, false, "UniversumComponent::pausePlanetOrbit::disableDistantShadows");
                        }
                    }
                }

                // Enable shadows with constrained far distance for surface mode
                if (nullptr != this->cachedSunLight)
                {
                    Ogre::Light* light = this->cachedSunLight;
                    Ogre::Real surfaceFarClip = this->farClipSurface->getReal();
                    NOWA::GraphicsModule::RenderCommand cmd = [light, surfaceFarClip]()
                    {
                        light->setCastShadows(true);
                        light->setShadowFarDistance(surfaceFarClip);
                    };
                    NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "UniversumComponent::pausePlanetOrbit::enableShadows");
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[UniversumComponent] Orbital motion paused for moon id=" + Ogre::StringConverter::toString(planetGameObjectId) + " -- landing state=APPROACHING, bodyRadius=" + Ogre::StringConverter::toString(this->landingBodyRadius));
                return;
            }
        }
    }

    void UniversumComponent::resumePlanetOrbit(unsigned long planetGameObjectId, unsigned long gameObjectId)
    {
        // Always reset landing state at the top regardless of which branch fires.
        if (true == this->landingInputLocked)
        {
            this->setPlayerInputLock(false);
            this->landingInputLocked = false;
        }
        this->landingState = LandingState::NONE;
        this->landedOnBodyId = 0ul;
        this->landingBodyRadius = 0.0f;
        this->landingFired = false;
        this->fakeLightElapsed = 0.0f;
        this->currentAmbientUpper = Ogre::Vector3(0.03f, 0.03f, 0.04f);
        this->currentAmbientLower = Ogre::Vector3(0.03f, 0.03f, 0.04f);

        {
            const unsigned long shipGOId = this->playerGameObjectId->getULong();
            if (0ul != shipGOId)
            {
                GameObjectPtr shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(shipGOId);
                if (nullptr != shipGo)
                {
                    auto actComp = NOWA::makeStrongPtr(shipGo->getComponent<PhysicsActiveComponent>());
                    if (nullptr != actComp)
                    {
                        actComp->removeCppContactCallback();
                    }
                }
            }
            this->landingContactActive = false;
            this->landingContactTimer = 0.0f;
        }

        // Restore shadow casting on all bodies and disable surface shadow mode
        for (SolarSystem& sys : this->solarSystems)
        {
            for (OrbitalBody& planet : sys.planets)
            {
                this->setBodyCastShadows(planet.gameObjectId, true, "UniversumComponent::resumePlanetOrbit::restoreShadows");
            }
            for (auto& moonPair : sys.moons)
            {
                this->setBodyCastShadows(moonPair.second.gameObjectId, true, "UniversumComponent::resumePlanetOrbit::restoreShadows");
            }
            // Sun never casts shadows
            this->setBodyCastShadows(sys.sunId, false, "UniversumComponent::resumePlanetOrbit::sunNoCastShadows");
        }

        // Disable shadows and reset far distance for space mode
        if (nullptr != this->cachedSunLight)
        {
            Ogre::Light* light = this->cachedSunLight;
            NOWA::GraphicsModule::RenderCommand cmd = [light]()
            {
                light->setCastShadows(false);
                light->setShadowFarDistance(0.0f); // 0 = use scene default / disabled
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "UniversumComponent::resumePlanetOrbit::disableShadows");
        }

        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                if (planet.gameObjectId != planetGameObjectId)
                {
                    continue;
                }

                GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                if (nullptr != planetGo)
                {
                    this->setPlanetDynamic(planetGo, true);
                }

                this->teardownLandingState(planetGameObjectId, gameObjectId, planet, true, system);

                GameObjectPtr sunGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(system.sunId);
                if (nullptr != sunGo)
                {
                    this->snapPlanetToOrbit(planet, sunGo->getPosition());
                }

                // Resume all moons of this planet that were force-paused during landing.
                size_t planetIdx = 0u;
                bool foundIdx = false;
                for (size_t pi = 0u; pi < system.planets.size(); ++pi)
                {
                    if (system.planets[pi].gameObjectId == planetGameObjectId)
                    {
                        planetIdx = pi;
                        foundIdx = true;
                        break;
                    }
                }
                if (true == foundIdx)
                {
                    for (auto& moonPair : system.moons)
                    {
                        if (moonPair.first != planetIdx)
                        {
                            continue;
                        }
                        if (true == moonPair.second.orbitalPaused)
                        {
                            moonPair.second.orbitalPaused = false;
                            GameObjectPtr moonGo2 = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moonPair.second.gameObjectId);
                            if (nullptr != moonGo2)
                            {
                                this->setPlanetDynamic(moonGo2, true);
                            }
                            this->snapMoonToOrbit(moonPair.second, planet);
                            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Moon id=" + Ogre::StringConverter::toString(moonPair.second.gameObjectId) + " resumed after player left parent planet.");
                        }
                    }
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Orbital motion resumed for planet id=" + Ogre::StringConverter::toString(planetGameObjectId));
                return;
            }

            for (auto& moonPair : system.moons)
            {
                if (moonPair.second.gameObjectId != planetGameObjectId)
                {
                    continue;
                }

                OrbitalBody& moon = moonPair.second;
                const size_t parentIdx = moonPair.first;

                GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                if (nullptr != moonGo)
                {
                    this->setPlanetDynamic(moonGo, true);
                }

                this->teardownLandingState(planetGameObjectId, gameObjectId, moon, true, system);

                if (parentIdx < system.planets.size())
                {
                    this->snapMoonToOrbit(moon, system.planets[parentIdx]);

                    // Resume the parent planet that was frozen when we landed on this moon.
                    OrbitalBody& parentPlanet = system.planets[parentIdx];
                    if (true == parentPlanet.orbitalPaused)
                    {
                        parentPlanet.orbitalPaused = false;
                        GameObjectPtr parentGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(parentPlanet.gameObjectId);
                        if (nullptr != parentGo)
                        {
                            this->setPlanetDynamic(parentGo, true);
                        }
                        GameObjectPtr sunGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(system.sunId);
                        if (nullptr != sunGo)
                        {
                            this->snapPlanetToOrbit(parentPlanet, sunGo->getPosition());
                        }
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Parent planet id=" + Ogre::StringConverter::toString(parentPlanet.gameObjectId) + " resumed after player left moon.");
                    }

                    // Resume sibling moons of this parent that were force-paused.
                    for (auto& siblingPair : system.moons)
                    {
                        if (siblingPair.first == parentIdx && siblingPair.second.gameObjectId != planetGameObjectId)
                        {
                            if (true == siblingPair.second.orbitalPaused)
                            {
                                siblingPair.second.orbitalPaused = false;
                                GameObjectPtr siblingGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(siblingPair.second.gameObjectId);
                                if (nullptr != siblingGo)
                                {
                                    this->setPlanetDynamic(siblingGo, true);
                                }
                                this->snapMoonToOrbit(siblingPair.second, parentPlanet);
                                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Sibling moon id=" + Ogre::StringConverter::toString(siblingPair.second.gameObjectId) + " resumed after player left moon.");
                            }
                        }
                    }
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Orbital motion resumed for moon id=" + Ogre::StringConverter::toString(planetGameObjectId));
                return;
            }
        }
    }

    // ---------------------------------------------------------------
    // update
    // ---------------------------------------------------------------
    void UniversumComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }
        this->elapsedTime += dt;

        // ---- Orbital motion ----
        if (true == this->useMotion->getBool())
        {
            for (SolarSystem& system : this->solarSystems)
            {
                GameObjectPtr sunGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(system.sunId);
                if (nullptr == sunGo)
                {
                    continue;
                }
                Ogre::Vector3 sunPos = sunGo->getPosition();

                for (OrbitalBody& planet : system.planets)
                {
                    if (true == planet.orbitalPaused)
                    {
                        continue;
                    }
                    GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                    if (nullptr == planetGo)
                    {
                        continue;
                    }
                    auto physComp = NOWA::makeStrongPtr(planetGo->getComponent<PhysicsArtifactComponent>());
                    if (nullptr == physComp)
                    {
                        continue;
                    }
                    planet.orbitalElapsed += dt;
                    planet.axialElapsed += dt;
                    float angle = planet.orbitalElapsed * planet.orbitalSpeed + planet.phaseOffset;
                    Ogre::Vector3 newPos(sunPos.x + std::cos(angle) * planet.orbitalRadius, sunPos.y + std::sin(angle * planet.orbitalTilt) * planet.orbitalRadius * 0.1f, sunPos.z + std::sin(angle) * planet.orbitalRadius);
                    planet.currentPosition = newPos;
                    Ogre::Radian axialAngle(planet.axialElapsed * planet.axialSpeed);
                    Ogre::Quaternion axialRot(axialAngle, Ogre::Vector3::UNIT_Y);
                    physComp->setPositionOrientation(newPos, axialRot);
                }

                for (auto& moonPair : system.moons)
                {
                    const size_t parentIdx = moonPair.first;
                    OrbitalBody& moon = moonPair.second;

                    if (true == moon.orbitalPaused || parentIdx >= system.planets.size())
                    {
                        continue;
                    }
                    moon.orbitalElapsed += dt;
                    moon.axialElapsed += dt;
                    Ogre::Vector3 parentPos = system.planets[parentIdx].currentPosition;
                    float angle = moon.orbitalElapsed * moon.orbitalSpeed + moon.phaseOffset;
                    Ogre::Vector3 newPos(parentPos.x + std::cos(angle) * moon.orbitalRadius, parentPos.y + std::sin(angle * moon.orbitalTilt) * moon.orbitalRadius * 0.1f, parentPos.z + std::sin(angle) * moon.orbitalRadius);
                    moon.currentPosition = newPos;

                    GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moon.gameObjectId);
                    if (nullptr == moonGo)
                    {
                        continue;
                    }
                    auto physComp = NOWA::makeStrongPtr(moonGo->getComponent<PhysicsArtifactComponent>());
                    if (nullptr == physComp)
                    {
                        continue;
                    }
                    Ogre::Radian axialAngle(moon.axialElapsed * moon.axialSpeed);
                    Ogre::Quaternion axialRot(axialAngle, Ogre::Vector3::UNIT_Y);
                    physComp->setPositionOrientation(newPos, axialRot);
                }
            }
        }

        this->updateLandingStateMachine(dt);

        // ---- Far-clip adaptation (space only) ----
        if (false == this->playerOnSurface)
        {
            const unsigned long playerGOId = this->playerGameObjectId->getULong();
            const unsigned long cameraGOId = this->cameraGameObjectId->getULong();
            if (0ul != playerGOId && 0ul != cameraGOId)
            {
                GameObjectPtr playerGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(playerGOId);
                GameObjectPtr cameraGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(cameraGOId);
                if (nullptr != playerGo && nullptr != cameraGo)
                {
                    Ogre::Vector3 playerPos = playerGo->getPosition();
                    float nearestSurface = std::numeric_limits<float>::max();

                    for (const unsigned long ownedId : this->ownedGameObjectIds)
                    {
                        GameObjectPtr ownedGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(ownedId);
                        if (nullptr == ownedGo)
                        {
                            continue;
                        }
                        auto terraComp = NOWA::makeStrongPtr(ownedGo->getComponent<PlanetTerraComponent>());
                        if (nullptr != terraComp)
                        {
                            float dist = (playerPos - ownedGo->getPosition()).length() - terraComp->getRadius();
                            if (dist < nearestSurface)
                            {
                                nearestSurface = dist;
                            }
                        }
                    }

                    const float surfaceThreshold = 50.0f;
                    float targetFarClip = (nearestSurface < surfaceThreshold) ? this->farClipSurface->getReal() : this->farClipSpace->getReal();
                    this->currentFarClip = this->currentFarClip + (targetFarClip - this->currentFarClip) * dt * this->farClipTransitionSpeed->getReal();
                }
            }
        }

        // ---- Sun light steering ----
        if (nullptr == this->cachedSunLight || 0ul == this->cameraGameObjectId->getULong())
        {
            return;
        }

        GameObjectPtr cameraGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraGameObjectId->getULong());
        if (nullptr == cameraGo)
        {
            return;
        }

        Ogre::Vector3 camPos = cameraGo->getPosition();

        // Find nearest solar system by actual sun GO position.
        float nearestDist = std::numeric_limits<float>::max();
        SolarSystem* nearestSystem = nullptr;
        for (SolarSystem& system : this->solarSystems)
        {
            GameObjectPtr testSun = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(system.sunId);
            if (nullptr == testSun)
            {
                continue;
            }
            float dist = (testSun->getPosition() - camPos).length();
            if (dist < nearestDist)
            {
                nearestDist = dist;
                nearestSystem = &system;
            }
        }
        if (nullptr == nearestSystem)
        {
            return;
        }

        GameObjectPtr sunGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(nearestSystem->sunId);
        if (nullptr == sunGo)
        {
            return;
        }

        const Ogre::Vector3 sunPos = sunGo->getPosition();

        // Resolve ship GO once -- used for both playerActuallyOnSurface and shipPos.
        GameObjectPtr shipGo;
        {
            const unsigned long shipGoId = this->playerGameObjectId->getULong();
            if (0ul != shipGoId)
            {
                shipGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(shipGoId);
            }
        }

        // playerActuallyOnSurface: ship (not camera) must be within 20 units of surface.
        bool playerActuallyOnSurface = false;
        if (true == this->playerOnSurface && this->landingBodyRadius > 0.0f && nullptr != shipGo)
        {
            float distToSurface = (shipGo->getPosition() - this->landingBodyCentre).length() - this->landingBodyRadius;
            playerActuallyOnSurface = (distToSurface <= 20.0f);
        }

        // Light color lerp runs every frame in all modes.
        this->currentLightColor = this->currentLightColor + (nearestSystem->sunLightColor - this->currentLightColor) * dt * 0.5f;

        // Power lerp only in space/approach mode.
        if (false == playerActuallyOnSurface)
        {
            if (true == this->firstLightFrame)
            {
                this->currentLightPower = nearestSystem->sunLightPower;
                this->firstLightFrame = false;
            }
            else
            {
                this->currentLightPower = this->currentLightPower + (nearestSystem->sunLightPower - this->currentLightPower) * dt * 0.5f;
            }
        }

        Ogre::Vector3 dir;
        Ogre::Vector3 ambientUpper = Ogre::Vector3(0.03f, 0.03f, 0.04f);
        Ogre::Vector3 ambientLower = Ogre::Vector3(0.03f, 0.03f, 0.04f);
        float finalLightPower = this->currentLightPower;

        if (false == playerActuallyOnSurface)
        {
            // SPACE / APPROACH MODE
            dir = camPos - sunPos;
            // Lerp ambient back to space values in case we just left a surface.
            const Ogre::Vector3 spaceAmb = Ogre::Vector3(0.03f, 0.03f, 0.04f);
            this->currentAmbientUpper = this->currentAmbientUpper + (spaceAmb - this->currentAmbientUpper) * dt * 2.0f;
            this->currentAmbientLower = this->currentAmbientLower + (spaceAmb - this->currentAmbientLower) * dt * 2.0f;
            ambientUpper = this->currentAmbientUpper;
            ambientLower = this->currentAmbientLower;
        }
        else
        {
            // SURFACE MODE
            this->fakeLightElapsed += dt;
            Ogre::Vector3 baseDir = (this->landingBodyCentre - sunPos).normalisedCopy();
            Ogre::Radian fakeAngle(this->fakeLightElapsed * this->fakeLightAxialSpeed);
            Ogre::Quaternion fakeRot(fakeAngle, Ogre::Vector3::UNIT_Y);
            dir = fakeRot * baseDir;
            if (dir.squaredLength() < 1e-6f)
            {
                dir = Ogre::Vector3::UNIT_Y;
            }

            const Ogre::Vector3 shipPos = (nullptr != shipGo) ? shipGo->getPosition() : this->landingBodyCentre;
            const Ogre::Vector3 surfaceNormal = (shipPos - this->landingBodyCentre).normalisedCopy();
            const float sunElevation = surfaceNormal.dotProduct(-dir.normalisedCopy());
            float dayFactor = Ogre::Math::Clamp((sunElevation + 0.15f) / 0.30f, 0.0f, 1.0f);
            dayFactor = dayFactor * dayFactor * (3.0f - 2.0f * dayFactor);

            const Ogre::Vector3 nightUpper = Ogre::Vector3(0.01f, 0.01f, 0.02f);
            const Ogre::Vector3 nightLower = Ogre::Vector3(0.005f, 0.005f, 0.01f);
            const Ogre::Vector3 dayUpper = nearestSystem->sunLightColor * 0.4f;
            const Ogre::Vector3 dayLower = nearestSystem->sunLightColor * 0.15f;

            const Ogre::Vector3 targetAmbUpper = nightUpper + (dayUpper - nightUpper) * dayFactor;
            const Ogre::Vector3 targetAmbLower = nightLower + (dayLower - nightLower) * dayFactor;
            const float targetPower = nearestSystem->sunLightPower * dayFactor;

            // Lerp everything so the day/night transition is smooth and there is no
            // sudden jump when playerActuallyOnSurface first becomes true.
            const float lerpSpeed = 2.0f;
            this->currentLightPower = this->currentLightPower + (targetPower - this->currentLightPower) * dt * lerpSpeed;
            this->currentAmbientUpper = this->currentAmbientUpper + (targetAmbUpper - this->currentAmbientUpper) * dt * lerpSpeed;
            this->currentAmbientLower = this->currentAmbientLower + (targetAmbLower - this->currentAmbientLower) * dt * lerpSpeed;

            finalLightPower = this->currentLightPower;
            ambientUpper = this->currentAmbientUpper;
            ambientLower = this->currentAmbientLower;
        }

        if (dir.squaredLength() <= 0.0f)
        {
            return;
        }

        dir.normalise();

        Ogre::Light* light = this->cachedSunLight;
        Ogre::Vector3 color = this->currentLightColor;
        float power = finalLightPower;
        Ogre::Vector3 ambUpper = ambientUpper;
        Ogre::Vector3 ambLower = ambientLower;

        auto closureFunction = [light, dir, color, power, ambUpper, ambLower](Ogre::Real /*renderDt*/)
        {
            light->setDirection(dir);
            light->setDiffuseColour(color.x, color.y, color.z);
            light->setPowerScale(power);
            light->_getManager()->setAmbientLight(Ogre::ColourValue(ambUpper.x, ambUpper.y, ambUpper.z), Ogre::ColourValue(ambLower.x, ambLower.y, ambLower.z), -dir);
        };

        Ogre::String closureId = this->gameObjectPtr->getName() + "::sunLight";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closureFunction, false);
    }

    // =========================================================================
    //  actualizeValue
    // =========================================================================

    void UniversumComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (UniversumComponent::AttrRandomSeed() == attribute->getName())
        {
            this->setRandomSeed(attribute->getUInt());
        }
        else if (UniversumComponent::AttrSolarSystemCount() == attribute->getName())
        {
            this->setSolarSystemCount(attribute->getUInt());
        }
        else if (UniversumComponent::AttrSolarSystemDistanceMin() == attribute->getName())
        {
            this->setSolarSystemDistanceMin(attribute->getReal());
        }
        else if (UniversumComponent::AttrSolarSystemDistanceMax() == attribute->getName())
        {
            this->setSolarSystemDistanceMax(attribute->getReal());
        }
        else if (UniversumComponent::AttrPlanetsPerSystemMin() == attribute->getName())
        {
            this->setPlanetsPerSystemMin(attribute->getUInt());
        }
        else if (UniversumComponent::AttrPlanetsPerSystemMax() == attribute->getName())
        {
            this->setPlanetsPerSystemMax(attribute->getUInt());
        }
        else if (UniversumComponent::AttrUseMoons() == attribute->getName())
        {
            this->setUseMoons(attribute->getBool());
        }
        else if (UniversumComponent::AttrMoonsPerPlanetMin() == attribute->getName())
        {
            this->setMoonsPerPlanetMin(attribute->getUInt());
        }
        else if (UniversumComponent::AttrMoonsPerPlanetMax() == attribute->getName())
        {
            this->setMoonsPerPlanetMax(attribute->getUInt());
        }
        else if (UniversumComponent::AttrUseMotion() == attribute->getName())
        {
            this->setUseMotion(attribute->getBool());
        }
        else if (UniversumComponent::AttrUseOcean() == attribute->getName())
        {
            this->setUseOcean(attribute->getBool());
        }
        else if (UniversumComponent::AttrOceanProbability() == attribute->getName())
        {
            this->setOceanProbability(attribute->getReal());
        }
        else if (UniversumComponent::AttrSunRadius() == attribute->getName())
        {
            this->setSunRadius(attribute->getReal());
        }
        else if (UniversumComponent::AttrPlanetRadiusMin() == attribute->getName())
        {
            this->setPlanetRadiusMin(attribute->getReal());
        }
        else if (UniversumComponent::AttrPlanetRadiusMax() == attribute->getName())
        {
            this->setPlanetRadiusMax(attribute->getReal());
        }
        else if (UniversumComponent::AttrMoonRadius() == attribute->getName())
        {
            this->setMoonRadius(attribute->getReal());
        }
        else if (UniversumComponent::AttrOrbitalDistanceMin() == attribute->getName())
        {
            this->setOrbitalDistanceMin(attribute->getReal());
        }
        else if (UniversumComponent::AttrOrbitalDistanceMax() == attribute->getName())
        {
            this->setOrbitalDistanceMax(attribute->getReal());
        }
        else if (UniversumComponent::AttrMoonOrbitalDistanceMin() == attribute->getName())
        {
            this->setMoonOrbitalDistanceMin(attribute->getReal());
        }
        else if (UniversumComponent::AttrMoonOrbitalDistanceMax() == attribute->getName())
        {
            this->setMoonOrbitalDistanceMax(attribute->getReal());
        }
        else if (UniversumComponent::AttrOrbitalSpeedMin() == attribute->getName())
        {
            this->setOrbitalSpeedMin(attribute->getReal());
        }
        else if (UniversumComponent::AttrOrbitalSpeedMax() == attribute->getName())
        {
            this->setOrbitalSpeedMax(attribute->getReal());
        }
        else if (UniversumComponent::AttrAxialSpeedMin() == attribute->getName())
        {
            this->setAxialSpeedMin(attribute->getReal());
        }
        else if (UniversumComponent::AttrAxialSpeedMax() == attribute->getName())
        {
            this->setAxialSpeedMax(attribute->getReal());
        }
        else if (UniversumComponent::AttrPlayerGameObjectId() == attribute->getName())
        {
            this->setPlayerGameObjectId(attribute->getULong());
        }
        else if (UniversumComponent::AttrCameraGameObjectId() == attribute->getName())
        {
            this->setCameraGameObjectId(attribute->getULong());
        }
        else if (UniversumComponent::AttrSunLightGameObjectId() == attribute->getName())
        {
            this->setSunLightGameObjectId(attribute->getULong());
        }
        else if (UniversumComponent::AttrFarClipSpace() == attribute->getName())
        {
            this->setFarClipSpace(attribute->getReal());
        }
        else if (UniversumComponent::AttrFarClipSurface() == attribute->getName())
        {
            this->setFarClipSurface(attribute->getReal());
        }
        else if (UniversumComponent::AttrFarClipTransitionSpeed() == attribute->getName())
        {
            this->setFarClipTransitionSpeed(attribute->getReal());
        }
        else if (UniversumComponent::AttrScale() == attribute->getName())
        {
            this->setScale(attribute->getReal());
        }
        else if (UniversumComponent::AttrAutoPauseOrbit() == attribute->getName())
        {
            this->setAutoPauseOrbit(attribute->getBool());
        }
    }

    // =========================================================================
    //  executeAction -- Generate button
    // =========================================================================

    bool UniversumComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if (UniversumComponent::ActionGenerate() == actionId)
        {
            this->generateUniverse();
        }
        return true;
    }

    // =========================================================================
    //  writeXML
    // =========================================================================

    void UniversumComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2=uint, 6=real, 7=string, 12=bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        auto writeAttr = [&](const char* type, const char* attrName, const Ogre::String& data)
        {
            xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", type));
            propertyXML->append_attribute(doc.allocate_attribute("name", attrName));
            propertyXML->append_attribute(doc.allocate_attribute("data", doc.allocate_string(data.c_str())));
            propertiesXML->append_node(propertyXML);
        };

        writeAttr("2", "Random Seed", XMLConverter::ConvertString(doc, this->randomSeed->getUInt()));
        writeAttr("2", "Solar System Count", XMLConverter::ConvertString(doc, this->solarSystemCount->getUInt()));
        writeAttr("6", "Solar System Distance Min", XMLConverter::ConvertString(doc, this->solarSystemDistanceMin->getReal()));
        writeAttr("6", "Solar System Distance Max", XMLConverter::ConvertString(doc, this->solarSystemDistanceMax->getReal()));
        writeAttr("2", "Planets Per System Min", XMLConverter::ConvertString(doc, this->planetsPerSystemMin->getUInt()));
        writeAttr("2", "Planets Per System Max", XMLConverter::ConvertString(doc, this->planetsPerSystemMax->getUInt()));
        writeAttr("12", "Use Moons", XMLConverter::ConvertString(doc, this->useMoons->getBool()));
        writeAttr("2", "Moons Per Planet Min", XMLConverter::ConvertString(doc, this->moonsPerPlanetMin->getUInt()));
        writeAttr("2", "Moons Per Planet Max", XMLConverter::ConvertString(doc, this->moonsPerPlanetMax->getUInt()));
        writeAttr("12", "Use Motion", XMLConverter::ConvertString(doc, this->useMotion->getBool()));
        writeAttr("12", "Use Ocean", XMLConverter::ConvertString(doc, this->useOcean->getBool()));
        writeAttr("6", "Ocean Probability", XMLConverter::ConvertString(doc, this->oceanProbability->getReal()));
        writeAttr("6", "Sun Radius", XMLConverter::ConvertString(doc, this->sunRadius->getReal()));
        writeAttr("6", "Planet Radius Min", XMLConverter::ConvertString(doc, this->planetRadiusMin->getReal()));
        writeAttr("6", "Planet Radius Max", XMLConverter::ConvertString(doc, this->planetRadiusMax->getReal()));
        writeAttr("6", "Moon Radius", XMLConverter::ConvertString(doc, this->moonRadius->getReal()));
        writeAttr("6", "Orbital Distance Min", XMLConverter::ConvertString(doc, this->orbitalDistanceMin->getReal()));
        writeAttr("6", "Orbital Distance Max", XMLConverter::ConvertString(doc, this->orbitalDistanceMax->getReal()));
        writeAttr("6", "Moon Orbital Distance Min", XMLConverter::ConvertString(doc, this->moonOrbitalDistanceMin->getReal()));
        writeAttr("6", "Moon Orbital Distance Max", XMLConverter::ConvertString(doc, this->moonOrbitalDistanceMax->getReal()));
        writeAttr("6", "Orbital Speed Min", XMLConverter::ConvertString(doc, this->orbitalSpeedMin->getReal()));
        writeAttr("6", "Orbital Speed Max", XMLConverter::ConvertString(doc, this->orbitalSpeedMax->getReal()));
        writeAttr("6", "Axial Speed Min", XMLConverter::ConvertString(doc, this->axialSpeedMin->getReal()));
        writeAttr("6", "Axial Speed Max", XMLConverter::ConvertString(doc, this->axialSpeedMax->getReal()));
        writeAttr("2", "Player GameObject Id", XMLConverter::ConvertString(doc, this->playerGameObjectId->getULong()));
        writeAttr("2", "Camera GameObject Id", XMLConverter::ConvertString(doc, this->cameraGameObjectId->getULong()));
        writeAttr("2", "Sun Light GameObject Id", XMLConverter::ConvertString(doc, this->sunLightGameObjectId->getULong()));
        writeAttr("6", "Far Clip Space", XMLConverter::ConvertString(doc, this->farClipSpace->getReal()));
        writeAttr("6", "Far Clip Surface", XMLConverter::ConvertString(doc, this->farClipSurface->getReal()));
        writeAttr("6", "Far Clip Transition Speed", XMLConverter::ConvertString(doc, this->farClipTransitionSpeed->getReal()));
        writeAttr("6", "Scale", XMLConverter::ConvertString(doc, this->scale->getReal()));
        writeAttr("12", "Auto Pause Orbit On Surface", XMLConverter::ConvertString(doc, this->autoPauseOrbit->getBool()));

        // ---- Save solar systems so scene reload restores orbital motion --------
        writeAttr("2", "Saved System Count", XMLConverter::ConvertString(doc, static_cast<unsigned int>(this->solarSystems.size())));

        for (size_t si = 0u; si < this->solarSystems.size(); ++si)
        {
            const SolarSystem& sys = this->solarSystems[si];
            const Ogre::String sp = "Sys" + Ogre::StringConverter::toString(static_cast<unsigned int>(si)) + "_";

            writeAttr("2", doc.allocate_string((sp + "SunId").c_str()), XMLConverter::ConvertString(doc, sys.sunId));
            writeAttr("9", doc.allocate_string((sp + "LightColor").c_str()), XMLConverter::ConvertString(doc, sys.sunLightColor));
            writeAttr("6", doc.allocate_string((sp + "LightPower").c_str()), XMLConverter::ConvertString(doc, sys.sunLightPower));
            writeAttr("2", doc.allocate_string((sp + "PlanetCount").c_str()), XMLConverter::ConvertString(doc, static_cast<unsigned int>(sys.planets.size())));

            for (size_t pi = 0u; pi < sys.planets.size(); ++pi)
            {
                const OrbitalBody& p = sys.planets[pi];
                const Ogre::String pp = sp + "P" + Ogre::StringConverter::toString(static_cast<unsigned int>(pi)) + "_";
                writeAttr("2", doc.allocate_string((pp + "Id").c_str()), XMLConverter::ConvertString(doc, p.gameObjectId));
                writeAttr("6", doc.allocate_string((pp + "OrbR").c_str()), XMLConverter::ConvertString(doc, p.orbitalRadius));
                writeAttr("6", doc.allocate_string((pp + "OrbS").c_str()), XMLConverter::ConvertString(doc, p.orbitalSpeed));
                writeAttr("6", doc.allocate_string((pp + "OrbT").c_str()), XMLConverter::ConvertString(doc, p.orbitalTilt));
                writeAttr("6", doc.allocate_string((pp + "Phase").c_str()), XMLConverter::ConvertString(doc, p.phaseOffset));
                writeAttr("6", doc.allocate_string((pp + "AxS").c_str()), XMLConverter::ConvertString(doc, p.axialSpeed));
                writeAttr("6", doc.allocate_string((pp + "Grav").c_str()), XMLConverter::ConvertString(doc, p.gravityStrength));
            }

            writeAttr("2", doc.allocate_string((sp + "MoonCount").c_str()), XMLConverter::ConvertString(doc, static_cast<unsigned int>(sys.moons.size())));

            for (size_t mi = 0u; mi < sys.moons.size(); ++mi)
            {
                const Ogre::String mp = sp + "M" + Ogre::StringConverter::toString(static_cast<unsigned int>(mi)) + "_";
                writeAttr("2", doc.allocate_string((mp + "Parent").c_str()), XMLConverter::ConvertString(doc, static_cast<unsigned int>(sys.moons[mi].first)));
                const OrbitalBody& m = sys.moons[mi].second;
                writeAttr("2", doc.allocate_string((mp + "Id").c_str()), XMLConverter::ConvertString(doc, m.gameObjectId));
                writeAttr("6", doc.allocate_string((mp + "OrbR").c_str()), XMLConverter::ConvertString(doc, m.orbitalRadius));
                writeAttr("6", doc.allocate_string((mp + "OrbS").c_str()), XMLConverter::ConvertString(doc, m.orbitalSpeed));
                writeAttr("6", doc.allocate_string((mp + "OrbT").c_str()), XMLConverter::ConvertString(doc, m.orbitalTilt));
                writeAttr("6", doc.allocate_string((mp + "Phase").c_str()), XMLConverter::ConvertString(doc, m.phaseOffset));
                writeAttr("6", doc.allocate_string((mp + "AxS").c_str()), XMLConverter::ConvertString(doc, m.axialSpeed));
                writeAttr("6", doc.allocate_string((mp + "Grav").c_str()), XMLConverter::ConvertString(doc, m.gravityStrength));
            }
        }

        // Save owned GO ids so destroyUniverse can find them for .ptd file management.
        Ogre::String ownedIds;
        for (size_t i = 0u; i < this->ownedGameObjectIds.size(); ++i)
        {
            if (i > 0u)
            {
                ownedIds += ",";
            }
            ownedIds += Ogre::StringConverter::toString(this->ownedGameObjectIds[i]);
        }
        writeAttr("7", "Owned GO Ids", ownedIds);
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr UniversumComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // UniversumComponent owns a procedural solar system with dozens of generated
        // GameObjects, observers, and runtime state. Cloning it would duplicate all of
        // that into the same scene which makes no sense. Return nullptr to prevent it.
        return nullptr;
    }

    // =========================================================================
    //  getClassName / getParentClassName
    // =========================================================================

    Ogre::String UniversumComponent::getClassName(void) const
    {
        return "UniversumComponent";
    }

    Ogre::String UniversumComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    // =========================================================================
    //  Universe generation
    // =========================================================================

    // =========================================================================
    //  buildTexturePools
    //
    //  Scans the "TerrainTextures" resource group for files matching *_d.* (diffuse).
    //  Categorises by name fragment:
    //    contains "sun"  -> sunTextures
    //    contains "moon" -> moonTextures
    //    everything else -> planetTextures
    //
    //  Pattern mirrors the particle scanning code: openResource is not needed;
    //  findResourceNames returns the filename list directly.
    // =========================================================================

    // =========================================================================
    //  computeGravity
    //
    //  Surface gravity scales with planet radius via the shell theorem approximation:
    //    g = g_ref * (radius / ref_radius)
    //  Reference: 50m planet = 19.8 m/s^2 (feels like heavy Earth).
    //  Moon (5m) -> ~2 m/s^2 (floaty). Large planet (300m) -> ~120 m/s^2 (crushing).
    //  Clamped to [2, 150] so extremes stay playable.
    // =========================================================================

    float UniversumComponent::computeGravity(float radius) const
    {
        const float refRadius = 50.0f;
        const float refGravity = 19.8f;
        float g = refGravity * (radius / refRadius);
        return std::max(2.0f, std::min(150.0f, g));
    }

    // =========================================================================
    //  pausePlanetOrbit / resumePlanetOrbit
    //  Called by AreaOfInterestComponent Lua callbacks.
    // =========================================================================

    void UniversumComponent::setPlanetDynamic(GameObjectPtr goPtr, bool isDynamic)
    {
        if (nullptr == goPtr)
        {
            return;
        }
        // Primary GO setDynamic updates only the primary movableObject (the GO's own item).
        goPtr->setDynamic(isDynamic);

        // PlanetTerraComponent, PlanetOceanComponent, and PlanetSunComponent each own a
        // separate Ogre::Item. All items on a SceneNode MUST share the same static state
        // or Ogre asserts. Call setDynamic() on each sub-component explicitly.
        auto terraComp = NOWA::makeStrongPtr(goPtr->getComponent<PlanetTerraComponent>());
        if (nullptr != terraComp)
        {
            terraComp->setDynamic(isDynamic);
        }

        auto oceanComp = NOWA::makeStrongPtr(goPtr->getComponent<PlanetOceanComponent>());
        if (nullptr != oceanComp)
        {
            oceanComp->setDynamic(isDynamic);
        }

        auto sunComp = NOWA::makeStrongPtr(goPtr->getComponent<PlanetSunComponent>());
        if (nullptr != sunComp)
        {
            sunComp->setDynamic(isDynamic);
        }

        auto physComp = NOWA::makeStrongPtr(goPtr->getComponent<PhysicsArtifactComponent>());
        if (nullptr != physComp)
        {
            physComp->getBody()->setSleep(!isDynamic);
        }
    }

    // =========================================================================
    //  deriveNormalTextureName
    //
    //  Given a diffuse texture filename (e.g. "adesert_mntn2_d.jpg"), returns
    //  the matching normal map name ("adesert_mntn2_n.dds") if it exists in the
    //  provided pool. Returns empty string if no match is found.
    //
    //  Convention: replace "_d." with "_n." and change extension to ".dds".
    //  This matches the TerrainTextures naming scheme throughout.
    // =========================================================================

    static Ogre::String deriveNormalTextureName(const Ogre::String& diffuseName, const std::vector<Ogre::String>& normalPool)
    {
        const size_t pos = diffuseName.find("_d.");
        if (Ogre::String::npos == pos)
        {
            return "";
        }
        // Replace "_d.<ext>" with "_n.dds"
        const Ogre::String baseName = diffuseName.substr(0u, pos);
        const Ogre::String candidate = baseName + "_n.dds";

        // Check if it exists in the pool (case-sensitive -- filenames are lowercase)
        for (const Ogre::String& n : normalPool)
        {
            if (n == candidate)
            {
                return candidate;
            }
        }
        return "";
    }

    // =========================================================================
    //  Surface object management
    // =========================================================================

    void UniversumComponent::registerAndHideSurfaceObjects(void)
    {
        // Clear any previous surface object lists on all bodies.
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                planet.surfaceObjects.clear();
            }
            for (auto& moonPair : system.moons)
            {
                moonPair.second.surfaceObjects.clear();
            }
        }

        // Scan ALL GameObjects in the scene for PlanetSurfaceComponent.
        auto allGOs = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
        for (auto& goPair : *allGOs)
        {
            auto surfComp = NOWA::makeStrongPtr(goPair.second->getComponent<PlanetSurfaceComponent>());
            if (nullptr == surfComp)
            {
                continue;
            }
            const unsigned long targetId = surfComp->getTargetPlanetId();
            if (0ul == targetId)
            {
                continue;
            }

            // Find the OrbitalBody with this id and register the surface object.
            bool registered = false;
            for (SolarSystem& system : this->solarSystems)
            {
                if (true == registered)
                {
                    break;
                }
                for (OrbitalBody& planet : system.planets)
                {
                    if (planet.gameObjectId == targetId)
                    {
                        // Compute local offset from planet at its current (spawn) position.
                        GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(targetId);
                        if (nullptr != planetGo)
                        {
                            SurfaceObject so;
                            so.gameObjectId = goPair.second->getId();
                            so.savedWorldPosition = goPair.second->getPosition();
                            so.savedWorldOrientation = goPair.second->getOrientation();

                            Ogre::Quaternion planetRot = planetGo->getOrientation();
                            Ogre::Vector3 planetPos = planetGo->getPosition();
                            Ogre::Vector3 worldOffset = so.savedWorldPosition - planetPos;
                            so.localPosition = planetRot.Inverse() * worldOffset;
                            so.localOrientation = planetRot.Inverse() * so.savedWorldOrientation;

                            planet.surfaceObjects.push_back(so);
                            registered = true;

                            // Hide and disable physics.
                            this->hideSurfaceObjects(planet);
                        }
                        break;
                    }
                }
                if (true == registered)
                {
                    break;
                }
                for (auto& moonPair : system.moons)
                {
                    if (moonPair.second.gameObjectId == targetId)
                    {
                        GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(targetId);
                        if (nullptr != moonGo)
                        {
                            SurfaceObject so;
                            so.gameObjectId = goPair.second->getId();
                            so.savedWorldPosition = goPair.second->getPosition();
                            so.savedWorldOrientation = goPair.second->getOrientation();

                            Ogre::Quaternion moonRot = moonGo->getOrientation();
                            Ogre::Vector3 moonPos = moonGo->getPosition();
                            Ogre::Vector3 worldOffset = so.savedWorldPosition - moonPos;
                            so.localPosition = moonRot.Inverse() * worldOffset;
                            so.localOrientation = moonRot.Inverse() * so.savedWorldOrientation;

                            moonPair.second.surfaceObjects.push_back(so);
                            registered = true;

                            // Hide and disable physics.
                            this->hideSurfaceObjects(moonPair.second);
                        }
                        break;
                    }
                }
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Surface objects registered and hidden.");
    }

    void UniversumComponent::unregisterSurfaceObjects(void)
    {
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                planet.surfaceObjects.clear();
            }
            for (auto& moonPair : system.moons)
            {
                moonPair.second.surfaceObjects.clear();
            }
        }
    }

    void UniversumComponent::showSurfaceObjects(OrbitalBody& body)
    {
        GameObjectPtr hostGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(body.gameObjectId);
        if (nullptr == hostGo)
        {
            return;
        }
        const Ogre::Vector3 hostPos = hostGo->getPosition();
        const Ogre::Quaternion hostRot = hostGo->getOrientation();

        for (const SurfaceObject& so : body.surfaceObjects)
        {
            GameObjectPtr go = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(so.gameObjectId);
            if (nullptr == go)
            {
                continue;
            }
            // Place at correct world position relative to frozen planet.
            Ogre::Vector3 worldPos = hostPos + hostRot * so.localPosition;
            Ogre::Quaternion worldRot = hostRot * so.localOrientation;
            go->setVisible(true);

            // Use physics component setter if available, else move SceneNode directly.
            auto physComp = NOWA::makeStrongPtr(go->getComponent<PhysicsComponent>());
            if (nullptr != physComp)
            {
                physComp->setPosition(worldPos);
                physComp->setOrientation(worldRot);
                if (nullptr != physComp->getBody())
                {
                    physComp->setActivated(true);
                }
            }
            else
            {
                go->setAttributePosition(worldPos);
                go->setAttributeOrientation(worldRot);
            }
        }
    }

    void UniversumComponent::hideSurfaceObjects(OrbitalBody& body)
    {
        for (const SurfaceObject& so : body.surfaceObjects)
        {
            GameObjectPtr go = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(so.gameObjectId);
            if (nullptr == go)
            {
                continue;
            }
            go->setVisible(false);

            // Force physics to sleep so it doesn't affect the world while hidden.
            auto physComp = NOWA::makeStrongPtr(go->getComponent<PhysicsComponent>());
            if (nullptr != physComp && nullptr != physComp->getBody())
            {
                physComp->setActivated(false);
            }
        }
    }

    void UniversumComponent::restoreAllSurfaceObjects(void)
    {
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                for (const SurfaceObject& so : planet.surfaceObjects)
                {
                    GameObjectPtr go = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(so.gameObjectId);
                    if (nullptr == go)
                    {
                        continue;
                    }
                    go->setVisible(true);
                    auto physComp = NOWA::makeStrongPtr(go->getComponent<PhysicsComponent>());
                    if (nullptr != physComp)
                    {
                        physComp->setPosition(so.savedWorldPosition);
                        physComp->setOrientation(so.savedWorldOrientation);
                        if (nullptr != physComp->getBody())
                        {
                            physComp->setActivated(true);
                        }
                    }
                    else
                    {
                        go->setAttributePosition(so.savedWorldPosition);
                        go->setAttributeOrientation(so.savedWorldOrientation);
                    }
                }
                planet.surfaceObjects.clear();
            }
            for (auto& moonPair : system.moons)
            {
                for (const SurfaceObject& so : moonPair.second.surfaceObjects)
                {
                    GameObjectPtr go = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(so.gameObjectId);
                    if (nullptr == go)
                    {
                        continue;
                    }
                    go->setVisible(true);
                    auto physComp = NOWA::makeStrongPtr(go->getComponent<PhysicsComponent>());
                    if (nullptr != physComp)
                    {
                        physComp->setPosition(so.savedWorldPosition);
                        physComp->setOrientation(so.savedWorldOrientation);
                        if (nullptr != physComp->getBody())
                        {
                            physComp->setActivated(true);
                        }
                    }
                    else
                    {
                        go->setAttributePosition(so.savedWorldPosition);
                        go->setAttributeOrientation(so.savedWorldOrientation);
                    }
                }
                moonPair.second.surfaceObjects.clear();
            }
        }
    }

    void UniversumComponent::buildTexturePools(void)
    {
        this->sunTextures.clear();
        this->moonTextures.clear();
        this->planetTextures.clear();
        this->sunNormalTextures.clear();
        this->moonNormalTextures.clear();
        this->planetNormalTextures.clear();

        const Ogre::String resourceGroup = "TerrainTextures";

        Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

        if (false == rgm.resourceGroupExists(resourceGroup))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Resource group '" + resourceGroup + "' not found -- texture pools empty.");
            return;
        }

        Ogre::StringVectorPtr allFiles = rgm.findResourceNames(resourceGroup, "*");

        for (const Ogre::String& filename : *allFiles)
        {
            Ogre::String lower = filename;
            Ogre::StringUtil::toLowerCase(lower);

            // Diffuse: _d. files
            if (Ogre::String::npos != filename.find("_d."))
            {
                if (Ogre::String::npos != lower.find("sun"))
                {
                    this->sunTextures.push_back(filename);
                }
                else if (Ogre::String::npos != lower.find("moon"))
                {
                    this->moonTextures.push_back(filename);
                }
                else
                {
                    this->planetTextures.push_back(filename);
                }
            }
            // Normal: _n. files
            else if (Ogre::String::npos != filename.find("_n."))
            {
                if (Ogre::String::npos != lower.find("sun"))
                {
                    this->sunNormalTextures.push_back(filename);
                }
                else if (Ogre::String::npos != lower.find("moon"))
                {
                    this->moonNormalTextures.push_back(filename);
                }
                else
                {
                    this->planetNormalTextures.push_back(filename);
                }
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[UniversumComponent] Texture pools -- "
            "sun diffuse: " +
                Ogre::StringConverter::toString(static_cast<unsigned int>(this->sunTextures.size())) + " sun normal: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->sunNormalTextures.size())) +
                " | moon diffuse: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->moonTextures.size())) + " moon normal: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->moonNormalTextures.size())) +
                " | planet diffuse: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->planetTextures.size())) + " planet normal: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->planetNormalTextures.size())));
    }

    void UniversumComponent::destroyUniverse(void)
    {
        this->reset();

        if (nullptr != this->landingRayQuery)
        {
            this->gameObjectPtr->getSceneManager()->destroyQuery(this->landingRayQuery);
            this->landingRayQuery = nullptr;
        }

        // Now destroy all owned game objects.
        for (const unsigned long id : this->ownedGameObjectIds)
        {
            AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(id);
        }
        this->ownedGameObjectIds.clear();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Universe destroyed.");
    }

    void UniversumComponent::generateUniverse(void)
    {
        this->destroyUniverse();

        std::mt19937 rng(static_cast<unsigned int>(this->randomSeed->getUInt()));

        this->buildTexturePools();

        const float scaleFactor = std::pow(10.0f, this->scale->getReal());

        const unsigned int systemCount = this->solarSystemCount->getUInt();

        const float distMin = this->solarSystemDistanceMin->getReal();
        const float distMax = this->solarSystemDistanceMax->getReal();
        const float scaledDistMin = distMin * scaleFactor;
        const float scaledDistMax = distMax * scaleFactor;

        for (unsigned int s = 0u; s < systemCount; ++s)
        {
            float progress = (systemCount > 1u) ? static_cast<float>(s) / static_cast<float>(systemCount - 1u) : 1.0f;
            Ogre::String progressText = "Generating Universe: " + Ogre::StringConverter::toString(static_cast<int>(progress * 100.0f)) + "%";

            // Always consume both RNG values so the sequence for s>0 is deterministic.
            float angle = static_cast<float>(s) * 2.399f;
            float radDist = this->randFloat(rng, scaledDistMin, scaledDistMax) * static_cast<float>(s);
            float yVar = this->randFloat(rng, -distMin * 0.1f, distMin * 0.1f);

            Ogre::Vector3 systemPos;
            if (0u == s)
            {
                // First solar system sits at the scene origin so the player
                // spawns inside it. Subsequent systems spiral outward.
                systemPos = Ogre::Vector3::ZERO;
            }
            else
            {
                systemPos = Ogre::Vector3(std::cos(angle) * radDist, yVar, std::sin(angle) * radDist);
            }

            this->generateSolarSystem(s, systemPos, scaleFactor, rng);
        }

        const float approxMaxDist = this->orbitalDistanceMax->getReal() * scaleFactor;
        const float suggestedSpeed = std::max(10.0f, approxMaxDist * 0.1f);

        boost::shared_ptr<EventDataFeedback> speedEvent(boost::make_shared<EventDataFeedback>(true, "[UNIVERSUM_CAMSPEED:" + Ogre::StringConverter::toString(suggestedSpeed) + "]"));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(speedEvent);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Universe generated: " + Ogre::StringConverter::toString(systemCount) + " solar system(s), " +
                                                                               Ogre::StringConverter::toString(static_cast<unsigned int>(this->ownedGameObjectIds.size())) +
                                                                               " bodies."
                                                                               " Suggested camera speed: " +
                                                                               Ogre::StringConverter::toString(suggestedSpeed));
    }

    void UniversumComponent::generateSolarSystem(size_t systemIndex, const Ogre::Vector3& systemPosition, float scaleFactor, std::mt19937& rng)
    {
        SolarSystem system;
        system.position = systemPosition;

        // Pick a random star type -- this drives the directional light color when
        // the camera is in this solar system.
        static const Ogre::Vector3 sunColors[] = {
            Ogre::Vector3(1.0f, 0.95f, 0.80f), // G-type: warm yellow (like our sun)
            Ogre::Vector3(1.0f, 1.0f, 0.90f),  // F-type: white-yellow
            Ogre::Vector3(1.0f, 0.75f, 0.50f), // K-type: orange
            Ogre::Vector3(1.0f, 0.50f, 0.30f), // M-type: red-orange (red dwarf)
            Ogre::Vector3(0.80f, 0.90f, 1.0f), // B-type: blue-white
        };
        std::uniform_int_distribution<int> colorPick(0, 4);
        system.sunLightColor = sunColors[colorPick(rng)];
        system.sunLightPower = this->randFloat(rng, 2.5f, 5.0f);

        // Create sun.
        system.sunId = this->createSun(systemPosition, scaleFactor, rng);
        if (0ul != system.sunId)
        {
            this->ownedGameObjectIds.push_back(system.sunId);
        }

        // Create planets.
        const int planetCount = this->randInt(rng, static_cast<int>(this->planetsPerSystemMin->getUInt()), static_cast<int>(this->planetsPerSystemMax->getUInt()));

        const float sunScaledRadius = this->sunRadius->getReal() * scaleFactor;
        const float planetMaxRadius = this->planetRadiusMax->getReal() * scaleFactor;

        // Planet orbits must start outside the sun: enforce hard clearance.
        const float minPlanetOrbit = sunScaledRadius + planetMaxRadius + sunScaledRadius * 0.5f;
        const float configOrbMin = this->orbitalDistanceMin->getReal() * scaleFactor;
        const float configOrbMax = this->orbitalDistanceMax->getReal() * scaleFactor;
        const float safeOrbMin = std::max(configOrbMin, minPlanetOrbit);
        const float safeOrbMax = std::max(configOrbMax, safeOrbMin + planetMaxRadius * 2.0f * static_cast<float>(planetCount));
        const float orbitalStep = (safeOrbMax - safeOrbMin) / static_cast<float>(std::max(1, planetCount));

        float currentOrbitalRadius = safeOrbMin;

        for (int p = 0; p < planetCount; ++p)
        {
            // Place planet in its own lane with small positive variance.
            float orbRadius = currentOrbitalRadius + this->randFloat(rng, 0.0f, orbitalStep * 0.15f);
            currentOrbitalRadius += orbitalStep;

            float phaseOffset = this->randFloat(rng, 0.0f, Ogre::Math::TWO_PI);
            float initialAngle = phaseOffset;
            Ogre::Vector3 planetPos(systemPosition.x + std::cos(initialAngle) * orbRadius, systemPosition.y, systemPosition.z + std::sin(initialAngle) * orbRadius);

            float planetRadius = this->randFloat(rng, this->planetRadiusMin->getReal() * scaleFactor, this->planetRadiusMax->getReal() * scaleFactor);

            unsigned long planetId = this->createPlanet(planetPos, planetRadius, rng);
            if (0ul == planetId)
            {
                continue;
            }
            this->ownedGameObjectIds.push_back(planetId);

            OrbitalBody planetBody;
            planetBody.gameObjectId = planetId;
            planetBody.orbitalRadius = orbRadius;
            planetBody.orbitalSpeed = this->randFloat(rng, this->orbitalSpeedMin->getReal(), this->orbitalSpeedMax->getReal());
            planetBody.orbitalTilt = this->randFloat(rng, 0.0f, 0.3f);
            planetBody.phaseOffset = phaseOffset;
            planetBody.axialSpeed = this->randFloat(rng, this->axialSpeedMin->getReal(), this->axialSpeedMax->getReal());
            planetBody.orbitalElapsed = 0.0f;
            planetBody.axialElapsed = 0.0f;
            planetBody.orbitalPaused = false;
            planetBody.gravityStrength = this->computeGravity(planetRadius);
            planetBody.currentPosition = planetPos;

            const size_t planetIndex = system.planets.size();
            system.planets.push_back(planetBody);

            // Create moons -- each moon orbits its parent planet.
            if (true == this->useMoons->getBool())
            {
                const int moonCount = this->randInt(rng, static_cast<int>(this->moonsPerPlanetMin->getUInt()), static_cast<int>(this->moonsPerPlanetMax->getUInt()));

                const float moonScaledRadius = this->moonRadius->getReal() * scaleFactor;
                const float moonClearance = planetRadius + moonScaledRadius + planetRadius * 0.3f;
                const float moonOrbMin = std::max(this->moonOrbitalDistanceMin->getReal() * scaleFactor, moonClearance);
                const float moonOrbMax = std::max(this->moonOrbitalDistanceMax->getReal() * scaleFactor, moonOrbMin + moonScaledRadius * 3.0f * static_cast<float>(std::max(1, moonCount)));
                const float moonOrbStep = (moonOrbMax - moonOrbMin) / static_cast<float>(std::max(1, moonCount));

                float moonOrbRadius = moonOrbMin;

                for (int m = 0; m < moonCount; ++m)
                {
                    float mOrbRadius = moonOrbRadius + this->randFloat(rng, 0.0f, moonOrbStep * 0.2f);
                    moonOrbRadius += moonOrbStep;

                    float mPhase = this->randFloat(rng, 0.0f, Ogre::Math::TWO_PI);
                    Ogre::Vector3 moonPos(planetPos.x + std::cos(mPhase) * mOrbRadius, planetPos.y, planetPos.z + std::sin(mPhase) * mOrbRadius);

                    unsigned long moonId = this->createMoon(moonPos, moonScaledRadius, rng);
                    if (0ul == moonId)
                    {
                        continue;
                    }
                    this->ownedGameObjectIds.push_back(moonId);

                    OrbitalBody moonBody;
                    moonBody.gameObjectId = moonId;
                    moonBody.orbitalRadius = mOrbRadius;
                    moonBody.orbitalSpeed = this->randFloat(rng, this->orbitalSpeedMin->getReal() * 2.0f, this->orbitalSpeedMax->getReal() * 2.0f);
                    moonBody.orbitalTilt = this->randFloat(rng, 0.0f, 0.2f);
                    moonBody.phaseOffset = mPhase;
                    moonBody.axialSpeed = this->randFloat(rng, this->axialSpeedMin->getReal(), this->axialSpeedMax->getReal());
                    moonBody.orbitalElapsed = 0.0f;
                    moonBody.axialElapsed = 0.0f;
                    moonBody.orbitalPaused = false;
                    moonBody.gravityStrength = this->computeGravity(moonScaledRadius);
                    moonBody.currentPosition = moonPos;

                    system.moons.push_back(std::make_pair(planetIndex, moonBody));
                }
            }
        }
        this->solarSystems.push_back(system);
    }

    unsigned long UniversumComponent::createSun(const Ogre::Vector3& position, float scaleFactor, std::mt19937& rng)
    {
        Ogre::SceneManager* sceneMgr = this->gameObjectPtr->getSceneManager();

        Ogre::String goName = "Universum_Sun_" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->ownedGameObjectIds.size()));
        AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(goName);

        Ogre::SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
        node->setPosition(position);
        node->setName(goName);

        GameObjectPtr goPtr = GameObjectFactory::getInstance()->createGameObject(sceneMgr, node, nullptr, NOWA::ITEM, NOWA::makeUniqueID());

        if (nullptr == goPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Failed to create sun GameObject.");
            return 0ul;
        }

        // Create PlanetSunComponent deferred: set all Variant attrs before postInit
        // so the sun is built once with correct values, not default-create + recreate.
        auto sunRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, PlanetSunComponent::getStaticClassName());
        if (nullptr != sunRaw)
        {
            sunRaw->getAttribute(PlanetSunComponent::AttrRadius())->setValue(this->sunRadius->getReal() * scaleFactor);
            // Sun UV scale: large body, use radius * 0.5 clamped to [64, 256].
            const float sunScaledR = this->sunRadius->getReal() * scaleFactor;
            const float sunUVScale = std::max(64.0f, std::min(256.0f, sunScaledR * 0.5f));
            // sunRaw->getAttribute(PlanetSunComponent::AttrBaseUVScale())->setValue(sunUVScale);  // TODO: add AttrBaseUVScale to PlanetSunComponent

            // Diffuse texture set directly on PlanetSunComponent (no DatablockPbsComponent needed).
            if (false == this->sunTextures.empty())
            {
                std::uniform_int_distribution<size_t> pick(0u, this->sunTextures.size() - 1u);
                // sunRaw->getAttribute(PlanetSunComponent::AttrDiffuseTextureName())->setValue(this->sunTextures[pick(rng)]);  // TODO: add AttrDiffuseTextureName to PlanetSunComponent
            }
            if (false == this->sunNormalTextures.empty())
            {
                std::uniform_int_distribution<size_t> pick(0u, this->sunNormalTextures.size() - 1u);
                // sunRaw->getAttribute(PlanetSunComponent::AttrNormalTextureName())->setValue(this->sunNormalTextures[pick(rng)]);  // TODO: add AttrNormalTextureName to PlanetSunComponent
            }

            // postInit creates the sun exactly once with all attributes already configured.
            sunRaw->postInit();
        }

        // Render distance = 0 means infinite -- generated bodies must be visible at any range.
        if (nullptr != goPtr->getMovableObject())
        {
            // Use goPtr->setRenderDistance to go through the proper enqueue path.
            // Pass a large value -- 0 means "skip" in setRenderDistance.
            goPtr->setRenderDistance(1000000u);

            goPtr->setLodDistance(this->sunRadius->getReal() * 6.0f);
        }

        if (true == this->useMotion->getBool())
        {
            GameObjectFactory::getInstance()->createComponent(goPtr, PhysicsArtifactComponent::getStaticClassName(), false);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Created sun '" + goName + "' at " + Ogre::StringConverter::toString(position));

        return goPtr->getId();
    }

    unsigned long UniversumComponent::createPlanet(const Ogre::Vector3& initialPosition, float radius, std::mt19937& rng)
    {
        Ogre::SceneManager* sceneMgr = this->gameObjectPtr->getSceneManager();

        Ogre::String goName = "Universum_Planet_" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->ownedGameObjectIds.size()));
        AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(goName);

        Ogre::SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
        node->setPosition(initialPosition);
        node->setName(goName);

        GameObjectPtr goPtr = GameObjectFactory::getInstance()->createGameObject(sceneMgr, node, nullptr, NOWA::ITEM, NOWA::makeUniqueID());

        if (nullptr == goPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Failed to create planet GameObject.");
            return 0ul;
        }
        // PlanetTerraComponent + DatablockPbsComponent: deferred so radius and textures
        // are set before postInit builds the geometry -- one create, no recreate.
        auto terraRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, PlanetTerraComponent::getStaticClassName());
        auto procRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, ProceduralPlanetComponent::getStaticClassName());

        if (nullptr != terraRaw)
        {
            terraRaw->getAttribute(PlanetTerraComponent::AttrRadius())->setValue(radius);
            // Planet UV scale: radius * 0.5 clamped to [32, 128].
            const float planetUVScale = std::max(32.0f, std::min(128.0f, radius * 0.5f));
            terraRaw->getAttribute(PlanetTerraComponent::AttrBaseUVScale())->setValue(planetUVScale);
            terraRaw->postInit();

            // Set render distance IMMEDIATELY after PlanetTerra postInit, while its item
            // is still the movableObject on the game object. If we wait until after
            // PlanetOceanComponent is created, ocean's init() overwrites movableObject
            // and setRenderDistance would only apply to the ocean item, not the terrain.
            goPtr->setRenderDistance(1000000u);
            goPtr->setLodDistance(radius * 4.0f);
        }

        if (nullptr != procRaw)
        {
            procRaw->postInit();

            // Trigger procedural surface generation now that postInit has run.
            auto procComp = NOWA::makeStrongPtr(goPtr->getComponent<ProceduralPlanetComponent>());
            if (nullptr != procComp)
            {
                procComp->executeAction(ProceduralPlanetComponent::ActionGenerate(), nullptr);
            }
        }

        // DatablockPbsComponent -- regular createComponent, then set textures via setters.
        GameObjectFactory::getInstance()->createComponent(goPtr, "DatablockPbsComponent", false);

        auto datablockPbsCompPtr = NOWA::makeStrongPtr(goPtr->getComponent<DatablockPbsComponent>());
        if (nullptr != datablockPbsCompPtr)
        {
            if (false == this->planetTextures.empty())
            {
                std::uniform_int_distribution<size_t> pick(0u, this->planetTextures.size() - 1u);
                const Ogre::String& chosenDiffuse = this->planetTextures[pick(rng)];
                datablockPbsCompPtr->setDiffuseTextureName(chosenDiffuse);

                // Pair diffuse with its matching normal if available,
                // else fall back to a random normal from the pool.
                const Ogre::String pairedNormal = deriveNormalTextureName(chosenDiffuse, this->planetNormalTextures);
                if (false == pairedNormal.empty())
                {
                    datablockPbsCompPtr->setNormalTextureName(pairedNormal);
                }
                else if (false == this->planetNormalTextures.empty())
                {
                    std::uniform_int_distribution<size_t> pickN(0u, this->planetNormalTextures.size() - 1u);
                    datablockPbsCompPtr->setNormalTextureName(this->planetNormalTextures[pickN(rng)]);
                }
            }

            // 4 shuffled detail layers, each paired with their matching normal map.
            if (false == this->planetTextures.empty())
            {
                std::vector<size_t> indices;
                indices.reserve(this->planetTextures.size());
                for (size_t i = 0u; i < this->planetTextures.size(); ++i)
                {
                    indices.push_back(i);
                }
                std::shuffle(indices.begin(), indices.end(), rng);

                const size_t count = this->planetTextures.size();

                const Ogre::String& d0 = this->planetTextures[indices[0u % count]];
                const Ogre::String& d1 = this->planetTextures[indices[1u % count]];
                const Ogre::String& d2 = this->planetTextures[indices[2u % count]];
                const Ogre::String& d3 = this->planetTextures[indices[3u % count]];

                datablockPbsCompPtr->setDetail0TextureName(d0);
                datablockPbsCompPtr->setDetail1TextureName(d1);
                datablockPbsCompPtr->setDetail2TextureName(d2);
                datablockPbsCompPtr->setDetail3TextureName(d3);

                // Pair each detail diffuse with its matching _n.dds normal map.
                const Ogre::String n0 = deriveNormalTextureName(d0, this->planetNormalTextures);
                const Ogre::String n1 = deriveNormalTextureName(d1, this->planetNormalTextures);
                const Ogre::String n2 = deriveNormalTextureName(d2, this->planetNormalTextures);
                const Ogre::String n3 = deriveNormalTextureName(d3, this->planetNormalTextures);

                if (false == n0.empty())
                {
                    datablockPbsCompPtr->setDetail0NMTextureName(n0);
                }
                if (false == n1.empty())
                {
                    datablockPbsCompPtr->setDetail1NMTextureName(n1);
                }
                if (false == n2.empty())
                {
                    datablockPbsCompPtr->setDetail2NMTextureName(n2);
                }
                if (false == n3.empty())
                {
                    datablockPbsCompPtr->setDetail3NMTextureName(n3);
                }
            }
        }

        // PlanetOceanComponent -- probability-gated, also deferred.
        if (true == this->useOcean->getBool())
        {
            std::uniform_real_distribution<float> dice(0.0f, 1.0f);
            if (dice(rng) < this->oceanProbability->getReal())
            {
                auto oceanRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, PlanetOceanComponent::getStaticClassName());
                if (nullptr != oceanRaw)
                {
                    oceanRaw->getAttribute(PlanetOceanComponent::AttrRadius())->setValue(radius + 2.0f);
                    oceanRaw->postInit();

                    // Ocean creates its own item and overwrites movableObject -- set its
                    // render distance separately directly on the ocean item.
                    auto oceanComp = NOWA::makeStrongPtr(goPtr->getComponent<PlanetOceanComponent>());
                    if (nullptr != oceanComp && nullptr != oceanComp->getOcean() && nullptr != oceanComp->getOcean()->getItem())
                    {
                        NOWA::GraphicsModule::RenderCommand oceanRdCmd = [oceanComp]
                        {
                            oceanComp->getOcean()->getItem()->setRenderingDistance(std::numeric_limits<float>::max());
                        };
                        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "UniversumComponent::oceanRenderDistance");
                    }
                }
            }
        }

        if (true == this->useMotion->getBool())
        {
            GameObjectFactory::getInstance()->createComponent(goPtr, PhysicsArtifactComponent::getStaticClassName(), false);
        }

        // Assign planet category so dynamic bodies (player, spacecraft) can reference it
        // via PhysicsActiveComponent::gravitySourceCategory = "Planets".
        goPtr->changeCategory(UniversumComponent::PlanetCategory());

        // AreaOfInterestComponent: sphere trigger around the planet for enter/leave callbacks.
        // Atmosphere radius = 2x the planet radius. When player enters, Lua fires
        // reactOnEnter -> pausePlanetOrbit. When player leaves -> resumePlanetOrbit.
        {
            auto aoiRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, AreaOfInterestComponent::getStaticClassName());
            if (nullptr != aoiRaw)
            {
                // Atmosphere zone = planet radius * 2 (above surface but well inside orbital lane).
                aoiRaw->getAttribute("Radius")->setValue(radius * 2.0f);
                aoiRaw->getAttribute("Categories")->setValue(Ogre::String("All"));
                aoiRaw->getAttribute("Update Threshold")->setValue(0.1f);
                aoiRaw->postInit();
                // Attach automatic observer -- owner = UniversumComponent, deleted in destroyUniverse().
                auto aoiComp = NOWA::makeStrongPtr(goPtr->getComponent<AreaOfInterestComponent>());
                if (nullptr != aoiComp)
                {
                    PlanetOrbitObserver* obs = new PlanetOrbitObserver(this, goPtr->getId());
                    this->planetObservers.push_back(obs);
                    aoiComp->attachTriggerObserver(obs);
                }
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Created planet '" + goName + "' radius=" + Ogre::StringConverter::toString(radius) +
                                                                               " gravity=" + Ogre::StringConverter::toString(this->computeGravity(radius)) + " at " + Ogre::StringConverter::toString(initialPosition));

        return goPtr->getId();
    }

    unsigned long UniversumComponent::createMoon(const Ogre::Vector3& initialPosition, float radius, std::mt19937& rng)
    {
        Ogre::SceneManager* sceneMgr = this->gameObjectPtr->getSceneManager();

        Ogre::String goName = "Universum_Moon_" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->ownedGameObjectIds.size()));
        AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(goName);

        Ogre::SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
        node->setPosition(initialPosition);
        node->setName(goName);

        GameObjectPtr goPtr = GameObjectFactory::getInstance()->createGameObject(sceneMgr, node, nullptr, NOWA::ITEM, NOWA::makeUniqueID());

        if (nullptr == goPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Failed to create moon GameObject.");
            return 0ul;
        }

        // PlanetTerraComponent + DatablockPbsComponent: deferred creation.
        auto terraRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, PlanetTerraComponent::getStaticClassName());

        if (nullptr != terraRaw)
        {
            // Do not use blend weight for moons!
            boost::dynamic_pointer_cast<PlanetTerraComponent>(terraRaw)->setBlendTexSize(0u);

            terraRaw->getAttribute(PlanetTerraComponent::AttrRadius())->setValue(radius);

            // Moons are small -- reduce UV tiling so texture isn't over-repeated.
            // Default is 128 (designed for large planets). Use radius/4 clamped to [8, 32].
            const float moonUVScale = std::max(8.0f, std::min(32.0f, radius * 0.25f));
            terraRaw->getAttribute(PlanetTerraComponent::AttrBaseUVScale())->setValue(moonUVScale);

            terraRaw->postInit();

            // Set immediately while terra item is still the movableObject.
            goPtr->setRenderDistance(1000000u);
            goPtr->setLodDistance(radius * 4.0f);
        }

        // DatablockPbsComponent -- regular createComponent, then set textures via setters.
        GameObjectFactory::getInstance()->createComponent(goPtr, "DatablockPbsComponent", false);

        auto datablockPbsCompPtr = NOWA::makeStrongPtr(goPtr->getComponent<DatablockPbsComponent>());
        if (nullptr != datablockPbsCompPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Moon texture pools -- moon: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->moonTextures.size())) +
                                                                                   " planet fallback: " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->planetTextures.size())));

            // Diffuse: moon pool with fallback to planet pool.
            const std::vector<Ogre::String>& diffPool = this->moonTextures.empty() ? this->planetTextures : this->moonTextures;
            if (false == diffPool.empty())
            {
                std::uniform_int_distribution<size_t> pick(0u, diffPool.size() - 1u);
                const Ogre::String& chosenDiffuse = diffPool[pick(rng)];
                datablockPbsCompPtr->setDiffuseTextureName(chosenDiffuse);

                // Pair with matching normal, falling back to random from normal pool.
                const std::vector<Ogre::String>& normalPool = this->moonNormalTextures.empty() ? this->planetNormalTextures : this->moonNormalTextures;
                const Ogre::String pairedNormal = deriveNormalTextureName(chosenDiffuse, normalPool);
                if (false == pairedNormal.empty())
                {
                    datablockPbsCompPtr->setNormalTextureName(pairedNormal);
                }
                else if (false == normalPool.empty())
                {
                    std::uniform_int_distribution<size_t> pickN(0u, normalPool.size() - 1u);
                    datablockPbsCompPtr->setNormalTextureName(normalPool[pickN(rng)]);
                }
            }

            // 2 detail layers for moons, each paired with their normal.
            const std::vector<Ogre::String>& detailPool = this->moonTextures.empty() ? this->planetTextures : this->moonTextures;
            const std::vector<Ogre::String>& detailNormalPool = this->moonNormalTextures.empty() ? this->planetNormalTextures : this->moonNormalTextures;

            if (false == detailPool.empty())
            {
                // Deactitvated detail normal stuff, because no moon brush painting and layers are used!
#if 0
                std::vector<size_t> indices;
                indices.reserve(detailPool.size());
                for (size_t i = 0u; i < detailPool.size(); ++i)
                {
                    indices.push_back(i);
                }
                std::shuffle(indices.begin(), indices.end(), rng);

                const size_t count = detailPool.size();
                const Ogre::String& d0 = detailPool[indices[0u % count]];
                const Ogre::String& d1 = detailPool[indices[1u % count]];

                datablockPbsCompPtr->setDetail0TextureName(d0);
                datablockPbsCompPtr->setDetail1TextureName(d1);

                const Ogre::String n0 = deriveNormalTextureName(d0, detailNormalPool);
                const Ogre::String n1 = deriveNormalTextureName(d1, detailNormalPool);

                if (false == n0.empty())
                {
                    datablockPbsCompPtr->setDetail0NMTextureName(n0);
                }
                if (false == n1.empty())
                {
                    datablockPbsCompPtr->setDetail1NMTextureName(n1);
                }
#else
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL0);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL1);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL2);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL3);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL0_NM);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL1_NM);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL2_NM);
                datablockPbsCompPtr->clearTexture(Ogre::PBSM_DETAIL3_NM);
#endif
            }
        }

        if (true == this->useMotion->getBool())
        {
            GameObjectFactory::getInstance()->createComponent(goPtr, PhysicsArtifactComponent::getStaticClassName(), false);
        }

        // Moons are also gravity sources -- player can land on them.
        goPtr->changeCategory(UniversumComponent::PlanetCategory());

        {
            auto aoiRaw = GameObjectFactory::getInstance()->createComponentDeferred(goPtr, AreaOfInterestComponent::getStaticClassName());
            if (nullptr != aoiRaw)
            {
                aoiRaw->getAttribute("Radius")->setValue(radius * 2.0f);
                aoiRaw->getAttribute("Categories")->setValue(Ogre::String("All"));
                aoiRaw->getAttribute("Update Threshold")->setValue(0.1f);
                aoiRaw->postInit();
                // Attach automatic observer -- owner = UniversumComponent, deleted in destroyUniverse().
                auto aoiComp = NOWA::makeStrongPtr(goPtr->getComponent<AreaOfInterestComponent>());
                if (nullptr != aoiComp)
                {
                    PlanetOrbitObserver* obs = new PlanetOrbitObserver(this, goPtr->getId());
                    this->planetObservers.push_back(obs);
                    aoiComp->attachTriggerObserver(obs);
                }
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Created moon '" + goName + "' radius=" + Ogre::StringConverter::toString(radius) +
                                                                               " gravity=" + Ogre::StringConverter::toString(this->computeGravity(radius)) + " at " + Ogre::StringConverter::toString(initialPosition));

        return goPtr->getId();
    }

    // =========================================================================
    //  Progress event
    // =========================================================================

    void UniversumComponent::postProgressEvent(const Ogre::String& text)
    {
        // EventDataFeedback(bool success, string message) is already used by
        // GameObjectFactory for editor status bar messages -- reuse the same event
        // so DesignState's existing listener displays it automatically.
        boost::shared_ptr<EventDataFeedback> feedbackEvent(boost::make_shared<EventDataFeedback>(true, text));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(feedbackEvent);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] " + text);
    }

    // =========================================================================
    //  Random helpers
    // =========================================================================

    float UniversumComponent::randFloat(std::mt19937& rng, float lo, float hi)
    {
        if (lo >= hi)
        {
            return lo;
        }
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(rng);
    }

    int UniversumComponent::randInt(std::mt19937& rng, int lo, int hi)
    {
        if (lo >= hi)
        {
            return lo;
        }
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng);
    }

    unsigned long UniversumComponent::getInnermostOverlapId() const
    {
        unsigned long bestId = 0ul;
        float smallestRadius = std::numeric_limits<float>::max();

        for (const SolarSystem& system : this->solarSystems)
        {
            for (const OrbitalBody& planet : system.planets)
            {
                if (this->activeTriggerOverlaps.count(planet.gameObjectId))
                {
                    // Use orbitalRadius as a proxy for "how deep" the body is.
                    // Moons have smaller radii than planets so they naturally win.
                    if (planet.orbitalRadius < smallestRadius)
                    {
                        smallestRadius = planet.orbitalRadius;
                        bestId = planet.gameObjectId;
                    }
                }
            }
            for (const auto& moonPair : system.moons)
            {
                const OrbitalBody& moon = moonPair.second;
                if (this->activeTriggerOverlaps.count(moon.gameObjectId))
                {
                    if (moon.orbitalRadius < smallestRadius)
                    {
                        smallestRadius = moon.orbitalRadius;
                        bestId = moon.gameObjectId;
                    }
                }
            }
        }
        return bestId;
    }

    unsigned long UniversumComponent::getCurrentlyPausedBodyId() const
    {
        for (const SolarSystem& system : this->solarSystems)
        {
            for (const OrbitalBody& planet : system.planets)
            {
                if (true == planet.orbitalPaused)
                {
                    return planet.gameObjectId;
                }
            }
            for (const auto& moonPair : system.moons)
            {
                if (true == moonPair.second.orbitalPaused)
                {
                    return moonPair.second.gameObjectId;
                }
            }
        }
        return 0ul;
    }

    void UniversumComponent::handleSceneParsed(EventDataPtr eventData)
    {
        // Does not work, its still to early and design state not ready, but maybe works in direct scenario

        // Fire camera speed event so DesignState sets the editor camera speed correctly,
        // matching what generateUniverse() would have posted.
        // Formula mirrors generateUniverse: max orbital dist * 0.1 / 10s.
        const float scaleFactor = std::pow(10.0f, this->scale->getReal());
        const float approxMaxDist = this->orbitalDistanceMax->getReal() * scaleFactor;
        const float suggestedSpeed = std::max(10.0f, approxMaxDist * 0.1f);
        boost::shared_ptr<EventDataFeedback> speedEvent(boost::make_shared<EventDataFeedback>(true, "[UNIVERSUM_CAMSPEED:" + Ogre::StringConverter::toString(suggestedSpeed) + "]"));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(speedEvent);
    }

    // =========================================================================
    //  Setters / Getters -- all trivial variant setters
    // =========================================================================

    void UniversumComponent::setRandomSeed(unsigned int seed)
    {
        this->randomSeed->setValue(seed);
    }
    unsigned int UniversumComponent::getRandomSeed(void) const
    {
        return this->randomSeed->getUInt();
    }

    void UniversumComponent::setSolarSystemCount(unsigned int count)
    {
        this->solarSystemCount->setValue(count);
    }
    unsigned int UniversumComponent::getSolarSystemCount(void) const
    {
        return this->solarSystemCount->getUInt();
    }

    void UniversumComponent::setSolarSystemDistanceMin(Ogre::Real dist)
    {
        this->solarSystemDistanceMin->setValue(dist);
    }
    Ogre::Real UniversumComponent::getSolarSystemDistanceMin(void) const
    {
        return this->solarSystemDistanceMin->getReal();
    }

    void UniversumComponent::setSolarSystemDistanceMax(Ogre::Real dist)
    {
        this->solarSystemDistanceMax->setValue(dist);
    }
    Ogre::Real UniversumComponent::getSolarSystemDistanceMax(void) const
    {
        return this->solarSystemDistanceMax->getReal();
    }

    void UniversumComponent::setPlanetsPerSystemMin(unsigned int count)
    {
        this->planetsPerSystemMin->setValue(count);
    }
    unsigned int UniversumComponent::getPlanetsPerSystemMin(void) const
    {
        return this->planetsPerSystemMin->getUInt();
    }

    void UniversumComponent::setPlanetsPerSystemMax(unsigned int count)
    {
        this->planetsPerSystemMax->setValue(count);
    }
    unsigned int UniversumComponent::getPlanetsPerSystemMax(void) const
    {
        return this->planetsPerSystemMax->getUInt();
    }

    void UniversumComponent::setUseMoons(bool value)
    {
        this->useMoons->setValue(value);
    }
    bool UniversumComponent::getUseMoons(void) const
    {
        return this->useMoons->getBool();
    }

    void UniversumComponent::setMoonsPerPlanetMin(unsigned int count)
    {
        this->moonsPerPlanetMin->setValue(count);
    }
    unsigned int UniversumComponent::getMoonsPerPlanetMin(void) const
    {
        return this->moonsPerPlanetMin->getUInt();
    }

    void UniversumComponent::setMoonsPerPlanetMax(unsigned int count)
    {
        this->moonsPerPlanetMax->setValue(count);
    }
    unsigned int UniversumComponent::getMoonsPerPlanetMax(void) const
    {
        return this->moonsPerPlanetMax->getUInt();
    }

    void UniversumComponent::setUseMotion(bool value)
    {
        this->useMotion->setValue(value);
    }
    bool UniversumComponent::getUseMotion(void) const
    {
        return this->useMotion->getBool();
    }

    void UniversumComponent::setUseOcean(bool value)
    {
        this->useOcean->setValue(value);
    }
    bool UniversumComponent::getUseOcean(void) const
    {
        return this->useOcean->getBool();
    }

    void UniversumComponent::setOceanProbability(Ogre::Real prob)
    {
        this->oceanProbability->setValue(prob);
    }
    Ogre::Real UniversumComponent::getOceanProbability(void) const
    {
        return this->oceanProbability->getReal();
    }

    void UniversumComponent::setSunRadius(Ogre::Real radius)
    {
        this->sunRadius->setValue(radius);
    }
    Ogre::Real UniversumComponent::getSunRadius(void) const
    {
        return this->sunRadius->getReal();
    }

    void UniversumComponent::setPlanetRadiusMin(Ogre::Real radius)
    {
        this->planetRadiusMin->setValue(radius);
    }
    Ogre::Real UniversumComponent::getPlanetRadiusMin(void) const
    {
        return this->planetRadiusMin->getReal();
    }

    void UniversumComponent::setPlanetRadiusMax(Ogre::Real radius)
    {
        this->planetRadiusMax->setValue(radius);
    }
    Ogre::Real UniversumComponent::getPlanetRadiusMax(void) const
    {
        return this->planetRadiusMax->getReal();
    }

    void UniversumComponent::setMoonRadius(Ogre::Real radius)
    {
        this->moonRadius->setValue(radius);
    }
    Ogre::Real UniversumComponent::getMoonRadius(void) const
    {
        return this->moonRadius->getReal();
    }

    void UniversumComponent::setOrbitalDistanceMin(Ogre::Real dist)
    {
        this->orbitalDistanceMin->setValue(dist);
    }
    Ogre::Real UniversumComponent::getOrbitalDistanceMin(void) const
    {
        return this->orbitalDistanceMin->getReal();
    }

    void UniversumComponent::setOrbitalDistanceMax(Ogre::Real dist)
    {
        this->orbitalDistanceMax->setValue(dist);
    }
    Ogre::Real UniversumComponent::getOrbitalDistanceMax(void) const
    {
        return this->orbitalDistanceMax->getReal();
    }

    void UniversumComponent::setMoonOrbitalDistanceMin(Ogre::Real dist)
    {
        this->moonOrbitalDistanceMin->setValue(dist);
    }
    Ogre::Real UniversumComponent::getMoonOrbitalDistanceMin(void) const
    {
        return this->moonOrbitalDistanceMin->getReal();
    }

    void UniversumComponent::setMoonOrbitalDistanceMax(Ogre::Real dist)
    {
        this->moonOrbitalDistanceMax->setValue(dist);
    }
    Ogre::Real UniversumComponent::getMoonOrbitalDistanceMax(void) const
    {
        return this->moonOrbitalDistanceMax->getReal();
    }

    void UniversumComponent::setOrbitalSpeedMin(Ogre::Real speed)
    {
        this->orbitalSpeedMin->setValue(speed);
    }
    Ogre::Real UniversumComponent::getOrbitalSpeedMin(void) const
    {
        return this->orbitalSpeedMin->getReal();
    }

    void UniversumComponent::setOrbitalSpeedMax(Ogre::Real speed)
    {
        this->orbitalSpeedMax->setValue(speed);
    }
    Ogre::Real UniversumComponent::getOrbitalSpeedMax(void) const
    {
        return this->orbitalSpeedMax->getReal();
    }

    void UniversumComponent::setAxialSpeedMin(Ogre::Real speed)
    {
        this->axialSpeedMin->setValue(speed);
    }
    Ogre::Real UniversumComponent::getAxialSpeedMin(void) const
    {
        return this->axialSpeedMin->getReal();
    }

    void UniversumComponent::setAxialSpeedMax(Ogre::Real speed)
    {
        this->axialSpeedMax->setValue(speed);
    }
    Ogre::Real UniversumComponent::getAxialSpeedMax(void) const
    {
        return this->axialSpeedMax->getReal();
    }

    void UniversumComponent::setPlayerGameObjectId(unsigned long id)
    {
        this->playerGameObjectId->setValue(id);
    }
    unsigned long UniversumComponent::getPlayerGameObjectId(void) const
    {
        return this->playerGameObjectId->getULong();
    }

    void UniversumComponent::setCameraGameObjectId(unsigned long id)
    {
        this->cameraGameObjectId->setValue(id);
    }

    unsigned long UniversumComponent::getCameraGameObjectId(void) const
    {
        return this->cameraGameObjectId->getULong();
    }

    void UniversumComponent::setSunLightGameObjectId(unsigned long id)
    {
        this->sunLightGameObjectId->setValue(id);

        // Re-cache the raw light pointer immediately when the id changes at runtime.
        this->cachedSunLight = nullptr;
        GameObjectPtr lightGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
        if (nullptr != lightGo)
        {
            auto lightComp = NOWA::makeStrongPtr(lightGo->getComponent<LightDirectionalComponent>());
            if (nullptr != lightComp)
            {
                this->cachedSunLight = lightComp->getOgreLight();
            }
        }
    }

    unsigned long UniversumComponent::getSunLightGameObjectId(void) const
    {
        return this->sunLightGameObjectId->getULong();
    }

    void UniversumComponent::setFarClipSpace(Ogre::Real distance)
    {
        this->farClipSpace->setValue(distance);
    }
    Ogre::Real UniversumComponent::getFarClipSpace(void) const
    {
        return this->farClipSpace->getReal();
    }

    void UniversumComponent::setFarClipSurface(Ogre::Real distance)
    {
        this->farClipSurface->setValue(distance);
    }
    Ogre::Real UniversumComponent::getFarClipSurface(void) const
    {
        return this->farClipSurface->getReal();
    }

    void UniversumComponent::setFarClipTransitionSpeed(Ogre::Real speed)
    {
        this->farClipTransitionSpeed->setValue(speed);
    }
    Ogre::Real UniversumComponent::getFarClipTransitionSpeed(void) const
    {
        return this->farClipTransitionSpeed->getReal();
    }

    void UniversumComponent::setScale(Ogre::Real scale)
    {
        this->scale->setValue(scale);
    }

    Ogre::Real UniversumComponent::getScale(void) const
    {
        return this->scale->getReal();
    }

    void UniversumComponent::setAutoPauseOrbit(bool autoPause)
    {
        this->autoPauseOrbit->setValue(autoPause);
    }

    bool UniversumComponent::getAutoPauseOrbit(void) const
    {
        return this->autoPauseOrbit->getBool();
    }

    // =========================================================================
    //  Lua delegation -- react callbacks + call dispatchers
    // =========================================================================

    void UniversumComponent::reactOnPlanetEntered(luabind::object closureFunction)
    {
        this->planetEnteredClosureFunction = closureFunction;
    }

    void UniversumComponent::reactOnPlanetLeft(luabind::object closureFunction)
    {
        this->planetLeftClosureFunction = closureFunction;
    }

    void UniversumComponent::reactOnUniverseGenerated(luabind::object closureFunction)
    {
        this->universeGeneratedClosureFunction = closureFunction;
    }

    void UniversumComponent::reactOnLanding(luabind::object closureFunction)
    {
        this->landingClosureFunction = closureFunction;
    }

    void UniversumComponent::reactOnLanded(luabind::object closureFunction)
    {
        this->landedClosureFunction = closureFunction;
    }

    void UniversumComponent::callPlanetEnteredFunction(unsigned long planetId, unsigned long enteringGoId)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->planetEnteredClosureFunction.is_valid())
        {
            return;
        }
        NOWA::AppStateManager::LogicCommand logicCommand = [this, planetId, enteringGoId]()
        {
            try
            {
                auto gameObjectController = AppStateManager::getSingletonPtr()->getGameObjectController();

                auto planetGameObjectPtr = gameObjectController->getGameObjectFromId(planetId);
                auto enteringGameObjectPtr = gameObjectController->getGameObjectFromId(enteringGoId);

                GameObject* planetGameObject = nullptr;
                GameObject* enteringGameObject = nullptr;

                if (nullptr != planetGameObjectPtr)
                {
                    planetGameObject = planetGameObjectPtr.get();
                }

                if (nullptr != enteringGameObjectPtr)
                {
                    enteringGameObject = enteringGameObjectPtr.get();
                }

                luabind::call_function<void>(this->planetEnteredClosureFunction, planetGameObject, enteringGameObject);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Lua error in reactOnPlanetEntered: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    void UniversumComponent::callPlanetLeftFunction(unsigned long planetId, unsigned long enteringGoId)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->planetLeftClosureFunction.is_valid())
        {
            return;
        }
        NOWA::AppStateManager::LogicCommand logicCommand = [this, planetId, enteringGoId]()
        {
            try
            {
                auto gameObjectController = AppStateManager::getSingletonPtr()->getGameObjectController();

                auto planetGameObjectPtr = gameObjectController->getGameObjectFromId(planetId);
                auto enteringGameObjectPtr = gameObjectController->getGameObjectFromId(enteringGoId);

                GameObject* planetGameObject = nullptr;
                GameObject* enteringGameObject = nullptr;

                if (nullptr != planetGameObjectPtr)
                {
                    planetGameObject = planetGameObjectPtr.get();
                }

                if (nullptr != enteringGameObjectPtr)
                {
                    enteringGameObject = enteringGameObjectPtr.get();
                }

                luabind::call_function<void>(this->planetLeftClosureFunction, planetGameObject, enteringGameObject);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Lua error in reactOnPlanetLeft: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    void UniversumComponent::callUniverseGeneratedFunction(void)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->universeGeneratedClosureFunction.is_valid())
        {
            return;
        }
        NOWA::AppStateManager::LogicCommand logicCommand = [this]()
        {
            try
            {
                luabind::call_function<void>(this->universeGeneratedClosureFunction);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[UniversumComponent] Lua error in reactOnUniverseGenerated: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    UniversumComponent* getUniversumComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<UniversumComponent>(gameObject->getComponentWithOccurrence<UniversumComponent>(occurrenceIndex)).get();
    }

    UniversumComponent* getUniversumComponent(GameObject* gameObject)
    {
        return makeStrongPtr<UniversumComponent>(gameObject->getComponent<UniversumComponent>()).get();
    }

    UniversumComponent* getUniversumComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<UniversumComponent>(gameObject->getComponentFromName<UniversumComponent>(name)).get();
    }

    void internalSetPlayerGameObjectId(UniversumComponent* instance, const Ogre::String& gameObjectId)
    {
        instance->setPlayerGameObjectId(Ogre::StringConverter::parseUnsignedLong(gameObjectId));
    }

    Ogre::String internalGetPlayerGameObjectId(UniversumComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getPlayerGameObjectId());
    }

    void internalSetCameraGameObjectId(UniversumComponent* instance, const Ogre::String& gameObjectId)
    {
        instance->setCameraGameObjectId(Ogre::StringConverter::parseUnsignedLong(gameObjectId));
    }

    Ogre::String internalGetCameraGameObjectId(UniversumComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getCameraGameObjectId());
    }

    void internalSetSunLightGameObjectId(UniversumComponent* instance, const Ogre::String& gameObjectId)
    {
        instance->setSunLightGameObjectId(Ogre::StringConverter::parseUnsignedLong(gameObjectId));
    }

    Ogre::String internalGetSunLightGameObjectId(UniversumComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getSunLightGameObjectId());
    }

    void UniversumComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[luabind::class_<UniversumComponent, GameObjectComponent>("UniversumComponent")
                .def("setRandomSeed", &UniversumComponent::setRandomSeed)
                .def("getRandomSeed", &UniversumComponent::getRandomSeed)
                .def("setSolarSystemCount", &UniversumComponent::setSolarSystemCount)
                .def("getSolarSystemCount", &UniversumComponent::getSolarSystemCount)
                .def("setUseMoons", &UniversumComponent::setUseMoons)
                .def("getUseMoons", &UniversumComponent::getUseMoons)
                .def("setUseMotion", &UniversumComponent::setUseMotion)
                .def("getUseMotion", &UniversumComponent::getUseMotion)
                .def("setUseOcean", &UniversumComponent::setUseOcean)
                .def("getUseOcean", &UniversumComponent::getUseOcean)
                .def("setOceanProbability", &UniversumComponent::setOceanProbability)
                .def("getOceanProbability", &UniversumComponent::getOceanProbability)
                .def("setSunRadius", &UniversumComponent::setSunRadius)
                .def("getSunRadius", &UniversumComponent::getSunRadius)
                .def("setPlanetRadiusMin", &UniversumComponent::setPlanetRadiusMin)
                .def("getPlanetRadiusMin", &UniversumComponent::getPlanetRadiusMin)
                .def("setPlanetRadiusMax", &UniversumComponent::setPlanetRadiusMax)
                .def("getPlanetRadiusMax", &UniversumComponent::getPlanetRadiusMax)
                .def("setPlayerGameObjectId", &internalSetPlayerGameObjectId)
                .def("getPlayerGameObjectId", &internalGetPlayerGameObjectId)
                .def("setCameraGameObjectId", &internalSetCameraGameObjectId)
                .def("getCameraGameObjectId", &internalGetCameraGameObjectId)
                .def("setSunLightGameObjectId", &internalSetSunLightGameObjectId)
                .def("getSunLightGameObjectId", &internalGetSunLightGameObjectId)
                .def("pausePlanetOrbit", &UniversumComponent::pausePlanetOrbit)
                .def("resumePlanetOrbit", &UniversumComponent::resumePlanetOrbit)
                .def("computeGravity", &UniversumComponent::computeGravity)
                .def("setAutoPauseOrbit", &UniversumComponent::setAutoPauseOrbit)
                .def("getAutoPauseOrbit", &UniversumComponent::getAutoPauseOrbit)
                .def("reactOnPlanetEntered", &UniversumComponent::reactOnPlanetEntered)
                .def("reactOnPlanetLeft", &UniversumComponent::reactOnPlanetLeft)
                .def("reactOnLanding", &UniversumComponent::reactOnLanding)
                .def("reactOnLanded", &UniversumComponent::reactOnLanded)
                .def("requestLanding", &UniversumComponent::requestLanding)
                .def("requestTakeoff", &UniversumComponent::requestTakeoff)

                .def("reactOnUniverseGenerated", &UniversumComponent::reactOnUniverseGenerated)];

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "class inherits GameObjectComponent", UniversumComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setRandomSeed(uint seed)", "Sets the random seed for reproducible universe generation.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "uint getRandomSeed()", "Gets the current random seed.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setSolarSystemCount(uint count)", "Sets the number of solar systems to generate.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setUseMotion(bool enable)", "Enables or disables kinematic orbital motion.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setUseOcean(bool enable)", "Enables or disables ocean generation on planets.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setPlayerGameObjectId(String id)",
            "Sets the player GameObject id (as string -- Lua numbers lose precision for large 64-bit ids). "
            "The id is used by the AOI observer to only trigger planet-pause events when THIS object enters.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "String getPlayerGameObjectId()", "Gets the player GameObject id as string.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setCameraGameObjectId(String id)",
            "Sets the camera GameObject id (as string). The camera's far-clip distance is lerped between farClipSpace and farClipSurface based on proximity to planets.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "String getCameraGameObjectId()", "Gets the camera GameObject id as string.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setSunLightGameObjectId(String id)", "Sets the directional light GameObject id (as string) to steer as the sun. Defaults to MAIN_LIGHT_ID.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "String getSunLightGameObjectId()", "Gets the sun light GameObject id as string.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setAutoPauseOrbit(bool enable)",
            "When enabled, orbital motion pauses automatically when any object enters a planet AreaOfInterest zone, and resumes when it leaves. Day/night is faked via directional light rotation during the pause.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "bool getAutoPauseOrbit()", "Gets whether automatic orbital pause on surface is enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnPlanetEntered(func closure(planetGameObject, enteredGameObject))",
            "Registers a Lua closure called when any object enters a planet or moon atmosphere zone. "
            "Receives the planet GameObject. Wire this in the LuaScriptComponent connect() of the "
            "same GameObject as UniversumComponent. Example: "
            "universum:reactOnPlanetEntered(function(planetGameObject, enteredGameObject) log('Entered ' .. tostring(enteredGameObject:getId())) end)");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnPlanetLeft(func closure(planetGameObject, enteredGameObject))",
            "Registers a Lua closure called when any object leaves a planet or moon atmosphere zone. "
            "Receives the planet GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnLanding(func closure(bodyGameObject, shipGameObject))",
            "Registers a closure called once when the ship is close enough to land "
            "(within landingThreshold, default 30 units above the surface). "
            "Use this to show your Land button. The callback does not lock input. "
            "Call universumComp:requestLanding() from the button handler to start autopilot.");

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnLanded(func closure(bodyGameObject, shipGameObject))", "Registers a closure called once when the ship landed.");

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void requestLanding()",
            "Starts the landing autopilot. "
            "Locks input immediately and drives the ship to the surface. "
            "Fires reactOnLanded when settled. "
            "For testing without a button: call this directly inside reactOnLanding.");

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void requestTakeoff()",
            "Starts the takeoff autopilot. "
            "Locks input immediately and drives the ship to the surface. "
            "For testing without a button: call this directly inside reactOnLanded.");

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnUniverseGenerated(func closure())",
            "Registers a Lua closure called once after generateUniverse() completes successfully. "
            "Use this to read planet ids, set up your player spawn point, configure gravity sources, etc.");

        gameObjectClass.def("getUniversumComponentFromName", &getUniversumComponentFromName);
        gameObjectClass.def("getUniversumComponent", (UniversumComponent * (*)(GameObject*)) & getUniversumComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "UniversumComponent getUniversumComponent()", "Gets the UniversumComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "UniversumComponent getUniversumComponentFromName(String name)", "Gets the UniversumComponent by name.");

        gameObjectControllerClass.def("castUniversumComponent", &GameObjectController::cast<UniversumComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "UniversumComponent castUniversumComponent(UniversumComponent other)", "Casts type for Lua auto completion.");
    }

    std::optional<NOWA::GameObjectTypeDescriptor> UniversumComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = eType::CUSTOM;
        desc.displayName = "Universum";
        desc.meshToDisplay = "Node.mesh";
        desc.needsMeshItem = true;
        desc.forceZeroTransform = false;
        desc.autoComponents = {UniversumComponent::getStaticClassName()};
        return desc;
    }

    bool UniversumComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto thisCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<UniversumComponent>());
        if (nullptr == thisCompPtr)
        {
            return true;
        }
        return false;
    }

} // namespace NOWA