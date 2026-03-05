#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionSerializer.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_World.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

// Newton 4.x
#include "ndPolygonSoupBuilder.h"
#include "ndShape.h"
#include "ndShapeBox.h"
#include "ndShapeCapsule.h"
#include "ndShapeChamferCylinder.h"
#include "ndShapeCone.h"
#include "ndShapeCylinder.h"
#include "ndShapeHeightfield.h"
#include "ndShapeInstance.h"
#include "ndShapeNull.h"
#include "ndShapeSphere.h"
#include "ndShapeStatic_bvh.h"

namespace
{
    const char kMagic[8] = {'O', 'N', 'C', 'O', 'L', 'L', '4', '\0'};
    const ndUnsigned32 kVersion = 1u;

    inline void logCritical(const Ogre::String& where, const Ogre::String& text)
    {
        if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
        {
            lm->logMessage("[CollisionSerializer][" + where + "]: " + text, Ogre::LML_CRITICAL);
        }
    }

    inline bool hasPlyHeader(const char* data, size_t size)
    {
        if (size < 4)
        {
            return false;
        }
        return (data[0] == 'p' && data[1] == 'l' && data[2] == 'y' && (data[3] == '\n' || data[3] == '\r'));
    }

    template <class T> void writeVal(std::ostream& os, const T& v)
    {
        os.write(reinterpret_cast<const char*>(&v), sizeof(T));
    }

    template <class T> void readVal(std::istream& is, T& v)
    {
        is.read(reinterpret_cast<char*>(&v), sizeof(T));
    }

