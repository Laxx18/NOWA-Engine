#include "NOWAPrecompiled.h"
#include "Gizmo.h"

#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"

// Debug test:
#include "MyGUI.h"

namespace
{
    struct colourMap
    {
        int key;
        Ogre::ColourValue value;
    } colourMap[10] = {{0, Ogre::ColourValue(1.0f, 1.0f, 0.0f, 0.5f)}, {1, Ogre::ColourValue(1.0f, 1.0f, 0.0f, 1.0f)}, {2, Ogre::ColourValue(1.0f, 0.0f, 0.0f, 1.0f)}, {3, Ogre::ColourValue(0.0f, 1.0f, 0.0f, 1.0f)},
        {4, Ogre::ColourValue(0.0f, 0.0f, 1.0f, 1.0f)}, {5, Ogre::ColourValue(1.0f, 0.0f, 1.0f, 1.0f)}, {6, Ogre::ColourValue(0.0f, 1.0f, 1.0f, 1.0f)}, {7, Ogre::ColourValue(0.5f, 0.5f, 0.0f, 1.0f)}, {8, Ogre::ColourValue(0.0f, 0.5f, 0.5f, 1.0f)},
        {8, Ogre::ColourValue(0.5f, 0.0f, 0.5f, 1.0f)}};

}

namespace NOWA
{

    Gizmo::Gizmo() :
        sceneManager(nullptr),
        camera(nullptr),
        thickness(0.6f),
        arrowNodeX(nullptr),
        arrowNodeY(nullptr),
        arrowNodeZ(nullptr),
        sphereNode(nullptr),
        selectNode(nullptr),
        arrowItemX(nullptr),
        arrowItemY(nullptr),
        arrowItemZ(nullptr),
        sphereItem(nullptr),
        defaultCategory(0 << 0),
        translationLine(nullptr),
        translationLineNode(nullptr),
        rotationCircle(nullptr),
        rotationCircleNode(nullptr),
        translationCaption(nullptr),
        rotationCaption(nullptr),
        gizmoState(GIZMO_NONE),
        constraintAxis(Ogre::Vector3::ZERO)
    {
    }

    Gizmo::~Gizmo()
    {
        if (this->arrowNodeX == nullptr)
        {
            return;
        }

        NOWA::GraphicsModule::getInstance()->removeTrackedNode(arrowNodeX);
        NOWA::GraphicsModule::getInstance()->removeTrackedNode(arrowNodeY);
        NOWA::GraphicsModule::getInstance()->removeTrackedNode(arrowNodeZ);
        NOWA::GraphicsModule::getInstance()->removeTrackedNode(sphereNode);
        NOWA::GraphicsModule::getInstance()->removeTrackedNode(selectNode);

        auto sceneManager = this->sceneManager;

        auto arrowNodeX = this->arrowNodeX;
        auto arrowNodeY = this->arrowNodeY;
        auto arrowNodeZ = this->arrowNodeZ;
        auto sphereNode = this->sphereNode;
        auto selectNode = this->selectNode;

        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto sphereItem = this->sphereItem;

        ENQUEUE_DESTROY_COMMAND("Destroy Gizmo", _10(sceneManager, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, selectNode, arrowItemX, arrowItemY, arrowItemZ, sphereItem), {
                arrowNodeX->detachAllObjects();
                arrowNodeY->detachAllObjects();
                arrowNodeZ->detachAllObjects();
                sphereNode->detachAllObjects();
                selectNode->detachAllObjects();

                if (arrowItemX)
                {
                    sceneManager->destroyItem(arrowItemX);
                }
                if (arrowItemY)
                {
                    sceneManager->destroyItem(arrowItemY);
                }
                if (arrowItemZ)
                {
                    sceneManager->destroyItem(arrowItemZ);
                }
                if (sphereItem)
                {
                    sceneManager->destroyItem(sphereItem);
                }

                sceneManager->destroySceneNode(arrowNodeX);
                sceneManager->destroySceneNode(arrowNodeY);
                sceneManager->destroySceneNode(arrowNodeZ);
                sceneManager->destroySceneNode(sphereNode);
                sceneManager->destroySceneNode(selectNode);
            });

        this->destroyLine();
        this->destroyCircle();

        delete this->translationCaption;
        this->translationCaption = nullptr;

        delete this->rotationCaption;
        this->rotationCaption = nullptr;
    }

