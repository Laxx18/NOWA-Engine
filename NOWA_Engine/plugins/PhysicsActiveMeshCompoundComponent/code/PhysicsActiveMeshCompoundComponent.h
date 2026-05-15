#ifndef PHYSICS_ACTIVE_MESH_COMPOUND_COMPONENT_H
#define PHYSICS_ACTIVE_MESH_COMPOUND_COMPONENT_H

#include "utilities/rapidxml.hpp"
#include "gameobject/PhysicsActiveCompoundComponent.h"
#include "Vao/OgreVertexArrayObject.h"
#include "OgrePlugin.h"

namespace NOWA
{
    /**
     * @class PhysicsActiveMeshCompoundComponent
     * @brief Automatically builds a compound convex physics collision hull by reading
     *        the vertex positions directly from the GameObject's Ogre::Item mesh.
     *
     * No XML config file or manual CollisionData list is required.  The component
     * reads mesh vertices at initialisation time, decomposes them into one or more
     * convex hulls according to the chosen mode, and assembles a Newton
     * CompoundCollision from the result.
     *
     * Decomposition modes
     * ───────────────────
     *   Single Hull   – every vertex feeds one convex hull.
     *                   Fastest; good enough for roughly-convex meshes.
     *   Per Sub-Mesh  – each submesh produces its own hull.
     *                   Best when the mesh has spatially separate regions.
     *   Octant 2×2×2  – AABB split into 8  cells; one hull per non-empty cell.
     *   Octant 3×3×3  – AABB split into 27 cells; one hull per non-empty cell.
     *                   Trades hull count for better concave approximation.
     *
     * Render-thread safety
     * ────────────────────
     * Vertex extraction uses Ogre-Next readRequests / mapAsyncTickets and Newton
     * CollisionPrimitive creation – both must run on the render thread.
     * The component uses enqueueAndWait() for those steps and creates the Newton
     * Body afterwards on the calling (main) thread, identical to the parent.
     */
    class EXPORTED PhysicsActiveMeshCompoundComponent : public PhysicsActiveCompoundComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<PhysicsActiveMeshCompoundComponent> PhysicsActiveMeshCompoundCompPtr;

    public:
        PhysicsActiveMeshCompoundComponent();
        virtual ~PhysicsActiveMeshCompoundComponent();

        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override;
        virtual void shutdown() override;
        virtual void uninstall() override;
        virtual const Ogre::String& getName() const override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // ── Plugin / component lifecycle ──────────────────────────────────────
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual Ogre::String getParentParentClassName(void) const override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual void actualizeValue(Variant* attribute) override;

        virtual bool isMovable(void) const override
        {
            return true;
        }

        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        // ── Static registration ───────────────────────────────────────────────
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PhysicsActiveMeshCompoundComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "PhysicsActiveMeshCompoundComponent";
        }
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Automatically builds a compound convex physics collision hull "
                   "from the mesh of the GameObject's Ogre::Item.\n\n"
                   "No XML config file or manual collision shapes are required.\n"
                   "Mesh vertices are read at initialisation and decomposed into one or "
                   "more convex hulls based on the chosen Decomposition Mode.\n\n"
                   "DECOMPOSITION MODES:\n"
                   "- Single Hull  : All mesh vertices form one convex hull.\n"
                   "  Best for roughly convex shapes (crates, barrels, boulders).\n"
                   "- Per Sub-Mesh : Each submesh gets its own hull.\n"
                   "  Good when the mesh has spatially separate parts.\n"
                   "- Octant 2x2x2 : AABB split into 2x2x2 = 8 cells.\n"
                   "- Octant 3x3x3 : AABB split into 3x3x3 = 27 cells.\n"
                   "  Higher octant counts give better concave approximation at the cost "
                   "of more hull primitives and slightly higher simulation cost.\n\n"
                   "Hull Tolerance: Newton vertex-merging threshold (default 0.001).\n"
                   "  Lower = more precise hull geometry.\n"
                   "  Higher = simplified/smaller hull vertex count.\n\n"
                   "NOTE: The component reads the mesh on the render thread using Ogre-Next "
                   "async tickets. The GO must have an Ogre::Item attached.";
        }

        // ── Attribute setters / getters ───────────────────────────────────────
        void setConvexDecompositionMode(const Ogre::String& mode);
        Ogre::String getConvexDecompositionMode(void) const;

        void setHullTolerance(Ogre::Real tolerance);
        Ogre::Real getHullTolerance(void) const;

    public:
        // ── Static attribute name strings ─────────────────────────────────────
        static Ogre::String AttrConvexDecompositionMode(void)
        {
            return "Decomposition Mode";
        }
        static Ogre::String AttrHullTolerance(void)
        {
            return "Hull Tolerance";
        }

    protected:
        /**
         * @brief   Overrides the parent to build collision hulls from mesh vertices
         *          instead of reading an XML config file or an inline CollisionData list.
         */
        virtual bool createDynamicBody(void) override;

    private:
        // ── Render-thread mesh extraction ─────────────────────────────────────

        /**
         * @brief Reads position data for every submesh via Ogre-Next async tickets.
         *        Must be called on the render thread.
         * @param outVerticesPerSubMesh One entry per submesh, each holding the local
         *                              vertex positions (NOT scaled by the GO transform).
         * @return True if at least one vertex was extracted.
         */
        bool extractVerticesFromMesh(std::vector<std::vector<Ogre::Vector3>>& outVerticesPerSubMesh);

        /**
         * @brief Detects whether the VAO's position stream is stored as HALF4 or FLOAT3
         *        and reads one vertex accordingly.  Advances the data pointer by one stride.
         */
        Ogre::Vector3 readPositionVertex(const Ogre::uint8*& data, Ogre::VertexElementType posType, size_t stride) const;

        // ── Convex decomposition ──────────────────────────────────────────────

        /**
         * @brief Orchestrates vertex extraction and hull creation in a single
         *        render-thread command.  Must be called on the render thread.
         * @param collisionList Receives the created OgreNewt collision primitives.
         * @param partVolume    Accumulated volume of all created hulls.
         * @param inertia       Inertia vector updated by the hulls (passed through to body).
         * @return True if at least one hull was created.
         */
        bool buildCollisionListFromMesh(std::vector<OgreNewt::CollisionPtr>& collisionList, Ogre::Real& partVolume, Ogre::Vector3& inertia);

        /**
         * @brief Splits a flat vertex cloud into groups by a uniform grid.
         *        Vertices whose cell has fewer than 4 points are merged into the
         *        nearest populated cell so no hull is silently discarded.
         * @param allVertices  Source vertex positions (already world-scaled).
         * @param divisions    Grid side length (2 → 2³=8 cells, 3 → 3³=27 cells).
         * @param outGroups    One entry per non-empty cell with ≥ 4 vertices.
         */
        void splitVerticesByOctant(const std::vector<Ogre::Vector3>& allVertices, int divisions, std::vector<std::vector<Ogre::Vector3>>& outGroups);

        /**
         * @brief Attempts to create a ConvexHull from a vertex cloud.
         *        Logs a warning and returns nullptr if Newton rejects the hull
         *        (e.g. degenerate/coplanar points).
         */
        OgreNewt::CollisionPtr tryCreateConvexHull(const std::vector<Ogre::Vector3>& vertices, Ogre::Real tolerance);

    private:
        Variant* convexDecompositionMode;
        Variant* hullTolerance;
        Ogre::String name;
    };

} // namespace NOWA

#endif // PHYSICS_ACTIVE_MESH_COMPOUND_COMPONENT_H
