#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"

#include "OgreBitwise.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreAsyncTicket.h"

#include "OgreEntity.h"
#include "OgreSceneNode.h"
#include "OgreSubMesh.h"

namespace
{
    bool IsFiniteVec3(const Ogre::Vector3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    bool isValidSoupTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2)
    {
        // ── 1. Degenerate edges ──────────────────────────────────────────────────
        const float MIN_EDGE2 = 1.0e-10f; // slightly more conservative than 1e-12
        const Ogre::Vector3 e01 = v1 - v0;
        const Ogre::Vector3 e12 = v2 - v1;
        const Ogre::Vector3 e20 = v0 - v2;

        if (e01.squaredLength() < MIN_EDGE2)
        {
            return false;
        }
        if (e12.squaredLength() < MIN_EDGE2)
        {
            return false;
        }
        if (e20.squaredLength() < MIN_EDGE2)
        {
            return false;
        }

        // ── 2. Degenerate area ───────────────────────────────────────────────────
        const Ogre::Vector3 cross = e01.crossProduct(-e20); // == (v1-v0) x (v2-v0)
        const float MIN_AREA2 = 1.0e-10f;
        if (cross.squaredLength() < MIN_AREA2)
        {
            return false;
        }

        // ── 3. Newton 4 GenerateConvexCap assert guard ───────────────────────────
        // The assert fires when an adjacent face's unit normal has a dot product
        // ≥ 0.2 with the shared edge direction.  If THIS face's normal is nearly
        // parallel to any of its own edges, it WILL trigger that assert when it
        // appears as the "adjacent" face of some neighbor.  Discard it now.
        //
        // Threshold 0.17 gives a small margin below Newton's 0.2 to account for
        // floating point and the transform in CalculateGlobalNormal.
        const float MAX_NORMAL_EDGE_DOT = 0.17f;

        const Ogre::Vector3 normal = cross.normalisedCopy();

        const Ogre::Vector3 edges[3] = {e01, e12, e20};
        for (const Ogre::Vector3& e : edges)
        {
            const float len = e.length();
            if (len < 1.0e-8f)
            {
                return false; // shouldn't reach here after check 1, but be safe
            }
            const float d = std::abs(normal.dotProduct(e / len));
            if (d > MAX_NORMAL_EDGE_DOT)
            {
                return false; // this face would trigger GenerateConvexCap assert
            }
        }

        return true;
    }
}

