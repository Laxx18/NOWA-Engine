/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef BLINK_COMPONENT_H
#define BLINK_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "gameobject/GameObjectController.h"

namespace NOWA
{
    class PhysicsComponent;

    /**
     * @class BlinkComponent
     * @brief Cycles one or more GameObjects through a Solid -> FadeOut -> Gone -> FadeIn loop,
     *        like Mega Man blink platforms.
     *
     * Attach to any "manager" node. Configure GameObjectCount and assign the
     * GameObjectIds of the target objects (they must have an Item as movable object
     * and optionally a PhysicsComponent). Each target is driven independently through
     * the four phases using per-entry timing Variants.
     *
     * Preset modes (BlinkMode):
     *   "Single"        — one object, simple on/off cycling.
     *   "Sequential"    — objects blink out one after the other with an offset delay.
     *   "Alternating"   — even-index objects and odd-index objects alternate.
     *   "AllAtOnce"     — every object blinks in sync.
     *
     * Lua example:
     * @code
     *   local bc = go:getBlinkComponent()
     *   bc:setBlinkMode("Sequential")
     *   bc:setGameObjectCount(3)
     *   bc:setGameObjectId(0, "12345678")
     *   bc:setStartDelay(0, 0.0)
     *   bc:setSolidTime(0, 2.0)
     *   bc:setFadeOutTime(0, 0.5)
     *   bc:setGoneTime(0, 1.0)
     *   bc:setFadeInTime(0, 0.5)
     * @endcode
     */
    class EXPORTED BlinkComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<BlinkComponent> BlinkCompPtr;

        // Internal per-object phase
        enum class Phase
        {
            Delay,
            Solid,
            FadeOut,
            Gone,
            FadeIn
        };

    public:
        BlinkComponent();

        virtual ~BlinkComponent();

        // ----- Ogre::Plugin -----
        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see		Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see		Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see		Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // ----- GameObjectComponent identity -----
        virtual unsigned int getClassId(void) const override
        {
            return BlinkComponent::getStaticClassId();
        }

        virtual Ogre::String getClassName(void) const override
        {
            return BlinkComponent::getStaticClassName();
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("BlinkComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "BlinkComponent";
        }

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Attach to any manager GameObject. Assign GameObjectIds of the target "
                   "objects (they need an Item movable object). Choose a BlinkMode and tune "
                   "SolidTime, FadeOutTime, GoneTime and FadeInTime per entry. "
                   "Optionally set StartDelay to offset individual platforms in Sequential mode.";
        }

        // ----- lifecycle -----
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void onRemoveComponent(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual void update(Ogre::Real dt, bool notSimulating) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
        static bool canStaticAddComponent(GameObject* gameObject);

        // ----- global settings -----
        void setBlinkMode(const Ogre::String& mode);
        Ogre::String getBlinkMode(void) const;

        void setGameObjectCount(unsigned int count);
        unsigned int getGameObjectCount(void) const;

        // ----- per-entry settings -----
        void setGameObjectId(unsigned int index, const Ogre::String& id);
        Ogre::String getGameObjectId(unsigned int index) const;

        void setStartDelay(unsigned int index, Ogre::Real delay);
        Ogre::Real getStartDelay(unsigned int index) const;

        void setSolidTime(unsigned int index, Ogre::Real t);
        Ogre::Real getSolidTime(unsigned int index) const;

        void setFadeOutTime(unsigned int index, Ogre::Real t);
        Ogre::Real getFadeOutTime(unsigned int index) const;

        void setGoneTime(unsigned int index, Ogre::Real t);
        Ogre::Real getGoneTime(unsigned int index) const;

        void setFadeInTime(unsigned int index, Ogre::Real t);
        Ogre::Real getFadeInTime(unsigned int index) const;

    public:
        // ----- Attr keys -----
        static const Ogre::String AttrBlinkMode(void)
        {
            return "Blink Mode";
        }
        static const Ogre::String AttrGameObjectCount(void)
        {
            return "GameObject Count";
        }
        static const Ogre::String AttrGameObjectId(void)
        {
            return "GameObject Id";
        }
        static const Ogre::String AttrStartDelay(void)
        {
            return "Start Delay";
        }
        static const Ogre::String AttrSolidTime(void)
        {
            return "Solid Time";
        }
        static const Ogre::String AttrFadeOutTime(void)
        {
            return "Fade Out Time";
        }
        static const Ogre::String AttrGoneTime(void)
        {
            return "Gone Time";
        }
        static const Ogre::String AttrFadeInTime(void)
        {
            return "Fade In Time";
        }

    private:
        // helpers
        void enterPhase(unsigned int index, Phase newPhase);
        void applyTransparency(unsigned int index, Ogre::Real alpha);
        void restoreAllTargets(void);

        // Runtime cache resolved in postInit / connect
        struct TargetEntry
        {
            GameObjectPtr gameObjectPtr;
            Ogre::Item* item = nullptr;
            PhysicsComponent* physics = nullptr;
            Phase phase = Phase::Delay;
            Ogre::Real timer = 0.0f;
            std::vector<std::pair<Ogre::HlmsDatablock*, unsigned int>> clonedDatablocks;
        };

        void resolveTargets(void);

    private:
        Ogre::String name;

        // ----- fixed Variants -----
        Variant* blinkMode;
        Variant* gameObjectCount;

        // ----- dynamic Variant arrays (one entry per target) -----
        std::vector<Variant*> gameObjectIds;
        std::vector<Variant*> startDelays;
        std::vector<Variant*> solidTimes;
        std::vector<Variant*> fadeOutTimes;
        std::vector<Variant*> goneTimes;
        std::vector<Variant*> fadeInTimes;

        // ----- runtime state -----
        std::vector<TargetEntry> targets;
    };

} // namespace NOWA

#endif
