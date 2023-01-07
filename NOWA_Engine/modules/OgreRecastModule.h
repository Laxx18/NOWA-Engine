#ifndef OGRE_RECAST_MODULE_H
#define OGRE_RECAST_MODULE_H

#include "OgreRecastDefinitions.h"
#include "OgreRecast.h"
#include "OgreDetourCrowd.h"
#include "OgreDetourTileCache.h"

#include "main/Events.h"
#include "defines.h"

namespace NOWA
{
	class GameObject;

	class EXPORTED OgreRecastModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		/** 
		 * @param[in] sceneManager 	The sceneManager for the geometry.
		 * @param[in] params 		The navigation mesh configuration parameters. Description:
		 * 								cellSize: Is the width and depth resolution used when sampling the source geometry.
		 * 								The width and depth of the cell columns that make up voxel fields.
		 * 								Cells are laid out on the width/depth plane of voxel fields. Width is associated with the x-axis of the source geometry. Depth is associated with the z-axis.
		 * 								A lower value allows for the generated meshes to more closely match the source geometry, but at a higher processing and memory cost.
		 * 								The xz-plane cell size to use for fields. [Limit: > 0] [Units: wu].
		 * 								CellSize and CellHeight define voxel/grid/cell size. So their values have significant side effects on all parameters defined in voxel units.
		 * 								The minimum value for this parameter depends on the platform's floating point accuracy, with the practical minimum usually around 0.05.
		 *
		 *  							cellheight: Is the height resolution used when sampling the source geometry. The height of the voxels in voxel fields.
		 * 								Height is associated with the y-axis of the source geometry.
		 * 								A smaller value allows for the final meshes to more closely match the source geometry at a potentially higher processing cost.
		 * 								(Unlike cellSize, using a lower value for cellHeight does not significantly increase memory use.)
		 * 								The y-axis cell size to use for fields. [Limit: > 0] [Units: wu].
		 * 								CellSize and CellHeight define voxel/grid/cell size. So their values have significant side effects on all parameters defined in voxel units.
		 * 								The minimum value for this parameter depends on the platform's floating point accuracy, with the practical minimum usually around 0.05.
		 * 								Setting ch lower will result in more accurate detection of areas the agent can still pass under, as min walkable height is discretisized
		 * 								in number of cells. Also walkableClimb's precision is affected by ch in the same way, along with some other parameters.
		 *
		 *								agentMaxSlope: The maximum slope that is considered traversable (in degrees).
		 * 								[Limits: 0 <= value < 90]
		 * 								The practical upper limit for this parameter is usually around 85 degrees.
		 *
		 *								agentHeight: The height of an agent. Defines the minimum height that agents can walk under. Parts of the navmesh with lower ceilings will be pruned off.
		 *                              This parameter serves at setting walkableHeight (minTraversableHeight) parameter, precision of this parameter is determined by cellHeight.
		 *
		 *								agentMaxClimb: The Maximum ledge height that is considered to still be traversable.
		 * 								This parameter serves at setting walkableClimb (maxTraversableStep) parameter, precision of this parameter is determined by cellHeight (ch).
		 * 								[Limit: >=0]
		 * 								Allows the mesh to flow over low lying obstructions such as curbs and up/down stairways. The value is usually set to how far up/down an agent can step.
		 *
		 *								agentRadius: The radius on the xz (ground) plane of the circle that describes the agent (character) size.
		 * 								Serves at setting walkableRadius (traversableAreaBorderSize) parameter, the precision of walkableRadius is affected by cellSize (cs).
		 * 								This parameter is also used by DetourCrowd to determine the area other agents have to avoid in order not to collide with an agent.
		 * 								The distance to erode/shrink the walkable area of the height field away from obstructions.
		 * 								[Limit: >=0]
		 * 								In general, this is the closest any part of the final mesh should get to an obstruction in the source geometry. It is usually set to the maximum agent radius.
		 * 								While a value of zero is legal, it is not recommended and can result in odd edge case issues.
		 *
		 *								maxEdgeLen: The maximum allowed length for contour edges along the border of the mesh.
		 * 								[Limit: >=0]
		 * 								Extra vertices will be inserted as needed to keep contour edges below this length. A value of zero effectively disables this feature.
		 * 								Serves at setting maxEdgeLen, the precision of maxEdgeLen is affected by cellSize.
		 *
		 *								edgeMaxError: The maximum distance a simplified contour's border edges should deviate the original raw contour. (edge matching)
		 * 								[Limit: >=0] [Units: wu]
		 * 								The effect of this parameter only applies to the xz-plane.
		 * 								Also called maxSimplificationError or edgeMaxDeviation.
		 * 								The maximum distance the edges of meshes may deviate from the source geometry.
		 * 								A lower value will result in mesh edges following the xz-plane geometry contour more accurately at the expense of an increased triangle count.
		 * 								A value to zero is not recommended since it can result in a large increase in the number of polygons in the final meshes at a high processing cost.
		 *
		 *								regionMinSize: The minimum number of cells allowed to form isolated island areas (size).
		 * 							 	[Limit: >=0]
		 * 								Any regions that are smaller than this area will be marked as unwalkable. This is useful in removing useless regions that can sometimes form on geometry such as table tops, box tops, etc.
		 * 								Serves at setting minRegionArea, which will be set to the square of this value (the regions are square, thus area = size * size)
		 *
		 *								regionMergeSize: Any regions with a span count smaller than this value will, if possible, be merged with larger regions.
		 * 								[Limit: >=0] [Units: vx]
		 * 								Serves at setting MergeRegionArea, which will be set to the square of this value (the regions are square, thus area = size * size)
		 *
		 *								 vertsPerPoly: The maximum number of vertices allowed for polygons generated during the contour to polygon conversion process.
		 * 								[Limit: >= 3]
		 * 								If the mesh data is to be used to construct a Detour navigation mesh, then the upper limit is limited to <= DT_VERTS_PER_POLYGON (=6).
		 * 								The maximum number of vertices per polygon for polygons generated during the voxel to polygon conversion process.
		 * 								Higher values increase processing cost, but can also result in better formed polygons in the final meshes.
		 *								A value of around 6 is generally adequate with diminishing returns for higher values.
		 *
		 *								detailSampleDist: Sets the sampling distance to use when generating the detail mesh.
		 * 								(For height detail only.) [Limits: 0 or >= 0.9] [Units: wu]
		 * 								Also called contourSampleDistance.
		 * 								Sets the sampling distance to use when matching the detail mesh to the surface of the original geometry.
		 * 								Impacts how well the final detail mesh conforms to the surface contour of the original geometry.
		 *								Higher values result in a detail mesh which conforms more closely to the original geometry's surface at the cost of a higher final triangle count and higher processing cost.
		 * 								Setting this argument to less than 0.9 disables this functionality.
		 *
		 *								detailSampleMaxError: The maximum distance the detail mesh surface should deviate from height field data.
		 * 								(For height detail only.) [Limit: >=0] [Units: wu]
		 * 								Also called contourMaxDeviation
		 * 								The maximum distance the surface of the detail mesh may deviate from the surface of the original geometry.
		 * 								The accuracy is impacted by contourSampleDistance.
		 * 								The value of this parameter has no meaning if contourSampleDistance is set to zero.
		 * 								Setting the value to zero is not recommended since it can result in a large increase in the number of triangles in the final detail mesh at a high processing cost.
		 *
		 *								keepInterResult: Determines whether intermediary results are stored in OgreRecast class or whether they are removed after navmesh creation.
		 * @param[in] pointExtends 	Sets the offset size (box) around points used to look for navigation polygons. This offset is used in all search for points on the navigation mesh.
		 *                     		The maximum offset that a specified point can be off from the navigation mesh.
		 */
		OgreRecast* createOgreRecast(Ogre::SceneManager* sceneManager, OgreRecastConfigParams params, const Ogre::Vector3& pointExtends = Ogre::Vector3(5.0f, 5.0f, 5.0f));

