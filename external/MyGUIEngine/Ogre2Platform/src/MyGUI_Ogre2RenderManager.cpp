/*!
	@file
	@author		Albert Semenov
	@date		04/2008
	@update		2026 v2 VAO rewrite for Ogre-Next by NOWA-Engine
*/

#include "MyGUI_Ogre2DataManager.h"
#include "MyGUI_Ogre2RenderManager.h"
#include "MyGUI_Ogre2Texture.h"
#include "MyGUI_Ogre2VertexBuffer.h"
#include "MyGUI_Ogre2Diagnostic.h"
#include "MyGUI_Timer.h"
#include "MyGUI_Gui.h"

#include <Compositor/OgreCompositorManager2.h>
#include <OgreTextureGpu.h>
#include <OgreTextureGpuManager.h>
#include <OgreWindow.h>

#include <atomic>

namespace MyGUI
{
	// v2 fix: was 254, which collides with NOWA::RENDER_QUEUE_MAX / the v1
	// Ogre::Overlay queue (also default-registered at 254 by OverlayManager).
	// That queue must stay V1_FAST for v1 Overlays; MyGUI's VAO renderables
	// need FAST. Use a queue NOWA doesn't already claim — 253 sits between
	// RENDER_QUEUE_GIZMO (252) and RENDER_QUEUE_MAX (254), so painter's order
	// is: scene -> gizmos -> MyGUI -> v1 debug overlays.
	// If your engine's eRenderQueues enum ever claims 253, change this and
	// the corresponding gap in the enum together.
	const Ogre::uint8 Ogre2RenderManager::RENDER_QUEUE_OVERLAY = 253;

	MyGUIPass::MyGUIPass(const Ogre::CompositorPassDef* definition, const Ogre::RenderTargetViewDef* target,
		Ogre::CompositorNode* parentNode)
		: Ogre::CompositorPass(definition, parentNode)
	{
		this->initialize(target);
	}

	void MyGUIPass::execute(const Ogre::Camera* lodCameraconst)
	{
		static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->render();
	}

	Ogre2RenderManager& Ogre2RenderManager::getInstance()
	{
		return *getInstancePtr();
	}
	Ogre2RenderManager* Ogre2RenderManager::getInstancePtr()
	{
		return static_cast<Ogre2RenderManager*>(RenderManager::getInstancePtr());
	}

	Ogre2RenderManager::Ogre2RenderManager() :
		mUpdate(false),
		mSceneManager(nullptr),
		mWindow(nullptr),
		mRenderSystem(nullptr),
		mIsInitialise(false),
		mManualRender(false),
		mCountBatch(0)
	{
	}

	void Ogre2RenderManager::initialise(Ogre::Window* _window, Ogre::SceneManager* _scene)
	{
		MYGUI_PLATFORM_ASSERT(!mIsInitialise, getClassTypeName() << " initialised twice");
		MYGUI_PLATFORM_LOG(Info, "* Initialise: " << getClassTypeName());

		mPassProvider.reset(new OgreCompositorPassProvider());

		Ogre::CompositorManager2* pCompositorManager = Ogre::Root::getSingleton().getCompositorManager2();

		// don't overwrite a custom pass provider that the user may have registered already
		if (!pCompositorManager->getCompositorPassProvider())
		{
			pCompositorManager->setCompositorPassProvider(mPassProvider.get());
		}
		else
		{
			MYGUI_PLATFORM_LOG(Warning, "A custom pass provider is already installed." <<
				"MyGui passes will not work unless the registered provider can create MyGui passes");
		}

		mSceneManager = nullptr;
		mWindow = nullptr;
		mUpdate = false;
		mRenderSystem = nullptr;

		Ogre::Root* root = Ogre::Root::getSingletonPtr();
		if (root != nullptr)
			setRenderSystem(root->getRenderSystem());
		setRenderWindow(_window);
		setSceneManager(_scene);

		root->addFrameListener(this);

		mQueueMoveable = new Ogre2GuiMoveable(Ogre::Id::generateNewId<Ogre2GuiRenderable>(), &mDummyObjMemMgr, mSceneManager, RENDER_QUEUE_OVERLAY);
		mQueueSceneNode = new Ogre::SceneNode(0, 0, &mDummyNodeMemMgr, 0);
		mQueueSceneNode->_getFullTransformUpdated();
		mQueueSceneNode->attachObject(mQueueMoveable);

		MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully initialized");
		mIsInitialise = true;
	}

