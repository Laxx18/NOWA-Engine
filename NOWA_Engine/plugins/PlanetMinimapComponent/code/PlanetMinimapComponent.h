/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PLANETMINIMAPCOMPONENT_H
#define PLANETMINIMAPCOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "utilities/LowPassAngleFilter.h"

namespace NOWA
{
    class CameraComponent;
    class PlanetTerraComponent;

    /**
     * @brief		Can be used for painting a minimap for a sphere planet/moon, tracked by a PlanetTerraComponent.
     *				Two modes are supported, mirroring MinimapComponent's structure but adapted to a sphere:
     *				1. WholeSceneVisible = true: The ENTIRE planet is shown, fog of war/discovery included. This is
     *				   NOT a camera render -- a sphere cannot be photographed from one side and show "the whole
     *				   planet" without distortion/seams, same reason a flat world map can't either. Instead this
     *				   mirrors how PlanetTerra itself already represents the whole sphere as a flat 2D array (its
     *				   blend weight texture / height data, indexed by the same theta/phi -> u/v unwrap used in
     *				   PlanetTerra::generateBaseSphere): a CPU-baked equirectangular texture of the planet's surface
     *				   (via PlanetTerraComponent::sampleHeightAndNormalAtDirection, height-shaded) is uploaded into
     *				   minimapTexture once at setup, and fog of war/discovery/the tracked target are all computed in
     *				   that same (u,v) space -- so everything lines up with the planet's own UV parameterization.
     *				   The target is shown as a separate small MyGUI marker widget positioned via its UV, not baked
     *				   into the texture (the standard "blip on the map" approach). No camera or compositor workspace
     *				   is used in this mode at all.
     *				2. WholeSceneVisible = false: A real 3D camera follows the tracked game object along the planet's
     *				   surface, positioned above it along the local outward (radial) direction and always looking
     *				   back toward the planet's center, instead of following along the fixed world Y axis as
     *				   MinimapComponent does on a flat scene. This still uses a camera + compositor workspace, since
     *				   it is a genuine local 3D view, not an unwrap.
     *				Note: Fog of war/discovery is only available in WholeSceneVisible mode, exactly like MinimapComponent.
     */
    class EXPORTED PlanetMinimapComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<PlanetMinimapComponent> PlanetMinimapComponentPtr;

    public:
        PlanetMinimapComponent();

        virtual ~PlanetMinimapComponent();

        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         * @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
         */
        virtual void initialise() override {};

        /**
         * @see		Ogre::Plugin::shutdown
         * @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
         */
        virtual void shutdown() override {};

        /**
         * @see		Ogre::Plugin::uninstall
         * @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
         */
        virtual void uninstall() override {};

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
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::onOtherComponentAdded
         */
        virtual void onOtherComponentAdded(unsigned int index) override;

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

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        /**
         * @see		GameObjectComponent::isActivated
         */
        virtual bool isActivated(void) const override;

        /**
         * @brief Sets the game object id of the planet (must hold a PlanetTerraComponent), which shall be displayed on the minimap.
         * @param[in] planetId The planet game object id to set
         */
        void setPlanetId(unsigned long planetId);

        /**
         * @brief Gets the game object id of the planet, which is displayed on the minimap.
         * @return planetId The planet game object id to get
         */
        unsigned long getPlanetId(void) const;

        /**
         * @brief Gets the planet game object for the given planet id.
         */
        GameObject* getPlanetGameObject(void) const;

        /**
         * @brief Sets target id for the game object, which shall be tracked on the planet's surface.
         * @param[in] targetId The target id to set
         */
        void setTargetId(unsigned long targetId);

        /**
         * @brief Gets the target id for the game object, which is tracked.
         * @return targetId The target id to get
         */
        unsigned long getTargetId(void) const;

        void setTextureSize(unsigned int textureSize);

        unsigned int getTextureSize(void) const;

        void setMinimapGeometry(const Ogre::Vector4& minimapGeometry);

        Ogre::Vector4 getMinimapGeometry(void) const;

        void setTransparent(bool transparent);

        bool getTransparent(void) const;

        void setUseFogOfWar(bool useFogOfWar);

        bool getUseFogOfWar(void) const;

        void setUseDiscovery(bool useDiscovery);

        bool getUseDiscovery(void) const;

        void setPersistDiscovery(bool persistDiscovery);

        bool getPersistDiscovery(void) const;

        void setVisibilityRadius(Ogre::Real visibilityRadius);

