#ifndef PLAYER_START_COMPONENT_H
#define PLAYER_START_COMPONENT_H

#include "GameObjectComponent.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    /**
     * @class 	PlayerStartComponent
     * @brief 	Place it on a (non global) marker game object in a level and set 'Target Id' to a global game object, e.g. the player.
     *			When the scene is loaded - in the game as well as in NOWA-Design - the target is moved to the marker's transform.
     *			Every game object attached to the target via a TagPointComponent (e.g. a weapon in the hand) is moved along,
     *			keeping its transform relative to the target.
     *
     *			When it places the target:
     *			- The scene was opened directly (NOWA-Design, first scene of a game): always. One spawn point per level is enough,
     *			  because arrivals through an exit are handled by the exits ("doors") themselves.
     *			- The scene was entered through an ExitComponent: only if its game object NAME equals the exit's 'Target Location Name'.
     *			  Usually that name belongs to the matching exit of the target scene, which places the player itself (ExitComponent::applyArrival).
     *			- Nothing in the scene has the requested name (typo): as a fallback, with a warning in the log.
     */
    class EXPORTED PlayerStartComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<PlayerStartComponent> PlayerStartCompPtr;

    public:
        PlayerStartComponent();

        virtual ~PlayerStartComponent();

        /////////////////////////////////////////////////////////////////////////////

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @brief Moves the target (and everything attached to it) to this spawn point. Called by the DotSceneImportModule after all other
         *		  post inits, so the global target already exists.
         */
        virtual bool postInit(void) override;

        virtual bool connect(void) override;

        virtual bool disconnect(void) override;

        virtual bool onCloned(void) override;

        virtual void update(Ogre::Real dt, bool notSimulating) override;

        virtual void actualizeValue(Variant* attribute) override;

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see		GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlayerStartComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlayerStartComponent";
        }

        /////////////////////////////////////////////////////////////////////////////

        void setActivated(bool activated);

        bool isActivated(void) const;

        void setTargetId(unsigned long targetId);

        unsigned long getTargetId(void) const;

        /////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Moves the target to this spawn point, plus every game object attached to the target via a TagPointComponent,
         *		  with the same rigid transform, so that attachments keep their pose relative to the target.
         * @return true, if the target exists and was moved.
         */
        bool applyToTarget(void);

        /**
         * @brief Places the target at the given transform, plus every game object attached to the target via a TagPointComponent, with the
         *		  same rigid transform. Shared by PlayerStartComponent and ExitComponent (arrival at a door).
         * @return true, if the target is not null.
         */
        static bool placeWithAttachments(GameObject* targetGameObject, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        /////////////////////////////////////////////////////////////////////////////

        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrTargetId(void)
        {
            return "TargetId";
        }

    private:
        /**
         * @brief Whether this spawn point is the one to use for the current scene load, see the class description.
         */
        bool isSelected(void) const;

        /**
         * @brief Places one game object, taking its physics body into account (dynamic, kinematic or none).
         */
        static void placeGameObject(GameObject* gameObject, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

    private:
        Variant* activated;
        Variant* targetId;
    };

}; // namespace end

#endif