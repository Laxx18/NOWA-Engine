#include "NOWAPrecompiled.h"

#include "Ocean.h"
#include "OgreHlmsOcean.h"
#include "OgreCamera.h"
#include "OgreSceneManager.h"
#include "OgreRoot.h"
#include "OgreTimer.h"
#include "OgreHlmsOceanDatablock.h"
#include "Compositor/OgreCompositorManager2.h"

namespace Ogre
{
    const String OceanFactory::FACTORY_TYPE_NAME = "Ocean";

    const String& OceanFactory::getType(void) const
    {
        return OceanFactory::FACTORY_TYPE_NAME;
    }

    MovableObject* OceanFactory::createInstanceImpl(IdType id, ObjectMemoryManager* objectMemoryManager, SceneManager* manager, const NameValuePairList* params)
    {
        CompositorManager2* compositorManager = Ogre::Root::getSingletonPtr()->getCompositorManager2();
        const Camera* camera = 0;

        if (nullptr != params)
        {
            NameValuePairList::const_iterator itCamera = params->find("camera");
            if (itCamera != params->end())
            {
                camera = manager->findCameraNoThrow(itCamera->second);
            }
        }

        return OGRE_NEW Ocean(id, objectMemoryManager, manager, 0, compositorManager, camera);
    }

    void OceanFactory::destroyInstance(MovableObject* obj)
    {
        OGRE_DELETE obj;
    }

