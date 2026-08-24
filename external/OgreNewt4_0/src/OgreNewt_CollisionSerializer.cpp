#include "OgreNewt_Stdafx.h"
#include "OgreNewt_CollisionSerializer.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_World.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
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

    // Attention: Format for a PREBUILT BVH. The .ply path stores raw geometry, which means every
    // load has to parse ASCII numbers AND rebuild the whole bounding volume hierarchy - the
    // expensive half. ndShapeStatic_bvh can be constructed straight from the three serialized
    // arrays (points / indices / nodes) WITHOUT calling Create() or CalculateAdjacent(), so this
    // format turns "import" into what it always claimed to be: a load.
    const char kBvhMagic[8] = {'O', 'N', 'B', 'V', 'H', '1', '0', '\0'};
    const ndUnsigned32 kBvhVersion = 1u;

    inline bool hasBvhHeader(const char* data, size_t size)
    {
        return (size >= sizeof(kBvhMagic)) && (0 == std::memcmp(data, kBvhMagic, sizeof(kBvhMagic)));
    }

    template <class T> void writeArray(std::ostream& os, const ndArray<T>& values)
    {
        const ndUnsigned32 count = static_cast<ndUnsigned32>(values.GetCount());
        os.write(reinterpret_cast<const char*>(&count), sizeof(count));

        if (count > 0)
        {
            // Attention: raw byte dump. All three types are trivially copyable PODs, but that also
            // makes the format compiler and platform specific - it is a cache, not an interchange
            // format. kBvhVersion must be bumped whenever a Newton update changes their layout.
            os.write(reinterpret_cast<const char*>(&values[0]), std::streamsize(sizeof(T) * count));
        }
    }

    template <class T> bool readArray(std::istream& is, ndArray<T>& values)
    {
        ndUnsigned32 count = 0;
        is.read(reinterpret_cast<char*>(&count), sizeof(count));

        if (!is.good() && 0 != count)
        {
            return false;
        }

        values.SetCount(ndInt32(count));

        if (count > 0)
        {
            is.read(reinterpret_cast<char*>(&values[0]), std::streamsize(sizeof(T) * count));
            if (!is.good())
            {
                return false;
            }
        }

        return true;
    }

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

    // ── Fast, allocation-free ASCII number scanning ────────────────────────────
    // Replaces getline()+istringstream per line, which is the dominant cost for
    // large ASCII PLY files (each line otherwise pays for a heap-allocating
    // getline copy, a locale-aware istringstream construction, and formatted
    // operator>> extraction). strtof/strtol skip leading whitespace (including
    // newlines) themselves, so this naturally walks across "x y z" / "n i0 i1..."
    // lines without needing explicit line boundaries.
    inline float fastNextFloat(const char*& p)
    {
        char* endPtr = nullptr;
        float v = std::strtof(p, &endPtr);
        p = endPtr;
        return v;
    }

    inline long fastNextInt(const char*& p)
    {
        char* endPtr = nullptr;
        long v = std::strtol(p, &endPtr, 10);
        p = endPtr;
        return v;
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
    Ogre::String CollisionSerializer::getBvhSidecarPath(const Ogre::String& filePath)
    {
        const size_t dotPosition = filePath.find_last_of('.');
        const size_t slashPosition = filePath.find_last_of("/\\");

        // Only treat it as an extension if the dot comes after the last path separator.
        if (Ogre::String::npos != dotPosition && (Ogre::String::npos == slashPosition || dotPosition > slashPosition))
        {
            return filePath.substr(0, dotPosition) + ".bvh";
        }

        return filePath + ".bvh";
    }

    CollisionPtr CollisionSerializer::importCollisionCached(const Ogre::String& filePath, World* world)
    {
        // Attention: This is the generic accelerator for EVERY collision that ends up as an
        // ndShapeStatic_bvh - which includes compounds. A compound is written with SavePLY(), and
        // that bakes all child shapes into a single polygon soup, so loading it produces one large
        // tree collision. The expensive part of that load is not reading the file, it is rebuilding
        // the bounding volume hierarchy (measured at ~100 ms for 35k points).
        //
        // So: on the first load the .ply is read as before and a .bvh sidecar is written next to
        // it. Every later load takes the sidecar and skips the rebuild entirely.
        //
        // Attention: the original file is NEVER deleted or modified. It stays the source of truth,
        // the sidecar is a derived cache that can be deleted at any time.
        const Ogre::String bvhPath = getBvhSidecarPath(filePath);

        // 1) Try the prebuilt sidecar first.
        {
            std::ifstream bvhFile(bvhPath.c_str(), std::ios::binary);
            if (bvhFile.good())
            {
                bvhFile.seekg(0, std::ios::end);
                const std::streamoff bvhSize = bvhFile.tellg();
                bvhFile.seekg(0, std::ios::beg);

                if (bvhSize > 0)
                {
                    std::vector<char> bvhBuffer(static_cast<size_t>(bvhSize));
                    bvhFile.read(bvhBuffer.data(), bvhSize);
                    bvhFile.close();

                    std::stringbuf sb;
                    sb.sputn(bvhBuffer.data(), std::streamsize(bvhBuffer.size()));
                    std::istream is(&sb);

                    CollisionPtr cached = this->importBvh(is, world);
                    if (cached)
                    {
                        return cached;
                    }

                    // Attention: a stale or corrupt sidecar must not block loading. Fall through to
                    // the original file and overwrite the sidecar below.
                    logCritical("importCollisionCached", "Prebuilt BVH '" + bvhPath + "' could not be used, falling back to '" + filePath + "'.");
                }
            }
        }

        // 2) Load the original file.
        std::ifstream sourceFile(filePath.c_str(), std::ios::binary);
        if (false == sourceFile.good())
        {
            logCritical("importCollisionCached", "Cannot open '" + filePath + "'.");
            return CollisionPtr();
        }

        sourceFile.seekg(0, std::ios::end);
        const std::streamoff sourceSize = sourceFile.tellg();
        sourceFile.seekg(0, std::ios::beg);

        if (sourceSize <= 0)
        {
            logCritical("importCollisionCached", "'" + filePath + "' is empty.");
            return CollisionPtr();
        }

        std::vector<unsigned char> sourceBuffer(static_cast<size_t>(sourceSize));
        sourceFile.read(reinterpret_cast<char*>(sourceBuffer.data()), sourceSize);
        sourceFile.close();

        Ogre::MemoryDataStream sourceStream(sourceBuffer.data(), sourceBuffer.size(), false, true);
        CollisionPtr result = this->importCollision(sourceStream, world);

        if (!result)
        {
            return result;
        }

        // 3) Write the sidecar, but only if the result really is a BVH shape. Heightfields and
        //    other shapes legitimately are not, and simply keep loading the normal way.
        ndShapeInstance* const instance = result->getShapeInstance();
        if (nullptr != instance)
        {
            ndShape* const shape = instance->GetShape();
            if (nullptr != shape && nullptr != shape->GetAsShapeStaticBVH())
            {
                if (true == this->exportBvh(result, bvhPath))
                {
                    if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
                    {
                        lm->logMessage("[CollisionSerializer][importCollisionCached] Wrote prebuilt BVH sidecar '" + bvhPath + "' - subsequent loads will skip the hierarchy rebuild.", Ogre::LML_CRITICAL);
                    }
                }
            }
        }

        return result;
    }

    CollisionPtr CollisionSerializer::importBvh(std::istream& is, World* world)
    {
        CollisionPtr dest;

        char magic[8];
        is.read(magic, sizeof(magic));
        if (0 != std::memcmp(magic, kBvhMagic, sizeof(kBvhMagic)))
        {
            logCritical("importBvh", "Bad magic.");
            return dest;
        }

        ndUnsigned32 version = 0;
        readVal(is, version);
        if (version != kBvhVersion)
        {
            // Attention: NOT an error. The caller is expected to fall back to the .ply file and
            // write a fresh .bvh afterwards. That is the migration path after a Newton update.
            logCritical("importBvh", "Version mismatch, the prebuilt BVH cache is stale.");
            return dest;
        }

        auto start = std::chrono::high_resolution_clock::now();

        ndArray<ndVector> points;
        ndArray<ndInt32> indices;
        ndArray<ndAabbPolygonSoup::ndNode> nodes;

        if (false == readArray(is, points) || false == readArray(is, indices) || false == readArray(is, nodes))
        {
            logCritical("importBvh", "Truncated or corrupt prebuilt BVH file.");
            return dest;
        }

        if (0 == points.GetCount() || 0 == indices.GetCount() || 0 == nodes.GetCount())
        {
            logCritical("importBvh", "Prebuilt BVH file is empty.");
            return dest;
        }

        // No Create(), no CalculateAdjacent() - the hierarchy is taken as is.
        ndShape* shape = new ndShapeStatic_bvh(points, indices, nodes);
        dest = CollisionPtr(new Collision(world, ndSharedPtr<ndShapeInstance>(new ndShapeInstance(shape))));

        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = finish - start;

        if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
        {
            lm->logMessage("[CollisionSerializer][importBvh] Loaded prebuilt BVH with " + Ogre::StringConverter::toString((unsigned int)points.GetCount()) + " points and " + Ogre::StringConverter::toString((unsigned int)nodes.GetCount()) +
                               " nodes in " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + "s",
                Ogre::LML_CRITICAL);
        }

        return dest;
    }

    bool CollisionSerializer::exportBvh(const CollisionPtr& collision, const Ogre::String& filename)
    {
        if (!collision)
        {
            logCritical("exportBvh", "Argument collision is NULL.");
            return false;
        }

        ndShapeInstance* const instance = collision->getShapeInstance();
        if (nullptr == instance)
        {
            logCritical("exportBvh", "Collision has no shape instance.");
            return false;
        }

        ndShape* const shape = instance->GetShape();
        if (nullptr == shape)
        {
            logCritical("exportBvh", "Collision has no shape.");
            return false;
        }

        // Attention: Use Newton's OWN typed downcast, never a static_cast based on
        // Collision::getCollisionPrimitiveType(). That helper reports TreeCollisionPrimitiveType
        // for other static meshes as well - a heightfield terrain among them. ndShapeHeightfield
        // derives from ndShapeStaticMesh but NOT from ndAabbPolygonSoup, so the static_cast chain
        // produced a bogus pointer (the second base is at a non-zero offset that simply does not
        // exist there) and Serialize() crashed. GetAsShapeStaticBVH() returns nullptr for anything
        // that is not a real BVH shape.
        //
        // Note on access: the override in ndShapeStatic_bvh is protected, but access is checked
        // against the STATIC type of the expression - and the declaration in ndShape is public, so
        // calling it through an ndShape* is fine.
        const ndShapeStatic_bvh* const bvhShape = shape->GetAsShapeStaticBVH();

        if (nullptr == bvhShape)
        {
            // Not an error: heightfields and other static meshes legitimately end up here. They
            // have to be written through the generic ONCOLL4 path instead, i.e. with a filename
            // that does not end in ".bvh".
            logCritical("exportBvh", "Shape is not an ndShapeStatic_bvh (heightfield or other static mesh?), no prebuilt BVH written.");
            return false;
        }

        // ndShapeStatic_bvh inherits ndAabbPolygonSoup publicly, and GetMeshShape() - which is
        // protected - does nothing else than call Serialize() on itself. We take the same route
        // directly. Requires D_CORE_API on the array overload of ndAabbPolygonSoup::Serialize.
        const ndAabbPolygonSoup* const soup = static_cast<const ndAabbPolygonSoup*>(bvhShape);

        ndArray<ndVector> points;
        ndArray<ndInt32> indices;
        ndArray<ndAabbPolygonSoup::ndNode> nodes;

        soup->Serialize(points, indices, nodes);

        if (0 == points.GetCount() || 0 == indices.GetCount() || 0 == nodes.GetCount())
        {
            logCritical("exportBvh", "Serialize produced an empty hierarchy, nothing written.");
            return false;
        }

        std::ofstream os(filename.c_str(), std::ios::binary);
        if (!os.good())
        {
            logCritical("exportBvh", "Unable to open file '" + filename + "' for writing.");
            return false;
        }

        os.write(kBvhMagic, sizeof(kBvhMagic));
        writeVal(os, kBvhVersion);

        writeArray(os, points);
        writeArray(os, indices);
        writeArray(os, nodes);

        const bool succeeded = os.good();
        os.close();

        if (false == succeeded)
        {
            // Attention: a half written cache file is worse than none - it would be detected as
            // valid by the magic and then load a truncated hierarchy. Remove it.
            std::remove(filename.c_str());
            logCritical("exportBvh", "Writing failed, removed the incomplete file '" + filename + "'.");
            return false;
        }

        if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
        {
            lm->logMessage("[CollisionSerializer][exportBvh] Wrote prebuilt BVH with " + Ogre::StringConverter::toString((unsigned int)points.GetCount()) + " points, " + Ogre::StringConverter::toString((unsigned int)indices.GetCount()) +
                               " indices and " + Ogre::StringConverter::toString((unsigned int)nodes.GetCount()) + " nodes to '" + filename + "'",
                Ogre::LML_CRITICAL);
        }

        return true;
    }

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

        auto parseStart = std::chrono::high_resolution_clock::now();

        // Everything after the header is pure "x y z" / "n i0 i1 i2 ..." tokens.
        // Slurp it once into a contiguous, null-terminated buffer and scan it
        // directly with strtof/strtol instead of getline()+istringstream per line.
        std::string body((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        const char* p = body.data();

        std::vector<Ogre::Vector3> verts(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i)
        {
            verts[i].x = fastNextFloat(p);
            verts[i].y = fastNextFloat(p);
            verts[i].z = fastNextFloat(p);
        }

        // Collect triangulated indices (fan triangulation for polygons)
        std::vector<int> indices;
        indices.reserve(faceCount * 3);
        for (size_t i = 0; i < faceCount; ++i)
        {
            int n = static_cast<int>(fastNextInt(p));
            if (n < 3)
            {
                continue;
            }

            std::vector<int> poly(n);
            for (int k = 0; k < n; ++k)
            {
                poly[k] = static_cast<int>(fastNextInt(p));
            }

            for (int k = 1; k < n - 1; ++k)
            {
                indices.push_back(poly[0]);
                indices.push_back(poly[k]);
                indices.push_back(poly[k + 1]);
            }
        }

        auto parseFinish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> parseElapsed = parseFinish - parseStart;
        // Attention: LML_CRITICAL on purpose while measuring - LML_TRIVIAL is filtered out by the
        // default log detail level, which is why this line never appeared. Set it back to
        // LML_TRIVIAL once the split between parsing and BVH build is known.
        if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
        {
            lm->logMessage("[CollisionSerializer][importPLY] Parsed " + Ogre::StringConverter::toString((unsigned int)vertexCount) + " vertices and " + Ogre::StringConverter::toString((unsigned int)faceCount) + " faces in " +
                               Ogre::StringConverter::toString(parseElapsed.count() * 0.001) + "s",
                Ogre::LML_CRITICAL);
        }

        if (verts.empty() || indices.empty())
        {
            logCritical("importPLY", "PLY has no triangles.");
            return dest;
        }

        auto soupStart = std::chrono::high_resolution_clock::now();

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

        auto soupFinish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> soupElapsed = soupFinish - soupStart;
        if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
        {
            lm->logMessage("[CollisionSerializer][importPLY] Built BVH from " + Ogre::StringConverter::toString((unsigned int)totalFacesAdded) + " faces in " + Ogre::StringConverter::toString(soupElapsed.count() * 0.001) + "s", Ogre::LML_CRITICAL);
        }

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
        const bool writeBvh = Ogre::StringUtil::endsWith(lower, ".bvh");

        // Attention: A ".bvh" filename MUST go through exportBvh(). Without this branch it fell
        // through to the generic primitive path below, which writes the kMagic header and then
        // calls exportPrimitive() - and that has no serializer for tree collisions. The result was
        // a 1 KB file containing nothing but "ONCOLL4", which on load yields an EMPTY collision:
        // no error, no crash, the object simply has no collision at all.
        if (false == writeBvh)
        {
            // Attention: INVALIDATE the sidecar. importCollisionCached() writes a <name>.bvh next
            // to the source file, and that cache is derived from the geometry as it was at the
            // time. Rewriting the source here means the geometry changed - a regenerated foliage
            // distribution, an edited road - so a stale sidecar would silently keep serving the
            // OLD collision while the visible mesh is the new one. There is no error and no crash,
            // just physics that no longer matches what you see.
            const Ogre::String staleSidecar = getBvhSidecarPath(filename);

            if (0 == std::remove(staleSidecar.c_str()))
            {
                if (Ogre::LogManager* lm = Ogre::LogManager::getSingletonPtr())
                {
                    lm->logMessage("[CollisionSerializer][exportCollision] Removed stale prebuilt BVH '" + staleSidecar + "', it will be rebuilt on the next load.", Ogre::LML_TRIVIAL);
                }
            }
        }

        if (writeBvh)
        {
            if (false == this->exportBvh(collision, filename))
            {
                logCritical("exportCollision", "Could not write the prebuilt BVH to '" + filename +
                                                   "'. No file was written - delete any leftover file, otherwise a later load "
                                                   "would silently produce a collision without geometry.");
            }
            return;
        }

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

        if (hasBvhHeader(buffer.data(), buffer.size()))
        {
            std::stringbuf bvhBuffer;
            bvhBuffer.sputn(buffer.data(), std::streamsize(buffer.size()));
            std::istream bvhStream(&bvhBuffer);
            dest = importBvh(bvhStream, world);
            stream.close();
            return dest;
        }

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