/*
	OgreCrowd
	---------

	Copyright (c) 2012 Jonas Hauquier

	Additional contributions by:

	- mkultra333
	- Paul Wilson

	Sincere thanks and to:

	- Mikko Mononen (developer of Recast navigation libraries)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

*/

#include "OgreRecast.h"
#include "RecastInputGeom.h"
#include "DetourTileCache/DetourTileCacheBuilder.h"
#include "OgreRecastNavmeshPruner.h"
#include "RecastDebugDraw.h"

#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsUnlit.h"

namespace
{
	void createUnlitDatablock(const Ogre::String& datablockName)
	{
		Ogre::Hlms* hlms = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlms);

		Ogre::HlmsBlendblock blendblock;
		blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA); // Example: Transparent alpha blending

		Ogre::HlmsMacroblock macroblock;
		macroblock.mCullMode = Ogre::CULL_NONE; // Example: No culling
		macroblock.mDepthCheck = true;
		macroblock.mDepthWrite = true;

		// Check if datablock already exists
		Ogre::HlmsDatablock* existingDatablock = hlmsUnlit->getDatablock(datablockName);

		if (false == existingDatablock)
		{
			Ogre::HlmsUnlitDatablock* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
				hlmsUnlit->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(macroblock), Ogre::HlmsBlendblock(blendblock), Ogre::HlmsParamVec()));

			existingDatablock = datablock;
		}

		existingDatablock->setMacroblock(macroblock);
		existingDatablock->setBlendblock(blendblock);

		// Set color usage
		static_cast<Ogre::HlmsUnlitDatablock*>(existingDatablock)->setUseColour(true); // Enable manual colour
	}
}

OgreRecast::OgreRecast(Ogre::SceneManager* sceneMgr, const OgreRecastConfigParams& configParams)
	: m_pSceneMgr(sceneMgr),
	m_pRecastSN(NULL),
#if OGRE_RECAST_DEBUG
	m_sg(NULL),
#endif
	m_rebuildSg(false),
	mFilter(0),
	mNavmeshPruner(0),
	m_ctx(0),
	m_pRecastMOWalk(nullptr),
	m_pRecastMONeighbour(nullptr),
	m_pRecastMOBoundary(nullptr),
	m_configParams(configParams),
	m_debugDrawer(new OgreRecastDebugDraw(sceneMgr))
{
	// Init recast stuff in a safe state

	m_triareas = NULL;
	m_solid = NULL;
	m_chf = NULL;
	m_cset = NULL;
	m_pmesh = NULL;
	//m_cfg;   
	m_dmesh = NULL;
	m_geom = NULL;
	m_navMesh = NULL;
	m_navQuery = NULL;
	//m_navMeshDrawFlags;
	m_ctx = NULL;

	RecastCleanup(); // TODO ?? don't know if I should do this prior to making any recast stuff, but the demo did.
	m_pRecastMOPath = NULL;

	m_pRecastSN = m_pSceneMgr->getRootSceneNode()->createChildSceneNode();
	m_pRecastSN->setName("RecastSN");


	m_pLog = Ogre::LogManager::getSingletonPtr();


	// Set default size of box around points to look for nav polygons
	mExtents[0] = 32.0f; mExtents[1] = 32.0f; mExtents[2] = 32.0f;

	// Setup the default query filter
	mFilter = new dtQueryFilter();
	mFilter->setIncludeFlags(0xFFFF);    // Include all
	mFilter->setExcludeFlags(0);         // Exclude none
	// Area flags for polys to consider in search, and their cost
	mFilter->setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);       // TODO have a way of configuring the filter
	mFilter->setAreaCost(DT_TILECACHE_WALKABLE_AREA, 1.0f);


	// Init path store. MaxVertex 0 means empty path slot
	for (int i = 0; i < MAX_PATHSLOT; i++)
	{
		m_PathStore[i].MaxVertex = 0;
		m_PathStore[i].Target = 0;
	}

	// Must be called, so that nav mesh debug colls do not overpaint other rendered enities/items
	createUnlitDatablock("recastdebug");
	createUnlitDatablock("recastdebug1");

	// Set configuration
	configure(m_configParams);
}

OgreRecast::~OgreRecast(void)
{
	if (nullptr != m_debugDrawer)
	{
		delete m_debugDrawer;
		m_debugDrawer = nullptr;
	}
}


/**
 * Cleanup recast stuff, not debug manualobjects.
**/
void OgreRecast::RecastCleanup()
{
	if (m_triareas) delete[] m_triareas;
	m_triareas = 0;

	rcFreeHeightField(m_solid);
	m_solid = 0;
	rcFreeCompactHeightfield(m_chf);
	m_chf = 0;
	rcFreeContourSet(m_cset);
	m_cset = 0;
	rcFreePolyMesh(m_pmesh);
	m_pmesh = 0;
	rcFreePolyMeshDetail(m_dmesh);
	m_dmesh = 0;
	dtFreeNavMesh(m_navMesh);
	m_navMesh = 0;

	dtFreeNavMeshQuery(m_navQuery);
	m_navQuery = 0;

	if (m_ctx)
	{
		delete m_ctx;
		m_ctx = 0;
	}
}


