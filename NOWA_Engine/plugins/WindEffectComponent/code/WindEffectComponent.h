/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef WIND_EFFECT_COMPONENT_H
#define WIND_EFFECT_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "gameobject/GameObjectController.h"

namespace NOWA
{
    class HlmsWind;

    /**
     * @class WindEffectComponent
     * @brief Drop-in replacement for WindComponent that drives the global ambient
     *        Wind-shader state (strength, direction, turbulence frequency) and layers
     *        two extra effects on top of it: a slow continuous "breathing" sine sway
     *        of the strength, and occasional randomized gust impulses with their own
     *        direction variance. While a gust is in progress, the component also
     *        feeds a single HlmsWind interactor slot with a moving push position that
     *        travels along the gust direction, so the foliage visibly ripples in a
     *        wave as the gust front sweeps through the scene -- the same interactor
     *        mechanism WindInteractionComponent uses for moving GameObjects.
     *
     * Only ONE of WindComponent / WindEffectComponent should be active at a time,
     * since both drive the same HlmsWind singleton ambient state. Because the gust
     * front optionally occupies one of the WIND_MAX_INTERACTORS (8) slots shared
     * with all WindInteractionComponent instances in the scene, UseGustInteractor
     * can be switched off to free that slot up for moving GameObjects instead.
     *
     * Presets ("Calm", "Gentle Breeze", "Fresh Wind", "Strong Wind", "Storm",
     * "Hurricane") configure the ambient strength/frequency plus the breathing and
     * gust shaping values in one click; tweaking any single value afterwards flips
     * the preset to "Custom" automatically.
     *
     * Lua example:
     * @code
     *   local we = go:getWindEffectComponent()
     *   we:setPresetName("Storm")
     *   we:setWindDirection(Vector3(1, 0, 0.3))
     *   we:setGustDirectionVariance(40.0)
     * @endcode
     */
    class EXPORTED WindEffectComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<WindEffectComponent> WindEffectCompPtr;

    public:
        WindEffectComponent();

        virtual ~WindEffectComponent();

        /**
         * @see Ogre::Plugin::install
         * Registers this component class with the GameObjectFactory.
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

        virtual unsigned int getClassId(void) const override;

        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("WindEffectComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "WindEffectComponent";
        }

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Alternative to WindComponent. Drives the global ambient Wind-shader strength, "
                   "direction and turbulence frequency, plus a slow continuous breathing sway and randomized "
                   "gust impulses with their own direction variance. While a gust is active, a travelling push "
                   "position is fed into one HlmsWind interactor slot so the foliage visibly ripples as the "
                   "gust front sweeps through the scene. Only one WindComponent or WindEffectComponent should "
                   "be active at a time. Presets: Calm, Gentle Breeze, Fresh Wind, Strong Wind, Storm, Hurricane, Custom.";
        }

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        virtual bool postInit(void) override;

        virtual void onRemoveComponent(void) override;

        virtual bool connect(void) override;

        virtual bool disconnect(void) override;

        virtual void update(Ogre::Real dt, bool notSimulating) override;

        virtual void actualizeValue(Variant* attribute) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @brief Sets the wind preset. Applies the ambient strength/frequency plus the
         *        breathing and gust shaping values for that preset. WindDirection and
         *        UseGustInteractor are left untouched, since those are scene-specific.
         *        Setting any other value afterwards flips this back to "Custom".
         */
        void setPresetName(const Ogre::String& presetName);
        Ogre::String getPresetName(void) const;

        /**
         * @brief Sets the dominant world-space wind direction (automatically normalized).
         */
        void setWindDirection(const Ogre::Vector3& windDirection);
        Ogre::Vector3 getWindDirection(void) const;

        /**
         * @brief Sets the base ambient wind displacement strength (0.0 - 5.0).
         */
        void setWindStrength(Ogre::Real windStrength);
        Ogre::Real getWindStrength(void) const;

