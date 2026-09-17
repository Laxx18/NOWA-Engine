/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_PLATFORM_BOUNDARY_COMPONENT_H
#define PROCEDURAL_PLATFORM_BOUNDARY_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/PlatformComponentBase.h"
#include "main/Events.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralPlatformBoundaryComponent
     * @brief Fixed-size level boundary for a 2.5D jump'n'run: a rectangular shell of floor,
     *        ceiling and two side walls, with doorways cut out on a one-meter grid.
     *
     * This is the deliberately simplified sibling of ProceduralPlatformComponent. That component
     * is an interactive drawing tool: the designer drags paths with the mouse, the geometry
     * follows a Catmull-Rom curve, and the resulting shape is whatever was drawn. Useful for the
     * platforms INSIDE a level, wrong for the level's outer bounds.
     *
     * A level boundary has the opposite requirements, which is why this is a separate component
     * rather than a mode of the other one:
     *
     *  - Its size must be an exact, typed-in number. A jump'n'run whose levels are chained
     *    together and shown on a minimap needs 100 x 20 meters to mean exactly that, which a
     *    mouse drag can never guarantee.
     *  - There is no interactive mode at all. Every dimension is a property; changing one throws
     *    the whole boundary away and regenerates it.
     *  - The only editing operation is removing whole one-meter cells to form doorways, so a
     *    level can open into the next one. Cell removal is snapped to the meter grid by
     *    construction, not by rounding a mouse position afterwards.
     *
     * What it shares with ProceduralPlatformComponent, on purpose: the dual Surface/Ground
     * datablock split, the same mesh buffer layout, the same segment-overlay style for showing
     * the selection, the same undo/redo transaction events, and the same PhysicsArtifactComponent
     * hand-off for collision.
     *
     * What it deliberately does not have: grass, trees, junctions, depth nudging, curves,
     * smoothing, and any form of mouse-driven creation.
     *
     * COORDINATE MODEL
     *
     * The boundary is built in the GameObject's local space, with its lower-left-front corner at
     * the origin:
     *
     *   x: 0 .. Boundary Width   (length of the level, the direction the player runs)
     *   y: 0 .. Boundary Height  (floor at y = 0, ceiling at y = Height)
     *   z: -Depth/2 .. +Depth/2  (centred, so the play plane at z = 0 stays the middle)
     *
     * Floor and ceiling span the full width. The side walls fill only the gap between them, so
     * the four sides meet without overlapping geometry at the corners.
     */
    class EXPORTED ProceduralPlatformBoundaryComponent : public PlatformComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<ProceduralPlatformBoundaryComponent> ProceduralPlatformBoundaryCompPtr;

    public:
        /**
         * @brief Which of the four sides of the boundary shell a cell belongs to.
         *
         * FLOOR and CEILING are indexed along x, LEFT and RIGHT along y. Keeping the side as an
         * explicit enum rather than flattening everything into one running index means a cell
         * survives a resize in the only way that makes sense: a doorway on the floor at x = 12
         * is still a doorway on the floor at x = 12 after the level is made taller.
         */
        enum class BoundarySide
        {
            FLOOR = 0,
            CEILING = 1,
            LEFT = 2,
            RIGHT = 3
        };

        /**
         * @brief One removable one-meter cell of the boundary shell.
         *
         * Ordered so it can live in a std::set: removals are a set, not a list, because removing
         * the same cell twice has to be a no-op and lookup during mesh generation happens once
         * per cell.
         */
        struct BoundaryCell
        {
            BoundarySide side = BoundarySide::FLOOR;
            int index = 0;

            bool operator<(const BoundaryCell& other) const
            {
                if (this->side != other.side)
                {
                    return static_cast<int>(this->side) < static_cast<int>(other.side);
                }
                return this->index < other.index;
            }

            bool operator==(const BoundaryCell& other) const
            {
                return this->side == other.side && this->index == other.index;
            }
        };

        // Same two-buffer split as ProceduralPlatformComponent: SURFACE is the face pointing into
        // the play area (what the player sees and walks on), GROUND is everything else - the
        // outer shell and the side faces of a doorway.
        enum class BoundaryMeshBuffer
        {
            SURFACE,
            GROUND
        };

        enum class EditMode
        {
            OBJECT = 0,
            SEGMENT = 1
        };

    public:
        ProceduralPlatformBoundaryComponent();

        virtual ~ProceduralPlatformBoundaryComponent();

        // Ogre::Plugin
        virtual const Ogre::String& getName() const override;

        virtual void install(const Ogre::NameValuePairList* options) override;

        virtual void shutdown() override;

        virtual void uninstall() override;

        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // OIS listeners
        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool keyPressed(const OIS::KeyEvent& evt) override;

        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

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
         * @see		GameObjectComponent::onAddComponent
         */
        virtual void onAddComponent(void) override;

        /**
         * @see GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void) override;

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        /**
         * @see GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralPlatformBoundaryComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "PlatformComponentBase";
        }

        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralPlatformBoundaryComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralPlatformBoundaryComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates a fixed-size rectangular level boundary for 2.5D jump'n'run levels - floor, ceiling and two side walls,\n"
                   "sized in exact meters so levels can be chained and shown on a minimap.\n\n"

                   "SIZE:\n"
                   "- 'Boundary Width', 'Boundary Height' and 'Boundary Depth' are the level's outer dimensions in meters.\n"
                   "  Defaults are 100 x 20 x 10.\n"
                   "- 'Wall Thickness' is how solid floor, ceiling and walls are. It does not change the outer size.\n"
                   "- Changing any of these regenerates the whole boundary from scratch. There is no mouse-driven editing:\n"
                   "  every dimension is typed in, so the numbers on the minimap are exactly the numbers in the level.\n\n"

                   "DOORWAYS (Segment Mode):\n"
                   "- Set 'Edit Mode' to 'Segment' to start cutting doorways.\n"
                   "- Left-click anywhere on the boundary to select the one-meter cell under the cursor. The selection snaps to\n"
                   "  the meter grid automatically and is drawn as an outline.\n"
                   "- Press X to remove the selected cell, creating a gap. Works anywhere: in a side wall for a level exit,\n"
                   "  in the ceiling for a shaft, in the floor for a pit.\n"
                   "- Press SHIFT+X on a removed cell to put it back.\n"
                   "- Removing several neighbouring cells widens a doorway; the geometry is merged, so a three-meter gap is one\n"
                   "  opening rather than three.\n"
                   "- Every removal and restore is a normal undo step (CTRL+Z).\n\n"

                   "MATERIALS:\n"
                   "- 'Surface Datablock' is used for the faces pointing into the level - what the player sees and walks on.\n"
                   "- 'Ground Datablock' is used for the outer shell and for the cut faces of a doorway.\n"
                   "- 'Surface UV Tiling' and 'Ground UV Tiling' scale the textures in meters.\n\n"

                   "COLLISION:\n"
                   "- Add a PhysicsArtifactComponent to the same game object and the boundary is rebuilt as a collision hull\n"
                   "  automatically whenever its geometry changes.\n\n"

                   "LUA API:\n"
                   "- getProceduralPlatformBoundaryComponent() on a GameObject returns this component.\n"
                   "- setBoundaryWidth(w), setBoundaryHeight(h), setBoundaryDepth(d) set the size in meters.\n"
                   "- removeCell(side, index) / restoreCell(side, index) cut or close a doorway; side is 0=floor, 1=ceiling,\n"
                   "  2=left, 3=right.\n"
                   "- clearAllCells() restores every removed cell.\n";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Platform Boundary";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = true;
            desc.autoComponents = {"ProceduralPlatformBoundaryComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

    public:
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrBoundaryWidth(void)
        {
            return "Boundary Width";
        }
        static Ogre::String AttrBoundaryHeight(void)
        {
            return "Boundary Height";
        }
        static Ogre::String AttrBoundaryDepth(void)
        {
            return "Boundary Depth";
        }
        static Ogre::String AttrWallThickness(void)
        {
            return "Wall Thickness";
        }
        static Ogre::String AttrSurfaceDatablock(void)
        {
            return "Surface Datablock";
        }
        static Ogre::String AttrGroundDatablock(void)
        {
            return "Ground Datablock";
        }
        static Ogre::String AttrSurfaceUVTiling(void)
        {
            return "Surface UV Tiling";
        }
        static Ogre::String AttrGroundUVTiling(void)
        {
            return "Ground UV Tiling";
        }
        static Ogre::String AttrEditMode(void)
        {
            return "Edit Mode";
        }
        static Ogre::String AttrRemovedCells(void)
        {
            return "Removed Cells";
        }

    public:
        void setActivated(bool activated) override;

        bool isActivated(void) const override;

        /**
         * @brief Serializes this boundary's full reconstructible state to a byte buffer:
         *        dimensions, wall thickness, both datablocks, both UV tilings, edit mode and the
         *        removed-cell set.
         *
         * This is the data-level interface PlatformComponentBase declares, so undo/redo (and any
         * other code holding only a PlatformComponentBase pointer) can snapshot and restore a
         * boundary without linking against this plugin - the same role
         * ProceduralPlatformComponent::getPlatformData plays there.
         *
         * Deliberately NOT the swept mesh. Unlike that component's hand-drawn path, a boundary's
         * geometry is a pure, cheap function of the values below - a handful of boxes, not a
         * curve - so setPlatformData restores the values and calls rebuildMesh() rather than
         * caching and replaying vertex buffers. That keeps every undo step a few dozen bytes
         * instead of a copy of the whole mesh.
         */
        virtual std::vector<unsigned char> getPlatformData(void) const override;

        virtual void setPlatformData(const std::vector<unsigned char>& data) override;

        virtual bool getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const override;

        void setBoundaryWidth(Ogre::Real width);

        Ogre::Real getBoundaryWidth(void) const;

        void setBoundaryHeight(Ogre::Real height);

        Ogre::Real getBoundaryHeight(void) const;

        void setBoundaryDepth(Ogre::Real depth);

        Ogre::Real getBoundaryDepth(void) const;

        void setWallThickness(Ogre::Real thickness);

        Ogre::Real getWallThickness(void) const;

        void setSurfaceDatablock(const Ogre::String& datablock);

        Ogre::String getSurfaceDatablock(void) const;

        void setGroundDatablock(const Ogre::String& datablock);

        Ogre::String getGroundDatablock(void) const;

        void setSurfaceUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getSurfaceUVTiling(void) const;

        void setGroundUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getGroundUVTiling(void) const;

        void setEditMode(const Ogre::String& editMode);

        Ogre::String getEditMode(void) const;

        /**
         * @brief Removes one cell, cutting a doorway.
         * @param[in] side  0 = floor, 1 = ceiling, 2 = left wall, 3 = right wall.
         * @param[in] index Cell index along that side, in meters from the origin.
         */
        void removeCell(int side, int index);

        void restoreCell(int side, int index);

        void clearAllCells(void);

        unsigned int getRemovedCellCount(void) const;

        /**
         * @brief Announces that this component is now the one editing its GameObject.
         *
         * Same mechanism as ProceduralPlatformComponent::claimEditFocus - see there. Queues an
         * EventDataEditorMode built with its claiming constructor, so every editing component on
         * the object stands down unless it is the named owner.
         */
        void claimEditFocus(void);

        bool isEditFocusOwner(void) const;

    private:
        EditMode getEditModeEnum(void) const;

        // ── Mesh generation ──────────────────────────────────────────────────────────────
        void rebuildMesh(void);

        void addBoundaryQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Coord, Ogre::Real v1Coord,
            BoundaryMeshBuffer targetBuffer);

        /**
         * @brief Emits one axis-aligned box, with the face named by innerNormal going to SURFACE.
         *
         * A run of kept cells always forms exactly such a box, which is why the whole boundary can
         * be built from this one primitive.
         */
        void addBoundaryBox(const Ogre::Vector3& minCorner, const Ogre::Vector3& maxCorner, const Ogre::Vector3& innerNormal);

        void createBoundaryMesh(void);

        void createBoundaryMeshInternal(const std::vector<float>& surfaceVerts, const std::vector<Ogre::uint32>& surfaceInds, size_t numSurfaceVerts, const std::vector<float>& groundVerts, const std::vector<Ogre::uint32>& groundInds,
            size_t numGroundVerts);

        void destroyBoundaryMesh(void);

        void updatePhysicsCollision(void);

        /**
         * @brief Recomputes this boundary's own world-space extents and broadcasts them as the
         *        current scene bounds.
         *
         * A level boundary IS the level's extents by definition, so this needs no whole-scene AABB
         * scan: it transforms its own local box (0,0,-depth/2)..(width,height,depth/2) by the game
         * object's node transform and fires the SAME EventDataBoundsUpdated event FollowCamera2D
         * already listens for, followed by Core::setCurrentSceneBounds(...) - exactly the pair a
         * full scene scan would produce at load time, just computed directly instead of requested
         * from elsewhere.
         *
         * Called after every geometry change that can move the boundary's own extents: a
         * dimension changed, a doorway cut or restored, activation toggled, undo/redo. Guarded by
         * getIsDestroying() so a scene teardown does not broadcast bounds for a game object that is
         * itself about to disappear.
         */
        void notifySceneBoundsDirty(void) const;


        // ── Cell grid ────────────────────────────────────────────────────────────────────
        int getCellCount(BoundarySide side) const;

        bool isCellRemoved(BoundarySide side, int index) const;

        /**
         * @brief Finds the cell under a screen position by intersecting the boundary's own planes.
         * @return True if a cell was hit; outCell then holds it.
         */
        bool pickCell(Ogre::Real screenX, Ogre::Real screenY, BoundaryCell& outCell) const;

        /**
         * @brief The world-space corner points of one cell's inner face, for drawing the outline.
         */
        void getCellOutline(const BoundaryCell& cell, std::vector<Ogre::Vector3>& outCorners) const;

        Ogre::String serializeRemovedCells(void) const;

        void deserializeRemovedCells(const Ogre::String& data);

        void applyCellChange(const std::set<BoundaryCell>& newRemovedCells, const Ogre::String& transactionName);

        // ── Overlay ──────────────────────────────────────────────────────────────────────
        void createSegmentOverlay(void);

        void destroySegmentOverlay(void);

        void scheduleSegmentOverlayUpdate(void);

        // ── Input / editor state ─────────────────────────────────────────────────────────
        void addInputListener(void);

        void removeInputListener(void);

        void updateModificationState(void);

        void handleMeshModifyMode(NOWA::EventDataPtr eventData);

        void handleGameObjectSelected(NOWA::EventDataPtr eventData);

        bool raycastBoundaryPlane(Ogre::Real screenX, Ogre::Real screenY, const Ogre::Vector3& planeNormal, Ogre::Real planeOffset, Ogre::Vector3& hitPosition) const;

    private:
        // Distinct from ProceduralPlatformComponent's PLATFORMDATA_MAGIC/VERSION - the two
        // components' getPlatformData/setPlatformData formats are unrelated, and a mismatched
        // magic value is what turns "wrong component tried to read this buffer" into a clear
        // rejected-with-a-log-line instead of a misparsed disaster.
        static const uint32_t BOUNDARYDATA_MAGIC = 0x504C4244; // "PLBD" in hex
        static const uint32_t BOUNDARYDATA_VERSION = 1;

    private:
        Ogre::String name;

        Variant* activated;
        Variant* boundaryWidth;
        Variant* boundaryHeight;
        Variant* boundaryDepth;
        Variant* wallThickness;
        Variant* surfaceDatablock;
        Variant* groundDatablock;
        Variant* surfaceUVTiling;
        Variant* groundUVTiling;
        Variant* editMode;

        // ── Geometry buffers ─────────────────────────────────────────────────────────────
        // Same interleaved layout as ProceduralPlatformComponent: pos.xyz, normal.xyz, uv.xy.
        std::vector<float> surfaceVertices;
        std::vector<Ogre::uint32> surfaceIndices;
        Ogre::uint32 currentSurfaceVertexIndex;

        std::vector<float> groundVertices;
        std::vector<Ogre::uint32> groundIndices;
        Ogre::uint32 currentGroundVertexIndex;

        Ogre::Item* boundaryItem;
        Ogre::String boundaryMeshName;

        // The cells the designer has cut away. Everything else is solid.
        std::set<BoundaryCell> removedCells;

        // ── Editor state ─────────────────────────────────────────────────────────────────
        bool hasSelectedCell;
        BoundaryCell selectedCell;

        Ogre::SceneNode* segOverlayNode;
        Ogre::ManualObject* segOverlayObject;

        bool isEditorMeshModifyMode;
        bool isSelected;
        bool isShiftKeyDown;
        Ogre::String editFocusOwner;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

}; // namespace end

#endif