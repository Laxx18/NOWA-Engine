/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_PIPE_COMPONENT_H
#define PROCEDURAL_PIPE_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/PlatformComponentBase.h"
#include "main/Events.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralPipeComponent
     * @brief Interactive pipe/tube building component for 2.5D Metroidvania levels - click
     *        and drag to sweep a tube along a path on a fixed depth plane, the same way
     *        ProceduralPlatformComponent sweeps a slab.
     *
     * The path model, the editor interaction (Object/Segment mode, snapping, preview, depth
     * nudge), the "Path Data" scene persistence and the undo/redo blob are deliberately the
     * SAME as ProceduralPlatformComponent's - a pipe is a platform component whose
     * cross-section happens to be a ring instead of a box. What is genuinely different is
     * only the sweep (generatePipeRings) and the near-side handling below.
     *
     * NEAR SIDE (the reason this component exists rather than "a platform with a round mesh"):
     * in a 2.5D game the player runs INSIDE the tube, and the half of the tube between the
     * camera and the player would hide them. Three ways out, all available at runtime and
     * from Lua via setNearSideMode:
     *
     *   - SOLID: ordinary closed tube. Right for pipes in the background.
     *   - TRANSPARENT: the near arc gets its OWN datablock, so it can be an alpha-blended
     *     version of the pipe material. The player shows through, the tube still reads as a
     *     tube. This is a pure datablock switch - no rebuild.
     *   - HIDDEN: the near arc is not generated at all and the two longitudinal cut edges are
     *     closed with rim strips, giving a clean half-open trough. Needs a rebuild, so it is
     *     the one mode that costs something to toggle.
     *
     * Flipping the winding / normals in Z was considered and deliberately NOT done: reversing
     * culling is a macroblock flag (or HlmsPbsDatablock::setTwoSidedLighting) and needs no
     * geometry at all, and flipping the whole tube would also make it invisible from any other
     * camera angle. The near/far SPLIT is what actually buys something a datablock cannot do -
     * two independently shaded halves.
     *
     * WALL THICKNESS: the sweep builds an outer shell, an inner shell with reversed winding
     * and inward normals, and closing rings at both ends. That means the interior is properly
     * lit without two-sided tricks, the pipe ends look solid instead of paper-thin, and the
     * PhysicsArtifactComponent collision has a real inner surface for the player to run on.
     * Set Wall Thickness to 0 for a single-shell tube (cheaper, but backfacing inside).
     *
     * Derives from PlatformComponentBase for the same reason ProceduralPlatformComponent does:
     * it is what the undo/redo command and the cross-component snap lookup talk to
     * (setPlatformData/getPlatformData/getNearestPointOnPlatform). The undo events
     * (EventDataPlatformModifyEnd) are reused as-is rather than introducing a parallel
     * EventDataPipeModifyEnd - the payload is an opaque blob and the handler resolves the
     * component through the base class, so a second event type would buy nothing but a second
     * thing to keep in sync.
     */
    class EXPORTED ProceduralPipeComponent : public PlatformComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<ProceduralPipeComponent> ProceduralPipeComponentPtr;

        enum class BuildState
        {
            IDLE = 0,
            DRAGGING,
            CONFIRMING
        };

        enum class EditMode
        {
            OBJECT,
            SEGMENT
        };

        /**
         * @brief How the arc facing the camera is treated. See the class comment.
         */
        enum class NearSideMode
        {
            SOLID = 0,
            FADE = 1,
            HIDDEN = 2
        };

        // Two submeshes, split by radial angle rather than by structural role (which is what
        // the platform's SURFACE/GROUND split is). Both shells and both end rings feed both
        // buffers - a quad goes wherever its own angle puts it - so the near arc can be
        // shaded, faded or dropped as one piece.
        enum class PipeMeshBuffer
        {
            FAR_SIDE,
            NEAR_SIDE
        };

        struct PipeControlPoint
        {
            // x = horizontal position along the pipe plane.
            // y = 0 (placeholder - the real height is kept separately, exactly as in
            //     ProceduralPlatformComponent, so both components' path blobs stay
            //     byte-compatible and the shared undo machinery does not care which one it
            //     is restoring).
            // z = authored depth offset (the U / SHIFT+U nudge writes here). Only a chain's
            //     two END values are read; rebuildMesh ramps linearly between them.
            Ogre::Vector3 position;
            Ogre::Real rawHeight = 0.0f;
            Ogre::Real smoothedHeight = 0.0f;
            Ogre::Real distFromStart = 0.0f;
            // DERIVED, never serialized: the depth this point is actually DRAWN at.
            Ogre::Real renderZ = 0.0f;
        };

        struct PipeSegment
        {
            std::vector<PipeControlPoint> controlPoints;
            bool isCurved = false;
            Ogre::Real curvature = 0.0f;
        };

        /**
         * @brief Endpoint identity on a 5 cm grid in (x, height). Depth is deliberately NOT
         *        part of it, so two runs nudged to different depths still register as meeting
         *        if their in-plane endpoint coincides.
         */
        struct EndpointKey
        {
            int x = 0;
            int h = 0;

            bool operator<(const EndpointKey& other) const
            {
                return x < other.x || (x == other.x && h < other.h);
            }
        };

        /**
         * @brief Three or more arms meeting at one endpoint. Collected during the sweep, once
         *        every chain knows where it starts and ends.
         *
         * No CSG is involved in filling one: a sphere is placed at the meeting point, every
         * arm is trimmed back so its mouth sits INSIDE that sphere, and the sphere is built
         * with a hole where each arm enters. The arm's own tube then covers the hole's rim from
         * both sides. That is the standard way pipe fittings are modelled, it needs nothing but
         * a dot product per quad, and - unlike a boolean union - it keeps the near/far split
         * and the UVs intact.
         */
        struct PipeJunction
        {
            // Mesh-local. Averaged over the arriving arm endpoints, because a depth nudge can
            // make two arms reach the same (x, height) at different z.
            Ogre::Vector3 centre = Ogre::Vector3::ZERO;
            // One per arm, pointing AWAY from the centre, mesh-local and unit length.
            std::vector<Ogre::Vector3> armDirections;
        };

        /**
         * @brief One sample of the finished centerline, in mesh-local space. Built by every
         *        rebuild and queried by the Lua path API (getPointAtDistance and friends), so
         *        a script can move the player along the pipe or ask where along it he is.
         *        Never serialized - it is a pure function of the path.
         */
        struct PipePathSample
        {
            Ogre::Vector3 position;
            Ogre::Vector3 direction; // Unit tangent, pointing from chain start toward chain end
            Ogre::Real distance = 0.0f;
        };

    public:
        ProceduralPipeComponent();
        virtual ~ProceduralPipeComponent();

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

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

        /**
         * @see		GameObjectComponent::onAddComponent
         */
        virtual void onAddComponent(void) override;

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralPipeComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralPipeComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates procedural 2.5D pipes/tubes on a fixed depth plane, for Metroidvania-style levels.\n"
                   "The player can run THROUGH the tube; the arc facing the camera can be faded or cut away so he stays visible.\n\n"
                   "PIPE BUILDING (Object Mode):\n"
                   "- Left-click anywhere to start a new pipe segment. The first click of the whole chain fixes\n"
                   "  the local Z (depth) plane every further segment will be drawn on.\n"
                   "- Move the mouse to preview the segment, then left-click again to confirm it.\n"
                   "- Hold SHIFT while confirming to automatically chain the next segment from the endpoint.\n"
                   "- Hold CTRL to constrain the segment direction to horizontal or vertical.\n"
                   "- Right-click or press ESC to cancel the current segment.\n"
                   "- Press CTRL+Z to undo the last confirmed segment.\n\n"
                   "SEGMENT MODE:\n"
                   "- Set the 'Edit Mode' property to 'Segment' to enter segment editing.\n"
                   "- Left-click near any pipe segment to select it. The selected segment is highlighted.\n"
                   "- Press X to delete the selected segment, E to extend a new one from its tail endpoint.\n"
                   "- Press U / SHIFT+U to shift the selected segment one Pipe Radius along the depth axis, so\n"
                   "  two crossing runs pass in front of / behind each other instead of intersecting. Only a\n"
                   "  chain's two END segments move the ramp - nudging a middle segment changes nothing.\n"
                   "- Press ESC to deselect.\n"
                   "- A green snap circle appears near an existing endpoint; release there to connect.\n\n"
                   "GEOMETRY:\n"
                   "- 'Pipe Radius' is the outer radius of the tube in meters.\n"
                   "- 'Wall Thickness' builds a second, inner shell plus closing rings at both ends. The inside\n"
                   "  is then correctly lit and solid-looking, and the physics collision has a real inner\n"
                   "  surface. 0 means a single shell - cheaper, but the inside is backfacing.\n"
                   "- 'Radial Segments' is how many vertices each ring has (higher = rounder).\n"
                   "- 'Curve Subdivisions' controls how many points are interpolated along the path.\n"
                   "- 'Smoothing Factor' blends height changes between connected segments.\n\n"
                   "JUNCTIONS:\n"
                   "- Three or more arms meeting at one endpoint form a junction, and a sphere is built to\n"
                   "  fill it. Each arm is trimmed back so its mouth sits inside that sphere, and the sphere\n"
                   "  gets a hole where every arm enters - so the player can run straight through in any\n"
                   "  direction. The near-side cut continues across the sphere, so a junction does not put an\n"
                   "  opaque ball in front of him exactly where the interesting part is.\n"
                   "- 'Junction Hub Scale' is the sphere's radius as a multiple of Pipe Radius. 1.3 gives the\n"
                   "  usual bulged fitting; larger values a chunky ball joint. Below ~1.1 the sphere hides\n"
                   "  inside the arms and the joint opens up.\n"
                   "- Endpoints are matched in (x, height) only, so two arms nudged to different depths still\n"
                   "  count as meeting; the hub is then placed between them.\n\n"
                   "NEAR SIDE (the arc between camera and player):\n"
                   "- 'Near Side Mode':\n"
                   "    Solid       - closed tube, the player is hidden inside it.\n"
                   "    Transparent - the near arc uses 'Near Datablock', typically an alpha-blended copy of\n"
                   "                  the pipe material, so the player shows through. No rebuild on switch.\n"
                   "    Hidden      - the near arc is not built at all and the cut edges get rim strips,\n"
                   "                  giving an open trough. Triggers a rebuild.\n"
                   "- 'Near Side Arc' is how wide that arc is, in degrees around the camera direction (+Z in\n"
                   "  the pipe's own frame). 0 disables the split entirely - everything becomes far side.\n"
                   "- Do NOT flip vertices to see inside: reverse the culling on the datablock instead, or use\n"
                   "  Hlms two-sided lighting. Flipping geometry costs a rebuild and breaks every other view.\n\n"
                   "LUA API:\n"
                   "- getProceduralPipeComponent() on a GameObject returns this component.\n"
                   "- setNearSideMode('Solid'|'Transparent'|'Hidden'), getNearSideMode().\n"
                   "- getPipeLength() returns the swept centerline length in meters.\n"
                   "- getPointAtDistance(d) / getDirectionAtDistance(d) - world position/tangent d meters in.\n"
                   "- getPointAt(t) / getDirectionAt(t) - same with t normalized to 0..1.\n"
                   "- getDistanceOnPipe(worldPos) - how far along the pipe a world position projects, which is\n"
                   "  how a script tracks where the player currently is inside it.\n"
                   "- addPipeSegment(start, end), getSegmentCount(), setPipeRadius(r), setWallThickness(t).\n";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Pipe";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = true;
            desc.autoComponents = {"ProceduralPipeComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralPipeComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "PlatformComponentBase";
        }

        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // Pipe building API
        void startPipePlacement(const Ogre::Vector3& worldPosition);

        void updatePipePreview(const Ogre::Vector3& worldPosition);

        void confirmPipe(void);

        void updateContinuationPoint(void);

        void cancelPipe(void);

        void removeLastSegment(void);

        void clearAllSegments(void);

        // Mesh operations
        void rebuildMesh(void);

        /**
         * @brief Intersects the mouse ray with the fixed local depth plane. localZOffset
         *        slides that plane along its own normal, so Segment mode can pick on the
         *        plane a nudged segment is actually DRAWN on.
         */
        bool raycastFixedPlane(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition, Ogre::Real localZOffset = 0.0f);

        int findNearestSegmentOnScreen(Ogre::Real screenX, Ogre::Real screenY, Ogre::Real radius);

        // Attribute access
        void setActivated(bool activated);

        bool isActivated(void) const;

        void setPipeRadius(Ogre::Real radius);

        Ogre::Real getPipeRadius(void) const;

        void setWallThickness(Ogre::Real thickness);

        Ogre::Real getWallThickness(void) const;

        void setRadialSegments(int segments);

        int getRadialSegments(void) const;

        /**
         * @brief Solid / Transparent / Hidden. Solid<->Transparent is a datablock swap on the
         *        near submesh and costs nothing; anything involving Hidden rebuilds, because
         *        that mode changes which triangles exist.
         */
        void setNearSideMode(const Ogre::String& mode);

        Ogre::String getNearSideMode(void) const;

        NearSideMode getNearSideModeEnum(void) const;

        void setNearSideArc(Ogre::Real degrees);

        Ogre::Real getNearSideArc(void) const;

        /**
         * @brief Alpha the near arc is rendered with in Transparent mode. 1.0 means "leave the
         *        datablock exactly as authored" and is the way to use a hand-made transparent
         *        material without this component touching it.
         */
        /**
         * @brief Radius of the sphere that fills a junction, as a multiple of Pipe Radius.
         *        Must stay above 1 or the sphere disappears inside the arms; around 1.3 gives
         *        the usual bulged fitting look.
         */
        void setJunctionHubScale(Ogre::Real scale);

        Ogre::Real getJunctionHubScale(void) const;

        void setNearSideAlpha(Ogre::Real alpha);

        Ogre::Real getNearSideAlpha(void) const;

        /**
         * @brief Which side of the tube counts as the near one. Default is local -Z, which is
         *        the camera side for a GameObject placed with an unrotated frame. Tick this
         *        when a pipe's object is turned 180 degrees and the cut ends up at the back.
         */
        void setInvertNearSide(bool invert);

        bool getInvertNearSide(void) const;

        void setSnapToGrid(bool snap);

        bool getSnapToGrid(void) const;

        void setGridSize(Ogre::Real size);

        Ogre::Real getGridSize(void) const;

        void setSmoothingFactor(Ogre::Real factor);

        Ogre::Real getSmoothingFactor(void) const;

        void setCurveSubdivisions(int subdivisions);

        int getCurveSubdivisions(void) const;

        void setPipeDatablock(const Ogre::String& datablock);

        Ogre::String getPipeDatablock(void) const;

        void setNearDatablock(const Ogre::String& datablock);

        Ogre::String getNearDatablock(void) const;

        void setPipeUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getPipeUVTiling(void) const;

        void setEditMode(const Ogre::String& editMode);

        EditMode getEditModeEnum(void) const;

        void claimEditFocus(void);

        bool isEditFocusOwner(void) const;

        void deleteSelectedSegment(void);

        void createSegmentOverlay(void);

        void destroySegmentOverlay(void);

        void scheduleSegmentOverlayUpdate(void);

        virtual void setPlatformData(const std::vector<unsigned char>& data) override;

        virtual std::vector<unsigned char> getPlatformData(void) const override;

        virtual bool getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const override;

        void addPipeSegment(const Ogre::Vector3& start, const Ogre::Vector3& end);

        int getSegmentCount(void) const;

        // ── Path query API (Lua) ─────────────────────────────────────────────────────
        // All of these work off pathSamples, which every rebuild refills, and all of them
        // return WORLD space - a script should never have to know about pipeFrame/pipeOrigin.

        /**
         * @brief Total swept length in meters. 0 when there is no pipe yet.
         */
        Ogre::Real getPipeLength(void) const;

        Ogre::Vector3 getPointAtDistance(Ogre::Real distance) const;

        Ogre::Vector3 getDirectionAtDistance(Ogre::Real distance) const;

        Ogre::Vector3 getPointAt(Ogre::Real t) const;

        Ogre::Vector3 getDirectionAt(Ogre::Real t) const;

        /**
         * @brief Projects a world position onto the centerline and returns how far along the
         *        pipe that projection lies, in meters. This is the "where is the player right
         *        now" query - feed it the player's position each frame.
         */
        Ogre::Real getDistanceOnPipe(const Ogre::Vector3& worldPosition) const;

    public:
        // Static attribute names
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrPipeRadius(void)
        {
            return "Pipe Radius";
        }
        static Ogre::String AttrWallThickness(void)
        {
            return "Wall Thickness";
        }
        static Ogre::String AttrRadialSegments(void)
        {
            return "Radial Segments";
        }
        static Ogre::String AttrNearSideMode(void)
        {
            return "Near Side Mode";
        }
        static Ogre::String AttrNearSideArc(void)
        {
            return "Near Side Arc";
        }
        static Ogre::String AttrNearSideAlpha(void)
        {
            return "Near Side Alpha";
        }
        static Ogre::String AttrInvertNearSide(void)
        {
            return "Invert Near Side";
        }
        static Ogre::String AttrJunctionHubScale(void)
        {
            return "Junction Hub Scale";
        }
        static Ogre::String AttrSnapToGrid(void)
        {
            return "Snap To Grid";
        }
        static Ogre::String AttrGridSize(void)
        {
            return "Grid Size";
        }
        static Ogre::String AttrSmoothingFactor(void)
        {
            return "Smoothing Factor";
        }
        static Ogre::String AttrCurveSubdivisions(void)
        {
            return "Curve Subdivisions";
        }
        static Ogre::String AttrPipeDatablock(void)
        {
            return "Pipe Datablock";
        }
        static Ogre::String AttrNearDatablock(void)
        {
            return "Near Datablock";
        }
        static Ogre::String AttrPipeUVTiling(void)
        {
            return "Pipe UV Tiling";
        }
        // Not an editor attribute - pure serialisation state, written by writeXML and read by
        // init() at the same position in the property order. Same mechanism and the same blob
        // layout as ProceduralPlatformComponent's.
        static Ogre::String AttrPathData(void)
        {
            return "Path Data";
        }
        static Ogre::String AttrEditMode(void)
        {
            return "Edit Mode";
        }

    protected:
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool keyPressed(const OIS::KeyEvent& evt) override;

        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

    private:
        void createPipeMesh(void);

        void createPipeMeshInternal(const std::vector<float>& farVerts, const std::vector<Ogre::uint32>& farInds, size_t numFarVerts, const std::vector<float>& nearVerts, const std::vector<Ogre::uint32>& nearInds, size_t numNearVerts,
            const Ogre::Vector3& origin);

        void destroyPipeMesh(void);

        void destroyPreviewMesh(void);

        void updatePreviewMesh(void);

        /**
         * @brief Applies the current near/far datablocks to the live Item without touching
         *        geometry. This is what makes Solid<->Transparent free.
         */
        void applyDatablocks(void);

        /**
         * @brief Works out which datablock the near arc should use, creating or refreshing the
         *        transparent clone when needed. Render thread only - it creates Hlms objects.
         */
        Ogre::HlmsDatablock* resolveNearSideDatablock(void);

        /**
         * @brief Drops the transparent clone, after pointing the near submesh back at the body
         *        datablock - destroying a datablock an Item still references is a crash, not a
         *        leak. Render thread only.
         */
        void destroyClonedNearDatablock(void);

        // ── Sweep ────────────────────────────────────────────────────────────────────
        /**
         * @brief Sweeps one continuous chain of path points into tube geometry.
         *
         * Ring i lies in the plane spanned by the depth axis (0,0,1) and the in-plane path
         * normal, both taken at that point; the normal is mitered between the incoming and
         * outgoing direction and stretched by 1/cos(half turn) so a bend stays watertight
         * without the cross-section pinching. Only the normal needs the miter - the depth axis
         * is constant for the whole component, which is exactly what makes a 2.5D sweep so
         * much simpler than a general 3D one.
         *
         * @param[in] points     Chain points, already subdivided, smoothed and resampled.
         * @param[in] capFront   Close the ring at the chain start (false where another run continues).
         * @param[in] capBack    Close the ring at the chain end.
         */
        void generatePipeRings(const std::vector<PipeControlPoint>& points, bool capFront, bool capBack);

        /**
         * @brief Fills one junction: two spheres (outer and inner wall), each with a hole
         *        punched where every arm enters, and rim strips along the near-side cut when
         *        the near arc is hidden. Split into the same two buffers the tube uses, so the
         *        cut-away or faded arc continues straight through the junction instead of
         *        running into an opaque ball exactly where the player is.
         */
        void generateJunctionHub(const PipeJunction& junction);

        /**
         * @brief Shortens a resampled path at either end, so an arm's mouth ends up inside the
         *        hub sphere rather than poking out the far side of it.
         */
        std::vector<PipeControlPoint> trimPathEnds(const std::vector<PipeControlPoint>& points, Ogre::Real trimFront, Ogre::Real trimBack) const;

        static EndpointKey makeEndpointKey(const PipeControlPoint& cp);

        /**
         * @brief Every segment end, grouped by endpoint. The second element of each pair is 0
         *        for the segment's front point and 1 for its back point. An entry with three or
         *        more incidences IS a junction.
         */
        std::map<EndpointKey, std::vector<std::pair<size_t, int>>> buildEndpointMap(void) const;

        /**
         * @brief Which buffer a direction belongs to. The ring and the hub both route through
         *        this, so a quad on the sphere lands on the same side of the cut as the tube
         *        quad it meets.
         */
        PipeMeshBuffer bufferForDirection(const Ogre::Vector3& localDirection) const;

        /**
         * @brief Which buffer a ring vertex at this angle belongs to. theta is measured from
         *        the +Z (camera) direction, so |theta| < arc/2 is the near arc.
         */
        PipeMeshBuffer bufferForAngle(Ogre::Real theta) const;

        void addPipeQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Val, Ogre::Real v1Val,
            PipeMeshBuffer targetBuffer);

        // Spline / path helpers - same math as ProceduralPlatformComponent, operating on
        // (x, height) because that is the whole path: depth is a separate, ramped quantity.
        Ogre::Vector2 evaluateCatmullRom(const std::vector<PipeControlPoint>& points, Ogre::Real t);

        std::vector<PipeControlPoint> subdivideWithHeightInterpolation(const std::vector<PipeControlPoint>& points);

        std::vector<PipeControlPoint> resamplePathUniformly(const std::vector<PipeControlPoint>& densePath, Ogre::Real stepMeters);

        void smoothHeightTransitions(std::vector<PipeControlPoint>& points);

        /**
         * @brief Links segments that share an endpoint into continuous chains, so a pipe drawn
         *        as five clicks is swept as ONE tube rather than five stubs with caps between
         *        them. Deliberately simpler than the platform's chain builder: no junction
         *        handling (three arms meeting would need a real hub patch on a round
         *        cross-section, which is a separate piece of work) and no hairpin splitting.
         * @return Each chain as a list of segment indices, in walk order, with a flag per
         *         entry saying whether that segment is traversed backwards.
         */
        std::vector<std::vector<std::pair<size_t, bool>>> buildChains(void) const;

        Ogre::Vector3 snapToGridFunc(const Ogre::Vector3& position);

        bool detectSnapToOwnPipe(const Ogre::Vector3& worldPos, Ogre::Real radius);

        void scheduleSnapIndicatorUpdate(void);

        // ── Path persistence ─────────────────────────────────────────────────────────
        // Byte-identical layout to ProceduralPlatformComponent's "Path Data" blob (version,
        // segment count, origin, then the control points), so the two can be compared, and so
        // a path can be moved between a platform and a pipe by hand if it ever needs to be.
        Ogre::String serializePathData(void) const;

        bool deserializePathData(const Ogre::String& encodedData);

        void handleMeshModifyMode(NOWA::EventDataPtr eventData);

        void handleGameObjectSelected(NOWA::EventDataPtr eventData);

        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);

        void handleSceneParsed(NOWA::EventDataPtr eventData);

        void addInputListener(void);

        void removeInputListener(void);

        void updateModificationState(void);

    private:
        // In-memory blob for getPlatformData/setPlatformData (undo/redo) only. Carries the
        // mesh buffers as well as the path: an undo step is restored into a live component in
        // the same session, so the cached geometry cannot be stale and restoring it is much
        // cheaper than a sweep.
        static const uint32_t PIPEDATA_MAGIC = 0x50495045; // "PIPE"
        static const uint32_t PIPEDATA_VERSION = 1;

        // Scene-XML "Path Data" format. 2 to match the platform's, since the layout IS the
        // platform's - see serializePathData.
        static const uint32_t PATHDATA_VERSION = 2;

    private:
        Ogre::String name;

        // Name of the datablock CLONE the Transparent mode works on, and the name of the
        // datablock it was cloned FROM. The clone exists because the near arc usually shares
        // its material with the body: calling setTransparency on that shared datablock would
        // fade the whole pipe, every other object using it included. Empty when no clone is
        // currently alive.
        Ogre::String clonedNearDatablockName;
        Ogre::String clonedNearSourceName;

        // Attributes
        Variant* activated;
        Variant* pipeRadius;
        Variant* wallThickness;
        Variant* radialSegments;
        Variant* nearSideMode;
        Variant* nearSideArc;
        Variant* junctionHubScale;
        Variant* nearSideAlpha;
        Variant* invertNearSide;
        Variant* snapToGrid;
        Variant* gridSize;
        Variant* smoothingFactor;
        Variant* curveSubdivisions;
        Variant* pipeDatablock;
        Variant* nearDatablock;
        Variant* pipeUVTiling;
        Variant* editMode;

        // Path
        std::vector<PipeSegment> pipeSegments;
        PipeSegment currentSegment;
        BuildState buildState;
        bool isEditorMeshModifyMode;
        bool isSelected;

        // Mesh data, split by radial angle
        std::vector<float> farVertices;
        std::vector<Ogre::uint32> farIndices;
        Ogre::uint32 currentFarVertexIndex;

        std::vector<float> nearVertices;
        std::vector<Ogre::uint32> nearIndices;
        Ogre::uint32 currentNearVertexIndex;

        // Centerline samples for the Lua path API, mesh-local, refilled by every rebuild.
        std::vector<PipePathSample> pathSamples;

        // Ogre objects
        Ogre::MeshPtr pipeMesh;
        Ogre::Item* pipeItem;
        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // Input state. isShiftPressed is the AUTO-CHAIN flag (force-set by the E-extend
        // handler and confirmPipe), isShiftKeyDown is the physical key - same split, and same
        // reason, as in ProceduralPlatformComponent.
        bool isShiftPressed;
        bool isShiftKeyDown;
        bool isCtrlPressed;
        Ogre::String editFocusOwner;
        Ogre::Vector3 lastValidPosition;

        Ogre::Vector3 pipeOrigin;
        bool hasPipeOrigin;

        Ogre::Quaternion pipeFrame;
        Ogre::Vector3 pipePlaneAnchor;

        // Cached geometry for the undo/redo blob
        std::vector<float> cachedFarVertices;
        std::vector<Ogre::uint32> cachedFarIndices;
        size_t cachedNumFarVertices;

        std::vector<float> cachedNearVertices;
        std::vector<Ogre::uint32> cachedNearIndices;
        size_t cachedNumNearVertices;

        Ogre::Vector3 cachedPipeOrigin;
        bool originPositionSet;

        bool hasLoadedPipeEndpoint;
        Ogre::Vector3 loadedPipeEndpoint;
        Ogre::Real loadedPipeEndpointHeight;

        // Segment mode
        int selectedSegmentIndex; // -1 = nothing selected

        Ogre::SceneNode* segOverlayNode;
        Ogre::ManualObject* segOverlayObject;
        bool isExtendingFromSegment;

        bool isSnapToOwnPipe;
        Ogre::Vector3 snapToPipePoint;
        int snapToPipeSegmentIdx;
        Ogre::Real snapRadius;

        bool pipeLoadedFromScene;

        // Set by clone() when it copied a non-empty path; postInit does the single rebuild.
        // A clone has neither an init() nor a scene-parsed event to hang its first build on.
        bool pipeClonedNeedsRebuild;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

} // namespace NOWA

#endif // PROCEDURAL_PIPE_COMPONENT_H
