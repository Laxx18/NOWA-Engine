/*
    OgreCrowd — OgreRecast.cpp

    Copyright (c) 2012 Jonas Hauquier
    Additional contributions by mkultra333, Paul Wilson.

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

#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsUnlit.h"

// ---------------------------------------------------------------------------
// Local helper: create an unlit datablock for debug drawing
// ---------------------------------------------------------------------------
namespace
{
    void createUnlitDatablock(const Ogre::String& name)
    {
        Ogre::Hlms* hlms = Ogre::Root::getSingleton()
            .getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
        Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlms);

        Ogre::HlmsBlendblock blendblock;
        blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);

        Ogre::HlmsMacroblock macroblock;
        macroblock.mCullMode = Ogre::CULL_NONE;
        macroblock.mDepthCheck = true;
        macroblock.mDepthWrite = true;

        Ogre::HlmsDatablock* db = hlmsUnlit->getDatablock(name);
        if (nullptr == db)
        {
            db = hlmsUnlit->createDatablock(name, name,
                Ogre::HlmsMacroblock(macroblock),
                Ogre::HlmsBlendblock(blendblock),
                Ogre::HlmsParamVec());
        }
        db->setMacroblock(macroblock);
        db->setBlendblock(blendblock);
        static_cast<Ogre::HlmsUnlitDatablock*>(db)->setUseColour(true);
    }

    static float frand()
    {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
}

// ===========================================================================
// Construction / destruction
// ===========================================================================

OgreRecast::OgreRecast(Ogre::SceneManager* sceneMgr,
    const OgreRecastConfigParams& configParams)
    : m_pSceneMgr(sceneMgr),
    m_debugDrawer(new OgreRecastDebugDraw(sceneMgr)),
    m_pRecastMOPath(nullptr),
    m_pRecastSN(nullptr),
    m_triareas(nullptr),
    m_solid(nullptr),
    m_chf(nullptr),
    m_cset(nullptr),
    m_pmesh(nullptr),
    m_dmesh(nullptr),
    m_ctx(nullptr),
    m_geom(nullptr),
    m_navMesh(nullptr),
    m_navQuery(nullptr),
    m_offMeshConCount(0),
    m_rebuildSg(false),
    mNavmeshPruner(nullptr),
    mFilter(nullptr),
    m_configParams(configParams)
{
    m_pLog = Ogre::LogManager::getSingletonPtr();

    RecastCleanup();

    m_pRecastSN = m_pSceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
    m_pRecastSN->setName("RecastSN");

    // Default search extents
    mExtents[0] = 32.0f; mExtents[1] = 32.0f; mExtents[2] = 32.0f;

    mFilter = new dtQueryFilter();
    mFilter->setIncludeFlags(0xFFFF);
    mFilter->setExcludeFlags(0);
    mFilter->setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
    mFilter->setAreaCost(DT_TILECACHE_WALKABLE_AREA, 1.0f);

    for (int i = 0; i < MAX_PATHSLOT; ++i)
    {
        m_PathStore[i].MaxVertex = 0;
        m_PathStore[i].Target = 0;
    }

    // Datablocks for debug drawing (navmesh visual + path line)
    createUnlitDatablock("recastdebug");
    createUnlitDatablock("recastdebug1");

    configure(m_configParams);
}

OgreRecast::~OgreRecast()
{
    delete m_debugDrawer;
    m_debugDrawer = nullptr;

    delete mFilter;
    mFilter = nullptr;

    delete mNavmeshPruner;
    mNavmeshPruner = nullptr;

    // Per-slot queries
    {
        std::lock_guard<std::mutex> lk(m_slotNavQueriesMutex);
        for (auto& kv : m_slotNavQueries)
            dtFreeNavMeshQuery(kv.second);
        m_slotNavQueries.clear();
        m_slotRefCounts.clear();
    }
}

// ===========================================================================
// Configuration
// ===========================================================================

void OgreRecast::RecastCleanup()
{
    delete[] m_triareas; m_triareas = nullptr;
    rcFreeHeightField(m_solid);         m_solid = nullptr;
    rcFreeCompactHeightfield(m_chf);    m_chf = nullptr;
    rcFreeContourSet(m_cset);           m_cset = nullptr;
    rcFreePolyMesh(m_pmesh);            m_pmesh = nullptr;
    rcFreePolyMeshDetail(m_dmesh);      m_dmesh = nullptr;
    dtFreeNavMesh(m_navMesh);           m_navMesh = nullptr;
    dtFreeNavMeshQuery(m_navQuery);     m_navQuery = nullptr;
    delete m_ctx;                       m_ctx = nullptr;
}

void OgreRecast::configure(const OgreRecastConfigParams& params)
{
    m_configParams = params;

    delete m_ctx;
    m_ctx = new rcContext(true);

    m_cellSize = params.getCellSize();
    m_cellHeight = params.getCellHeight();
    m_agentMaxSlope = params.getAgentMaxSlope();
    m_agentHeight = params.getAgentHeight();
    m_agentMaxClimb = params.getAgentMaxClimb();
    m_agentRadius = params.getAgentRadius();
    m_edgeMaxLen = params.getEdgeMaxLen();
    m_edgeMaxError = params.getEdgeMaxError();
    m_regionMinSize = params.getRegionMinSize();
    m_regionMergeSize = params.getRegionMergeSize();
    m_vertsPerPoly = params.getVertsPerPoly();
    m_detailSampleDist = params.getDetailSampleDist();
    m_detailSampleMaxError = params.getDetailSampleMaxError();
    m_keepInterResults = params.getKeepInterResults();

    memset(&m_cfg, 0, sizeof(m_cfg));
    m_cfg.cs = m_cellSize;
    m_cfg.ch = m_cellHeight;
    m_cfg.walkableSlopeAngle = m_agentMaxSlope;
    m_cfg.walkableHeight = params._getWalkableheight();
    m_cfg.walkableClimb = params._getWalkableClimb();
    m_cfg.walkableRadius = params._getWalkableRadius();
    m_cfg.maxEdgeLen = params._getMaxEdgeLen();
    m_cfg.maxSimplificationError = m_edgeMaxError;
    m_cfg.minRegionArea = params._getMinRegionArea();
    m_cfg.mergeRegionArea = params._getMergeRegionArea();
    m_cfg.maxVertsPerPoly = m_vertsPerPoly;
    m_cfg.detailSampleDist = (float)params._getDetailSampleDist();
    m_cfg.detailSampleMaxError = (float)params._getDetailSampleMaxError();

    // Debug draw offsets derived from cell dimensions
    m_navMeshOffsetFromGround = m_cellHeight / 5.0f;
    m_navMeshEdgesOffsetFromGround = m_cellHeight / 3.0f;
    m_pathOffsetFromGround = m_agentHeight + m_navMeshOffsetFromGround;

    // Debug draw colours
    m_navmeshNeighbourEdgeCol = Ogre::ColourValue(0.9f, 0.9f, 0.9f);   // light grey
    m_navmeshOuterEdgeCol = Ogre::ColourValue(0.0f, 0.0f, 0.0f);   // black
    m_navmeshGroundPolygonCol = Ogre::ColourValue(0.0f, 0.7f, 0.0f);   // green
    m_navmeshOtherPolygonCol = Ogre::ColourValue(0.0f, 0.175f, 0.0f); // dark green
    m_pathCol = Ogre::ColourValue(1.0f, 0.0f, 0.0f);   // red
}

// ===========================================================================
// Navmesh build (single non-tiled mesh — used when no TileCache)
// ===========================================================================

bool OgreRecast::NavMeshBuild(const std::vector<Ogre::v1::Entity*>& srcMeshes,
    const std::vector<Ogre::Item*>& srcItems)
{
    if (srcMeshes.empty() && srcItems.empty())
    {
        m_pLog->logMessage("[OgreRecast] NavMeshBuild: no source geometry supplied.");
        return false;
    }
    return NavMeshBuild(new InputGeom(srcMeshes, srcItems));
}

bool OgreRecast::NavMeshBuild(InputGeom* input)
{
    m_pLog->logMessage("[OgreRecast] NavMeshBuild start");

    // ── Step 1: Rasterize ──────────────────────────────────────────────────
    m_geom = input;

    const float* bmin = m_geom->getMeshBoundsMin();
    const float* bmax = m_geom->getMeshBoundsMax();

    rcVcopy(m_cfg.bmin, bmin);
    rcVcopy(m_cfg.bmax, bmax);
    rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs,
        &m_cfg.width, &m_cfg.height);

    m_solid = rcAllocHeightfield();
    if (!m_solid)
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: out of memory 'solid'."); return false;
    }
    if (!rcCreateHeightfield(m_ctx, *m_solid,
        m_cfg.width, m_cfg.height,
        m_cfg.bmin, m_cfg.bmax,
        m_cfg.cs, m_cfg.ch))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcCreateHeightfield failed."); return false;
    }

    m_triareas = new unsigned char[m_geom->getTriCount()];
    memset(m_triareas, 0, m_geom->getTriCount() * sizeof(unsigned char));

    rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle,
        m_geom->getVerts(), m_geom->getVertCount(),
        m_geom->getTris(), m_geom->getTriCount(), m_triareas);
    rcRasterizeTriangles(m_ctx,
        m_geom->getVerts(), m_geom->getVertCount(),
        m_geom->getTris(), m_triareas, m_geom->getTriCount(),
        *m_solid, m_cfg.walkableClimb);

    if (!m_keepInterResults)
    {
        delete[] m_triareas; m_triareas = nullptr;
    }

    // ── Step 2: Filter ─────────────────────────────────────────────────────
    rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
    rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight,
        m_cfg.walkableClimb, *m_solid);
    rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);

    // ── Step 3: Compact heightfield ────────────────────────────────────────
    m_chf = rcAllocCompactHeightfield();
    if (!m_chf)
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: out of memory 'chf'."); return false;
    }
    if (!rcBuildCompactHeightfield(m_ctx,
        m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcBuildCompactHeightfield failed."); return false;
    }

    if (!m_keepInterResults)
    {
        rcFreeHeightField(m_solid); m_solid = nullptr;
    }

    if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcErodeWalkableArea failed."); return false;
    }

    // ── Step 4: Regions ────────────────────────────────────────────────────
    if (!rcBuildDistanceField(m_ctx, *m_chf))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcBuildDistanceField failed."); return false;
    }
    if (!rcBuildRegions(m_ctx, *m_chf, m_cfg.borderSize,
        m_cfg.minRegionArea, m_cfg.mergeRegionArea))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcBuildRegions failed."); return false;
    }

    // ── Step 5: Contours ───────────────────────────────────────────────────
    m_cset = rcAllocContourSet();
    if (!m_cset)
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: out of memory 'cset'."); return false;
    }
    if (!rcBuildContours(m_ctx, *m_chf,
        m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcBuildContours failed."); return false;
    }

    // ── Step 6: Poly mesh ──────────────────────────────────────────────────
    m_pmesh = rcAllocPolyMesh();
    if (!m_pmesh)
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: out of memory 'pmesh'."); return false;
    }
    if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcBuildPolyMesh failed."); return false;
    }

    // ── Step 7: Detail mesh ────────────────────────────────────────────────
    m_dmesh = rcAllocPolyMeshDetail();
    if (!m_dmesh)
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: out of memory 'dmesh'."); return false;
    }
    if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf,
        m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
    {
        m_pLog->logMessage("ERROR: NavMeshBuild: rcBuildPolyMeshDetail failed."); return false;
    }

    if (!m_keepInterResults)
    {
        rcFreeCompactHeightfield(m_chf); m_chf = nullptr;
        rcFreeContourSet(m_cset);        m_cset = nullptr;
    }

    // ── Step 8: Detour navmesh ─────────────────────────────────────────────
    if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
    {
        for (int i = 0; i < m_pmesh->npolys; ++i)
        {
            if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
            {
                m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
                m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
            }
        }

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
        params.offMeshConVerts = m_offMeshConVerts;
        params.offMeshConRad = m_offMeshConRads;
        params.offMeshConDir = m_offMeshConDirs;
        params.offMeshConAreas = m_offMeshConAreas;
        params.offMeshConFlags = m_offMeshConFlags;
        params.offMeshConUserID = m_offMeshConId;
        params.offMeshConCount = 0;
        params.walkableHeight = m_agentHeight;
        params.walkableRadius = m_agentRadius;
        params.walkableClimb = m_agentMaxClimb;
        rcVcopy(params.bmin, m_pmesh->bmin);
        rcVcopy(params.bmax, m_pmesh->bmax);
        params.cs = m_cfg.cs;
        params.ch = m_cfg.ch;

        unsigned char* navData = nullptr;
        int navDataSize = 0;
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            m_pLog->logMessage("ERROR: NavMeshBuild: dtCreateNavMeshData failed."); return false;
        }

        m_navMesh = dtAllocNavMesh();
        if (!m_navMesh)
        {
            dtFree(navData); m_pLog->logMessage("ERROR: NavMeshBuild: dtAllocNavMesh failed."); return false;
        }

        dtStatus status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
        if (dtStatusFailed(status))
        {
            dtFree(navData); m_pLog->logMessage("ERROR: NavMeshBuild: m_navMesh->init failed."); return false;
        }

        m_navQuery = dtAllocNavMeshQuery();
        status = m_navQuery->init(m_navMesh, 65535);
        if (dtStatusFailed(status))
        {
            m_pLog->logMessage("ERROR: NavMeshBuild: m_navQuery->init failed."); return false;
        }
    }

    m_pLog->logMessage("[OgreRecast] NavMeshBuild end");
    return true;
}

// ===========================================================================
// Navmesh visual accumulation (used by OgreDetourTileCache::drawDetail)
// ===========================================================================

void OgreRecast::beginNavMeshAccum()
{
    m_navVisual.beginAccum();
}

void OgreRecast::flushNavMesh()
{
    m_navVisual.flush(m_pSceneMgr, m_pRecastSN);
}

void OgreRecast::destroyNavMesh()
{
    m_navVisual.destroy(m_pSceneMgr);
}

void OgreRecast::recreateDrawer()
{
    // Destroy the static navmesh Ogre::Mesh items
    m_navVisual.destroy(m_pSceneMgr);
    // Destroy the path line ManualObject
    m_debugDrawer->mustRecreate();
}

// ---------------------------------------------------------------------------
// CreateRecastPolyMesh
// Accumulates one poly mesh tile into m_navVisual CPU buffers.
// OgreDetourTileCache calls this for each tile; drawNavMesh calls
// flushNavMesh() once at the end to upload everything to the GPU as a
// single Ogre::Mesh (2 draw calls: triangles + lines).
// ---------------------------------------------------------------------------
void OgreRecast::CreateRecastPolyMesh(
    const Ogre::String /*name*/,
    const unsigned short* verts, const int nverts,
    const unsigned short* polys, const int npolys,
    const unsigned char* areas, const int /*maxpolys*/,
    const unsigned short* regions, const int nvp,
    const float cs, const float ch, const float* orig,
    bool draw, bool colorRegions)
{
    if (false == draw || 0 == npolys)
        return;

    auto toVec = [&](int vi, float yOffset) -> Ogre::Vector3
    {
        const unsigned short* v = &verts[vi * 3];
        return Ogre::Vector3(
            orig[0] + v[0] * cs,
            orig[1] + v[1] * ch + yOffset,
            orig[2] + v[2] * cs);
    };

    // ── Walkable polygon fill ──────────────────────────────────────────────
    for (int i = 0; i < npolys; ++i)
    {
        if (areas[i] != SAMPLE_POLYAREA_GROUND
            && areas[i] != DT_TILECACHE_WALKABLE_AREA)
            continue;

        Ogre::ColourValue col;
        if (colorRegions)
        {
            const unsigned short reg = regions[i];
            if (reg != 0xFFFF) // RC_NULL_REGION
            {
                const uint32_t seed = static_cast<uint32_t>(reg) * 2654435761u;
                col = Ogre::ColourValue(
                    ((seed >> 16) & 0xFF) / 255.0f,
                    ((seed >> 8) & 0xFF) / 255.0f,
                    ((seed) & 0xFF) / 255.0f, 0.6f);
            }
            else
            {
                col = Ogre::ColourValue(0.4f, 0.4f, 0.4f, 0.5f); // RC_NULL_REGION: grey
            }
        }
        else
        {
            col = (areas[i] == SAMPLE_POLYAREA_GROUND)
                ? m_navmeshGroundPolygonCol
                : m_navmeshOtherPolygonCol;
        }

        const unsigned short* p = &polys[i * nvp * 2];
        for (int j = 2; j < nvp; ++j)
        {
            if (p[j] == RC_MESH_NULL_IDX)
                break;

            m_navVisual.addWalkPoly(
                toVec(p[0], m_navMeshOffsetFromGround),
                toVec(p[j - 1], m_navMeshOffsetFromGround),
                toVec(p[j], m_navMeshOffsetFromGround),
                col);
        }
    }

    // ── Neighbour edges (shared between adjacent polys) ───────────────────
    for (int i = 0; i < npolys; ++i)
    {
        const unsigned short* p = &polys[i * nvp * 2];
        for (int j = 0; j < nvp; ++j)
        {
            if (p[j] == RC_MESH_NULL_IDX) break;
            if (p[nvp + j] == RC_MESH_NULL_IDX) continue; // boundary, not neighbour

            const int jNext = (j + 1 < nvp && p[j + 1] != RC_MESH_NULL_IDX) ? j + 1 : 0;
            m_navVisual.addEdge(
                toVec(p[j], m_navMeshEdgesOffsetFromGround),
                toVec(p[jNext], m_navMeshEdgesOffsetFromGround),
                m_navmeshNeighbourEdgeCol);
        }
    }

    // ── Boundary edges (outer edges without neighbour) ────────────────────
    for (int i = 0; i < npolys; ++i)
    {
        const unsigned short* p = &polys[i * nvp * 2];
        for (int j = 0; j < nvp; ++j)
        {
            if (p[j] == RC_MESH_NULL_IDX) break;
            if (p[nvp + j] != RC_MESH_NULL_IDX) continue; // neighbour, not boundary

            const int jNext = (j + 1 < nvp && p[j + 1] != RC_MESH_NULL_IDX) ? j + 1 : 0;
            m_navVisual.addEdge(
                toVec(p[j], m_navMeshEdgesOffsetFromGround),
                toVec(p[jNext], m_navMeshEdgesOffsetFromGround),
                m_navmeshOuterEdgeCol);
        }
    }
}

