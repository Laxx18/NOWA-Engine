#ifndef UNIVERSUM_COMPONENT_H
#define UNIVERSUM_COMPONENT_H

#include "../../AreaOfInterestComponent/code/AreaOfInterestComponent.h"
#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

#include "luabind/luabind.hpp"
#include "luabind/object.hpp"

#include <random>
#include <vector>

namespace NOWA
{
    class AtmosphereComponent;

    /**
     * @class UniversumComponent
     * @brief Orchestrates procedural universe generation.
     *
     * Owns no mesh geometry. Creates and destroys GameObjects with
     * PlanetTerraComponent, ProceduralPlanetComponent, PlanetOceanComponent,
     * and (if useMotion) PhysicsActiveKinematicComponent.
     *
     * All created GameObjects are tracked in ownedGameObjectIds.
     * Clicking Generate destroys everything in that list and rebuilds from seed.
     */
    class EXPORTED UniversumComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<UniversumComponent> UniversumCompPtr;

    public:
        // Category assigned to all planet/moon GameObjects so PhysicsActiveComponent
        // gravitySourceCategory can find them for surface gravity calculation.
        static const Ogre::String PlanetCategory(void)
        {
            return "Planets";
        }

        // ---- Attr string keys ------------------------------------------------
        static const Ogre::String AttrRandomSeed(void)
        {
            return "Random Seed";
        }
        static const Ogre::String AttrSolarSystemCount(void)
        {
            return "Solar System Count";
        }
        static const Ogre::String AttrSolarSystemDistanceMin(void)
        {
            return "Solar System Distance Min";
        }
        static const Ogre::String AttrSolarSystemDistanceMax(void)
        {
            return "Solar System Distance Max";
        }
        static const Ogre::String AttrPlanetsPerSystemMin(void)
        {
            return "Planets Per System Min";
        }
        static const Ogre::String AttrPlanetsPerSystemMax(void)
        {
            return "Planets Per System Max";
        }
        static const Ogre::String AttrUseMoons(void)
        {
            return "Use Moons";
        }
        static const Ogre::String AttrMoonsPerPlanetMin(void)
        {
            return "Moons Per Planet Min";
        }
        static const Ogre::String AttrMoonsPerPlanetMax(void)
        {
            return "Moons Per Planet Max";
        }
        static const Ogre::String AttrUseMotion(void)
        {
            return "Use Motion";
        }
        static const Ogre::String AttrUseOcean(void)
        {
            return "Use Ocean";
        }
        static const Ogre::String AttrOceanProbability(void)
        {
            return "Ocean Probability";
        }
        static const Ogre::String AttrSunRadius(void)
        {
            return "Sun Radius";
        }
        static const Ogre::String AttrPlanetRadiusMin(void)
        {
            return "Planet Radius Min";
        }
        static const Ogre::String AttrPlanetRadiusMax(void)
        {
            return "Planet Radius Max";
        }
        static const Ogre::String AttrMoonRadius(void)
        {
            return "Moon Radius";
        }
        static const Ogre::String AttrOrbitalDistanceMin(void)
        {
            return "Orbital Distance Min";
        }
        static const Ogre::String AttrOrbitalDistanceMax(void)
        {
            return "Orbital Distance Max";
        }
        static const Ogre::String AttrMoonOrbitalDistanceMin(void)
        {
            return "Moon Orbital Distance Min";
        }
        static const Ogre::String AttrMoonOrbitalDistanceMax(void)
        {
            return "Moon Orbital Distance Max";
        }
        static const Ogre::String AttrOrbitalSpeedMin(void)
        {
            return "Orbital Speed Min";
        }
        static const Ogre::String AttrOrbitalSpeedMax(void)
        {
            return "Orbital Speed Max";
        }
        static const Ogre::String AttrAxialSpeedMin(void)
        {
            return "Axial Speed Min";
        }
        static const Ogre::String AttrAxialSpeedMax(void)
        {
            return "Axial Speed Max";
        }
        static const Ogre::String AttrPlayerGameObjectId(void)
        {
            return "Player GameObject Id";
        }
        static const Ogre::String AttrCameraGameObjectId(void)
        {
            return "Camera GameObject Id";
        }
        static const Ogre::String AttrSunLightGameObjectId(void)
        {
            return "Sun Light GameObject Id";
        }
        static const Ogre::String AttrFarClipSpace(void)
        {
            return "Far Clip Space";
        }
        static const Ogre::String AttrFarClipSurface(void)
        {
            return "Far Clip Surface";
        }
        static const Ogre::String AttrFarClipTransitionSpeed(void)
        {
            return "Far Clip Transition Speed";
        }
        static const Ogre::String AttrScale(void)
        {
            return "Scale";
        }
        static const Ogre::String AttrGenerate(void)
        {
            return "Generate";
        }
        static const Ogre::String AttrAutoPauseOrbit(void)
        {
            return "Auto Pause Orbit On Surface";
        }
        static Ogre::String ActionGenerate()
        {
            return "UniversumComponent.Generate";
        }