    // ── Triangle validation (same logic as in OgreNewt_Collision.cpp) ─────────
    // Rejects degenerate triangles AND triangles whose face normal is nearly
    // parallel to one of their own edges, which triggers Newton 4's
    // GenerateConvexCap assert during contact resolution.
    static bool isValidSoupTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2)
    {
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

        const Ogre::Vector3 cross = e01.crossProduct(-e20);
        const float MIN_AREA2 = 1.0e-10f;
        if (cross.squaredLength() < MIN_AREA2)
        {
            return false;
        }

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

namespace OgreNewt
{
    void CollisionSerializer::exportPrimitive(std::ostream& os, const CollisionPtr& col)
    {
        const ndShape* shape = col->getNewtonConstCollision();
        if (!shape)
        {
            logCritical("exportPrimitive", "Collision has no shape.");
            return;
        }

        const CollisionPrimitiveType type = Collision::getCollisionPrimitiveType(shape);
        writeVal(os, static_cast<ndUnsigned32>(type));

        ndShapeInfo info = shape->GetShapeInfo();

        switch (type)
        {
        case NullPrimitiveType:
            break;

        case BoxPrimitiveType:
            writeVal(os, info.m_box.m_x);
            writeVal(os, info.m_box.m_y);
            writeVal(os, info.m_box.m_z);
            break;

        case EllipsoidPrimitiveType:
            writeVal(os, info.m_sphere.m_radius);
            writeVal(os, info.m_sphere.m_radius);
            writeVal(os, info.m_sphere.m_radius);
            break;

        case CylinderPrimitiveType:
            writeVal(os, info.m_cylinder.m_radio0);
            writeVal(os, info.m_cylinder.m_radio1);
            writeVal(os, info.m_cylinder.m_height);
            break;

        case CapsulePrimitiveType:
            writeVal(os, info.m_capsule.m_radio0);
            writeVal(os, info.m_capsule.m_radio1);
            writeVal(os, info.m_capsule.m_height);
            break;

        case ConePrimitiveType:
            writeVal(os, info.m_cone.m_radius);
            writeVal(os, info.m_cone.m_height);
            break;

        case ChamferCylinderPrimitiveType:
            writeVal(os, info.m_chamferCylinder.m_radius);
            writeVal(os, info.m_chamferCylinder.m_height);
            break;

        case HeighFieldPrimitiveType:
        {
            const ndShapeHeightfield* hf = const_cast<ndShape*>(shape)->GetAsShapeHeightfield();
            if (!hf)
            {
                logCritical("exportPrimitive", "Invalid heightfield.");
                return;
            }

            ndInt32 w = hf->GetWith();
            ndInt32 h = hf->GetHeight();
            const ndFloat32 sx = hf->GetWithScale();
            const ndFloat32 sz = hf->GetHeightScale();

            writeVal(os, w);
            writeVal(os, h);
            writeVal(os, sx);
            writeVal(os, sz);

            const ndArray<ndFloat32>& elev = hf->GetElevationMap();
            const size_t count = size_t(w) * size_t(h);
            for (size_t i = 0; i < count; ++i)
            {
                writeVal(os, elev[i]);
            }
            break;
        }

        case ConvexHullPrimitiveType:
        case TreeCollisionPrimitiveType:
        case CompoundCollisionPrimitiveType:
        case ConcaveHullPrimitiveType:
        default:
            logCritical("exportPrimitive", "Serializer for this collision type not implemented yet.");
            break;
        }
    }

    CollisionPtr CollisionSerializer::importPrimitive(std::istream& is, World* world)
    {
        ndUnsigned32 typeU = 0;
        readVal(is, typeU);
        CollisionPrimitiveType type = static_cast<CollisionPrimitiveType>(typeU);

        CollisionPtr dest;

        switch (type)
        {
        case NullPrimitiveType:
            dest = CollisionPtr(new CollisionPrimitives::Null(world));
            break;

        case BoxPrimitiveType:
        {
            float sx, sy, sz;
            readVal(is, sx);
            readVal(is, sy);
            readVal(is, sz);
            dest = CollisionPtr(new CollisionPrimitives::Box(world, Ogre::Vector3(sx, sy, sz), 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case EllipsoidPrimitiveType:
        {
            float rx, ry, rz;
            readVal(is, rx);
            readVal(is, ry);
            readVal(is, rz);
            dest = CollisionPtr(new CollisionPrimitives::Ellipsoid(world, Ogre::Vector3(rx * 2.0f, ry * 2.0f, rz * 2.0f), 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case CylinderPrimitiveType:
        {
            float r0, r1, h;
            readVal(is, r0);
            readVal(is, r1);
            readVal(is, h);
            dest = CollisionPtr(new CollisionPrimitives::Cylinder(world, 0.5f * (r0 + r1), h, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case CapsulePrimitiveType:
        {
            float r0, r1, h;
            readVal(is, r0);
            readVal(is, r1);
            readVal(is, h);
            float r = 0.5f * (r0 + r1);
            dest = CollisionPtr(new CollisionPrimitives::Capsule(world, r, h + 2.0f * r, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case ConePrimitiveType:
        {
            float r, h;
            readVal(is, r);
            readVal(is, h);
            dest = CollisionPtr(new CollisionPrimitives::Cone(world, r, h, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case ChamferCylinderPrimitiveType:
        {
            float r, h;
            readVal(is, r);
            readVal(is, h);
            dest = CollisionPtr(new CollisionPrimitives::ChamferCylinder(world, r, h, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case HeighFieldPrimitiveType:
        {
            int w, h;
            float sx, sz;
            readVal(is, w);
            readVal(is, h);
            readVal(is, sx);
            readVal(is, sz);
            std::vector<float> elev(size_t(w) * size_t(h));
            for (size_t i = 0; i < elev.size(); ++i)
            {
                readVal(is, elev[i]);
            }
            dest = CollisionPtr(new CollisionPrimitives::HeightField(world, w, h, elev.data(), 1.0f, sx, sz, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY, 0));
            break;
        }

        case ConvexHullPrimitiveType:
        case TreeCollisionPrimitiveType:
        case CompoundCollisionPrimitiveType:
        case ConcaveHullPrimitiveType:
        default:
            logCritical("importPrimitive", "Deserializer for this collision type not implemented yet.");
            break;
        }

        return dest;
    }

    // ── PLY importer ──────────────────────────────────────────────────────────
    CollisionPtr CollisionSerializer::importPLY(std::istream& is, World* world)
    {
        CollisionPtr dest;

        std::string line;
        size_t vertexCount = 0, faceCount = 0;
        bool headerDone = false;

        if (!std::getline(is, line) || line.rfind("ply", 0) != 0)
        {
            logCritical("importPLY", "Invalid PLY header.");
            return dest;
        }

        while (std::getline(is, line))
        {
            if (line.find("element vertex") != std::string::npos)
            {
                std::istringstream ss(line);
                std::string a, b;
                ss >> a >> b >> vertexCount;
            }
            else if (line.find("element face") != std::string::npos)
            {
                std::istringstream ss(line);
                std::string a, b;
                ss >> a >> b >> faceCount;
            }
            else if (line == "end_header")
            {
                headerDone = true;
                break;
            }
        }

        if (!headerDone)
        {
            logCritical("importPLY", "PLY header not terminated (no end_header).");
            return dest;
        }

        std::vector<Ogre::Vector3> verts(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            if (!std::getline(is, line))
            {
                logCritical("importPLY", "Unexpected EOF reading vertices.");
                return dest;
            }
            std::istringstream ss(line);
            ss >> verts[i].x >> verts[i].y >> verts[i].z;
        }

        // Collect triangulated indices (fan triangulation for polygons)
        std::vector<int> indices;
        indices.reserve(faceCount * 3);
        for (size_t i = 0; i < faceCount; ++i)
        {
            if (!std::getline(is, line))
            {
                logCritical("importPLY", "Unexpected EOF reading faces.");
                return dest;
            }
            std::istringstream ss(line);
            int n;
            ss >> n;
            if (n < 3)
            {
                continue;
            }

            std::vector<int> poly(n);
            for (int k = 0; k < n; ++k)
            {
                ss >> poly[k];
            }

            for (int k = 1; k < n - 1; ++k)
            {
                indices.push_back(poly[0]);
                indices.push_back(poly[k]);
                indices.push_back(poly[k + 1]);
            }
        }

        if (verts.empty() || indices.empty())
        {
            logCritical("importPLY", "PLY has no triangles.");
            return dest;
        }

        ndPolygonSoupBuilder soup;
        soup.Begin();
        size_t totalFacesAdded = 0;

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const int a = indices[i + 0];
            const int b = indices[i + 1];
            const int c = indices[i + 2];

            if (a < 0 || b < 0 || c < 0 || a >= (int)verts.size() || b >= (int)verts.size() || c >= (int)verts.size())
            {
                continue;
            }

            if (!isValidSoupTriangle(verts[a], verts[b], verts[c]))
            {
                continue;
            }

            ndVector f[3];
            f[0] = ndVector(verts[a].x, verts[a].y, verts[a].z, 1.0f);
            f[1] = ndVector(verts[b].x, verts[b].y, verts[b].z, 1.0f);
            f[2] = ndVector(verts[c].x, verts[c].y, verts[c].z, 1.0f);
            soup.AddFace(&f[0], 3, 0);
            ++totalFacesAdded;
        }

        if (totalFacesAdded == 0)
        {
            logCritical("importPLY", "PLY has no valid triangles after filtering.");
            return dest;
        }

        soup.End(true);

        ndShape* shape = new ndShapeStatic_bvh(soup);
        dest = CollisionPtr(new Collision(world, ndSharedPtr<ndShapeInstance>(new ndShapeInstance(shape))));
        return dest;
    }

    CollisionSerializer::CollisionSerializer()
    {
    }
    CollisionSerializer::~CollisionSerializer()
    {
    }

    void CollisionSerializer::exportCollision(const CollisionPtr& collision, const Ogre::String& filename)
    {
        if (!collision)
        {
            logCritical("exportCollision", "Argument collision is NULL.");
            return;
        }

        Ogre::String lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        const bool writePly = Ogre::StringUtil::endsWith(lower, ".ply");

        if (writePly)
        {
            if (collision->getShapeInstance())
            {
                collision->getShapeInstance()->SavePLY(filename.c_str());
                return;
            }
            logCritical("exportCollision", "PLY export requires ndShapeInstance (SavePLY). "
                                           "Define OGRENEWT_HAS_SHAPE_INSTANCE and store it in Collision.");
            return;
        }

        std::ofstream os(filename.c_str(), std::ios::binary);
        if (!os.good())
        {
            logCritical("exportCollision", "Unable to open file '" + filename + "' for writing.");
            return;
        }

        os.write(kMagic, sizeof(kMagic));
        writeVal(os, kVersion);
        exportPrimitive(os, collision);
    }

    CollisionPtr CollisionSerializer::importCollision(Ogre::DataStream& stream, OgreNewt::World* world)
    {
        CollisionPtr dest;
        if (!world)
        {
            return dest;
        }

        const size_t size = stream.size();
        if (size == 0)
        {
            logCritical("importCollision", "Stream is empty.");
            stream.close();
            return dest;
        }

        std::vector<char> buffer(size);
        stream.read(buffer.data(), size);

        if (hasPlyHeader(buffer.data(), buffer.size()))
        {
            std::stringbuf sb;
            sb.sputn(buffer.data(), std::streamsize(buffer.size()));
            std::istream is(&sb);
            dest = importPLY(is, world);
            stream.close();
            return dest;
        }

        if (size < sizeof(kMagic) + sizeof(kVersion))
        {
            logCritical("importCollision", "Serialized collision too small.");
            stream.close();
            return dest;
        }

        std::stringbuf sb;
        sb.sputn(buffer.data(), std::streamsize(buffer.size()));
        std::istream is(&sb);

        char magic[8];
        is.read(magic, sizeof(magic));
        if (std::memcmp(magic, kMagic, sizeof(kMagic)) != 0)
        {
            logCritical("importCollision", "Unknown collision serialization format (bad magic).");
            stream.close();
            return dest;
        }

        ndUnsigned32 ver = 0;
        readVal(is, ver);
        if (ver != kVersion)
        {
            logCritical("importCollision", "Unsupported collision serialization version.");
            stream.close();
            return dest;
        }

        dest = importPrimitive(is, world);
        stream.close();
        return dest;
    }

} // namespace OgreNewt