void OgreRecast::configure(const OgreRecastConfigParams& params)
{
	m_configParams = params;
	// NOTE: this is one of the most important parts to get it right!!
	// Perhaps the most important part of the above is setting the agent size with m_agentHeight and m_agentRadius,
	// and the voxel cell size used, m_cellSize and m_cellHeight. In my project 1 units is a little less than 1 meter,
	// so I've set the agent to 2.5 units high, and the cell sizes to sub-meter size.
	// This is about the same as in the original cell sizes in the Recast/Detour demo.

	// Smaller cellsizes are the most accurate at finding all the places we could go, but are also slow to generate.
	// Might be suitable for pre-generated meshes. Though it also produces a lot more polygons.

	if (m_ctx)
	{
		delete m_ctx;
		m_ctx = 0;
	}
	m_ctx = new rcContext(true);

	m_cellSize = m_configParams.getCellSize();
	m_cellHeight = m_configParams.getCellHeight();
	m_agentMaxSlope = m_configParams.getAgentMaxSlope();
	m_agentHeight = m_configParams.getAgentHeight();
	m_agentMaxClimb = m_configParams.getAgentMaxClimb();
	m_agentRadius = m_configParams.getAgentRadius();
	m_edgeMaxLen = m_configParams.getEdgeMaxLen();
	m_edgeMaxError = m_configParams.getEdgeMaxError();
	m_regionMinSize = m_configParams.getRegionMinSize();
	m_regionMergeSize = m_configParams.getRegionMergeSize();
	m_vertsPerPoly = m_configParams.getVertsPerPoly();
	m_detailSampleDist = m_configParams.getDetailSampleDist();
	m_detailSampleMaxError = m_configParams.getDetailSampleMaxError();
	m_keepInterResults = m_configParams.getKeepInterResults();

	// Init cfg object
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = m_configParams._getWalkableheight();
	m_cfg.walkableClimb = m_configParams._getWalkableClimb();
	m_cfg.walkableRadius = m_configParams._getWalkableRadius();
	m_cfg.maxEdgeLen = m_configParams._getMaxEdgeLen();
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionArea = m_configParams._getMinRegionArea();
	m_cfg.mergeRegionArea = m_configParams._getMergeRegionArea();
	m_cfg.maxVertsPerPoly = m_vertsPerPoly;
	m_cfg.detailSampleDist = (float)m_configParams._getDetailSampleDist();
	m_cfg.detailSampleMaxError = (float)m_configParams._getDetailSampleMaxError();


	// Demo specific parameters
	m_navMeshOffsetFromGround = m_cellHeight / 5;         // Distance above ground for drawing navmesh polygons
	m_navMeshEdgesOffsetFromGround = m_cellHeight / 3;    // Distance above ground for drawing edges of navmesh (should be slightly higher than navmesh polygons)
	m_pathOffsetFromGround = m_agentHeight + m_navMeshOffsetFromGround; // Distance above ground for drawing path debug lines relative to cellheight (should be higher than navmesh polygons)

	// Colors for navmesh debug drawing
	m_navmeshNeighbourEdgeCol = Ogre::ColourValue(0.9, 0.9, 0.9);   // Light Grey
	m_navmeshOuterEdgeCol = Ogre::ColourValue(0, 0, 0);         // Black
	m_navmeshGroundPolygonCol = Ogre::ColourValue(0, 0.7, 0);       // Green
	m_navmeshOtherPolygonCol = Ogre::ColourValue(0, 0.175, 0);     // Dark green
	m_pathCol = Ogre::ColourValue(1, 0, 0);         // Red
}



/**
 * Now for the navmesh creation function.
 * I've mostly taken this from the demo, apart from the top part where I create the triangles. Recast needs a bunch of input vertices and triangles from your map to build the navigation mesh. Where you get those verts amd triangles is up to you, my map loader was already outputing verts and triangle so it was easy to use those. Make sure the triangles wind the correct way or Recast will try to build the navmesh on the outside of your map.
 * There's some potentially groovy stuff in there that I haven't touched, like filtering and different weights for different types of zones. Also I've just gone for the simplest navmesh type, there's also other modes like tiling navmeshes which I've ignored.
 * This method is heavily based on Sample_SoloMesh::handleBuild() from the recastnavigation demo.
 *
 * Perhaps the most important part of the above is setting the agent size with m_agentHeight and m_agentRadius, and the voxel cell size used, m_cellSize and m_cellHeight. In my project 32.0 units is 1 meter, so I've set the agent to 48 units high, and the cell sizes are quite large. The original cell sizes in the Recast/Detour demo were down around 0.3.
**/
bool OgreRecast::NavMeshBuild(const std::vector<Ogre::v1::Entity*>& srcMeshes, const std::vector<Ogre::Item*>& srcItemsA)
{
	if (srcMeshes.empty())
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("Warning: Called NavMeshBuild without any entities/items. No navmesh was built.");
		return false;
	}

	return NavMeshBuild(new InputGeom(srcMeshes, srcItemsA));
}