// ===========================================================================
// Path line (ManualObject, a handful of vertices per path)
// ===========================================================================

void OgreRecast::CreateRecastPathLine(int nPathSlot, bool enable)
{
    // Remove any previously drawn line
    RemoveRecastPathLine();

    if (!enable)
        return;

    const int nVertCount = m_PathStore[nPathSlot].MaxVertex;
    if (nVertCount < 2)
        return;

    m_pRecastMOPath = m_pSceneMgr->createManualObject();
    m_pRecastMOPath->setRenderQueueGroup(10);
    m_pRecastMOPath->setName("recastdebug1");
    m_pRecastMOPath->estimateVertexCount(nVertCount);
    m_pRecastMOPath->estimateIndexCount(nVertCount);
    m_pRecastMOPath->begin("recastdebug1", Ogre::OT_LINE_STRIP);

    for (int nVert = 0; nVert < nVertCount; ++nVert)
    {
        m_pRecastMOPath->position(
            m_PathStore[nPathSlot].PosX[nVert],
            m_PathStore[nPathSlot].PosY[nVert] + m_pathOffsetFromGround,
            m_PathStore[nPathSlot].PosZ[nVert]);
        m_pRecastMOPath->colour(Ogre::ColourValue::White);
        m_pRecastMOPath->index(nVert);
    }

    m_pRecastMOPath->end();
    m_pRecastSN->attachObject(m_pRecastMOPath);
}