        Ogre::Real getVisibilityRadius(void) const;

        void setUseVisibilitySpotlight(bool useVisibilitySpotlight);

        bool getUseVisibilitySpotlight(void) const;

        void setWholeSceneVisible(bool wholeSceneVisible);

        bool getWholeSceneVisible(void) const;

        void setCameraHeight(Ogre::Real cameraHeight);

        Ogre::Real getCameraHeight(void) const;

        void setMinimapMask(const Ogre::String& minimapMask);

        Ogre::String getMinimapMask(void) const;

        void setTargetMarkerImage(const Ogre::String& targetMarkerImage);

        Ogre::String getTargetMarkerImage(void) const;

        /**
         * @brief Sets the count of generic compass objects shown alongside the main tracked target. Each compass
         *		  object is independently configured (CompassGameObjectId/CompassImage/CompassToolTipText) and gets
         *		  its own marker + distance label: in WholeSceneVisible mode as a true-position marker on the baked
         *		  overview (the bake covers the entire sphere, so any compass object is always representable); in
         *		  follow mode as a marker wandering around the rim of the local minimap once out of view, pointing
         *		  in the direction to walk, derived from the minimap camera's current actual orientation. This
         *		  replaces what used to be a single hard-coded "ship" compass target -- use this to track anything
         *		  with a position: a ship, a quest objective, an NPC, etc. Mirrors
         *		  AnimationSequenceComponent::setAnimationCount's indexed-Variant-list convention.
         * @param[in] compassObjectCount The compass object count to set
         */
        void setCompassObjectCount(unsigned int compassObjectCount);

        /**
         * @brief Gets the compass object count
         * @return compassObjectCount The compass object count
         */
        unsigned int getCompassObjectCount(void) const;

        /**
         * @brief Sets the game object id to track for the compass object at the given index. 0 disables that slot
         *		  (its marker/label stay hidden) without needing to shrink the list.
         */
        void setCompassGameObjectId(unsigned int index, unsigned long compassGameObjectId);

        unsigned long getCompassGameObjectId(unsigned int index) const;

        /**
         * @brief Sets the marker icon image for the compass object at the given index (added to the Minimap.xml in
         *		  the MyGui/Minimap folder, same as MinimapMask/TargetMarkerImage). Left empty, no marker is shown
         *		  for that slot even if CompassGameObjectId is set.
         */
        void setCompassImage(unsigned int index, const Ogre::String& compassImage);

        Ogre::String getCompassImage(unsigned int index) const;

        /**
         * @brief Sets the tooltip text shown when hovering the compass object's marker at the given index (e.g.
         *		  "Player Ship", "Quest: Find the Relic"). Left empty, no tooltip is shown for that slot.
         */
        void setCompassToolTipText(unsigned int index, const Ogre::String& compassToolTipText);

        Ogre::String getCompassToolTipText(unsigned int index) const;

        /**
         * @brief Sets the colour shown where none of the 4 paint layers are active (PlanetTerra's "base diffuse
         *		  shows" state -- all blend weights ~0). Used by bakePlanetOverviewTexture as the baseline the 4
         *		  layer colours blend toward/away from, based on PlanetTerraComponent::getPlanetTerra()'s blend
         *		  weight data. These defaults are placeholders -- tune them to roughly match this planet's actual
         *		  Detail0..3TextureName/base diffuse colours for a readable overview.
         */
        void setBaseTerrainColor(const Ogre::Vector3& baseTerrainColor);

        Ogre::Vector3 getBaseTerrainColor(void) const;

        void setLayer0Color(const Ogre::Vector3& layer0Color);

        Ogre::Vector3 getLayer0Color(void) const;

        void setLayer1Color(const Ogre::Vector3& layer1Color);

        Ogre::Vector3 getLayer1Color(void) const;

        void setLayer2Color(const Ogre::Vector3& layer2Color);

        Ogre::Vector3 getLayer2Color(void) const;

        void setLayer3Color(const Ogre::Vector3& layer3Color);

        Ogre::Vector3 getLayer3Color(void) const;

        void setUseRoundMinimap(bool useRoundMinimap);

        bool getUseRoundMinimap(void) const;

        void setRotateMinimap(bool rotateMinimap);

        bool getRotateMinimap(void) const;

        void clearDiscoveryState(void);

        bool saveDiscoveryState(void);

        bool loadDiscoveryState(void);