    public:
        UniversumComponent();
        virtual ~UniversumComponent();

        // ---- Ogre::Plugin interface ------------------------------------------
        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override;
        virtual void shutdown() override;
        virtual void uninstall() override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;
        virtual const Ogre::String& getName() const override;

        // ---- GameObjectComponent interface -----------------------------------
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("UniversumComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "UniversumComponent";
        }

        virtual unsigned int getClassId(void) const override
        {
            return NOWA::getIdFromName("UniversumComponent");
        }

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Procedural universe generator. Creates solar systems with suns, planets, moons, and optional oceans driven by a reproducible random seed.\n"
                   "Generation: Click Generate to destroy and rebuild the universe from the current seed and configuration.\n"
                   "Scale: Scale=1 gives sun~250m, planets 80-300m. Scale=2 multiplies all radii and distances by 10.\n"
                   "Textures: Scans the TerrainTextures resource group for _d. (diffuse) and _n. (normal) files.\n"
                   "  Files containing 'sun' go to the sun pool, 'moon' to the moon pool, all others to the planet pool.\n"
                   "  Each planet gets a random diffuse, normal, and 4 shuffled detail textures paired with their _n.dds normals.\n"
                   "  Moons get 2 detail layers. Add more textures to TerrainTextures to increase variety.\n"
                   "Orbital motion: Planets orbit their sun via PhysicsActiveKinematicComponent::setPosition each frame.\n"
                   "  Only the planet the player is currently on is frozen -- all other planets and moons keep moving.\n"
                   "  Axial rotation of the frozen planet is also stopped so buildings stay physically valid (TreeCollision).\n"
                   "  Day/night is faked by rotating the directional light direction at the planet's axial speed.\n"
                   "  Moons of the frozen planet still orbit normally -- player watches them arc across the sky.\n"
                   "  Pause/resume can also be driven from Lua: universum:pausePlanetOrbit(id) / resumePlanetOrbit(id).\n"
                   "Auto Pause Orbit On Surface: When enabled (default), orbital motion pauses automatically when\n"
                   "  any object enters a planet's AreaOfInterestComponent zone, and resumes when it leaves.\n"
                   "  Disable this flag if you want full manual control via Lua reactOnEnter / reactOnLeave instead.\n"
                   "Gravity: All planets and moons are assigned category 'Planets'.\n"
                   "  Set PhysicsActiveComponent::gravitySourceCategory='Planets' on the player or spacecraft.\n"
                   "  Surface gravity scales with radius: 50m reference = 19.8 m/s^2, clamped to [2, 150].\n"
                   "AreaOfInterest: Each planet/moon gets an AreaOfInterestComponent (radius = 2x planet radius).\n"
                   "  The C++ PlanetOrbitObserver is attached automatically -- no Lua script needed for basic use.\n"
                   "Camera: Set CameraGameObjectId to the main camera (defaults to MAIN_CAMERA_ID).\n"
                   "  Far clip lerps between FarClipSpace (default 1000000) and FarClipSurface (default 5000).\n"
                   "  Camera speed hint is posted as EventDataFeedback '[UNIVERSUM_CAMSPEED:value]' after Generate.\n"
                   "Sun light: Set SunLightGameObjectId to the scene directional light (defaults to MAIN_LIGHT_ID).\n"
                   "  Light direction, color, and power are updated each frame toward the nearest solar system.\n"
                   "  While on a surface, the light rotates to fake sunrise and sunset.\n"
                   "  Each star type gets a randomised color: G=warm yellow, F=white, K=orange, M=red, B=blue-white.\n"
                   "Player setup: PlayerGameObjectId + PhysicsActiveComponent with gravitySourceCategory='Planets'.\n"
                   "  The player is always a dynamic body. The planet kinematic body provides the collision surface.\n"
                   "  Spacecraft, hinge joints, and wheels all work normally when landed (objects are in close range).\n"
                   "PlanetSurfaceComponent: Add this component to any building, prop, or object placed on a planet.\n"
                   "  Set its TargetPlanetId to the planet's Id (shown in the Properties panel).\n"
                   "  At simulation start all surface objects are hidden and their physics disabled.\n"
                   "  When the player enters the planet's atmosphere zone, objects are repositioned and shown.\n"
                   "  Local offsets are computed at simulation start: localPos = planetRot.Inverse * (objPos - planetPos).\n"
                   "  When the player leaves, objects are hidden again. On simulation stop, all are fully restored.\n"
                   "  Works for any component type: PhysicsArtifactComponent (buildings), props, vehicles, etc.";
        }

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual void onRemoveComponent(void) override;
        virtual void update(Ogre::Real dt, bool notSimulating) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
        static bool canStaticAddComponent(GameObject* gameObject);

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();

        // ---- Setters / Getters -----------------------------------------------
        void setRandomSeed(unsigned int seed);
        unsigned int getRandomSeed(void) const;

        void setSolarSystemCount(unsigned int count);
        unsigned int getSolarSystemCount(void) const;

        void setSolarSystemDistanceMin(Ogre::Real dist);
        Ogre::Real getSolarSystemDistanceMin(void) const;

        void setSolarSystemDistanceMax(Ogre::Real dist);
        Ogre::Real getSolarSystemDistanceMax(void) const;

        void setPlanetsPerSystemMin(unsigned int count);
        unsigned int getPlanetsPerSystemMin(void) const;

        void setPlanetsPerSystemMax(unsigned int count);
        unsigned int getPlanetsPerSystemMax(void) const;

        void setUseMoons(bool useMoons);
        bool getUseMoons(void) const;

        void setMoonsPerPlanetMin(unsigned int count);
        unsigned int getMoonsPerPlanetMin(void) const;

        void setMoonsPerPlanetMax(unsigned int count);
        unsigned int getMoonsPerPlanetMax(void) const;

        void setUseMotion(bool useMotion);
        bool getUseMotion(void) const;

        void setUseOcean(bool useOcean);
        bool getUseOcean(void) const;

        void setOceanProbability(Ogre::Real probability);
        Ogre::Real getOceanProbability(void) const;

        void setSunRadius(Ogre::Real radius);
        Ogre::Real getSunRadius(void) const;

        void setPlanetRadiusMin(Ogre::Real radius);
        Ogre::Real getPlanetRadiusMin(void) const;

        void setPlanetRadiusMax(Ogre::Real radius);
        Ogre::Real getPlanetRadiusMax(void) const;

        void setMoonRadius(Ogre::Real radius);
        Ogre::Real getMoonRadius(void) const;

        void setOrbitalDistanceMin(Ogre::Real dist);
        Ogre::Real getOrbitalDistanceMin(void) const;

        void setOrbitalDistanceMax(Ogre::Real dist);
        Ogre::Real getOrbitalDistanceMax(void) const;

        void setMoonOrbitalDistanceMin(Ogre::Real dist);
        Ogre::Real getMoonOrbitalDistanceMin(void) const;

        void setMoonOrbitalDistanceMax(Ogre::Real dist);
        Ogre::Real getMoonOrbitalDistanceMax(void) const;

        void setOrbitalSpeedMin(Ogre::Real speed);
        Ogre::Real getOrbitalSpeedMin(void) const;

        void setOrbitalSpeedMax(Ogre::Real speed);
        Ogre::Real getOrbitalSpeedMax(void) const;

        void setAxialSpeedMin(Ogre::Real speed);
        Ogre::Real getAxialSpeedMin(void) const;

        void setAxialSpeedMax(Ogre::Real speed);
        Ogre::Real getAxialSpeedMax(void) const;

        void setPlayerGameObjectId(unsigned long id);
        unsigned long getPlayerGameObjectId(void) const;

        void setSunLightGameObjectId(unsigned long id);
        unsigned long getSunLightGameObjectId(void) const;

        void setCameraGameObjectId(unsigned long id);
        unsigned long getCameraGameObjectId(void) const;

        void setFarClipSpace(Ogre::Real distance);
        Ogre::Real getFarClipSpace(void) const;

        void setFarClipSurface(Ogre::Real distance);
        Ogre::Real getFarClipSurface(void) const;

        // ---- Inner observer: wired automatically to each planet's AreaOfInterest ----

        /**
         * @brief Per-planet observer that pauses/resumes orbital motion automatically
         *        when the player (or any GO in the AreaOfInterest categories) enters or
         *        leaves the planet's atmosphere zone.
         *        Created at generateUniverse() time; attached via attachTriggerObserver().
         */
        class PlanetOrbitObserver : public AreaOfInterestComponent::ITriggerSphereQueryObserver
        {
        public:
            PlanetOrbitObserver(UniversumComponent* owner, unsigned long planetId);

            virtual void onEnter(GameObject* gameObject) override;
            
            virtual void onLeave(GameObject* gameObject) override;

        private:
            UniversumComponent* owner;
            unsigned long planetId;
        };

    public:
        void setFarClipTransitionSpeed(Ogre::Real speed);
        Ogre::Real getFarClipTransitionSpeed(void) const;

        void setScale(Ogre::Real scale);
        Ogre::Real getScale(void) const;

        void setAutoPauseOrbit(bool autoPause);
        bool getAutoPauseOrbit(void) const;

        /**
         * @brief Registers a Lua closure called when the player enters a planet's atmosphere.
         * @param closureFunction  Lua function receiving (planetId: uint).
         * @note  Add a LuaScriptComponent to the same GameObject as UniversumComponent,
         *        then call this from its connect() function.
         */
        void reactOnPlanetEntered(luabind::object closureFunction);

        /**
         * @brief Registers a Lua closure called when the player leaves a planet's atmosphere.
         * @param closureFunction  Lua function receiving (planetId: uint).
         */
        void reactOnPlanetLeft(luabind::object closureFunction);

        /**
         * @brief Registers a Lua closure called once after generateUniverse() completes.
         * @param closureFunction  Lua function receiving no arguments.
         */
        void reactOnUniverseGenerated(luabind::object closureFunction);

        void reactOnLanded(luabind::object closureFunction);

        void requestTakeoff(void);

        void requestLanding(void);

        void reactOnLanding(luabind::object closureFunction);

        void callLandingFunction(unsigned long bodyId, unsigned long shipId);

        void callLandedFunction(unsigned long bodyId, unsigned long shipId);

    private:
        // ---- Internal structs ------------------------------------------------

        struct SurfaceObject
        {
            unsigned long gameObjectId = 0ul;
            Ogre::Vector3 localPosition = Ogre::Vector3::ZERO;              // Offset from planet center in planet-local space
            Ogre::Quaternion localOrientation = Ogre::Quaternion::IDENTITY; // Orientation relative to planet
            Ogre::Vector3 savedWorldPosition = Ogre::Vector3::ZERO;         // World position at connect() for disconnect() restore
            Ogre::Quaternion savedWorldOrientation = Ogre::Quaternion::IDENTITY;
        };

        struct OrbitalBody
        {
            unsigned long gameObjectId = 0ul;
            float orbitalRadius = 0.0f;
            float orbitalSpeed = 0.0f;
            float orbitalTilt = 0.0f;
            float phaseOffset = 0.0f;
            float axialSpeed = 0.0f;
            float orbitalElapsed = 0.0f;  // Pauses when player is on this body
            float axialElapsed = 0.0f;    // Never pauses -- drives day/night cycle
            bool orbitalPaused = false;   // Set by AreaOfInterest enter/leave
            float gravityStrength = 0.0f; // Computed from radius: larger = stronger
            Ogre::Vector3 currentPosition = Ogre::Vector3::ZERO;
            std::vector<SurfaceObject> surfaceObjects; // GOs placed on this body's surface
        };

        struct SolarSystem
        {
            unsigned long sunId = 0ul;
            Ogre::Vector3 position = Ogre::Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
            std::vector<OrbitalBody> planets;
            // pair: <parentPlanetIndex, moonData>
            std::vector<std::pair<size_t, OrbitalBody>> moons;
            Ogre::Vector3 sunLightColor = Ogre::Vector3::UNIT_SCALE; // Randomised star color at generation time
            float sunLightPower = 1.0f;                              // Randomised star intensity
        };

        enum class LandingState : unsigned int
        {
            NONE = 0,        // in space, no body frozen
            APPROACHING = 1, // inside AOI, orbit frozen, ship still flying above
            LANDING = 2,     // within altitudeThreshold, auto-landing active
            LANDED = 3,      // on surface, input locked, waiting for requestTakeoff()
            TAKING_OFF = 4   // rising, input unlocked; -> APPROACHING when clear
        };

        // ---- Generation helpers ----------------------------------------------

        /** Destroys all GameObjects created by this component. */
        void destroyUniverse(void);

        /** Top-level generation entry point — called by executeAction("Generate"). */
        void generateUniverse(void);

        /**
         * @brief Scans the "TerrainTextures" resource group for diffuse textures (_d.)
         *        and fills three pools: sunTextures, moonTextures, planetTextures.
         *        Sun textures must contain "sun" in their name.
         *        Moon textures must contain "moon" in their name.
         *        All others go into the planet pool.
         */
        void buildTexturePools(void);

        /** Places one solar system at world position and fills the SolarSystem struct. */
        void generateSolarSystem(size_t systemIndex, const Ogre::Vector3& systemPosition, float scaleFactor, std::mt19937& rng);

        /** Creates the sun GameObject for a solar system. Returns the new id. */
        unsigned long createSun(const Ogre::Vector3& position, float scaleFactor, std::mt19937& rng);

        /** Creates a planet GameObject. Returns the new id. */
        unsigned long createPlanet(const Ogre::Vector3& initialPosition, float radius, std::mt19937& rng);

        /** Creates a moon GameObject. Returns the new id. */
        unsigned long createMoon(const Ogre::Vector3& initialPosition, float radius, std::mt19937& rng);

        /** Posts a progress text event visible in the editor status bar. */
        void postProgressEvent(const Ogre::String& text);

        /**
         * @brief Scans all GameObjects with PlanetSurfaceComponent, groups them by
         *        TargetPlanetId, stores local offsets, hides all of them.
         *        Called at connect() time before simulation starts.
         */
        void registerAndHideSurfaceObjects(void);

        /**
         * @brief Shows and repositions all surface objects for the given orbital body,
         *        placing them at planetPos + planetRot * localOffset.
         *        Called when a planet's orbit is paused (player on surface).
         */
        void showSurfaceObjects(OrbitalBody& body);

        /**
         * @brief Hides all surface objects for the given orbital body and disables
         *        their physics. Called when orbit resumes (player leaves surface).
         */
        void hideSurfaceObjects(OrbitalBody& body);

        /**
         * @brief Restores all surface objects to their original world positions
         *        and makes them visible again. Called from disconnect().
         */
        void restoreAllSurfaceObjects(void);

        void callPlanetEnteredFunction(unsigned long planetId, unsigned long enteringGoId);
        void callPlanetLeftFunction(unsigned long planetId, unsigned long enteringGoId);
        void callUniverseGeneratedFunction(void);

        /**
         * @brief Called by AreaOfInterestComponent Lua callbacks when a player enters
         *        or leaves a planet's atmosphere zone. Pauses/resumes orbital translation
         *        while keeping axial rotation running for the day/night cycle.
         */
        void pausePlanetOrbit(unsigned long planetGameObjectId, unsigned long gameObjectId);
        void resumePlanetOrbit(unsigned long planetGameObjectId, unsigned long gameObjectId);

        /**
         * @brief Sets dynamic state on a planet/moon GO and all its sub-component items.
         *        GameObject::setDynamic only updates the primary movableObject.
         *        PlanetTerraComponent, PlanetOceanComponent, and PlanetSunComponent each
         *        own a separate Ogre::Item -- all must match the SceneNode static state
         *        or Ogre asserts "movableobject is static, while node is not".
         * @param goPtr     The planet or moon GameObject.
         * @param isDynamic Whether to make it dynamic (true) or static (false).
         */
        void setPlanetDynamic(GameObjectPtr goPtr, bool isDynamic);

        /**
         * @brief Computes surface gravity from planet radius.
         *        Scales linearly: reference 50m planet = 19.8 m/s^2.
         *        Larger planets are stronger, moons are weaker.
         */
        float computeGravity(float radius) const;

        /** Returns a uniform float in [lo, hi] from rng. */
        float randFloat(std::mt19937& rng, float lo, float hi);

        /** Returns a uniform int in [lo, hi] from rng. */
        int randInt(std::mt19937& rng, int lo, int hi);

        unsigned long getCurrentlyPausedBodyId() const;

        unsigned long getInnermostOverlapId() const;

        void applyShipMovement(GameObjectPtr shipGo, const Ogre::Vector3& velocity, const Ogre::Quaternion& targetOrient, float orientStrength);
        void setPlayerInputLock(bool locked);
        void updateLandingStateMachine(Ogre::Real dt);

        // Snaps a frozen moon back onto its orbital circle around the
        // planet's current position before resuming orbital motion.
        // Called by resumePlanetOrbit() to prevent visible teleport on leave.
        void snapMoonToOrbit(OrbitalBody& moon, const OrbitalBody& parentPlanet);

        void snapPlanetToOrbit(OrbitalBody& planet, const Ogre::Vector3& sunPos);

        void restoreSpaceShadowSettings();
        void setBodyCastShadows(unsigned long bodyGoId, bool castShadows, const Ogre::String& tag);
        void setupLandingState(OrbitalBody& body, unsigned long bodyGoId, float axialSpeedOverride, unsigned long playerGoId);
        bool teardownLandingState(unsigned long planetGameObjectId, unsigned long gameObjectId, OrbitalBody& body, bool isHideSurface, SolarSystem& system);

        bool findFlatLandingSpot(const Ogre::Vector3& shipPos, const Ogre::Vector3& surfaceNormal, const Ogre::Vector3& bodyCentre, Ogre::Real bodyRadius, GameObjectPtr shipGo, Ogre::Vector3& outTarget);

        void reset(void);
    private:
        void handleSceneParsed(EventDataPtr eventData);
    private:
        Ogre::String name;

        // ---- Variants --------------------------------------------------------
        Variant* generate;
        Variant* randomSeed;
        Variant* solarSystemCount;
        Variant* solarSystemDistanceMin;
        Variant* solarSystemDistanceMax;
        Variant* planetsPerSystemMin;
        Variant* planetsPerSystemMax;
        Variant* useMoons;
        Variant* moonsPerPlanetMin;
        Variant* moonsPerPlanetMax;
        Variant* useMotion;
        Variant* useOcean;
        Variant* oceanProbability;
        Variant* sunRadius;
        Variant* planetRadiusMin;
        Variant* planetRadiusMax;
        Variant* moonRadius;
        Variant* orbitalDistanceMin;
        Variant* orbitalDistanceMax;
        Variant* moonOrbitalDistanceMin;
        Variant* moonOrbitalDistanceMax;
        Variant* orbitalSpeedMin;
        Variant* orbitalSpeedMax;
        Variant* axialSpeedMin;
        Variant* axialSpeedMax;
        Variant* playerGameObjectId;
        Variant* cameraGameObjectId;
        Variant* sunLightGameObjectId;
        Variant* farClipSpace;
        Variant* farClipSurface;
        Variant* farClipTransitionSpeed;
        Variant* scale;
        Variant* autoPauseOrbit;

        // ---- Runtime state ---------------------------------------------------
        std::vector<SolarSystem> solarSystems;
        std::vector<unsigned long> ownedGameObjectIds;
        float elapsedTime;
        float currentFarClip;
        bool postInitDone;
        bool loadedFromFile;             // True when solarSystems were restored from scene XML -- skip generateUniverse.
        Ogre::Light* cachedSunLight;     // Raw ptr to the hijacked scene directional light
        Ogre::Vector3 currentLightColor; // Lerp target for smooth system transitions
        float currentLightPower;
        bool playerOnSurface;      // True when any planet has orbitalPaused==true
        bool firstLightFrame;

        // ---- Lua delegation closures -----------------------------------------
        luabind::object planetEnteredClosureFunction;
        luabind::object planetLeftClosureFunction;
        luabind::object universeGeneratedClosureFunction;
        luabind::object landingClosureFunction;
        luabind::object landedClosureFunction;
        // Attached to each planet/moon's AreaOfInterestComponent at generateUniverse time.
        // Destroyed in destroyUniverse.
        std::vector<PlanetOrbitObserver*> planetObservers;
        std::set<unsigned long> activeTriggerOverlaps;

        // Texture pools built once per generateUniverse() call from TerrainTextures group.
        std::vector<Ogre::String> sunTextures;
        std::vector<Ogre::String> moonTextures;
        std::vector<Ogre::String> planetTextures;
        std::vector<Ogre::String> sunNormalTextures;
        std::vector<Ogre::String> moonNormalTextures;
        std::vector<Ogre::String> planetNormalTextures;

        AtmosphereComponent* cachedAtmosphereComponent;

        // Landing state machine
        LandingState landingState;
        Ogre::Vector3 landingBodyCentre;
        float landingBodyRadius;
        unsigned long landedOnBodyId;
        float landingAltitudeThreshold; // auto-land below this (units)
        float takeoffClearanceAltitude; // takeoff done above this (units)
        float takeoffSpeed;             // units/s during ascent
        bool landingInputLocked;
        bool landingFired;
        float landingDebugTimer;
        float fakeLightElapsed;
        float fakeLightAxialSpeed;
        bool shadowsConfiguredForSurface;
        bool landingContactActive;
        float landingContactTimer;

        Ogre::Vector3 currentAmbientUpper;
        Ogre::Vector3 currentAmbientLower;

        // Ray query for flat landing spot search
        Ogre::RaySceneQuery* landingRayQuery;

        // The resolved flat landing target (world space), valid once findFlatLandingSpot succeeds
        Ogre::Vector3 resolvedLandingTarget;
        bool resolvedLandingTargetValid;

                 // Tuning
        static constexpr float LANDING_MAX_GRADIENT_DEG = 15.0f; // max acceptable slope
        static constexpr float LANDING_SEARCH_RADIUS = 30.0f;    // search radius on surface
        static constexpr int LANDING_SEARCH_RINGS = 4;           // concentric rings
        static constexpr int LANDING_SEARCH_RING_STEPS = 8;      // samples per ring
    };

} // namespace NOWA

#endif // UNIVERSUM_COMPONENT_H