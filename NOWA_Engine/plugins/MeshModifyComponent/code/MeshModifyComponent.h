/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef MESHMODIFYCOMPONENT_H
#define MESHMODIFYCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace Ogre
{
	class VertexBufferPacked;
	class IndexBufferPacked;
}

namespace NOWA
{
	/**
	 * @brief	Component for real-time mesh modification/sculpting.
	 *			Allows brush-based vertex manipulation on meshes.
	 *
	 *			Supported operations:
	 *			- Push/Pull vertices along normals
	 *			- Smooth vertices
	 *			- Flatten to plane
	 *			- Pinch/Inflate
	 *
	 *			Uses BT_DYNAMIC_PERSISTENT buffers for efficient GPU updates.
	 */
	class EXPORTED MeshModifyComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener
	{
	public:
		typedef boost::shared_ptr<MeshModifyComponent> MeshModifyComponentPtr;

		enum class BrushMode
		{
			PUSH,		// Push vertices outward along normal
			PULL,		// Pull vertices inward along normal
			SMOOTH,		// Average vertex positions with neighbors
			FLATTEN,	// Flatten to average plane
			PINCH,		// Pull vertices toward brush center
			INFLATE		// Push vertices along their own normals
		};

		// Interleaved vertex structure matching Ogre-Next PBS requirements
		struct VertexElementInfo
		{
			bool hasPosition;
			bool hasNormal;
			bool hasTangent;
			bool hasQTangent;		// Quaternion tangent (packed normal+tangent)
			bool hasUV;
			Ogre::VertexElementType positionType;
			Ogre::VertexElementType normalType;
			Ogre::VertexElementType tangentType;
			Ogre::VertexElementType uvType;

			VertexElementInfo() 
				: hasPosition(false), hasNormal(false), hasTangent(false), hasQTangent(false), hasUV(false),
				  positionType(Ogre::VET_FLOAT3), normalType(Ogre::VET_FLOAT3),
				  tangentType(Ogre::VET_FLOAT4), uvType(Ogre::VET_FLOAT2) {}
		};

	public:
		MeshModifyComponent();
		virtual ~MeshModifyComponent();

		/**
		 * @see		Ogre::Plugin::install
		 */
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		 * @see		Ogre::Plugin::initialise
		 */
		virtual void initialise() override {};

		/**
		 * @see		Ogre::Plugin::shutdown
		 */
		virtual void shutdown() override {};

		/**
		 * @see		Ogre::Plugin::uninstall
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

		// --- Brush Settings ---

		void setBrushName(const Ogre::String& brushName);
		Ogre::String getBrushName(void) const;

		void setBrushSize(Ogre::Real brushSize);
		Ogre::Real getBrushSize(void) const;

		void setBrushIntensity(Ogre::Real brushIntensity);
		Ogre::Real getBrushIntensity(void) const;

		void setBrushFalloff(Ogre::Real brushFalloff);
		Ogre::Real getBrushFalloff(void) const;

		void setBrushMode(const Ogre::String& brushMode);
		Ogre::String getBrushModeString(void) const;
		BrushMode getBrushMode(void) const;

		void setCategory(const Ogre::String& category);
		Ogre::String getCategory(void) const;

		// --- Mesh Operations ---

		/**
		 * @brief	Resets the mesh to its original state
		 */
		void resetMesh(void);

		/**
		 * @brief	Exports the modified mesh to a file
		 * @param	fileName	The output file name (without extension)
		 * @return	True if export was successful
		 */
		bool exportMesh(const Ogre::String& fileName);

		/**
		 * @brief	Gets the number of vertices in the editable mesh
		 */
		size_t getVertexCount(void) const;

		/**
		 * @brief	Gets the number of indices in the editable mesh
		 */
		size_t getIndexCount(void) const;

	public:
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MeshModifyComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MeshModifyComponent";
		}