        /**
         * @brief Re-bakes the equirectangular overview texture (see class doc) from the planet's current height data.
         *		  Called automatically once at setup, in WholeSceneVisible mode. Call this manually after sculpting has
         *		  changed the planet's terrain, since the bake is a one-shot snapshot, not a live view.
         */
        void rebakePlanetOverview(void);

        /**
         * @brief Re-targets this already-activated minimap/camera onto a different planet (and optionally a different
         *		  tracked target), without leaking the previous compositor workspace/textures/widgets and without losing
         *		  the previous planet's discovery progress. This is the supported way to reuse one PlanetMinimapComponent
         *		  across several planets, e.g. when the player travels from one planet/moon to another:
         *		  1. Saves the current discovery state under the CURRENT PlanetId, if PersistDiscovery is set.
         *		  2. Fully tears down the current workspace/textures/widgets.
         *		  3. Sets the new PlanetId (and TargetId, if newTargetId != 0).
         *		  4. Re-resolves planetGameObject/planetTerraComponent/targetGameObject and sets the minimap back up,
         *		     which loads the discovery state for the NEW PlanetId, if PersistDiscovery is set.
         * @param[in] newPlanetId The game object id of the planet to switch to (must hold a PlanetTerraComponent).
         * @param[in] newTargetId The game object id of the new tracked target, or 0 to keep the current target.
         */
        void switchPlanet(unsigned long newPlanetId, unsigned long newTargetId);

        Ogre::TextureGpu* getMinimapTexture(void) const;

    public:
        /**
         * @see		GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("PlanetMinimapComponent");
        }

        /**
         * @see		GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "PlanetMinimapComponent";
        }

        /**
         * @see		GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Can be used for painting a minimap for a sphere planet/moon. Also fog of war is possible and the given game object id is tracked on the planet's surface. "
                   "Requirements: This component must be placed under a separate game object with a camera component, which is not the MainCamera. The referenced planet game object must hold a PlanetTerraComponent. "
                   "Note: The minimap can only be used with default direction of -z, which is also the default graphics engine mesh direction. "
				"Note: If WholeSceneVisible is true, the WHOLE planet is shown as a baked equirectangular (height-shaded) snapshot, with fog of war/discovery and the tracked target's marker computed in the same UV space -- no camera is used in this mode. "
                   "Call rebakePlanetOverview() after sculpting changes the terrain. "
                   "Note: If WholeSceneVisible is false, a real camera follows the target along the planet's surface, oriented toward the planet's center; fog of war is not available in this mode.";
        }

        /**
         * @see	GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();

    public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrPlanetId(void) { return "Planet Id"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrTextureSize(void) { return "Texture Size"; }
		static const Ogre::String AttrMinimapGeometry(void) { return "Minimap Geometry"; }
		static const Ogre::String AttrTransparent(void) { return "Transparent"; }
		static const Ogre::String AttrUseFogOfWar(void) { return "Use Fog of War"; }
		static const Ogre::String AttrUseDiscovery(void) { return "Use Discovery"; }
		static const Ogre::String AttrPersistDiscovery(void) { return "Persist Discovery"; }
		static const Ogre::String AttrVisibilityRadius(void) { return "Visibility Radius"; }
		static const Ogre::String AttrUseVisibilitySpotlight(void) { return "Use Visibility Spotlight"; }
		static const Ogre::String AttrWholeSceneVisible(void) { return "Whole Scene Visible"; }
		static const Ogre::String AttrCameraHeight(void) { return "Camera Height"; }
		static const Ogre::String AttrMinimapMask(void) { return "Minimap Mask"; }
		static const Ogre::String AttrTargetMarkerImage(void) { return "Target Marker Image"; }
		static const Ogre::String AttrCompassObjectCount(void) { return "Compass Object Count"; }
		static const Ogre::String AttrCompassGameObjectId(void) { return "Compass Game Object Id "; }
		static const Ogre::String AttrCompassImage(void) { return "Compass Image "; }
		static const Ogre::String AttrCompassToolTipText(void) { return "Compass ToolTip Text "; }
		static const Ogre::String AttrBaseTerrainColor(void) { return "Base Terrain Color"; }
		static const Ogre::String AttrLayer0Color(void) { return "Layer0 Color"; }
		static const Ogre::String AttrLayer1Color(void) { return "Layer1 Color"; }
		static const Ogre::String AttrLayer2Color(void) { return "Layer2 Color"; }
		static const Ogre::String AttrLayer3Color(void) { return "Layer3 Color"; }
		static const Ogre::String AttrUseRoundMinimap(void) { return "Use Round Minimap"; }
		static const Ogre::String AttrRotateMinimap(void) { return "Rotate Minimap"; }
    private:
        void setupPlanetMinimap(void);

        Ogre::TextureGpu* createMinimapTexture(const Ogre::String& name, unsigned int width, unsigned int height);

        Ogre::TextureGpu* createFogOfWarTexture(const Ogre::String& name, unsigned int width, unsigned int height);

        void createRoundMinimapWorkspace(void);

        void createMinimapWorkspace(void);

        void updateFogOfWarTexture(const Ogre::Vector3& position, const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetDefaultDirection, Ogre::Real radius);

        void removeWorkspace(void);

        /**
         * @brief Converts a world-space direction from the planet's center into the same (u,v) in [0,1)x[0,1]
         *		  equirectangular parameterization PlanetTerra::generateBaseSphere uses for its vertices/UVs and
         *		  sampleHeightAndNormalAtDirection uses internally: u = theta / (2*PI), v = phi / PI, with
         *		  theta = atan2(dir.z, dir.x) wrapped into [0, 2*PI) and phi = acos(dir.y). Keeping this identical to
         *		  PlanetTerra's own convention is what makes the baked overview texture, the fog of war overlay and
         *		  the target marker all line up with each other and with the planet's own blend/height data.
         */
        Ogre::Vector2 worldDirectionToUV(const Ogre::Vector3& direction) const;

