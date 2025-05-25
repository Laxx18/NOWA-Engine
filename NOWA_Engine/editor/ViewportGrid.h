

/*
-----------------------------------------------------------------------------
This source file is supposed to be used with OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/
 
Copyright (c) 2007 Jeroen Dierckx
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef OGRE_VIEWPORTGRID_H
#define OGRE_VIEWPORTGRID_H

// Must be ported, ManualObject, viewport, renderqueue will not work
 
// Includes
// #include "OgreRenderTargetlistener.h"
#include "defines.h"
 
namespace NOWA
{
	/** Use this class to render a dynamically adjusting grid inside an Ogre viewport.
		@todo
		Make the grid work with an arbitrary rotated orthogonal camera (e.g. for working in object space mode).
		Or should the grid be rendered in object space too then?
	*/
	class EXPORTED ViewportGrid/* : public Ogre::RenderTargetListener*/
	{
	public:
		//! The render layer of the grid in orthogonal view
		enum RenderLayer
		{
			RL_BEHIND, //!< Behind all objects
			RL_INFRONT //!< In front of all objects
		};
	public:
		// Constructors and destructor
		ViewportGrid(Ogre::SceneManager* sceneManager, Ogre::Camera* camera);
		virtual ~ViewportGrid();
 
		//! Grid colour
		const Ogre::ColourValue& getColor() const { return this->color1; }
		void setColor(const Ogre::ColourValue& color);
 
		//! Grid division (the number of new lines that are created when zooming in).
		unsigned int getDivision() const { return this->division; }
		void setDivision(unsigned int division);

		//! Set the grid position
		void setPosition(const Ogre::Vector3& position);
		Ogre::Vector3 getPosition() const;

		//! Set the grid orientation
		void setOrientation(const Ogre::Quaternion& orientation);
		Ogre::Quaternion getOrientation() const;

		//! Grid render layer (behind of in front of the objects).
		RenderLayer getRenderLayer() const { return this->layer; }
		void setRenderLayer(RenderLayer layer);
 
		//! Size of the grid in perspective view, that is the count of quadrads, e.g. 4 means 4x4 quadrads
		Ogre::Real getPerspectiveSize() const { return this->perspSize; }
		void setPerspectiveSize(Ogre::Real size);
 
		//! Render scaling info? Defaults to true.
		//! @todo Implement this
		bool getRenderScale() const { return this->renderScale; }
		void setRenderScale(bool enabled = true);
 
		//! Render mini axes? Defaults to true.
		//! @todo Implement this
		bool getRenderMiniAxes() const { return renderMiniAxes; }
		void setRenderMiniAxes(bool enabled = true);
 
		// Enable / disable
		bool isEnabled() const;
		void enable();
		void disable();
		void toggle();
		void setEnabled(bool enabled);
		bool getEnabled(void) const;

		void update(void);
 
		// Other functions
		void applyForceUpdate() { this->forceUpdate = true; }
 
	protected:
		// RenderTargetListener
		/*void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
		void postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);*/
 
		// Other protected functions
		void createGrid();
 
		void intenralUpdate();
		void updateOrtho();
		void updatePersp();
 
		bool checkUpdate();
		bool checkUpdateOrtho();
		bool checkUpdatePersp();
	protected:
		// Member variables
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		bool enabled;
		RenderLayer layer;

		Ogre::Camera* prevCamera;
		bool prevOrtho;
		Ogre::Vector3 prevCamPos;
		Ogre::Real prevNear;
		Ogre::Radian prevFOVy;
		Ogre::Real prevAspectRatio;
		bool forceUpdate;

		Ogre::ManualObject* grid;
		Ogre::SceneNode* node;

		Ogre::ColourValue color1;
		Ogre::ColourValue color2;
		unsigned int division;
		Ogre::Real perspSize;
		bool renderScale;
		bool renderMiniAxes;

	};
} // namespace end NOWA

#endif