		static bool canStaticAddComponent(GameObject* gameObject);

		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Attach to a GameObject with a mesh to enable real-time sculpting. "
				"Use left mouse button to sculpt. Ctrl+Left to invert brush effect.";
		}

		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrBrushName(void) { return "Brush Name"; }
		static const Ogre::String AttrBrushSize(void) { return "Brush Size"; }
		static const Ogre::String AttrBrushIntensity(void) { return "Brush Intensity"; }
		static const Ogre::String AttrBrushFalloff(void) { return "Brush Falloff"; }
		static const Ogre::String AttrBrushMode(void) { return "Brush Mode"; }
		static const Ogre::String AttrCategory(void) { return "Category"; }

	protected:
		virtual bool mouseMoved(const OIS::MouseEvent& evt) override;
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	private:
		/**
		 * @brief	Creates the editable mesh by cloning the original with dynamic buffers
		 * @return	True if successful
		 */
		bool createEditableMesh(void);

		/**
		 * @brief	Destroys the editable mesh and cleans up resources
		 */
		void destroyEditableMesh(void);

		/**
		 * @brief	Extracts vertex data from the original mesh into CPU-side arrays
		 * @return	True if successful
		 */
		bool extractMeshData(void);

		/**
		 * @brief	Creates dynamic vertex/index buffers for real-time updates
		 * @return	True if successful
		 */
		bool createDynamicBuffers(void);

		/**
		 * @brief	Uploads the current vertex data to the GPU
		 *			Must be called from render thread!
		 */
		void uploadVertexData(void);

		/**
		 * @brief	Calculates brush influence at a given distance
		 * @param	distance		Distance from brush center
		 * @param	brushRadius		Brush radius
		 * @return	Influence factor [0, 1]
		 */
		Ogre::Real calculateBrushInfluence(Ogre::Real distance, Ogre::Real brushRadius) const;

		/**
		 * @brief	Applies brush modification to vertices within range
		 * @param	brushCenterLocal	Brush center in local/object space
		 * @param	invertEffect		If true, inverts the brush effect
		 */
		void applyBrush(const Ogre::Vector3& brushCenterLocal, bool invertEffect);

		/**
		 * @brief	Recalculates normals for all vertices based on face normals
		 */
		void recalculateNormals(void);

		/**
		 * @brief	Recalculates tangents for all vertices (for normal mapping)
		 */
		void recalculateTangents(void);

		/**
		 * @brief	Detects the vertex format of the original mesh
		 */
		bool detectVertexFormat(Ogre::VertexArrayObject* vao);

		/**
		 * @brief	Finds neighboring vertices for smoothing operations
		 *			Builds adjacency information from index buffer
		 */
		void buildVertexAdjacency(void);

		/**
		 * @brief	Performs raycast to find brush position on mesh
		 * @param	screenX		Screen X coordinate [0, 1]
		 * @param	screenY		Screen Y coordinate [0, 1]
		 * @param	hitPosition	Output: Hit position in world space
		 * @param	hitNormal	Output: Surface normal at hit point
		 * @return	True if mesh was hit
		 */
		bool raycastMesh(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition, Ogre::Vector3& hitNormal);

		/**
		 * @brief	Converts world position to local/object space
		 */
		Ogre::Vector3 worldToLocal(const Ogre::Vector3& worldPos) const;

		/**
		 * @brief	Converts local position to world space
		 */
		Ogre::Vector3 localToWorld(const Ogre::Vector3& localPos) const;

	private:
		Ogre::String name;

		// Editable mesh data
		Ogre::Item* editableItem;
		Ogre::MeshPtr editableMesh;
		Ogre::VertexBufferPacked* dynamicVertexBuffer;
		Ogre::IndexBufferPacked* dynamicIndexBuffer;

		// CPU-side vertex data (working copy)
		std::vector<Ogre::Vector3> vertices;			// Current positions
		std::vector<Ogre::Vector3> normals;				// Current normals
		std::vector<Ogre::Vector4> tangents;			// Tangents (xyz = tangent, w = binormal sign)
		std::vector<Ogre::Vector2> uvCoordinates;		// Texture coordinates (unchanged)
		std::vector<Ogre::uint32> indices;				// Index buffer

		// Original data for reset
		std::vector<Ogre::Vector3> originalVertices;
		std::vector<Ogre::Vector3> originalNormals;
		std::vector<Ogre::Vector4> originalTangents;

		// Vertex adjacency for smoothing
		std::vector<std::vector<size_t>> vertexNeighbors;

		// Brush data (loaded from image)
		std::vector<Ogre::Real> brushData;
		size_t brushImageWidth;
		size_t brushImageHeight;

		// Mesh format info
		VertexElementInfo vertexFormat;
		bool isIndices32;
		size_t vertexCount;
		size_t indexCount;
		size_t bytesPerVertex;

		// Input state
		Ogre::RaySceneQuery* raySceneQuery;
		bool isPressing;
		bool isCtrlPressed;
		Ogre::Vector3 lastBrushPosition;

		// Pending upload flag (for thread-safe updates)
		std::atomic<bool> pendingUpload;

		// Attributes
		Variant* activated;
		Variant* brushName;
		Variant* brushSize;
		Variant* brushIntensity;
		Variant* brushFalloff;
		Variant* brushMode;
		Variant* category;
	};

}; // namespace end

#endif