    Ocean::Ocean(IdType id, ObjectMemoryManager* objectMemoryManager, SceneManager* sceneManager, uint8 renderQueueId, CompositorManager2* compositorManager, const Camera* camera)
        : MovableObject(id, objectMemoryManager, sceneManager, renderQueueId),
        m_hlmsOceanIndex(std::numeric_limits<uint32>::max()),
        m_width(0u),
        m_depth(0u),
        m_depthWidthRatio(1.0f),
        m_invWidth(1.0f),
        m_invDepth(1.0f),
        m_uvScale(1.0f),
        m_xzDimensions(Vector2::UNIT_SCALE),
        m_xzInvDimensions(Vector2::UNIT_SCALE),
        m_xzRelativeSize(Vector2::UNIT_SCALE),
        m_height(1.0f),
        m_WavesIntensity(0.5f),
        m_OceanOrigin(Vector3::ZERO),
        m_basePixelDimension(256u),
        m_useSkirts(true),
        m_oceanTime(0.0f),
        m_waveTimeScale(1.0f),
        m_waveFrequencyScale(1.0f),
        m_waveChaos(0.0f),
        m_isUnderwater(false),
        m_currentCell(0u),
        m_prevLightDir(Vector3::ZERO),
        m_compositorManager(compositorManager),
        m_camera(camera)
    {
    }
    //-----------------------------------------------------------------------------------
    Ocean::~Ocean()
    {
        if (!m_OceanCells[0].empty() && m_OceanCells[0].back().getDatablock())
        {
            HlmsDatablock* datablock = m_OceanCells[0].back().getDatablock();
            OGRE_ASSERT_HIGH(dynamic_cast<HlmsOcean*>(datablock->getCreator()));
            HlmsOcean* hlms = static_cast<HlmsOcean*>(datablock->getCreator());
            hlms->_unlinkOcean(this);
        }

        for (size_t i = 0u; i < 2u; ++i)
            m_OceanCells[i].clear();
    }
    //-----------------------------------------------------------------------------------
    inline OceanGridPoint Ocean::worldToGrid(const Vector3& vPos) const
    {
        OceanGridPoint retVal;
        const float fWidth = static_cast<float>(m_width);
        const float fDepth = static_cast<float>(m_depth);

        const float fX = floorf(((vPos.x - m_OceanOrigin.x) * m_xzInvDimensions.x) * fWidth);
        const float fZ = floorf(((vPos.z - m_OceanOrigin.z) * m_xzInvDimensions.y) * fDepth);
        retVal.x = fX >= 0.0f ? static_cast<int32>(fX) : -1;
        retVal.z = fZ >= 0.0f ? static_cast<int32>(fZ) : -1;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    inline Vector2 Ocean::gridToWorld(const OceanGridPoint& gPos) const
    {
        Vector2 retVal;
        const float fWidth = static_cast<float>(m_width);
        const float fDepth = static_cast<float>(m_depth);

        retVal.x = (float(gPos.x) / fWidth) * m_xzDimensions.x + m_OceanOrigin.x;
        retVal.y = (float(gPos.z) / fDepth) * m_xzDimensions.y + m_OceanOrigin.z;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool Ocean::isVisible(const OceanGridPoint& gPos, const OceanGridPoint& gSize) const
    {
        if (gPos.x >= static_cast<int32>(m_width) ||
            gPos.z >= static_cast<int32>(m_depth) ||
            gPos.x + gSize.x <= 0 ||
            gPos.z + gSize.z <= 0)
        {
            //Outside Ocean bounds.
            return false;
        }

        //        return true;

        const Vector2 cellPos = gridToWorld(gPos);
        const Vector2 cellSize(Real(gSize.x + 1) * m_xzRelativeSize.x,
            Real(gSize.z + 1) * m_xzRelativeSize.y);

        const Vector3 vHalfSize = Vector3(cellSize.x, m_height, cellSize.y) * 0.5f;
        const Vector3 vCenter = Vector3(cellPos.x, m_OceanOrigin.y, cellPos.y) + vHalfSize;

        for (unsigned i = 0; i < 6u; ++i)
        {
            //Skip far plane if view frustum is infinite
            if (i == FRUSTUM_PLANE_FAR && m_camera->getFarClipDistance() == 0)
                continue;

            Plane::Side side = m_camera->getFrustumPlane((uint16)i).getSide(vCenter, vHalfSize);

            //We only need one negative match to know the obj is outside the frustum
            if (side == Plane::NEGATIVE_SIDE)
                return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    void Ocean::addRenderable(const OceanGridPoint& gridPos, const OceanGridPoint& cellSize, uint32 lodLevel)
    {
        OceanCell* cell = &m_OceanCells[0][m_currentCell++];
        cell->setOrigin(gridPos, static_cast<uint32>(cellSize.x), static_cast<uint32>(cellSize.z), lodLevel);
        m_collectedCells[0].push_back(cell);
    }
    //-----------------------------------------------------------------------------------
    void Ocean::optimizeCellsAndAdd(void)
    {
        //Keep iterating until m_collectedCells[0] stops shrinking
        size_t numCollectedCells = std::numeric_limits<size_t>::max();
        while (numCollectedCells != m_collectedCells[0].size())
        {
            numCollectedCells = m_collectedCells[0].size();

            if (m_collectedCells[0].size() > 1)
            {
                m_collectedCells[1].clear();

                std::vector<OceanCell*>::const_iterator itor = m_collectedCells[0].begin();
                std::vector<OceanCell*>::const_iterator end = m_collectedCells[0].end();

                while (end - itor >= 2u)
                {
                    OceanCell* currCell = *itor;
                    OceanCell* nextCell = *(itor + 1);

                    m_collectedCells[1].push_back(currCell);
                    if (currCell->merge(nextCell))
                        itor += 2;
                    else
                        ++itor;
                }

                while (itor != end)
                    m_collectedCells[1].push_back(*itor++);

                m_collectedCells[1].swap(m_collectedCells[0]);
            }
        }

        std::vector<OceanCell*>::const_iterator itor = m_collectedCells[0].begin();
        std::vector<OceanCell*>::const_iterator end = m_collectedCells[0].end();
        while (itor != end)
            mRenderables.push_back(*itor++);

        m_collectedCells[0].clear();
    }

    void Ocean::updateLocalBounds(void)
    {
        const float dimX = std::max(m_xzDimensions.x, 0.001f);
        const float dimZ = std::max(m_xzDimensions.y, 0.001f);
        const float halfX = dimX * 0.5f;
        const float halfZ = dimZ * 0.5f;

        // cover wave peaks (pick a conservative multiplier)
        const float halfY = std::max(1.0f, m_height * 2.0f);

        const Vector3 center(m_OceanOrigin.x + halfX, m_OceanOrigin.y, m_OceanOrigin.z + halfZ);
        const Vector3 halfSize(halfX, halfY, halfZ);

        Aabb aabb;
        aabb.mCenter = center;
        aabb.mHalfSize = halfSize;

        setLocalAabb(aabb);
    }
    //-----------------------------------------------------------------------------------
    void Ocean::setBasePixelDimension(uint32 basePixelDimension)
    {
        m_basePixelDimension = basePixelDimension;

        // If ocean is already created, recreate cells with new dimension
        if (!m_OceanCells[0].empty())
        {
            // Save the current datablock
            HlmsDatablock* datablock = m_OceanCells[0].back().getDatablock();

            // Recreate the cell structure
            createOceanCells();

            // Reapply datablock if we had one
            if (datablock)
            {
                for (size_t i = 0u; i < 2u; ++i)
                {
                    std::vector<OceanCell>::iterator itor = m_OceanCells[i].begin();
                    std::vector<OceanCell>::iterator end = m_OceanCells[i].end();

                    while (itor != end)
                    {
                        itor->setDatablock(datablock);
                        ++itor;
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void Ocean::createOceanCells()
    {
        // Calculate how many cells we need
        const uint32 basePixelDimension = m_basePixelDimension;
        const uint32 vertPixelDimension = static_cast<uint32>(m_basePixelDimension * m_depthWidthRatio);
        const uint32 maxPixelDimension = std::max(basePixelDimension, vertPixelDimension);
        const uint32 maxRes = std::max(m_width, m_depth);

        uint32 numCells = 16u; // 4x4
        uint32 accumDim = 0u;
        uint32 iteration = 1u;

        while (accumDim < maxRes)
        {
            numCells += 12u; // 4x4 - 2x2
            accumDim += maxPixelDimension * (1u << iteration);
            ++iteration;
        }

        numCells += 12u;
        accumDim += maxPixelDimension * (1u << iteration);
        ++iteration;

        // Resize cell arrays
        for (size_t i = 0u; i < 2u; ++i)
        {
            m_OceanCells[i].clear();
            m_OceanCells[i].resize(numCells, OceanCell(this));
        }

        // Initialize all cells
        VaoManager* vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        for (size_t i = 0u; i < 2u; ++i)
        {
            std::vector<OceanCell>::iterator itor = m_OceanCells[i].begin();
            std::vector<OceanCell>::iterator end = m_OceanCells[i].end();
            const std::vector<OceanCell>::iterator begin = itor;

            while (itor != end)
            {
                // Cells >= 20 use skirts (16 base cells + 4 for oceanTime)
                itor->initialize(vaoManager, m_useSkirts && (itor - begin) >= 20u);
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void Ocean::sortCellsByDepth(const Vector3& cameraPos)
    {
        // Only sort if we have transparency enabled
        if (m_OceanCells[0].empty())
            return;

        HlmsDatablock* datablock = m_OceanCells[0].back().getDatablock();
        if (!datablock)
            return;

        HlmsOceanDatablock* oceanDB = static_cast<HlmsOceanDatablock*>(datablock);
        if (oceanDB->getTransparency() >= 1.0f)
            return; // Fully opaque, no sorting needed

        // Sort renderables back-to-front for proper alpha blending
        std::sort(mRenderables.begin(), mRenderables.end(),
            [&cameraPos](const Renderable* a, const Renderable* b) -> bool
            {
                const OceanCell* cellA = static_cast<const OceanCell*>(a);
                const OceanCell* cellB = static_cast<const OceanCell*>(b);

                // Get cell center positions
                Vector2 posA = cellA->getParentOcean()->gridToWorld(
                    OceanGridPoint{ cellA->getGridX(), cellA->getGridZ() });
                Vector2 posB = cellB->getParentOcean()->gridToWorld(
                    OceanGridPoint{ cellB->getGridX(), cellB->getGridZ() });

                Vector3 centerA(posA.x, cellA->getParentOcean()->getOceanOrigin().y, posA.y);
                Vector3 centerB(posB.x, cellB->getParentOcean()->getOceanOrigin().y, posB.y);

                float distA = cameraPos.squaredDistance(centerA);
                float distB = cameraPos.squaredDistance(centerB);

                // Sort back-to-front (farthest first)
                return distA > distB;
            });
    }
    //-----------------------------------------------------------------------------------
    void Ocean::update(Ogre::Real dt)
    {
        // This is the ONLY time value that goes into the shader
        m_oceanTime += dt * m_waveTimeScale;

        if (m_oceanTime > 100000.0f)
            m_oceanTime -= 100000.0f;

        m_WavesIntensity = std::max(m_WavesIntensity, 0.001f);
        m_height = m_WavesIntensity * 0.9 + 0.1;
        
        mRenderables.clear();
        m_currentCell = 0;

        Vector3 camPos = m_camera->getDerivedPosition();

        // Determine whether camera is underwater. We use a small hysteresis to avoid flickering at the waterline.
        // Surface level is the ocean base Y (same as cellData.pos.y).
        const float surfaceY = m_OceanOrigin.y;
        const float hysteresis = 0.05f;
        const float enterUnder = surfaceY - hysteresis;
        const float leaveUnder = surfaceY + hysteresis;
        if( !m_isUnderwater )
            m_isUnderwater = camPos.y < enterUnder;
        else
            m_isUnderwater = camPos.y < leaveUnder;

        const int32 basePixelDimension = static_cast<int32>(m_basePixelDimension);
        const int32 vertPixelDimension = static_cast<int32>(float(m_basePixelDimension) * m_depthWidthRatio);

        OceanGridPoint cellSize;
        cellSize.x = basePixelDimension;
        cellSize.z = vertPixelDimension;

        //Quantize the camera position to basePixelDimension steps
        OceanGridPoint camCenter = worldToGrid(camPos);
        camCenter.x = (camCenter.x / basePixelDimension) * basePixelDimension;
        camCenter.z = (camCenter.z / vertPixelDimension) * vertPixelDimension;

        uint32 currentLod = 0;

        //        camCenter.x = 64;
        //        camCenter.z = 64;

                //LOD 0: Add full 4x4 grid
        for (int32 z = -2; z < 2; ++z)
        {
            for (int32 x = -2; x < 2; ++x)
            {
                OceanGridPoint pos = camCenter;
                pos.x += x * cellSize.x;
                pos.z += z * cellSize.z;

                if (isVisible(pos, cellSize))
                    addRenderable(pos, cellSize, currentLod);
            }
        }

        optimizeCellsAndAdd();

        m_currentCell = 20u; //The first 16 + 4 (oceanTime) cells don't use skirts.

        const uint32 maxRes = std::max(m_width, m_depth);
        //TODO: When we're too far (outside the Ocean), just display a 4x4 grid or something like that.

        size_t numObjectsAdded = std::numeric_limits<size_t>::max();
        //LOD n: Add 4x4 grid, ignore 2x2 center (which
        //is the same as saying the borders of the grid)
        while (numObjectsAdded != m_currentCell ||
            (mRenderables.empty() && (1u << currentLod) <= maxRes))
        {
            numObjectsAdded = m_currentCell;

            cellSize.x <<= 1u;
            cellSize.z <<= 1u;
            ++currentLod;

            //Row 0
            {
                const int32 z = 1;
                for (int32 x = -2; x < 2; ++x)
                {
                    OceanGridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }
            //Row 3
            {
                const int32 z = -2;
                for (int32 x = -2; x < 2; ++x)
                {
                    OceanGridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }
            //Cells [0, 1] & [0, 2];
            {
                const int32 x = -2;
                for (int32 z = -1; z < 1; ++z)
                {
                    OceanGridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }
            //Cells [3, 1] & [3, 2];
            {
                const int32 x = 1;
                for (int32 z = -1; z < 1; ++z)
                {
                    OceanGridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }

            optimizeCellsAndAdd();
        }

        updateLocalBounds();

        // Sort cells for proper transparency rendering
        if (m_camera)
        {
            sortCellsByDepth(m_camera->getDerivedPosition());
        }
    }

    //-----------------------------------------------------------------------------------
    void Ocean::create(const Vector3 center, const Vector2& dimensions)
    {
        m_OceanOrigin = center - Ogre::Vector3(dimensions.x, 0, dimensions.y) * 0.5f;
        m_xzDimensions = dimensions;
        m_xzInvDimensions = 1.0f / m_xzDimensions;
        m_basePixelDimension = 64u; // Default value

        m_width = 4096;
        m_depth = 4096;
        m_depthWidthRatio = float(m_depth) / float(m_width);
        m_invWidth = 1.0f / float(m_width);
        m_invDepth = 1.0f / float(m_depth);

        m_xzRelativeSize = m_xzDimensions / Vector2(static_cast<Real>(m_width),
            static_cast<Real>(m_depth));

        // Use the new createOceanCells() method instead of inline code
        createOceanCells();

        updateLocalBounds();
    }
    //-----------------------------------------------------------------------------------
    void Ocean::setWavesIntensity(float intensity)
    {
        m_WavesIntensity = intensity;
    }
    //-----------------------------------------------------------------------------------
    void Ocean::setWavesScale(float scale)
    {
        scale = std::max(scale, 0.0001f);
        m_uvScale = 1.0f / scale;
    }
	//-----------------------------------------------------------------------------------
    bool Ocean::isUnderwater(const Vector3& worldPos) const
    {
        return worldPos.y < m_OceanOrigin.y;
    }
	//-----------------------------------------------------------------------------------
    bool Ocean::isUnderwater(const SceneNode* sceneNode) const
    {
        if (!sceneNode)
        {
            return false;
        }

        return isUnderwater(sceneNode->_getDerivedPosition());
    }
    //-----------------------------------------------------------------------------------
    void Ocean::setDatablock(HlmsDatablock* datablock)
    {
        if (!datablock && !m_OceanCells[0].empty() && m_OceanCells[0].back().getDatablock())
        {
            // Unsetting the datablock. We have no way of unlinking later on. Do it now
            HlmsDatablock* oldDatablock = m_OceanCells[0].back().getDatablock();
            OGRE_ASSERT_HIGH(dynamic_cast<HlmsOcean*>(oldDatablock->getCreator()));
            HlmsOcean* hlms = static_cast<HlmsOcean*>(oldDatablock->getCreator());
            hlms->_unlinkOcean(this);
        }

        for (size_t i = 0u; i < 2u; ++i)
        {
            std::vector<OceanCell>::iterator itor = m_OceanCells[i].begin();
            std::vector<OceanCell>::iterator end = m_OceanCells[i].end();

            while (itor != end)
            {
                itor->setDatablock(datablock);
                ++itor;
            }
        }

        // Add linking logic
        if (m_hlmsOceanIndex == std::numeric_limits<uint32>::max())
        {
            OGRE_ASSERT_HIGH(dynamic_cast<HlmsOcean*>(datablock->getCreator()));
            HlmsOcean* hlms = static_cast<HlmsOcean*>(datablock->getCreator());
            hlms->_linkOcean(this);
        }
    }
    //-----------------------------------------------------------------------------------
    void Ocean::setCamera(Ogre::Camera* camera)
    {
        m_camera = camera;
    }
    //-----------------------------------------------------------------------------------
    const String& Ocean::getMovableType(void) const
    {
        static const String movType = "Ocean";
        return movType;
    }
    //-----------------------------------------------------------------------------------
    void Ocean::_swapSavedState()
    {
        m_OceanCells[0].swap(m_OceanCells[1]);
        m_savedState.m_renderables.swap(mRenderables);
        std::swap(m_savedState.m_currentCell, m_currentCell);
        std::swap(m_savedState.m_camera, m_camera);
    }
}