#include "NOWAPrecompiled.h"

// =============================================================================
//  MeshSelector.cpp
// =============================================================================

#include "NOWAPrecompiled.h"
#include "ScreenRectSelector.h"
#include "modules/GraphicsModule.h"

#include <algorithm>
#include <queue>
#include <unordered_set>

namespace NOWA
{

    // =============================================================================
    //  ScreenRectSelector
    // =============================================================================

    ScreenRectSelector::ScreenRectSelector()
        : selRect(nullptr),
        selNode(nullptr),
        sceneManager(nullptr),
        dragging(false)
    {
    }

    ScreenRectSelector::~ScreenRectSelector()
    {
        // destroy() must be called explicitly before the render window closes;
        // just guard against accidental destruction without cleanup.
        if (this->selRect || this->selNode)
        {
            this->destroy();
        }
    }

    void ScreenRectSelector::init(Ogre::SceneManager* sm, Ogre::Camera* cam)
    {
        this->sceneManager = sm;

        auto rectCopy = &this->selRect;
        auto nodeCopy = &this->selNode;

        NOWA::GraphicsModule::RenderCommand cmd = [this, sm, cam, rectCopy, nodeCopy]()
        {
            *rectCopy = new SelectionRectangle("MeshEditSelRect_" + Ogre::StringConverter::toString(reinterpret_cast<uintptr_t>(this)), sm, cam);
            *nodeCopy = sm->getRootSceneNode()->createChildSceneNode();
            (*nodeCopy)->attachObject(*rectCopy);
            (*rectCopy)->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ScreenRectSelector::init");
    }

    void ScreenRectSelector::destroy()
    {
        if (!this->selRect && !this->selNode)
        {
            return;
        }

        // Copy raw pointers — never capture 'this' in async commands
        auto rectCopy = this->selRect;
        auto nodeCopy = this->selNode;
        auto smCopy = this->sceneManager;

        this->selRect = nullptr;
        this->selNode = nullptr;
        this->sceneManager = nullptr;

        NOWA::GraphicsModule::RenderCommand cmd = [rectCopy, nodeCopy, smCopy]()
        {
            if (rectCopy && nodeCopy)
            {
                nodeCopy->detachObject(rectCopy);
                delete rectCopy;
            }
            if (smCopy && nodeCopy)
            {
                smCopy->getRootSceneNode()->removeAndDestroyChild(nodeCopy);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ScreenRectSelector::destroy");
    }

    void ScreenRectSelector::beginDrag(float nx, float ny)
    {
        this->start = Ogre::Vector2(nx, ny);
        this->current = Ogre::Vector2(nx, ny);
        this->dragging = true;
    }

    void ScreenRectSelector::updateDrag(float nx, float ny)
    {
        if (!this->dragging || !this->selRect)
        {
            return;
        }

        this->current = Ogre::Vector2(nx, ny);

        const float x1 = std::min(this->start.x, this->current.x);
        const float y1 = std::min(this->start.y, this->current.y);
        const float x2 = std::max(this->start.x, this->current.x);
        const float y2 = std::max(this->start.y, this->current.y);

        // selectionRect runs on render thread but SelectionRectangle uses
        // ENQUEUE_RENDER_COMMAND internally, so this call is safe from main thread.
        this->selRect->setCorners(x1, y1, x2, y2);

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (this->selRect)
            {
                this->selRect->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ScreenRectSelector::showRect");
    }

    void ScreenRectSelector::endDrag(IScreenRectSelectable& target, bool add, bool remove, float minArea)
    {
        if (!this->dragging)
        {
            return;
        }

        // Hide visual
        NOWA::GraphicsModule::RenderCommand hideCmd = [this]()
        {
            if (this->selRect)
            {
                this->selRect->setVisible(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(hideCmd), "ScreenRectSelector::hideRect");

        this->dragging = false;

        // Normalised rect — sorted so x1<=x2, y1<=y2
        float x1, y1, x2, y2;
        this->getRect(x1, y1, x2, y2);

        // Reject tiny accidental drags
        if ((x2 - x1) * (y2 - y1) < minArea)
        {
            return;
        }

        // Test every element — entirely on main thread, safe
        const size_t n = target.getElementCount();
        std::vector<size_t> hits;
        hits.reserve(n / 4);

        for (size_t i = 0; i < n; ++i)
        {
            float nx = 0.f, ny = 0.f;
            if (!target.projectElement(i, nx, ny))
            {
                continue;
            }
            if (nx >= x1 && nx <= x2 && ny >= y1 && ny <= y2)
            {
                hits.push_back(i);
            }
        }

        if (!hits.empty())
        {
            target.applyRectSelection(hits, add, remove);
        }
    }

    void ScreenRectSelector::cancelDrag()
    {
        this->dragging = false;
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (this->selRect)
            {
                this->selRect->setVisible(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ScreenRectSelector::cancelDrag");
    }

    bool ScreenRectSelector::isInsideRect(float nx, float ny) const
    {
        float x1, y1, x2, y2;
        this->getRect(x1, y1, x2, y2);
        return nx >= x1 && nx <= x2 && ny >= y1 && ny <= y2;
    }

    void ScreenRectSelector::getRect(float& x1, float& y1, float& x2, float& y2) const
    {
        x1 = std::min(this->start.x, this->current.x);
        y1 = std::min(this->start.y, this->current.y);
        x2 = std::max(this->start.x, this->current.x);
        y2 = std::max(this->start.y, this->current.y);
    }

} // namespace NOWA