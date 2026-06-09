#pragma once

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreTextureGpu.h"
#include "Vao/OgreVertexBufferPacked.h"

#include "defines.h"

namespace NOWA
{
    // =========================================================================
    //  PlanetSun
    //
    //  A UV-sphere with a per-frame animated plasma/lava emissive texture.
    //  The geometry is static (no vertex animation) — the illusion of a boiling
    //  solar surface comes entirely from the animated 128x128 emissive map
    //  uploaded to PBSM_EMISSIVE every frame.
    //
    //  Usage:
    //    PlanetSun* sun = new PlanetSun("MySun", sceneManager);
    //    sun->create(55.0f, 32, 32, attachNode, "PlanetSunDefaultMaterial");
    //    // every render frame:
    //    sun->update(deltaTime);
    //    // cleanup:
    //    sun->destroy();
    //    delete sun;
    // =========================================================================

    class EXPORTED PlanetSun
    {
    public:
        PlanetSun(const Ogre::String& objectName, Ogre::SceneManager* sceneManager);
        ~PlanetSun();

        // Lifecycle
        bool create(float radius, int segmentsH, int segmentsV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName, float lodDistance);
        void destroy();
        bool recreate(float radius, int segmentsH, int segmentsV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName, float lodDistance);
        void update(float deltaTime);

        // Material
        void setDatablockByName(const Ogre::String& name);
        void setEmissiveColour(const Ogre::Vector3& colour);
        void setDiffuseColour(const Ogre::Vector3& colour);
        void setRoughness(float roughness);

        // Animation parameters
        void  setAnimationSpeed(float speed);
        float getAnimationSpeed() const;

        void  setPlasmaScale(float scale);
        float getPlasmaScale() const;

        // Accessors
        Ogre::Item*         getItem() const;
        Ogre::SceneNode*    getAttachedNode() const;
        Ogre::SceneManager* getSceneManager() const;
        float               getRadius() const;

        void setDynamic(bool dynamic);

    private:
        void generateBaseSphere();
        Ogre::Item* buildItem(const Ogre::String& meshName);
        void initPlasmaTexture();
        void updateAndUpload();
        void buildLodVaos(Ogre::SubMesh* subMesh);
    private:
        Ogre::String        objectName;
        Ogre::SceneManager* sceneManager;
        Ogre::SceneNode*    attachedNode;
        Ogre::Item*         sunItem;
        Ogre::MeshPtr       sunMesh;
        Ogre::VertexBufferPacked* vertexBuffer;

        size_t vertexCount;
        size_t indexCount;
        float  sunTime;
        float  animationSpeed;
        float  plasmaScale;
        float  radius;
        int    segmentsH;
        int    segmentsV;
        float lodDistance = 1000.0f;

        std::vector<Ogre::Vector3> baseDirs;
        std::vector<Ogre::Vector2> uvCoords;
        std::vector<Ogre::Vector4> tangents;
        std::vector<uint32_t>      indices;
    };

} // namespace NOWA