bool OgreRecast::NavMeshBuild(InputGeom* input)
{
	// TODO: clean up unused variables


	m_pLog->logMessage("NavMeshBuild Start");


	//
	// Step 1. Initialize build config.
	//

	// Reset build times gathering.
	m_ctx->resetTimers();

	// Start the build process.
	m_ctx->startTimer(RC_TIMER_TOTAL);



	//
	// Step 2. Rasterize input polygon soup.
	//

	InputGeom *inputGeom = input;
	rcVcopy(m_cfg.bmin, inputGeom->getMeshBoundsMin());
	rcVcopy(m_cfg.bmax, inputGeom->getMeshBoundsMax());
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	int nverts = inputGeom->getVertCount();
	int ntris = inputGeom->getTriCount();
	Ogre::Vector3 min; FloatAToOgreVect3(inputGeom->getMeshBoundsMin(), min);
	Ogre::Vector3 max; FloatAToOgreVect3(inputGeom->getMeshBoundsMax(), max);

	//Ogre::LogManager::getSingletonPtr()->logMessage("Bounds: "+Ogre::StringConverter::toString(min) + "   "+ Ogre::StringConverter::toString(max));


	m_pLog->logMessage("Building navigation:");
	m_pLog->logMessage(" - " + Ogre::StringConverter::toString(m_cfg.width) + " x " + Ogre::StringConverter::toString(m_cfg.height) + " cells");
	m_pLog->logMessage(" - " + Ogre::StringConverter::toString(nverts / 1000.0f) + " K verts, " + Ogre::StringConverter::toString(ntris / 1000.0f) + " K tris");

	// Allocate voxel heightfield where we rasterize our input data to.
	m_solid = rcAllocHeightfield();
	if (!m_solid)
	{
		m_pLog->logMessage("ERROR: buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not create solid heightfield. Possibly it requires too much memory, try setting a higher cellSize and cellHeight value.");
		return false;
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// an array which can hold the max number of triangles you need to process.
	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		m_pLog->logMessage("ERROR: buildNavigation: Out of memory 'm_triareas' (" + Ogre::StringConverter::toString(ntris) + ").");
		return false;
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris * sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, inputGeom->getVerts(), inputGeom->getVertCount(), inputGeom->getTris(), inputGeom->getTriCount(), m_triareas);
	rcRasterizeTriangles(m_ctx, inputGeom->getVerts(), inputGeom->getVertCount(), inputGeom->getTris(), m_triareas, inputGeom->getTriCount(), *m_solid, m_cfg.walkableClimb);

	if (!m_keepInterResults)
	{
		delete[] m_triareas;
		m_triareas = 0;
	}





	//
	// Step 3. Filter walkables surfaces.
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);








	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		m_pLog->logMessage("ERROR: buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not build compact data.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}


	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not erode walkable areas.");
		return false;
	}

	// TODO implement
	   // (Optional) Mark areas.
	   //const ConvexVolume* vols = m_geom->getConvexVolumes();
	   //for (int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
	   //   rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);


	   // Prepare for region partitioning, by calculating distance field along the walkable surface.
	if (!rcBuildDistanceField(m_ctx, *m_chf))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not build distance field.");
		return false;
	}

	// Partition the walkable surface into simple regions without holes.
	if (!rcBuildRegions(m_ctx, *m_chf, m_cfg.borderSize, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not build regions.");
		return false;
	}








	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		m_pLog->logMessage("ERROR: buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not create contours.");
		return false;
	}

	if (m_cset->nconts == 0)
	{
		// In case of errors see: http://groups.google.com/group/recastnavigation/browse_thread/thread/a6fbd509859a12c8
		// You should probably tweak the parameters
		m_pLog->logMessage("ERROR: No contours created (Recast)!");
	}





	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		m_pLog->logMessage("ERROR: buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		// Try modifying the parameters. I experienced this error when setting agentMaxClimb too high.
		m_pLog->logMessage("ERROR: buildNavigation: Could not triangulate contours.");
		return false;
	}









	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		m_pLog->logMessage("ERROR: buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		m_pLog->logMessage("ERROR: buildNavigation: Could not build detail mesh.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.








	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.


	if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		m_pLog->logMessage("Detour 1000");

		unsigned char* navData = 0;
		int navDataSize = 0;


		// Update poly flags from areas.
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
			{
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			}
		}


		// Set navmesh params
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;

		// no off mesh connections yet
		m_offMeshConCount = 0;
		params.offMeshConVerts = m_offMeshConVerts;
		params.offMeshConRad = m_offMeshConRads;
		params.offMeshConDir = m_offMeshConDirs;
		params.offMeshConAreas = m_offMeshConAreas;
		params.offMeshConFlags = m_offMeshConFlags;
		params.offMeshConUserID = m_offMeshConId;
		params.offMeshConCount = m_offMeshConCount;

		params.walkableHeight = m_agentHeight;
		params.walkableRadius = m_agentRadius;
		params.walkableClimb = m_agentMaxClimb;
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;


		m_pLog->logMessage("Detour 2000");

		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			m_pLog->logMessage("ERROR: Could not build Detour navmesh.");
			return false;
		}

		m_pLog->logMessage("Detour 3000");

		m_navMesh = dtAllocNavMesh();
		if (!m_navMesh)
		{
			dtFree(navData);
			m_pLog->logMessage("ERROR: Could not create Detour navmesh");
			return false;
		}

		m_pLog->logMessage("Detour 4000");

		dtStatus status;

		status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			dtFree(navData);
			m_pLog->logMessage("ERROR: Could not init Detour navmesh");
			return false;
		}

		m_pLog->logMessage("Detour 5000");

		m_navQuery = dtAllocNavMeshQuery();
		status = m_navQuery->init(m_navMesh, 2048);

		m_pLog->logMessage("Detour 5500");

		if (dtStatusFailed(status))
		{
			m_pLog->logMessage("ERROR: Could not init Detour navmesh query");
			return false;
		}

		m_pLog->logMessage("Detour 6000");
	}

	m_ctx->stopTimer(RC_TIMER_TOTAL);


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	// cleanup stuff we don't need
 //   delete [] rc_verts ;
 //   delete [] rc_tris ;
 //   delete [] rc_trinorms ;

	//CreateRecastPolyMesh(*m_pmesh) ;   // Debug render it

	m_pLog->logMessage("NavMeshBuild End");
	return true;
}






#include <math.h>