		/** 
		 * @brief Destroyes the internal navigation content.
		 */
		void destroyContent(void);

		/** 
		 * @brief Adds a static (not movable) obstacle mesh from the given game object id. The obstacle is unwalkable, so the path will be created around this obstacle.
		 * @param[in] id The game object id to get the mesh.
		 */
		void addStaticObstacle(unsigned long id);
		
		/** 
		 * @brief Removes the static (not movable) obstacle mesh from the given game object id. This can be used e.g. to simulate an open or closed gate.
		 * @param[in] id The game object id to get the mesh.
		 */
		void removeStaticObstacle(unsigned long id);

		/** 
		 * @brief Adds a dynamic (movable) obstacle mesh from the given game object id. The obstacle is unwalkable, so the path will be created around this obstacle.
		 *  	  The object can be moved at runtime, so that path will be recalculated.
		 * @param[in] id					The game object id to get the mesh.
		 * @param[in] walkable				Whether this game object mesh is walk able or not
		 * @param[in] externalInputGeom		The external input geom to add, instead creating a new one
		 * @param[in] externalConvexVolume	The external convex volume to add, instead creating a new one
		 */
		void addDynamicObstacle(unsigned long id, bool walkable, InputGeom* externalInputGeom = nullptr, ConvexVolume* externalConvexVolume = nullptr);