void OgreRecast::RemoveRecastPathLine()
{
    if (nullptr != m_pRecastMOPath)
    {
        try
        {
            m_pRecastSN->detachObject(m_pRecastMOPath);
            m_pSceneMgr->destroyManualObject(m_pRecastMOPath);
        }
        catch (...)
        {
        }
        m_pRecastMOPath = nullptr;
    }
}

// ===========================================================================
// Pathfinding
// ===========================================================================

int OgreRecast::FindPath(float* pStartPos, float* pEndPos,
    int nPathSlot, int nTarget)
{
    dtStatus status;
    dtPolyRef StartPoly, EndPoly;
    float StartNearest[3], EndNearest[3];
    dtPolyRef PolyPath[MAX_PATHPOLY];
    int nPathCount = 0;
    float StraightPath[MAX_PATHVERT * 3];
    int nVertCount = 0;

    status = m_navQuery->findNearestPoly(pStartPos, mExtents, mFilter, &StartPoly, StartNearest);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -1;

    status = m_navQuery->findNearestPoly(pEndPos, mExtents, mFilter, &EndPoly, EndNearest);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -2;

    status = m_navQuery->findPath(StartPoly, EndPoly, StartNearest, EndNearest,
        mFilter, PolyPath, &nPathCount, MAX_PATHPOLY);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -3;
    if (nPathCount == 0) return -4;

    status = m_navQuery->findStraightPath(StartNearest, EndNearest,
        PolyPath, nPathCount,
        StraightPath, nullptr, nullptr,
        &nVertCount, MAX_PATHVERT);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -5;
    if (nVertCount == 0) return -6;

    int nIndex = 0;
    for (int nVert = 0; nVert < nVertCount; ++nVert)
    {
        m_PathStore[nPathSlot].PosX[nVert] = StraightPath[nIndex++];
        m_PathStore[nPathSlot].PosY[nVert] = StraightPath[nIndex++];
        m_PathStore[nPathSlot].PosZ[nVert] = StraightPath[nIndex++];
    }
    m_PathStore[nPathSlot].MaxVertex = nVertCount;
    m_PathStore[nPathSlot].Target = nTarget;
    return nVertCount;
}