/**
 * Now for the pathfinding code.
 * This takes a start point and an end point and, if possible, generates a list of lines in a path. It might fail if the start or end points aren't near any navmesh polygons, or if the path is too long, or it can't make a path, or various other reasons. So far I've not had problems though.
 *
 * nTarget: The index number for the slot in which the found path is to be stored
 * nPathSlot: Number identifying the target the path leads to
 *
 * Return codes:
 *  0   found path
 *  -1  Couldn't find polygon nearest to start point
 *  -2  Couldn't find polygon nearest to end point
 *  -3  Couldn't create a path
 *  -4  Couldn't find a path
 *  -5  Couldn't create a straight path
 *  -6  Couldn't find a straight path
**/
int OgreRecast::FindPath(float* pStartPos, float* pEndPos, int nPathSlot, int nTarget)
{
	dtStatus status;
	dtPolyRef StartPoly;
	float StartNearest[3];
	dtPolyRef EndPoly;
	float EndNearest[3];
	dtPolyRef PolyPath[MAX_PATHPOLY];
	int nPathCount = 0;
	float StraightPath[MAX_PATHVERT * 3];
	int nVertCount = 0;


	// find the start polygon
	status = m_navQuery->findNearestPoly(pStartPos, mExtents, mFilter, &StartPoly, StartNearest);
	if ((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK)) return -1; // couldn't find a polygon

	// find the end polygon
	status = m_navQuery->findNearestPoly(pEndPos, mExtents, mFilter, &EndPoly, EndNearest);
	if ((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK)) return -2; // couldn't find a polygon

	status = m_navQuery->findPath(StartPoly, EndPoly, StartNearest, EndNearest, mFilter, PolyPath, &nPathCount, MAX_PATHPOLY);
	if ((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK)) return -3; // couldn't create a path
	if (nPathCount == 0) return -4; // couldn't find a path

	status = m_navQuery->findStraightPath(StartNearest, EndNearest, PolyPath, nPathCount, StraightPath, NULL, NULL, &nVertCount, MAX_PATHVERT);
	if ((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK)) return -5; // couldn't create a path
	if (nVertCount == 0) return -6; // couldn't find a path

	// At this point we have our path.  Copy it to the path store
	int nIndex = 0;
	for (int nVert = 0; nVert < nVertCount; nVert++)
	{
		m_PathStore[nPathSlot].PosX[nVert] = StraightPath[nIndex++];
		m_PathStore[nPathSlot].PosY[nVert] = StraightPath[nIndex++];
		m_PathStore[nPathSlot].PosZ[nVert] = StraightPath[nIndex++];

		//sprintf(m_chBug, "Path Vert %i, %f %f %f", nVert, m_PathStore[nPathSlot].PosX[nVert], m_PathStore[nPathSlot].PosY[nVert], m_PathStore[nPathSlot].PosZ[nVert]) ;
		//m_pLog->logMessage(m_chBug);
	}
	m_PathStore[nPathSlot].MaxVertex = nVertCount;
	m_PathStore[nPathSlot].Target = nTarget;

	return nVertCount;

}

int OgreRecast::FindPath(Ogre::Vector3 startPos, Ogre::Vector3 endPos, int nPathSlot, int nTarget)
{
	float start[3];
	float end[3];
	OgreVect3ToFloatA(startPos, start);
	OgreVect3ToFloatA(endPos, end);

	return FindPath(start, end, nPathSlot, nTarget);
}

std::vector<Ogre::Vector3> OgreRecast::getPath(int pathSlot)
{
	std::vector<Ogre::Vector3> result;
	if (pathSlot < 0 || pathSlot >= MAX_PATHSLOT || m_PathStore[pathSlot].MaxVertex <= 0)
		return result;

	PATHDATA *path = &(m_PathStore[pathSlot]);
	result.reserve(path->MaxVertex);
	for (int i = 0; i < path->MaxVertex; i++)
	{
		result.push_back(Ogre::Vector3(path->PosX[i], path->PosY[i], path->PosZ[i]));
	}

	return result;
}


int OgreRecast::getTarget(int pathSlot)
{
	if (pathSlot < 0 || pathSlot >= MAX_PATHSLOT)
		return 0;

	return m_PathStore[pathSlot].Target;
}




/**
 * Debug drawing functionality:
**/

void OgreRecast::drawNavMesh(bool draw)
{
	m_debugDrawer->draw(draw);
	if (m_pmesh)
	{
		if (true == draw)
			duDebugDrawPolyMesh(m_debugDrawer, *m_pmesh);
		else
			removeDrawnNavmesh(0);
	}
}

void OgreRecast::recreateDrawer(void)
{
	m_debugDrawer->mustRecreate();
}

void OgreRecast::drawPolyMesh(const rcPolyMesh &mesh, bool draw, bool colorRegions)
{
	const int nvp = mesh.nvp;
	const float cs = mesh.cs;
	const float ch = mesh.ch;
	const float* orig = mesh.bmin;

	const unsigned short* verts = mesh.verts;
	const unsigned short* polys = mesh.polys;
	const unsigned char* areas = mesh.areas;
	const unsigned short* regions = mesh.regs;
	const int nverts = mesh.nverts;
	const int npolys = mesh.npolys;
	const int maxpolys = mesh.maxpolys;


	CreateRecastPolyMesh("SingleNavmesh", verts, nverts, polys, npolys, areas, maxpolys, regions, nvp, cs, ch, orig, draw, colorRegions);
}