namespace OgreNewt
{
    namespace CollisionPrimitives
    {
        Null::Null(const OgreNewt::World* world) : Collision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeNull()));
        }

        // --------------------------------
        // Box
        // --------------------------------

        Box::Box(const World* world) : ConvexCollision(world)
        {
        }

        Box::Box(const World* world, const Ogre::Vector3& size, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeBox(ndFloat32(size.x), ndFloat32(size.y), ndFloat32(size.z))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        // --------------------------------
        // Ellipsoid / Sphere
        // --------------------------------

        Ellipsoid::Ellipsoid(const World* world) : ConvexCollision(world)
        {
        }

        Ellipsoid::Ellipsoid(const World* world, const Ogre::Vector3& size, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);

            if ((size.x == size.y) && (size.y == size.z))
            {
                // Uniform sphere: use size.x as radius (same as ND3)
                 m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeSphere(ndFloat32(size.x))));

                m_shapeInstancePtr->SetLocalMatrix(localM);
            }
            else
            {
                const Ogre::Real radius = std::min(std::min(size.x, size.y), size.z);
                const Ogre::Vector3 scale = Ogre::Vector3(size.x / radius, size.y / radius, size.z / radius);

                m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeSphere(ndFloat32(radius))));

                m_shapeInstancePtr->SetLocalMatrix(localM);
                m_shapeInstancePtr->SetScale(ndVector(ndFloat32(scale.x), ndFloat32(scale.y), ndFloat32(scale.z), ndFloat32(1.0f)));
            }
        }

        // --------------------------------
        // Cylinder
        // --------------------------------

        Cylinder::Cylinder(const World* world) : ConvexCollision(world)
        {
        }

        Cylinder::Cylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
           m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeCylinder(ndFloat32(radius), ndFloat32(radius), ndFloat32(height))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        // --------------------------------
        // Capsule
        // --------------------------------

        Capsule::Capsule(const World* world) : ConvexCollision(world)
        {
        }

        Capsule::Capsule(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeCapsule(ndFloat32(radius), ndFloat32(radius), ndFloat32(height - (radius * 2.0f)))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        Capsule::Capsule(const World* world, const Ogre::Vector3& size, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeCapsule(ndFloat32(size.x), ndFloat32(size.z), ndFloat32(size.y))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        // --------------------------------
        // Cone
        // --------------------------------

        Cone::Cone(const World* world) : ConvexCollision(world)
        {
        }

        Cone::Cone(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeCone(ndFloat32(radius), ndFloat32(height))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        // --------------------------------
        // ChamferCylinder
        // --------------------------------

        ChamferCylinder::ChamferCylinder(const World* world) : ConvexCollision(world)
        {
        }

        ChamferCylinder::ChamferCylinder(const World* world, Ogre::Real radius, Ogre::Real height, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos) : ConvexCollision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeChamferCylinder(ndFloat32(radius), ndFloat32(height))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        ConvexHull::ConvexHull(const World* world) : ConvexCollision(world)
        {
        }

        ConvexHull::ConvexHull(const World* world, Ogre::v1::Entity* obj, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance, const Ogre::Vector3& forceScale) : ConvexCollision(world)
        {
            OGRE_ASSERT_LOW(obj);
            Ogre::Vector3 scale(1, 1, 1);

            if (Ogre::Node* node = obj->getParentNode())
            {
                scale = node->_getDerivedScaleUpdated();
            }

            if (forceScale != Ogre::Vector3::ZERO)
            {
                scale = forceScale;
            }

            Ogre::v1::MeshPtr mesh = obj->getMesh();
            if (mesh.isNull())
            {
                return;
            }

            const unsigned short subCount = mesh->getNumSubMeshes();
            size_t totalVerts = 0;

            // Pass 1: count vertices (shared once)
            bool sharedAdded = false;
            for (unsigned short i = 0; i < subCount; ++i)
            {
                Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);

                if (!sub)
                {
                    continue;
                }

                Ogre::v1::VertexData* vdata = nullptr;
                if (sub->useSharedVertices)
                {
                    if (!sharedAdded)
                    {
                        vdata = mesh->sharedVertexData[0];
                        sharedAdded = true;
                    }
                }
                else
                {
                    vdata = sub->vertexData[0];
                }
                if (vdata)
                {
                    totalVerts += vdata->vertexCount;
                }
            }
            if (totalVerts < 4)
            {
                return;
            }

            // Pass 2: read positions -> temp CPU array of Ogre::Vector3
            std::vector<Ogre::Vector3> verts;
            verts.reserve(totalVerts);
            sharedAdded = false;

            for (unsigned short i = 0; i < subCount; ++i)
            {
                Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
                if (!sub)
                {
                    continue;
                }

                Ogre::v1::VertexData* vdata = nullptr;
                if (sub->useSharedVertices)
                {
                    if (!sharedAdded)
                    {
                        vdata = mesh->sharedVertexData[0];
                        sharedAdded = true;
                    }
                }
                else
                {
                    vdata = sub->vertexData[0];
                }

                if (!vdata)
                {
                    continue;
                }

                const Ogre::v1::VertexElement* posElem = vdata->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
                if (!posElem)
                {
                    continue;
                }

                const size_t start = vdata->vertexStart;
                const size_t vCount = vdata->vertexCount;

                Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vdata->vertexBufferBinding->getBuffer(posElem->getSource());
                if (!vbuf)
                {
                    continue;
                }

                unsigned char* base = static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
                const size_t stride = vbuf->getVertexSize();

                for (size_t j = 0; j < vCount; ++j)
                {
                    unsigned char* ptr = base + (start + j) * stride;
                    float* p = nullptr;
                    posElem->baseVertexPointerToElement(ptr, &p);
                    Ogre::Vector3 v(p[0], p[1], p[2]);
                    verts.push_back(v * scale);
                }
                vbuf->unlock();
            }

            // Clean input & convert to contiguous ndFloat32 buffer (XYZXYZ…)
            const float dedupEps2 = 1e-16f; // collapse ~1e-8 distance
            std::vector<ndFloat32> points;
            points.reserve(verts.size() * 3);
            {
                // optional: compute bbox for adaptive tolerance
                Ogre::AxisAlignedBox aabb;
                for (const auto& v : verts)
                {
                    if (IsFiniteVec3(v))
                    {
                        aabb.merge(v);
                    }
                }

                const Ogre::Real diag = aabb.isNull() ? Ogre::Real(1) : aabb.getSize().length();

                if (tolerance <= 0)
                {
                    tolerance = std::max<Ogre::Real>(diag * Ogre::Real(1e-4), 1e-6f);
                }

                // dedup + push
                for (const auto& v : verts)
                {
                    if (!IsFiniteVec3(v))
                    {
                        continue;
                    }

                    bool dup = false;
                    // cheap linear dedup (ok for typical mesh vertex counts for hulls)
                    for (size_t i = 0; i + 2 < points.size(); i += 3)
                    {
                        const float dx = points[i + 0] - v.x;
                        const float dy = points[i + 1] - v.y;
                        const float dz = points[i + 2] - v.z;
                        if (dx * dx + dy * dy + dz * dz <= dedupEps2)
                        {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup)
                    {
                        points.push_back(v.x);
                        points.push_back(v.y);
                        points.push_back(v.z);
                    }
                }
            }

            if (points.size() < 12)
            { // < 4 points
                return;
            }

            const ndInt32 count = ndInt32(points.size() / 3);
            const ndInt32 stride = ndInt32(sizeof(ndFloat32) * 3); // tightly packed floats

            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeConvexHull(count, stride, ndFloat32(tolerance), reinterpret_cast<const ndFloat32*>(&points[0]))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        ConvexHull::ConvexHull(const World* world, Ogre::Item* item, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance, const Ogre::Vector3& forceScale) : ConvexCollision(world)
        {
            OGRE_ASSERT_LOW(item);
            Ogre::Vector3 scale(1, 1, 1);

            if (Ogre::Node* node = item->getParentNode())
            {
                scale = node->_getDerivedScaleUpdated();
            }
            if (forceScale != Ogre::Vector3::ZERO)
            {
                scale = forceScale;
            }

            std::vector<Ogre::Vector3> verts;
            verts.reserve(4096);

            Ogre::MeshPtr mesh = item->getMesh();
            if (mesh.isNull())
            {
                return;
            }

            for (Ogre::SubMesh* sub : mesh->getSubMeshes())
            {
                if (!sub)
                {
                    continue;
                }

                // Use VpNormal like the working example
                const Ogre::VertexArrayObjectArray& vaos = sub->mVao[Ogre::VpNormal];
                if (vaos.empty())
                {
                    continue;
                }

                Ogre::VertexArrayObject* vao = vaos[0];
                if (!vao)
                {
                    continue;
                }

                // Setup read request (following working example pattern)
                Ogre::VertexArrayObject::ReadRequestsVec requests;
                requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

                vao->readRequests(requests);
                vao->mapAsyncTickets(requests);

                // ADD THIS CHECK - validate data is available!
                if (!requests[0].data)
                {
                    vao->unmapAsyncTickets(requests);
                    continue;
                }

                const Ogre::VertexBufferPacked* vb = requests[0].vertexBuffer;
                if (!vb)
                {
                    vao->unmapAsyncTickets(requests);
                    continue;
                }

                const size_t count = vb->getNumElements();
                verts.reserve(verts.size() + count);

                // Process vertices following the working example pattern
                if (requests[0].type == Ogre::VET_HALF4)
                {
                    for (size_t i = 0; i < count; ++i)
                    {
                        const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                        Ogre::Vector3 v(Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1]), Ogre::Bitwise::halfToFloat(p[2]));
                        verts.push_back(v * scale);
                        requests[0].data += vb->getBytesPerElement();
                    }
                }
                else if (requests[0].type == Ogre::VET_FLOAT3 || requests[0].type == Ogre::VET_FLOAT4)
                {
                    for (size_t i = 0; i < count; ++i)
                    {
                        const float* p = reinterpret_cast<const float*>(requests[0].data);
                        Ogre::Vector3 v(p[0], p[1], p[2]);
                        verts.push_back(v * scale);
                        requests[0].data += vb->getBytesPerElement();
                    }
                }

                vao->unmapAsyncTickets(requests);
            }

            // Check if we got any vertices
            if (verts.size() < 4)
            {
                return;
            }

            // Rest of your deduplication and shape creation code...
            const float dedupEps2 = 1e-16f;
            std::vector<ndFloat32> points;
            points.reserve(verts.size() * 3);

            Ogre::AxisAlignedBox aabb;
            for (const auto& v : verts)
            {
                if (IsFiniteVec3(v))
                {
                    aabb.merge(v);
                }
            }

            const Ogre::Real diag = aabb.isNull() ? Ogre::Real(1) : aabb.getSize().length();
            if (tolerance <= 0)
            {
                tolerance = std::max<Ogre::Real>(diag * Ogre::Real(1e-4), 1e-6f);
            }

            for (const auto& v : verts)
            {
                if (!IsFiniteVec3(v))
                {
                    continue;
                }

                bool dup = false;
                for (size_t i = 0; i + 2 < points.size(); i += 3)
                {
                    const float dx = points[i + 0] - v.x;
                    const float dy = points[i + 1] - v.y;
                    const float dz = points[i + 2] - v.z;
                    if (dx * dx + dy * dy + dz * dz <= dedupEps2)
                    {
                        dup = true;
                        break;
                    }
                }
                if (!dup)
                {
                    points.push_back(v.x);
                    points.push_back(v.y);
                    points.push_back(v.z);
                }
            }

            if (points.size() < 12)
            {
                return;
            }

            const ndInt32 count = ndInt32(points.size() / 3);
            const ndInt32 stride = ndInt32(sizeof(ndFloat32) * 3);

            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeConvexHull(count, stride, ndFloat32(tolerance), reinterpret_cast<const ndFloat32*>(&points[0]))));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        ConvexHull::ConvexHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance) : ConvexCollision(world)
        {
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeConvexHull(vertcount, sizeof(Ogre::Vector3), ndFloat32(tolerance), (ndFloat32*)&verts[0].x)));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        ConcaveHull::ConcaveHull(const World* world) : ConvexCollision(world)
        {
        }

        ConcaveHull::ConcaveHull(const World* world, Ogre::v1::Entity* obj, unsigned int id, Ogre::Real tolerance, const Ogre::Vector3& scale) : ConvexCollision(world)
        {
            // Note: Newton 4.0 handles concave hulls differently
            // This may need to use ndShapeCompound with multiple convex pieces
            // Placeholder implementation - needs proper Newton 4.0 mesh decomposition
        }

        ConcaveHull::ConcaveHull(const World* world, const Ogre::Vector3* verts, int vertcount, unsigned int id, Ogre::Real tolerance) : ConvexCollision(world)
        {
        }

        // ─────────────────────────────────────────────────────────────────────────────
        // Shared triangle validation helper used by all TreeCollision constructors.
        //
        // Put this in a static anonymous namespace at the top of OgreNewt_Collision.cpp
        // (above all the TreeCollision constructors), replacing all the old inline
        // MIN_EDGE2 / MIN_AREA2 checks.
        //
        // Why the extra normal-vs-edge dot check exists:
        //   Newton 4's ndShapeConvexPolygon::GenerateConvexCap() asserts:
        //       edge.DotProduct(adjacentFaceNormal) < 0.2f
        //   for every shared edge between two adjacent soup faces.
        //   If face B's unit normal is nearly PARALLEL to one of face B's own edges,
        //   then any face A that shares that edge will trigger the assert during
        //   CalculatePolySoupToHullContactsDescrete. The assert fires in debug builds;
        //   in release it silently produces garbage contact normals.
        //
        //   This commonly happens in ProceduralRoadComponent at tight miter joins where
        //   Catmull-Rom spline geometry produces long thin triangles whose long axis
        //   aligns nearly with the face normal.
        //
        //   We detect and discard such triangles BEFORE AddFace, which is the only
        //   safe fix — the assert is inside Newton's BVH traversal, not accessible
        //   from OgreNewt.
        //
        //   Threshold 0.17 gives a margin below Newton's 0.2 to account for floating
        //   point error and the CalculateGlobalNormal transform (invScale + rotation).
        // ─────────────────────────────────────────────────────────────────────────────

        namespace
        {
            static bool isValidSoupTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2)
            {
                // 1. Reject degenerate edges
                const float MIN_EDGE2 = 1.0e-10f;
                const Ogre::Vector3 e01 = v1 - v0;
                const Ogre::Vector3 e12 = v2 - v1;
                const Ogre::Vector3 e20 = v0 - v2;
                if (e01.squaredLength() < MIN_EDGE2)
                {
                    return false;
                }
                if (e12.squaredLength() < MIN_EDGE2)
                {
                    return false;
                }
                if (e20.squaredLength() < MIN_EDGE2)
                {
                    return false;
                }

                // 2. Reject degenerate area
                const Ogre::Vector3 cross = e01.crossProduct(-e20); // (v1-v0) x (v2-v0)
                const float MIN_AREA2 = 1.0e-10f;
                if (cross.squaredLength() < MIN_AREA2)
                {
                    return false;
                }

                // 3. Reject triangles whose normal is nearly parallel to one of their
                //    own edges — these trigger Newton 4's GenerateConvexCap assert.
                const float MAX_DOT = 0.17f;
                const Ogre::Vector3 normal = cross.normalisedCopy();
                const Ogre::Vector3 edges[3] = {e01, e12, e20};
                for (const Ogre::Vector3& e : edges)
                {
                    const float len = e.length();
                    if (len < 1.0e-8f)
                    {
                        return false;
                    }
                    if (std::abs(normal.dotProduct(e / len)) > MAX_DOT)
                    {
                        return false;
                    }
                }

                return true;
            }
        } // anonymous namespace

        // ─────────────────────────────────────────────────────────────────────────────
        // TreeCollision — v1::Entity
        // ─────────────────────────────────────────────────────────────────────────────
        OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(const OgreNewt::World* world, Ogre::v1::Entity* obj, bool optimize, unsigned int id, FaceWinding fw) : Collision(world)
        {
            Ogre::Vector3 scale(1, 1, 1);
            if (Ogre::Node* node = obj->getParentNode())
            {
                scale = node->_getDerivedScaleUpdated();
            }

            FaceWinding localFw = fw;
            if (scale.x * scale.y * scale.z < 0.0f)
            {
                localFw = (fw == FW_DEFAULT) ? FW_REVERSE : FW_DEFAULT;
            }

            ndPolygonSoupBuilder meshBuilder;
            meshBuilder.Begin();

            Ogre::v1::MeshPtr mesh = obj->getMesh();
            const unsigned short subCount = mesh->getNumSubMeshes();
            size_t totalFacesAdded = 0;

            for (unsigned short i = 0; i < subCount; ++i)
            {
                Ogre::v1::SubMesh* sub = mesh->getSubMesh(i);
                if (!sub)
                {
                    continue;
                }

                Ogre::v1::VertexData* vData = sub->useSharedVertices ? mesh->sharedVertexData[0] : sub->vertexData[0];
                if (!vData)
                {
                    continue;
                }

                const Ogre::v1::VertexElement* posElem = vData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
                if (!posElem)
                {
                    continue;
                }

                const size_t vStart = vData->vertexStart;
                const size_t vCount = vData->vertexCount;

                Ogre::v1::HardwareVertexBufferSharedPtr vBuf = vData->vertexBufferBinding->getBuffer(posElem->getSource());
                if (!vBuf)
                {
                    continue;
                }

                unsigned char* vBase = static_cast<unsigned char*>(vBuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
                const size_t vStride = vBuf->getVertexSize();

                Ogre::v1::IndexData* iData = sub->indexData[0];
                if (!iData)
                {
                    vBuf->unlock();
                    continue;
                }

                Ogre::v1::HardwareIndexBufferSharedPtr iBuf = iData->indexBuffer;
                if (iBuf.isNull())
                {
                    vBuf->unlock();
                    continue;
                }

                const bool use32 = (iBuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);
                const size_t iStart = iData->indexStart;
                const size_t iCount = iData->indexCount;

                if (iCount < 3)
                {
                    vBuf->unlock();
                    continue;
                }

                unsigned char* iBase = static_cast<unsigned char*>(iBuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

                const size_t triCount = iCount / 3;

                for (size_t t = 0; t < triCount; ++t)
                {
                    unsigned int idx[3];
                    if (use32)
                    {
                        const unsigned int* src = reinterpret_cast<unsigned int*>(iBase);
                        idx[0] = src[iStart + t * 3 + 0];
                        idx[1] = src[iStart + t * 3 + 1];
                        idx[2] = src[iStart + t * 3 + 2];
                    }
                    else
                    {
                        const unsigned short* src = reinterpret_cast<unsigned short*>(iBase);
                        idx[0] = src[iStart + t * 3 + 0];
                        idx[1] = src[iStart + t * 3 + 1];
                        idx[2] = src[iStart + t * 3 + 2];
                    }

                    if (idx[0] < vStart || idx[0] >= vStart + vCount || idx[1] < vStart || idx[1] >= vStart + vCount || idx[2] < vStart || idx[2] >= vStart + vCount)
                    {
                        continue;
                    }

                    Ogre::Vector3 vtx[3];
                    for (int j = 0; j < 3; ++j)
                    {
                        const size_t vIdx = idx[j] - vStart;
                        unsigned char* ptr = vBase + vIdx * vStride;
                        float* pos = nullptr;
                        posElem->baseVertexPointerToElement(ptr, &pos);
                        vtx[j] = Ogre::Vector3(pos[0], pos[1], pos[2]) * scale;
                    }

                    if (!isValidSoupTriangle(vtx[0], vtx[1], vtx[2]))
                    {
                        continue;
                    }

                    ndVector face[3];
                    if (localFw == FW_DEFAULT)
                    {
                        face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
                        face[1] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
                        face[2] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
                    }
                    else
                    {
                        face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
                        face[1] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
                        face[2] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
                    }

                    meshBuilder.AddFace(&face[0], 3, id);
                    ++totalFacesAdded;
                }

                vBuf->unlock();
                iBuf->unlock();
            }

            if (totalFacesAdded == 0)
            {
                Ogre::LogManager::getSingleton().logMessage("OgreNewt::TreeCollision (Entity) - WARNING: mesh '" + mesh->getName() + "' produced zero valid triangles. Creating empty collision shape.", Ogre::LML_CRITICAL);
                meshBuilder.End(false);
            }
            else
            {
                meshBuilder.End(optimize);
            }

            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
        }

        // ─────────────────────────────────────────────────────────────────────────────
        // TreeCollision — Ogre::Item  (v2)
        // ─────────────────────────────────────────────────────────────────────────────
        TreeCollision::TreeCollision(const World* world, Ogre::Item* item, bool optimize, unsigned int id, FaceWinding fw) : Collision(world)
        {
            m_categoryId = id;
            start(id);

            Ogre::MeshPtr mesh = item->getMesh();
            if (mesh.isNull())
            {
                ndPolygonSoupBuilder dummy;
                dummy.Begin();
                dummy.End(false);
                m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(dummy)));
                return;
            }

            Ogre::Vector3 scale(1, 1, 1);
            if (Ogre::Node* node = item->getParentNode())
            {
                scale = node->_getDerivedScaleUpdated();
            }

            FaceWinding localFw = fw;
            if (scale.x * scale.y * scale.z < 0.0f)
            {
                localFw = (fw == FW_DEFAULT) ? FW_REVERSE : FW_DEFAULT;
            }

            ndPolygonSoupBuilder meshBuilder;
            meshBuilder.Begin();
            size_t totalFacesAdded = 0;

            for (Ogre::SubMesh* subMesh : mesh->getSubMeshes())
            {
                if (!subMesh)
                {
                    continue;
                }

                const Ogre::VertexArrayObjectArray& vaos = subMesh->mVao[0];
                if (vaos.empty())
                {
                    continue;
                }

                Ogre::VertexArrayObject* vao = vaos[0];
                Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
                if (!indexBuffer)
                {
                    continue;
                }

                // --- read positions ---
                Ogre::VertexArrayObject::ReadRequestsVec vReqs;
                vReqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
                vao->readRequests(vReqs);
                vao->mapAsyncTickets(vReqs);

                const Ogre::VertexBufferPacked* vb = vReqs[0].vertexBuffer;
                const size_t vCount = vb ? vb->getNumElements() : 0;
                std::vector<Ogre::Vector3> verts(vCount);
                bool formatOk = true;

                if (vReqs[0].type == Ogre::VET_HALF4)
                {
                    for (size_t i = 0; i < vCount; ++i)
                    {
                        const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(vReqs[0].data);
                        verts[i] = Ogre::Vector3(Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1]), Ogre::Bitwise::halfToFloat(p[2])) * scale;
                        vReqs[0].data += vb->getBytesPerElement();
                    }
                }
                else if (vReqs[0].type == Ogre::VET_FLOAT3 || vReqs[0].type == Ogre::VET_FLOAT4)
                {
                    for (size_t i = 0; i < vCount; ++i)
                    {
                        const float* p = reinterpret_cast<const float*>(vReqs[0].data);
                        verts[i] = Ogre::Vector3(p[0], p[1], p[2]) * scale;
                        vReqs[0].data += vb->getBytesPerElement();
                    }
                }
                else
                {
                    Ogre::LogManager::getSingleton().logMessage("OgreNewt::TreeCollision (Item) - WARNING: submesh of '" + mesh->getName() + "' has unrecognised position vertex format " + Ogre::StringConverter::toString(vReqs[0].type) +
                                                                    ". Expected VET_FLOAT3, VET_FLOAT4 or VET_HALF4. Submesh skipped.",
                        Ogre::LML_CRITICAL);
                    formatOk = false;
                }

                vao->unmapAsyncTickets(vReqs);

                if (!formatOk || vCount == 0)
                {
                    continue;
                }

                // --- read indices ---
                Ogre::IndexBufferPacked::IndexType idxType = indexBuffer->getIndexType();
                Ogre::AsyncTicketPtr ticket = indexBuffer->readRequest(0, indexBuffer->getNumElements());
                const void* idxData = ticket->map();
                const size_t idxCount = indexBuffer->getNumElements();

                auto emitTri = [&](uint32_t a, uint32_t b, uint32_t c)
                {
                    if (a >= verts.size() || b >= verts.size() || c >= verts.size())
                    {
                        return;
                    }

                    const Ogre::Vector3& v0 = verts[a];
                    const Ogre::Vector3& v1 = verts[b];
                    const Ogre::Vector3& v2 = verts[c];

                    if (!isValidSoupTriangle(v0, v1, v2))
                    {
                        return;
                    }

                    ndVector face[3];
                    if (localFw == FW_DEFAULT)
                    {
                        face[0] = ndVector(v0.x, v0.y, v0.z, 1.0f);
                        face[1] = ndVector(v1.x, v1.y, v1.z, 1.0f);
                        face[2] = ndVector(v2.x, v2.y, v2.z, 1.0f);
                    }
                    else
                    {
                        face[0] = ndVector(v0.x, v0.y, v0.z, 1.0f);
                        face[1] = ndVector(v2.x, v2.y, v2.z, 1.0f);
                        face[2] = ndVector(v1.x, v1.y, v1.z, 1.0f);
                    }

                    meshBuilder.AddFace(&face[0], 3, id);
                    ++totalFacesAdded;
                };

                if (idxType == Ogre::IndexBufferPacked::IT_16BIT)
                {
                    const uint16_t* idx = reinterpret_cast<const uint16_t*>(idxData);
                    for (size_t k = 0; k + 2 < idxCount; k += 3)
                    {
                        emitTri(idx[k], idx[k + 1], idx[k + 2]);
                    }
                }
                else
                {
                    const uint32_t* idx = reinterpret_cast<const uint32_t*>(idxData);
                    for (size_t k = 0; k + 2 < idxCount; k += 3)
                    {
                        emitTri(idx[k], idx[k + 1], idx[k + 2]);
                    }
                }

                ticket->unmap();
            }

            if (totalFacesAdded == 0)
            {
                Ogre::LogManager::getSingleton().logMessage("OgreNewt::TreeCollision (Item) - WARNING: mesh '" + mesh->getName() + "' produced zero valid triangles. Creating empty collision shape.", Ogre::LML_CRITICAL);
                meshBuilder.End(false);
            }
            else
            {
                meshBuilder.End(optimize);
            }

            finish(optimize);
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
        }

        // ─────────────────────────────────────────────────────────────────────────────
        // TreeCollision — float* vertices + int* indices
        // ─────────────────────────────────────────────────────────────────────────────
        OgreNewt::CollisionPrimitives::TreeCollision::TreeCollision(const OgreNewt::World* world, int numVertices, int numIndices, const float* vertices, const int* indices, bool optimize, unsigned int id, FaceWinding fw) : Collision(world)
        {
            ndPolygonSoupBuilder meshBuilder;
            meshBuilder.Begin();

            if (!vertices || !indices || numVertices <= 0 || numIndices < 3)
            {
                meshBuilder.End(false);
                m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
                return;
            }

            FaceWinding localFw = fw;
            const int triCount = numIndices / 3;
            size_t totalFacesAdded = 0;

            for (int i = 0; i < triCount; ++i)
            {
                const int i0 = indices[i * 3 + 0];
                const int i1 = indices[i * 3 + 1];
                const int i2 = indices[i * 3 + 2];

                if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= numVertices || i1 >= numVertices || i2 >= numVertices)
                {
                    continue;
                }

                Ogre::Vector3 a(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
                Ogre::Vector3 b(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
                Ogre::Vector3 c(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

                if (!isValidSoupTriangle(a, b, c))
                {
                    continue;
                }

                ndVector face[3];
                if (localFw == FW_DEFAULT)
                {
                    face[0] = ndVector(a.x, a.y, a.z, 1.0f);
                    face[1] = ndVector(b.x, b.y, b.z, 1.0f);
                    face[2] = ndVector(c.x, c.y, c.z, 1.0f);
                }
                else
                {
                    face[0] = ndVector(a.x, a.y, a.z, 1.0f);
                    face[1] = ndVector(c.x, c.y, c.z, 1.0f);
                    face[2] = ndVector(b.x, b.y, b.z, 1.0f);
                }

                meshBuilder.AddFace(&face[0], 3, id);
                ++totalFacesAdded;
            }

            if (totalFacesAdded == 0)
            {
                Ogre::LogManager::getSingleton().logMessage("OgreNewt::TreeCollision (float*/int*) - WARNING: input produced zero valid triangles."
                                                            " Creating empty collision shape.",
                    Ogre::LML_CRITICAL);
                meshBuilder.End(false);
            }
            else
            {
                meshBuilder.End(optimize);
            }

            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
        }

        // ─────────────────────────────────────────────────────────────────────────────
        // TreeCollision — Ogre::Vector3* vertices + v1::IndexData*
        // ─────────────────────────────────────────────────────────────────────────────
        TreeCollision::TreeCollision(const World* world, int numVertices, Ogre::Vector3* vertices, Ogre::v1::IndexData* indexData, bool optimize, unsigned int id, FaceWinding fw) : Collision(world)
        {
            m_categoryId = id;
            start(id);

            ndPolygonSoupBuilder meshBuilder;
            meshBuilder.Begin();

            if (!vertices || numVertices <= 0 || !indexData || !indexData->indexBuffer)
            {
                finish(optimize);
                meshBuilder.End(false);
                m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
                return;
            }

            Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;
            const size_t indexCount = indexData->indexCount;
            const size_t indexStart = indexData->indexStart;

            if (indexCount < 3)
            {
                finish(optimize);
                meshBuilder.End(false);
                m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
                return;
            }

            const size_t triCount = indexCount / 3;
            const size_t indexSize = ibuf->getIndexSize();
            assert(indexSize == 2 || indexSize == 4);

            void* indexDataRaw = ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);
            const bool use32 = (indexSize == 4);

            auto readIndex = [&](size_t i) -> uint32_t
            {
                return use32 ? static_cast<uint32_t>(reinterpret_cast<uint32_t*>(indexDataRaw)[i]) : static_cast<uint32_t>(reinterpret_cast<uint16_t*>(indexDataRaw)[i]);
            };

            FaceWinding localFw = fw;
            size_t totalFacesAdded = 0;

            for (size_t t = 0; t < triCount; ++t)
            {
                const uint32_t i0 = readIndex(indexStart + t * 3 + 0);
                const uint32_t i1 = readIndex(indexStart + t * 3 + 1);
                const uint32_t i2 = readIndex(indexStart + t * 3 + 2);

                if (i0 >= static_cast<uint32_t>(numVertices) || i1 >= static_cast<uint32_t>(numVertices) || i2 >= static_cast<uint32_t>(numVertices))
                {
                    continue;
                }

                Ogre::Vector3 vtx[3] = {vertices[i0], vertices[i1], vertices[i2]};

                if (localFw == FW_REVERSE)
                {
                    std::swap(vtx[1], vtx[2]);
                }

                if (!isValidSoupTriangle(vtx[0], vtx[1], vtx[2]))
                {
                    continue;
                }

                ndVector face[3];
                if (localFw == FW_DEFAULT)
                {
                    face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
                    face[1] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
                    face[2] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
                }
                else
                {
                    face[0] = ndVector(vtx[0].x, vtx[0].y, vtx[0].z, 1.0f);
                    face[1] = ndVector(vtx[2].x, vtx[2].y, vtx[2].z, 1.0f);
                    face[2] = ndVector(vtx[1].x, vtx[1].y, vtx[1].z, 1.0f);
                }

                meshBuilder.AddFace(&face[0], 3, id);
                ++totalFacesAdded;
            }

            ibuf->unlock();

            if (totalFacesAdded == 0)
            {
                Ogre::LogManager::getSingleton().logMessage("OgreNewt::TreeCollision (Vector3*/IndexData*) - WARNING: input produced zero valid triangles."
                                                            " Creating empty collision shape.",
                    Ogre::LML_CRITICAL);
                meshBuilder.End(false);
            }
            else
            {
                meshBuilder.End(optimize);
            }

            finish(optimize);
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeStatic_bvh(meshBuilder)));
        }

        void TreeCollision::start(unsigned int id)
        {
            m_categoryId = id;
            m_faceCount = 0;
        }

        void TreeCollision::addPoly(Ogre::Vector3* polys, unsigned int ID)
        {
            // This would need to be implemented with a persistent ndPolygonSoupBuilder
            // For now, use the constructor-based approach
        }

        void TreeCollision::finish(bool optimize)
        {
            // Finalization happens in constructor
        }

        void TreeCollision::setFaceId(unsigned int faceId)
        {
            m_categoryId = faceId;
        }

        int TreeCollision::CountFaces(void* const context, const ndFloat32* const polygon, ndInt32 strideInBytes, const ndInt32* const indexArray, ndInt32 indexCount)
        {
            TreeCollision* const me = (TreeCollision*)context;
            me->m_faceCount++;
            return 1;
        }

        int TreeCollision::MarkFaces(void* const context, const ndFloat32* const polygon, ndInt32 strideInBytes, const ndInt32* const indexArray, ndInt32 indexCount)
        {
            TreeCollision* const me = (TreeCollision*)context;
            me->m_faceCount++;
            return 1;
        }

        void TreeCollision::setRayCastCallbackActive(bool active, ndShape* col)
        {
            // Newton 4.0 handles raycasting differently
        }

        HeightField::HeightField(const World* world) : Collision(world), m_faceCount(0), m_categoryId(0)
        {
        }

        HeightField::HeightField(const World* world, int width, int height, Ogre::Real* elevationMap, Ogre::Real verticalScale, Ogre::Real horizontalScaleX, Ogre::Real horizontalScaleZ, const Ogre::Vector3& position,
            const Ogre::Quaternion& orientation, unsigned int shapeID) :
            Collision(world),
            m_faceCount(0),
            m_categoryId(shapeID)
        {
            // 1) Build ND4 heightfield (try inverted diagonals like the demo)
            ndShapeHeightfield* heightfield = new ndShapeHeightfield(ndInt32(width), ndInt32(height), ndShapeHeightfield::m_invertedDiagonals, ndFloat32(horizontalScaleX), ndFloat32(horizontalScaleZ));

            // 2) Wrap in shape instance
            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(heightfield));

            // 3) Fill elevation map, scaled in Y
            ndArray<ndReal>& heights = heightfield->GetElevationMap();
            const int count = width * height;
            ndAssert(heights.GetCount() == count);

            for (int i = 0; i < count; ++i)
            {
                const ndFloat32 h = ndFloat32(elevationMap[i] * verticalScale);
                heights[i] = ndReal(h);
            }

            heightfield->UpdateElevationMapAabb();

            // 4) Local transform (position + orientation)
            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orientation, position, localM);
            localM.m_posit.m_w = ndFloat32(1.0f);
            m_shapeInstancePtr->SetLocalMatrix(localM);
        }

        void HeightField::setFaceId(unsigned int faceId)
        {
            m_categoryId = faceId;
        }

        void HeightField::setRayCastCallbackActive(bool active, ndShape* col)
        {
            // Newton 4.0 handles raycasting differently
        }

        CompoundCollision::CompoundCollision(const World* world) : Collision(world)
        {
        }

        CompoundCollision::CompoundCollision(const World* world, std::vector<OgreNewt::CollisionPtr> col_array, unsigned int id) : Collision(world)
        {
            ndShapeCompound* compound = new ndShapeCompound();
            compound->BeginAddRemove();

            for (const OgreNewt::CollisionPtr& col : col_array)
            {
                if (col && col->getNewtonCollision())
                {
                    // IMPORTANT: preserve local transform of the child
                    ndShapeInstance* childInstSrc = col->getShapeInstance(); // new accessor
                    ndShapeInstance* instance = nullptr;

                    if (childInstSrc)
                    {
                        // Copy full instance including its local matrix
                        instance = new ndShapeInstance(*childInstSrc);
                    }
                    else
                    {
                        // Fallback: at least use the raw shape (no offset)
                        instance = new ndShapeInstance(col->getNewtonCollision());
                    }

                    compound->AddCollision(instance);
                }
            }

            compound->EndAddRemove();

            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(compound));
        }

        Pyramid::Pyramid(const World* world) : ConvexCollision(world)
        {
        }

        Pyramid::Pyramid(const World* world, const Ogre::Vector3& size, unsigned int id, const Ogre::Quaternion& orient, const Ogre::Vector3& pos, Ogre::Real tolerance) : ConvexCollision(world)
        {
            // Create pyramid vertices
            ndFloat32 vertices[15];
            unsigned short idx = 0;

            for (int ix = -1; ix <= 1; ix += 2)
            {
                for (int iz = -1; iz <= 1; iz += 2)
                {
                    vertices[idx++] = (size.x / 2.0f) * ix;
                    vertices[idx++] = -(size.y / 3.0f);
                    vertices[idx++] = (size.z / 2.0f) * iz;
                }
            }

            vertices[idx++] = 0.0f;
            vertices[idx++] = (size.y * 2.0f / 3.0f);
            vertices[idx++] = 0.0f;

            m_shapeInstancePtr = ndSharedPtr<ndShapeInstance>(new ndShapeInstance(new ndShapeConvexHull(5, sizeof(ndFloat32) * 3, ndFloat32(tolerance), vertices)));

            ndMatrix localM;
            OgreNewt::Converters::QuatPosToMatrix(orient, pos, localM);
            m_shapeInstancePtr->SetLocalMatrix(localM);;
        }

    } // end namespace CollisionPrimitives

} // end namespace OgreNewt
