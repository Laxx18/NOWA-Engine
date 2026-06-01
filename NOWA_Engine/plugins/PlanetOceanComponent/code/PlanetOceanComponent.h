/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PLANET_OCEAN_COMPONENT_H
#define PLANET_OCEAN_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "planetocean/PlanetOcean.h"

namespace NOWA
{
    /**
     * @brief Spherical ocean component animated via CPU Gerstner waves.
     *        Drop onto any GameObject. The ocean sphere is created automatically
     *        during postInit and animated each frame. Radius should be set slightly
     *        larger than any terrain sphere beneath it.
     */
    class EXPORTED PlanetOceanComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<PlanetOceanComponent> PlanetOceanComponentPtr;

    public:
        PlanetOceanComponent();

        virtual ~PlanetOceanComponent();

        /**
         * @see Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        /**
         * @see GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void) override;

        /**
         * @see GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        /**
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }

        /**
         * @brief Sets whether the component is activated.
         * @param[in] activated Whether to activate the component
         */
        void setActivated(bool activated);

        /**
         * @brief Gets whether the component is activated.
         * @return True if activated
         */
        bool isActivated(void) const;

        /**
         * @brief Sets the ocean sphere radius. Should be slightly larger than the terrain sphere.
         * @param[in] radius Radius in world units
         */
        void setRadius(Ogre::Real radius);

        /**
         * @brief Gets the ocean sphere radius.
         */
        Ogre::Real getRadius(void) const;

        void setDeepColour(const Ogre::Vector3& colour);
        Ogre::Vector3 getDeepColour(void) const;

        void setShallowColour(const Ogre::Vector3& colour);
        Ogre::Vector3 getShallowColour(void) const;

        /** @brief PBS roughness [0.001, 1]. Uses specular_fresnel workflow. 0.06 = calm ocean. */
        void setRoughness(Ogre::Real roughness);
        Ogre::Real getRoughness(void) const;

        /** @brief Water transparency [0=opaque, 1=fully transparent]. */
        void setTransparency(Ogre::Real transparency);
        Ogre::Real getTransparency(void) const;

        /** @brief Optional cubemap name for reflections, empty to skip. */
        void setReflectionMap(const Ogre::String& cubemapName);
        Ogre::String getReflectionMap(void) const;

        /** @brief Global amplitude multiplier for all waves. 0 = flat ocean. */
        void setWaveAmplitudeScale(Ogre::Real scale);
        Ogre::Real getWaveAmplitudeScale(void) const;

        /** @brief Wave crest height in metres on a 50m reference planet. Scales with radius. */
        void setWave0Amplitude(Ogre::Real v);
        Ogre::Real getWave0Amplitude(void) const;
        /** @brief Cycles per hemisphere (radius-independent). 2 = two crests across equator. */
        void setWave0Frequency(Ogre::Real v);
        Ogre::Real getWave0Frequency(void) const;
        /** @brief Phase speed in radians per second. */
        void setWave0Speed(Ogre::Real v);
        Ogre::Real getWave0Speed(void) const;
        /** @brief Propagation direction in radians (0 = +X axis). */
        void setWave0Direction(Ogre::Real v);
        Ogre::Real getWave0Direction(void) const;

        void setWave1Amplitude(Ogre::Real v);
        Ogre::Real getWave1Amplitude(void) const;
        void setWave1Frequency(Ogre::Real v);
        Ogre::Real getWave1Frequency(void) const;
        void setWave1Speed(Ogre::Real v);
        Ogre::Real getWave1Speed(void) const;
        void setWave1Direction(Ogre::Real v);
        Ogre::Real getWave1Direction(void) const;

        void setWave2Amplitude(Ogre::Real v);
        Ogre::Real getWave2Amplitude(void) const;
        void setWave2Frequency(Ogre::Real v);
        Ogre::Real getWave2Frequency(void) const;
        void setWave2Speed(Ogre::Real v);
        Ogre::Real getWave2Speed(void) const;
        void setWave2Direction(Ogre::Real v);
        Ogre::Real getWave2Direction(void) const;

        void setWave3Amplitude(Ogre::Real v);
        Ogre::Real getWave3Amplitude(void) const;
        void setWave3Frequency(Ogre::Real v);
        Ogre::Real getWave3Frequency(void) const;
        void setWave3Speed(Ogre::Real v);
        Ogre::Real getWave3Speed(void) const;
        void setWave3Direction(Ogre::Real v);
        Ogre::Real getWave3Direction(void) const;