        Ogre::Vector2 uvToPixelCoordinates(const Ogre::Vector2& uv, unsigned int textureWidth, unsigned int textureHeight) const;

        /**
         * @brief CPU-bakes a height-shaded equirectangular image of the whole planet into minimapTexture via a
         *		  staging texture, replacing the literal camera render a flat MinimapComponent would use. One pixel
         *		  per (u,v) is inverse-mapped to a world direction and sampled via
         *		  PlanetTerraComponent::sampleHeightAndNormalAtDirection. This is a one-shot snapshot, not a live
         *		  view -- call rebakePlanetOverview() again after sculpting changes the terrain.
         */
        void bakePlanetOverviewTexture(void);

        /**
         * @brief Repositions (and creates on first use) the small marker widget showing the tracked target's
         *		  position on the equirectangular overview map, from its current world direction relative to the
         *		  planet's center mapped through worldDirectionToUV/uvToPixelCoordinates onto minimapWidget's pixel
         *		  rect. This is the standard "blip on the map" approach -- the target is never baked into the texture.
         */
        void updateOverviewTargetMarker(const Ogre::Vector3& targetWorldPosition);

        /**
         * @brief Calls updateSingleCompassObject for every configured compass object (index 0..CompassObjectCount-1).
         *		  followCameraPosition/Orientation/Distance must be the exact values updateFollowCamera computed
         *		  THIS SAME TICK (its own out-parameters) in follow mode, or any value (e.g. ZERO/IDENTITY/0) in
         *		  WholeSceneVisible mode, where they are unused.
         */
        void updateCompassObjects(const Ogre::Vector3& targetWorldPosition, const Ogre::Vector3& followCameraPosition, const Ogre::Quaternion& followCameraOrientation, Ogre::Real followCameraDistance);

        /**
         * @brief Repositions (and hides if unresolved) the compass object marker + distance label at the given
         *		  index. In WholeSceneVisible mode this is a true position on the baked overview map, same mechanism
         *		  as updateOverviewTargetMarker. In follow mode this is the object's TRUE screen position when it is
         *		  within the local view's footprint, clamped onto the rim ring (at the correct bearing) once it
         *		  would fall outside it. targetWorldPosition is the tracked player/target's true position, used
         *		  (independent of the camera offset) to compute and display the real distance in meters.
         */
        void updateSingleCompassObject(unsigned int index, const Ogre::Vector3& targetWorldPosition, const Ogre::Vector3& followCameraPosition, const Ogre::Quaternion& followCameraOrientation, Ogre::Real followCameraDistance);

        /**
         * @brief (Re-)creates the marker + distance-label widgets for every currently configured compass object,
         *		  destroying any previous ones first. Called from setupPlanetMinimap and whenever CompassObjectCount
         *		  changes at runtime.
         */
        void generateCompassObjects(void);

