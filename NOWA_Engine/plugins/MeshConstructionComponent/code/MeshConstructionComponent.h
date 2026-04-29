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
     * Editor:  original Ogre::Item untouched.
     *
     * Simulation (connect() + activated):
     *  - Original Item swapped for a BT_DYNAMIC_DEFAULT copy.
     *  - Triangles sorted by centroid Y at connect() time.
     *  - Each update() tick the index buffer is rebuilt: triangles with
     *    centroid <= cutoffY are real indices; those above are degenerate
     *    [0,0,0]. Vertex buffer is never touched → no squish artefact.
     *  - Camera-facing progress bar (optional) using exactly the
     *    ValueBarComponent drawing convention.
     *  - Camera-facing percentage text (optional) via MovableText +
     *    addTrackedNode, same as GameObjectTitleComponent.
     *  - On 100%: final blocking index upload → setActivated(false) →
     *    Lua closure via AppStateManager::enqueue().
     *
     * disconnect() restores the original Item (enqueueAndWait).
     * Crash-safe: all render-resource cleanup uses enqueueAndWait.
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
                   "Optional camera-facing progress bar and percentage text.";
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
         * @brief  If true the mesh deconstructs top-to-bottom instead of
         *         building bottom-to-top.  Useful for demolition scenarios.
         */
        void setInvert(bool invert);
        bool getInvert(void) const;

        /**
         * @brief  Construction axis. "X" = left→right, "Y" = bottom→top,
         *         "Z" = front→back. Invert reverses the direction.
         *         Changing this only takes effect on the next connect().
         */
        void setConstructionAxis(const Ogre::String& axis);
        Ogre::String getConstructionAxis(void) const;

        /** Lua callback, fired at 100%. Signature: function() */
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
        static const Ogre::String AttrInvert()
        {
            return "Invert";
        }
        static const Ogre::String AttrConstructionAxis()
        {
            return "Construction Axis";
        }

    private:
        struct SubMeshInfo
        {
            size_t vertexOffset = 0;
            size_t vertexCount = 0;
            size_t indexOffset = 0;
            size_t indexCount = 0;
            bool hasTangent = false;
            size_t floatsPerVertex = 8;
            Ogre::VertexBufferPacked* vertexBuffer = nullptr; // BT_DYNAMIC_DEFAULT
            Ogre::IndexBufferPacked* indexBuffer = nullptr;   // BT_DYNAMIC_DEFAULT
            std::vector<size_t> sortedTriIndices;
            std::vector<float> sortedCentroid; ///< centroid along the chosen construction axis
        };

        // All render-thread helpers:
        bool extractMeshDataAndCreateBuffers(void);
        void destroyConstructionResources(void);
        void createOverlays(void);
        void destroyOverlays(void);

        // Shared by connect/disconnect/onRemove — always enqueueAndWait.
        void restoreOriginalItemAndCleanup(void);

        // Uploads the final index state synchronously (true=all visible, false=all degenerate).
        void uploadFinalIndicesBlocking(bool makeAllVisible);

        /// Returns the axis component (x/y/z) of a vector based on the current axis variant.
        float axisComponent(const Ogre::Vector3& v) const;

        Ogre::String trackedClosureId(void) const;

    private:
        Ogre::String componentName;
        Ogre::String originalMeshName;

        // Local-space Y extents — used for progress bar/text positioning (always Y).
        Ogre::Real meshMinY;
        Ogre::Real meshMaxY;
        // Local-space extents along the chosen construction axis.
        Ogre::Real meshAxisMin;
        Ogre::Real meshAxisMax;

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

        // ── Overlays ───────────────────────────────────────────────────────────
        // All nodes are children of the root scene node (no inherited transform).
        Ogre::SceneNode* barLineNode;
        Ogre::ManualObject* barObject;
        bool barCouldDraw; ///< ValueBarComponent couldDraw pattern

        Ogre::SceneNode* textLineNode;
        MovableText* percentageText;

        // Logic-thread state
        Ogre::Real elapsed;
        Ogre::Real constructionProgress;
        bool isConstructionFinished;

        luabind::object closureFunction;

        Variant* activated;
        Variant* constructionTime;
        Variant* showProgressBar;
        Variant* showPercentageText;
        Variant* invert;
        Variant* constructionAxis;
    };

} // namespace NOWA

#endif // MESHCONSTRUCTIONCOMPONENT_H