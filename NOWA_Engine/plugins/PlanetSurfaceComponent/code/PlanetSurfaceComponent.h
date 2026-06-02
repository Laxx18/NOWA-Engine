#ifndef PLANET_SURFACE_COMPONENT_H
#define PLANET_SURFACE_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    /**
     * @class PlanetSurfaceComponent
     * @brief Lightweight label component that marks a GameObject as belonging to a
     *        specific planet or moon managed by UniversumComponent.
     *
     * Place this component on buildings, props, or any GameObject that sits on a
     * planet surface. Set TargetPlanetId to the Id of the planet it is placed on.
     *
     * UniversumComponent scans for these at simulation start, hides all of them,
     * and shows/repositions them only when the player is on that planet's surface.
     * On simulation stop, all objects are restored to their original world positions.
     */
    class EXPORTED PlanetSurfaceComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<PlanetSurfaceComponent> PlanetSurfaceCompPtr;

    public:
        static const Ogre::String AttrTargetPlanetId(void)
        {
            return "Target Planet Id";
        }

    public:
        PlanetSurfaceComponent();
        virtual ~PlanetSurfaceComponent();

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
            return NOWA::getIdFromName("PlanetSurfaceComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlanetSurfaceComponent";
        }

        virtual unsigned int getClassId(void) const override
        {
            return NOWA::getIdFromName("PlanetSurfaceComponent");
        }

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Marks this GameObject as a surface object belonging to a specific planet or moon "
                   "managed by UniversumComponent.\n"
                   "Usage: Place buildings, props, or any object on a planet in the editor. "
                   "Add PlanetSurfaceComponent and set Target Planet Id to the planet's Id "
                   "(visible in the Properties panel).\n"
                   "UniversumComponent hides this object when the planet is orbiting and "
                   "shows/repositions it when the player enters the planet's atmosphere zone. "
                   "On simulation stop all objects are restored to their original positions.";
        }

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static bool canStaticAddComponent(GameObject* gameObject);

        static void createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass);

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();

        // ---- Setter / Getter -------------------------------------------------
        void setTargetPlanetId(unsigned long id);
        unsigned long getTargetPlanetId(void) const;

    private:
        Ogre::String name;
        Variant* targetPlanetId;
    };

} // namespace NOWA

#endif // PLANET_SURFACE_COMPONENT_H