	void Ogre2RenderManager::shutdown()
	{
		MYGUI_PLATFORM_ASSERT(mIsInitialise, getClassTypeName() << " is not initialised");
		MYGUI_PLATFORM_LOG(Info, "* Shutdown: " << getClassTypeName());

		destroyAllResources();

		delete mQueueSceneNode;
		delete mQueueMoveable;

		setSceneManager(nullptr);
		setRenderWindow(nullptr);
		setRenderSystem(nullptr);

		Ogre::Root::getSingleton().removeFrameListener(this);

		Ogre::CompositorManager2* pCompositorManager = Ogre::Root::getSingleton().getCompositorManager2();
		if (pCompositorManager->getCompositorPassProvider() == mPassProvider.get())
			pCompositorManager->setCompositorPassProvider(NULL);

		mPassProvider.reset();

		MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully shutdown");
		mIsInitialise = false;
	}

	void Ogre2RenderManager::setRenderSystem(Ogre::RenderSystem* _render)
	{
		// отписываемся
		if (mRenderSystem != nullptr)
		{
			mRenderSystem->removeListener(this);
			mRenderSystem = nullptr;
		}

		mRenderSystem = _render;

		// подписываемся на рендер евент
		if (mRenderSystem != nullptr)
		{
			mRenderSystem->addListener(this);

			// v2: the vertex colour element is VET_UBYTE4_NORM, which the GPU
			// reads in memory byte order R,G,B,A on every render system. On
			// little-endian this equals a uint32 packed as A<<24|B<<16|G<<8|R,
			// i.e. MyGUI's ColourABGR — no per-rendersystem query needed.
			// (getColourVertexElementType() described the v1 VET_COLOUR
			// behavior and no longer applies.)
			mVertexFormat = VertexColourType::ColourABGR;

			updateRenderInfo();
		}
	}

	Ogre::RenderSystem* Ogre2RenderManager::getRenderSystem()
	{
		return mRenderSystem;
	}

