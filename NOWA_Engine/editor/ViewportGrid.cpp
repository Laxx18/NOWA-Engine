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

#include "NOWAPrecompiled.h"

#include "ViewportGrid.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
 
namespace NOWA
{
	ViewportGrid::ViewportGrid(Ogre::SceneManager* sceneManager, Ogre::Camera* camera)
		: sceneManager(sceneManager),
		camera(camera),
		enabled(false),
		prevCamera(nullptr),
		prevOrtho(false),
		prevNear(0.0f),
		prevFOVy(0.0f),
		prevAspectRatio(0.0f),
		forceUpdate(true),
		grid(nullptr),
		node(nullptr),
		color1(0.7f, 0.7f, 0.7f),
		color2(0.7f, 0.7f, 0.7f),
		division(10),
		perspSize(1000.0f),
		renderScale(true),
		renderMiniAxes(true)
	{
		assert(this->sceneManager);
		
		createGrid();
		setRenderLayer(RL_BEHIND);
	}

	ViewportGrid::~ViewportGrid()
	{
		if (this->grid != nullptr)
		{
			auto gridLocal = this->grid;
			this->grid = nullptr;
			auto sceneManager = this->sceneManager;

			ENQUEUE_DESTROY_COMMAND("ViewportGrid::DestroyManualObject", _2(sceneManager, gridLocal),
			{
				sceneManager->destroyManualObject(gridLocal);
			});
		}

		if (this->node != nullptr)
		{
			auto nodeLocal = this->node;

			auto sceneManager = this->sceneManager;
			NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->node);
			this->node = nullptr;

			ENQUEUE_DESTROY_COMMAND("ViewportGrid::DestroySceneNode", _2(sceneManager, nodeLocal),
			{
				sceneManager->destroySceneNode(nodeLocal);
			});
		}
	}


	/** Sets the colour of the major grid lines (the minor lines are alpha-faded out/in when zooming out/in)
		@note The alpha value is automatically set to one
	*/
	void ViewportGrid::setColor(const Ogre::ColourValue& color)
	{
		// Force alpha = 1 for the primary colour
		color1 = color;
		color1.a = 1.0f;
		color2 = color1;
		this->applyForceUpdate();
	}

	/** Sets in how many lines a grid has to be divided when zoomed in.
		Defaults to 10.
	*/
	void ViewportGrid::setDivision(unsigned int division)
	{
		this->division = division;
		this->applyForceUpdate();
	}

	void ViewportGrid::setPosition(const Ogre::Vector3& position)
	{
		NOWA::GraphicsModule::getInstance()->updateNodePosition(this->node, position, false);
		// this->applyForceUpdate();
	}

	Ogre::Vector3 ViewportGrid::getPosition() const
	{
		return this->node->getPosition();
	}

	void ViewportGrid::setOrientation(const Ogre::Quaternion& orientation)
	{
		NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->node, orientation, false);
		// this->applyForceUpdate();
	}

	Ogre::Quaternion ViewportGrid::getOrientation() const
	{
		return this->node->getOrientation();
	}

	/** Sets the render layer of the grid
		@note Ignored in perspective view.
		@see Ogre::ViewportGrid::RenderLayer
	*/
	void ViewportGrid::setRenderLayer(RenderLayer layer)
	{
		this->layer = layer;

		//switch (layer)
		//{
		//default:
		//case RL_BEHIND:
		//	// Render just before the world geometry
		//	this->grid->setRenderQueueGroup(120 - 1);
		//	break;

		//case RL_INFRONT:
		//	// Render just before the overlays
		//	this->grid->setRenderQueueGroup(100 - 1);
		//	break;
		//}
	}

	/** Sets the size of the grid in perspective view.
		Defaults to 100 units.
		@note Ignored in orthographic view.
	*/
	void ViewportGrid::setPerspectiveSize(Ogre::Real size)
	{
		this->perspSize = size;
		this->applyForceUpdate();
	}

	/** Sets whether to render scaling info in an overlay.
		This looks a bit like the typical scaling info on a map.
	*/
	void ViewportGrid::setRenderScale(bool enabled)
	{
		this->renderScale = enabled;
		this->applyForceUpdate();
	}

	/** Sets whether to render mini-axes in an overlay.
	*/
	void ViewportGrid::setRenderMiniAxes(bool enabled)
	{
		this->renderMiniAxes = enabled;
		this->applyForceUpdate();
	}

	bool ViewportGrid::isEnabled() const
	{
		return this->enabled;
	}

	void ViewportGrid::enable()
	{
		this->enabled = true;

		auto nodeLocal = this->node;
		auto gridLocal = this->grid;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ViewportGrid::enable", _2(nodeLocal, gridLocal),
		{
			if (!gridLocal->isAttached())
			{
				nodeLocal->attachObject(gridLocal);
			}
		});

		this->applyForceUpdate();
	}

	void ViewportGrid::disable()
	{
		this->enabled = false;

		auto nodeLocal = this->node;
		auto gridLocal = this->grid;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ViewportGrid::disable", _2(nodeLocal, gridLocal),
		{
			if (gridLocal->isAttached())
			{
				nodeLocal->detachObject(gridLocal);
			}
		});
	}


	void ViewportGrid::toggle()
	{
		this->setEnabled(!this->enabled);
	}

	void ViewportGrid::setEnabled(bool enabled)
	{
		this->enabled = enabled;
		if (enabled)
		{
			enable();
		}
		else
		{
			disable();
		}
	}

	bool ViewportGrid::getEnabled(void) const
	{
		return this->enabled;
	}

	void ViewportGrid::update(void)
	{
		if (this->enabled)
		{
			this->intenralUpdate();
		}
	}

	/****************************
	* Other protected functions* 
	****************************/

	void ViewportGrid::createGrid()
	{
		ENQUEUE_RENDER_COMMAND("ViewportGrid::createGrid",
		{
			Ogre::String name = "ViewportGrid";

			// Create the manual object
			// this->grid = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
			this->grid = this->sceneManager->createManualObject(Ogre::SCENE_DYNAMIC);
			this->grid->setName(name);
			this->grid->setQueryFlags(0 << 0);
			this->grid->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);

			// Really important: When an unlit datablock is used for manual object, no shadows must be cast!! else there is an error in an dx script with 'depthRange'!
			this->grid->setCastShadows(false);

			// Create the scene node (not attached yet)
			this->node = this->sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->node->setName(name);
			this->node->attachObject(this->grid);
			this->enabled = false;
		});
	}

	void ViewportGrid::intenralUpdate()
	{
		if (!enabled) return;

		// Check if an update is necessary
		if (!checkUpdate() && !forceUpdate)
		{
			return;
		}
		if (this->camera->getProjectionType() == Ogre::PT_ORTHOGRAPHIC)
		{
			updateOrtho();
		}
		else
		{
			updatePersp();
		}

		forceUpdate = false;
	}

	void ViewportGrid::updateOrtho()
	{
		auto closureFunction = [this](Ogre::Real renderDt)
		{
			// Screen dimensions
			int width = Core::getSingletonPtr()->getOgreRenderWindow()->getWidth();
			int height = Core::getSingletonPtr()->getOgreRenderWindow()->getHeight();

			// Camera information
			const Ogre::Vector3& camPos = this->camera->getPosition();
			Ogre::Vector3 camDir = this->camera->getDirection();
			Ogre::Vector3 camUp = this->camera->getUp();
			Ogre::Vector3 camRight = this->camera->getRight();

			// Translation in grid space
			Ogre::Real dx = camPos.dotProduct(camRight);
			Ogre::Real dy = camPos.dotProduct(camUp);

			// Frustum dimensions
			// Note: Tan calculates the opposite side of a _right_ triangle given its angle, so we make sure it is one, and double the result
			Ogre::Real worldWidth = 2.0f * Ogre::Math::Tan(this->camera->getFOVy() / 2) * this->camera->getAspectRatio() * this->camera->getNearClipDistance();
			Ogre::Real worldHeight = worldWidth / this->camera->getAspectRatio();
			Ogre::Real worldLeft = dx - worldWidth / 2;
			Ogre::Real worldRight = dx + worldWidth / 2;
			Ogre::Real worldBottom = dy - worldHeight / 2;
			Ogre::Real worldTop = dy + worldHeight / 2;

			// Conversion values (note: same as working with the height values)
			Ogre::Real worldToScreen = width / worldWidth;
			Ogre::Real screenToWorld = worldWidth / width;

			//! @todo Treshold should be dependent on window width/height (min? max?) so there are no more then division full alpha-lines
			static const int treshold = 10; // Treshhold in pixels

			// Calculate the spacing multiplier
			Ogre::Real mult = 0;
			int exp = 0;
			Ogre::Real temp = worldToScreen; // 1 world unit
			if (worldToScreen < treshold)
			{
				while (temp < treshold)
				{
					++exp;
					temp *= treshold;
				}

				mult = Ogre::Math::Pow(static_cast<Ogre::Real>(division), static_cast<Ogre::Real>(exp));
			}
			else
			{
				while (temp > division * treshold)
				{
					++exp;
					temp /= treshold;
				}

				mult = Ogre::Math::Pow(1.0f / static_cast<Ogre::Real>(division), static_cast<Ogre::Real>(exp));
			}

			// Interpolate alpha for (multiplied) spacing between treshold and division*  treshold
			color2.a = worldToScreen * mult / (division * treshold - treshold);
			if (color2.a > 1.0f)
			{
				color2.a = 1.0f;
			}
			// Calculate the horizontal zero-axis color
			Ogre::Real camRightX = Ogre::Math::Abs(camRight.x);
			Ogre::Real camRightY = Ogre::Math::Abs(camRight.y);
			Ogre::Real camRightZ = Ogre::Math::Abs(camRight.z);
			const Ogre::ColourValue& horAxisColor = Ogre::Math::RealEqual(camRightX, 1.0f) ? Ogre::ColourValue::Red
				: Ogre::Math::RealEqual(camRightY, 1.0f) ? Ogre::ColourValue::Green
				: Ogre::Math::RealEqual(camRightZ, 1.0f) ? Ogre::ColourValue::Blue : color1;

			// Calculate the vertical zero-axis color
			Ogre::Real camUpX = Ogre::Math::Abs(camUp.x);
			Ogre::Real camUpY = Ogre::Math::Abs(camUp.y);
			Ogre::Real camUpZ = Ogre::Math::Abs(camUp.z);
			const Ogre::ColourValue& vertAxisColor = Ogre::Math::RealEqual(camUpX, 1.0f) ? Ogre::ColourValue::Red
				: Ogre::Math::RealEqual(camUpY, 1.0f) ? Ogre::ColourValue::Green
				: Ogre::Math::RealEqual(camUpZ, 1.0f) ? Ogre::ColourValue::Blue : color1;

			// The number of lines
			int numLinesWidth = (int)(worldWidth / mult) + 1;
			int numLinesHeight = (int)(worldHeight / mult) + 1;

			// Start creating or updating the grid
			this->grid->estimateVertexCount(2 * numLinesWidth + 2 * numLinesHeight);
			// this->grid->clear();

			// Start updating the manual object
			if (this->grid && this->grid->getNumSections() > 0)
			{
				this->grid->beginUpdate(0);
			}
			else
			{
				this->grid->begin("GreenNoLighting", Ogre::OperationType::OT_LINE_LIST);
			}

			// Vertical lines
			Ogre::Real startX = mult * (int)(worldLeft / mult);
			Ogre::Real x = startX;
			int i = 0;
			while (x <= worldRight)
			{
				// Get the right color for this line
				int multX = static_cast<int>((x == 0.0f) ? x : (x < 0.0f) ? (int)(x / mult - 0.5f) : (int)(x / mult + 0.5f));
				const Ogre::ColourValue& colour = (multX == 0.0f) ? vertAxisColor : (multX % (int)division) ? color2 : color1;

				// Add the line
				this->grid->position(x, worldBottom, 0);
				this->grid->colour(colour);
				this->grid->index(i);
				this->grid->position(x, worldTop, 0);
				this->grid->colour(colour);
				this->grid->index(i + 1);

				x += mult;
				i += 2;
			}

			// Horizontal lines
			Ogre::Real startY = mult * (int)(worldBottom / mult);
			Ogre::Real y = startY;
			while (y <= worldTop)
			{
				// Get the right color for this line
				int multY = static_cast<int>((y == 0.0f) ? y : (y < 0.0f) ? (int)(y / mult - 0.5f) : (int)(y / mult + 0.5f));
				const Ogre::ColourValue& colour = (multY == 0.0f) ? horAxisColor : (multY % (int)division) ? color2 : color1;

				// Add the line
				this->grid->position(worldLeft, y, 0.0f);
				this->grid->colour(colour);
				this->grid->index(i);
				this->grid->position(worldRight, y, 0.0f);
				this->grid->colour(colour);
				this->grid->index(i + 1);

				y += mult;
				i += 2;
			}

			this->grid->end();

			this->node->setOrientation(this->camera->getOrientation());
		};
		Ogre::String id = "ViewportGrid::updateOrtho";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
	}

	void ViewportGrid::updatePersp()
	{
		auto closureFunction = [this](Ogre::Real renderDt)
		{
			//! @todo Calculate the spacing multiplier
			Ogre::Real mult = 1;

			//! @todo Interpolate alpha
			color2.a = 0.5f;
			//if(colour2.a > 1.0f) colour2.a = 1.0f;

			// Calculate the horizontal zero-axis color
			const Ogre::ColourValue & horAxisColor = Ogre::ColourValue::Red;

			// Calculate the vertical zero-axis color
			const Ogre::ColourValue & vertAxisColor = Ogre::ColourValue::Blue;

			// The number of lines
			int numLines = (int)(this->perspSize / mult) + 1;

			// Start creating or updating the grid
			this->grid->estimateVertexCount(4 * numLines);
			// this->grid->clear();

			// Start updating the manual object
			if (this->grid && this->grid->getNumSections() > 0)
			{
				this->grid->beginUpdate(0);
			}
			else
			{
				this->grid->begin("GreenNoLighting", Ogre::OperationType::OT_LINE_LIST);
			}

			// Vertical lines
			Ogre::Real start = mult * (int)(-this->perspSize / 2 / mult);
			Ogre::Real x = start;

			int i = 0;
			while (x <= perspSize / 2.0f)
			{
				// Get the right color for this line
				int multX = static_cast<int>((x == 0.0f) ? x : (x < 0.0f) ? (int)(x / mult - 0.5f) : (int)(x / mult + 0.5f));
				const Ogre::ColourValue& colour = (multX == 0.0f) ? vertAxisColor : (multX % (int)division) ? color2 : color1;

				// Add the line
				this->grid->position(x, 0, -this->perspSize / 2.0f);
				this->grid->colour(colour);
				this->grid->index(i);
				this->grid->position(x, 0, this->perspSize / 2.0f);
				this->grid->colour(colour);
				this->grid->index(i + 1);

				x += mult;
				i += 2;
			}

			// Horizontal lines
			Ogre::Real y = start;
			while (y <= this->perspSize / 2.0f)
			{
				// Get the right color for this line
				int multY = static_cast<int>((y == 0.0f) ? y : (y < 0.0f) ? (int)(y / mult - 0.5f) : (int)(y / mult + 0.5f));
				const Ogre::ColourValue& colour = (multY == 0.0f) ? horAxisColor : (multY % (int)division) ? color2 : color1;

				// Add the line
				this->grid->position(-this->perspSize / 2.0f, 0, y);
				this->grid->colour(colour);
				this->grid->index(i);
				this->grid->position(this->perspSize / 2.0f, 0, y);
				this->grid->colour(colour);
				this->grid->index(i + 1);

				y += mult;
				i += 2;
			}

			this->grid->end();

			// Normal orientation, grid in the X-Z plane
			// this->node->resetOrientation();
		};
		Ogre::String id = "ViewportGrid::updatePersp";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
	}

	/* Checks if an update is necessary*/
	bool ViewportGrid::checkUpdate()
	{
		bool update = false;

		if (this->camera != this->prevCamera)
		{
			this->prevCamera = this->camera;
			update = true;
		}

		bool ortho = (this->camera->getProjectionType() == Ogre::PT_ORTHOGRAPHIC);
		if (ortho != prevOrtho)
		{
			prevOrtho = ortho;
			update = true;

			//Ogre::HlmsDatablock* dataBlock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HlmsTypes::HLMS_UNLIT)->getDatablock("RedLine");

			//// Set correct material properties
			//if (nullptr != dataBlock)
			//{
			//	dataBlock->getMacroblock()->mDepthWrite = (false == ortho);
			//	dataBlock->getMacroblock()->mDepthCheck = (false == ortho);
			//}
		}

		return update || ortho ? checkUpdateOrtho() : checkUpdatePersp();
	}

	bool ViewportGrid::checkUpdateOrtho()
	{
		bool update = false;

		if (!this->camera) return false;

		if (this->camera->getPosition() != prevCamPos)
		{
			prevCamPos = this->camera->getPosition();
			update = true;
		}

		if (this->camera->getNearClipDistance() != prevNear)
		{
			prevNear = this->camera->getNearClipDistance();
			update = true;
		}

		if (this->camera->getFOVy() != prevFOVy)
		{
			prevFOVy = this->camera->getFOVy();
			update = true;
		}

		if (this->camera->getAspectRatio() != prevAspectRatio)
		{
			prevAspectRatio = this->camera->getAspectRatio();
			update = true;
		}

		return update;
	}

	bool ViewportGrid::checkUpdatePersp()
	{
		//! @todo Add a check if grid line splitting/merging is implemented
		return false;
	}

} // namespace end NOWA