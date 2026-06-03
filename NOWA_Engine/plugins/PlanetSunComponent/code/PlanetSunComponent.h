/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PLANET_SUN_COMPONENT_H
#define PLANET_SUN_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "OgrePlugin.h"
#include "planetsun/PlanetSun.h"
#include "main/Events.h"

namespace NOWA
{
    class EXPORTED PlanetSunComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<PlanetSunComponent> PlanetSunComponentPtr;

    public:
        PlanetSunComponent();
        virtual ~PlanetSunComponent();

        // Ogre::Plugin
        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override;
        virtual void shutdown() override;
        virtual void uninstall() override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;
        virtual const Ogre::String& getName() const override;

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual void onRemoveComponent(void) override;
        virtual void update(Ogre::Real dt, bool notSimulating) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

    public:
        static unsigned int getStaticClassId()
        {
            return NOWA::getIdFromName("PlanetSunComponent");
        }
        static Ogre::String getStaticClassName()
        {
            return "PlanetSunComponent";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();

        static Ogre::String DefaultMaterialName()
        {
            return "PlanetSunDefaultMaterial";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText()
        {
            return "Spherical sun surface with animated procedural plasma/lava emissive texture updated every frame. "
                   "The plasma animation runs entirely on the GPU-uploaded 128x128 emissive map — no vertex deformation. "
                   "Place a PointLight or DirectionalLight on the same GameObject for a complete sun setup. "
                   "AnimationSpeed controls boiling rate; PlasmaScale controls granule density.";
        }

    public:
        static Ogre::String AttrActivated()
        {
            return "Activated";
        }
        static Ogre::String AttrRadius()
        {
            return "Radius";
        }
        static Ogre::String AttrSegmentsH()
        {
            return "SegmentsH";
        }
        static Ogre::String AttrSegmentsV()
        {
            return "SegmentsV";
        }
        static Ogre::String AttrDatablock()
        {
            return "Datablock";
        }
        static Ogre::String AttrEmissiveColour()
        {
            return "EmissiveColour";
        }
        static Ogre::String AttrDiffuseColour()
        {
            return "DiffuseColour";
        }
        static Ogre::String AttrRoughness()
        {
            return "Roughness";
        }
        static Ogre::String AttrAnimationSpeed()
        {
            return "AnimationSpeed";
        }
        static Ogre::String AttrPlasmaScale()
        {
            return "PlasmaScale";
        }

        // Setters / Getters
        void setActivated(bool activated);
        bool isActivated() const;
        void setRadius(float radius);
        float getRadius() const;
        void setSegmentsH(unsigned int segH);
        unsigned int getSegmentsH() const;
        void setSegmentsV(unsigned int segV);
        unsigned int getSegmentsV() const;
        void setEmissiveColour(const Ogre::Vector3& colour);
        Ogre::Vector3 getEmissiveColour() const;
        void setDiffuseColour(const Ogre::Vector3& colour);
        Ogre::Vector3 getDiffuseColour() const;
        void setRoughness(float roughness);
        float getRoughness() const;
        void setAnimationSpeed(float speed);
        float getAnimationSpeed() const;
        void setPlasmaScale(float scale);
        float getPlasmaScale() const;

        PlanetSun* getSun() const
        {
            return this->sun;
        }

        void setDynamic(bool dynamic);

    private:
        void createSun();
        void destroySun();

    private:
        Ogre::String name;
        PlanetSun* sun;
        bool postInitDone;
        Ogre::String sunUpdateClosureId;

        Variant* activated;
        Variant* radius;
        Variant* segmentsH;
        Variant* segmentsV;
        Variant* datablock;
        Variant* emissiveColour;
        Variant* diffuseColour;
        Variant* roughness;
        Variant* animationSpeed;
        Variant* plasmaScale;
    };

} // namespace NOWA

#endif