#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionSerializer.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_World.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

// Newton 4.x
#include "ndShape.h"
#include "ndShapeInstance.h"
#include "ndShapeNull.h"
#include "ndShapeBox.h"
#include "ndShapeSphere.h"
#include "ndShapeCylinder.h"
#include "ndShapeCapsule.h"
#include "ndShapeCone.h"
#include "ndShapeChamferCylinder.h"
#include "ndShapeHeightfield.h"
#include "ndPolygonSoupBuilder.h"
#include "ndShapeStatic_bvh.h"

namespace OgreNewt
{

namespace
{
    static const char kMagic[8] = { 'O','N','C','O','L','L','4','\0' };
    static const ndUnsigned32 kVersion = 1u;

    inline bool hasPlyHeader(const char* data, size_t size)
    {
        // ASCII PLY starts with "ply\n"
        if (size < 4)
            return false;
        return (data[0]=='p' && data[1]=='l' && data[2]=='y' && (data[3]=='\n' || data[3]=='\r'));
    }

    template <class T>
    void writeVal(std::ostream& os, const T& v)
    {
        os.write(reinterpret_cast<const char*>(&v), sizeof(T));
    }

    template <class T>
    void readVal(std::istream& is, T& v)
    {
        is.read(reinterpret_cast<char*>(&v), sizeof(T));
    }

    void exportPrimitive(std::ostream& os, const CollisionPtr& col)
    {
        const ndShape* shape = col->getNewtonConstCollision();
        if (!shape)
            OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Collision has no shape", "CollisionSerializer::exportCollision");

        const CollisionPrimitiveType type = Collision::getCollisionPrimitiveType(shape);
        writeVal(os, static_cast<ndUnsigned32>(type));

        // Extract parameters from ndShapeInfo where possible
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
            // Sphere radius stored three times to allow non-uniform scaling round-trip later if needed
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
            writeVal(os, info.m_chamferCylinder.m_r);
            writeVal(os, info.m_chamferCylinder.m_height);
            break;

        case HeighFieldPrimitiveType:
        {
            const ndShapeHeightfield* hf = const_cast<ndShape*>(shape)->GetAsShapeHeightfield();
            if (!hf)
                OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Invalid heightfield", "CollisionSerializer::exportCollision");

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
                writeVal(os, elev[i]);
            break;
        }

        // not yet supported in this compact format:
        case ConvexHullPrimitiveType:
        case TreeCollisionPrimitiveType:
        case CompoundCollisionPrimitiveType:
        case ConcaveHullPrimitiveType:
        default:
            OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Serializer for this collision type not implemented yet.", "CollisionSerializer::exportCollision");
        }
    }