int OgreRecast::FindPath(Ogre::Vector3 startPos, Ogre::Vector3 endPos,
    int nPathSlot, int nTarget)
{
    float start[3], end[3];
    OgreVect3ToFloatA(startPos, start);
    OgreVect3ToFloatA(endPos, end);
    return FindPath(start, end, nPathSlot, nTarget);
}

int OgreRecast::FindPathWithQuery(dtNavMeshQuery* query,
    float* pStartPos, float* pEndPos,
    int nPathSlot, int nTarget)
{
    // Shared lock: multiple agents read m_navMesh concurrently.
    // OgreDetourTileCache::handleUpdate holds unique_lock during tile rebuilds.
    std::shared_lock<std::shared_mutex> readLock(m_navMeshMutex);

    dtPolyRef StartPoly, EndPoly;
    float StartNearest[3], EndNearest[3];
    dtPolyRef PolyPath[MAX_PATHPOLY];
    int nPathCount = 0;
    float StraightPath[MAX_PATHVERT * 3];
    int nVertCount = 0;

    dtStatus status;
    status = query->findNearestPoly(pStartPos, mExtents, mFilter, &StartPoly, StartNearest);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -1;
    status = query->findNearestPoly(pEndPos, mExtents, mFilter, &EndPoly, EndNearest);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -2;
    status = query->findPath(StartPoly, EndPoly, StartNearest, EndNearest,
        mFilter, PolyPath, &nPathCount, MAX_PATHPOLY);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -3;
    if (nPathCount == 0) return -4;
    status = query->findStraightPath(StartNearest, EndNearest,
        PolyPath, nPathCount,
        StraightPath, nullptr, nullptr,
        &nVertCount, MAX_PATHVERT);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK)) return -5;
    if (nVertCount == 0) return -6;

    int nIndex = 0;
    for (int nVert = 0; nVert < nVertCount; ++nVert)
    {
        m_PathStore[nPathSlot].PosX[nVert] = StraightPath[nIndex++];
        m_PathStore[nPathSlot].PosY[nVert] = StraightPath[nIndex++];
        m_PathStore[nPathSlot].PosZ[nVert] = StraightPath[nIndex++];
    }
    m_PathStore[nPathSlot].MaxVertex = nVertCount;
    m_PathStore[nPathSlot].Target = nTarget;
    return nVertCount;
}