        /**
         * @brief Destroys all compass object marker/distance-label widgets and clears the widget vectors. Does NOT
         *		  touch the CompassGameObjectId/CompassImage/CompassToolTipText Variants themselves.
         */
        void destroyCompassObjects(void);

        /**
         * @brief Positions and orients the camera above the given world position along the local outward (radial)
         *		  direction from the planet's center, always looking back toward the planet's center. Called every
         *		  logic tick in WholeSceneVisible == false mode, replacing MinimapComponent::updateMinimapCamera.
         *		  Outputs the exact camera position/orientation/distance just computed via outCameraPosition/
         *		  outCameraOrientation/outCameraDistance, so callers (updateCompassObjects) can use the authoritative
         *		  values directly instead of reading them back from the Camera object afterwards.
         */
        void updateFollowCamera(const Ogre::Vector3& targetWorldPosition, const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetDefaultDirection,
            Ogre::Vector3& outCameraPosition, Ogre::Quaternion& outCameraOrientation, Ogre::Real& outCameraDistance);

        /**
         * @brief Builds the camera orientation that re-targets MinimapComponent's fixed "looks straight down world -Y"
         *		  pose onto an arbitrary outward surface normal, so the camera always looks back toward the planet's center.
         */
        Ogre::Quaternion buildSurfaceLookOrientation(const Ogre::Vector3& outwardNormal) const;

        /**
         * @brief Computes a continuous heading in degrees of worldForward projected onto the tangent plane at
         *		  outwardNormal, measured against a local "north" reference (world +Y projected onto the same tangent
         *		  plane, with a world +Z fallback near the poles). Feeds the same LowPassAngleFilter MinimapComponent
         *		  uses for RotateMinimap, replacing the plain world yaw used there.
         */
        Ogre::Real computeTangentHeadingDegrees(const Ogre::Vector3& outwardNormal, const Ogre::Vector3& worldForward) const;

        /**
         * @brief Formats a world-unit distance (assumed to already be in meters, NOWA-Engine's standard world unit)
         *		  as a short human-readable string: "342 m" below 1000, "1.2 km" at or above 1000.
         */
        Ogre::String formatDistanceMeters(Ogre::Real distanceMeters) const;

        void clearFogOfWar(void);

    private:
        void deleteGameObjectDelegate(EventDataPtr eventData);

    private:
        Ogre::String name;

        GameObject* planetGameObject;
        PlanetTerraComponent* planetTerraComponent;
        Ogre::TextureGpu* minimapTexture;
        Ogre::StagingTexture* minimapStagingTexture;
        Ogre::TextureGpu* fogOfWarTexture;
        Ogre::StagingTexture* fogOfWarStagingTexture;
        Ogre::TextureGpuManager* textureManager;
        Ogre::CompositorWorkspace* workspace;
        Ogre::CompositorChannelVec externalChannels;
        Ogre::String minimapWorkspaceName;
        Ogre::String minimapNodeName;
        MyGUI::ImageBox* minimapWidget;
        MyGUI::ImageBox* maskWidget;
        MyGUI::ImageBox* fogOfWarWidget;
        MyGUI::ImageBox* targetMarkerWidget;
        std::vector<MyGUI::ImageBox*> compassObjectWidgets;
        std::vector<MyGUI::TextBox*> compassObjectDistanceTexts;

        CameraComponent* cameraComponent;

        std::vector<std::vector<bool>> discoveryState;

        Ogre::Real timeSinceLastUpdate;
        bool followCameraLogged;
        unsigned int compassLogCounter;
        LowPassAngleFilter filter;

        Variant* activated;
        Variant* planetId;
        Variant* targetId;
        Variant* textureSize;
        Variant* minimapGeometry;
        Variant* transparent;
        Variant* useFogOfWar;
        Variant* useDiscovery;
        Variant* persistDiscovery;
        Variant* visibilityRadius;
        Variant* useVisibilitySpotlight;
        Variant* wholeSceneVisible;
        Variant* cameraHeight;
        Variant* minimapMask;
        Variant* targetMarkerImage;
        Variant* compassObjectCount;
        std::vector<Variant*> compassGameObjectIds;
        std::vector<Variant*> compassImages;
        std::vector<Variant*> compassToolTipTexts;
        Variant* baseTerrainColor;
        Variant* layer0Color;
        Variant* layer1Color;
        Variant* layer2Color;
        Variant* layer3Color;
        Variant* useRoundMinimap;
        Variant* rotateMinimap;
    };

}; // namespace end

#endif