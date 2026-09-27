#include "NOWAPrecompiled.h"
#include "OgreNewtModule.h"
#include "GraphicsModule.h"
#include "main/AppStateManager.h"

#include <thread>

namespace NOWA
{
    namespace
    {
        /**
         * @brief	Computes how many threads the Newton worker pool may use.
         * @param	desiredThreadCount	The thread count requested by the scene configuration. Values below 1 mean "use the hardware default".
         * @return	The clamped thread count, always at least 1.
         * @note	Only half of the logical cores are used, so the render thread, the logic thread and the OS keep enough room.
         *			The result is clamped to 16, because Newton's own pool limit is small and a huge pool only costs synchronization time.
         */
        int computePhysicsThreadCount(int desiredThreadCount)
        {
            int hardwareThreadCount = static_cast<int>(std::thread::hardware_concurrency());
            if (hardwareThreadCount < 1)
            {
                // hardware_concurrency() is allowed to return 0 when it cannot determine the core count.
                hardwareThreadCount = 1;
            }

            int usableThreadCount = hardwareThreadCount;
            if (usableThreadCount >= 2)
            {
                usableThreadCount /= 2;
            }

            int resultThreadCount = desiredThreadCount;
            if (resultThreadCount < 1)
            {
                resultThreadCount = usableThreadCount;
            }
            if (resultThreadCount > usableThreadCount)
            {
                resultThreadCount = usableThreadCount;
            }
            if (resultThreadCount < 1)
            {
                resultThreadCount = 1;
            }
            if (resultThreadCount > 16)
            {
                resultThreadCount = 16;
            }

            return resultThreadCount;
        }
    }