// TODO make this only create an ogre entity, put the demo specific drawing in a separate DebugDrawing class __declspec( dllexport ) to separate it from the reusable recast wrappers
void OgreRecast::CreateRecastPolyMesh(const Ogre::String name, const unsigned short *verts, const int nverts, const unsigned short *polys, const int npolys, 
	const unsigned char *areas, const int maxpolys, const unsigned short *regions, const int nvp, const float cs, const float ch, const float *orig, bool draw, bool colorRegions)
{
	if (false == draw)
	{
		if (nullptr != m_pRecastMOWalk)
		{
			m_pRecastMOWalk->clear();
			m_pRecastMONeighbour->clear();
			m_pRecastMOBoundary->clear();

			m_pRecastSN->detachObject(m_pRecastMOWalk);
			m_pRecastSN->detachObject(m_pRecastMONeighbour);
			m_pRecastSN->detachObject(m_pRecastMOBoundary);

			m_pSceneMgr->destroyManualObject(m_pRecastMOWalk);
			m_pSceneMgr->destroyManualObject(m_pRecastMONeighbour);
			m_pSceneMgr->destroyManualObject(m_pRecastMOBoundary);

			m_pRecastMOWalk = nullptr;
			m_pRecastMONeighbour = nullptr;
			m_pRecastMOBoundary = nullptr;
		}

		return;
	}
	
	m_flDataX = npolys;
	m_flDataY = nverts;

	// When drawing regions choose different random colors for each region
 // TODO maybe cache generated colors so when rebuilding tiles the same colors can be reused? If possible
	Ogre::ColourValue* regionColors = NULL;
	if (colorRegions)
	{
		regionColors = new Ogre::ColourValue[maxpolys];
		for (int i = 0; i < maxpolys; ++i)
		{
			regionColors[i] = Ogre::ColourValue(Ogre::Math::RangeRandom(0, 1), Ogre::Math::RangeRandom(0, 1), Ogre::Math::RangeRandom(0, 1), 1);
		}
	}

	int nIndex = 0;
	m_nAreaCount = npolys;

	if (m_nAreaCount)
	{

		// start defining the manualObject with the navmesh planes
		m_pRecastMOWalk = m_pSceneMgr->createManualObject();
		m_pRecastMOWalk->setRenderQueueGroup(10);
		m_pRecastMOWalk->setCastShadows(false);
		m_pRecastMOWalk->setName("RecastMOWalk_" + name);
		m_pRecastMOWalk->begin("recastdebug", Ogre::OperationType::OT_TRIANGLE_LIST);
		for (int i = 0; i < npolys; ++i)
		{    // go through all polygons
			if (areas[i] == SAMPLE_POLYAREA_GROUND || areas[i] == DT_TILECACHE_WALKABLE_AREA)
			{
				const unsigned short* p = &polys[i*nvp * 2];

				unsigned short vi[3];
				for (int j = 2; j < nvp; ++j) // go through all verts in the polygon
				{
					if (p[j] == RC_MESH_NULL_IDX) break;
					vi[0] = p[0];
					vi[1] = p[j - 1];
					vi[2] = p[j];
					for (int k = 0; k < 3; ++k) // create a 3-vert triangle for each 3 verts in the polygon.
					{
						const unsigned short* v = &verts[vi[k] * 3];
						const float x = orig[0] + v[0] * cs;
						const float y = orig[1] + (v[1]/*+1*/)*ch;
						const float z = orig[2] + v[2] * cs;

						m_pRecastMOWalk->position(x, y + m_navMeshOffsetFromGround, z);
						if (colorRegions)
						{
							m_pRecastMOWalk->colour(regionColors[regions[i]]);  // Assign vertex color
						}
						else
						{
							if (areas[i] == SAMPLE_POLYAREA_GROUND)
								m_pRecastMOWalk->colour(m_navmeshGroundPolygonCol);
							else
								m_pRecastMOWalk->colour(m_navmeshOtherPolygonCol);
						}

					}
					m_pRecastMOWalk->triangle(nIndex, nIndex + 1, nIndex + 2);
					nIndex += 3;
				}
			}
		}
		m_pRecastMOWalk->end();


		nIndex = 0;

		// Define manualObject with the navmesh edges between neighbouring polygons
		m_pRecastMONeighbour = m_pSceneMgr->createManualObject();
		m_pRecastMONeighbour->setRenderQueueGroup(10);
		m_pRecastMONeighbour->setCastShadows(false);
		m_pRecastMONeighbour->setName("RecastMONeighbour_" + name);
		m_pRecastMONeighbour->begin("recastdebug", Ogre::OperationType::OT_LINE_LIST);

		for (int i = 0; i < npolys; ++i)
		{
			const unsigned short* p = &polys[i*nvp * 2];
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == RC_MESH_NULL_IDX) break;
				if (p[nvp + j] == RC_MESH_NULL_IDX) continue;
				int vi[2];
				vi[0] = p[j];
				if (j + 1 >= nvp || p[j + 1] == RC_MESH_NULL_IDX)
					vi[1] = p[0];
				else
					vi[1] = p[j + 1];
				for (int k = 0; k < 2; ++k)
				{
					const unsigned short* v = &verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1]/*+1*/)*ch /*+ 0.1f*/;
					const float z = orig[2] + v[2] * cs;
					//dd->vertex(x, y, z, coln);
					m_pRecastMONeighbour->position(x, y + m_navMeshEdgesOffsetFromGround, z);
					m_pRecastMONeighbour->colour(m_navmeshNeighbourEdgeCol);
					m_pRecastMONeighbour->index(nIndex++);
				}
				
				nIndex += 2;
			}
		}

		m_pRecastMONeighbour->end();

		nIndex = 0;

		// Define manualObject with navmesh outer edges (boundaries)
		m_pRecastMOBoundary = m_pSceneMgr->createManualObject();
		m_pRecastMOBoundary->setRenderQueueGroup(10);
		m_pRecastMOBoundary->setCastShadows(false);
		m_pRecastMOBoundary->setName("RecastMOBoundary_" + name);
		m_pRecastMOBoundary->begin("recastdebug", Ogre::OperationType::OT_LINE_LIST);

		for (int i = 0; i < npolys; ++i)
		{
			const unsigned short* p = &polys[i*nvp * 2];
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == RC_MESH_NULL_IDX) break;
				if (p[nvp + j] != RC_MESH_NULL_IDX) continue;
				int vi[2];
				vi[0] = p[j];
				if (j + 1 >= nvp || p[j + 1] == RC_MESH_NULL_IDX)
					vi[1] = p[0];
				else
					vi[1] = p[j + 1];
				for (int k = 0; k < 2; ++k)
				{
					const unsigned short* v = &verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1]/*+1*/)*ch /*+ 0.1f*/;
					const float z = orig[2] + v[2] * cs;
					//dd->vertex(x, y, z, colb);

					m_pRecastMOBoundary->position(x, y + m_navMeshEdgesOffsetFromGround, z);
					m_pRecastMOBoundary->colour(m_navmeshOuterEdgeCol);
					m_pRecastMOBoundary->index(nIndex++);
				}
				
				nIndex += 2;
			}
		}

		m_pRecastMOBoundary->end();