std::vector<Ogre::Vector3> OgreRecast::getPath(int pathSlot)
{
    std::vector<Ogre::Vector3> result;
    if (pathSlot < 0 || pathSlot >= MAX_PATHSLOT
        || m_PathStore[pathSlot].MaxVertex <= 0)
        return result;

    result.reserve(m_PathStore[pathSlot].MaxVertex);
    for (int i = 0; i < m_PathStore[pathSlot].MaxVertex; ++i)
        result.emplace_back(m_PathStore[pathSlot].PosX[i],
        m_PathStore[pathSlot].PosY[i],
        m_PathStore[pathSlot].PosZ[i]);
    return result;
}

int OgreRecast::getTarget(int pathSlot)
{
    if (pathSlot < 0 || pathSlot >= MAX_PATHSLOT) return 0;
    return m_PathStore[pathSlot].Target;
}

// ===========================================================================
// Per-slot nav query management (concurrent pathfinding)
// ===========================================================================

void OgreRecast::createNavQueryForSlot(int pathSlot)
{
    std::lock_guard<std::mutex> lk(m_slotNavQueriesMutex);
    m_slotRefCounts[pathSlot]++;
    if (m_slotNavQueries.find(pathSlot) == m_slotNavQueries.end())
    {
        dtNavMeshQuery* q = dtAllocNavMeshQuery();
        q->init(m_navMesh, 2048);
        m_slotNavQueries[pathSlot] = q;
    }
}

