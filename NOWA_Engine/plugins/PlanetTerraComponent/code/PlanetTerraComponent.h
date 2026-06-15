/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PLANET_TERRA_COMPONENT_H
#define PLANET_TERRA_COMPONENT_H

#include "OgrePlugin.h"

#include "gameobject/GameObjectComponent.h"
#include "gameobject/PlanetTerraComponentBase.h"
#include "planetterra/PlanetTerra.h"
#include "main/Events.h"

// OIS forward declarations (included via NOWAPrecompiled typically)
#include "OISKeyboard.h"
#include "OISMouse.h"

namespace NOWA
{
    /**
     * @brief Spherical planet terrain component with sculpting and multi-layer painting.
     *
     * Owns a PlanetTerra object (mesh, blend-weight texture, CPU height/blend data).
     * Automatically placed alongside a DatablockPbsComponent whose material starts
     * as "PlanetTerraDefaultMaterial" (4 terrain detail layers).
     *
     * The blend-weight texture ("<goName>_blendWeight", RGBA8) is wired automatically
     * into PBSM_DETAIL_WEIGHT in connect(). Channel R/G/B/A controls detail0..3 weight.
     *
     * Sculpt brush: moves vertices radially along the sphere surface.
     * Paint brush:  writes RGBA weights into the blend texture.
     * Shift held:   inverts the active brush.
     *
     * Undo: each complete mouse press→release stroke queues
     *   EventDataCommandTransactionBegin
     *   EventDataPlanetTerraModifyEnd(oldData, newData, goId)
     *   EventDataCommandTransactionEnd
     * The EditorManager stores old/new and calls setPlanetData() on undo/redo.
     *
     * Serialization: scalar properties in scene XML; heavy geometry in a binary
     * sidecar "<goName>_PlanetTerra_<id>.ptd" next to the scene file.
     */
    class EXPORTED PlanetTerraComponent : public PlanetTerraComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        PlanetTerraComponent();
        virtual ~PlanetTerraComponent();

        virtual void install(const Ogre::NameValuePairList* options) override;

        virtual void initialise() override;

        virtual void shutdown() override;

        virtual void uninstall() override;

        virtual const Ogre::String& getName() const override;

        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        /**
         * @brief Reads scalar properties from scene XML. Heavy geometry is loaded
         *        from the binary sidecar file in postInit().
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @brief Creates the PlanetTerra, loads sidecar data if present, and
         *        applies the default PBS datablock on first placement.
         */
        virtual bool postInit(void) override;

        /**
         * @brief Wires the runtime blend-weight TextureGpu* into PBSM_DETAIL_WEIGHT
         *        on the sibling DatablockPbsComponent. Called after all postInit()
         *        calls have completed so the PBS datablock is already active.
         */
        virtual bool connect(void) override;

        virtual bool disconnect(void) override;

        virtual bool onCloned(void) override;

        virtual void onAddComponent(void) override;

        virtual void onRemoveComponent(void) override;

        virtual void onOtherComponentRemoved(unsigned int index) override;