		/** 
		 * @brief Removes the dynamic (movable) obstacle mesh from the given game object id.
		 * @param[in] id		The game object id to get the mesh.
		 * @param[in] destroy	If set to true, the internal convex volume and input geom will also be destroyed
		 * @return The InputGeom and ConvexVolue, but only if the parameter destroy is set to false, else the pair with 2x nullptr is delivered
		 */
		std::pair<InputGeom*, ConvexVolume*> removeDynamicObstacle(unsigned long id, bool destroy = true);

		/** 
		 * @brief Activates or deactivates an existing dynamic (movable) obstacle mesh from the given game object id at runtime, without the need of removing it for better performance.
		 * @param[in] id The game object id to get the mesh.
		 */
		void activateDynamicObstacle(unsigned long id, bool activate);

		/** 
		 * @brief Adds terra for input geom mesh from the given game object id.
		 * @param[in] id					The game object id to get the mesh.
		 */
		void addTerra(unsigned long id);

		/** 
		 * @brief Removes terra and input geom mesh.
		 * @param[in] id		The game object id to get the mesh.
		 * @param[in] destroy	If set to true, the internal input geom will also be destroyed
		 * @return				The InputGeom, but only if the parameter destroy is set to false, else the pair with 2x nullptr is delivered
		 */
		InputGeom* removeTerra(unsigned long id, bool destroy = true);

		/** 
		 * @brief Gets whether OgreRecast has been feed with game object meshes yet. This can be used to determine whether to build the navigation mesh or not. 
		 * @return hasNavigationElement true, if there are some navigation elements added by @addStaticObstacle or @addDynamicObstacle
		 */
		bool hasNavigationMeshElements(void) const;

		/** 
		 * @brief Builds the navigation mesh. Its likely that it will not work at the first, as maybe some config params are not tweaked correctly. So an error log is produced and an event fired
		 *		  @eventDataNavigationMeshFeedback so that the developer may react whether the navigation mesh could be created or not.
		 */
		void buildNavigationMesh(void);

		/** 
		 * @brief Debug draws the navigation mesh for better analysis.
		 * @param[in] draw Whether to draw or not
		 */
		void debugDrawNavMesh(bool draw);

		/** 
		 * @brief Updates the Ogre recast navigation data like moving dynamic obstacles etc. and re-adapding navigation mesh.
		 * @param[in] dt The delta time, usually 0.016 = 1 / 60 frames
		 */
		void update(Ogre::Real dt);

	   /** 
		* @brief Tries to find an optimal path. It takes a start point and an end point and, if possible, generates a list of lines in a path.
		* It might fail if the start or end points aren't near any navigation mesh polygons, or if the path is too long, or it can't make a path, or various other reasons.So far I've not had problems though.
		* @param[in] startPosition The start position for the path finding
		* @param[in] endPosition The end position for the path finding
		* @param[in] pathSlot Number identifying the target the path leads to
		* @param[in] targetSlot: The index number for the slot in which the found path is to be stored
		* @param[in] drawPath: Whether to draw the path or not
		* @return	 true, if a path could be found, else false
		*/
		bool findPath(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, int pathSlot, int targetSlot, bool drawPath = false);

		/** 
		* @brief Removes the drawn path.
		*/
		void removeDrawnPath(void);

		/** 
		 * @brief Gets the OgreRecast pointer. From this pointer the internal Recast data structure is callable if the developer wants to more customize navigation.
		 * @return OgreRecast The OgreRecast pointer to get.
		 */
		OgreRecast* getOgreRecast(void) const;

		OgreDetourTileCache* getOgreDetourTileCache(void) const;

		OgreDetourCrowd* getOgreDetourCrowd(void) const;

		void handleSceneModified(NOWA::EventDataPtr eventData);
	private:
		OgreRecastModule(const Ogre::String& appStateName);
		~OgreRecastModule();

		void createInputGeom(void);
	private:
		Ogre::String appStateName;
		OgreRecast* ogreRecast;
		OgreDetourTileCache* detourTileCache;
		OgreDetourCrowd* detourCrowd;
		std::vector<unsigned long> staticObstacles;
		std::map<unsigned long, std::pair<InputGeom*, ConvexVolume*>> dynamicObstacles;
		std::map<unsigned long, InputGeom*> terraInputGeomCells;
		bool mustRegenerate;
		bool hasValidNavMesh;
		bool debugDraw;
	};

}; //namespace end

#endif