void OgreRecast::destroyNavQueryForSlot(int pathSlot)
{
    std::lock_guard<std::mutex> lk(m_slotNavQueriesMutex);
    auto refIt = m_slotRefCounts.find(pathSlot);
    if (refIt == m_slotRefCounts.end()) return;
    if (--refIt->second > 0) return;
    m_slotRefCounts.erase(refIt);
    auto qIt = m_slotNavQueries.find(pathSlot);
    if (qIt != m_slotNavQueries.end())
    {
        dtFreeNavMeshQuery(qIt->second);
        m_slotNavQueries.erase(qIt);
    }
}

bool OgreRecast::hasNavQueryForSlot(int pathSlot) const
{
    std::lock_guard<std::mutex> lk(m_slotNavQueriesMutex);
    return m_slotNavQueries.count(pathSlot) > 0;
}

int OgreRecast::acquireNextFreeSlot()
{
    std::lock_guard<std::mutex> lk(m_slotNavQueriesMutex);
    int slot = 0;
    while (m_slotNavQueries.count(slot)) ++slot;
    dtNavMeshQuery* q = dtAllocNavMeshQuery();
    q->init(m_navMesh, 2048);
    m_slotNavQueries[slot] = q;
    m_slotRefCounts[slot] = 1;
    return slot;
}

