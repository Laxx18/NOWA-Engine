/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef MESHCONSTRUCTIONCOMPONENT_H
#define MESHCONSTRUCTIONCOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

namespace Ogre
{
    class VertexBufferPacked;
    class IndexBufferPacked;
    class ManualObject;
}

namespace NOWA
{
    class MovableText;
}

namespace NOWA
{
    /**
     * @class   MeshConstructionComponent
     * @brief   Warcraft-2-style mesh construction animation.
     *
     * In the editor the original Ogre::Item is never touched.
     *
     * When simulation starts (connect()) and activated == true:
     *  - Original Item is swapped for a BT_DYNAMIC_DEFAULT copy.
     *  - Triangles are sorted by centroid Y at connect() time.
     *  - Each update() tick the index buffer is rebuilt:
     *      triangles with centroid <= cutoffY  → real indices (visible)
     *      triangles with centroid >  cutoffY  → degenerate [0,0,0] (hidden)
     *    This eliminates the "squish/scale-up" artefact completely.
     *  - Optionally a camera-facing progress bar and/or percentage text above
     *    the mesh are shown, following the ValueBarComponent / SpeechBubble
     *    drawing conventions exactly.
     *  - A glowing ring marks the construction front at the current cutoffY.
     *  - On completion: setActivated(false) + Lua closure via
     *    AppStateManager::enqueue() (logic thread).
     *
     * disconnect() restores the original Item.
     */
    class EXPORTED MeshConstructionComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<MeshConstructionComponent> MeshConstructionComponentPtr;

    public:
        MeshConstructionComponent();
        virtual ~MeshConstructionComponent();

        // ── Ogre::Plugin ──────────────────────────────────────────────────────
        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override
        {
        }
        virtual void shutdown() override
        {
        }
        virtual void uninstall() override
        {
        }
        virtual const Ogre::String& getName() const override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // ── GameObjectComponent ───────────────────────────────────────────────
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual void onRemoveComponent(void) override;
        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        virtual void setActivated(bool activated) override;
        virtual bool isActivated(void) const override;

    public:
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("MeshConstructionComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "MeshConstructionComponent";
        }
        static bool canStaticAddComponent(GameObject* gameObject);
        static Ogre::String getStaticInfoText(void)
        {
            return "Warcraft-2-style bottom-to-top mesh construction animation. "
                   "Simulation-only — editor always shows the full mesh. "
                   "Optionally shows a progress bar and/or percentage text.";
        }
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        void setConstructionTime(Ogre::Real seconds);
        Ogre::Real getConstructionTime(void) const;

        void setShowProgressBar(bool show);
        bool getShowProgressBar(void) const;

        void setShowPercentageText(bool show);
        bool getShowPercentageText(void) const;

        Ogre::Real getConstructionProgress(void) const;
        bool getIsConstructionFinished(void) const;

        /**
         * @brief  Lua callback fired when construction reaches 100 %.
         *         Signature: function()
         */
        void reactOnConstructionDone(luabind::object closureFunction);

    public:
        static const Ogre::String AttrActivated()
        {
            return "Activated";
        }
        static const Ogre::String AttrConstructionTime()
        {
            return "Construction Time (s)";
        }
        static const Ogre::String AttrShowProgressBar()
        {
            return "Show Progress Bar";
        }
        static const Ogre::String AttrShowPercentageText()
        {
            return "Show Percentage Text";
        }

    private:
        struct SubMeshInfo
        {
            size_t vertexOffset = 0;
            size_t vertexCount = 0;
            size_t indexOffset = 0; ///< into this->indices[]
            size_t indexCount = 0;
            bool hasTangent = false;
            size_t floatsPerVertex = 8;

            Ogre::VertexBufferPacked* vertexBuffer = nullptr; ///< BT_DYNAMIC_DEFAULT
            Ogre::IndexBufferPacked* indexBuffer = nullptr;   ///< BT_DYNAMIC_DEFAULT

            // Built once at connect(); binary-searched every update().
            std::vector<size_t> sortedTriIndices; ///< triangle# in ascending centroid-Y order
            std::vector<float> sortedCentroidY;   ///< parallel ascending centroid-Y values
        };

        bool extractMeshDataAndCreateBuffers(void); ///< render thread
        void destroyConstructionResources(void);    ///< render thread
        void createOverlays(void);                  ///< render thread
        void destroyOverlays(void);                 ///< render thread

        Ogre::String trackedClosureId(void) const;

    private:
        Ogre::String componentName;

        // Read in postInit from getBounds() — refined from actual vertices in connect().
        Ogre::String originalMeshName;
        Ogre::Real meshMinY;
        Ogre::Real meshMaxY;
        Ogre::Real meshXZRadius; ///< max distance of any vertex from the Y-axis

        std::vector<Ogre::Vector3> vertices;
        std::vector<Ogre::Vector3> normals;
        std::vector<Ogre::Vector4> tangents;
        std::vector<Ogre::Vector2> uvCoordinates;
        std::vector<Ogre::uint32> indices;
        size_t vertexCount;
        size_t indexCount;
        std::vector<SubMeshInfo> subMeshInfoList;

        Ogre::Item* constructionItem;
        Ogre::MeshPtr constructionMesh;

        // ── Progress bar overlay ──────────────────────────────────────────────
        // lineNode is a child of root scene node (no transform).
        // All vertex positions are supplied in world space inside the closure.
        // Material: WhiteNoLightingBackground (same as ValueBarComponent).
        Ogre::SceneNode* barLineNode;
        Ogre::ManualObject* barObject;
        bool barCouldDraw; ///< guards beginUpdate vs begin

        // ── Percentage text ───────────────────────────────────────────────────
        // textNode is a child of the game-object's scene node.
        // addTrackedNode() makes the render thread orient it toward the camera.
        Ogre::SceneNode* textNode;
        MovableText* percentageText;

        // ── Glow ring ─────────────────────────────────────────────────────────
        Ogre::SceneNode* glowLineNode;
        Ogre::ManualObject* glowObject;
        bool glowCouldDraw;

        // Logic-thread state
        Ogre::Real elapsed;
        Ogre::Real constructionProgress;
        bool isConstructionFinished;

        luabind::object closureFunction;

        Variant* activated;
        Variant* constructionTime;
        Variant* showProgressBar;
        Variant* showPercentageText;
    };

} // namespace NOWA

#endif // MESHCONSTRUCTIONCOMPONENT_H