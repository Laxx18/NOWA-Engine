#include "NOWAPrecompiled.h"

#if 0
#include "OceanCell.h"
#include "Ocean.h"

#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

namespace Ogre
{
    OceanCell::OceanCell( Ocean *parentOcean ) :
        m_gridX( 0 ),
        m_gridZ( 0 ),
        m_lodLevel( 0 ),
        m_verticesPerLine( 1 ),
        m_sizeX( 0 ),
        m_sizeZ( 0 ),
        m_vaoManager( 0 ),
        m_parentOcean( parentOcean ),
        m_useSkirts( false )
    {
    }
    //-----------------------------------------------------------------------------------
    OceanCell::~OceanCell()
    {
        VertexArrayObjectArray::const_iterator itor = mVaoPerLod[VpNormal].begin();
        VertexArrayObjectArray::const_iterator end  = mVaoPerLod[VpNormal].end();

        while( itor != end )
            m_vaoManager->destroyVertexArrayObject( *itor++ );

        mVaoPerLod[VpNormal].clear();
        mVaoPerLod[VpShadow].clear();
    }
    //-----------------------------------------------------------------------------------
    void OceanCell::initialize( VaoManager *vaoManager, bool useSkirts )
    {
        assert( mVaoPerLod[VpNormal].empty() && "Already initialized!" );
        m_vaoManager = vaoManager;
        m_useSkirts = useSkirts;

        //Setup bufferless vao
        VertexBufferPackedVec vertexBuffers;
        VertexArrayObject *vao = vaoManager->createVertexArrayObject(
                    vertexBuffers, 0, OT_TRIANGLE_STRIP );
        mVaoPerLod[VpNormal].push_back( vao );
        mVaoPerLod[VpShadow].push_back( vao );
    }
    //-----------------------------------------------------------------------------------
    void OceanCell::setOrigin( const OceanGridPoint &gridPos, uint32 horizontalPixelDim,
                                 uint32 verticalPixelDim, uint32 lodLevel )
    {
        m_gridX             = gridPos.x;
        m_gridZ             = gridPos.z;
        m_lodLevel          = lodLevel;

        horizontalPixelDim  = std::min( horizontalPixelDim, m_parentOcean->m_width - m_gridX );
        verticalPixelDim    = std::min( verticalPixelDim, m_parentOcean->m_depth - m_gridZ );

        m_sizeX = horizontalPixelDim;
        m_sizeZ = verticalPixelDim;

        //Divide by 2^lodLevel and round up
        horizontalPixelDim  = (horizontalPixelDim + (1u << lodLevel) - 1u) >> lodLevel;
        verticalPixelDim    = (verticalPixelDim + (1u << lodLevel) - 1u) >> lodLevel;

        //Add an extra vertex to fill the gaps to the next OceanCell.
        horizontalPixelDim  += 1u;
        verticalPixelDim    += 1u;

        horizontalPixelDim  = std::max( horizontalPixelDim, 2u );
        verticalPixelDim    = std::max( verticalPixelDim, 2u );

        if( m_useSkirts )
        {
            //Add two extra vertices & two extra rows for the skirts.
            horizontalPixelDim  += 2u;
            verticalPixelDim    += 2u;
        }

        m_verticesPerLine   = horizontalPixelDim * 2u + 2u;

        assert( m_verticesPerLine * (verticalPixelDim - 1u) > 0u );

        VertexArrayObject *vao = mVaoPerLod[VpNormal][0];
        vao->setPrimitiveRange( 0, m_verticesPerLine * (verticalPixelDim - 1u) );
    }
    //-----------------------------------------------------------------------
    bool OceanCell::merge( OceanCell *next )
    {
        bool merged = false;

        if( m_lodLevel == next->m_lodLevel )
        {
            OceanGridPoint pos;
            pos.x = m_gridX;
            pos.z = m_gridZ;
            uint32 horizontalPixelDim  = m_sizeX;
            uint32 verticalPixelDim    = m_sizeZ;

            if( (this->m_gridX + this->m_sizeX == next->m_gridX ||
                 next->m_gridX + next->m_sizeX == this->m_gridX) &&
                 m_gridZ == next->m_gridZ && m_sizeZ == next->m_sizeZ )
            {
                //Merge horizontally
                pos.x = std::min( m_gridX, next->m_gridX );
                horizontalPixelDim += next->m_sizeX;

                this->setOrigin( pos, horizontalPixelDim, verticalPixelDim, m_lodLevel );
                merged = true;
            }
            else if( (this->m_gridZ + this->m_sizeZ == next->m_gridZ ||
                      next->m_gridZ + next->m_sizeZ == this->m_gridZ) &&
                      m_gridX == next->m_gridX && m_sizeX == next->m_sizeX )
            {
                //Merge vertically
                pos.z = std::min( m_gridZ, next->m_gridZ );
                verticalPixelDim += next->m_sizeZ;

                this->setOrigin( pos, horizontalPixelDim, verticalPixelDim, m_lodLevel );
                merged = true;
            }
        }

        return merged;
    }
    //-----------------------------------------------------------------------
    void OceanCell::uploadToGpu( uint32 * RESTRICT_ALIAS gpuPtr ) const
    {
        //uint32 rows = (m_sizeZ + (1u << m_lodLevel) - 1u) >> m_lodLevel;
        VertexArrayObject *vao = mVaoPerLod[VpNormal][0];

        //uvec4 numVertsPerLine
        gpuPtr[0] = m_verticesPerLine;
        gpuPtr[1] = m_lodLevel;
        gpuPtr[2] = vao->getPrimitiveCount() / m_verticesPerLine - 2u;
        gpuPtr[3] = *reinterpret_cast<uint32*>( &m_parentOcean->m_uvScale );

        //ivec4 xzTexPosBounds
        ((int32*RESTRICT_ALIAS)gpuPtr)[4] = m_gridX;
        ((int32*RESTRICT_ALIAS)gpuPtr)[5] = m_gridZ;
        ((int32*RESTRICT_ALIAS)gpuPtr)[6] = m_parentOcean->m_width - 1u;
        ((int32*RESTRICT_ALIAS)gpuPtr)[7] = m_parentOcean->m_depth - 1u;

        ((float*RESTRICT_ALIAS)gpuPtr)[8]  = m_parentOcean->m_OceanOrigin.x;
        ((float*RESTRICT_ALIAS)gpuPtr)[9]  = m_parentOcean->m_OceanOrigin.y;
        ((float*RESTRICT_ALIAS)gpuPtr)[10] = m_parentOcean->m_OceanOrigin.z;
        ((float*RESTRICT_ALIAS)gpuPtr)[11] = m_parentOcean->m_invWidth;

        ((float*RESTRICT_ALIAS)gpuPtr)[12] = m_parentOcean->m_xzRelativeSize.x;
        ((float*RESTRICT_ALIAS)gpuPtr)[13] = m_parentOcean->m_height;
        ((float*RESTRICT_ALIAS)gpuPtr)[14] = m_parentOcean->m_xzRelativeSize.y;
        ((float*RESTRICT_ALIAS)gpuPtr)[15] = m_parentOcean->m_invDepth;
    }
    //-----------------------------------------------------------------------
    const LightList& OceanCell::getLights(void) const
    {
        return m_parentOcean->queryLights();
    }
    //-----------------------------------------------------------------------------
    void OceanCell::getRenderOperation( v1::RenderOperation& op, bool casterPass )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Items do not implement getRenderOperation. You've put an Item in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix Items and Entities",
                     "OceanCell::getRenderOperation" );
    }
    //-----------------------------------------------------------------------------
    void OceanCell::getWorldTransforms(Matrix4* xform) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Items do not implement getWorldTransforms. You've put an Item in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix Items and Entities",
                     "OceanCell::getWorldTransforms" );
    }
    //-----------------------------------------------------------------------------
    bool OceanCell::getCastsShadows(void) const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Items do not implement getCastsShadows. You've put an Item in "
                     "the wrong RenderQueue ID (which is set to be compatible with "
                     "v1::Entity). Do not mix Items and Entities",
                     "OceanCell::getCastsShadows" );
    }
}

#endif