#if OGRE_RECAST_DEBUG
		// Render navmesh tiles more efficiently using staticGeometry

		// Early out if empty meshes drawn
		if (m_pRecastMOWalk->getNumSections() == 0)
			return;

		if (!m_sg)
		{
			m_sg = m_pSceneMgr->createStaticGeometry("NavmeshDebugStaticGeom");
			Ogre::Vector3 bmin; Ogre::Vector3 bmax; Ogre::Vector3 bsize;
			FloatAToOgreVect3(m_cfg.bmin, bmin);
			FloatAToOgreVect3(m_cfg.bmax, bmax);
			bsize = bmax - bmin;
			m_sg->setRegionDimensions(bsize);
			m_sg->setOrigin(bmin);
		}


		m_pRecastMOWalk->convertToMesh("mesh_" + m_pRecastMOWalk->getName());
		Ogre::v1::Entity *walkEnt = m_pSceneMgr->createEntity("ent_" + m_pRecastMOWalk->getName(), "mesh_" + m_pRecastMOWalk->getName());
		m_sg->addEntity(walkEnt, Ogre::Vector3::ZERO);

		// TODO line drawing does not work with staticGeometry
		if (false && m_pRecastMONeighbour->getNumSections() > 0)
		{
			m_pRecastMONeighbour->convertToMesh("mesh_" + m_pRecastMONeighbour->getName());     // Creating meshes from manualobjects without polygons is not a good idea!
			Ogre::v1::Entity *neighbourEnt = m_pSceneMgr->createEntity("ent_" + m_pRecastMONeighbour->getName(), "mesh_" + m_pRecastMONeighbour->getName());
			m_sg->addEntity(neighbourEnt, Ogre::Vector3::ZERO);
		}

		if (false && m_pRecastMOBoundary->getNumSections() > 0)
		{
			m_pRecastMOBoundary->convertToMesh("mesh_" + m_pRecastMOBoundary->getName());
			Ogre::v1::Entity *boundaryEnt = m_pSceneMgr->createEntity("ent_" + m_pRecastMOBoundary->getName(), "mesh_" + m_pRecastMOBoundary->getName());
			m_sg->addEntity(boundaryEnt, Ogre::Vector3::ZERO);
			}

		// Set dirty flag of solid geometry so it will be rebuilt next update()
		m_rebuildSg = true;
#else
		// Add manualobjects directly to scene (can be slow for lots of tiles)
		m_pRecastSN->attachObject(m_pRecastMOWalk);
		m_pRecastSN->attachObject(m_pRecastMONeighbour);
		m_pRecastSN->attachObject(m_pRecastMOBoundary);
#endif


		}// end areacount

	if (regionColors)
		delete[] regionColors;

#if OGRE_RECAST_DEBUG
	Ogre::LogManager::getSingletonPtr()->logMessage("Added navmesh part " + name + " to the scene.");
#endif
	}

float OgreRecast::getAgentRadius()
{
	return m_agentRadius;
}

float OgreRecast::getAgentHeight()
{
	return m_agentHeight;
}

float OgreRecast::getPathOffsetFromGround()
{
	return m_pathOffsetFromGround;
}

float OgreRecast::getNavmeshOffsetFromGround()
{
	return m_navMeshOffsetFromGround;
}

rcConfig OgreRecast::getConfig()
{
	return m_cfg;
}

OgreRecastConfigParams OgreRecast::getConfigParams(void) const
{
	return m_configParams;
}

void OgreRecast::update()
{
	// Fully rebuild static geometry after a reset (when tiles should be removed)
	if (m_rebuildSg)
	{
#if OGRE_RECAST_DEBUG
		m_sg->reset();

		// Add navmesh tiles (polys) to static geometry
		Ogre::SceneManager::MovableObjectIterator iterator = m_pSceneMgr->getMovableObjectIterator("Entity");
		while (iterator.hasMoreElements())
		{
			Ogre::v1::Entity* ent = static_cast<Ogre::v1::Entity*>(iterator.getNext());
			// Add all navmesh poly debug entities
			if (Ogre::StringUtil::startsWith(ent->getName(), "ent_recastmowalk_"))
				m_sg->addEntity(ent, Ogre::Vector3::ZERO);
		}
		m_sg->build();
#else
		Ogre::SceneManager::MovableObjectIterator iterator = m_pSceneMgr->getMovableObjectIterator("Entity");
#endif

		// Batch all lines together in one single manualObject (since we cannot use staticGeometry for lines)

		auto& itor = m_pSceneMgr->getMovableObjectIterator("ManualObject");
		while (itor.hasMoreElements())
		{
			// is that correct??
			Ogre::ManualObject* object = static_cast<Ogre::ManualObject*>(itor.peekNext());
			if (object->getName() == "AllNeighbourLines")
			{
				m_pRecastSN->detachObject(object);
				m_pSceneMgr->destroyManualObject(object);
				break;
			}
			itor.moveNext();
		}

		itor = m_pSceneMgr->getMovableObjectIterator("ManualObject");
		while (itor.hasMoreElements())
		{
			// is that correct??
			Ogre::ManualObject* object = static_cast<Ogre::ManualObject*>(itor.peekNext());
			if (object->getName() == "AllBoundaryLines")
			{
				m_pRecastSN->detachObject(object);
				m_pSceneMgr->destroyManualObject(object);
				break;
			}
			itor.moveNext();
		}

		Ogre::ManualObject *allNeighbourLines = m_pSceneMgr->createManualObject();
		allNeighbourLines->setRenderQueueGroup(10);
		allNeighbourLines->setName("AllNeighbourLines");
		allNeighbourLines->begin("recastdebug", Ogre::OperationType::OT_LINE_LIST);
		allNeighbourLines->colour(m_navmeshNeighbourEdgeCol);

		Ogre::ManualObject *allBoundaryLines = m_pSceneMgr->createManualObject();
		allBoundaryLines->setRenderQueueGroup(10);
		allBoundaryLines->setName("AllBoundaryLines");
		allBoundaryLines->begin("recastdebug", Ogre::OperationType::OT_LINE_LIST);
		allBoundaryLines->colour(m_navmeshOuterEdgeCol);

		iterator = m_pSceneMgr->getMovableObjectIterator("ManualObject");
		while (iterator.hasMoreElements())
		{
			Ogre::ManualObject* man = static_cast<Ogre::ManualObject*>(iterator.getNext());

			if (Ogre::StringUtil::startsWith(man->getName(), "recastmoneighbour_"))
			{
				std::vector<Ogre::Vector3> verts = getManualObjectVertices(man);

				for (std::vector<Ogre::Vector3>::iterator iter = verts.begin(); iter != verts.end(); iter++)
				{
					allNeighbourLines->position(*iter);
				}
			}
			else if (Ogre::StringUtil::startsWith(man->getName(), "recastmoboundary_"))
			{
				std::vector<Ogre::Vector3> verts = getManualObjectVertices(man);

				for (std::vector<Ogre::Vector3>::iterator iter = verts.begin(); iter != verts.end(); iter++)
				{
					allBoundaryLines->position(*iter);
				}
			}
		}
		allNeighbourLines->end();
		allBoundaryLines->end();

		m_pRecastSN->attachObject(allNeighbourLines);
		m_pRecastSN->attachObject(allBoundaryLines);


		m_rebuildSg = false;
	}
}

