#include "NOWAPrecompiled.h"
#include "UniversumComponent.h"

#include "../../AreaOfInterestComponent/code/AreaOfInterestComponent.h"
#include "../../PlanetOceanComponent/code/PlanetOceanComponent.h"
#include "../../PlanetSunComponent/code/PlanetSunComponent.h"
#include "../../PlanetSurfaceComponent/code/PlanetSurfaceComponent.h"
#include "../../PlanetTerraComponent/code/PlanetTerraComponent.h"
#include "../../ProceduralPlanetComponent/code/ProceduralPlanetComponent.h"
#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "gameobject/DatablockPbsComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/LightDirectionalComponent.h"
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
        orbitalDistanceMin(new Variant(UniversumComponent::AttrOrbitalDistanceMin(), 80.0f, this->attributes)),
        orbitalDistanceMax(new Variant(UniversumComponent::AttrOrbitalDistanceMax(), 400.0f, this->attributes)),
        moonOrbitalDistanceMin(new Variant(UniversumComponent::AttrMoonOrbitalDistanceMin(), 20.0f, this->attributes)),
        moonOrbitalDistanceMax(new Variant(UniversumComponent::AttrMoonOrbitalDistanceMax(), 60.0f, this->attributes)),
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
        fakeLightElapsed(0.0f),
        fakeLightAxialSpeed(0.0f),
        playerOnSurface(false),
        loadedFromFile(false)
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

        this->currentFarClip = this->farClipSpace->getReal();
        this->postInitDone = true;

        // If the universe was loaded from a saved scene, planet GOs already exist in the
        // scene. Re-attach PlanetOrbitObserver to each planet's AreaOfInterestComponent
        // so auto-pause works without the user clicking Generate again.
        if (true == this->loadedFromFile && false == this->solarSystems.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[UniversumComponent] Restored from scene -- re-attaching observers for " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->solarSystems.size())) + " solar system(s).");

            // Fire camera speed event so DesignState sets the editor camera speed correctly,
            // matching what generateUniverse() would have posted.
            // Formula mirrors generateUniverse: max orbital dist * 0.1 / 10s.
            const float scaleFactor = std::pow(10.0f, this->scale->getReal());
            const float approxMaxDist = this->orbitalDistanceMax->getReal() * scaleFactor;
            const float suggestedSpeed = std::max(10.0f, approxMaxDist * 0.1f);
            boost::shared_ptr<EventDataFeedback> speedEvent(boost::make_shared<EventDataFeedback>(true, "[UNIVERSUM_CAMSPEED:" + Ogre::StringConverter::toString(suggestedSpeed) + "]"));
            AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(speedEvent);

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
        }

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
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(shadowCmd), "UniversumComponent::connect::disableShadows");
        }

        // Ensure all planet/moon GOs and their sub-component items (PlanetTerra, PlanetOcean,
        // PlanetSun) are consistently SCENE_STATIC at simulation start. On scene load,
        // sub-component items may default to SCENE_DYNAMIC while the SceneNode is already
        // SCENE_STATIC (saved state), causing "movableobject is static while node is not" crash.
        if (false == this->solarSystems.empty())
        {
            this->registerAndHideSurfaceObjects();
        }

        // All orbiting planets/moons must be SCENE_DYNAMIC so that setPositionOrientation()
        // updates their visual position each frame. SCENE_STATIC nodes require
        // sceneManager->notifyStaticDirty(node) after every move -- the OgreNewt render callback
        // does not call this, so static planets silently never move in the scene.
        // The earlier stutter was from the AreaOfInterest observer firing for all bodies
        // (not just the player) -- that is now fixed by the player-ID filter. setDynamic(true)
        // itself is safe for orbital motion.
        if (false == this->solarSystems.empty())
        {
            for (SolarSystem& system : this->solarSystems)
            {
                for (OrbitalBody& planet : system.planets)
                {
                    GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                    if (nullptr != planetGo && false == planet.orbitalPaused)
                    {
                        this->setPlanetDynamic(planetGo, true);
                    }
                }
                for (auto& moonPair : system.moons)
                {
                    GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moonPair.second.gameObjectId);
                    if (nullptr != moonGo && false == moonPair.second.orbitalPaused)
                    {
                        this->setPlanetDynamic(moonGo, true);
                    }
                }
            }
        }

        return true;
    }

    bool UniversumComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        // Planets/moons were always SCENE_STATIC -- nothing to reset.
        // They were set to SCENE_DYNAMIC in connect() for orbital motion.
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                if (nullptr != planetGo)
                {
                    this->setPlanetDynamic(planetGo, false);
                }
            }
            for (auto& moonPair : system.moons)
            {
                GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moonPair.second.gameObjectId);
                if (nullptr != moonGo)
                {
                    this->setPlanetDynamic(moonGo, false);
                }
            }
        }

        // Restore all surface objects to original world positions and make visible.
        this->restoreAllSurfaceObjects();

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
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(shadowCmd), "UniversumComponent::disconnect::restoreShadows");
            }
        }

        this->fakeLightElapsed = 0.0f;
        this->fakeLightAxialSpeed = 0.0f;
        this->playerOnSurface = false;

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

        return true;
    }

    void UniversumComponent::onRemoveComponent(void)
    {
        this->destroyUniverse();
        GameObjectComponent::onRemoveComponent();
    }

    // =========================================================================
    //  update() -- orbital motion + far-clip adaptation
    // =========================================================================

    void UniversumComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }

        this->elapsedTime += dt;

        // Orbital motion -- kinematic transform update.
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
                    GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                    if (nullptr == planetGo)
                    {
                        continue;
                    }

                    auto physComp = NOWA::makeStrongPtr(planetGo->getComponent<PhysicsArtifactComponent>());
                    if (nullptr != physComp)
                    {
                        if (false == planet.orbitalPaused)
                        {
                            // Normal: orbital translation + axial rotation both run.
                            planet.orbitalElapsed += dt;
                            planet.axialElapsed += dt;

                            float angle = planet.orbitalElapsed * planet.orbitalSpeed + planet.phaseOffset;
                            Ogre::Vector3 newPos(sunPos.x + std::cos(angle) * planet.orbitalRadius, sunPos.y + std::sin(angle * planet.orbitalTilt) * planet.orbitalRadius * 0.1f, sunPos.z + std::sin(angle) * planet.orbitalRadius);
                            planet.currentPosition = newPos;

                            Ogre::Radian axialAngle(planet.axialElapsed * planet.axialSpeed);
                            Ogre::Quaternion axialRot(axialAngle, Ogre::Vector3::UNIT_Y);

                            // Single atomic call: sets both pos+rot, sets m_validToUpdateStatic=true,
                            // and calls updateNode(1.0f) inline -- SceneNode updated immediately
                            // on the main thread without waiting for interalPostUpdate().
                            physComp->getBody()->setPositionOrientation(newPos, axialRot);
                        }
                        // else: planet is fully frozen -- neither position nor orientation
                        // changes. Buildings (TreeCollision) stay valid. Day/night is
                        // faked via the directional light below.
                    }
                }

                for (auto& moonPair : system.moons)
                {
                    const size_t parentIdx = moonPair.first;
                    OrbitalBody& moon = moonPair.second;

                    if (parentIdx >= system.planets.size())
                    {
                        continue;
                    }

                    // Moons ALWAYS keep orbiting regardless of whether their parent planet
                    // is paused -- player on the surface should see moons arc across the sky.
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
                    if (nullptr != physComp)
                    {
                        Ogre::Radian axialAngle(moon.axialElapsed * moon.axialSpeed);
                        Ogre::Quaternion axialRot(axialAngle, Ogre::Vector3::UNIT_Y);
                        physComp->getBody()->setPositionOrientation(newPos, axialRot);
                    }
                }
            }

            // Advance fake light timer only when player is on a surface.
            if (true == this->playerOnSurface)
            {
                this->fakeLightElapsed += dt;
            }
        }

        // Far-clip adaptation.
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

                auto camComp = NOWA::makeStrongPtr(cameraGo->getComponent<CameraComponent>());
                if (nullptr != camComp)
                {
                    // setFarClipDistance handles render thread dispatch internally.
                    // Never call getCamera()->setFarClipDistance() directly -- all
                    // Ogre write operations must happen on the render thread.
                    camComp->setFarClipDistance(this->currentFarClip);
                }
            }
        }

        // Sun light steering -- only the nearest solar system drives the scene directional light.
        if (nullptr != this->cachedSunLight && 0ul != this->cameraGameObjectId->getULong())
        {
            GameObjectPtr cameraGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraGameObjectId->getULong());

            if (nullptr != cameraGo)
            {
                Ogre::Vector3 camPos = cameraGo->getPosition();

                float nearestDist = std::numeric_limits<float>::max();
                SolarSystem* nearestSystem = nullptr;

                for (SolarSystem& system : this->solarSystems)
                {
                    float dist = (system.position - camPos).length();
                    if (dist < nearestDist)
                    {
                        nearestDist = dist;
                        nearestSystem = &system;
                    }
                }

                if (nullptr != nearestSystem)
                {
                    GameObjectPtr sunGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(nearestSystem->sunId);

                    if (nullptr != sunGo)
                    {
                        this->currentLightColor = this->currentLightColor + (nearestSystem->sunLightColor - this->currentLightColor) * dt * 0.5f;
                        this->currentLightPower = this->currentLightPower + (nearestSystem->sunLightPower - this->currentLightPower) * dt * 0.5f;

                        Ogre::Vector3 dir;

                        if (false == this->playerOnSurface)
                        {
                            // SPACE MODE: real direction from actual sun position toward camera.
                            dir = camPos - sunGo->getPosition();
                        }
                        else
                        {
                            // SURFACE MODE: universe is frozen. Rotate the light around the
                            // planet's up axis (Y) at the captured axial speed to fake the
                            // day/night cycle. The player sees the sun arc across the sky as
                            // if the planet were spinning, but nothing physically moves.
                            //
                            // Base direction: sun-to-planet (frozen sun position).
                            Ogre::Vector3 baseDir = camPos - sunGo->getPosition();
                            if (baseDir.squaredLength() > 0.0f)
                            {
                                baseDir.normalise();
                                // Rotate base direction around world Y at axialSpeed.
                                Ogre::Radian fakeAngle(this->fakeLightElapsed * this->fakeLightAxialSpeed);
                                Ogre::Quaternion fakeRot(fakeAngle, Ogre::Vector3::UNIT_Y);
                                dir = fakeRot * baseDir;
                            }
                            else
                            {
                                dir = Ogre::Vector3::UNIT_Y;
                            }

                            // Moon eclipse: if a moon is between the sun and the player,
                            // dim the light analytically. This replaces what a shadow map
                            // would do at orbital scale -- at these distances the penumbra
                            // is enormous and a smooth multiplier is correct and cheap.
                            float eclipseFactor = 1.0f;
                            Ogre::Vector3 toSun = -dir; // dir is planet-to-light
                            for (const auto& moonPair : nearestSystem->moons)
                            {
                                const OrbitalBody& moon = moonPair.second;
                                Ogre::Vector3 toMoon = (moon.currentPosition - camPos);
                                float moonDist = toMoon.length();
                                if (moonDist > 0.0f)
                                {
                                    toMoon /= moonDist;
                                    float alignment = toSun.dotProduct(toMoon);
                                    if (alignment > 0.9980f)
                                    {
                                        // Moon is close to sun direction from player.
                                        // Use angular size proxy to estimate coverage.
                                        // gravityStrength ~ 19.8 * (radius/50) so radius = gravityStrength * 50 / 19.8
                                        float moonRadius = moon.gravityStrength * 50.0f / 19.8f;
                                        float angularSize = moonRadius / std::max(moonDist, 1.0f);
                                        float coverage = Ogre::Math::Clamp(angularSize * 80.0f, 0.0f, 1.0f);
                                        float thisEclipse = 1.0f - coverage * 0.9f;
                                        if (thisEclipse < eclipseFactor)
                                        {
                                            eclipseFactor = thisEclipse;
                                        }
                                    }
                                }
                            }
                            this->currentLightPower *= eclipseFactor;
                        }

                        if (dir.squaredLength() > 0.0f)
                        {
                            dir.normalise();

                            Ogre::Light* light = this->cachedSunLight;
                            Ogre::Vector3 color = this->currentLightColor;
                            float power = this->currentLightPower;

                            auto closureFunction = [light, dir, color, power](Ogre::Real renderDt)
                            {
                                light->setDirection(dir);
                                light->setDiffuseColour(color.x, color.y, color.z);
                                light->setPowerScale(power);
                                light->_getManager()->setAmbientLight(light->_getManager()->getAmbientLightUpperHemisphere(), light->_getManager()->getAmbientLightLowerHemisphere(), -dir);
                            };

                            Ogre::String closureId = this->gameObjectPtr->getName() + "::sunLight";
                            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closureFunction, false);
                        }
                    }
                }
            }
        }
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

    void UniversumComponent::pausePlanetOrbit(unsigned long planetGameObjectId)
    {
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                if (planet.gameObjectId == planetGameObjectId)
                {
                    planet.orbitalPaused = true;
                    this->playerOnSurface = true;
                    this->fakeLightAxialSpeed = planet.axialSpeed;
                    this->fakeLightElapsed = planet.axialElapsed;
                    // Planet was already SCENE_STATIC (we never called setDynamic(true)).
                    // setDynamic(false) is a no-op here but kept for clarity -- planet is
                    // confirmed static and collision is fully active.
                    GameObjectPtr frozenGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                    if (nullptr != frozenGo)
                    {
                        this->setPlanetDynamic(frozenGo, false);
                    }
                    // Snap far clip immediately -- PSSM split points recalculate from
                    // the camera near/far each frame. Skipping the lerp prevents a
                    // several-second window of broken shadow resolution while the lerp
                    // transitions from farClipSpace (1,000,000) to farClipSurface (5000).
                    this->currentFarClip = this->farClipSurface->getReal();
                    // Enable shadow casting -- surface mode has tight PSSM coverage.
                    if (nullptr != this->cachedSunLight)
                    {
                        Ogre::Light* light = this->cachedSunLight;
                        NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
                        {
                            light->setCastShadows(true);
                        };
                        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(shadowCmd), "UniversumComponent::pausePlanetOrbit::enableShadows");
                    }

                    // Tighten PSSM lambda for surface mode.
                    // ESM shadow nodes default to lambda=0.70 (even split, bad for close shadows).
                    // PCF nodes default to 0.95 which is already good.
                    // Boosting to 0.95 pushes cascade 0 from ~170m to ~13m -- tight for player/building shadows.
                    // updateShadowGlobalBias() modifies the CompositorShadowNodeDef -- effective next frame.
                    {
                        const Ogre::String& shadowNodeName = WorkspaceModule::getInstance()->shadowNodeName;
                        Ogre::CompositorShadowNodeDef* shadowDef = Ogre::Root::getSingleton().getCompositorManager2()->getShadowNodeDefinitionNonConst(shadowNodeName);
                        if (nullptr != shadowDef)
                        {
                            const size_t numDefs = shadowDef->getNumShadowTextureDefinitions();
                            for (size_t si = 0u; si < numDefs; ++si)
                            {
                                Ogre::ShadowTextureDefinition* texDef = shadowDef->getShadowTextureDefinitionNonConst(si);
                                texDef->pssmLambda = 0.95f;
                            }
                        }
                    }
                    // Show and position all surface objects now that this planet is frozen.
                    this->showSurfaceObjects(planet);
                    this->callPlanetEnteredFunction(planetGameObjectId);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[UniversumComponent] Orbital motion paused for planet id=" + Ogre::StringConverter::toString(planetGameObjectId) + " -- fake day/night active at axialSpeed=" + Ogre::StringConverter::toString(this->fakeLightAxialSpeed));
                    return;
                }
            }
            for (auto& moonPair : system.moons)
            {
                if (moonPair.second.gameObjectId == planetGameObjectId)
                {
                    moonPair.second.orbitalPaused = true;
                    this->playerOnSurface = true;
                    this->fakeLightAxialSpeed = moonPair.second.axialSpeed;
                    this->fakeLightElapsed = moonPair.second.axialElapsed;
                    GameObjectPtr frozenMoonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                    if (nullptr != frozenMoonGo)
                    {
                        this->setPlanetDynamic(frozenMoonGo, false);
                    }
                    this->currentFarClip = this->farClipSurface->getReal();
                    if (nullptr != this->cachedSunLight)
                    {
                        Ogre::Light* light = this->cachedSunLight;
                        NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
                        {
                            light->setCastShadows(true);
                        };
                        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(shadowCmd), "UniversumComponent::pausePlanetOrbit::enableShadows");
                    }

                    // Tighten PSSM lambda for surface mode.
                    // ESM shadow nodes default to lambda=0.70 (even split, bad for close shadows).
                    // PCF nodes default to 0.95 which is already good.
                    // Boosting to 0.95 pushes cascade 0 from ~170m to ~13m -- tight for player/building shadows.
                    // updateShadowGlobalBias() modifies the CompositorShadowNodeDef -- effective next frame.
                    {
                        const Ogre::String& shadowNodeName = WorkspaceModule::getInstance()->shadowNodeName;
                        Ogre::CompositorShadowNodeDef* shadowDef = Ogre::Root::getSingleton().getCompositorManager2()->getShadowNodeDefinitionNonConst(shadowNodeName);
                        if (nullptr != shadowDef)
                        {
                            const size_t numDefs = shadowDef->getNumShadowTextureDefinitions();
                            for (size_t si = 0u; si < numDefs; ++si)
                            {
                                Ogre::ShadowTextureDefinition* texDef = shadowDef->getShadowTextureDefinitionNonConst(si);
                                texDef->pssmLambda = 0.95f;
                            }
                        }
                    }
                    this->showSurfaceObjects(moonPair.second);
                    this->callPlanetEnteredFunction(planetGameObjectId);
                    return;
                }
            }
        }
    }

    void UniversumComponent::resumePlanetOrbit(unsigned long planetGameObjectId)
    {
        for (SolarSystem& system : this->solarSystems)
        {
            for (OrbitalBody& planet : system.planets)
            {
                if (planet.gameObjectId == planetGameObjectId)
                {
                    planet.orbitalPaused = false;
                    this->hideSurfaceObjects(planet);
                    this->callPlanetLeftFunction(planetGameObjectId);

                    // Re-enable dynamic so setPositionOrientation() moves the SceneNode again.
                    GameObjectPtr resumedGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                    if (nullptr != resumedGo)
                    {
                        this->setPlanetDynamic(resumedGo, true);
                    }

                    // Check if any other body is still paused before clearing playerOnSurface.
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
                        // Truly in space again -- disable shadows and snap far clip.
                        this->currentFarClip = this->farClipSpace->getReal();
                        if (nullptr != this->cachedSunLight)
                        {
                            Ogre::Light* light = this->cachedSunLight;
                            NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
                            {
                                light->setCastShadows(false);
                            };
                            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(shadowCmd), "UniversumComponent::resumePlanetOrbit::disableShadows");
                        }

                        // Restore PSSM lambda to compositor defaults when back in space (shadows off).
                        {
                            const Ogre::String& shadowNodeName = WorkspaceModule::getInstance()->shadowNodeName;
                            Ogre::CompositorShadowNodeDef* shadowDef = Ogre::Root::getSingleton().getCompositorManager2()->getShadowNodeDefinitionNonConst(shadowNodeName);
                            if (nullptr != shadowDef)
                            {
                                const bool isEsm = (shadowNodeName.find("ESM") != Ogre::String::npos);
                                const float defaultLambda = isEsm ? 0.70f : 0.95f;
                                const size_t numDefs = shadowDef->getNumShadowTextureDefinitions();
                                for (size_t si = 0u; si < numDefs; ++si)
                                {
                                    Ogre::ShadowTextureDefinition* texDef = shadowDef->getShadowTextureDefinitionNonConst(si);
                                    texDef->pssmLambda = defaultLambda;
                                }
                            }
                        }
                    }

                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Orbital motion resumed for planet id=" + Ogre::StringConverter::toString(planetGameObjectId));
                    return;
                }
            }
            for (auto& moonPair : system.moons)
            {
                if (moonPair.second.gameObjectId == planetGameObjectId)
                {
                    moonPair.second.orbitalPaused = false;
                    this->hideSurfaceObjects(moonPair.second);
                    this->callPlanetLeftFunction(planetGameObjectId);

                    GameObjectPtr resumedMoonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planetGameObjectId);
                    if (nullptr != resumedMoonGo)
                    {
                        this->setPlanetDynamic(resumedMoonGo, true);
                    }

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
                        this->currentFarClip = this->farClipSpace->getReal();
                        if (nullptr != this->cachedSunLight)
                        {
                            Ogre::Light* light = this->cachedSunLight;
                            NOWA::GraphicsModule::RenderCommand shadowCmd = [light]
                            {
                                light->setCastShadows(false);
                            };
                            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(shadowCmd), "UniversumComponent::resumePlanetOrbit::disableShadows");
                        }
                    }
                    return;
                }
            }
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
                        }
                        break;
                    }
                }
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Surface objects registered and hidden.");
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
                    physComp->getBody()->setAutoSleep(0);
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
                physComp->getBody()->setAutoSleep(1);
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
                            physComp->getBody()->setAutoSleep(0);
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
                            physComp->getBody()->setAutoSleep(0);
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
        // ---- Step 1: null out AreaOfInterestComponent observer pointers ----
        // PlanetOrbitObserver objects are owned by UniversumComponent and deleted below.
        // AreaOfInterestComponent stores a raw pointer to them. If we delete the observers
        // while AoI still holds the pointer, any later setActivated(false) call (e.g. at
        // engine exit via destroyContent()) will dereference a dangling pointer and crash.
        // Calling attachTriggerObserver(nullptr) is safe -- it just assigns nullptr;
        // it does NOT delete the old observer since AoI only deletes in onRemoveComponent.
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

        // ---- Step 2: delete observer objects (AoI no longer holds the pointer) ----
        for (PlanetOrbitObserver* obs : this->planetObservers)
        {
            delete obs;
        }
        this->planetObservers.clear();

        // Now destroy all owned game objects.
        for (const unsigned long id : this->ownedGameObjectIds)
        {
            AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(id);
        }
        this->ownedGameObjectIds.clear();
        this->solarSystems.clear();
        this->elapsedTime = 0.0f;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Universe destroyed.");
    }

    void UniversumComponent::generateUniverse(void)
    {
        this->destroyUniverse();

        std::mt19937 rng(static_cast<unsigned int>(this->randomSeed->getUInt()));

        // Build texture pools from the TerrainTextures resource group.
        this->buildTexturePools();

        // Scale multiplier: each unit of scale multiplies all radii and distances by 10.
        // scale=1 -> factor=10, scale=2 -> factor=100, scale=0.5 -> factor~3.16
        const float scaleFactor = std::pow(10.0f, this->scale->getReal());

        const unsigned int systemCount = this->solarSystemCount->getUInt();
        this->postProgressEvent("Generating Universe: 0%");

        // Place solar systems in a spiral pattern to avoid overlap.
        const float distMin = this->solarSystemDistanceMin->getReal();
        const float distMax = this->solarSystemDistanceMax->getReal();

        for (unsigned int s = 0u; s < systemCount; ++s)
        {
            float progress = (systemCount > 1u) ? static_cast<float>(s) / static_cast<float>(systemCount - 1u) : 1.0f;

            Ogre::String progressText = "Generating Universe: " + Ogre::StringConverter::toString(static_cast<int>(progress * 100.0f)) + "%";
            this->postProgressEvent(progressText);

            // Spiral layout: each system is placed on an Archimedean spiral.
            float angle = static_cast<float>(s) * 2.399f; // golden angle approx
            float radDist = this->randFloat(rng, distMin, distMax) * (static_cast<float>(s) + 1.0f);
            Ogre::Vector3 systemPos(std::cos(angle) * radDist, this->randFloat(rng, -distMin * 0.1f, distMin * 0.1f), std::sin(angle) * radDist);

            this->generateSolarSystem(s, systemPos, scaleFactor, rng);
        }

        this->postProgressEvent("Universe ready: " + Ogre::StringConverter::toString(systemCount) + " solar system(s), " + Ogre::StringConverter::toString(static_cast<unsigned int>(this->ownedGameObjectIds.size())) + " bodies total.");

        // Suggest a camera move speed appropriate for this universe scale.
        // Target: camera crosses the max orbital distance in ~10 seconds.
        const float approxMaxDist = this->orbitalDistanceMax->getReal() * scaleFactor;
        const float suggestedSpeed = std::max(10.0f, approxMaxDist * 0.1f);

        // Post as structured feedback so DesignState can parse and apply it automatically.
        // Format: "[UNIVERSUM_CAMSPEED:value]"
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
                aoiRaw->getAttribute("Update Threshold")->setValue(0.5f);
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
            terraRaw->getAttribute(PlanetTerraComponent::AttrRadius())->setValue(radius);

            // Moons are small -- reduce UV tiling so texture isn't over-repeated.
            // Default is 128 (designed for large planets). Use radius/4 clamped to [8, 32].
            const float moonUVScale = std::max(8.0f, std::min(32.0f, radius * 0.25f));
            terraRaw->getAttribute(PlanetTerraComponent::AttrBaseUVScale())->setValue(moonUVScale);

            terraRaw->postInit();

            // Set immediately while terra item is still the movableObject.
            goPtr->setRenderDistance(1000000u);
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
                aoiRaw->getAttribute("Update Threshold")->setValue(0.5f);
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

    void UniversumComponent::callPlanetEnteredFunction(unsigned long planetId)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->planetEnteredClosureFunction.is_valid())
        {
            return;
        }
        NOWA::AppStateManager::LogicCommand logicCommand = [this, planetId]()
        {
            try
            {
                luabind::call_function<void>(this->planetEnteredClosureFunction, planetId);
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

    void UniversumComponent::callPlanetLeftFunction(unsigned long planetId)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->planetLeftClosureFunction.is_valid())
        {
            return;
        }
        NOWA::AppStateManager::LogicCommand logicCommand = [this, planetId]()
        {
            try
            {
                luabind::call_function<void>(this->planetLeftClosureFunction, planetId);
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

    void UniversumComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<UniversumComponent, GameObjectComponent>("UniversumComponent")
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
                .def("setPlayerGameObjectId", &UniversumComponent::setPlayerGameObjectId)
                .def("getPlayerGameObjectId", &UniversumComponent::getPlayerGameObjectId)
                .def("setCameraGameObjectId", &UniversumComponent::setCameraGameObjectId)
                .def("getCameraGameObjectId", &UniversumComponent::getCameraGameObjectId)
                .def("pausePlanetOrbit", &UniversumComponent::pausePlanetOrbit)
                .def("resumePlanetOrbit", &UniversumComponent::resumePlanetOrbit)
                .def("computeGravity", &UniversumComponent::computeGravity)
                .def("setAutoPauseOrbit", &UniversumComponent::setAutoPauseOrbit)
                .def("getAutoPauseOrbit", &UniversumComponent::getAutoPauseOrbit)
                .def("reactOnPlanetEntered", &UniversumComponent::reactOnPlanetEntered)
                .def("reactOnPlanetLeft", &UniversumComponent::reactOnPlanetLeft)
                .def("reactOnUniverseGenerated", &UniversumComponent::reactOnUniverseGenerated)];

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "class inherits GameObjectComponent", UniversumComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setRandomSeed(uint seed)", "Sets the random seed for reproducible universe generation.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "uint getRandomSeed()", "Gets the current random seed.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setSolarSystemCount(uint count)", "Sets the number of solar systems to generate.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setUseMotion(bool enable)", "Enables or disables kinematic orbital motion.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setUseOcean(bool enable)", "Enables or disables ocean generation on planets.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setPlayerGameObjectId(uint id)", "Sets the player GameObject Id for far-clip adaptation.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setCameraGameObjectId(uint id)", "Sets the camera GameObject Id whose far-clip is adapted.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setAutoPauseOrbit(bool enable)",
            "When enabled, orbital motion pauses automatically when any object enters a planet AreaOfInterest zone, and resumes when it leaves. Day/night is faked via directional light rotation during the pause.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "bool getAutoPauseOrbit()", "Gets whether automatic orbital pause on surface is enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnPlanetEntered(func closure(planetId))",
            "Registers a Lua closure called when any object enters a planet or moon atmosphere zone. "
            "Receives the planet GameObject Id. Wire this in the LuaScriptComponent connect() of the "
            "same GameObject as UniversumComponent. Example: "
            "universum:reactOnPlanetEntered(function(planetId) log('Entered ' .. tostring(planetId)) end)");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void reactOnPlanetLeft(func closure(planetId))",
            "Registers a Lua closure called when any object leaves a planet or moon atmosphere zone. "
            "Receives the planet GameObject Id.");
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