        /**
         * @brief Sets the speed at which the 3D Perlin turbulence field animates (0.1 - 10.0).
         */
        void setWindFrequency(Ogre::Real windFrequency);
        Ogre::Real getWindFrequency(void) const;

        /**
         * @brief Enables/disables the slow continuous breathing sway layered on top of the base strength.
         */
        void setSineModulationEnabled(bool sineModulationEnabled);
        bool getSineModulationEnabled(void) const;

        /**
         * @brief Sets the breathing sway amplitude as a fraction of the base strength (0.0 - 1.0).
         */
        void setSineModulationAmplitude(Ogre::Real sineModulationAmplitude);
        Ogre::Real getSineModulationAmplitude(void) const;

        /**
         * @brief Sets the breathing sway speed in Hz; how many full sway cycles per second (0.01 - 5.0).
         */
        void setSineModulationFrequency(Ogre::Real sineModulationFrequency);
        Ogre::Real getSineModulationFrequency(void) const;

        /**
         * @brief Enables/disables randomized gust impulses on top of the base strength.
         */
        void setGustEnabled(bool gustEnabled);
        bool getGustEnabled(void) const;

        /**
         * @brief Sets the minimum random delay in seconds before the next gust may start (0.5 - 120.0).
         */
        void setGustMinInterval(Ogre::Real gustMinInterval);
        Ogre::Real getGustMinInterval(void) const;

        /**
         * @brief Sets the maximum random delay in seconds before the next gust may start (0.5 - 120.0).
         */
        void setGustMaxInterval(Ogre::Real gustMaxInterval);
        Ogre::Real getGustMaxInterval(void) const;

        /**
         * @brief Sets how long a single gust impulse lasts in seconds (0.2 - 30.0). The strength and the
         *        travelling push both rise and fall smoothly (sine shaped) across this duration.
         */
        void setGustDuration(Ogre::Real gustDuration);
        Ogre::Real getGustDuration(void) const;

        /**
         * @brief Sets how many times the base strength is multiplied at the peak of a gust (1.0 - 6.0).
         */
        void setGustStrengthMultiplier(Ogre::Real gustStrengthMultiplier);
        Ogre::Real getGustStrengthMultiplier(void) const;

        /**
         * @brief Sets the maximum random yaw deviation in degrees of a gust's direction from
         *        WindDirection (0.0 - 90.0). A new random value is picked at the start of each gust.
         */
        void setGustDirectionVariance(Ogre::Real gustDirectionVariance);
        Ogre::Real getGustDirectionVariance(void) const;

        /**
         * @brief Sets whether one of the WIND_MAX_INTERACTORS (8) HlmsWind slots is occupied to
         *        feed a travelling push position into the foliage while a gust is active. Disable
         *        this to free the slot up for WindInteractionComponent instances instead.
         */
        void setUseGustInteractor(bool useGustInteractor);
        bool getUseGustInteractor(void) const;

        /**
         * @brief Sets the peak foliage push magnitude of the gust front interactor (0.0 - 5.0).
         */
        void setGustPushStrength(Ogre::Real gustPushStrength);
        Ogre::Real getGustPushStrength(void) const;

        /**
         * @brief Sets the radius in world units of the gust front interactor (0.5 - 80.0).
         */
        void setGustRadius(Ogre::Real gustRadius);
        Ogre::Real getGustRadius(void) const;

        /**
         * @brief Sets how fast in world units per second the gust front interactor travels
         *        away from the owner along the gust direction while a gust is active (0.0 - 50.0).
         */
        void setGustTravelSpeed(Ogre::Real gustTravelSpeed);
        Ogre::Real getGustTravelSpeed(void) const;