std::vector<Ogre::Vector3> OgreRecast::getManualObjectVertices(Ogre::ManualObject *manual)
{
	std::vector<Ogre::Vector3> returnVertices;
	unsigned long thisSectionStart = 0;
	for (size_t i = 0; i < manual->getNumSections(); i++)
	{
		Ogre::ManualObject::ManualObjectSection * section = manual->getSection(i);
		Ogre::v1::RenderOperation * renderOp = nullptr;
		section->getRenderOperation(*renderOp, false);
		//Collect the vertices
		{
			const Ogre::v1::VertexElement * vertexElement = renderOp->vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
			Ogre::v1::HardwareVertexBufferSharedPtr vertexBuffer = renderOp->vertexData->vertexBufferBinding->getBuffer(vertexElement->getSource());

			char * verticesBuffer = (char*)vertexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);
			float * positionArrayHolder;

			thisSectionStart = returnVertices.size();

			returnVertices.reserve(returnVertices.size() + renderOp->vertexData->vertexCount);

			for (unsigned int j = 0; j < renderOp->vertexData->vertexCount; j++)
			{
				vertexElement->baseVertexPointerToElement(verticesBuffer + j * vertexBuffer->getVertexSize(), &positionArrayHolder);
				Ogre::Vector3 vertexPos = Ogre::Vector3(positionArrayHolder[0],
					positionArrayHolder[1],
					positionArrayHolder[2]);

				//vertexPos = (orient * (vertexPos * scale)) + position;

				returnVertices.push_back(vertexPos);
			}

			vertexBuffer->unlock();
		}
	}

	return returnVertices;
}

void OgreRecast::CreateRecastPathLine(int nPathSlot, bool enable)
{
	// Remove previously drawn line
	if (m_pRecastMOPath)
	{
		try
		{
			m_pRecastSN->detachObject(m_pRecastMOPath);
			m_pSceneMgr->destroyManualObject(m_pRecastMOPath);
			// delete m_pRecastMOPath;
			m_pRecastMOPath = nullptr;
		}
		catch(...)
		{
		
		}
	}

	if (true == enable)
	{
		// Create new manualobject for the line
		m_pRecastMOPath = m_pSceneMgr->createManualObject();
		// m_pRecastMOPath = new Ogre::v1::ManualObject(0, &m_pSceneMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), m_pSceneMgr);
		m_pRecastMOPath->setRenderQueueGroup(10);
		m_pRecastMOPath->setName("recastdebug1");

		int nVertCount = m_PathStore[nPathSlot].MaxVertex;
		m_pRecastMOPath->estimateVertexCount(nVertCount);
		m_pRecastMOPath->estimateIndexCount(nVertCount / 2);
		m_pRecastMOPath->begin("recastdebug1", Ogre::OperationType::OT_LINE_STRIP);

		for (int nVert = 0; nVert < nVertCount; nVert++)
		{
			m_pRecastMOPath->position(m_PathStore[nPathSlot].PosX[nVert], m_PathStore[nPathSlot].PosY[nVert] + m_pathOffsetFromGround, m_PathStore[nPathSlot].PosZ[nVert]);
			m_pRecastMOPath->colour(Ogre::ColourValue::White);   // Assign vertex color
			m_pRecastMOPath->index(nVert);
			//sprintf(m_chBug, "Line Vert %i, %f %f %f", nVert, m_PathStore[nPathSlot].PosX[nVert], m_PathStore[nPathSlot].PosY[nVert], m_PathStore[nPathSlot].PosZ[nVert]) ;
			//m_pLog->logMessage(m_chBug);
		}

		m_pRecastMOPath->end();
		m_pRecastSN->attachObject(m_pRecastMOPath);
	}
}

void OgreRecast::RemoveRecastPathLine(void)
{
	if (m_pRecastMOPath)
	{
		try
		{
			m_pRecastSN->detachObject(m_pRecastMOPath);
			m_pSceneMgr->destroyManualObject(m_pRecastMOPath);
			// delete m_pRecastMOPath;
			m_pRecastMOPath = nullptr;
		}
		catch(...)
		{
		
		}
	}
}






/**
  * Helpers
  **/

void OgreRecast::OgreVect3ToFloatA(const Ogre::Vector3 vect, float* result)
{
	result[0] = vect[0];
	result[1] = vect[1];
	result[2] = vect[2];
};