    void Gizmo::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real thickness, const Ogre::String& materialNameX, const Ogre::String& materialNameY, const Ogre::String& materialNameZ, const Ogre::String& materialNameHighlight)
    {
        this->sceneManager = sceneManager;
        this->camera = camera;
        this->thickness = thickness;
        this->materialNameX = materialNameX;
        this->materialNameY = materialNameY;
        this->materialNameZ = materialNameZ;
        this->materialNameHighlight = materialNameHighlight;
        // Register default category for gizmo
        this->defaultCategory = AppStateManager::getSingletonPtr()->getGameObjectController()->registerCategory("Default");

        ENQUEUE_RENDER_COMMAND("Gizmo::init", {
            // Create the three arrows in x-y-z axes
            // This is the main node for selected objects, at which the gizmo will be placed
            this->selectNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            this->selectNode->setName("SelectNode");

            this->arrowNodeX = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            this->arrowNodeX->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y));

            this->arrowNodeY = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            this->arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));

            this->arrowNodeZ = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);

            // Sphere remains always the same, so just set all settings here
            this->sphereNode = this->selectNode->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            this->sphereNode->setName("SphereGizmoNode");
            this->sphereItem = this->sceneManager->createItem("gizmosphere.mesh");
            this->sphereItem->setName("SphereGizmoItem");
            this->sphereItem->setDatablock("BaseWhiteLine");
            // this->sphereItem->setQueryFlags(this->categorySphere);
            // Not ported

            this->sphereItem->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);

            this->sphereItem->setCastShadows(false);
            this->sphereNode->attachObject(this->sphereItem);
            this->sphereNode->setVisible(false);

            this->firstPlane = Ogre::Plane(Ogre::Vector3::UNIT_X, this->selectNode->_getDerivedPositionUpdated());
            this->secondPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, this->selectNode->_getDerivedPositionUpdated());
            this->thirdPlane = Ogre::Plane(Ogre::Vector3::UNIT_Z, this->selectNode->_getDerivedPositionUpdated());

            this->createLine();
            this->createCircle();

            this->enabled = false;

            this->setEnabled(false);
        });
    }

    void Gizmo::update(void)
    {
        if (false == this->enabled)
        {
            return;
        }

        // Compute all values on logic thread
        Ogre::Vector3 nodePos = this->sphereNode->_getDerivedPositionUpdated();
        Ogre::Vector3 camPos = this->camera->getDerivedPosition();

        Ogre::Real camDistance = 0.0;
        if (this->camera->getProjectionType() == Ogre::PT_PERSPECTIVE)
        {
            camDistance = nodePos.distance(camPos);
        }
        else
        {
            camDistance = nodePos.distance(Ogre::Vector3(0, this->camera->getOrthoWindowHeight(), 0));
        }

        Ogre::Real fovy = this->camera->getFOVy().valueRadians();
        Ogre::Real distance = camDistance * 0.03f;

        Ogre::Vector3 sphereScale(distance * fovy * 0.5f, distance * fovy * 0.5f, distance * fovy * 0.85f);

        auto closureFunction = [this, sphereScale, distance, fovy](Ogre::Real renderDt)
        {
            if (this->translationLineNode)
            {
                this->translationLineNode->setPosition(this->getPosition());
            }

            if (sphereNode)
            {
                sphereNode->setScale(sphereScale);
            }

            if (this->arrowItemX && this->sceneManager && this->sceneManager->hasMovableObject(arrowItemX))
            {
                bool useRingMesh = (arrowItemX->getMesh()->getName() == "Ring.mesh");

                Ogre::Vector3 arrowScaleLocal;
                if (!useRingMesh)
                {
                    arrowScaleLocal = Ogre::Vector3(distance * thickness * fovy, distance * thickness * fovy, distance * fovy);
                }
                else
                {
                    arrowScaleLocal = Ogre::Vector3(distance * fovy, distance * thickness * fovy, distance * fovy);
                }

                this->arrowNodeX->setScale(arrowScaleLocal);
                this->arrowNodeY->setScale(arrowScaleLocal);
                this->arrowNodeZ->setScale(arrowScaleLocal);
            }
        };
        Ogre::String id = "Gizmo::update";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
    }

    void Gizmo::redefineFirstPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
    {
        this->firstPlane.redefine(normal, position);
    }

    void Gizmo::redefineSecondPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
    {
        this->secondPlane.redefine(normal, position);
    }

    void Gizmo::redefineThirdPlane(const Ogre::Vector3& normal, const Ogre::Vector3& position)
    {
        this->thirdPlane.redefine(normal, position);
    }

    void Gizmo::changeToTranslateGizmo()
    {
        auto sceneManager = this->sceneManager;
        auto defaultCategory = this->defaultCategory;
        auto materialNameX = this->materialNameX;
        auto materialNameY = this->materialNameY;
        auto materialNameZ = this->materialNameZ;
        auto arrowNodeX = this->arrowNodeX;
        auto arrowNodeY = this->arrowNodeY;
        auto arrowNodeZ = this->arrowNodeZ;
        auto sphereNode = this->sphereNode;
        auto constraintAxis = this->constraintAxis;

        auto oldArrowItemX = this->arrowItemX;
        auto oldArrowItemY = this->arrowItemY;
        auto oldArrowItemZ = this->arrowItemZ;

        auto thisPtr = this;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::changeToTranslateGizmo",
            _14(sceneManager, defaultCategory, materialNameX, materialNameY, materialNameZ, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, constraintAxis, oldArrowItemX, oldArrowItemY, oldArrowItemZ, thisPtr), {
                // Destroy old entities
                if (oldArrowItemX && sceneManager->hasMovableObject(oldArrowItemX))
                {
                    sceneManager->destroyItem(oldArrowItemX);
                }
                if (oldArrowItemY && sceneManager->hasMovableObject(oldArrowItemY))
                {
                    sceneManager->destroyItem(oldArrowItemY);
                }
                if (oldArrowItemZ && sceneManager->hasMovableObject(oldArrowItemZ))
                {
                    sceneManager->destroyItem(oldArrowItemZ);
                }

                // Create new X arrow entity
                Ogre::Item* newArrowItemX = sceneManager->createItem("Arrow.mesh");
                newArrowItemX->setName("XArrowGizmoItem");
                newArrowItemX->setDatablock(materialNameX);
                newArrowItemX->setQueryFlags(defaultCategory);
                newArrowItemX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemX->setCastShadows(false);
                if (arrowNodeX)
                {
                    arrowNodeX->attachObject(newArrowItemX);
                }

                // Create new Y arrow entity
                Ogre::Item* newArrowItemY = sceneManager->createItem("Arrow.mesh");
                newArrowItemY->setName("YArrowGizmoItem");
                newArrowItemY->setDatablock(materialNameY);
                newArrowItemY->setQueryFlags(defaultCategory);
                newArrowItemY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemY->setCastShadows(false);
                if (arrowNodeY)
                {
                    arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));
                    arrowNodeY->attachObject(newArrowItemY);
                }

                // Create new Z arrow entity
                Ogre::Item* newArrowItemZ = sceneManager->createItem("Arrow.mesh");
                newArrowItemZ->setName("ZArrowGizmoItem");
                newArrowItemZ->setDatablock(materialNameZ);
                newArrowItemZ->setQueryFlags(defaultCategory);
                newArrowItemZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemZ->setCastShadows(false);
                if (arrowNodeZ)
                {
                    arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
                    arrowNodeZ->attachObject(newArrowItemZ);
                }

                if (sphereNode)
                {
                    sphereNode->setVisible(true);
                }

                // Update pointers safely on render thread
                thisPtr->arrowItemX = newArrowItemX;
                thisPtr->arrowItemY = newArrowItemY;
                thisPtr->arrowItemZ = newArrowItemZ;

                thisPtr->setupFlag();
                thisPtr->unHighlightGizmo();

                // Handle constraintAxis visibility
                if (arrowNodeX && constraintAxis.x != 0.0f)
                {
                    arrowNodeX->setVisible(false);
                }
                if (arrowNodeY && constraintAxis.y != 0.0f)
                {
                    arrowNodeY->setVisible(false);
                }
                if (arrowNodeZ && constraintAxis.z != 0.0f)
                {
                    arrowNodeZ->setVisible(false);
                }
            });
    }

    void Gizmo::changeToScaleGizmo()
    {
        auto sceneManager = this->sceneManager;
        auto defaultCategory = this->defaultCategory;
        auto materialNameX = this->materialNameX;
        auto materialNameY = this->materialNameY;
        auto materialNameZ = this->materialNameZ;
        auto arrowNodeX = this->arrowNodeX;
        auto arrowNodeY = this->arrowNodeY;
        auto arrowNodeZ = this->arrowNodeZ;
        auto sphereNode = this->sphereNode;

        auto oldArrowItemX = this->arrowItemX;
        auto oldArrowItemY = this->arrowItemY;
        auto oldArrowItemZ = this->arrowItemZ;

        auto thisPtr = this;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::changeToScaleGizmo",
            _13(sceneManager, defaultCategory, materialNameX, materialNameY, materialNameZ, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, oldArrowItemX, oldArrowItemY, oldArrowItemZ, thisPtr), {
                if (oldArrowItemX && sceneManager->hasMovableObject(oldArrowItemX))
                {
                    sceneManager->destroyItem(oldArrowItemX);
                }
                if (oldArrowItemY && sceneManager->hasMovableObject(oldArrowItemY))
                {
                    sceneManager->destroyItem(oldArrowItemY);
                }
                if (oldArrowItemZ && sceneManager->hasMovableObject(oldArrowItemZ))
                {
                    sceneManager->destroyItem(oldArrowItemZ);
                }

                // Create new X entity
                Ogre::Item* newArrowItemX = sceneManager->createItem("Scale.mesh");
                newArrowItemX->setName("XArrowGizmoItem");
                newArrowItemX->setDatablock(materialNameX);
                newArrowItemX->setQueryFlags(defaultCategory);
                newArrowItemX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemX->setCastShadows(false);
                if (arrowNodeX)
                {
                    arrowNodeX->attachObject(newArrowItemX);
                }

                // Create new Y entity
                Ogre::Item* newArrowItemY = sceneManager->createItem("Scale.mesh");
                newArrowItemY->setName("YArrowGizmoItem");
                newArrowItemY->setDatablock(materialNameY);
                newArrowItemY->setQueryFlags(defaultCategory);
                newArrowItemY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemY->setCastShadows(false);
                if (arrowNodeY)
                {
                    arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X));
                    arrowNodeY->attachObject(newArrowItemY);
                }

                // Create new Z entity
                Ogre::Item* newArrowItemZ = sceneManager->createItem("Scale.mesh");
                newArrowItemZ->setName("ZArrowGizmoItem");
                newArrowItemZ->setDatablock(materialNameZ);
                newArrowItemZ->setQueryFlags(defaultCategory);
                newArrowItemZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemZ->setCastShadows(false);
                if (arrowNodeZ)
                {
                    arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
                    arrowNodeZ->attachObject(newArrowItemZ);
                }

                if (sphereNode)
                {
                    sphereNode->setVisible(true);
                }

                // Update pointers safely in render thread
                thisPtr->arrowItemX = newArrowItemX;
                thisPtr->arrowItemY = newArrowItemY;
                thisPtr->arrowItemZ = newArrowItemZ;

                // Call functions that touch render state inside render thread
                thisPtr->setupFlag();
                thisPtr->unHighlightGizmo();
            });
    }

    void Gizmo::changeToRotateGizmo()
    {
        // Capture this and pointers to all needed members and params safely for lambda capture
        auto sceneManager = this->sceneManager;
        auto defaultCategory = this->defaultCategory;
        auto materialNameX = this->materialNameX;
        auto materialNameY = this->materialNameY;
        auto materialNameZ = this->materialNameZ;
        auto arrowNodeX = this->arrowNodeX;
        auto arrowNodeY = this->arrowNodeY;
        auto arrowNodeZ = this->arrowNodeZ;
        auto sphereNode = this->sphereNode;

        // Also capture pointers to entities so we can destroy them safely in render thread
        auto oldArrowItemX = this->arrowItemX;
        auto oldArrowItemY = this->arrowItemY;
        auto oldArrowItemZ = this->arrowItemZ;

        // Pointers to this to update members inside render thread
        auto thisPtr = this;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::changeToRotateGizmo",
            _13(sceneManager, defaultCategory, materialNameX, materialNameY, materialNameZ, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, oldArrowItemX, oldArrowItemY, oldArrowItemZ, thisPtr), {
                // Destroy old entities if valid
                if (oldArrowItemX && sceneManager->hasMovableObject(oldArrowItemX))
                {
                    sceneManager->destroyItem(oldArrowItemX);
                }
                if (oldArrowItemY && sceneManager->hasMovableObject(oldArrowItemY))
                {
                    sceneManager->destroyItem(oldArrowItemY);
                }
                if (oldArrowItemZ && sceneManager->hasMovableObject(oldArrowItemZ))
                {
                    sceneManager->destroyItem(oldArrowItemZ);
                }

                // Create new X entity
                Ogre::Item* newArrowItemX = sceneManager->createItem("Ring.mesh");
                newArrowItemX->setName("XArrowGizmoItem");
                newArrowItemX->setDatablock(materialNameX);
                newArrowItemX->setQueryFlags(defaultCategory);
                newArrowItemX->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemX->setCastShadows(false);
                if (arrowNodeX)
                {
                    arrowNodeX->attachObject(newArrowItemX);
                }

                // Create new Y entity
                Ogre::Item* newArrowItemY = sceneManager->createItem("Ring.mesh");
                newArrowItemY->setName("YArrowGizmoItem");
                newArrowItemY->setDatablock(materialNameY);
                newArrowItemY->setQueryFlags(defaultCategory);
                newArrowItemY->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemY->setCastShadows(false);
                if (arrowNodeY)
                {
                    arrowNodeY->setOrientation(Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_X));
                    arrowNodeY->attachObject(newArrowItemY);
                }

                // Create new Z entity
                Ogre::Item* newArrowItemZ = sceneManager->createItem("Ring.mesh");
                newArrowItemZ->setName("ZArrowGizmoItem");
                newArrowItemZ->setDatablock(materialNameZ);
                newArrowItemZ->setQueryFlags(defaultCategory);
                newArrowItemZ->setRenderQueueGroup(NOWA::RENDER_QUEUE_GIZMO);
                newArrowItemZ->setCastShadows(false);
                if (arrowNodeZ)
                {
                    // 270 degrees around Z axis for proper arc direction
                    arrowNodeZ->setOrientation(Ogre::Quaternion(Ogre::Degree(270), Ogre::Vector3::UNIT_Z));
                    arrowNodeZ->attachObject(newArrowItemZ);
                }

                if (sphereNode)
                {
                    sphereNode->setVisible(false);
                }

                // Update this->arrowItem* pointers on logic side is unsafe here,
                // but we can store them here in thisPtr for thread safety.
                thisPtr->arrowItemX = newArrowItemX;
                thisPtr->arrowItemY = newArrowItemY;
                thisPtr->arrowItemZ = newArrowItemZ;

                // Call setupFlag() and unHighlightGizmo() inside render command to avoid threading issues
                thisPtr->setupFlag();
                thisPtr->unHighlightGizmo();
            });
    }

    void Gizmo::setEnabled(bool enable)
    {
        // Update logic-side flag immediately
        this->enabled = enable;

        // Capture pointers and values needed on render thread
        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto sphereItem = this->sphereItem;
        auto arrowNodeX = this->arrowNodeX;
        auto arrowNodeY = this->arrowNodeY;
        auto arrowNodeZ = this->arrowNodeZ;
        auto sphereNode = this->sphereNode;
        auto defaultCategory = this->defaultCategory;
        auto constraintAxis = this->constraintAxis;

        // Enqueue render commands for all render state changes
        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setEnabled",
            _11(arrowItemX, arrowItemY, arrowItemZ, sphereItem, arrowNodeX, arrowNodeY, arrowNodeZ, sphereNode, defaultCategory, constraintAxis, enable), {
                if (arrowItemZ && arrowItemZ->getParentSceneNode()) // safer check than hasMovableObject
                {
                    if (enable)
                    {
                        if (arrowItemX)
                        {
                            arrowItemX->setQueryFlags(defaultCategory);
                        }
                        if (arrowItemY)
                        {
                            arrowItemY->setQueryFlags(defaultCategory);
                        }
                        if (arrowItemZ)
                        {
                            arrowItemZ->setQueryFlags(defaultCategory);
                        }
                        if (sphereItem)
                        {
                            sphereItem->setQueryFlags(defaultCategory);
                        }

                        if (arrowNodeX)
                        {
                            arrowNodeX->setVisible(true);
                        }
                        if (arrowNodeY)
                        {
                            arrowNodeY->setVisible(true);
                        }
                        if (arrowNodeZ)
                        {
                            arrowNodeZ->setVisible(true);
                        }
                        if (sphereNode)
                        {
                            sphereNode->setVisible(true);
                        }
                    }
                    else
                    {
                        if (arrowItemX)
                        {
                            arrowItemX->setQueryFlags(0);
                        }
                        if (arrowItemY)
                        {
                            arrowItemY->setQueryFlags(0);
                        }
                        if (arrowItemZ)
                        {
                            arrowItemZ->setQueryFlags(0);
                        }
                        if (sphereItem)
                        {
                            sphereItem->setQueryFlags(0);
                        }

                        if (arrowNodeX)
                        {
                            arrowNodeX->setVisible(false);
                        }
                        if (arrowNodeY)
                        {
                            arrowNodeY->setVisible(false);
                        }
                        if (arrowNodeZ)
                        {
                            arrowNodeZ->setVisible(false);
                        }
                        if (sphereNode)
                        {
                            sphereNode->setVisible(false);
                        }
                    }
                }

                // Hide arrows according to constraintAxis
                if (constraintAxis.x != 0.0f && arrowNodeX)
                {
                    arrowNodeX->setVisible(false);
                }

                if (constraintAxis.y != 0.0f && arrowNodeY)
                {
                    arrowNodeY->setVisible(false);
                }

                if (constraintAxis.z != 0.0f && arrowNodeZ)
                {
                    arrowNodeZ->setVisible(false);
                }
            });

        // Since gizmoState is a logic variable, update it on logic thread directly (outside enqueue)
        if (false == enable)
        {
            this->gizmoState = GIZMO_NONE;
        }
    }

    bool Gizmo::isEnabled() const
    {
        return this->enabled;
    }

    void Gizmo::setSphereEnabled(bool enable)
    {
        auto sphereNode = this->sphereNode;
        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setSphereEnabled", _2(sphereNode, enable), {
            if (sphereNode)
            {
                sphereNode->setVisible(enable);
            }
        });
    }

    void Gizmo::highlightXArrow(void)
    {
        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto materialNameHighlight = this->materialNameHighlight;
        auto materialNameY = this->materialNameY;
        auto materialNameZ = this->materialNameZ;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightXArrow", _6(arrowItemX, arrowItemY, arrowItemZ, materialNameHighlight, materialNameY, materialNameZ), {
            if (arrowItemX)
            {
                arrowItemX->setDatablock(materialNameHighlight);
            }
            if (arrowItemY)
            {
                arrowItemY->setDatablock(materialNameY);
            }
            if (arrowItemZ)
            {
                arrowItemZ->setDatablock(materialNameZ);
            }
        });

        // Update gizmoState safely on logic thread since it is not a render resource
        this->gizmoState = GIZMO_ARROW_X;
    }

    void Gizmo::highlightYArrow(void)
    {
        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto materialNameX = this->materialNameX;
        auto materialNameHighlight = this->materialNameHighlight;
        auto materialNameZ = this->materialNameZ;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightYArrow", _6(arrowItemX, arrowItemY, arrowItemZ, materialNameX, materialNameHighlight, materialNameZ), {
            if (arrowItemX)
            {
                arrowItemX->setDatablock(materialNameX);
            }
            if (arrowItemY)
            {
                arrowItemY->setDatablock(materialNameHighlight);
            }
            if (arrowItemZ)
            {
                arrowItemZ->setDatablock(materialNameZ);
            }
        });

        this->gizmoState = GIZMO_ARROW_Y;
    }

    void Gizmo::highlightZArrow(void)
    {
        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto materialNameX = this->materialNameX;
        auto materialNameY = this->materialNameY;
        auto materialNameHighlight = this->materialNameHighlight;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightZArrow", _6(arrowItemX, arrowItemY, arrowItemZ, materialNameX, materialNameY, materialNameHighlight), {
            if (arrowItemX)
            {
                arrowItemX->setDatablock(materialNameX);
            }
            if (arrowItemY)
            {
                arrowItemY->setDatablock(materialNameY);
            }
            if (arrowItemZ)
            {
                arrowItemZ->setDatablock(materialNameHighlight);
            }
        });

        this->gizmoState = GIZMO_ARROW_Z;
    }

    void Gizmo::highlightSphere(void)
    {
        auto sphereItem = this->sphereItem;
        auto materialNameHighlight = this->materialNameHighlight;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::highlightSphere", _2(sphereItem, materialNameHighlight), {
            if (sphereItem)
            {
                sphereItem->setDatablock(materialNameHighlight);
            }
        });

        this->gizmoState = GIZMO_SPHERE;
    }

    void Gizmo::unHighlightGizmo(void)
    {
        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto sphereItem = this->sphereItem;
        auto materialNameX = this->materialNameX;
        auto materialNameY = this->materialNameY;
        auto materialNameZ = this->materialNameZ;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::unHighlightGizmo", _7(arrowItemX, arrowItemY, arrowItemZ, sphereItem, materialNameX, materialNameY, materialNameZ), {
            if (arrowItemX)
            {
                arrowItemX->setDatablock(materialNameX);
            }
            if (arrowItemY)
            {
                arrowItemY->setDatablock(materialNameY);
            }
            if (arrowItemZ)
            {
                arrowItemZ->setDatablock(materialNameZ);
            }
            if (sphereItem)
            {
                sphereItem->setDatablock("WhiteNoLighting");
            }
        });

        this->gizmoState = GIZMO_NONE;
    }

    void Gizmo::setupFlag(void)
    {
        // Capture pointers locally
        auto arrowItemX = this->arrowItemX;
        auto arrowItemY = this->arrowItemY;
        auto arrowItemZ = this->arrowItemZ;
        auto sphereItem = this->sphereItem;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setupFlag", _4(arrowItemX, arrowItemY, arrowItemZ, sphereItem), {
            if (arrowItemX)
            {
                arrowItemX->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
            }
            if (arrowItemY)
            {
                arrowItemY->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
            }
            if (arrowItemZ)
            {
                arrowItemZ->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
            }
            if (sphereItem)
            {
                sphereItem->setVisibilityFlags(NOWA::GIZMO_VISIBLE_MASK_VISIBLE);
            }
        });
    }

    Ogre::Real Gizmo::getGizmoBoundingRadius(void) const
    {
        if (this->arrowItemX != nullptr && this->sceneManager->hasMovableObject(this->arrowItemX))
        {
            // Attention: Is this correct?
            return this->arrowNodeX->getAttachedObject(0)->getWorldAabb().getRadius() * 1.2f;
        }
        else
        {
            return 10.0;
        }
    }

    Ogre::Plane& Gizmo::getFirstPlane(void)
    {
        return this->firstPlane;
    }

    Ogre::Plane& Gizmo::getSecondPlane(void)
    {
        return this->secondPlane;
    }

    Ogre::Plane& Gizmo::getThirdPlane(void)
    {
        return this->thirdPlane;
    }

    void Gizmo::setState(Gizmo::eGizmoState gizmoState)
    {
        this->gizmoState = gizmoState;
    }

    Gizmo::eGizmoState Gizmo::getState(void) const
    {
        return this->gizmoState;
    }

    void Gizmo::changeThickness(Ogre::Real thickness)
    {
        this->thickness = thickness;
    }

    void Gizmo::changeMaterials(const Ogre::String& materialNameX, const Ogre::String& materialNameY, const Ogre::String& materialNameZ, const Ogre::String& materialNameHighlight)
    {
        this->materialNameX = materialNameX;
        this->materialNameY = materialNameY;
        this->materialNameZ = materialNameZ;
        this->materialNameHighlight = materialNameHighlight;
    }

    Ogre::SceneNode* Gizmo::getArrowNodeX(void)
    {
        return this->arrowNodeX;
    }

    Ogre::SceneNode* Gizmo::getArrowNodeY(void)
    {
        return this->arrowNodeY;
    }

    Ogre::SceneNode* Gizmo::getArrowNodeZ(void)
    {
        return this->arrowNodeZ;
    }

    Ogre::SceneNode* Gizmo::getSelectedNode(void)
    {
        return this->selectNode;
    }

    Ogre::Item* Gizmo::getArrowItemX(void)
    {
        return this->arrowItemX;
    }

    Ogre::Item* Gizmo::getArrowItemY(void)
    {
        return this->arrowItemY;
    }

    Ogre::Item* Gizmo::getArrowItemZ(void)
    {
        return this->arrowItemZ;
    }

    Ogre::Item* Gizmo::getSphereItem(void)
    {
        return this->sphereItem;
    }

    void Gizmo::createLine(void)
    {
        ENQUEUE_RENDER_COMMAND_WAIT("Gizmo::createLine", {
            this->translationLineNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
            // this->translationLine = new Ogre::v1::ManualObject(0, &this->sceneManager->_getItemMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
            this->translationLine = this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
            // this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
            this->translationLine->setQueryFlags(0 << 0);
            this->translationLineNode->attachObject(this->translationLine);
            this->translationLine->setCastShadows(false);
            this->translationLine->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
            // this->translationLine->setDebugDisplayEnabled(true);

            this->translationCaption = new ObjectTitle("TranslationCaption", this->translationLine, this->camera, "BlueHighway", Ogre::ColourValue::White);
        });
    }

    void Gizmo::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, Ogre::Real thickness, const Ogre::String& materialName)
    {
        auto closureFunction = [this, startPosition, endPosition, thickness, materialName](Ogre::Real renderDt)
        {
            Ogre::Real const relativeThickness = this->sphereNode->getScale().x * thickness;
            this->translationLine->clear();
            this->translationLine->begin(materialName, Ogre::OperationType::OT_LINE_LIST);

            this->translationLine->position(startPosition);
            this->translationLine->index(0);
            this->translationLine->position(endPosition);
            this->translationLine->index(1);
            this->translationLine->end();

            if (this->translationCaption)
            {
                this->translationCaption->update();
            }
        };
        Ogre::String id = "Gizmo::drawLine";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
    }

    void Gizmo::hideLine(void)
    {
        ENQUEUE_RENDER_COMMAND("Gizmo::hideLine", {
            this->translationLine->clear();
            this->translationCaption->setTitle("");
        });
    }

    void Gizmo::destroyLine()
    {
        if (this->translationLineNode == nullptr)
        {
            return;
        }

        auto sceneManager = this->sceneManager;
        auto lineNode = this->translationLineNode;
        auto line = this->translationLine;

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Destroy Gizmo Line", _3(sceneManager, lineNode, line), {
            lineNode->detachAllObjects();
            if (line)
            {
                sceneManager->destroyManualObject(line);
            }
            if (lineNode->getParentSceneNode())
            {
                lineNode->getParentSceneNode()->removeAndDestroyChild(lineNode);
            }
        });

        this->translationLine = nullptr;
        this->translationLineNode = nullptr;
    }

    void Gizmo::destroyCircle()
    {
        auto sceneManager = this->sceneManager;
        auto rotationCircle = this->rotationCircle;
        auto rotationCircleNode = this->rotationCircleNode;

        if (rotationCircleNode == nullptr)
        {
            return;
        }

        ENQUEUE_RENDER_COMMAND_MULTI("Gizmo::destroyCircle", _3(sceneManager, rotationCircle, rotationCircleNode), {
            rotationCircleNode->detachAllObjects();

            if (rotationCircle != nullptr)
            {
                sceneManager->destroyManualObject(rotationCircle);
                // Can't set Gizmo's rotationCircle pointer here safely,
                // because this lambda captures copies
            }

            // Remove and destroy the child node
            if (rotationCircleNode->getParentSceneNode())
            {
                rotationCircleNode->getParentSceneNode()->removeAndDestroyChild(rotationCircleNode);
            }
        });

        // After enqueueing the render command, clear your pointers on the main thread:
        this->rotationCircle = nullptr;
        this->rotationCircleNode = nullptr;
    }

    void Gizmo::createCircle(void)
    {
        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::createCircle", _1(this), {
            this->rotationCircleNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
            this->rotationCircle = this->sceneManager->createManualObject();
            this->rotationCircle->setQueryFlags(0 << 0);
            this->rotationCircleNode->attachObject(this->rotationCircle);
            this->rotationCircle->setCastShadows(false);

            this->rotationCircle->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
            // this->rotationCircle->setDebugDisplayEnabled(true);
            // this->rotationCircle->setDynamic(true);

            this->rotationCaption = new ObjectTitle("RotationCaption", this->rotationCircle, this->camera, "BlueHighway", Ogre::ColourValue::White);
        });
    }

    void Gizmo::drawCircle(const Ogre::Quaternion& orientation, Ogre::Real fromAngle, Ogre::Real toAngle, bool counterClockWise, Ogre::Real thickness, const Ogre::String& materialName)
    {
        auto rotationCircle = this->rotationCircle;
        auto rotationCircleNode = this->rotationCircleNode;
        auto selectNode = this->selectNode;
        auto sphereNode = this->sphereNode;
        auto rotationCaption = this->rotationCaption;

        auto closureFunction = [rotationCircle, rotationCircleNode, selectNode, sphereNode, rotationCaption, fromAngle, toAngle, counterClockWise, thickness, materialName](Ogre::Real renderDt)
        {
            if (!rotationCircle || !rotationCircleNode || !selectNode || !sphereNode)
            {
                return; // Safety check to avoid null pointers
            }

            // Draw a circle for visual effect
            rotationCircle->clear();
            rotationCircle->begin(materialName, Ogre::OperationType::OT_TRIANGLE_LIST);

            Ogre::Real const radius = sphereNode->getScale().x * 5.0f;
            Ogre::Real const accuracy = 50.0f; // Number of plots for accuracy
            Ogre::Real const relativeThickness = sphereNode->getScale().x * thickness;
            unsigned int index = 0;
            int colourFactor = 1;
            Ogre::ColourValue value;

            if (true == counterClockWise)
            {
                for (Ogre::Real theta = fromAngle; theta < toAngle; theta += Ogre::Math::PI / accuracy)
                {
                    colourFactor = static_cast<int>(Ogre::Math::Abs(theta / Ogre::Math::TWO_PI)) % 10;

                    rotationCircle->position(radius * cos(theta), 0.0f, radius * sin(theta));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    rotationCircle->position(radius * cos(theta - Ogre::Math::PI / accuracy), 0.0f, radius * sin(theta - Ogre::Math::PI / accuracy));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    rotationCircle->position((radius - relativeThickness) * cos(theta - Ogre::Math::PI / accuracy), 0.0f, (radius - relativeThickness) * sin(theta - Ogre::Math::PI / accuracy));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    rotationCircle->position((radius - relativeThickness) * cos(theta), 0.0f, (radius - relativeThickness) * sin(theta));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    // Join the 4 vertices created above to form a quad.
                    rotationCircle->quad(index, index + 1, index + 2, index + 3);
                    index += 4;
                }
            }
            else
            {
                for (Ogre::Real theta = fromAngle; theta > toAngle; theta -= Ogre::Math::PI / accuracy)
                {
                    colourFactor = static_cast<int>(Ogre::Math::Abs(theta / Ogre::Math::TWO_PI)) % 10;
                    rotationCircle->position(radius * cos(theta), 0.0f, radius * sin(theta));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    rotationCircle->position(radius * cos(theta - Ogre::Math::PI / accuracy), 0.0f, radius * sin(theta - Ogre::Math::PI / accuracy));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    rotationCircle->position((radius - relativeThickness) * cos(theta - Ogre::Math::PI / accuracy), 0.0f, (radius - relativeThickness) * sin(theta - Ogre::Math::PI / accuracy));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    rotationCircle->position((radius - relativeThickness) * cos(theta), 0.0f, (radius - relativeThickness) * sin(theta));
                    // rotationCircle->colour(colourMap[colourFactor].value);
                    // Join the 4 vertices created above to form a quad.
                    rotationCircle->quad(index, index + 1, index + 2, index + 3);
                    index += 4;
                }
            }
            rotationCircle->end();
        };
        Ogre::String id = "Gizmo::drawCircle";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);

        // rotationCircleNode->setPosition(selectNode->_getDerivedPositionUpdated());
        // rotationCircleNode->setOrientation(orientation);
        NOWA::GraphicsModule::getInstance()->updateNodeTransform(this->rotationCircleNode, selectNode->_getDerivedPositionUpdated(), orientation);

        if (rotationCaption)
        {
            rotationCaption->update();
        }
    }

    void Gizmo::hideCircle(void)
    {
        auto rotationCircle = this->rotationCircle;
        auto rotationCaption = this->rotationCaption;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::hideCircle", _2(rotationCircle, rotationCaption), {
            if (rotationCircle)
            {
                rotationCircle->clear();
            }

            if (rotationCaption)
            {
                rotationCaption->setTitle("");
            }
        });
    }

    void Gizmo::setTranslationCaption(const Ogre::String& caption, const Ogre::ColourValue& color)
    {
        auto translationCaption = this->translationCaption;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setTranslationCaption", _3(translationCaption, caption, color), {
            if (translationCaption)
            {
                translationCaption->setTitle(caption);
                translationCaption->setColor(color);
            }
        });
    }

    void Gizmo::setRotationCaption(const Ogre::String& caption, const Ogre::ColourValue& color)
    {
        auto rotationCaption = this->rotationCaption;

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setRotationCaption", _3(rotationCaption, caption, color), {
            if (rotationCaption)
            {
                rotationCaption->setTitle(caption);
                rotationCaption->setColor(color);
            }
        });
    }

    void Gizmo::setPosition(const Ogre::Vector3& position)
    {
        auto closureFunction = [this, position](Ogre::Real renderDt)
        {
            if (this->constraintAxis == Ogre::Vector3::ZERO)
            {
                this->selectNode->setPosition(position);
            }
            else
            {
                if (this->constraintAxis.x != 0.0f)
                {
                    this->selectNode->setPosition(Ogre::Vector3(this->constraintAxis.x, position.y, position.z));
                }
                if (this->constraintAxis.y != 0.0f)
                {
                    this->selectNode->setPosition(Ogre::Vector3(position.x, this->constraintAxis.y, position.z));
                }
                if (this->constraintAxis.z != 0.0f)
                {
                    this->selectNode->setPosition(Ogre::Vector3(position.x, position.y, this->constraintAxis.z));
                }
            }
        };
        Ogre::String id = "Gizmo::setPosition";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
    }

    Ogre::Vector3 Gizmo::getPosition(void) const
    {
        return this->selectNode->_getDerivedPositionUpdated();
    }

    void Gizmo::setOrientation(const Ogre::Quaternion& orientation)
    {
        auto closureFunction = [this, orientation](Ogre::Real renderDt)
        {
            this->selectNode->setOrientation(orientation);
        };
        Ogre::String id = "Gizmo::setOrientation";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
    }

    Ogre::Quaternion Gizmo::getOrientation(void) const
    {
        return this->selectNode->_getDerivedOrientationUpdated();
    }

    void Gizmo::translate(const Ogre::Vector3& translateVector)
    {
        Ogre::Vector3 internalTranslateVector = translateVector;
        if (this->constraintAxis.x != 0.0f)
        {
            internalTranslateVector *= Ogre::Vector3(0.0f, 1.0f, 1.0f);
        }
        if (this->constraintAxis.y != 0.0f)
        {
            internalTranslateVector *= Ogre::Vector3(1.0f, 0.0f, 1.0f);
        }
        if (this->constraintAxis.z != 0.0f)
        {
            internalTranslateVector *= Ogre::Vector3(1.0f, 1.0f, 0.0f);
        }

        Ogre::Vector3 newPosition = this->selectNode->getPosition() + internalTranslateVector;
        NOWA::GraphicsModule::getInstance()->updateNodePosition(this->selectNode, newPosition, false);
    }

    void Gizmo::rotate(const Ogre::Quaternion& rotateQuaternion)
    {
        Ogre::Quaternion newOrientation = this->selectNode->getOrientation() * rotateQuaternion;
        NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->selectNode, newOrientation, false);
    }

    void Gizmo::setConstraintAxis(const Ogre::Vector3& constraintAxis)
    {
        // Update the member variable immediately (logic thread)
        this->constraintAxis = constraintAxis;

        // Capture the nodes and constraintAxis for the render thread lambda
        auto arrowNodeX = this->arrowNodeX;
        auto arrowNodeY = this->arrowNodeY;
        auto arrowNodeZ = this->arrowNodeZ;
        auto constraintAxisCopy = constraintAxis; // capture by value

        ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("Gizmo::setConstraintAxis", _4(arrowNodeX, arrowNodeY, arrowNodeZ, constraintAxisCopy), {
            // Show all arrows first
            arrowNodeX->setVisible(true);
            arrowNodeY->setVisible(true);
            arrowNodeZ->setVisible(true);

            // Hide arrows according to constraintAxis
            if (constraintAxisCopy.x != 0.0f)
            {
                arrowNodeX->setVisible(false);
            }
            if (constraintAxisCopy.y != 0.0f)
            {
                arrowNodeY->setVisible(false);
            }
            if (constraintAxisCopy.z != 0.0f)
            {
                arrowNodeZ->setVisible(false);
            }
        });
    }

    Ogre::Vector3 Gizmo::getCurrentDirection(void)
    {
        Ogre::Vector3 direction = Ogre::Vector3::ZERO;

        switch (this->getState())
        {
        case Gizmo::GIZMO_ARROW_X:
        {
            direction = this->getOrientation().xAxis();
            break;
        }
        case Gizmo::GIZMO_ARROW_Y:
        {
            direction = this->getOrientation().yAxis();
            break;
        }
        case Gizmo::GIZMO_ARROW_Z:
        {
            direction = this->getOrientation().zAxis();
            break;
        }
        case Gizmo::GIZMO_SPHERE:
        {
            /*gizmoDirectionX = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().xAxis();
            gizmoDirectionY = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().yAxis();
            gizmoDirectionZ = this->gizmo->getSelectedNode()->_getDerivedOrientationUpdated().zAxis();*/
        }
        }

        return direction;
    }

}; // namespace end