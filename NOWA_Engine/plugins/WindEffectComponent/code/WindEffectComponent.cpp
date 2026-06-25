#include "NOWAPrecompiled.h"
#include "WindEffectComponent.h"
#include "OgreAbiUtils.h"
#include "gameobject/GameObjectFactory.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "shader/HlmsWind.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    WindEffectComponent::WindEffectComponent() :
        GameObjectComponent(),
        name("WindEffectComponent"),
        hlmsWind(nullptr),
        interactorSlot(-1),
        applyingPreset(false),
        elapsedTime(0.0f),
        gustActive(false),
        gustElapsed(0.0f),
        timeSinceLastGust(0.0f),
        nextGustTime(10.0f),
        gustTravelOffset(0.0f),
        currentGustDirection(Ogre::Vector3::UNIT_X),
        presetName(new Variant(WindEffectComponent::AttrPresetName(), {"Calm", "Gentle Breeze", "Fresh Wind", "Strong Wind", "Storm", "Hurricane", "Custom"}, this->attributes)),
        windDirection(new Variant(WindEffectComponent::AttrWindDirection(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes)),
        windStrength(new Variant(WindEffectComponent::AttrWindStrength(), 0.3f, this->attributes)),
        windFrequency(new Variant(WindEffectComponent::AttrWindFrequency(), 0.25f, this->attributes)),

        sineModulationEnabled(new Variant(WindEffectComponent::AttrSineModulationEnabled(), true, this->attributes)),
        sineModulationAmplitude(new Variant(WindEffectComponent::AttrSineModulationAmplitude(), 0.15f, this->attributes)),
        sineModulationFrequency(new Variant(WindEffectComponent::AttrSineModulationFrequency(), 0.08f, this->attributes)),

        gustEnabled(new Variant(WindEffectComponent::AttrGustEnabled(), true, this->attributes)),
        gustMinInterval(new Variant(WindEffectComponent::AttrGustMinInterval(), 12.0f, this->attributes)),
        gustMaxInterval(new Variant(WindEffectComponent::AttrGustMaxInterval(), 22.0f, this->attributes)),
        gustDuration(new Variant(WindEffectComponent::AttrGustDuration(), 4.0f, this->attributes)),
        gustStrengthMultiplier(new Variant(WindEffectComponent::AttrGustStrengthMultiplier(), 1.6f, this->attributes)),
        gustDirectionVariance(new Variant(WindEffectComponent::AttrGustDirectionVariance(), 15.0f, this->attributes)),

        useGustInteractor(new Variant(WindEffectComponent::AttrUseGustInteractor(), true, this->attributes)),
        gustPushStrength(new Variant(WindEffectComponent::AttrGustPushStrength(), 0.6f, this->attributes)),
        gustRadius(new Variant(WindEffectComponent::AttrGustRadius(), 8.0f, this->attributes)),
        gustTravelSpeed(new Variant(WindEffectComponent::AttrGustTravelSpeed(), 3.0f, this->attributes))
    {
        this->presetName->addUserData(GameObject::AttrActionNeedRefresh());

        this->windFrequency->setConstraints(0.1f, 10.0f);
        this->windStrength->setConstraints(0.0f, 5.0f);

        this->sineModulationAmplitude->setConstraints(0.0f, 1.0f);
        this->sineModulationFrequency->setConstraints(0.01f, 5.0f);

        this->gustMinInterval->setConstraints(0.5f, 120.0f);
        this->gustMaxInterval->setConstraints(0.5f, 120.0f);
        this->gustDuration->setConstraints(0.2f, 30.0f);
        this->gustStrengthMultiplier->setConstraints(1.0f, 6.0f);
        this->gustDirectionVariance->setConstraints(0.0f, 90.0f);

        this->gustPushStrength->setConstraints(0.0f, 5.0f);
        this->gustRadius->setConstraints(0.5f, 80.0f);
        this->gustTravelSpeed->setConstraints(0.0f, 50.0f);

        this->presetName->setDescription("Selects a wind preset, applying strength, frequency, breathing and gust "
                                         "shaping values in one click. WindDirection and UseGustInteractor are left "
                                         "untouched. Manually changing any other value sets this back to 'Custom'.");
        this->windDirection->setDescription("Dominant wind direction in world space (automatically normalized). "
                                            "(1,0,0) = east wind, (-1,0,0) = west, (0,0,1) = south. "
                                            "The Y component has no effect on ground-level vegetation.");
        this->windStrength->setDescription("Base ambient wind displacement strength. 0 = no movement. "
                                           "0.5 = gentle breeze. 2.0+ = strong storm. Typical range: 0..2.");
        this->windFrequency->setDescription("Speed at which the 3D Perlin turbulence field animates. "
                                            "1.0 = default. Higher values = more rapid fluttering between blades.");

        this->sineModulationEnabled->setDescription("Enables a slow continuous breathing sway layered on top of WindStrength, "
                                                    "in addition to the per-blade Perlin turbulence.");
        this->sineModulationAmplitude->setDescription("Breathing sway amplitude, as a fraction of WindStrength. "
                                                      "0 = no breathing. 0.2 = strength gently swells +-20 percent.");
        this->sineModulationFrequency->setDescription("Breathing sway speed in Hz (full sway cycles per second). "
                                                      "Typical range: 0.05 (slow swell) .. 0.5 (restless).");

        this->gustEnabled->setDescription("Enables occasional randomized gust impulses on top of WindStrength, each with "
                                          "its own direction variance and an optional travelling foliage push.");
        this->gustMinInterval->setDescription("Minimum random delay in seconds before the next gust may start.");
        this->gustMaxInterval->setDescription("Maximum random delay in seconds before the next gust may start. "
                                              "A new random delay in [Min, Max] is picked after each gust ends.");
        this->gustDuration->setDescription("How long a single gust lasts in seconds. Strength and the travelling push "
                                           "both rise and fall smoothly (sine shaped) across this duration.");
        this->gustStrengthMultiplier->setDescription("How many times WindStrength is multiplied at the peak of a gust. "
                                                     "1.0 = gusts invisible. 2.0 = strength doubles at the peak.");
        this->gustDirectionVariance->setDescription("Maximum random yaw deviation in degrees of a gust's direction "
                                                    "from WindDirection. A new random value is picked per gust.");

        this->useGustInteractor->setDescription("Occupies one of the WIND_MAX_INTERACTORS (8) HlmsWind slots to feed a "
                                                "travelling push position into the foliage while a gust is active. "
                                                "Disable to free the slot up for WindInteractionComponent instances.");
        this->gustPushStrength->setDescription("Peak foliage push magnitude of the gust front interactor. "
                                               "Falls off quadratically to zero at GustRadius, like WindInteractionComponent.");
        this->gustRadius->setDescription("Radius in world units of the gust front interactor.");
        this->gustTravelSpeed->setDescription("Speed in world units per second at which the gust front interactor "
                                              "travels away from the owner GameObject along the gust direction.");
    }

    WindEffectComponent::~WindEffectComponent()
    {
    }

    void WindEffectComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<WindEffectComponent>(WindEffectComponent::getStaticClassId(), WindEffectComponent::getStaticClassName());
    }

    void WindEffectComponent::initialise()
    {
        // Called too early
    }

    void WindEffectComponent::shutdown()
    {
        // Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
    }

    void WindEffectComponent::uninstall()
    {
        // Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
    }

    void WindEffectComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& WindEffectComponent::getName() const
    {
        return this->name;
    }

    GameObjectCompPtr WindEffectComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // Not cloneable: WindEffectComponent drives the global HlmsWind ambient singleton state,
        // just like WindComponent. Having two active instances at once would fight over the same
        // strength/direction/frequency values, so only one should ever exist in the scene.
        return nullptr;
    }

    bool WindEffectComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPresetName())
        {
            this->presetName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWindDirection())
        {
            this->windDirection->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(1.0f, 0.0f, 0.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWindStrength())
        {
            this->windStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrWindFrequency())
        {
            this->windFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.25f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSineModulationEnabled())
        {
            this->sineModulationEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSineModulationAmplitude())
        {
            this->sineModulationAmplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.15f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSineModulationFrequency())
        {
            this->sineModulationFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.08f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustEnabled())
        {
            this->gustEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustMinInterval())
        {
            this->gustMinInterval->setValue(XMLConverter::getAttribReal(propertyElement, "data", 12.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustMaxInterval())
        {
            this->gustMaxInterval->setValue(XMLConverter::getAttribReal(propertyElement, "data", 22.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustDuration())
        {
            this->gustDuration->setValue(XMLConverter::getAttribReal(propertyElement, "data", 4.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustStrengthMultiplier())
        {
            this->gustStrengthMultiplier->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.6f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustDirectionVariance())
        {
            this->gustDirectionVariance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 15.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrUseGustInteractor())
        {
            this->useGustInteractor->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustPushStrength())
        {
            this->gustPushStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.6f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustRadius())
        {
            this->gustRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 8.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGustTravelSpeed())
        {
            this->gustTravelSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 3.0f));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool WindEffectComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WindEffectComponent] Init wind effect component for game object: " + this->gameObjectPtr->getName());

        Ogre::Hlms* hlmsBase = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER0);
        if (nullptr != hlmsBase)
        {
            this->hlmsWind = static_cast<HlmsWind*>(hlmsBase);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WindEffectComponent] Could not find HlmsWind! Affected game object: " + this->gameObjectPtr->getName());
            return false;
        }

        // Apply the loaded preset (does nothing for "Custom") and push the base ambient
        // values immediately, so the very first frame already looks correct.
        this->applyPresetValues(this->presetName->getListSelectedValue());
        this->applyBaseAmbientToHlmsWind();

        return true;
    }

    void WindEffectComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WindEffectComponent] Remove wind effect component for game object: " + this->gameObjectPtr->getName());

        if (nullptr != this->hlmsWind)
        {
            if (this->interactorSlot >= 0)
            {
                this->hlmsWind->releaseInteractorSlot(this->interactorSlot);
                this->interactorSlot = -1;
            }

            // Reset ambient wind to neutral so removing the component stops wind effects, consistent with WindComponent.
            this->hlmsWind->setWindStrength(0.0f);
        }
    }

    bool WindEffectComponent::connect(void)
    {
        GameObjectComponent::connect();

        // Reset all runtime breathing/gust state so simulation always starts clean.
        this->elapsedTime = 0.0f;
        this->gustActive = false;
        this->gustElapsed = 0.0f;
        this->gustTravelOffset = 0.0f;
        this->timeSinceLastGust = 0.0f;
        this->nextGustTime = Ogre::Math::RangeRandom(this->gustMinInterval->getReal(), this->gustMaxInterval->getReal());
        this->diagnosticHasBaseline = false;

        if (nullptr != this->hlmsWind && true == this->useGustInteractor->getBool() && this->interactorSlot < 0)
        {
            this->interactorSlot = this->hlmsWind->acquireInteractorSlot();
        }

        this->applyBaseAmbientToHlmsWind();

        return true;
    }

    bool WindEffectComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        if (nullptr != this->hlmsWind && this->interactorSlot >= 0)
        {
            this->hlmsWind->releaseInteractorSlot(this->interactorSlot);
            this->interactorSlot = -1;
        }
        return true;
    }

    void WindEffectComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }

        if (nullptr == this->hlmsWind || nullptr == this->gameObjectPtr)
        {
            return;
        }

        this->elapsedTime += dt;

        const Ogre::Real baseStrength = this->windStrength->getReal();
        const Ogre::Vector3 baseDirection = this->windDirection->getVector3();

        // ---- Continuous breathing sway: a slow sine modulation of the ambient strength. ----
        Ogre::Real breathingFactor = 0.0f;
        if (true == this->sineModulationEnabled->getBool())
        {
            const Ogre::Real sineFrequency = this->sineModulationFrequency->getReal();
            breathingFactor = Ogre::Math::Sin(this->elapsedTime * sineFrequency * Ogre::Math::TWO_PI) * this->sineModulationAmplitude->getReal();
        }

        // ---- Gust impulse state machine: occasional stronger pushes with direction variance. ----
        Ogre::Real gustEnvelope = 0.0f;
        Ogre::Vector3 gustDirection = baseDirection;

        if (true == this->gustEnabled->getBool())
        {
            if (false == this->gustActive)
            {
                this->timeSinceLastGust += dt;
                if (this->timeSinceLastGust >= this->nextGustTime)
                {
                    // Start a new gust: pick a random direction deviation and reset the travel offset.
                    this->gustActive = true;
                    this->gustElapsed = 0.0f;
                    this->gustTravelOffset = 0.0f;

                    const Ogre::Real varianceDegrees = this->gustDirectionVariance->getReal();
                    const Ogre::Real randomYaw = Ogre::Math::RangeRandom(-varianceDegrees, varianceDegrees);
                    this->currentGustDirection = Ogre::Quaternion(Ogre::Degree(randomYaw), Ogre::Vector3::UNIT_Y) * baseDirection;
                    if (this->currentGustDirection.squaredLength() > 0.0f)
                    {
                        this->currentGustDirection.normalise();
                    }
                }
            }
            else
            {
                this->gustElapsed += dt;
                const Ogre::Real duration = this->gustDuration->getReal();

                Ogre::Real t = this->gustElapsed / duration;
                if (t > 1.0f)
                {
                    t = 1.0f;
                }

                // Smooth rise-and-fall sine impulse shape: 0 -> 1 -> 0 across the gust duration.
                gustEnvelope = Ogre::Math::Sin(t * Ogre::Math::PI);
                gustDirection = this->currentGustDirection;
                this->gustTravelOffset += this->gustTravelSpeed->getReal() * dt;

                if (this->gustElapsed >= duration)
                {
                    this->gustActive = false;
                    this->timeSinceLastGust = 0.0f;
                    this->nextGustTime = Ogre::Math::RangeRandom(this->gustMinInterval->getReal(), this->gustMaxInterval->getReal());
                }
            }
        }

        // ---- Compose the final ambient values and push them straight to HlmsWind. ----
        // These are simple scalar/vector field writes on HlmsWindListener::pendingParams (no locking,
        // no render thread hop needed), exactly like WindInteractionComponent's per-frame slot updates.
        const Ogre::Real strengthMultiplier = this->gustStrengthMultiplier->getReal();
        const Ogre::Real finalStrength = baseStrength * (1.0f + breathingFactor) * (1.0f + (strengthMultiplier - 1.0f) * gustEnvelope);

        Ogre::Vector3 finalDirection = baseDirection * (1.0f - gustEnvelope) + gustDirection * gustEnvelope;
        if (finalDirection.squaredLength() > 0.0f)
        {
            finalDirection.normalise();
        }
        else
        {
            finalDirection = baseDirection;
        }

        const Ogre::Real baseFrequency = this->windFrequency->getReal();
       //  const Ogre::Real finalFrequency = baseFrequency * (1.0f + 0.5f * (strengthMultiplier - 1.0f) * gustEnvelope);
        const Ogre::Real finalFrequency = baseFrequency;
        // ---- Self-diagnosing guard: log full state on any suspicious single-frame jump. ----
        // This only checks the values WE computed, right before sending them to HlmsWind.
        // If this never fires but the visible glitch still happens, the bug is downstream
        // (most likely a torn read on HlmsWindListener::pendingParams between the logic
        // thread writing it every frame and the render thread copying it every frame with
        // no synchronization) rather than in this component's own math.
        if (true == this->diagnosticHasBaseline)
        {
            const Ogre::Real directionDot = this->diagnosticLastDirection.dotProduct(finalDirection);
            const Ogre::Real strengthJump = Ogre::Math::Abs(finalStrength - this->diagnosticLastStrength);

            // cos(60 degrees) = 0.5 -- flag any single-frame direction change bigger than 60
            // degrees, or any single-frame strength jump bigger than the base strength itself.
            // if (directionDot < 0.5f || strengthJump > (baseStrength + 0.001f))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[WindEffectComponent][ANOMALY] GameObject: " + this->gameObjectPtr->getName() +
                    " dt=" + Ogre::StringConverter::toString(dt) +
                    " elapsedTime=" + Ogre::StringConverter::toString(this->elapsedTime) +
                    " gustActive=" + Ogre::StringConverter::toString(this->gustActive) +
                    " gustElapsed=" + Ogre::StringConverter::toString(this->gustElapsed) +
                    " nextGustTime=" + Ogre::StringConverter::toString(this->nextGustTime) +
                    " timeSinceLastGust=" + Ogre::StringConverter::toString(this->timeSinceLastGust) +
                    " gustEnvelope=" + Ogre::StringConverter::toString(gustEnvelope) +
                    " breathingFactor=" + Ogre::StringConverter::toString(breathingFactor) +
                    " baseDirection=" + Ogre::StringConverter::toString(baseDirection) +
                    " currentGustDirection=" + Ogre::StringConverter::toString(this->currentGustDirection) +
                    " prevFinalDirection=" + Ogre::StringConverter::toString(this->diagnosticLastDirection) +
                    " newFinalDirection=" + Ogre::StringConverter::toString(finalDirection) +
                    " directionDot=" + Ogre::StringConverter::toString(directionDot) +
                    " prevFinalStrength=" + Ogre::StringConverter::toString(this->diagnosticLastStrength) +
                    " newFinalStrength=" + Ogre::StringConverter::toString(finalStrength));
            }
        }

        this->diagnosticLastDirection = finalDirection;
        this->diagnosticLastStrength = finalStrength;
        // this->diagnosticHasBaseline = true;

        this->hlmsWind->setWindStrength(static_cast<float>(finalStrength));
        this->hlmsWind->setWindDirection(finalDirection);
        this->hlmsWind->setWindFrequency(static_cast<float>(finalFrequency));

        // ---- Travelling gust front interactor: pushes foliage as the gust sweeps through the scene. ----
        if (true == this->useGustInteractor->getBool() && this->interactorSlot >= 0)
        {
            if (gustEnvelope > 0.0001f)
            {
                const Ogre::Real radius = this->gustRadius->getReal();

                // The interactor pushes foliage radially away from its own current position. Once it has
                // travelled a distance close to its own radius, the area around the owner -- which used to
                // be in front of the push -- ends up behind it, and the push there flips from forward to
                // backward all at once (a sudden, dramatic direction inversion). To avoid that, fade the
                // push strength out as the travelled distance approaches the radius, so it has already
                // reached zero before the flip would become visible, regardless of how strong the time
                // envelope still is at that point.
                Ogre::Real travelFade = 1.0f;
                const Ogre::Real fadeStartDistance = radius * 0.5f;
                if (this->gustTravelOffset > fadeStartDistance)
                {
                    travelFade = 1.0f - (this->gustTravelOffset - fadeStartDistance) / (radius - fadeStartDistance);
                    if (travelFade < 0.0f)
                    {
                        travelFade = 0.0f;
                    }
                }

                // Also hard-stop the travel at the radius itself, so the front never drifts far away into
                // empty space once its push has already faded out.
                if (this->gustTravelOffset > radius)
                {
                    this->gustTravelOffset = radius;
                }

                const Ogre::Vector3& ownerPos = this->gameObjectPtr->getPosition();
                const Ogre::Vector3 worldPos = ownerPos + gustDirection * this->gustTravelOffset;
                const Ogre::Real pushStrength = this->gustPushStrength->getReal() * gustEnvelope * travelFade;

                this->hlmsWind->updateInteractorSlot(this->interactorSlot, worldPos.x, worldPos.z, radius, pushStrength);
            }
            else
            {
                // No active gust: keep the slot occupied but with zero strength so it has no visual effect.
                this->hlmsWind->updateInteractorSlot(this->interactorSlot, 0.0f, 0.0f, this->gustRadius->getReal(), 0.0f);
            }
        }
    }

    void WindEffectComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (WindEffectComponent::AttrPresetName() == attribute->getName())
        {
            this->setPresetName(attribute->getListSelectedValue());
        }
        else if (WindEffectComponent::AttrWindDirection() == attribute->getName())
        {
            this->setWindDirection(attribute->getVector3());
        }
        else if (WindEffectComponent::AttrWindStrength() == attribute->getName())
        {
            this->setWindStrength(attribute->getReal());
        }
        else if (WindEffectComponent::AttrWindFrequency() == attribute->getName())
        {
            this->setWindFrequency(attribute->getReal());
        }
        else if (WindEffectComponent::AttrSineModulationEnabled() == attribute->getName())
        {
            this->setSineModulationEnabled(attribute->getBool());
        }
        else if (WindEffectComponent::AttrSineModulationAmplitude() == attribute->getName())
        {
            this->setSineModulationAmplitude(attribute->getReal());
        }
        else if (WindEffectComponent::AttrSineModulationFrequency() == attribute->getName())
        {
            this->setSineModulationFrequency(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustEnabled() == attribute->getName())
        {
            this->setGustEnabled(attribute->getBool());
        }
        else if (WindEffectComponent::AttrGustMinInterval() == attribute->getName())
        {
            this->setGustMinInterval(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustMaxInterval() == attribute->getName())
        {
            this->setGustMaxInterval(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustDuration() == attribute->getName())
        {
            this->setGustDuration(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustStrengthMultiplier() == attribute->getName())
        {
            this->setGustStrengthMultiplier(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustDirectionVariance() == attribute->getName())
        {
            this->setGustDirectionVariance(attribute->getReal());
        }
        else if (WindEffectComponent::AttrUseGustInteractor() == attribute->getName())
        {
            this->setUseGustInteractor(attribute->getBool());
        }
        else if (WindEffectComponent::AttrGustPushStrength() == attribute->getName())
        {
            this->setGustPushStrength(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustRadius() == attribute->getName())
        {
            this->setGustRadius(attribute->getReal());
        }
        else if (WindEffectComponent::AttrGustTravelSpeed() == attribute->getName())
        {
            this->setGustTravelSpeed(attribute->getReal());
        }
    }

    void WindEffectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("name", "Preset Name"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->presetName->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        auto writeV3 = [&](const char* name, const Ogre::Vector3& v)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "9"));
            n->append_attribute(doc.allocate_attribute("name", name));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(n);
        };

        auto writeR = [&](const char* name, Ogre::Real r)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "6"));
            n->append_attribute(doc.allocate_attribute("name", name));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, r)));
            propertiesXML->append_node(n);
        };

        auto writeB = [&](const char* name, bool b)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "12"));
            n->append_attribute(doc.allocate_attribute("name", name));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, b)));
            propertiesXML->append_node(n);
        };

        writeV3("Wind Direction", this->windDirection->getVector3());
        writeR("Wind Strength", this->windStrength->getReal());
        writeR("Wind Frequency", this->windFrequency->getReal());

        writeB("Sine Modulation Enabled", this->sineModulationEnabled->getBool());
        writeR("Sine Modulation Amplitude", this->sineModulationAmplitude->getReal());
        writeR("Sine Modulation Frequency", this->sineModulationFrequency->getReal());

        writeB("Gust Enabled", this->gustEnabled->getBool());
        writeR("Gust Min Interval", this->gustMinInterval->getReal());
        writeR("Gust Max Interval", this->gustMaxInterval->getReal());
        writeR("Gust Duration", this->gustDuration->getReal());
        writeR("Gust Strength Multiplier", this->gustStrengthMultiplier->getReal());
        writeR("Gust Direction Variance", this->gustDirectionVariance->getReal());

        writeB("Use Gust Interactor", this->useGustInteractor->getBool());
        writeR("Gust Push Strength", this->gustPushStrength->getReal());
        writeR("Gust Radius", this->gustRadius->getReal());
        writeR("Gust Travel Speed", this->gustTravelSpeed->getReal());
    }

    void WindEffectComponent::markCustom(const Ogre::String& sourceAttributeName)
    {
        if (true == this->applyingPreset)
        {
            return;
        }

        // Diagnostic-only: logs every time the preset is actually reset to "Custom" and by
        // which attribute, so a suspicious reset right after picking a preset (with no manual
        // edit) can be traced back to whatever called the setter. Safe to remove once confirmed.
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[WindEffectComponent][PRESET-RESET] GameObject: " + (nullptr != this->gameObjectPtr ? this->gameObjectPtr->getName() : Ogre::String("?")) +
            " preset '" + this->presetName->getListSelectedValue() + "' reset to 'Custom' because of attribute: " + sourceAttributeName);

        this->presetName->setListSelectedValue("Custom");
    }

    void WindEffectComponent::applyBaseAmbientToHlmsWind(void)
    {
        if (nullptr == this->hlmsWind)
        {
            return;
        }

        this->hlmsWind->setWindStrength(static_cast<float>(this->windStrength->getReal()));
        this->hlmsWind->setWindDirection(this->windDirection->getVector3());
        this->hlmsWind->setWindFrequency(static_cast<float>(this->windFrequency->getReal()));
    }

    void WindEffectComponent::applyPresetValues(const Ogre::String& presetNameValue)
    {
        if ("Custom" == presetNameValue)
        {
            return;
        }

        // Apply only the strength/frequency/breathing/gust shaping example values.
        // WindDirection and UseGustInteractor are scene-specific and stay untouched.
        this->applyingPreset = true;

        if ("Calm" == presetNameValue)
        {
            this->windStrength->setValue(0.05f);
            this->windFrequency->setValue(0.15f);
            this->sineModulationEnabled->setValue(true);
            this->sineModulationAmplitude->setValue(0.05f);
            this->sineModulationFrequency->setValue(0.05f);
            this->gustEnabled->setValue(false);
            this->gustMinInterval->setValue(20.0f);
            this->gustMaxInterval->setValue(40.0f);
            this->gustDuration->setValue(3.0f);
            this->gustStrengthMultiplier->setValue(1.0f);
            this->gustDirectionVariance->setValue(0.0f);
            this->gustPushStrength->setValue(0.0f);
            this->gustRadius->setValue(6.0f);
            this->gustTravelSpeed->setValue(2.0f);
        }
        else if ("Gentle Breeze" == presetNameValue)
        {
            this->windStrength->setValue(0.3f);
            this->windFrequency->setValue(0.25f);
            this->sineModulationEnabled->setValue(true);
            this->sineModulationAmplitude->setValue(0.15f);
            this->sineModulationFrequency->setValue(0.08f);
            this->gustEnabled->setValue(true);
            this->gustMinInterval->setValue(12.0f);
            this->gustMaxInterval->setValue(22.0f);
            this->gustDuration->setValue(4.0f);
            this->gustStrengthMultiplier->setValue(1.6f);
            this->gustDirectionVariance->setValue(15.0f);
            this->gustPushStrength->setValue(0.6f);
            this->gustRadius->setValue(8.0f);
            this->gustTravelSpeed->setValue(3.0f);
        }
        else if ("Fresh Wind" == presetNameValue)
        {
            this->windStrength->setValue(0.6f);
            this->windFrequency->setValue(0.4f);
            this->sineModulationEnabled->setValue(true);
            this->sineModulationAmplitude->setValue(0.2f);
            this->sineModulationFrequency->setValue(0.12f);
            this->gustEnabled->setValue(true);
            this->gustMinInterval->setValue(8.0f);
            this->gustMaxInterval->setValue(16.0f);
            this->gustDuration->setValue(3.5f);
            this->gustStrengthMultiplier->setValue(2.0f);
            this->gustDirectionVariance->setValue(20.0f);
            this->gustPushStrength->setValue(1.0f);
            this->gustRadius->setValue(10.0f);
            this->gustTravelSpeed->setValue(4.5f);
        }
        else if ("Strong Wind" == presetNameValue)
        {
            this->windStrength->setValue(1.0f);
            this->windFrequency->setValue(0.65f);
            this->sineModulationEnabled->setValue(true);
            this->sineModulationAmplitude->setValue(0.25f);
            this->sineModulationFrequency->setValue(0.18f);
            this->gustEnabled->setValue(true);
            this->gustMinInterval->setValue(5.0f);
            this->gustMaxInterval->setValue(11.0f);
            this->gustDuration->setValue(3.0f);
            this->gustStrengthMultiplier->setValue(2.4f);
            this->gustDirectionVariance->setValue(28.0f);
            this->gustPushStrength->setValue(1.6f);
            this->gustRadius->setValue(14.0f);
            this->gustTravelSpeed->setValue(6.5f);
        }
        else if ("Storm" == presetNameValue)
        {
            this->windStrength->setValue(1.8f);
            this->windFrequency->setValue(1.0f);
            this->sineModulationEnabled->setValue(true);
            this->sineModulationAmplitude->setValue(0.3f);
            this->sineModulationFrequency->setValue(0.3f);
            this->gustEnabled->setValue(true);
            this->gustMinInterval->setValue(3.0f);
            this->gustMaxInterval->setValue(7.0f);
            this->gustDuration->setValue(2.5f);
            this->gustStrengthMultiplier->setValue(2.6f);
            this->gustDirectionVariance->setValue(35.0f);
            this->gustPushStrength->setValue(2.4f);
            this->gustRadius->setValue(18.0f);
            this->gustTravelSpeed->setValue(9.0f);
        }
        else if ("Hurricane" == presetNameValue)
        {
            this->windStrength->setValue(3.2f);
            this->windFrequency->setValue(1.6f);
            this->sineModulationEnabled->setValue(true);
            this->sineModulationAmplitude->setValue(0.35f);
            this->sineModulationFrequency->setValue(0.5f);
            this->gustEnabled->setValue(true);
            this->gustMinInterval->setValue(1.5f);
            this->gustMaxInterval->setValue(4.0f);
            this->gustDuration->setValue(2.0f);
            this->gustStrengthMultiplier->setValue(2.8f);
            this->gustDirectionVariance->setValue(45.0f);
            this->gustPushStrength->setValue(3.5f);
            this->gustRadius->setValue(24.0f);
            this->gustTravelSpeed->setValue(14.0f);
        }

        this->applyingPreset = false;
    }

    unsigned int WindEffectComponent::getClassId(void) const
    {
        return WindEffectComponent::getStaticClassId();
    }

    Ogre::String WindEffectComponent::getClassName(void) const
    {
        return "WindEffectComponent";
    }

    Ogre::String WindEffectComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void WindEffectComponent::setPresetName(const Ogre::String& presetNameValue)
    {
        this->presetName->setListSelectedValue(presetNameValue);
        this->applyPresetValues(presetNameValue);
        this->applyBaseAmbientToHlmsWind();
    }

    Ogre::String WindEffectComponent::getPresetName(void) const
    {
        return this->presetName->getListSelectedValue();
    }

    void WindEffectComponent::setWindDirection(const Ogre::Vector3& windDirectionValue)
    {
        Ogre::Vector3 normalized = windDirectionValue;
        if (normalized.squaredLength() > 0.0f)
        {
            normalized.normalise();
        }

        // No-op guard: a redundant call with the value that is already stored must never be
        // treated as a manual edit (e.g. an editor refresh re-reporting the value the preset
        // itself just set would otherwise flip the preset name back to "Custom").
        if (true == normalized.positionEquals(this->windDirection->getVector3(), 0.0001f))
        {
            return;
        }

        this->windDirection->setValue(normalized);
        this->markCustom(WindEffectComponent::AttrWindDirection());
        this->applyBaseAmbientToHlmsWind();
    }

    Ogre::Vector3 WindEffectComponent::getWindDirection(void) const
    {
        return this->windDirection->getVector3();
    }

    void WindEffectComponent::setWindStrength(Ogre::Real windStrengthValue)
    {
        if (true == Ogre::Math::RealEqual(this->windStrength->getReal(), windStrengthValue, 0.0001f))
        {
            return;
        }

        this->windStrength->setValue(windStrengthValue);
        this->markCustom(WindEffectComponent::AttrWindStrength());
        this->applyBaseAmbientToHlmsWind();
    }

    Ogre::Real WindEffectComponent::getWindStrength(void) const
    {
        return this->windStrength->getReal();
    }

    void WindEffectComponent::setWindFrequency(Ogre::Real windFrequencyValue)
    {
        if (true == Ogre::Math::RealEqual(this->windFrequency->getReal(), windFrequencyValue, 0.0001f))
        {
            return;
        }

        this->windFrequency->setValue(windFrequencyValue);
        this->markCustom(WindEffectComponent::AttrWindFrequency());
        this->applyBaseAmbientToHlmsWind();
    }

    Ogre::Real WindEffectComponent::getWindFrequency(void) const
    {
        return this->windFrequency->getReal();
    }

    void WindEffectComponent::setSineModulationEnabled(bool sineModulationEnabledValue)
    {
        if (this->sineModulationEnabled->getBool() == sineModulationEnabledValue)
        {
            return;
        }

        this->sineModulationEnabled->setValue(sineModulationEnabledValue);
        this->markCustom(WindEffectComponent::AttrSineModulationEnabled());
    }

    bool WindEffectComponent::getSineModulationEnabled(void) const
    {
        return this->sineModulationEnabled->getBool();
    }

    void WindEffectComponent::setSineModulationAmplitude(Ogre::Real sineModulationAmplitudeValue)
    {
        if (true == Ogre::Math::RealEqual(this->sineModulationAmplitude->getReal(), sineModulationAmplitudeValue, 0.0001f))
        {
            return;
        }

        this->sineModulationAmplitude->setValue(sineModulationAmplitudeValue);
        this->markCustom(WindEffectComponent::AttrSineModulationAmplitude());
    }

    Ogre::Real WindEffectComponent::getSineModulationAmplitude(void) const
    {
        return this->sineModulationAmplitude->getReal();
    }

    void WindEffectComponent::setSineModulationFrequency(Ogre::Real sineModulationFrequencyValue)
    {
        if (true == Ogre::Math::RealEqual(this->sineModulationFrequency->getReal(), sineModulationFrequencyValue, 0.0001f))
        {
            return;
        }

        this->sineModulationFrequency->setValue(sineModulationFrequencyValue);
        this->markCustom(WindEffectComponent::AttrSineModulationFrequency());
    }

    Ogre::Real WindEffectComponent::getSineModulationFrequency(void) const
    {
        return this->sineModulationFrequency->getReal();
    }

    void WindEffectComponent::setGustEnabled(bool gustEnabledValue)
    {
        if (this->gustEnabled->getBool() == gustEnabledValue)
        {
            return;
        }

        this->gustEnabled->setValue(gustEnabledValue);
        this->markCustom(WindEffectComponent::AttrGustEnabled());
    }

    bool WindEffectComponent::getGustEnabled(void) const
    {
        return this->gustEnabled->getBool();
    }

    void WindEffectComponent::setGustMinInterval(Ogre::Real gustMinIntervalValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustMinInterval->getReal(), gustMinIntervalValue, 0.0001f))
        {
            return;
        }

        this->gustMinInterval->setValue(gustMinIntervalValue);
        this->markCustom(WindEffectComponent::AttrGustMinInterval());
    }

    Ogre::Real WindEffectComponent::getGustMinInterval(void) const
    {
        return this->gustMinInterval->getReal();
    }

    void WindEffectComponent::setGustMaxInterval(Ogre::Real gustMaxIntervalValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustMaxInterval->getReal(), gustMaxIntervalValue, 0.0001f))
        {
            return;
        }

        this->gustMaxInterval->setValue(gustMaxIntervalValue);
        this->markCustom(WindEffectComponent::AttrGustMaxInterval());
    }

    Ogre::Real WindEffectComponent::getGustMaxInterval(void) const
    {
        return this->gustMaxInterval->getReal();
    }

    void WindEffectComponent::setGustDuration(Ogre::Real gustDurationValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustDuration->getReal(), gustDurationValue, 0.0001f))
        {
            return;
        }

        this->gustDuration->setValue(gustDurationValue);
        this->markCustom(WindEffectComponent::AttrGustDuration());
    }

    Ogre::Real WindEffectComponent::getGustDuration(void) const
    {
        return this->gustDuration->getReal();
    }

    void WindEffectComponent::setGustStrengthMultiplier(Ogre::Real gustStrengthMultiplierValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustStrengthMultiplier->getReal(), gustStrengthMultiplierValue, 0.0001f))
        {
            return;
        }

        this->gustStrengthMultiplier->setValue(gustStrengthMultiplierValue);
        this->markCustom(WindEffectComponent::AttrGustStrengthMultiplier());
    }

    Ogre::Real WindEffectComponent::getGustStrengthMultiplier(void) const
    {
        return this->gustStrengthMultiplier->getReal();
    }

    void WindEffectComponent::setGustDirectionVariance(Ogre::Real gustDirectionVarianceValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustDirectionVariance->getReal(), gustDirectionVarianceValue, 0.0001f))
        {
            return;
        }

        this->gustDirectionVariance->setValue(gustDirectionVarianceValue);
        this->markCustom(WindEffectComponent::AttrGustDirectionVariance());
    }

    Ogre::Real WindEffectComponent::getGustDirectionVariance(void) const
    {
        return this->gustDirectionVariance->getReal();
    }

    void WindEffectComponent::setUseGustInteractor(bool useGustInteractorValue)
    {
        if (this->useGustInteractor->getBool() == useGustInteractorValue)
        {
            return;
        }

        this->useGustInteractor->setValue(useGustInteractorValue);
        this->markCustom(WindEffectComponent::AttrUseGustInteractor());

        if (nullptr == this->hlmsWind)
        {
            return;
        }

        if (true == useGustInteractorValue)
        {
            if (this->interactorSlot < 0)
            {
                this->interactorSlot = this->hlmsWind->acquireInteractorSlot();
            }
        }
        else
        {
            if (this->interactorSlot >= 0)
            {
                this->hlmsWind->releaseInteractorSlot(this->interactorSlot);
                this->interactorSlot = -1;
            }
        }
    }

    bool WindEffectComponent::getUseGustInteractor(void) const
    {
        return this->useGustInteractor->getBool();
    }

    void WindEffectComponent::setGustPushStrength(Ogre::Real gustPushStrengthValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustPushStrength->getReal(), gustPushStrengthValue, 0.0001f))
        {
            return;
        }

        this->gustPushStrength->setValue(gustPushStrengthValue);
        this->markCustom(WindEffectComponent::AttrGustPushStrength());
    }

    Ogre::Real WindEffectComponent::getGustPushStrength(void) const
    {
        return this->gustPushStrength->getReal();
    }

    void WindEffectComponent::setGustRadius(Ogre::Real gustRadiusValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustRadius->getReal(), gustRadiusValue, 0.0001f))
        {
            return;
        }

        this->gustRadius->setValue(gustRadiusValue);
        this->markCustom(WindEffectComponent::AttrGustRadius());
    }

    Ogre::Real WindEffectComponent::getGustRadius(void) const
    {
        return this->gustRadius->getReal();
    }

    void WindEffectComponent::setGustTravelSpeed(Ogre::Real gustTravelSpeedValue)
    {
        if (true == Ogre::Math::RealEqual(this->gustTravelSpeed->getReal(), gustTravelSpeedValue, 0.0001f))
        {
            return;
        }

        this->gustTravelSpeed->setValue(gustTravelSpeedValue);
        this->markCustom(WindEffectComponent::AttrGustTravelSpeed());
    }

    Ogre::Real WindEffectComponent::getGustTravelSpeed(void) const
    {
        return this->gustTravelSpeed->getReal();
    }

    bool WindEffectComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto windEffectCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<WindEffectComponent>());

        auto gameobjectsWithWindComponent = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent("WindInteractorComponent");
        // No free wind interactor slots!
        if (gameobjectsWithWindComponent.size() >= 7)
        {
            return false;
        }

        // Constraint: cannot combine with a classic WindComponent on the same game object, and can
        // only be added once, since both drive the same global ambient HlmsWind state.
        if (nullptr == windEffectCompPtr && false == gameObject->hasComponent("WindComponent"))
        {
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------
    // Lua free functions
    // ------------------------------------------------------------
    WindEffectComponent* getWindEffectComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<WindEffectComponent>(gameObject->getComponentWithOccurrence<WindEffectComponent>(occurrenceIndex)).get();
    }

    WindEffectComponent* getWindEffectComponent(GameObject* gameObject)
    {
        return makeStrongPtr<WindEffectComponent>(gameObject->getComponent<WindEffectComponent>()).get();
    }

    WindEffectComponent* getWindEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<WindEffectComponent>(gameObject->getComponentFromName<WindEffectComponent>(name)).get();
    }

    void WindEffectComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<WindEffectComponent, GameObjectComponent>("WindEffectComponent")
                .def("setActivated", &WindEffectComponent::setActivated)
                .def("isActivated", &WindEffectComponent::isActivated)

                .def("setPresetName", &WindEffectComponent::setPresetName)
                .def("getPresetName", &WindEffectComponent::getPresetName)

                .def("setWindDirection", &WindEffectComponent::setWindDirection)
                .def("getWindDirection", &WindEffectComponent::getWindDirection)
                .def("setWindStrength", &WindEffectComponent::setWindStrength)
                .def("getWindStrength", &WindEffectComponent::getWindStrength)
                .def("setWindFrequency", &WindEffectComponent::setWindFrequency)
                .def("getWindFrequency", &WindEffectComponent::getWindFrequency)

                .def("setSineModulationEnabled", &WindEffectComponent::setSineModulationEnabled)
                .def("getSineModulationEnabled", &WindEffectComponent::getSineModulationEnabled)
                .def("setSineModulationAmplitude", &WindEffectComponent::setSineModulationAmplitude)
                .def("getSineModulationAmplitude", &WindEffectComponent::getSineModulationAmplitude)
                .def("setSineModulationFrequency", &WindEffectComponent::setSineModulationFrequency)
                .def("getSineModulationFrequency", &WindEffectComponent::getSineModulationFrequency)

                .def("setGustEnabled", &WindEffectComponent::setGustEnabled)
                .def("getGustEnabled", &WindEffectComponent::getGustEnabled)
                .def("setGustMinInterval", &WindEffectComponent::setGustMinInterval)
                .def("getGustMinInterval", &WindEffectComponent::getGustMinInterval)
                .def("setGustMaxInterval", &WindEffectComponent::setGustMaxInterval)
                .def("getGustMaxInterval", &WindEffectComponent::getGustMaxInterval)
                .def("setGustDuration", &WindEffectComponent::setGustDuration)
                .def("getGustDuration", &WindEffectComponent::getGustDuration)
                .def("setGustStrengthMultiplier", &WindEffectComponent::setGustStrengthMultiplier)
                .def("getGustStrengthMultiplier", &WindEffectComponent::getGustStrengthMultiplier)
                .def("setGustDirectionVariance", &WindEffectComponent::setGustDirectionVariance)
                .def("getGustDirectionVariance", &WindEffectComponent::getGustDirectionVariance)

                .def("setUseGustInteractor", &WindEffectComponent::setUseGustInteractor)
                .def("getUseGustInteractor", &WindEffectComponent::getUseGustInteractor)
                .def("setGustPushStrength", &WindEffectComponent::setGustPushStrength)
                .def("getGustPushStrength", &WindEffectComponent::getGustPushStrength)
                .def("setGustRadius", &WindEffectComponent::setGustRadius)
                .def("getGustRadius", &WindEffectComponent::getGustRadius)
                .def("setGustTravelSpeed", &WindEffectComponent::setGustTravelSpeed)
                .def("getGustTravelSpeed", &WindEffectComponent::getGustTravelSpeed)];

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "class inherits GameObjectComponent", WindEffectComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setActivated(bool activated)", "Sets whether this component is active.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "bool isActivated()", "Gets whether this component is active.");

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setPresetName(string presetName)",
            "Sets the wind preset. Possible values are: 'Calm', 'Gentle Breeze', 'Fresh Wind', 'Strong Wind', 'Storm', 'Hurricane', 'Custom'. "
            "Preset examples: "
            "Calm: WindStrength=0.05 WindFrequency=0.15 GustEnabled=false; "
            "Gentle Breeze: WindStrength=0.3 WindFrequency=0.25 GustStrengthMultiplier=1.6 GustDirectionVariance=15; "
            "Fresh Wind: WindStrength=0.6 WindFrequency=0.4 GustStrengthMultiplier=2.0 GustDirectionVariance=20; "
            "Strong Wind: WindStrength=1.0 WindFrequency=0.65 GustStrengthMultiplier=2.4 GustDirectionVariance=28; "
            "Storm: WindStrength=1.8 WindFrequency=1.0 GustStrengthMultiplier=2.6 GustDirectionVariance=35; "
            "Hurricane: WindStrength=3.2 WindFrequency=1.6 GustStrengthMultiplier=2.8 GustDirectionVariance=45. "
            "Note: WindDirection and UseGustInteractor are not touched by presets. If any other value is changed manually, the preset name becomes 'Custom'.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "string getPresetName()", "Gets the currently set preset name.");

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setWindDirection(Vector3 direction)",
            "Sets the dominant world-space wind direction (automatically normalized). "
            "(1,0,0) = east, (-1,0,0) = west, (0,0,1) = south, (0,0,-1) = north. The Y component has no visual effect.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "Vector3 getWindDirection()", "Gets the current normalized wind direction vector.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setWindStrength(float strength)", "Sets the base ambient wind displacement strength (0..5). Typical range: 0..2.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getWindStrength()", "Gets the current base wind strength.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setWindFrequency(float frequency)", "Sets the animation speed of the 3D Perlin turbulence field (0.1..10).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getWindFrequency()", "Gets the current turbulence animation speed.");

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setSineModulationEnabled(bool enabled)", "Enables/disables the slow continuous breathing sway layered on top of WindStrength.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "bool getSineModulationEnabled()", "Gets whether the breathing sway is enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setSineModulationAmplitude(float amplitude)", "Sets the breathing sway amplitude as a fraction of WindStrength (0..1).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getSineModulationAmplitude()", "Gets the breathing sway amplitude.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setSineModulationFrequency(float frequency)", "Sets the breathing sway speed in Hz, full sway cycles per second (0.01..5).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getSineModulationFrequency()", "Gets the breathing sway speed.");

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustEnabled(bool enabled)", "Enables/disables randomized gust impulses on top of WindStrength.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "bool getGustEnabled()", "Gets whether gusts are enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustMinInterval(float seconds)", "Sets the minimum random delay in seconds before the next gust may start (0.5..120).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustMinInterval()", "Gets the minimum gust interval.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustMaxInterval(float seconds)", "Sets the maximum random delay in seconds before the next gust may start (0.5..120).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustMaxInterval()", "Gets the maximum gust interval.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustDuration(float seconds)",
            "Sets how long a single gust lasts in seconds (0.2..30). Strength and the travelling push rise and fall smoothly across this duration.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustDuration()", "Gets the gust duration.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustStrengthMultiplier(float multiplier)", "Sets how many times WindStrength is multiplied at the peak of a gust (1..6).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustStrengthMultiplier()", "Gets the gust strength multiplier.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustDirectionVariance(float degrees)",
            "Sets the maximum random yaw deviation in degrees of a gust's direction from WindDirection (0..90). A new random value is picked per gust.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustDirectionVariance()", "Gets the gust direction variance.");

        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setUseGustInteractor(bool useGustInteractor)",
            "Sets whether one of the WIND_MAX_INTERACTORS (8) HlmsWind slots is occupied to feed a travelling push position "
            "into the foliage while a gust is active. Disable to free the slot up for WindInteractionComponent instances.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "bool getUseGustInteractor()", "Gets whether the gust front interactor slot is in use.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustPushStrength(float strength)", "Sets the peak foliage push magnitude of the gust front interactor (0..5).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustPushStrength()", "Gets the gust front push strength.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustRadius(float radius)", "Sets the radius in world units of the gust front interactor (0.5..80).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustRadius()", "Gets the gust front interactor radius.");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "void setGustTravelSpeed(float speed)", "Sets the speed in world units per second at which the gust front interactor travels away from the owner GameObject (0..50).");
        LuaScriptApi::getInstance()->addClassToCollection("WindEffectComponent", "float getGustTravelSpeed()", "Gets the gust front travel speed.");

        gameObjectClass.def("getWindEffectComponentFromName", &getWindEffectComponentFromName);
        gameObjectClass.def("getWindEffectComponent", (WindEffectComponent * (*)(GameObject*)) & getWindEffectComponent);
        gameObjectClass.def("getWindEffectComponent2", (WindEffectComponent * (*)(GameObject*, unsigned int)) & getWindEffectComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WindEffectComponent getWindEffectComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurrence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WindEffectComponent getWindEffectComponent()", "Gets the component. Use this if the game object has this component only once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "WindEffectComponent getWindEffectComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castWindEffectComponent", &GameObjectController::cast<WindEffectComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "WindEffectComponent castWindEffectComponent(WindEffectComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end