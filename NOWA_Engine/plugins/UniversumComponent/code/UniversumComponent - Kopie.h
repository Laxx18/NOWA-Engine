#ifndef UNIVERSUM_COMPONENT_H
#define UNIVERSUM_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

#include <random>
#include <vector>

namespace NOWA
{
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
            return "Procedural universe generator. Creates solar systems with suns, planets, "
                   "moons, and optional oceans driven by a random seed. Supports kinematic "
                   "orbital motion via PhysicsActiveKinematicComponent.";
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

        static void createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass);

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

        void setCameraGameObjectId(unsigned long id);
        unsigned long getCameraGameObjectId(void) const;

        void setFarClipSpace(Ogre::Real distance);
        Ogre::Real getFarClipSpace(void) const;

        void setFarClipSurface(Ogre::Real distance);
        Ogre::Real getFarClipSurface(void) const;

        void setFarClipTransitionSpeed(Ogre::Real speed);
        Ogre::Real getFarClipTransitionSpeed(void) const;

        void setScale(Ogre::Real scale);
        Ogre::Real getScale(void) const;

    private:
        // ---- Internal structs ------------------------------------------------

        struct OrbitalBody
        {
            unsigned long gameObjectId;
            float orbitalRadius;
            float orbitalSpeed;
            float orbitalTilt;
            float phaseOffset;
            float axialSpeed;
            Ogre::Vector3 currentPosition;
        };

        struct SolarSystem
        {
            unsigned long sunId;
            Ogre::Vector3 position;
            std::vector<OrbitalBody> planets;
            // pair: <parentPlanetIndex, moonData>
            std::vector<std::pair<size_t, OrbitalBody>> moons;
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

        /** Returns a uniform float in [lo, hi] from rng. */
        float randFloat(std::mt19937& rng, float lo, float hi);

        /** Returns a uniform int in [lo, hi] from rng. */
        int randInt(std::mt19937& rng, int lo, int hi);

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
        Variant* farClipSpace;
        Variant* farClipSurface;
        Variant* farClipTransitionSpeed;
        Variant* scale;

        // ---- Runtime state ---------------------------------------------------
        std::vector<SolarSystem> solarSystems;
        std::vector<unsigned long> ownedGameObjectIds;
        float elapsedTime;
        float currentFarClip;
        bool postInitDone;

        // Texture pools built once per generateUniverse() call from TerrainTextures group.
        std::vector<Ogre::String> sunTextures;
        std::vector<Ogre::String> moonTextures;
        std::vector<Ogre::String> planetTextures;
        std::vector<Ogre::String> sunNormalTextures;
        std::vector<Ogre::String> moonNormalTextures;
        std::vector<Ogre::String> planetNormalTextures;
    };

} // namespace NOWA

#endif // UNIVERSUM_COMPONENT_H