    OgreNewtModule::OgreNewtModule(const Ogre::String& appStateName) :
        appStateName(appStateName),
        ogreNewt(nullptr), // at this time, only one ogrenewt instance is supported
        showText(true),
        globalGravity(Ogre::Vector3(0.0f, -19.8f, 0.0f))
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreNewtModule] Module created");
    }

    OgreNewtModule::~OgreNewtModule()
    {
    }

    OgreNewt::World* OgreNewtModule::createPhysics(const Ogre::String& name, int solverModel, int broadPhaseAlgorithm, int multithreadSolverOnSingleIsland, int threadCount, Ogre::Real updateRate, Ogre::Real defaultLinearDamping,
        Ogre::Vector3 defaultAngularDamping)
    {
        if (nullptr != this->ogreNewt)
        {
            this->destroyContent();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Initializing OgreNewt");

        this->ogreNewt = new OgreNewt::World(updateRate, 2, name);

        // Attention: The Newton worker pool must be sized FIRST, on a completely empty world, and exactly once.
        // OgreNewt::World::setThreadCount() forwards to ndWorld::SetThreadCount() -> ndThreadPool::SetCount(),
        // and that function always does "delete[] m_workers; m_workers = new ndWorker[n]". So every call destroys
        // all existing Newton worker threads and spawns new ones. If such a rebuild happens while a previously
        // spawned std::thread has been created but not yet scheduled by the OS, that worker's ndThread object is
        // freed underneath it and its very first virtual dispatch runs through a dead vtable pointer. That is the
        // intermittent access violation inside ndThread::'vcall' that showed up directly on world creation.
        // Sizing the pool here, before any other world configuration, turns this into a pure allocation:
        // the constructor leaves the pool unsized on purpose, so this is the one and only SetCount() call.
        const int usedThreadCount = computePhysicsThreadCount(threadCount);
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(usedThreadCount) + " cores for physics simulation and updaterate: " + Ogre::StringConverter::toString(updateRate));
        this->ogreNewt->setThreadCount(usedThreadCount);

        this->ogreNewt->setSolverModel(solverModel);
        // this->ogreNewt->setBroadPhaseAlgorithm(broadPhaseAlgorithm);
        this->ogreNewt->setDefaultLinearDamping(defaultLinearDamping);
        this->ogreNewt->setDefaultAngularDamping(defaultAngularDamping);

        return this->ogreNewt;
    }

    OgreNewt::World* OgreNewtModule::createPerformantPhysics(const Ogre::String& name, Ogre::Real updateRate)
    {
        if (nullptr != this->ogreNewt)
        {
            this->destroyContent();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Initializing OgreNewt (performant)");
        this->ogreNewt = new OgreNewt::World(updateRate, 5, name);

        // Attention: Size the Newton worker pool first and only once. See createPhysics() for the full reason:
        // ndThreadPool::SetCount() rebuilds the whole worker array, and rebuilding it while freshly spawned
        // worker threads have not been scheduled yet is what produced the crash in ndThread::'vcall'.
        const int usedThreadCount = computePhysicsThreadCount(0);
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(usedThreadCount) + " cores for physics simulation");
        this->ogreNewt->setThreadCount(usedThreadCount);

        this->ogreNewt->setSolverModel(3);
        this->ogreNewt->setDefaultLinearDamping(0.1f);
        this->ogreNewt->setDefaultAngularDamping(Ogre::Vector3(0.1f, 0.1f, 0.1f));

        return this->ogreNewt;
    }

    OgreNewt::World* OgreNewtModule::createQualityPhysics(const Ogre::String& name, Ogre::Real updateRate)
    {
        if (nullptr != this->ogreNewt)
        {
            this->destroyContent();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Initializing OgreNewt (quality)");

        this->ogreNewt = new OgreNewt::World(updateRate, 5, name);

        // Attention: Size the Newton worker pool first and only once. See createPhysics() for the full reason.
        // Quality physics runs deterministically on a single thread on purpose, so the requested count is 1.
        // The previous code logged the core count but then always set 1, which was misleading in the log.
        const int usedThreadCount = 1;
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(usedThreadCount) + " cores for simulation (quality mode is deterministic and therefore single threaded)");
        this->ogreNewt->setThreadCount(usedThreadCount);

        this->ogreNewt->setSolverModel(4);
        this->ogreNewt->setDefaultLinearDamping(0.1f);
        this->ogreNewt->setDefaultAngularDamping(Ogre::Vector3(0.1f, 0.1f, 0.1f));

        return this->ogreNewt;
    }

    OgreNewt::World* OgreNewtModule::getOgreNewt(void) const
    {
        return this->ogreNewt;
    }

    void OgreNewtModule::enableOgreNewtCollisionLines(Ogre::SceneManager* sceneManager, bool showText)
    {
        this->showText = showText;
        if (!this->ogreNewt)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreNewtModule] Collision lines cannot be enabled, because OgreNewt has not been initialized.");
            return;
        }
        OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();
        debug.init(sceneManager, this->showText);
    }

    void OgreNewtModule::destroyContent(void)
    {
        // Is on logic main thread
        if (nullptr != this->ogreNewt)
        {
            // Attention: ~World() runs Sync() and CleanUp(), which joins all Newton worker threads and pushes
            // every Newton allocation back into the process wide ndFreeListAlloc pool. The pool is flushed again
            // as the first statement of the next World constructor (ClearCache()), so a following createPhysics()
            // never builds its thread pool out of recycled blocks of this world.
            delete this->ogreNewt;
            this->ogreNewt = nullptr;
        }
    }

    void OgreNewtModule::showOgreNewtCollisionLines(bool enabled)
    {
        if (this->ogreNewt)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, enabled]()
            {
                OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();
                if (enabled)
                {
                    debug.showDebugInformation();
                    debug.startRaycastRecording();
                    debug.clearRaycastsRecorded();
                }
                else
                {
                    debug.hideDebugInformation();
                    debug.clearRaycastsRecorded();
                    debug.stopRaycastRecording();
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MainMenuBar::showOgreNewtCollisionLines");
        }
    }

    void OgreNewtModule::setMaterialIdForDebugger(const OgreNewt::MaterialID* material, const Ogre::ColourValue& colour)
    {
        if (this->ogreNewt)
        {
            OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();

            // Predefined color palette with 31 distinct high-contrast colors
            static const std::vector<Ogre::ColourValue> colorPalette = {
                Ogre::ColourValue(1.0f, 0.0f, 0.0f), // 1 - Red
                Ogre::ColourValue(0.0f, 1.0f, 0.0f), // 2 - Green
                Ogre::ColourValue(0.0f, 0.0f, 1.0f), // 3 - Blue
                Ogre::ColourValue(1.0f, 1.0f, 0.0f), // 4 - Yellow
                Ogre::ColourValue(1.0f, 0.5f, 0.0f), // 5 - Orange
                Ogre::ColourValue(0.5f, 0.0f, 0.5f), // 6 - Violet
                Ogre::ColourValue(0.0f, 1.0f, 1.0f), // 7 - Cyan
                Ogre::ColourValue(1.0f, 0.0f, 1.0f), // 8 - Magenta
                Ogre::ColourValue(0.5f, 0.5f, 0.5f), // 9 - Grey
                Ogre::ColourValue(0.3f, 0.3f, 0.3f), // 10 - Dark Grey
                Ogre::ColourValue(0.8f, 0.3f, 0.1f), // 11 - Rust
                Ogre::ColourValue(0.2f, 0.7f, 0.3f), // 12 - Leaf Green
                Ogre::ColourValue(0.1f, 0.3f, 0.8f), // 13 - Deep Blue
                Ogre::ColourValue(0.9f, 0.2f, 0.5f), // 14 - Pink
                Ogre::ColourValue(0.6f, 0.4f, 0.2f), // 15 - Brown
                Ogre::ColourValue(0.7f, 0.8f, 0.3f), // 16 - Lime
                Ogre::ColourValue(0.3f, 0.6f, 0.9f), // 17 - Sky Blue
                Ogre::ColourValue(0.6f, 0.2f, 0.8f), // 18 - Purple
                Ogre::ColourValue(0.2f, 0.9f, 0.8f), // 19 - Turquoise
                Ogre::ColourValue(0.9f, 0.8f, 0.2f), // 20 - Gold
                Ogre::ColourValue(0.4f, 0.7f, 0.4f), // 21 - Forest Green
                Ogre::ColourValue(0.7f, 0.2f, 0.4f), // 22 - Raspberry
                Ogre::ColourValue(0.2f, 0.2f, 0.7f), // 23 - Navy
                Ogre::ColourValue(0.7f, 0.7f, 0.9f), // 24 - Light Lavender
                Ogre::ColourValue(0.9f, 0.6f, 0.2f), // 25 - Amber
                Ogre::ColourValue(0.6f, 0.9f, 0.2f), // 26 - Spring Green
                Ogre::ColourValue(0.2f, 0.9f, 0.6f), // 27 - Seafoam
                Ogre::ColourValue(0.5f, 0.1f, 0.7f), // 28 - Indigo
                Ogre::ColourValue(0.3f, 0.8f, 0.5f), // 29 - Mint
                Ogre::ColourValue(0.9f, 0.3f, 0.7f), // 30 - Rose
                Ogre::ColourValue(0.7f, 0.9f, 0.7f)  // 31 - Pale Green
            };

            // Static index to ensure unique color assignment
            static size_t colorIndex = 0;

            Ogre::ColourValue tempColour = colorPalette[colorIndex];

            // Cycle through colors
            colorIndex = (colorIndex + 1) % colorPalette.size();

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, &debug, material, tempColour]()
            {
                debug.setMaterialColor(material, tempColour);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MainMenuBar::setMaterialIdForDebugger");
        }
    }

    void OgreNewtModule::update(Ogre::Real dt)
    {
        if (nullptr != this->ogreNewt)
        {
            this->ogreNewt->update(dt);
            // this->ogreNewt->updateFixed(dt);
        }
    }

    void OgreNewtModule::setGlobalGravity(const Ogre::Vector3& globalGravity)
    {
        this->globalGravity = globalGravity;
    }

    Ogre::Vector3 OgreNewtModule::getGlobalGravity(void) const
    {
        return this->globalGravity;
    }

    void OgreNewtModule::registerRenderCallbackForBody(OgreNewt::Body* body)
    {
        if (nullptr == body)
        {
            return;
        }

        body->setRenderUpdateCallback(
            [](Ogre::SceneNode* node, const Ogre::Vector3& pos, const Ogre::Quaternion& rot, bool updateRot, bool updateStatic, bool isTeleport)
            {
                if (nullptr == node || !node->getParent())
                {
                    return;
                }

                // Important cases and most performant:
                // 1. In OgreNewt::Body setPositionOrientation is called for a non dynamic body then updateStatic is true
                // 2. In OgreNewt::Body setPositionOrientation is called for a dynamic body then isTeleport is true
                if (true == updateStatic || true == isTeleport)
                {
                    // Static path unchanged
                    Ogre::Node* parent = node->getParent();
                    const Ogre::Vector3 localPos = (parent->_getDerivedOrientationUpdated().Inverse() * (pos - parent->_getDerivedPositionUpdated())) / parent->_getDerivedScaleUpdated();
                    const Ogre::Quaternion localRot = parent->_getDerivedOrientationUpdated().Inverse() * rot;
                    NOWA::GraphicsModule::getInstance()->setNodePosition(node, localPos, false);
                    NOWA::GraphicsModule::getInstance()->setNodeOrientation(node, localRot, false);
                }
                // 3. In OgreNewt::Body movecallback transformcallback, force, velocity, etc. is called which only runs on a dynamic body, so this case is called
                else if (!node->isStatic())
                {
                    if (true == isTeleport)
                    {
                        // Dynamic node, but explicit desired teleport (Respawn, editor drag). So no interpolation
                        NOWA::GraphicsModule::getInstance()->teleportNodePosition(node, pos, false);
                        if (updateRot)
                        {
                            NOWA::GraphicsModule::getInstance()->teleportNodeOrientation(node, rot);
                        }
                    }
                    else
                    {
                        NOWA::GraphicsModule::getInstance()->updateNodePosition(node, pos, true);
                        if (updateRot)
                        {
                            NOWA::GraphicsModule::getInstance()->updateNodeOrientation(node, rot, true);
                        }
                    }
                }
            });
    }
} // namespace end