	void Ogre2RenderManager::setRenderWindow(Ogre::Window* _window)
	{
		// отписываемся
		if (mWindow != nullptr)
		{
			Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
			mWindow = nullptr;
		}

		mWindow = _window;

		if (mWindow != nullptr)
		{
			Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);
			windowResized(mWindow);
		}
	}

	void Ogre2RenderManager::setSceneManager(Ogre::SceneManager* _scene)
	{
		if (nullptr != mSceneManager)
		{
			mSceneManager = nullptr;
		}

		mSceneManager = _scene;
		if (mSceneManager != nullptr)
		{
			// v2: queues >= 100 default to V1_FAST. Our renderables are VAO
			// based, so the overlay queue MUST run in FAST (v2) mode —
			// otherwise Ogre calls getRenderOperation() and throws.
			// NOTE: if anything else (e.g. Ogre v1 Overlays, which also
			// default to queue 254) renders through this scene manager in
			// the same queue, it must be moved to another queue first.
			mSceneManager->getRenderQueue()->setRenderQueueMode(RENDER_QUEUE_OVERLAY, Ogre::RenderQueue::FAST);
			// MyGUI relies on painter's order == submission order.
			mSceneManager->getRenderQueue()->setSortRenderQueue(RENDER_QUEUE_OVERLAY, Ogre::RenderQueue::DisableSort);
		}
	}

	Ogre::SceneManager* Ogre2RenderManager::getSceneManager()
	{
		return mSceneManager;
	}

	bool Ogre2RenderManager::frameStarted(const Ogre::FrameEvent& evt)
	{
		// this used to be done in render(), but can't do this anymore:
		// adding Workspaces (e.g. RenderBox::requestUpdateCanvas)
		// can't be done while the CompositorManager is still iterating through workspaces in its update() method
		static Timer timer;
		static unsigned long last_time = timer.getMilliseconds();
		unsigned long now_time = timer.getMilliseconds();
		unsigned long time = now_time - last_time;

		onFrameEvent((float)((double)(time) / (double)1000));

		last_time = now_time;

		return true;
	}

	void Ogre2RenderManager::render()
	{
		Gui* gui = Gui::getInstancePtr();
		if (gui == nullptr)
			return;

		mCountBatch = 0;

		setManualRender(true);

		mSceneManager->getRenderQueue()->clear();

		// get mygui to iterate through renderables and call 'doRender'.
		// This will add renderables (v2, VAO based) to the Ogre render queue.
		// Note: RTT layers (Canvas/RenderBox) flush intermediately via
		// Ogre2RTTexture::begin()/end() so their batches land in the RTT.
		onRenderToTarget(this, mUpdate);

		// Draw the remaining (window) batches.
		flush();

		// сбрасываем флаг
		mUpdate = false;
	}

	void Ogre2RenderManager::flush()
	{
		// Attention: This is new: https://forums.ogre3d.org/viewtopic.php?f=25&t=95049
		mSceneManager->getRenderQueue()->renderPassPrepare(false, false);
		mSceneManager->getRenderQueue()->render(mSceneManager->getDestinationRenderSystem(), RENDER_QUEUE_OVERLAY, RENDER_QUEUE_OVERLAY + 1, false, false);
		// The queue is consumed per flush so the next target starts empty.
		mSceneManager->getRenderQueue()->clear();
	}

	void Ogre2RenderManager::eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList* parameters)
	{
		if (eventName == "DeviceLost")
		{
		}
		else if (eventName == "DeviceRestored")
		{
			// обновить всех
			mUpdate = true;
		}
	}

	IVertexBuffer* Ogre2RenderManager::createVertexBuffer()
	{
		return new Ogre2VertexBuffer();
	}

	void Ogre2RenderManager::destroyVertexBuffer(IVertexBuffer* _buffer)
	{
		// The buffer owns its VAO and its renderable — everything dies here.
		delete _buffer;
	}

	// для оповещений об изменении окна рендера
	void Ogre2RenderManager::windowResized(Ogre::Window* _window)
	{
		// INVESTIGATION + FIX candidate for the persistent 1px right/bottom gap on
		// MyGUI overlays: mViewSize used to be built from _window->getWidth()/
		// getHeight() - the WINDOW's reported logical client size. But what the
		// "MYGUI" custom compositor pass actually renders into is the window's real
		// backbuffer, _window->getTexture(). These two are usually the same, but
		// are not guaranteed to be identical, and a 1px mismatch here would explain
		// everything observed so far: MyGUI widgets are built pixel-exact against
		// mViewSize (confirmed via logging - CoordConverter rounding fix, pixel-exact
		// FadeComponent/FaderProcess creation), yet even an OVERSCANNED widget (2px
		// larger than mViewSize on every side) still shows the identical 1px gap.
		// That combination only makes sense if the actual render target being drawn
		// into (the real backbuffer) is smaller than mViewSize itself - in which case
		// the GPU's own viewport/scissor clips the overscan away right along with
		// everything else, no matter how MyGUI builds its widgets. Using the
		// backbuffer's own reported size instead of the window's fixes that
		// regardless of which of the two was actually the "wrong" one.
		const Ogre::uint32 windowWidth = _window->getWidth();
		const Ogre::uint32 windowHeight = _window->getHeight();
		Ogre::uint32 backbufferWidth = windowWidth;
		Ogre::uint32 backbufferHeight = windowHeight;

		Ogre::TextureGpu* backbuffer = _window->getTexture();
		if (nullptr != backbuffer)
		{
			backbufferWidth = backbuffer->getWidth();
			backbufferHeight = backbuffer->getHeight();

			if (backbufferWidth != windowWidth || backbufferHeight != windowHeight)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
					"[Ogre2RenderManager] windowResized(): Window::getWidth/Height() report (" +
					Ogre::StringConverter::toString(windowWidth) + ", " + Ogre::StringConverter::toString(windowHeight) +
					") but the real backbuffer TextureGpu reports (" + Ogre::StringConverter::toString(backbufferWidth) +
					", " + Ogre::StringConverter::toString(backbufferHeight) + "). This mismatch is very likely the "
					"actual cause of the persistent 1px overlay gap - MyGUI was building every widget against the "
					"WRONG (window-reported) size instead of the real backbuffer size. Using the backbuffer size now.");
			}
		}

		// Use the real backbuffer size - what the GPU actually renders into - rather
		// than the window's reported logical size, in case the two ever disagree.
		mViewSize.set(backbufferWidth, backbufferHeight);

		// обновить всех
		mUpdate = true;

		updateRenderInfo();

		onResizeView(mViewSize);
	}

	void Ogre2RenderManager::updateRenderInfo()
	{
		if (mRenderSystem != nullptr)
		{
			mInfo.maximumDepth = mRenderSystem->getMaximumDepthInputValue();

			// INVESTIGATION for the persistent 1px right/bottom gap on MyGUI overlays
			// (the 3D scene itself fills the screen exactly, only MyGUI content -
			// Fader, window skins, etc. - is missing a pixel at the right/bottom edge):
			//
			// getHorizontalTexelOffset()/getVerticalTexelOffset() exist ONLY for the old
			// Direct3D9 "directly map texels to pixels" half-texel correction: D3D9
			// sampled texels at pixel CORNERS instead of centers, so pixel-perfect 2D/GUI
			// rendering needed every vertex position nudged by -0.5 texel. MyGUI's core
			// bakes this hOffset/vOffset into every vertex position before converting to
			// NDC (see mInfo usage). None of Ogre-Next's render systems (D3D11, D3D12,
			// Vulkan, Metal, GL3+) have this quirk and the base Ogre::RenderSystem is
			// supposed to return 0 here - but if this fork's D3D11 RenderSystem (or a
			// shared base it inherits from) still returns a stray non-zero value, EVERY
			// GUI vertex gets shifted by a fraction of a pixel in NDC space. For a
			// full-screen quad that is built pixel-exact against the screen edges (as
			// FadeComponent/FaderProcess/MyGUIWindowComponent now are, see the earlier
			// CoordConverter fix), that shift is invisible in the middle of the screen
			// but clips/reveals a sliver exactly at one edge - matching the symptom.
			//
			// Log once (throttled) so a rebuild can confirm whether this render system
			// actually returns a non-zero value here, and clamp to 0 unconditionally
			// either way: a non-zero D3D9-style texel offset has no correct meaning on
			// any of Ogre-Next's render systems, so there is no legitimate case where
			// honoring it would be more correct than ignoring it.
			static std::atomic<unsigned int> texelOffsetLogCounter{ 0 };
			const float rawHTexelOffset = mRenderSystem->getHorizontalTexelOffset();
			const float rawVTexelOffset = mRenderSystem->getVerticalTexelOffset();
			if (0 == (texelOffsetLogCounter++ % 10))
			{
				if (0.0f != rawHTexelOffset || 0.0f != rawVTexelOffset)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
						"[Ogre2RenderManager] updateRenderInfo(): RenderSystem reports a NON-ZERO "
						"texel offset (h=" + Ogre::StringConverter::toString(rawHTexelOffset) +
						", v=" + Ogre::StringConverter::toString(rawVTexelOffset) + "). This is a "
						"legacy Direct3D9-only correction and should always be 0 on this render "
						"system - it is being clamped to 0 for GUI rendering below, but a non-zero "
						"value here is worth reporting upstream too (this may well be the source of "
						"the 1px overlay gap).");
				}
			}

			mInfo.hOffset = 0.0f;
			mInfo.vOffset = 0.0f;
			mInfo.aspectCoef = float(mViewSize.height) / float(mViewSize.width);
			mInfo.pixScaleX = 1.0f / float(mViewSize.width);
			mInfo.pixScaleY = 1.0f / float(mViewSize.height);
		}
	}

	void Ogre2RenderManager::doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count)
	{
		Ogre2VertexBuffer* vertexBuffer = static_cast<Ogre2VertexBuffer*>(_buffer);
		Ogre2Texture* texture = static_cast<Ogre2Texture*>(_texture);

		// v2: the renderable lives inside the vertex buffer; its mVaoPerLod is
		// already populated (guaranteed before any setDatablock call below).
		Ogre2GuiRenderable* renderable = vertexBuffer->getRenderable();

		// Only the drawn range changes per batch — the VAO itself persists.
		vertexBuffer->setOperationVertexCount(_count);

		// Guarded to avoid a redundant flushRenderables()/hash recalculation
		// every frame — MyGUI batches keep the same texture per buffer.
		Ogre::HlmsDatablock* datablock = texture->getDataBlock();
		if (renderable->getDatablock() != datablock)
		{
			renderable->setDatablock(datablock);
		}

		mSceneManager->getRenderQueue()->addRenderableV2(0, RENDER_QUEUE_OVERLAY, false, renderable, mQueueMoveable);

		++mCountBatch;
	}

	void Ogre2RenderManager::begin()
	{
	}

	void Ogre2RenderManager::end()
	{
	}

	ITexture* Ogre2RenderManager::createTexture(const std::string& _name)
	{
		MapTexture::const_iterator item = mTextures.find(_name);
		MYGUI_PLATFORM_ASSERT(item == mTextures.end(), "Texture '" << _name << "' already exist");

		Ogre2Texture* texture = new Ogre2Texture(_name, Ogre2DataManager::getInstance().getGroup());
		mTextures[_name] = texture;
		return texture;
	}

	void Ogre2RenderManager::destroyTexture(ITexture* _texture)
	{
		if (_texture == nullptr) return;

		MapTexture::iterator item = mTextures.find(_texture->getName());
		MYGUI_PLATFORM_ASSERT(item != mTextures.end(), "Texture '" << _texture->getName() << "' not found");

		mTextures.erase(item);
		delete _texture;
	}

	void Ogre2RenderManager::removeTexture(ITexture* _texture)
	{
		if (_texture == nullptr) return;

		MapTexture::iterator item = mTextures.find(_texture->getName());
		MYGUI_PLATFORM_ASSERT(item != mTextures.end(), "Texture '" << _texture->getName() << "' not found");

		mTextures.erase(item);
	}

	ITexture* Ogre2RenderManager::getTexture(const std::string& _name)
	{
		MapTexture::const_iterator item = mTextures.find(_name);
		if (item == mTextures.end())
		{
			Ogre::TextureGpuManager* textureMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			const Ogre::String* name = textureMgr->findResourceNameStr(_name);
			if (name)
			{
				Ogre::TextureGpu* texture = textureMgr->createOrRetrieveTexture(
					_name,
					Ogre::GpuPageOutStrategy::Discard,
					Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
					Ogre::TextureTypes::Type2D,
					Ogre::ResourceGroupManager::
					AUTODETECT_RESOURCE_GROUP_NAME);
				ITexture* result = createTexture(_name);
				static_cast<Ogre2Texture*>(result)->setOgreTexture(texture);
				return result;
			}
			return nullptr;
		}
		return item->second;
	}

	bool Ogre2RenderManager::isFormatSupported(PixelFormat _format, TextureUsage _usage)
	{
		if (_format == PixelFormat::L8A8) return false; // Not supported
		return true; // FIXME
	}

	void Ogre2RenderManager::destroyAllResources()
	{
		// v2: renderables are owned by their vertex buffers — nothing to do
		// here for them anymore.
		for (MapTexture::const_iterator item = mTextures.begin(); item != mTextures.end(); ++item)
		{
			delete item->second;
		}
		mTextures.clear();
	}

#if MYGUI_DEBUG_MODE == 1
	bool Ogre2RenderManager::checkTexture(ITexture* _texture)
	{
		for (MapTexture::const_iterator item = mTextures.begin(); item != mTextures.end(); ++item)
		{
			if (item->second == _texture)
				return true;
		}
		return false;
	}
#endif

	const IntSize& Ogre2RenderManager::getViewSize() const
	{
		return mViewSize;
	}

	VertexColourType Ogre2RenderManager::getVertexFormat()
	{
		return mVertexFormat;
	}

	const RenderTargetInfo& Ogre2RenderManager::getInfo()
	{
		return mInfo;
	}

	Ogre::Window* Ogre2RenderManager::getRenderWindow()
	{
		return mWindow;
	}

	bool Ogre2RenderManager::getManualRender()
	{
		return mManualRender;
	}

	void Ogre2RenderManager::setManualRender(bool _value)
	{
		mManualRender = _value;
	}

	size_t Ogre2RenderManager::getBatchCount() const
	{
		return mCountBatch;
	}

} // namespace MyGUI