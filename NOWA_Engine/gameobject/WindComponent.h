#ifndef WIND_COMPONENT_H
#define WIND_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
    /**
     * @class WindComponent
     * @brief Controls global wind parameters for the HlmsWind shader system.
     *
     * Place this component on any game object in the scene to activate wind-driven
     * vertex displacement for all objects that use a Wind HLMS datablock (HLMS_USER0).
     * Typical use: grass blades, foliage, cloth.
     *
     * Only one WindComponent should exist per scene. ProceduralFoliageVolumeComponent
     * and other wind-aware components automatically find and use it via the
     * GameObjectController.
     *
     * Wind is always independent of AtmosphereComponent and CompositorEffectComponents.
     */
    class EXPORTED WindComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::WindComponent> WindCompPtr;

    public:
        WindComponent();

        virtual ~WindComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        virtual bool postInit(void) override;

        virtual void onRemoveComponent(void) override;

        virtual bool connect(void) override;

        virtual bool disconnect(void) override;

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WindComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WindComponent";
        }

        /**
         * @see GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        /**
         * @see GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Controls global wind parameters (strength, direction, frequency) "
                   "for all objects using the HlmsWind shader (HLMS_USER0). "
                   "Only one WindComponent per scene is necessary. "
                   "Wind-aware components such as ProceduralFoliageVolumeComponent automatically "
                   "detect and use this component.";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = NOWA::CUSTOM;
            desc.displayName = "Wind";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.autoComponents = {WindComponent::getStaticClassName()};
            return desc;
        }

        /**
         * @see GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @brief Sets the overall wind strength.
         * @param strength Wind displacement scale. 0 = no wind. Typical range 0..2.
         *                 Values above 1 produce strong storm-like movement.
         */
        void setWindStrength(Ogre::Real strength);

        /**
         * @brief Gets the overall wind strength.
         */
        Ogre::Real getWindStrength(void) const;

        /**
         * @brief Sets the dominant wind direction in world space.
         * @param direction World-space direction vector. Automatically normalized.
         *                  Use (1,0,0) for east wind, (-1,0,0) for west, etc.
         *                  The Y component is ignored for ground-level vegetation.
         */
        void setWindDirection(const Ogre::Vector3& direction);

        /**
         * @brief Gets the dominant wind direction.
         */
        Ogre::Vector3 getWindDirection(void) const;

        /**
         * @brief Sets how fast the 3D noise field animates (turbulence speed).
         * @param frequency Animation speed multiplier. 1.0 = default. Higher values
         *                  produce more rapid turbulent variation between blades.
         *                  Typical range 0.1..5.0.
         */
        void setWindFrequency(Ogre::Real frequency);

        /**
         * @brief Gets the wind frequency.
         */
        Ogre::Real getWindFrequency(void) const;

    public:
        static const Ogre::String AttrWindStrength(void)
        {
            return "Wind Strength";
        }
        static const Ogre::String AttrWindDirection(void)
        {
            return "Wind Direction";
        }
        static const Ogre::String AttrWindFrequency(void)
        {
            return "Wind Frequency";
        }

    private:
        void applyToHlmsWind(void);

    private:
        Variant* windStrength;
        Variant* windDirection;
        Variant* windFrequency;
    };

}; // namespace end

#endif