        virtual void onOtherComponentAdded(unsigned int index) override;

        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual void update(Ogre::Real dt, bool notSimulating) override;

        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @brief Writes scalar properties to scene XML and saves the binary sidecar.
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }

        virtual Ogre::String getClassName(void) const override;

        virtual Ogre::String getParentClassName(void) const override;

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlanetTerraComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "PlanetTerraComponent";
        }

        void setActivated(bool activated);

        bool isActivated(void) const;

        void setRadius(Ogre::Real radius);

        virtual Ogre::Real getRadius(void) const override;

        virtual const std::vector<Ogre::Vector2>& getUvCoords(void) const override;

        void setSegmentsH(unsigned int segs);
        unsigned int getSegmentsH(void) const;

        void setSegmentsV(unsigned int segs);
        unsigned int getSegmentsV(void) const;

        void setBlendTexSize(unsigned int size);
        unsigned int getBlendTexSize(void) const;

        /** @brief Sets edit mode: "Sculpt" or "Paint". */
        void setEditMode(const Ogre::String& mode);
        Ogre::String getEditMode(void) const;

        /** @brief Sets brush mode: "Pull", "Push", "Smooth", "Flatten" or "Inflate". */
        void setBrushMode(const Ogre::String& mode);
        Ogre::String getBrushMode(void) const;

        /** @brief Sets the active paint layer "0".."3" (maps to R/G/B/A blend channel). */
        void setPaintLayer(const Ogre::String& layer);
        Ogre::String getPaintLayer(void) const;
        int getPaintLayerId(void) const;

        /** @brief Loads the named brush image from the Brushes resource group. */
        void setBrushName(const Ogre::String& name);
        Ogre::String getBrushName(void) const;

        /** @brief Brush radius — world units in Sculpt mode, blend-texture pixels in Paint mode. */
        void setBrushSize(float size);
        float getBrushSize(void) const;

        /** @brief Brush strength per application step (0..1). */
        void setBrushIntensity(float intensity);
        float getBrushIntensity(void) const;

        /** @brief Brush edge falloff exponent. Higher = sharper edge. */
        void setBrushFalloff(float falloff);
        float getBrushFalloff(void) const;

        /**
         * @brief Sets the UV tiling scale for all four detail texture layers (diffuse + NM).
         *        Higher = more tiling = smaller apparent texture size on the sphere.
         *        Formula: scale = sphere_circumference / desired_tile_size_in_metres.
         *        For radius 50 (circumference ~314m): scale 128 = ~2.5m per tile.
         */
        void setBaseUVScale(float scale);
        float getBaseUVScale(void) const;

        void setDetailUVScale(float scale);
        float getDetailUVScale(void) const;

        /**
         * @brief Exports the planet mesh to "<goName>_Planet.mesh" in the scene folder.
         * @return true on success.
         */
        bool bakeMeshToFile(void);

        /** @brief Returns a raw pointer to the owned PlanetTerra object. */
        PlanetTerra* getPlanetTerra(void) const
        {
            return this->planet;
        }

        /**
         * @brief Returns a compact binary snapshot of the current height + blend data.
         *        Used internally for undo stroke capture.
         */
        std::vector<unsigned char> getPlanetData(void) const;

        /**
         * @brief Restores height + blend data from a binary snapshot and uploads to GPU.
         *        Called by the EditorManager on undo/redo.
         */
        void setPlanetData(const std::vector<unsigned char>& data);

        void setDynamic(bool dynamic);

        Ogre::Real getComputedMaxRadius() const;

        bool findFlatLandingVertex(const Ogre::Vector3& outwardDirWorld, float searchHalfAngleDeg, Ogre::Vector3& outWorldPos, Ogre::Vector3& outWorldNormal) const;

        // Fills outSamples with surface data for every non-seam vertex that passes the
        // slope and height filters.  worldOffset = planet GO world position.
        // Returns false if no PlanetTerra is built yet.
        bool collectSurfaceSamples(float slopeMaxDeg, float heightMinLocal, float heightMaxLocal, std::vector<PlanetTerra::SurfaceSample>& outSamples, Ogre::Vector3& outWorldOffset) const;

        bool collectSurfaceSamplesInCone(const Ogre::Vector3& capDirWorld, float capHalfAngleDeg, float slopeMaxDeg, float heightMinLocal, float heightMaxLocal, std::vector<PlanetTerra::SurfaceSample>& outSamples,
            Ogre::Vector3& outWorldOffset) const;

        size_t getVertexCount(void) const;
    protected:
        /** @brief Left-click begins a brush stroke; captures pre-stroke snapshot for undo. */
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        /** @brief Left-release finalises the stroke and queues the undo event. */
        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        /** @brief Continues the active brush stroke while the mouse moves. */
        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        /** @brief Tracks Shift for brush inversion. */
        virtual bool keyPressed(const OIS::KeyEvent& evt) override;

        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

        /**
         * @brief Handles action buttons defined by Variant::AttrActionExec().
         *        Currently handles ActionBakeMesh().
         */
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

    public:
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText()
        {
            return "Spherical planet terrain with brush-based sculpting and multi-layer painting. "
                   "Blend-weight texture '<goName>_blendWeight' (RGBA8, 4 terrain layers) "
                   "must be referenced in the assigned PBS datablock.";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

    public:
        // ── Attribute name constants ────────────────────────────────────────────
        static Ogre::String AttrActivated()
        {
            return "Activated";
        }
        static Ogre::String AttrRadius()
        {
            return "Radius";
        }
        static Ogre::String AttrSegmentsH()
        {
            return "SegmentsH";
        }
        static Ogre::String AttrSegmentsV()
        {
            return "SegmentsV";
        }
        static Ogre::String AttrBlendTexSize()
        {
            return "Blend TexSize";
        }
        static Ogre::String AttrDatablock()
        {
            return "Datablock";
        }
        static Ogre::String AttrEditMode()
        {
            return "EditMode";
        }
        static Ogre::String AttrBrushMode()
        {
            return "Brush Mode";
        }
        static Ogre::String AttrPaintLayer()
        {
            return "Paint Layer";
        }
        static Ogre::String AttrBrushName()
        {
            return "Brush Name";
        }
        static Ogre::String AttrBrushSize()
        {
            return "Brush Size";
        }
        static Ogre::String AttrBrushIntensity()
        {
            return "Brush Intensity";
        }
        static Ogre::String AttrBrushFalloff()
        {
            return "Brush Falloff";
        }
        static Ogre::String AttrBaseUVScale()
        {
            return "Base UV Scale";
        }

        static Ogre::String AttrDetailUVScale()
        {
            return "Detail UV Scale";
        }

        static Ogre::String AttrDetail0NMTextureName()
        {
            return "Detail0 NM Texture";
        }
        static Ogre::String AttrDetail1NMTextureName()
        {
            return "Detail1 NM Texture";
        }
        static Ogre::String AttrDetail2NMTextureName()
        {
            return "Detail2 NM Texture";
        }
        static Ogre::String AttrDetail3NMTextureName()
        {
            return "Detail3 NM Texture";
        }

        static Ogre::String AttrBakeMesh()
        {
            return "Bake Mesh";
        }
        
        static Ogre::String ActionBakeMesh()
        {
            return "PlanetTerraComponent.BakeMesh";
        }

        // Name of the default PBS material loaded from PlanetTerraDefaultMaterial.material.json.
        // Applied automatically on first placement via applyDefaultDatablockIfNeeded().
        static Ogre::String DefaultMaterialName()
        {
            return "PlanetTerraDefaultMaterial";
        }

    private:
        // Planet lifecycle
        void createPlanet(void);
        void destroyPlanet(void);

        void deletePlanetDataFile(void);

        // Wires the runtime blend TextureGpu* into PBSM_DETAIL_WEIGHT on the
        // HlmsPbsDatablock that is currently active on the planet Item's SubItem.
        // Must run on render thread — called from connect() and after any rebuild.
        void wireBlendTextureToPbsDatablock(void);

        // Direct render-thread variant — called from inside the createPlanet render command
        // to avoid a nested enqueueAndWait deadlock.
        void wireBlendTextureToPbsDatablockInternal(void);

        // Input listener registration (render thread required)
        void addInputListener(void);
        void removeInputListener(void);

        // Per-stroke brush application
        void doApplySculptBrush(const Ogre::Vector3& worldHitPos, const Ogre::Vector3& worldHitNorm);
        void doApplyPaintBrush(const Ogre::Vector2& hitUV);

        // Activates/deactivates OIS listeners based on activated + isEditorMeshModifyMode + isSelected.
        void updateModificationState(void);

        // Serialization helpers
        Ogre::String getPlanetDataFilePath(void) const;
        void savePlanetDataToFile(void);
        bool loadPlanetDataFromFile(void);

        // Queues TransactionBegin + EventDataPlanetTerraModifyEnd(old, new) + TransactionEnd.
        // The EditorManager is the sole listener; it stores old/new and calls
        // setPlanetData() directly when undo/redo is invoked. No re-subscription needed.
        void fireUndoEvent(const std::vector<unsigned char>& oldData);

        // Brush image loading
        void loadBrushImage(const Ogre::String& brushName);

        // Returns the BrushMode enum matching the current brushMode list selection.
        PlanetTerra::BrushMode getBrushModeEnum(void) const;

        // Event handlers
        void handleMeshModifyMode(NOWA::EventDataPtr eventData);
        void handleGameObjectSelected(NOWA::EventDataPtr eventData);
        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);
        // Fired by GameObjectFactory after each component postInit completes.
        // Used to wire the blend texture after DatablockPbsComponent is fully ready.
        void handleNewComponent(NOWA::EventDataPtr eventData);

    private:
        Ogre::String name;
        PlanetTerra* planet = nullptr;
        Ogre::RaySceneQuery* rayQuery = nullptr;
        bool postInitDone = false;
        bool planetLoadedFromFile = false;
        bool isEditorMeshModifyMode = false;
        bool isSelected = false;
        bool isPressing = false;
        bool isShiftPressed = false;
        bool brushStrokeActive = false;

        // Brush image data
        std::vector<float> brushData;
        int brushImageW = 0;
        int brushImageH = 0;

        // Pre-stroke snapshot for undo (captured on mousePressed)
        std::vector<unsigned char> brushStrokePreEditData;

        // Data loaded from sidecar file; applied in createPlanet() then cleared
        std::vector<float> loadedHeightData;
        std::vector<uint8_t> loadedBlendData;

        bool hasModifiedData;

        // Attributes
        Variant* activated = nullptr;
        Variant* radius = nullptr;
        Variant* segmentsH = nullptr;
        Variant* segmentsV = nullptr;
        Variant* blendTexSize = nullptr;
        Variant* datablock = nullptr;
        Variant* editMode = nullptr;
        Variant* brushMode = nullptr;
        Variant* paintLayer = nullptr;
        Variant* brushName = nullptr;
        Variant* brushSize = nullptr;
        Variant* brushIntensity = nullptr;
        Variant* brushFalloff = nullptr;
        Variant* bakeMesh = nullptr;
        Variant* baseUVScale = nullptr;
        Variant* detailUVScale = nullptr;
        Variant* detail0NMTextureName = nullptr;
        Variant* detail1NMTextureName = nullptr;
        Variant* detail2NMTextureName = nullptr;
        Variant* detail3NMTextureName = nullptr;
    };

    using PlanetTerraComponentPtr = boost::shared_ptr<PlanetTerraComponent>;

} // namespace NOWA

#endif