void OgreRecast::FloatAToOgreVect3(const float* vect, Ogre::Vector3 &result)
{
	result.x = vect[0];
	result.y = vect[1];
	result.z = vect[2];
}

/**
  * Random number generator implementation used by getRandomNavMeshPoint method.
  **/
static float frand()
{
	return (float)rand() / (float)RAND_MAX;
}

Ogre::Vector3 OgreRecast::getRandomNavMeshPoint()
{
	float resultPoint[3];
	dtPolyRef resultPoly;
	m_navQuery->findRandomPoint(mFilter, frand, &resultPoly, resultPoint);

	return Ogre::Vector3(resultPoint[0], resultPoint[1], resultPoint[2]);
}

dtQueryFilter OgreRecast::getFilter()
{
	return *mFilter;    // Copy-on-return
}

void OgreRecast::setFilter(const dtQueryFilter filter)
{
	*mFilter = filter;    // Copy
		// TODO will this work? As I'm making a shallow copy of a class __declspec( dllexport ) that contains pointers
}

Ogre::Vector3 OgreRecast::getPointExtents()
{
	Ogre::Vector3 result;
	FloatAToOgreVect3(mExtents, result);
	return result;
}

void OgreRecast::setPointExtents(Ogre::Vector3 extents)
{
	OgreVect3ToFloatA(extents, mExtents);
}

Ogre::Vector3 OgreRecast::getRandomNavMeshPointInCircle(Ogre::Vector3 center, Ogre::Real radius)
{
	// First find nearest navmesh poly to center
	float pt[3];
	OgreVect3ToFloatA(center, pt);
	float rPt[3];
	dtPolyRef navmeshPoly;
	dtStatus status = m_navQuery->findNearestPoly(pt, mExtents, mFilter, &navmeshPoly, rPt);
	if ((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
		return center; // couldn't find a polygon


	// Then start searching at this poly for a random point within specified radius
	dtPolyRef resultPoly;
	m_navQuery->findRandomPointAroundCircle(navmeshPoly, pt, radius, mFilter, frand, &resultPoly, rPt);
	Ogre::Vector3 resultPt;
	FloatAToOgreVect3(rPt, resultPt);

	return resultPt;
}

Ogre::String OgreRecast::getPathFindErrorMsg(int errorCode)
{
	Ogre::String code = Ogre::StringConverter::toString(errorCode);
	switch (errorCode)
	{
	case 0:
		return code + " -- No error.";
	case -1:
		return code + " -- Couldn't find polygon nearest to start point.";
	case -2:
		return code + " -- Couldn't find polygon nearest to end point.";
	case -3:
		return code + " -- Couldn't create a path.";
	case -4:
		return code + " -- Couldn't find a path.";
	case -5:
		return code + " -- Couldn't create a straight path.";
	case -6:
		return code + " -- Couldn't find a straight path.";
	default:
		return code + " -- Unknown detour error code.";
	}
}

bool OgreRecast::findNearestPointOnNavmesh(Ogre::Vector3 position, Ogre::Vector3 &resultPt)
{
	dtPolyRef navmeshPoly;
	return findNearestPolyOnNavmesh(position, resultPt, navmeshPoly);
}

bool OgreRecast::findNearestPolyOnNavmesh(Ogre::Vector3 position, Ogre::Vector3 &resultPt, dtPolyRef &resultPoly)
{
	float pt[3];
	OgreVect3ToFloatA(position, pt);
	float rPt[3];
	dtStatus status = m_navQuery->findNearestPoly(pt, mExtents, mFilter, &resultPoly, rPt);
	if ((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
		return false; // couldn't find a polygon

	/*float height = 0.0f;
	m_navQuery->getPolyHeight(resultPoly, rPt, &height);
	Ogre::LogManager::getSingletonPtr()->logMessage("---->height: " + Ogre::StringConverter::toString(height));*/

	if (rPt[0] == 107374176. || rPt[0] == -107374176.)
		return false;

	FloatAToOgreVect3(rPt, resultPt);
	return true;
}


void OgreRecast::removeDrawnNavmesh(unsigned int tileRef)
{
	Ogre::String name = "RecastMOWalk_" + Ogre::StringConverter::toString(tileRef);
	Ogre::String entName = "";

	Ogre::MovableObject* object = nullptr;
	auto& itor = m_pRecastSN->getAttachedObjectIterator();
	while (itor.hasMoreElements())
	{
		object = itor.peekNext();
		if (object->getName() == name)
		{
			break;
		}
		itor.moveNext();
	}

	if (nullptr != object)
	{
		object->detachFromParent();
		m_pSceneMgr->destroyManualObject(dynamic_cast<Ogre::ManualObject*>(object));

		name = "RecastMONeighbour_" + Ogre::StringConverter::toString(tileRef);
		try
		{
			object = m_pRecastSN->getAttachedObject(name);
			object->detachFromParent();
			m_pSceneMgr->destroyManualObject(dynamic_cast<Ogre::ManualObject*>(object));
		}
		catch (...)
		{

		}

		try
		{
			name = "RecastMOBoundary_" + Ogre::StringConverter::toString(tileRef);
			object = m_pRecastSN->getAttachedObject(name);
			object->detachFromParent();
			m_pSceneMgr->destroyManualObject(dynamic_cast<Ogre::ManualObject*>(object));
		}
		catch (...)
		{

		}
	}
}

OgreRecastNavmeshPruner* OgreRecast::getNavmeshPruner()
{
	// Store singleton instance
	if (!mNavmeshPruner)
		mNavmeshPruner = new OgreRecastNavmeshPruner(this, m_navMesh);
	// TODO does m_navMesh object not change when navmesh is rebuilt?

// Return navmesh pruner for this recast mesh
	return mNavmeshPruner;
}