dtNavMeshQuery* OgreRecast::getNavQueryForSlot(int pathSlot) const
{
    std::lock_guard<std::mutex> lk(m_slotNavQueriesMutex);
    auto it = m_slotNavQueries.find(pathSlot);
    return (it != m_slotNavQueries.end()) ? it->second : nullptr;
}

// ===========================================================================
// Miscellaneous
// ===========================================================================

OgreRecastNavmeshPruner* OgreRecast::getNavmeshPruner()
{
    if (!mNavmeshPruner)
        mNavmeshPruner = new OgreRecastNavmeshPruner(this, m_navMesh);
    return mNavmeshPruner;
}

void OgreRecast::update()
{
    // Legacy: static geometry rebuild. Not used in tiled navmesh builds.
    // Kept as a no-op to satisfy any existing callers.
}

bool OgreRecast::findNearestPointOnNavmesh(Ogre::Vector3 position,
    Ogre::Vector3& resultPt)
{
    dtPolyRef poly;
    return findNearestPolyOnNavmesh(position, resultPt, poly);
}

bool OgreRecast::findNearestPolyOnNavmesh(Ogre::Vector3 position,
    Ogre::Vector3& resultPt,
    dtPolyRef& resultPoly)
{
    float pt[3], rPt[3];
    OgreVect3ToFloatA(position, pt);
    dtStatus status = m_navQuery->findNearestPoly(pt, mExtents, mFilter,
        &resultPoly, rPt);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
        return false;
    if (rPt[0] == 107374176.0f || rPt[0] == -107374176.0f)
        return false;
    FloatAToOgreVect3(rPt, resultPt);
    return true;
}

