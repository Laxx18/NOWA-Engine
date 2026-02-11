/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_WALL_COMPONENT_H
#define PROCEDURAL_WALL_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	 * @class ProceduralWallComponent
	 * @brief Interactive wall building component - click and drag to create walls dynamically
	 *
	 * Features:
	 * - Click and drag to draw walls in real-time
	 * - Optional corner pillars at start/end points
	 * - Terrain/ground adaptation via raycasting
	 * - Configurable wall dimensions and materials
	 * - Support for different wall styles (solid, fence, etc.)
	 * - Export generated mesh to file
	 * - Undo/Redo support for wall segments
	 */
	class EXPORTED ProceduralWallComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
	{
	public:
		typedef boost::shared_ptr<ProceduralWallComponent> ProceduralWallComponentPtr;

		enum class WallStyle
		{
			SOLID = 0,      // Solid rectangular wall
			FENCE = 1,      // Fence with posts and rails
			BATTLEMENT = 2, // Castle-style with crenellations
			ARCH = 3        // Wall with arch openings
		};

		enum class BuildState
		{
			IDLE = 0,
			DRAGGING,
			CONFIRMING
		};

		struct WallSegment
		{
			Ogre::Vector3 startPoint;
			Ogre::Vector3 endPoint;
			Ogre::Real groundHeightStart;
			Ogre::Real groundHeightEnd;
			bool hasStartPillar;
			bool hasEndPillar;
		};

	public:
		ProceduralWallComponent();
		virtual ~ProceduralWallComponent();

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
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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
			return NOWA::getIdFromName("ProceduralWallComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralWallComponent";
		}

		static bool canStaticAddComponent(GameObject* gameObject);

		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Create different types of walls.";
		}

		virtual Ogre::String getClassName(void) const override
		{
			return "ProceduralWallComponent";
		}

		virtual Ogre::String getParentClassName(void) const override
		{
			return "GameObjectComponent";
		}

		// Wall building API
		void startWallPlacement(const Ogre::Vector3& worldPosition);

		void updateWallPreview(const Ogre::Vector3& worldPosition);

		void confirmWall(void);

		void cancelWall(void);

		// Wall segment management
		void addWallSegment(const Ogre::Vector3& start, const Ogre::Vector3& end);

		void removeLastSegment(void);

		void clearAllSegments(void);

		// Mesh operations
		void rebuildMesh(void);

		bool exportMesh(const Ogre::String& filename);

		// Ground adaptation
		Ogre::Real getGroundHeight(const Ogre::Vector3& position);

		bool raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition);

		// Attribute access
		void setActivated(bool activated);

		bool isActivated(void) const;

		void setWallHeight(Ogre::Real height);

		Ogre::Real getWallHeight(void) const;

		void setWallThickness(Ogre::Real thickness);

		Ogre::Real getWallThickness(void) const;

		void setWallStyle(const Ogre::String& style);

		Ogre::String getWallStyle(void) const;

		void setSnapToGrid(bool snap);

		bool getSnapToGrid(void) const;

		void setGridSize(Ogre::Real size);

		Ogre::Real getGridSize(void) const;

		void setAdaptToGround(bool adapt);

		bool getAdaptToGround(void) const;

		void setCreatePillars(bool create);

		bool getCreatePillars(void) const;

		void setPillarSize(Ogre::Real size);

		Ogre::Real getPillarSize(void) const;

		void setWallDatablock(const Ogre::String& datablock);

		Ogre::String getWallDatablock(void) const;

		void setPillarDatablock(const Ogre::String& datablock);

		Ogre::String getPillarDatablock(void) const;

		void setUVTiling(const Ogre::Vector2& tiling);
		Ogre::Vector2 getUVTiling(void) const;

		void setFencePostSpacing(Ogre::Real spacing);
		void setBattlementWidth(Ogre::Real width);
		void setBattlementHeight(Ogre::Real height);
	public:

		// Static attribute names
		static Ogre::String AttrActivated(void)
		{
			return "Activated";
		}
		static Ogre::String AttrWallHeight(void)
		{
			return "Wall Height";
		}
		static Ogre::String AttrWallThickness(void)
		{
			return "Wall Thickness";
		}
		static Ogre::String AttrWallStyle(void)
		{
			return "Wall Style";
		}
		static Ogre::String AttrSnapToGrid(void)
		{
			return "Snap To Grid";
		}
		static Ogre::String AttrGridSize(void)
		{
			return "Grid Size";
		}
		static Ogre::String AttrAdaptToGround(void)
		{
			return "Adapt To Ground";
		}
		static Ogre::String AttrCreatePillars(void)
		{
			return "Create Pillars";
		}
		static Ogre::String AttrPillarSize(void)
		{
			return "Pillar Size";
		}
		static Ogre::String AttrWallDatablock(void)
		{
			return "Wall Datablock";
		}
		static Ogre::String AttrPillarDatablock(void)
		{
			return "Pillar Datablock";
		}
		static Ogre::String AttrUVTiling(void)
		{
			return "UV Tiling";
		}
		static Ogre::String AttrFencePostSpacing(void)
		{
			return "Fence Post Spacing";
		}
		static Ogre::String AttrBattlementWidth(void)
		{
			return "Battlement Width";
		}
		static Ogre::String AttrBattlementHeight(void)
		{
			return "Battlement Height";
		}
	protected:
		// Mouse handling for interactive wall building
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

		virtual bool keyPressed(const OIS::KeyEvent& evt) override;

		virtual bool keyReleased(const OIS::KeyEvent& evt) override;
	private:
		void createWallMesh(void);
		void createWallMeshInternal(const std::vector<float>& wallVertices, const std::vector<Ogre::uint32>& wallIndices, size_t numWallVertices,
			const std::vector<float>& pillarVertices, const std::vector<Ogre::uint32>& pillarIndices, size_t numPillarVertices,
			const Ogre::Vector3& wallOrigin);
		void destroyWallMesh(void);
		void destroyPreviewMesh(void);
		void updatePreviewMesh(void);

		// Geometry generation helpers
		void generateSolidWall(const WallSegment& segment);
		void generateFenceWall(const WallSegment& segment);
		void generateBattlementWall(const WallSegment& segment);
		void generateArchWall(const WallSegment& segment);
		void generatePillar(const Ogre::Vector3& position, Ogre::Real groundHeightOffset);

		void generateSolidWallWithSubdivision(const WallSegment& segment, int subdivisions);

		void addBox(Ogre::Real minX, Ogre::Real minY, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxY, Ogre::Real maxZ, Ogre::Real uTile, Ogre::Real vTile);
		void addQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real uTile, Ogre::Real vTile);

		void addPillarQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real uTile, Ogre::Real vTile);

		Ogre::Vector3 snapToGridFunc(const Ogre::Vector3& position);
		WallStyle getWallStyleEnum(void) const;

		Ogre::String getWallDataFilePath(void) const;

		bool saveWallDataToFile(void);

		bool loadWallDataFromFile(void);

		void deleteWallDataFile(void);

		void handleMeshModifyMode(NOWA::EventDataPtr eventData);

		void handleGameObjectSelected(NOWA::EventDataPtr eventData);

		void addInputListener(void);

		void removeInputListener(void);

	private:
		static const uint32_t WALLDATA_MAGIC = 0x57414C4C;  // "WALL" in hex
		static const uint32_t WALLDATA_VERSION = 1;
	private:
		Ogre::String name;
		// Attributes
		Variant* activated;
		Variant* wallHeight;
		Variant* wallThickness;
		Variant* wallStyle;
		Variant* snapToGrid;
		Variant* gridSize;
		Variant* adaptToGround;
		Variant* createPillars;
		Variant* pillarSize;
		Variant* wallDatablock;
		Variant* pillarDatablock;
		Variant* uvTiling;
		Variant* fencePostSpacing;
		Variant* battlementWidth;
		Variant* battlementHeight;

		// Wall segments
		std::vector<WallSegment> wallSegments;
		WallSegment currentSegment;
		BuildState buildState;

		// Mesh data
		std::vector<float> vertices;
		std::vector<Ogre::uint32> indices;
		Ogre::uint32 currentVertexIndex;

		std::vector<float> pillarVertices;
		std::vector<Ogre::uint32> pillarIndices;
		Ogre::uint32 currentPillarVertexIndex;

		// Ogre objects
		Ogre::MeshPtr wallMesh;
		Ogre::Item* wallItem;
		Ogre::MeshPtr previewMesh;
		Ogre::Item* previewItem;
		Ogre::SceneNode* previewNode;

		// Input state
		bool isShiftPressed;
		bool isCtrlPressed;
		Ogre::Vector3 lastValidPosition;

		// For continuous wall building (shift-click)
		bool continuousMode;
		Ogre::Vector3 wallOrigin;
		bool hasWallOrigin;
		Ogre::Plane groundPlane;
		Ogre::RaySceneQuery* groundQuery;

		std::vector<float>          cachedWallVertices;
		std::vector<Ogre::uint32>   cachedWallIndices;
		size_t                      cachedNumWallVertices;

		std::vector<float>          cachedPillarVertices;
		std::vector<Ogre::uint32>   cachedPillarIndices;
		size_t                      cachedNumPillarVertices;

		Ogre::Vector3               cachedWallOrigin;
		bool canModify;
	};

} // namespace NOWA

#endif // PROCEDURAL_WALL_COMPONENT_H