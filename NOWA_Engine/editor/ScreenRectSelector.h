#ifndef NOWA_SCREEN_RECT_SELECTOR_H
#define NOWA_SCREEN_RECT_SELECTOR_H
// =============================================================================
//  MeshSelector.h
//
//  Generic screen-space rectangle selection system.
//  Completely independent of MeshEditComponent — usable for any element type.
//
//  DESIGN
//  ──────
//  IScreenRectSelectable  — pure interface you implement for your element type
//  ScreenRectSelector     — manages drag state + SelectionRectangle visual,
//                           calls through the interface
//  floodSelect()          — free function for linked/connected selection
// =============================================================================

#include "defines.h"
#include "OgreCamera.h"
#include "OgreSceneManager.h"
#include "SelectionRectangle.h"

#include <cstddef>
#include <functional>
#include <vector>
#include <queue>

namespace NOWA
{

    // =============================================================================
    //  IScreenRectSelectable
    //
    //  Implement this for whatever you want to rectangle-select.
    //  MeshEditComponent will have three concrete implementations — one per mode.
    // =============================================================================
    class EXPORTED IScreenRectSelectable
    {
    public:
        virtual ~IScreenRectSelectable() = default;

        // Total number of candidate elements (vertices / edges / faces)
        virtual size_t getElementCount() const = 0;

        // Project element idx to normalised screen space [0,1]x[0,1].
        // Returns false if behind camera or otherwise un-projectable.
        virtual bool projectElement(size_t idx, float& outNX, float& outNY) const = 0;

        // Called once with all indices whose projection lies inside the rectangle.
        // add=true   -> union with current selection (Shift held)
        // remove=true-> subtract from current selection (Ctrl held)
        // both false -> replace selection
        virtual void applyRectSelection(const std::vector<size_t>& indices, bool add, bool remove) = 0;
    };

    // =============================================================================
    //  ScreenRectSelector
    //
    //  Drag-state machine + SelectionRectangle visual.
    //  Call onMousePress / onMouseMove / onMouseRelease from your input handlers.
    //  onMouseRelease calls IScreenRectSelectable::applyRectSelection.
    // =============================================================================
    class EXPORTED ScreenRectSelector
    {
    public:
        ScreenRectSelector();
        ~ScreenRectSelector();

        // Call once before use.  Allocates the SelectionRectangle on render thread.
        void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera);

        // Call once on cleanup (or when the owning component is removed).
        void destroy();

        // ── Input ─────────────────────────────────────────────────────────────────
        // Call from your mousePressed handler when you want to begin rect-select.
        void beginDrag(float nx, float ny);

        // Call from mouseMoved while isDragging().  Updates the visual rectangle.
        void updateDrag(float nx, float ny);

        // Call from mouseReleased.  Iterates all elements, tests them, calls
        // IScreenRectSelectable::applyRectSelection, hides visual, resets state.
        // Minimum area threshold (default 0.0001) prevents accidental tiny rects.
        void endDrag(IScreenRectSelectable& target, bool add, bool remove, float minArea = 0.0001f);

        // Cancel without selecting anything (Esc etc.)
        void cancelDrag();

        bool isDragging() const
        {
            return this->dragging;
        }

        // Convenience: is (nx, ny) inside the current drag rect?
        bool isInsideRect(float nx, float ny) const;

        // Get normalised corners (min/max automatically sorted)
        void getRect(float& outX1, float& outY1, float& outX2, float& outY2) const;

    private:
        SelectionRectangle* selRect;
        Ogre::SceneNode* selNode;
        Ogre::SceneManager* sceneManager;

        Ogre::Vector2 start;
        Ogre::Vector2 current;
        bool dragging;
    };

    // =============================================================================
    //  floodSelect — generic connected / linked selection
    //
    //  Starting from startIdx, performs a BFS expanding through element adjacency
    //  until all connected elements are reached.  Returns all found indices.
    //
    //  getNeighbors(idx) -> vector of adjacent element indices
    //
    //  Typical use:
    //    VERTEX mode  — getNeighbors returns vertexNeighbors[idx]
    //    EDGE mode    — getNeighbors returns edges sharing a vertex with this edge
    //    FACE mode    — getNeighbors returns faces sharing an edge with this face
    // =============================================================================
    // MeshSelector.h — ersetze die Deklaration durch die vollständige Definition

    inline std::vector<size_t> floodSelect(size_t startIdx, const std::function<std::vector<size_t>(size_t)>& getNeighbors)
    {
        std::vector<size_t> result;
        std::unordered_set<size_t> visited;
        std::queue<size_t> queue;

        visited.insert(startIdx);
        queue.push(startIdx);

        while (!queue.empty())
        {
            const size_t cur = queue.front();
            queue.pop();
            result.push_back(cur);

            for (size_t nb : getNeighbors(cur))
            {
                if (visited.insert(nb).second)
                {
                    queue.push(nb);
                }
            }
        }
        return result;
    }

} // namespace NOWA

#endif