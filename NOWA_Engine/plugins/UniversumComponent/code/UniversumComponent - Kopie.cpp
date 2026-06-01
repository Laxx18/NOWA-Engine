#include "NOWAPrecompiled.h"
#include "UniversumComponent.h"

#include "../../PlanetOceanComponent/code/PlanetOceanComponent.h"
#include "../../PlanetSunComponent/code/PlanetSunComponent.h"
#include "../../PlanetTerraComponent/code/PlanetTerraComponent.h"
#include "../../ProceduralPlanetComponent/code/ProceduralPlanetComponent.h"
#include "gameobject/DatablockPbsComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsActiveKinematicComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreCamera.h"

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
        farClipSpace(new Variant(UniversumComponent::AttrFarClipSpace(), 1000000.0f, this->attributes)),
        farClipSurface(new Variant(UniversumComponent::AttrFarClipSurface(), 5000.0f, this->attributes)),
        farClipTransitionSpeed(new Variant(UniversumComponent::AttrFarClipTransitionSpeed(), 0.5f, this->attributes)),
        scale(new Variant(UniversumComponent::AttrScale(), 1.0f, this->attributes)),
        generate(new Variant(UniversumComponent::AttrGenerate(), Ogre::String("Generate"), this->attributes)),
        elapsedTime(0.0f),
        currentFarClip(1000000.0f),
        postInitDone(false)
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
        this->useMotion->setDescription("Enable kinematic orbital motion. Planets and moons orbit their parents via PhysicsActiveKinematicComponent.");
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
        this->farClipSpace->setDescription("Camera far clip distance when the player is in open space.");
        this->farClipSurface->setDescription("Camera far clip distance when the player is on or near a planet surface.");
        this->farClipTransitionSpeed->setDescription("Lerp speed for far clip transitions between space and surface values.");
        this->scale->setDescription("Universe size scale. 1 = smallest units are ~10m. Each step multiplies all radii and distances by 10. "
                                    "scale=1: sun~25m, planets 8-30m. scale=2: sun~250m, planets 80-300m.");
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

        return success;
    }

    bool UniversumComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Init component for game object: " + this->gameObjectPtr->getName());

        this->currentFarClip = this->farClipSpace->getReal();

        this->postInitDone = true;
        return true;
    }

    bool UniversumComponent::connect(void)
    {
        GameObjectComponent::connect();
        // Reset elapsed time so orbital positions start from phase offsets on each simulation start.
        // Without this, bodies jump to elapsedTime * speed on restart instead of their spawn position.
        this->elapsedTime = 0.0f;
        return true;
    }

    bool UniversumComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
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

                // Orbital motion for kinematic bodies: use setPosition/setOrientation directly.
                // These are correct for kinematic bodies -- Newton designed them to be moved this way.
                // The "don't use setPosition" rule applies to DYNAMIC bodies only.
                // Velocity-based approach crashes at orbital scale because Newton's kinematic body
                // has a hard limit of 1 unit/step (~144 units/s at 144fps). Orbital velocities
                // (radius * angularSpeed) easily exceed 600 units/s at universe scale.
                for (OrbitalBody& planet : system.planets)
                {
                    float angle = this->elapsedTime * planet.orbitalSpeed + planet.phaseOffset;
                    Ogre::Vector3 newPos(sunPos.x + std::cos(angle) * planet.orbitalRadius, sunPos.y + std::sin(angle * planet.orbitalTilt) * planet.orbitalRadius * 0.1f, sunPos.z + std::sin(angle) * planet.orbitalRadius);

                    planet.currentPosition = newPos;

                    GameObjectPtr planetGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(planet.gameObjectId);
                    if (nullptr == planetGo)
                    {
                        continue;
                    }

                    auto physComp = NOWA::makeStrongPtr(planetGo->getComponent<PhysicsActiveKinematicComponent>());
                    if (nullptr != physComp)
                    {
                        physComp->setPosition(newPos);
                        Ogre::Radian axialAngle(this->elapsedTime * planet.axialSpeed);
                        Ogre::Quaternion axialRot(axialAngle, Ogre::Vector3::UNIT_Y);
                        physComp->setOrientation(axialRot);
                    }
                }

                // Update moon positions relative to their parent planet.
                for (auto& moonPair : system.moons)
                {
                    const size_t parentIdx = moonPair.first;
                    OrbitalBody& moon = moonPair.second;

                    if (parentIdx >= system.planets.size())
                    {
                        continue;
                    }

                    Ogre::Vector3 parentPos = system.planets[parentIdx].currentPosition;
                    float angle = this->elapsedTime * moon.orbitalSpeed + moon.phaseOffset;
                    Ogre::Vector3 newPos(parentPos.x + std::cos(angle) * moon.orbitalRadius, parentPos.y + std::sin(angle * moon.orbitalTilt) * moon.orbitalRadius * 0.1f, parentPos.z + std::sin(angle) * moon.orbitalRadius);

                    moon.currentPosition = newPos;

                    GameObjectPtr moonGo = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(moon.gameObjectId);
                    if (nullptr == moonGo)
                    {
                        continue;
                    }

                    auto physComp = NOWA::makeStrongPtr(moonGo->getComponent<PhysicsActiveKinematicComponent>());
                    if (nullptr != physComp)
                    {
                        physComp->setPosition(newPos);
                        Ogre::Radian axialAngle(this->elapsedTime * moon.axialSpeed);
                        Ogre::Quaternion axialRot(axialAngle, Ogre::Vector3::UNIT_Y);
                        physComp->setOrientation(axialRot);
                    }
                }
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
                    camComp->getCamera()->setFarClipDistance(this->currentFarClip);
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
        writeAttr("6", "Far Clip Space", XMLConverter::ConvertString(doc, this->farClipSpace->getReal()));
        writeAttr("6", "Far Clip Surface", XMLConverter::ConvertString(doc, this->farClipSurface->getReal()));
        writeAttr("6", "Far Clip Transition Speed", XMLConverter::ConvertString(doc, this->farClipTransitionSpeed->getReal()));
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr UniversumComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        UniversumCompPtr clonedCompPtr(boost::make_shared<UniversumComponent>());

        clonedCompPtr->randomSeed->setValue(this->randomSeed->getUInt());
        clonedCompPtr->solarSystemCount->setValue(this->solarSystemCount->getUInt());
        clonedCompPtr->solarSystemDistanceMin->setValue(this->solarSystemDistanceMin->getReal());
        clonedCompPtr->solarSystemDistanceMax->setValue(this->solarSystemDistanceMax->getReal());
        clonedCompPtr->planetsPerSystemMin->setValue(this->planetsPerSystemMin->getUInt());
        clonedCompPtr->planetsPerSystemMax->setValue(this->planetsPerSystemMax->getUInt());
        clonedCompPtr->useMoons->setValue(this->useMoons->getBool());
        clonedCompPtr->moonsPerPlanetMin->setValue(this->moonsPerPlanetMin->getUInt());
        clonedCompPtr->moonsPerPlanetMax->setValue(this->moonsPerPlanetMax->getUInt());
        clonedCompPtr->useMotion->setValue(this->useMotion->getBool());
        clonedCompPtr->useOcean->setValue(this->useOcean->getBool());
        clonedCompPtr->oceanProbability->setValue(this->oceanProbability->getReal());
        clonedCompPtr->sunRadius->setValue(this->sunRadius->getReal());
        clonedCompPtr->planetRadiusMin->setValue(this->planetRadiusMin->getReal());
        clonedCompPtr->planetRadiusMax->setValue(this->planetRadiusMax->getReal());
        clonedCompPtr->moonRadius->setValue(this->moonRadius->getReal());
        clonedCompPtr->orbitalDistanceMin->setValue(this->orbitalDistanceMin->getReal());
        clonedCompPtr->orbitalDistanceMax->setValue(this->orbitalDistanceMax->getReal());
        clonedCompPtr->moonOrbitalDistanceMin->setValue(this->moonOrbitalDistanceMin->getReal());
        clonedCompPtr->moonOrbitalDistanceMax->setValue(this->moonOrbitalDistanceMax->getReal());
        clonedCompPtr->orbitalSpeedMin->setValue(this->orbitalSpeedMin->getReal());
        clonedCompPtr->orbitalSpeedMax->setValue(this->orbitalSpeedMax->getReal());
        clonedCompPtr->axialSpeedMin->setValue(this->axialSpeedMin->getReal());
        clonedCompPtr->axialSpeedMax->setValue(this->axialSpeedMax->getReal());
        clonedCompPtr->playerGameObjectId->setValue(this->playerGameObjectId->getULong());
        clonedCompPtr->cameraGameObjectId->setValue(this->cameraGameObjectId->getULong());
        clonedCompPtr->farClipSpace->setValue(this->farClipSpace->getReal());
        clonedCompPtr->farClipSurface->setValue(this->farClipSurface->getReal());
        clonedCompPtr->farClipTransitionSpeed->setValue(this->farClipTransitionSpeed->getReal());
        clonedCompPtr->scale->setValue(this->scale->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
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
        // Delete all owned game objects via the controller.
        // deleteGameObject(id) queues an event -- the controller processes it
        // and calls onRemoveComponent on each component, then frees the object.
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
            GameObjectFactory::getInstance()->createComponent(goPtr, PhysicsActiveKinematicComponent::getStaticClassName(), false);
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
                datablockPbsCompPtr->setDiffuseTextureName(this->planetTextures[pick(rng)]);
            }
            if (false == this->planetNormalTextures.empty())
            {
                std::uniform_int_distribution<size_t> pick(0u, this->planetNormalTextures.size() - 1u);
                datablockPbsCompPtr->setNormalTextureName(this->planetNormalTextures[pick(rng)]);
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
                }
            }
        }

        // Render distance = 0 means infinite.
        if (nullptr != goPtr->getMovableObject())
        {
            // Use goPtr->setRenderDistance to go through the proper enqueue path.
            // Pass a large value -- 0 means "skip" in setRenderDistance.
            goPtr->setRenderDistance(1000000u);
        }

        if (true == this->useMotion->getBool())
        {
            GameObjectFactory::getInstance()->createComponent(goPtr, PhysicsActiveKinematicComponent::getStaticClassName(), false);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Created planet '" + goName + "' radius=" + Ogre::StringConverter::toString(radius) + " at " + Ogre::StringConverter::toString(initialPosition));

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
                datablockPbsCompPtr->setDiffuseTextureName(diffPool[pick(rng)]);
            }
            // Normal: moon normal pool with fallback to planet normal pool.
            const std::vector<Ogre::String>& normalPool = this->moonNormalTextures.empty() ? this->planetNormalTextures : this->moonNormalTextures;
            if (false == normalPool.empty())
            {
                std::uniform_int_distribution<size_t> pick(0u, normalPool.size() - 1u);
                datablockPbsCompPtr->setNormalTextureName(normalPool[pick(rng)]);
            }
        }

        // Render distance = 0 means infinite.
        if (nullptr != goPtr->getMovableObject())
        {
            // Use goPtr->setRenderDistance to go through the proper enqueue path.
            // Pass a large value -- 0 means "skip" in setRenderDistance.
            goPtr->setRenderDistance(1000000u);
        }

        if (true == this->useMotion->getBool())
        {
            GameObjectFactory::getInstance()->createComponent(goPtr, PhysicsActiveKinematicComponent::getStaticClassName(), false);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[UniversumComponent] Created moon '" + goName + "' radius=" + Ogre::StringConverter::toString(radius) + " at " + Ogre::StringConverter::toString(initialPosition));

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
                .def("getCameraGameObjectId", &UniversumComponent::getCameraGameObjectId)];

        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "class inherits GameObjectComponent", UniversumComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setRandomSeed(uint seed)", "Sets the random seed for reproducible universe generation.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "uint getRandomSeed()", "Gets the current random seed.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setSolarSystemCount(uint count)", "Sets the number of solar systems to generate.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setUseMotion(bool enable)", "Enables or disables kinematic orbital motion.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setUseOcean(bool enable)", "Enables or disables ocean generation on planets.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setPlayerGameObjectId(uint id)", "Sets the player GameObject Id for far-clip adaptation.");
        LuaScriptApi::getInstance()->addClassToCollection("UniversumComponent", "void setCameraGameObjectId(uint id)", "Sets the camera GameObject Id whose far-clip is adapted.");

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
        desc.needsMeshItem = false;
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