    CollisionPtr importPrimitive(std::istream& is, World* world)
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
            readVal(is, sx); readVal(is, sy); readVal(is, sz);
            dest = CollisionPtr(new CollisionPrimitives::Box(world, Ogre::Vector3(sx, sy, sz), 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case EllipsoidPrimitiveType:
        {
            float rx, ry, rz;
            readVal(is, rx); readVal(is, ry); readVal(is, rz);
            Ogre::Vector3 size(rx * 2.0f, ry * 2.0f, rz * 2.0f);
            dest = CollisionPtr(new CollisionPrimitives::Ellipsoid(world, size, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case CylinderPrimitiveType:
        {
            float r0, r1, h;
            readVal(is, r0); readVal(is, r1); readVal(is, h);
            float r = 0.5f * (r0 + r1);
            dest = CollisionPtr(new CollisionPrimitives::Cylinder(world, r, h, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case CapsulePrimitiveType:
        {
            float r0, r1, h;
            readVal(is, r0); readVal(is, r1); readVal(is, h);
            float r = 0.5f * (r0 + r1);
            float fullH = h + 2.0f * r;
            dest = CollisionPtr(new CollisionPrimitives::Capsule(world, r, fullH, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
            break;
        }

        case ConePrimitiveType:
        {
            float r, h;
            readVal(is, r); readVal(is, h);
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
                readVal(is, elev[i]);
            dest = CollisionPtr(new CollisionPrimitives::HeightField(world, w, h, elev.data(), 1.0f, sx, sz, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY, 0));
            break;
        }

        // not yet supported:
        case ConvexHullPrimitiveType:
        case TreeCollisionPrimitiveType:
        case CompoundCollisionPrimitiveType:
        case ConcaveHullPrimitiveType:
        default:
            OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Deserializer for this collision type not implemented yet.", "CollisionSerializer::importCollision");
        }

        return dest;
    }

    // Minimal ASCII PLY reader (vertices + triangular faces only)
    CollisionPtr importPLY(std::istream& is, World* world)
    {
        std::string line;
        size_t vertexCount=0, faceCount=0;
        bool headerDone=false;

        // Read header
        std::getline(is, line);
        if (line.rfind("ply", 0) != 0)
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Invalid PLY header", "CollisionSerializer::importCollision");

        while (std::getline(is, line))
        {
            if (line.find("element vertex") != std::string::npos)
            {
                std::istringstream ss(line); std::string a,b; ss>>a>>b>>vertexCount;
            }
            else if (line.find("element face") != std::string::npos)
            {
                std::istringstream ss(line);
                std::string a,b;

                ss>>a>>b>>faceCount;
            }
            else if (line == "end_header")
            {
                headerDone=true;
                break;
            }
        }
        if (!headerDone)
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "PLY header not terminated", "CollisionSerializer::importCollision");

        std::vector<Ogre::Vector3> verts(vertexCount);
        for (size_t i=0;i<vertexCount;i++)
        {
            if (!std::getline(is, line))
                OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unexpected EOF reading vertices", "CollisionSerializer::importCollision");
            std::istringstream ss(line);
            ss >> verts[i].x >> verts[i].y >> verts[i].z;
        }

        std::vector<int> indices; indices.reserve(faceCount*3);
        for (size_t i=0;i<faceCount;i++)
        {
            if (!std::getline(is, line))
                OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unexpected EOF reading faces", "CollisionSerializer::importCollision");
            std::istringstream ss(line);
            int n; ss >> n;
            if (n == 3) {
                int a,b,c; ss>>a>>b>>c;
                indices.push_back(a); indices.push_back(b); indices.push_back(c);
            }
        }
        if (verts.empty() || indices.empty())
            OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "PLY has no triangles", "CollisionSerializer::importCollision");

        ndPolygonSoupBuilder soup;
        soup.Begin();
        for (size_t i=0;i<indices.size(); i+=3) {
            ndVector f[3];
            for (int j=0;j<3;j++)
            {
                const Ogre::Vector3& v = verts[indices[i+j]];
                f[j] = ndVector(v.x, v.y, v.z, 1.0f);
            }
            soup.AddFace(&f[0].m_x, sizeof(ndVector), 3, 0);
        }
        soup.End(true);

        ndShapeStatic_bvh* shape = new ndShapeStatic_bvh(soup);
        CollisionPtr dest(new Collision(world, shape));

        return dest;
    }
} // anon

CollisionSerializer::CollisionSerializer() {}
CollisionSerializer::~CollisionSerializer() {}

void CollisionSerializer::exportCollision(const CollisionPtr& collision, const Ogre::String& filename)
{
    if (!collision)
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Argument collision is NULL", "CollisionSerializer::exportCollision");

    // if filename ends with .ply and we have an ndShapeInstance available (ND4), try SavePLY
    Ogre::String lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    const bool writePly = Ogre::StringUtil::endsWith(lower, ".ply");

    if (writePly) {
        // Access ndShapeInstance if your Collision stores it (friend access allowed).
        #ifdef OGRENEWT_HAS_SHAPE_INSTANCE
        if (collision->m_shapeInstance) {
            collision->m_shapeInstance->SavePLY(filename.c_str());
            return;
        }
        #endif
        // fall back: approximate by exporting triangles from ndShapeInfo is non-trivial.
        // For now, if no instance, we throw to make the failure explicit.
        OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
            "PLY export requires ndShapeInstance (SavePLY). Define OGRENEWT_HAS_SHAPE_INSTANCE and store it in Collision.",
            "CollisionSerializer::exportCollision");
    }

    // our custom compact binary format (ND4)
    std::ofstream os(filename.c_str(), std::ios::binary);
    if (!os.good())
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unable to open file " + filename + " for writing", "CollisionSerializer::exportCollision");

    os.write(kMagic, sizeof(kMagic));
    writeVal(os, kVersion);
    exportPrimitive(os, collision);
}

CollisionPtr CollisionSerializer::importCollision(Ogre::DataStream& stream, OgreNewt::World* world)
{
    CollisionPtr dest;
    if (!world)
        return dest;

    // slurp into memory
    const size_t size = stream.size();
    std::vector<char> buffer(size);
    stream.read(buffer.data(), size);

    // detect PLY ASCII
    if (hasPlyHeader(buffer.data(), buffer.size()))
    {
        std::stringbuf sb;
        sb.sputn(buffer.data(), std::streamsize(buffer.size()));
        std::istream is(&sb);
        dest = importPLY(is, world);
        stream.close();
        return dest;
    }

    // Otherwise expect our compact binary
    if (size < sizeof(kMagic) + sizeof(kVersion))
    {
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Serialized collision too small", "CollisionSerializer::importCollision");
    }

    std::stringbuf sb;
    sb.sputn(buffer.data(), std::streamsize(buffer.size()));
    std::istream is(&sb);

    char magic[8];
    is.read(magic, sizeof(magic));
    if (memcmp(magic, kMagic, sizeof(kMagic)) != 0)
    {
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unknown collision serialization format (bad magic).", "CollisionSerializer::importCollision");
    }

    ndUnsigned32 ver = 0;
    readVal(is, ver);
    if (ver != kVersion)
    {
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Unsupported collision serialization version.", "CollisionSerializer::importCollision");
    }

    dest = importPrimitive(is, world);
    stream.close();
    return dest;
}

} // namespace OgreNewt