Ogre::Vector3 OgreRecast::getRandomNavMeshPoint()
{
    float rPt[3];
    dtPolyRef poly;
    m_navQuery->findRandomPoint(mFilter, frand, &poly, rPt);
    return Ogre::Vector3(rPt[0], rPt[1], rPt[2]);
}

Ogre::Vector3 OgreRecast::getRandomNavMeshPointInCircle(Ogre::Vector3 center,
    Ogre::Real radius)
{
    float pt[3], rPt[3];
    OgreVect3ToFloatA(center, pt);
    dtPolyRef navPoly;
    dtStatus status = m_navQuery->findNearestPoly(pt, mExtents, mFilter,
        &navPoly, rPt);
    if ((status & DT_FAILURE) || (status & DT_STATUS_DETAIL_MASK))
        return center;
    dtPolyRef resultPoly;
    m_navQuery->findRandomPointAroundCircle(navPoly, pt, radius, mFilter,
        frand, &resultPoly, rPt);
    Ogre::Vector3 result;
    FloatAToOgreVect3(rPt, result);
    return result;
}

dtQueryFilter OgreRecast::getFilter()
{
    return *mFilter;
}

void OgreRecast::setFilter(const dtQueryFilter filter)
{
    *mFilter = filter;
}

Ogre::Vector3 OgreRecast::getPointExtents()
{
    Ogre::Vector3 v;
    FloatAToOgreVect3(mExtents, v);
    return v;
}

void OgreRecast::setPointExtents(Ogre::Vector3 extents)
{
    OgreVect3ToFloatA(extents, mExtents);
}

Ogre::String OgreRecast::getPathFindErrorMsg(int errorCode)
{
    switch (errorCode)
    {
    case  0: return "0 -- No error.";
    case -1: return "-1 -- Couldn't find polygon nearest to start point.";
    case -2: return "-2 -- Couldn't find polygon nearest to end point.";
    case -3: return "-3 -- Couldn't create a path.";
    case -4: return "-4 -- Couldn't find a path.";
    case -5: return "-5 -- Couldn't create a straight path.";
    case -6: return "-6 -- Couldn't find a straight path.";
    default: return Ogre::StringConverter::toString(errorCode) + " -- Unknown error.";
    }
}

void OgreRecast::OgreVect3ToFloatA(const Ogre::Vector3 vect, float* result)
{
    result[0] = vect.x;
    result[1] = vect.y;
    result[2] = vect.z;
}

void OgreRecast::FloatAToOgreVect3(const float* vect, Ogre::Vector3& result)
{
    result.x = vect[0];
    result.y = vect[1];
    result.z = vect[2];
}