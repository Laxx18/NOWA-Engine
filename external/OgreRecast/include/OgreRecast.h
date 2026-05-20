/*
    OgreCrowd — OgreRecast.h

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

#ifndef __OgreRecast_h_
#define __OgreRecast_h_

#include "OgreRecastDefinitions.h"
#include <Ogre.h>
#include "RecastInputGeom.h"
#include "OgreRecastDebugDraw.h"
#include "OgreRecastNavmeshVisual.h"

#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <map>

class __declspec(dllexport) OgreRecastNavmeshPruner;

// ---------------------------------------------------------------------------
// OgreRecastConfigParams
// ---------------------------------------------------------------------------
class __declspec(dllexport) OgreRecastConfigParams
{
public:
    OgreRecastConfigParams()
        : cellSize(0.3f), cellHeight(0.2f), agentMaxSlope(20.0f),
        agentHeight(2.5f), agentMaxClimb(1.0f), agentRadius(0.5f),
        edgeMaxLen(12.0f), edgeMaxError(1.3f), regionMinSize(50.0f),
        regionMergeSize(20.0f), vertsPerPoly(DT_VERTS_PER_POLYGON),
        detailSampleDist(6.0f), detailSampleMaxError(1.0f),
        keepInterResults(false)
    {
        eval();
    }

    // Setters
    void setCellSize(Ogre::Real v)
    {
        cellSize = v;            eval();
    }
    void setCellHeight(Ogre::Real v)
    {
        cellHeight = v;          eval();
    }
    void setAgentHeight(Ogre::Real v)
    {
        agentHeight = v;         eval();
    }
    void setAgentRadius(Ogre::Real v)
    {
        agentRadius = v;         eval();
    }
    void setAgentMaxClimb(Ogre::Real v)
    {
        agentMaxClimb = v;       eval();
    }
    void setAgentMaxSlope(Ogre::Real v)
    {
        agentMaxSlope = v;
    }
    void setRegionMinSize(Ogre::Real v)
    {
        regionMinSize = v;       eval();
    }
    void setRegionMergeSize(Ogre::Real v)
    {
        regionMergeSize = v;     eval();
    }
    void setEdgeMaxLen(Ogre::Real v)
    {
        edgeMaxLen = v;          eval();
    }
    void setEdgeMaxError(Ogre::Real v)
    {
        edgeMaxError = v;
    }
    void setVertsPerPoly(int v)
    {
        vertsPerPoly = v;
    }
    void setDetailSampleDist(Ogre::Real v)
    {
        detailSampleDist = v;    eval();
    }
    void setDetailSampleMaxError(Ogre::Real v)
    {
        detailSampleMaxError = v; eval();
    }
    void setKeepInterResults(bool v)
    {
        keepInterResults = v;
    }

    // Override derived params directly
    void _setWalkableHeight(int v)
    {
        _walkableHeight = v;
    }
    void _setWalkableClimb(int v)
    {
        _walkableClimb = v;
    }
    void _setWalkableRadius(int v)
    {
        _walkableRadius = v;
    }
    void _setMaxEdgeLen(int v)
    {
        _maxEdgeLen = v;
    }
    void _setMinRegionArea(int v)
    {
        _minRegionArea = v;
    }
    void _setMergeRegionArea(int v)
    {
        _mergeRegionArea = v;
    }
    void _setDetailSampleDist(Ogre::Real v)
    {
        _detailSampleDist = v;
    }
    void _setDetailSampleMaxError(Ogre::Real v)
    {
        _detailSampleMaxError = v;
    }

    // Getters
    Ogre::Real getCellSize()            const
    {
        return cellSize;
    }
    Ogre::Real getCellHeight()          const
    {
        return cellHeight;
    }
    Ogre::Real getAgentMaxSlope()       const
    {
        return agentMaxSlope;
    }
    Ogre::Real getAgentHeight()         const
    {
        return agentHeight;
    }
    Ogre::Real getAgentMaxClimb()       const
    {
        return agentMaxClimb;
    }
    Ogre::Real getAgentRadius()         const
    {
        return agentRadius;
    }
    Ogre::Real getEdgeMaxLen()          const
    {
        return edgeMaxLen;
    }
    Ogre::Real getEdgeMaxError()        const
    {
        return edgeMaxError;
    }
    Ogre::Real getRegionMinSize()       const
    {
        return regionMinSize;
    }
    Ogre::Real getRegionMergeSize()     const
    {
        return regionMergeSize;
    }
    int        getVertsPerPoly()        const
    {
        return vertsPerPoly;
    }
    Ogre::Real getDetailSampleDist()    const
    {
        return detailSampleDist;
    }
    Ogre::Real getDetailSampleMaxError()const
    {
        return detailSampleMaxError;
    }
    bool       getKeepInterResults()    const
    {
        return keepInterResults;
    }

    int  _getWalkableheight()       const
    {
        return _walkableHeight;
    }
    int  _getWalkableClimb()        const
    {
        return _walkableClimb;
    }
    int  _getWalkableRadius()       const
    {
        return _walkableRadius;
    }
    int  _getMaxEdgeLen()           const
    {
        return _maxEdgeLen;
    }
    int  _getMinRegionArea()        const
    {
        return _minRegionArea;
    }
    int  _getMergeRegionArea()      const
    {
        return _mergeRegionArea;
    }
    int  _getDetailSampleDist()     const
    {
        return (int)_detailSampleDist;
    }
    int  _getDetailSampleMaxError() const
    {
        return (int)_detailSampleMaxError;
    }

private:
    void eval()
    {
        _walkableHeight = (int)ceilf(agentHeight / cellHeight);
        _walkableClimb = (int)floorf(agentMaxClimb / cellHeight);
        _walkableRadius = (int)ceilf(agentRadius / cellSize);
        _maxEdgeLen = (int)(edgeMaxLen / cellSize);
        _minRegionArea = (int)rcSqr(regionMinSize);
        _mergeRegionArea = (int)rcSqr(regionMergeSize);
        _detailSampleDist = detailSampleDist < 0.9f ? 0.0f : cellSize * detailSampleDist;
        _detailSampleMaxError = cellHeight * detailSampleMaxError;
    }

    Ogre::Real cellSize, cellHeight, agentMaxSlope, agentHeight;
    Ogre::Real agentMaxClimb, agentRadius, edgeMaxLen, edgeMaxError;
    Ogre::Real regionMinSize, regionMergeSize, detailSampleDist, detailSampleMaxError;
    int        vertsPerPoly;
    bool       keepInterResults;

    int        _walkableHeight, _walkableClimb, _walkableRadius, _maxEdgeLen;
    int        _minRegionArea, _mergeRegionArea;
    Ogre::Real _detailSampleDist, _detailSampleMaxError;
};


// ---------------------------------------------------------------------------
// OgreRecast
// ---------------------------------------------------------------------------
class __declspec(dllexport) OgreRecast
{
public:
    OgreRecast(Ogre::SceneManager* sceneMgr,
        const OgreRecastConfigParams& configParams = OgreRecastConfigParams());
    ~OgreRecast();

    // ----- Configuration ---------------------------------------------------
    void configure(const OgreRecastConfigParams& params);

    float getAgentRadius()              const
    {
        return m_agentRadius;
    }
    float getAgentHeight()              const
    {
        return m_agentHeight;
    }
    float getPathOffsetFromGround()     const
    {
        return m_pathOffsetFromGround;
    }
    float getNavmeshOffsetFromGround()  const
    {
        return m_navMeshOffsetFromGround;
    }
    float getCellSize()                 const
    {
        return m_cellSize;
    }
    float getCellHeight()               const
    {
        return m_cellHeight;
    }

    rcConfig            getConfig()       const
    {
        return m_cfg;
    }
    OgreRecastConfigParams getConfigParams() const
    {
        return m_configParams;
    }
    Ogre::SceneManager* getSceneManager() const
    {
        return m_pSceneMgr;
    }

    // ----- Navmesh build ---------------------------------------------------
    bool NavMeshBuild(const std::vector<Ogre::v1::Entity*>& srcMeshes,
        const std::vector<Ogre::Item*>& srcItems);
    bool NavMeshBuild(InputGeom* input);
    void RecastCleanup();

    // ----- Pathfinding -----------------------------------------------------
    int  FindPath(float* pStartPos, float* pEndPos, int nPathSlot, int nTarget);
    int  FindPath(Ogre::Vector3 startPos, Ogre::Vector3 endPos,
        int nPathSlot, int nTarget);
    int  FindPathWithQuery(dtNavMeshQuery* query,
        float* pStartPos, float* pEndPos,
        int nPathSlot, int nTarget);

    std::vector<Ogre::Vector3> getPath(int pathSlot);
    int  getTarget(int pathSlot);
    Ogre::String getPathFindErrorMsg(int errorCode);

    bool findNearestPointOnNavmesh(Ogre::Vector3 position, Ogre::Vector3& resultPt);
    bool findNearestPolyOnNavmesh(Ogre::Vector3 position, Ogre::Vector3& resultPt,
        dtPolyRef& resultPoly);
    Ogre::Vector3 getRandomNavMeshPoint();
    Ogre::Vector3 getRandomNavMeshPointInCircle(Ogre::Vector3 center, Ogre::Real radius);

    dtQueryFilter getFilter();
    void          setFilter(const dtQueryFilter filter);
    Ogre::Vector3 getPointExtents();
    void          setPointExtents(Ogre::Vector3 extents);

    // Per-slot nav queries for concurrent threaded pathfinding
    void createNavQueryForSlot(int pathSlot);
    void destroyNavQueryForSlot(int pathSlot);
    bool hasNavQueryForSlot(int pathSlot) const;
    int  acquireNextFreeSlot();
    dtNavMeshQuery* getNavQueryForSlot(int pathSlot) const;

    // ----- Debug draw — navmesh (via OgreRecastNavmeshVisual) --------------
    // Called by OgreDetourTileCache::drawNavMesh() to accumulate tile geometry
    // into CPU buffers and then flush a single Ogre::Mesh to the GPU.
    void beginNavMeshAccum();
    void flushNavMesh();
    void destroyNavMesh();

    // Accumulate one poly mesh tile into the CPU buffers.
    // Called for every tile inside OgreDetourTileCache::drawDetail().
    void CreateRecastPolyMesh(const Ogre::String name,
        const unsigned short* verts, int nverts,
        const unsigned short* polys, int npolys,
        const unsigned char* areas, int maxpolys,
        const unsigned short* regions, int nvp,
        float cs, float ch, const float* orig,
        bool draw, bool colorRegions = true);

    // Destroy navmesh visual + path line ManualObject.
    // Called before a full redraw to avoid stale geometry accumulation.
    void recreateDrawer();

    // ----- Debug draw — path line (via ManualObject) -----------------------
    void CreateRecastPathLine(int nPathSlot, bool enable);
    void RemoveRecastPathLine();

    // ----- Misc ------------------------------------------------------------
    void update(); // legacy static geometry update — mostly unused

    OgreRecastNavmeshPruner* getNavmeshPruner();

    static void OgreVect3ToFloatA(const Ogre::Vector3 vect, float* result);
    static void FloatAToOgreVect3(const float* vect, Ogre::Vector3& result);

protected:

    Ogre::SceneManager* m_pSceneMgr;
    OgreRecastNavmeshVisual m_navVisual;   // static Ogre::Mesh navmesh visual
    OgreRecastDebugDraw* m_debugDrawer; // ManualObject path line only

    Ogre::ManualObject* m_pRecastMOPath;  // path line ManualObject
    Ogre::SceneNode* m_pRecastSN;      // root scene node for all recast visuals

    // Recast intermediate data
    unsigned char* m_triareas;
    rcHeightfield* m_solid;
    rcCompactHeightfield* m_chf;
    rcContourSet* m_cset;
    rcPolyMesh* m_pmesh;
    rcConfig              m_cfg;
    rcPolyMeshDetail* m_dmesh;
    rcContext* m_ctx;

    // Detour navmesh
    InputGeom* m_geom;
    dtNavMesh* m_navMesh;
    dtNavMeshQuery* m_navQuery;

    // Configuration values (mirrors m_configParams for fast access)
    float m_cellSize, m_cellHeight;
    float m_agentHeight, m_agentRadius, m_agentMaxClimb, m_agentMaxSlope;
    float m_regionMinSize, m_regionMergeSize;
    float m_edgeMaxLen, m_edgeMaxError;
    int   m_vertsPerPoly;
    float m_detailSampleDist, m_detailSampleMaxError;
    bool  m_keepInterResults;

    // Debug draw offsets and colours
    float m_pathOffsetFromGround;
    float m_navMeshOffsetFromGround;
    float m_navMeshEdgesOffsetFromGround;
    Ogre::ColourValue m_navmeshNeighbourEdgeCol;
    Ogre::ColourValue m_navmeshOuterEdgeCol;
    Ogre::ColourValue m_navmeshGroundPolygonCol;
    Ogre::ColourValue m_navmeshOtherPolygonCol;
    Ogre::ColourValue m_pathCol;

    // Path storage
    PATHDATA m_PathStore[MAX_PATHSLOT];

    // Query filter and search extents
    dtQueryFilter* mFilter;
    float          mExtents[3];

    // Off-mesh connections (unused, kept for future use)
    static const int MAX_OFFMESH_CONNECTIONS = 256;
    float         m_offMeshConVerts[MAX_OFFMESH_CONNECTIONS * 3 * 2];
    float         m_offMeshConRads[MAX_OFFMESH_CONNECTIONS];
    unsigned char m_offMeshConDirs[MAX_OFFMESH_CONNECTIONS];
    unsigned char m_offMeshConAreas[MAX_OFFMESH_CONNECTIONS];
    unsigned short m_offMeshConFlags[MAX_OFFMESH_CONNECTIONS];
    unsigned int  m_offMeshConId[MAX_OFFMESH_CONNECTIONS];
    int           m_offMeshConCount;

    bool m_rebuildSg;

    OgreRecastNavmeshPruner* mNavmeshPruner;
    Ogre::LogManager* m_pLog;
    OgreRecastConfigParams   m_configParams;

    // Per-slot nav queries for concurrent pathfinding
    std::unordered_map<int, dtNavMeshQuery*> m_slotNavQueries;
    std::map<int, int>                       m_slotRefCounts;
    mutable std::mutex                       m_slotNavQueriesMutex;

    // Protects m_navMesh against concurrent read (FindPathWithQuery)
    // and write (OgreDetourTileCache::handleUpdate -> buildNavMeshTile).
    mutable std::shared_mutex m_navMeshMutex;

private:
    friend class OgreDetourTileCache;
    friend class OgreDetourCrowd;
};

#endif // __OgreRecast_h_