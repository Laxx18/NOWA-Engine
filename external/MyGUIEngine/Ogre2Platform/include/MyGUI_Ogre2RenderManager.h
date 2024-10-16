/*!
	@file
	@author		Albert Semenov
	@date		04/2008
*/

#ifndef MYGUI_OGRE2_RENDER_MANAGER_H_
#define MYGUI_OGRE2_RENDER_MANAGER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_RenderFormat.h"
#include "MyGUI_IVertexBuffer.h"
#include "MyGUI_ITexture.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_Ogre2GuiRenderable.h"
#include "MyGUI_Ogre2GuiMoveable.h"
#include "MyGUI_Ogre2RenderManager.h"

#include <Ogre.h>
#include <OgreMovableObject.h>
#include <Compositor/Pass/OgreCompositorPass.h>
#include <Compositor/Pass/OgreCompositorPassDef.h>
#include <Compositor/Pass/OgreCompositorPassProvider.h>

#include "MyGUI_LastHeader.h"

namespace MyGUI
{
	class Ogre2RenderManager;
	class __declspec(dllexport) MyGUIPassDef : public Ogre::CompositorPassDef
	{
	public:
		MyGUIPassDef(Ogre::CompositorTargetDef *parentTargetDef)
			: Ogre::CompositorPassDef(Ogre::PASS_CUSTOM, parentTargetDef)
		{
		}
	};

	// TODO: The compositor manager allows adding this pass to any number of targets,
	// but MyGUI was designed to handle one window target only. For example,
	// when we add the pass to multiple targets, all RTTLayers would be unnecessarily updated multiple times.
	class __declspec(dllexport) MyGUIPass : public Ogre::CompositorPass
	{
	public:
		MyGUIPass(const Ogre::CompositorPassDef *definition, const Ogre::RenderTargetViewDef *target,
			Ogre::CompositorNode *parentNode);

		virtual void execute(const Ogre::Camera *lodCameraconst);
	};

	// The factory for the MyGUI compositor pass. Note that only one provider can be
	// registered with Ogre at a time, which is why we have exposed the MyGUIPass and MyGUIPassDef classes
	// publicly in this header. If users need their own custom passes, they can implement their own provider
	// which would return either a MyGUI pass or other custom passes depending on the customId.
	class __declspec(dllexport) OgreCompositorPassProvider : public Ogre::CompositorPassProvider
	{
	public:
		Ogre::CompositorPassDef* addPassDef(Ogre::CompositorPassType passType,
			Ogre::IdString customId,
			Ogre::CompositorTargetDef *parentTargetDef,
			Ogre::CompositorNodeDef *parentNodeDef)
		{
			if (customId == "MYGUI")
				return OGRE_NEW MyGUI::MyGUIPassDef(parentTargetDef);
			return nullptr;
		};

		Ogre::CompositorPass* addPass(const Ogre::CompositorPassDef *definition, Ogre::Camera *defaultCamera,
			Ogre::CompositorNode *parentNode, const Ogre::RenderTargetViewDef *target,
			Ogre::SceneManager *sceneManager)
		{
			return OGRE_NEW MyGUI::MyGUIPass(definition, target, parentNode);
		}
	};

	class __declspec(dllexport) Ogre2RenderManager :
		public RenderManager,
		public IRenderTarget,
		public Ogre::WindowEventListener,
		public Ogre::RenderSystem::Listener,
		public Ogre::FrameListener
	{
	public:
		Ogre2RenderManager();

		// FrameListener
		bool frameStarted(const Ogre::FrameEvent &evt);

		void initialise(Ogre::Window* _window, Ogre::SceneManager* _scene);
		void shutdown();

		static Ogre2RenderManager& getInstance();
		static Ogre2RenderManager* getInstancePtr();

		/** @see RenderManager::getViewSize */
		virtual const IntSize& getViewSize() const;

		/** @see RenderManager::getVertexFormat */
		virtual VertexColourType getVertexFormat();

		/** @see RenderManager::createVertexBuffer */
		virtual IVertexBuffer* createVertexBuffer();
		/** @see RenderManager::destroyVertexBuffer */
		virtual void destroyVertexBuffer(IVertexBuffer* _buffer);

		/** @see RenderManager::createTexture */
		virtual ITexture* createTexture(const std::string& _name);
		/** @see RenderManager::destroyTexture */
		virtual void destroyTexture(ITexture* _texture);

		/* 29.07.2024: Start: Added by Lax, because Ogre holds the texturepointer and MyGui and if one destroys the texture, ugly crash occurs. Hence MyGUI shall just remove the texture from its list! */
		void removeTexture(ITexture* _texture);

		/* 29.07.2024: End: Added by Lax

		/** @see RenderManager::getTexture */
		virtual ITexture* getTexture(const std::string& _name);

		/** @see RenderManager::isFormatSupported */
		virtual bool isFormatSupported(PixelFormat _format, TextureUsage _usage);

		/** @see IRenderTarget::begin */
		virtual void begin();
		/** @see IRenderTarget::end */
		virtual void end();

		/** @see IRenderTarget::doRender */
		virtual void doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count);

		/** @see IRenderTarget::getInfo */
		virtual const RenderTargetInfo& getInfo();

		void setRenderSystem(Ogre::RenderSystem* _render);
		Ogre::RenderSystem* getRenderSystem();

		void setRenderWindow(Ogre::Window* _window);

		/** Set scene manager where MyGUI will be rendered */
		void setSceneManager(Ogre::SceneManager* _scene);
		Ogre::SceneManager* getSceneManager();

		Ogre::Window* getRenderWindow();

		bool getManualRender();
		void setManualRender(bool _value);

		void render();

		size_t getBatchCount() const;

#if MYGUI_DEBUG_MODE == 1
		virtual bool checkTexture(ITexture* _texture);
#endif

	private:
		virtual void windowResized(Ogre::Window* _window);

		// восстанавливаем буферы
		virtual void eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList* parameters);

		void destroyAllResources();
		void updateRenderInfo();

	private:
		// флаг для обновления всех и вся
		bool mUpdate;

		IntSize mViewSize;

		Ogre::SceneManager* mSceneManager;

		VertexColourType mVertexFormat;

		// окно, на которое мы подписываемся для изменения размеров
		Ogre::Window* mWindow;

		Ogre::RenderSystem* mRenderSystem;

		RenderTargetInfo mInfo;

		typedef std::map<std::string, ITexture*> MapTexture;
		MapTexture mTextures;

		bool mIsInitialise;
		bool mManualRender;
		size_t mCountBatch;

	private:
		Ogre2GuiMoveable* mQueueMoveable;
		Ogre::SceneNode* mQueueSceneNode;

		Ogre::ObjectMemoryManager mDummyObjMemMgr;
		Ogre::NodeMemoryManager mDummyNodeMemMgr;


		typedef std::map<unsigned int, Ogre2GuiRenderable*> MapRenderable;
		MapRenderable mRenderables;

		static const Ogre::uint8 RENDER_QUEUE_OVERLAY;

		Ogre2GuiRenderable* createOrRetrieveRenderable(IVertexBuffer* _buffer , ITexture* _texture , size_t _count);
		// Is done in NOWA::Core
		std::unique_ptr<OgreCompositorPassProvider> mPassProvider;
	};

} // namespace MyGUI

#endif // MYGUI_OGRE_RENDER_MANAGER_H_