    public:
        static const Ogre::String AttrPresetName(void)
        {
            return "Preset Name";
        }
        static const Ogre::String AttrWindDirection(void)
        {
            return "Wind Direction";
        }
        static const Ogre::String AttrWindStrength(void)
        {
            return "Wind Strength";
        }
        static const Ogre::String AttrWindFrequency(void)
        {
            return "Wind Frequency";
        }
        static const Ogre::String AttrSineModulationEnabled(void)
        {
            return "Sine Modulation Enabled";
        }
        static const Ogre::String AttrSineModulationAmplitude(void)
        {
            return "Sine Modulation Amplitude";
        }
        static const Ogre::String AttrSineModulationFrequency(void)
        {
            return "Sine Modulation Frequency";
        }
        static const Ogre::String AttrGustEnabled(void)
        {
            return "Gust Enabled";
        }
        static const Ogre::String AttrGustMinInterval(void)
        {
            return "Gust Min Interval";
        }
        static const Ogre::String AttrGustMaxInterval(void)
        {
            return "Gust Max Interval";
        }
        static const Ogre::String AttrGustDuration(void)
        {
            return "Gust Duration";
        }
        static const Ogre::String AttrGustStrengthMultiplier(void)
        {
            return "Gust Strength Multiplier";
        }
        static const Ogre::String AttrGustDirectionVariance(void)
        {
            return "Gust Direction Variance";
        }
        static const Ogre::String AttrUseGustInteractor(void)
        {
            return "Use Gust Interactor";
        }
        static const Ogre::String AttrGustPushStrength(void)
        {
            return "Gust Push Strength";
        }
        static const Ogre::String AttrGustRadius(void)
        {
            return "Gust Radius";
        }
        static const Ogre::String AttrGustTravelSpeed(void)
        {
            return "Gust Travel Speed";
        }

    private:
        /**
         * @brief Applies all preset numeric values (strength/frequency/breathing/gust shaping)
         *        for the given preset name. WindDirection and UseGustInteractor are not touched.
         *        Does nothing for "Custom".
         */
        void applyPresetValues(const Ogre::String& presetName);

        /**
         * @brief Sets the preset back to "Custom" whenever an individual value is changed
         *        manually, unless a preset is currently being applied.
         */
        void markCustom(const Ogre::String& sourceAttributeName);

        /**
         * @brief Pushes WindStrength/WindDirection/WindFrequency straight to HlmsWind so changes
         *        are visible immediately, even while not simulating (e.g. editing in NOWA-Design).
         */
        void applyBaseAmbientToHlmsWind(void);

    private:
        Ogre::String name;
        HlmsWind* hlmsWind;
        int interactorSlot;
        bool applyingPreset;

        // Runtime breathing/gust state (not persisted, rebuilt on connect)
        Ogre::Real elapsedTime;
        bool gustActive;
        Ogre::Real gustElapsed;
        Ogre::Real timeSinceLastGust;
        Ogre::Real nextGustTime;
        Ogre::Real gustTravelOffset;
        Ogre::Vector3 currentGustDirection;

        // Diagnostic-only: tracks the last value actually sent to HlmsWind so a suspicious
        // single-frame jump can be logged with full state context. Safe to remove once the
        // root cause of the direction-inversion issue is confirmed and fixed.
        Ogre::Vector3 diagnosticLastDirection;
        Ogre::Real diagnosticLastStrength;
        bool diagnosticHasBaseline;

        Variant* presetName;
        Variant* windDirection;
        Variant* windStrength;
        Variant* windFrequency;

        Variant* sineModulationEnabled;
        Variant* sineModulationAmplitude;
        Variant* sineModulationFrequency;

        Variant* gustEnabled;
        Variant* gustMinInterval;
        Variant* gustMaxInterval;
        Variant* gustDuration;
        Variant* gustStrengthMultiplier;
        Variant* gustDirectionVariance;

        Variant* useGustInteractor;
        Variant* gustPushStrength;
        Variant* gustRadius;
        Variant* gustTravelSpeed;
    };

} // namespace NOWA

#endif