        PlanetOcean* getOcean(void) const;

    public:
        /**
         * @see GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlanetOceanComponent");
        }

        /**
         * @see GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "PlanetOceanComponent";
        }

        /**
         * @see GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates an animated spherical ocean around the parent GameObject. "
                   "Set Radius slightly larger than any terrain sphere beneath it. "
                   "Four independent Gerstner waves are configurable with direction, amplitude, "
                   "frequency and speed. Transparency and reflection cubemap are also configurable.";
        }

        /**
         * @see GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrRadius(void)
        {
            return "Radius";
        }
        static const Ogre::String AttrSegmentsH(void)
        {
            return "SegmentsH";
        }
        static const Ogre::String AttrSegmentsV(void)
        {
            return "SegmentsV";
        }
        static const Ogre::String AttrDatablock(void)
        {
            return "Datablock";
        }
        static const Ogre::String AttrDeepColour(void)
        {
            return "Deep Colour";
        }
        static const Ogre::String AttrShallowColour(void)
        {
            return "Shallow Colour";
        }
        static const Ogre::String AttrRoughness(void)
        {
            return "Roughness";
        }
        static const Ogre::String AttrTransparency(void)
        {
            return "Transparency";
        }
        static const Ogre::String AttrReflectionMap(void)
        {
            return "Reflection Map";
        }
        static const Ogre::String AttrWaveAmplitudeScale(void)
        {
            return "Wave Amplitude Scale";
        }
        static const Ogre::String AttrWave0Amplitude(void)
        {
            return "Wave0 Amplitude";
        }
        static const Ogre::String AttrWave0Frequency(void)
        {
            return "Wave0 Frequency";
        }
        static const Ogre::String AttrWave0Speed(void)
        {
            return "Wave0 Speed";
        }
        static const Ogre::String AttrWave0Direction(void)
        {
            return "Wave0 Direction";
        }
        static const Ogre::String AttrWave1Amplitude(void)
        {
            return "Wave1 Amplitude";
        }
        static const Ogre::String AttrWave1Frequency(void)
        {
            return "Wave1 Frequency";
        }
        static const Ogre::String AttrWave1Speed(void)
        {
            return "Wave1 Speed";
        }
        static const Ogre::String AttrWave1Direction(void)
        {
            return "Wave1 Direction";
        }
        static const Ogre::String AttrWave2Amplitude(void)
        {
            return "Wave2 Amplitude";
        }
        static const Ogre::String AttrWave2Frequency(void)
        {
            return "Wave2 Frequency";
        }
        static const Ogre::String AttrWave2Speed(void)
        {
            return "Wave2 Speed";
        }
        static const Ogre::String AttrWave2Direction(void)
        {
            return "Wave2 Direction";
        }
        static const Ogre::String AttrWave3Amplitude(void)
        {
            return "Wave3 Amplitude";
        }
        static const Ogre::String AttrWave3Frequency(void)
        {
            return "Wave3 Frequency";
        }
        static const Ogre::String AttrWave3Speed(void)
        {
            return "Wave3 Speed";
        }
        static const Ogre::String AttrWave3Direction(void)
        {
            return "Wave3 Direction";
        }
        static Ogre::String DefaultMaterialName(void)
        {
            return "PlanetOceanDefaultMaterial";
        }

    private:
        void createOcean(void);
        void destroyOcean(void);
        void applyWaveParam(int waveIndex);

    private:
        Ogre::String name;

        PlanetOcean* ocean;

        Variant* activated;
        Variant* radius;
        Variant* segmentsH;
        Variant* segmentsV;
        Variant* datablock;
        Variant* deepColour;
        Variant* shallowColour;
        Variant* roughness;
        Variant* transparency;
        Variant* reflectionMap;
        Variant* waveAmplitudeScale;
        Variant* wave0Amplitude;
        Variant* wave0Frequency;
        Variant* wave0Speed;
        Variant* wave0Direction;
        Variant* wave1Amplitude;
        Variant* wave1Frequency;
        Variant* wave1Speed;
        Variant* wave1Direction;
        Variant* wave2Amplitude;
        Variant* wave2Frequency;
        Variant* wave2Speed;
        Variant* wave2Direction;
        Variant* wave3Amplitude;
        Variant* wave3Frequency;
        Variant* wave3Speed;
        Variant* wave3Direction;

        bool postInitDone;
        Ogre::String oceanUpdateClosureId;
    };

} // namespace NOWA

#endif // PLANET_OCEAN_COMPONENT_H