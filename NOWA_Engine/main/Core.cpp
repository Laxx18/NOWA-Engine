#include "NOWAPrecompiled.h"
#include "Core.h"

#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "GpuParticles/Hlms/HlmsParticle.h"
#include "OgreArchiveManager.h"
#include "OgreFrameStats.h"
#include "OgreRectangle2D.h"
#include "OgreCodec.h"
#include "OgreHlmsDiskCache.h"
#include "OgreAbiUtils.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"

#include "ocean/OgreHlmsOcean.h"

#include "utilities/MovableText.h"
#include "modules/InputDeviceModule.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"
#include "ProcessManager.h"
#include "Events.h"

#include "OgreHlmsTerra.h"
#include "shader/HlmsWind.h"
#include "shader/HlmsFog.h"

// Lua console
#include "console/LuaConsole.h"
#include "modules/LuaScriptApi.h"
#include "modules/OgreALModule.h"

#include <shlobj.h>
#include <direct.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#ifdef WIN32
#include <WindowsIncludes.h>
#include <tlhelp32.h>

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <psapi.h>

#pragma comment(lib, "shell32")
#else
#include <X11/Xlib.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

#if OGRE_PROFILING
#include "OgreProfiler.h"
#endif

namespace NOWA
{
	ResourceLoadingListenerImpl::ResourceLoadingListenerImpl()
	{

	}

	ResourceLoadingListenerImpl::~ResourceLoadingListenerImpl()
	{

	}

	Ogre::SharedPtr<Ogre::DataStream> ResourceLoadingListenerImpl::resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource)
	{
		// Ogre::DataStreamResourcePtr dataStreamResource = Ogre::Datastream::getSingleton().load(name, group);
		// DataStreamPtr encoded = dataStreamResource->getDataStream();
		// return encoded;
		// Ogre::SharedPtr<Ogre::DataStream> dataStream = Ogre::ResourceGroupManager::getSingletonPtr()->createResource(name, group);
		// return dataStream;
		return Ogre::SharedPtr<Ogre::DataStream>();
	}

	bool ResourceLoadingListenerImpl::grouplessResourceExists(const Ogre::String& name)
	{
		if (name == "_rt.")
		{
			return true;
		}
		return false;
	}

	Ogre::SharedPtr<Ogre::DataStream> ResourceLoadingListenerImpl::grouplessResourceLoading(const Ogre::String& name)
	{
		// Ogre::SharedPtr<Ogre::DataStream> dataStream = Ogre::ResourceGroupManager::getSingletonPtr()->createResource(name);
		// return dataStream;
		return Ogre::SharedPtr<Ogre::DataStream>();
	}

	Ogre::SharedPtr<Ogre::DataStream> ResourceLoadingListenerImpl::grouplessResourceOpened(const Ogre::String& name, Ogre::Archive* archive, Ogre::SharedPtr<Ogre::DataStream>& dataStream)
	{
		return dataStream;
	}

	void ResourceLoadingListenerImpl::resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::SharedPtr<Ogre::DataStream>& dataStream)
	{

	}

	bool ResourceLoadingListenerImpl::resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Core::Core()
		: root(nullptr),
		renderWindow(nullptr),
		overlaySystem(nullptr),
		myGuiWorkspace(nullptr),
		writeAccessFolder("../../cache/."),
		isGame(true),
		myGui(nullptr),
		myGuiOgrePlatform(nullptr),
		engineResourceListener(nullptr),
		defaultEngineResourceListener(nullptr),
		pluginFactory(nullptr),
		timer(nullptr),
		optionLODBias(2.0f),
		optionTextureFiltering(2),
		optionAnisotropyLevel(4),
		optionQualityLevel(1),
		optionSoundVolume(70),
		optionMusicVolume(50),
		optionLogLevel(2),
		optionLuaGroupName("Lua"),
		optionLuaApiFilePath("C:/Program Files (x86)/ZeroBraneStudio/api/lua"),
		bShutdown(false),
		startAsServer(false),
		optionAreaOfInterest(0),
		optionPacketSendRate(5),
		optionInterpolationRate(33),
		optionPacketsPerSecond(16),
		optionPlayerColor(0),
		optionUseLuaConsole(true),
		optionUseLuaScript(true),
		requiredDiscSpaceMB(1024), // 1 Gigabyte
		requiredCPUSpeedMhz(1300), // 1.3 Ghz,
		requiredRAMPhysicalMB(1024), // 1 Gigabyte
		requiredRAMVirtualMB(512), // 0.5 Gigabyte
		globalRenderDistance(100.0f),
		optionLanguage(0), // English default
		borderType("none"),
		info(nullptr),
		timeSinceLastStatisticUpdate(0.0f),
		mostLeftNearPosition(Ogre::Vector3::ZERO),
		mostRightFarPosition(Ogre::Vector3::ZERO),
		resourceLoadingListener(nullptr),
		cryptKey(2),
		projectEncoded(false),
		useEntityType(true),
		baseListenerContainer(nullptr)
	{
		this->optionDesiredFramesUpdates = this->getScreenRefreshRate();
		this->optionDesiredSimulationUpdates = 30;
	}

	Core::~Core()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Starting shutdown");

		if (nullptr != this->baseListenerContainer)
		{
			Ogre::Hlms* hlms = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
			Ogre::HlmsPbs* pbs = static_cast<Ogre::HlmsPbs*>(hlms);
			pbs->setListener(nullptr);
			delete this->baseListenerContainer;
			this->baseListenerContainer = nullptr;
		}

		if (nullptr != InputDeviceCore::getSingletonPtr() && false == InputDeviceCore::getSingletonPtr()->getInputDeviceModules().empty())
		{
			this->saveCustomConfiguration();
		}

		ProcessManager::getInstance()->clearAllProcesses();

		if (nullptr != this->root)
		{
			auto factoryMovableText = this->root->getMovableObjectFactory("MovableText");
			if (nullptr != factoryMovableText)
			{
				delete factoryMovableText;
			}
			auto factoryRectangle2D = this->root->getMovableObjectFactory("Rectangle2D");
			if (nullptr != factoryRectangle2D)
			{
				delete factoryRectangle2D;
			}

			Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();
			Ogre::TextureGpuManager* textureGpuManager = renderSystem->getTextureGpuManager();
			if (nullptr != textureGpuManager)
			{
				textureGpuManager->setTextureGpuManagerListener(nullptr);
			}
		}
		
		if (nullptr != resourceLoadingListener)
		{
			delete this->resourceLoadingListener;
			this->resourceLoadingListener = nullptr;
		}
		if (nullptr != this->defaultEngineResourceListener)
		{
			delete this->defaultEngineResourceListener;
			this->defaultEngineResourceListener = nullptr;
		}

		Ogre::CompositorManager2* compositorManager = Ogre::Root::getSingletonPtr()->getCompositorManager2();

		if (nullptr != this->myGuiWorkspace && nullptr != this->root)
		{
			this->myGuiOgrePlatform->getRenderManagerPtr()->setSceneManager(nullptr);
			compositorManager->removeWorkspace(this->myGuiWorkspace);
			this->myGuiWorkspace = nullptr;
		}
		if (nullptr != this->myGui)
		{
			if (nullptr != this->info)
			{
				delete this->info;
				this->info = nullptr;
			}
			
			this->myGui->shutdown();
			delete this->myGui;
			this->myGui = nullptr;
			this->myGuiOgrePlatform->shutdown();
			delete this->myGuiOgrePlatform;
			this->myGuiOgrePlatform = nullptr;
		}
		
		if (nullptr != this->overlaySystem)
		{
			delete this->overlaySystem;
			this->overlaySystem = nullptr;
		}
		if (nullptr != this->pluginFactory)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Deleting plugin factory");
			delete this->pluginFactory;
			this->pluginFactory = nullptr;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Deleting pre load params of shader module");

		if (nullptr != this->renderWindow)
		{
			Ogre::WindowEventUtilities::removeWindowEventListener(this->renderWindow, this);
			// input manager is destroyed in windowClosed
			this->windowClosed(this->renderWindow);
		}

		/*if (this->inputManager)	 {
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Deleting input manager");
			OIS::InputManager::destroyInputSystem(this->inputManager);
			this->inputManager = nullptr;
			}*/
		if (true == this->optionUseLuaConsole)
		{
			if (LuaConsole::getSingletonPtr())
			{
				NOWA::LuaConsole::getSingletonPtr()->setVisible(false);
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Deleting lua console");
				LuaConsole::getSingletonPtr()->shutdown();

				delete LuaConsole::getSingletonPtr();
			}
		}
		if (true == this->optionUseLuaScript)
		{
			LuaScriptApi::getInstance()->destroyContent();
		}

		if (nullptr != this->timer)
		{
			delete this->timer;
			this->timer = nullptr;
		}

		this->saveHlmsDiskCache();

		if (nullptr != InputDeviceCore::getSingletonPtr())
		{
			delete InputDeviceCore::getSingletonPtr();
		}

		if (nullptr != this->root)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Deleting OgreRoot");
			// but in Ogre, when deleting root, it will call shutdown internally and tries to shutdown a ScriptSerializer, which has not been
			// created and not initialized with NULL, therefore crash, in the early state, when the config dialog appears, but is cancled!
			OGRE_DELETE this->root;
			this->root = nullptr;
		}
	}

	void Core::loadHlmsDiskCache(void)
	{
		Ogre::HlmsManager* hlmsManager = this->root->getHlmsManager();
		Ogre::HlmsDiskCache diskCache(hlmsManager);

		Ogre::ArchiveManager& archiveManager = Ogre::ArchiveManager::getSingleton();

		Ogre::Archive* rwAccessFolderArchive = archiveManager.load(this->writeAccessFolder, "FileSystem", true);

		// Load texture cache
		try
		{
			const Ogre::String filename = "NOWA_Engine.json";
			if (rwAccessFolderArchive->exists(filename))
			{
				Ogre::DataStreamPtr stream = rwAccessFolderArchive->open(filename);
				std::vector<char> fileData;
				fileData.resize(stream->size() + 1);
				if (!fileData.empty())
				{
					stream->read(&fileData[0], stream->size());
					// Add null terminator just in case (to prevent bad input)
					fileData.back() = '\0';
					Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
					textureManager->importTextureMetadataCache(stream->getName(), &fileData[0], false);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(LML_CRITICAL, "[INFO] Texture cache not found at " + this->writeAccessFolder + "/textureMetadataCache.json");
			}
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingleton().logMessage(e.getFullDescription());
		}

		//Make sure the microcode cache is enabled.
		Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache(true);
		const Ogre::String filename = "NOWA_Engine.cache";
		if (rwAccessFolderArchive->exists(filename))
		{
			Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->open(filename);
			Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache(shaderCacheFile);

			const size_t numThreads = std::max<size_t>(1u, Ogre::PlatformInformation::getNumLogicalCores());
			for (size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i)
			{
				Ogre::Hlms *hlms = hlmsManager->getHlms(static_cast<Ogre::HlmsTypes>(i));
				if (hlms)
				{
					Ogre::String filename = "NOWA_Engine" + Ogre::StringConverter::toString(i) + ".bin";

					try
					{
						if (rwAccessFolderArchive->exists(filename))
						{
							Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->open(filename);
							diskCache.loadFrom(diskCacheFile);
							diskCache.applyTo(hlms, numThreads);
						}
					}
					catch (Ogre::Exception&)
					{
						Ogre::LogManager::getSingleton().logMessage("Error loading cache from '" + this->writeAccessFolder + filename + "' ! If you have issues, try deleting the file and restarting the app");
					}
				}
			}
		}

		archiveManager.unload(rwAccessFolderArchive);
	}
	
	void Core::saveHlmsDiskCache(void)
	{
		if (nullptr == this->root)
		{
			return;
		}

		if (this->root->getRenderSystem() && Ogre::GpuProgramManager::getSingletonPtr())
		{
			Ogre::HlmsManager* hlmsManager = this->root->getHlmsManager();
			Ogre::HlmsDiskCache diskCache(hlmsManager);

			Ogre::ArchiveManager& archiveManager = Ogre::ArchiveManager::getSingleton();

			Ogre::Archive* rwAccessFolderArchive = archiveManager.load(this->writeAccessFolder, "FileSystem", false);

			Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			if (textureManager)
			{
				Ogre::String jsonString;
				textureManager->exportTextureMetadataCache(jsonString);
				const Ogre::String path = this->writeAccessFolder + "/NOWA_Engine.json";
				std::ofstream file(path.c_str(), std::ios::binary | std::ios::out);
				if (file.is_open())
					file.write(jsonString.c_str(), static_cast<std::streamsize>(jsonString.size()));
				file.close();
			}

			for (size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i)
			{
				Ogre::Hlms* hlms = hlmsManager->getHlms(static_cast<Ogre::HlmsTypes>(i));
				if (hlms && hlms->isShaderCodeCacheDirty())
				{
					diskCache.copyFrom(hlms);

					Ogre::String filename = "NOWA_Engine" + Ogre::StringConverter::toString(i) + ".bin";
					Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->create(filename);
					diskCache.saveTo(diskCacheFile);
				}
			}

			if (true == Ogre::GpuProgramManager::getSingleton().isCacheDirty())
			{
				try
				{
					// Save shader cache
					const Ogre::String filename = "NOWA_Engine.cache";
					Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->create(filename);
					Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache(shaderCacheFile);
				}
				catch (std::exception&)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: Something went wrong during GPU shader cache saving.");
				}
			}

			archiveManager.unload(this->writeAccessFolder);
		}
	}

	Core* Core::getSingletonPtr(void)
	{
		return msSingleton;
	}

	Core& Core::getSingleton(void)
	{
		assert(msSingleton);
		return (*msSingleton);
	}

	bool Core::initialize(const CoreConfiguration& coreConfiguration)
	{
		this->graphicsConfigName = coreConfiguration.graphicsConfigName;

		if (true == this->graphicsConfigName.empty())
			this->graphicsConfigName = "ogre.cfg";

		this->customConfigName = coreConfiguration.customConfigName;
		this->title = coreConfiguration.wndTitle;
		this->resourcesName = "../resources/" + coreConfiguration.resourcesName;
		this->isGame = coreConfiguration.isGame;
		this->logName = "../resources/" + this->title + Ogre::String("_") + this->graphicsConfigName + Ogre::String(".log");
		this->resourceGroupNames.clear();

		if (false == coreConfiguration.startProjectName.empty())
		{
			size_t found = coreConfiguration.startProjectName.find_last_of("/\\");
			if (Ogre::String::npos == found)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] Could not set project name because the project name or scene name is wrong! See: '" + coreConfiguration.startProjectName + "'");
				throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[Core] Could not set project name because the project name or scene name is wrong! See: '" + coreConfiguration.startProjectName + "'.\n", "NOWA");
				return false;
			}
			this->projectName = coreConfiguration.startProjectName.substr(0, found);
			this->worldName = coreConfiguration.startProjectName.substr(found + 1, coreConfiguration.startProjectName.size() - 1);
		}

		// Look if the application has been started with a custom .cfg file in the command line, if so, init root with that configfile
		try
		{
			const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
			this->root = OGRE_NEW Ogre::Root(&abiCookie, "plugins.cfg", this->graphicsConfigName, logName);
		}
		catch (Ogre::InternalErrorException& e)
		{
			Ogre::String description = "[Core]: Internal error: " + e.getFullDescription();

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, description);

			size_t found = description.find("Could not load dynamic library");
			if (Ogre::String::npos != found)
			{
				description += "\n\nPlease check in the Debug/Release folder in the plugins.cfg, that the corresponding plugin does exist! And if not, remove it from the list!";
			}
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: " + description);
			throw Ogre::InternalErrorException(1, description, "Init root", __FILE__, __LINE__);
		}

		// Enable sRGB Gamma Conversion mode by default for all renderers, but still allow to override it via config dialog
        for (auto& it = this->root->getAvailableRenderers().begin(); it != this->root->getAvailableRenderers().end(); ++it)
        {
            Ogre::RenderSystem* rs = *it;
            rs->setConfigOption("sRGB Gamma Conversion", "Yes");
        }
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: Configuration filename: " + this->graphicsConfigName);
		FILE* fp = nullptr;
		if (this->graphicsConfigName.length() > 0)
		{
			fp = fopen(this->graphicsConfigName.c_str(), "r");
		}
		else
		{
			fp = fopen("ogre.cfg", "r");
		}

		//pruefen ob die Ogre-Konfigurationsdatei existiert
		if (fp == nullptr)
		{
			//Wenn diese nicht existiert, dann wird config dialog gezeigt, damit der Benutzer Einstellungen fuer die Grafik vornehmen kann
			if (!this->root->showConfigDialog())
			{
				//Wenn Abbrechen im Konfigurationsdialog gedrueckt wird
				return false;
			}
		}
		else
		{
			//Ansonsten werden die Daten aus der "*.cfg" im Hintergrund geladen
			this->root->restoreConfig();
			fclose(fp);
		}

		#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if(false == this->root->getRenderSystem())
        {
            Ogre::RenderSystem* renderSystem = this->root->getRenderSystemByName("Metal Rendering Subsystem");
            this->root->setRenderSystem(renderSystem);
        }
    #endif
    #if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        if( !this->root->getRenderSystem() )
        {
            Ogre::RenderSystem* renderSystem = this->root->getRenderSystemByName("Vulkan Rendering Subsystem");
            this->root->setRenderSystem(renderSystem);
        }
    #endif
		this->root->initialise(false);

		Ogre::ConfigOptionMap& cfgOpts = this->root->getRenderSystem()->getConfigOptions();

		int width = 1280;
		int height = 720;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
		{
			Ogre::Vector2 screenRes = iOSUtils::getScreenResolutionInPoints();
			width = static_cast<int>(screenRes.x);
			height = static_cast<int>(screenRes.y);
		}
#endif

		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

		Ogre::ConfigOptionMap::iterator opt = cfgOpts.find("Video Mode");
		if(opt != cfgOpts.end() && !opt->second.currentValue.empty())
		{
			//Ignore leading space
			const Ogre::String::size_type start = opt->second.currentValue.find_first_of("012356789");
			//Get the width and height
			Ogre::String::size_type widthEnd = opt->second.currentValue.find(' ', start);
			// we know that the height starts 3 characters after the width and goes until the next space
			Ogre::String::size_type heightEnd = opt->second.currentValue.find(' ', widthEnd + 3);
			// Now we can parse out the values
			width = Ogre::StringConverter::parseInt(opt->second.currentValue.substr(0, widthEnd));
			height = Ogre::StringConverter::parseInt(opt->second.currentValue.substr(widthEnd + 3, heightEnd));
			if (1040 == height)
			{
				height = 1080;
			}
		}

		Ogre::NameValuePairList params;
		bool fullscreen = Ogre::StringConverter::parseBool(cfgOpts["Full Screen"].currentValue);

		params.insert(std::make_pair("title", coreConfiguration.wndTitle));
		params.insert( std::make_pair("gamma", cfgOpts["sRGB Gamma Conversion"].currentValue));
		params.insert(std::make_pair("hidden", "false")); // if true, no window is shown, what is the purpose of this param?
		params.insert(std::make_pair("Fast Shader Build Hack", "Yes"));

		params.insert(std::make_pair("FSAA", cfgOpts["FSAA"].currentValue));
		params.insert(std::make_pair("VSync", cfgOpts["VSync"].currentValue));
		if (cfgOpts.find("VSync Method") != cfgOpts.end())
			params.insert(std::make_pair("vsync_method", cfgOpts["VSync Method"].currentValue));
		params.insert(std::make_pair("reverse_depth", "Yes"));
		Ogre::String useVsync = cfgOpts["VSync"].currentValue;

		// Params for window inside graphics
		params.insert(std::make_pair("vsyncInterval", cfgOpts["VSync Interval"].currentValue));
		/*params.insert(std::make_pair("vsync", cfgOpts["VSync"].currentValue));
		params.insert(std::make_pair("left", "0"));
		params.insert(std::make_pair("top", "0"));
		params.insert(std::make_pair("parentWindowHandle", "0"));
		params.insert(std::make_pair("externalWindowHandle", "0"));
		params.insert(std::make_pair("border", "fixed"));
		params.insert(std::make_pair("alwaysWindowedMode", "Yes"));
		params.insert(std::make_pair("enableDoubleClick", "Yes"));
		params.insert(std::make_pair("useFlipMode", "Yes"));*/
		params.insert(std::make_pair("monitorIndex", "0"));

		// Note: Fixes MyGUI Font rendering issues if set to "none"
		params.insert(std::make_pair("border", this->borderType));
		
			/*parentWindowHandle       -> parentHwnd
			externalWindowHandle     -> externalHandle
			border : none, fixed
			outerDimensions
			monitorIndex
			useFlipMode
			vsync
			vsyncInterval
			alwaysWindowedMode
			enableDoubleClick*/

		// Add also custom params
		for (auto& customParam : coreConfiguration.customParams)
		{
			params.insert(std::make_pair(customParam.first, customParam.second));
		}

		// this->initMiscParamsListener(params);

		this->renderWindow = Ogre::Root::getSingleton().createRenderWindow(coreConfiguration.wndTitle, width, height, fullscreen, &params);

		// Check for adequate machine resources.
		bool resourceCheck = false;
		while (false == resourceCheck)
		{
			DWORDLONG thisRAMPhysical;
			DWORDLONG thisRAMVirtual;
			DWORDLONG thisDiscSpace;

			if (false == this->checkMemory(this->requiredRAMPhysicalMB, this->requiredRAMVirtualMB, thisRAMPhysical, thisRAMVirtual))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] RAM is to low to execute this application. A physical RAM with more than "
					+ Ogre::StringConverter::toString(this->requiredRAMPhysicalMB) + " MB is required and a virtual RAM with more than "
					+ Ogre::StringConverter::toString(this->requiredRAMVirtualMB) + " MB is required.");
				throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[Core] RAM is to low to execute this application. A physical RAM with more than "
					+ Ogre::StringConverter::toString(this->requiredRAMPhysicalMB) + " MB is required and a virtual RAM with more than "
					+ Ogre::StringConverter::toString(this->requiredRAMVirtualMB) + " MB is required.\n", "NOWA");
				return false;
			}
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: Available Physical RAM size: " + Ogre::StringConverter::toString(static_cast<DWORD>(thisRAMPhysical))
				+ " MB, which is enough for this application (> "
				+ Ogre::StringConverter::toString(this->requiredRAMPhysicalMB) + ") and available virtual RAM size: " + Ogre::StringConverter::toString(static_cast<DWORD>(thisRAMVirtual))
				+ " MB, which is enough for this application (> "
				+ Ogre::StringConverter::toString(this->requiredRAMVirtualMB) + ")");

			if (!this->checkStorage(this->requiredDiscSpaceMB, thisDiscSpace))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] Disc space is to low to execute this application. A disc space with more than "
					+ Ogre::StringConverter::toString(this->requiredDiscSpaceMB) + " MB is required");
				throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[Core] Disc space is to low to execute this application. A disc space with more than "
					+ Ogre::StringConverter::toString(this->requiredDiscSpaceMB) + " MB is required\n", "NOWA");
				return false;
			}
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: Disc space: " + Ogre::StringConverter::toString(static_cast<DWORD>(thisDiscSpace))
				+ " MB (" + Ogre::StringConverter::toString(static_cast<DWORD>(thisDiscSpace / 1024)) 
				+ " GB, " + Ogre::StringConverter::toString(static_cast<float>((thisDiscSpace / (1024.0f * 1024.0f)))) 
				+ " TB), which is enough for this application (> " + Ogre::StringConverter::toString(this->requiredDiscSpaceMB) + ")");

			DWORD thisCPU = this->readCPUSpeed();
			if (static_cast<int>(thisCPU) < this->requiredCPUSpeedMhz)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] CPU is too slow for to execute this application. A processor with more than "
					+ Ogre::StringConverter::toString(this->requiredCPUSpeedMhz / 1000.0f) + " Ghz is required!");
				throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[Core] CPU is too slow for to execute this application. A processor with more than "
					+ Ogre::StringConverter::toString(this->requiredCPUSpeedMhz / 1000.0f) + " Ghz is required!\n", "NOWA");
				return false;
			}
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: Cpu speed: " + Ogre::StringConverter::toString(thisCPU / 1000.0f) + " Ghz, which is enough for this application (> "
				+ Ogre::StringConverter::toString(requiredCPUSpeedMhz / 1000.0f) + ")");
			resourceCheck = true;
		}

		// FIXME: porting to linux
		// works strangly only with release on windows
		/*#ifdef WIN32 & !_DEBUG
		HWND handle = 0;
		this->renderWindow->getCustomAttribute("WINDOW", handle);

		if (handle != NULL)
		SetForegroundWindow(handle);
		#endif*/
		//this->root->clearEventTimes();

		// create a dummy scene manager and camera, to show the state when loading resources
		Ogre::SceneManager* dummySceneManager = this->root->createSceneManager(Ogre::ST_GENERIC, 1, "DummyScene");
		Ogre::Camera* dummyCamera = dummySceneManager->createCamera("DummyCamera");

		this->registerHlms();

		// vsync on off at runtime: http://www.ogre3d.org/forums/viewtopic.php?f=2&t=61531

		this->timer = new Ogre::Timer();
		this->timer->reset();

		new InputDeviceCore();

		InputDeviceCore::getSingletonPtr()->initialise(this->renderWindow);
		InputDeviceCore::getSingletonPtr()->addKeyListener(this, "Core");
		InputDeviceCore::getSingletonPtr()->addMouseListener(this, "Core");
		InputDeviceCore::getSingletonPtr()->addJoystickListener(this, "Core");

		Ogre::String strXMLConfigName = "defaultConfig.xml"; // Always use defaultConfig! Or add new parameter for custom config to
		if (false == this->customConfigName.empty())
		{
			strXMLConfigName = this->customConfigName;
		}
		this->loadCustomConfiguration(strXMLConfigName.c_str());

		// Set initial mouse clipping size
		this->windowResized(this->renderWindow);

		// Register as a Window listener
		Ogre::WindowEventUtilities::addWindowEventListener(this->renderWindow, this);
		// this->root->getRenderSystem()->addListener(this);

		// this->resourceLoadingListener = new ResourceLoadingListenerImpl();
		// Ogre::ResourceGroupManager::getSingletonPtr()->setLoadingListener(this->resourceLoadingListener);

		this->overlaySystem = OGRE_NEW Ogre::v1::OverlaySystem();
		dummySceneManager->addRenderQueueListener(this->overlaySystem);
		dummySceneManager->getRenderQueue()->setSortRenderQueue(Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId, Ogre::RenderQueue::StableSort);

		// Add factories
		this->root->addMovableObjectFactory(OGRE_NEW MovableTextFactory());
		this->root->addMovableObjectFactory(OGRE_NEW Ogre::v1::Rectangle2DFactory());

		Ogre::String resourceGroupName;
		Ogre::String type;
		Ogre::String path;
		Ogre::ConfigFile cf;

		cf.load(this->resourcesName);

		unsigned int loadableObjectsCount = 0;
		unsigned int jsonMaterialsCount = 0;
		// Go through all sections & settings in the file
		Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
		while (seci.hasMoreElements())
		{
			resourceGroupName = seci.peekNextKey();
			Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
			if (resourceGroupName != "Hlms")
			{
				Ogre::ConfigFile::SettingsMultiMap::const_iterator it;
				for (it = settings->cbegin(); it != settings->cend(); ++it)
				{
					type = it->first;
					path = it->second;
					Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation(path, type, resourceGroupName);
				}

				if (false == resourceGroupName.empty())
				{
					Ogre::StringVectorPtr jsonFiles = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames(resourceGroupName, "*.json");
					if (jsonFiles->size() > 0)
					{
						jsonMaterialsCount += static_cast<unsigned int>(jsonFiles->size());
					}
				}
			}

			if (resourceGroupName != "Essential" && resourceGroupName != "General" && resourceGroupName != "PostProcessing" && resourceGroupName != "Hlms"
				&& resourceGroupName != "Internal" && resourceGroupName != "AutoDetect" && resourceGroupName != "Lua" && 
				resourceGroupName != "Audio" && resourceGroupName != "TerrainTextures" && resourceGroupName != "Projects" && resourceGroupName != "NOWA_Design" 
				&& false == resourceGroupName.empty())
			{
				this->resourceGroupNames.emplace(resourceGroupName);
			}
		}

		// Count the resource objects for progress visualisation
		// loadableObjectsCount = (jsonMaterialsCount - resourceGroupNames.size()) * 2;
		loadableObjectsCount = static_cast<unsigned int>(this->resourceGroupNames.size());
		if (loadableObjectsCount < 0)
			loadableObjectsCount = 0;
		
		// Initialize the essential resource group, in which the gui resources are located, so that a loading screen can be visualized
		try
		{
			Ogre::ResourceGroupManager::getSingletonPtr()->initialiseResourceGroup("Essential", true);
		}
		catch (Ogre::Exception& e)
		{
			Ogre::String description = e.getDescription();
			description += "\n\nPlease check your resources file, propably there are some resources missing: '" + this->resourcesName + "' If you do not know what to todo, copy the 'NOWA_Design.cfg' and rename it.\n\n";
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: " + description);
			throw Ogre::ItemIdentityException(1, description, "initialiseResourceGroup(Essential, true)", __FILE__, __LINE__);
		}

		// Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("TerrainTextures", false);

		// After that my gui must be initialized
		this->initMyGui(dummySceneManager, dummyCamera, logName);

		Ogre::CompositorManager2* compositorManager = this->root->getCompositorManager2();

		// After that all 2.0 compositors/pbs etc. parsed, because a MYGUI pass is also used in PbsMaterials.compositor
		try
		{
			Ogre::ResourceGroupManager::getSingletonPtr()->initialiseResourceGroup("General", true);
		}
		catch (Ogre::Exception& e)
		{
			Ogre::String description = e.getDescription();
			description += "\n\nPlease check your resources file, propably there are some resources missing: '" + this->resourcesName + "' If you do not know what to todo, copy the 'NOWA_Design.cfg' and rename it.\n\n";
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: " + description);
			throw Ogre::ItemIdentityException(1, description, "initialiseResourceGroup(General, true)", __FILE__, __LINE__);
		}

		// After that the NOWAPbsWorkspace can be added
		const Ogre::String workspaceName = "NOWAPbsWorkspace";
		const Ogre::IdString workspaceNameHash = workspaceName;
		
		this->myGuiWorkspace = compositorManager->addWorkspace(dummySceneManager, this->renderWindow->getTexture(), dummyCamera, workspaceNameHash, true);

		// Fps statistics container
		this->info = new diagnostic::StatisticInfo();
		this->info->setVisible(false);
		
		this->defaultEngineResourceListener = new EngineResourceRotateListener(this->renderWindow);
		this->defaultEngineResourceListener->showLoadingBar(1, 1, 1.0f / static_cast<Ogre::Real>(loadableObjectsCount));

		// Evil, as no sky boxes will work anymore!
#if 0
		std::vector<Ogre::String> filters = { "png", "jpg", "bmp", "tga", "gif", "tif", "dds" };

		for (auto& resourceGroupName : resourceGroupNames)
		{
			if ("Skies" == resourceGroupName)
				continue;
			// Ogre::StringVector extensions = Ogre::Codec::getExtensions();
			// for (Ogre::StringVector::iterator itExt = extensions.begin(); itExt != extensions.end(); ++itExt)
			for (auto& filter : filters)
			{
				Ogre::StringVectorPtr names = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames(resourceGroupName, "*." + filter/**itExt*/);
				for (Ogre::StringVector::iterator itName = names->begin(); itName != names->end(); ++itName)
				{
					Ogre::ResourceGroupManager::getSingleton().declareResource(*itName, "Texture", resourceGroupName);
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: Declared resource texture: " + *itName + " from group: " + resourceGroupName);
				}
			}
		}
#endif

		try
		{
			Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(true);
		}
		catch (Ogre::Exception& e)
		{
			Ogre::String description = e.getDescription();
			description += "\n\nPlease check your resources file, propably there are some resources missing: '" + this->resourcesName + "' If you do not know what to todo, copy the 'NOWA_Design.cfg' and rename it.\n\n";
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: " + description);
			throw Ogre::ItemIdentityException(1, description, "initialiseAllResourceGroups(true)", __FILE__, __LINE__);
		}

		// Initialize resources for LTC area lights and accurate specular reflections (IBL)
        Ogre::Hlms* hlms = this->root->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
        Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlms );
        try
        {
            hlmsPbs->loadLtcMatrix();
        }
        catch( Ogre::FileNotFoundException &e )
        {
            Ogre::LogManager::getSingleton().logMessage(e.getFullDescription(), Ogre::LML_CRITICAL);
            Ogre::LogManager::getSingleton().logMessage(
                "WARNING: LTC matrix textures could not be loaded. Accurate specular IBL reflections "
                "and LTC area lights won't be available or may not function properly!",
                Ogre::LML_CRITICAL);
        }

		
		// Create once custom temp textures for dither and half tone effect
		this->createCustomTextures();

		this->defaultEngineResourceListener->hideLoadingBar();

		// Remove workspace after resources had been parsed
		this->myGuiOgrePlatform->getRenderManagerPtr()->setSceneManager(nullptr);
		compositorManager->removeWorkspace(this->myGuiWorkspace);
		this->myGuiWorkspace = nullptr;

		dummySceneManager->removeRenderQueueListener(this->overlaySystem);
		dummySceneManager->destroyCamera(dummyCamera);
		dummyCamera = nullptr;
		this->root->destroySceneManager(dummySceneManager);
		dummySceneManager = nullptr;

		//this->renderWindow->setAutoUpdated(false);
		// this->renderWindow->setActive(true);

		// Create the plugin factory
		Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing PluginFactory ***");
		this->pluginFactory = new PluginFactory();
		Ogre::LogManager::getSingletonPtr()->logMessage("*** Finished: Initializing PluginFactory ***");

		// Force init the first time
		GameObjectFactory::getInstance();

		if (this->optionUseLuaScript)
		{
			// Create lua script and bind NOWA
			LuaScriptApi::getInstance()->init(this->optionLuaGroupName);

			LuaScriptApi::getInstance()->generateLuaApi();
		}

		if (this->optionUseLuaConsole && this->optionUseLuaScript)
		{
			// Create lua console
			new LuaConsole();
			LuaConsole::getSingletonPtr()->init(LuaScriptApi::getInstance()->getLua());
		}

		// Set default mask for all ogre objects
		Ogre::MovableObject::setDefaultQueryFlags(0 << 0);

		Ogre::TextureGpuManager* textureGpuManager = this->root->getRenderSystem()->getTextureGpuManager();
		textureGpuManager->setTextureGpuManagerListener(this);

		// Test:
		// textureGpuManager->dumpMemoryUsage(Ogre::LogManager::getSingletonPtr()->getDefaultLog());

#ifdef _DEBUG
//Debugging multithreaded code is a PITA, disable it.
		const size_t numThreads = 1;
#else
	//getNumLogicalCores() may return 0 if couldn't detect
		const size_t numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
#endif
		// Loads textures in background in multiple threads
		Ogre::TextureGpuManager* hlmsTextureManager = this->root->getRenderSystem()->getTextureGpuManager();
		hlmsTextureManager->setMultiLoadPool(numThreads);

		// setTightMemoryBudget();
		setRelaxedMemoryBudget();
		this->defaultBudget = textureGpuManager->getBudget();

#if OGRE_PROFILING
		Ogre::Profiler::getSingleton().setEnabled(true);
#if OGRE_PROFILING == OGRE_PROFILING_INTERNAL
		Ogre::Profiler::getSingleton().endProfile("");
#endif
#if OGRE_PROFILING == OGRE_PROFILING_INTERNAL_OFFLINE
		Ogre::Profiler::getSingleton().getOfflineProfiler().setDumpPathsOnShutdown(
			this->writeAccessFolder + "ProfilePerFrame", this->writeAccessFolder + "ProfileAccum");
#endif
#endif
		
		return true;
	}

	void Core::registerHlms(void)
	{
		Ogre::String dataFolder = "../../media/";
		Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();

		Ogre::String shaderSyntax = "GLSL";
		if (renderSystem->getName() == "OpenGL ES 2.x Rendering Subsystem")
		{
			shaderSyntax = "GLSLES";
		}
		if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
		{
			shaderSyntax = "HLSL";
		}
		else if (renderSystem->getName() == "Metal Rendering Subsystem")
		{
			shaderSyntax = "Metal";
		}

		//At this point rootHlmsFolder should be a valid path to the Hlms data folder

		Ogre::HlmsUnlit* hlmsUnlit = nullptr;
		Ogre::HlmsPbs* hlmsPbs = nullptr;
		Ogre::HlmsTerra* hlmsTerra = nullptr;
		HlmsWind* hlmsWind = nullptr;
#if 0
		Ogre::HlmsOcean* hlmsOcean = nullptr;
#endif

		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();

		//For retrieval of the paths to the different folders needed
		Ogre::String mainFolderPath;
		Ogre::StringVector libraryFoldersPaths;
		Ogre::StringVector::const_iterator libraryFolderPathIt;
		Ogre::StringVector::const_iterator libraryFolderPathEn;

		Ogre::ArchiveManager& archiveManager = Ogre::ArchiveManager::getSingleton();

		// HlmsUnlit
		{
			//Get the path to all the subdirectories used by HlmsUnlit
			Ogre::HlmsUnlit::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			Ogre::Archive* archiveUnlit = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);
			Ogre::ArchiveVec archiveUnlitLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive* archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archiveUnlitLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}

			//Create and register the unlit Hlms
			hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit(archiveUnlit, &archiveUnlitLibraryFolders);
			hlmsManager->registerHlms(hlmsUnlit);
		}

		// HlmsPbs
		{
			Ogre::HlmsPbs::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			Ogre::Archive* archivePbs = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);

			//Get the library archive(s)
			Ogre::ArchiveVec archivePbsLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive *archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archivePbsLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}
#if 0
			// If no fog is used in scene, causes white flickering, hence it has been deactivated
			Ogre::Archive* archiveFog = archiveManager.load(dataFolder + "Hlms/Fog/Any", "FileSystem", true);
			archivePbsLibraryFolders.push_back(archiveFog);
#endif

			// Create and register
			hlmsPbs = OGRE_NEW Ogre::HlmsPbs(archivePbs, &archivePbsLibraryFolders);
			
			this->baseListenerContainer = new HlmsBaseListenerContainer();
			hlmsPbs->setListener(this->baseListenerContainer);
			hlmsManager->registerHlms(hlmsPbs);

			// hlmsPbs->setIndustryCompatible(true);
		}

		// HlmsTerra
		{
			// Note: Terra uses HLMS_USER3

			//Get the path to all the subdirectories used by HlmsTerra
			Ogre::HlmsTerra::getDefaultPaths(mainFolderPath, libraryFoldersPaths);

			Ogre::Archive* archiveTerra = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);

			Ogre::ArchiveVec archiveTerraLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive* archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archiveTerraLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}

			//Create and register the terra Hlms
			hlmsTerra = OGRE_NEW Ogre::HlmsTerra(archiveTerra, &archiveTerraLibraryFolders);
			hlmsManager->registerHlms(hlmsTerra);

			//Add Terra's piece files that customize the PBS implementation.
			//These pieces are coded so that they will be activated when
			//we set the HlmsPbsTerraShadows listener and there's an active Terra
			//(see Tutorial_TerrainGameState::createScene01)
			Ogre::Hlms* hlmsPbs = hlmsManager->getHlms(Ogre::HLMS_PBS);
			Ogre::Archive* archivePbs = hlmsPbs->getDataFolder();
			Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
			libraryPbs.push_back(Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows", "FileSystem", true));
			hlmsPbs->reloadFrom(archivePbs, &libraryPbs);
		}

		// HlmsWind
		{
			// Note: HlmsWind uses HLMS_USER0
			HlmsWind::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			
			Ogre::ArchiveVec archive;
 
			for (Ogre::String str : libraryFoldersPaths)
			{
				Ogre::Archive* archiveLibrary = Ogre::ArchiveManager::getSingleton().load(dataFolder + str, "FileSystem", true);
				archive.push_back(archiveLibrary);
			}
 
			Ogre::Archive* archivePbs = Ogre::ArchiveManager::getSingleton().load(dataFolder + mainFolderPath, "FileSystem", true);
			// Create and register the wind Hlms
			hlmsWind = OGRE_NEW HlmsWind(archivePbs, &archive);
			hlmsManager->registerHlms(hlmsWind);
		}

#if 0
		// Ocean
		{
			// Note: Ocean uses HLMS_USER2
			// ATTENTION: GPUParticles already uses 2!! Check for other options
			//// Register Ocean
			//// https://forums.ogre3d.org/viewtopic.php?f=25&t=93592&hilit=ocean

			//// Note a the moment only OpenGL available
			//Ogre::Archive* archiveOcean = Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/GLSL", "FileSystem", true);
			//hlmsOcean = OGRE_NEW Ogre::HlmsOcean(archiveOcean, &libraryPbs);
			//hlmsManager->registerHlms(hlmsOcean);
			//WorkspaceModule::getInstance()->setHlmsOcean(hlmsOcean);
			//hlmsOcean->setDebugOutputPath(false, false);

			//Ogre::Archive* archiveLibraryCustom = Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/GLSL/Custom", "FileSystem", true);
			//libraryPbs.push_back(archiveLibraryCustom);
			//hlmsPbs->reloadFrom(archivePbs, &libraryPbs);

			/*
			when you have a reflection probe, set it
			Ogre::HlmsOcean* hlmsOcean = static_cast<Ogre::HlmsOcean*>( Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms( Ogre::HLMS_USER2 ) );
			hlmsOcean->setEnvProbe( probeTexture );
			*/

// Attention Debug and check this, if all pathes are correct
			//Create & Register HlmsTerra
			//Get the path to all the subdirectories used by HlmsTerra
			Ogre::HlmsOcean::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			Ogre::Archive* archiveOcean = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);

			Ogre::ArchiveVec archiveOceanLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive *archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archiveOceanLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}

			//Create and register the terra Hlms
			hlmsOcean = OGRE_NEW Ogre::HlmsOcean(archiveOcean, &archiveOceanLibraryFolders);
			hlmsManager->registerHlms(hlmsOcean);

			//Add Terra's piece files that customize the PBS implementation.
			//These pieces are coded so that they will be activated when
			//we set the HlmsPbsTerraShadows listener and there's an active Terra
			//(see Tutorial_TerrainGameState::createScene01)
			Ogre::Hlms* hlmsPbs = hlmsManager->getHlms(Ogre::HLMS_PBS);
			Ogre::Archive* archivePbs = hlmsPbs->getDataFolder();
			Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
			libraryPbs.push_back(Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/" + shaderSyntax + "/Custom", "FileSystem", true));
			hlmsPbs->reloadFrom(archivePbs, &libraryPbs);

		}
#endif

		// HlmsParticles
		{
			// Note: HlmsParticles uses HLMS_USER2

			//Create and register the Gpu Particles Hlms
			HlmsParticle::registerHlms(dataFolder, dataFolder);
		}

		if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
		{
			//Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
			//and below to avoid saturating AMD's discard limit (8MB) or
			//saturate the PCIE bus in some low end machines.
			bool supportsNoOverwriteOnTextureBuffers;
			renderSystem->getCustomAttribute("MapNoOverwriteOnDynamicBufferSRV", &supportsNoOverwriteOnTextureBuffers);

			if (!supportsNoOverwriteOnTextureBuffers)
			{
				hlmsPbs->setTextureBufferDefaultSize(512 * 1024);
				hlmsUnlit->setTextureBufferDefaultSize(512 * 1024);
				hlmsTerra->setTextureBufferDefaultSize(512 * 1024);
				hlmsWind->setTextureBufferDefaultSize(512 * 1024);
#if 0
				hlmsOcean->setTextureBufferDefaultSize(512 * 1024);
#endif
			}
		}

		// Disable the nasty shader cache files creation on exe dir
		hlmsPbs->setDebugOutputPath(false, false);
		hlmsUnlit->setDebugOutputPath(false, false);
		hlmsTerra->setDebugOutputPath(false, false);

		// Set shader cache for faster loading
		this->loadHlmsDiskCache();

		/*
		These pool IDs must be unique per texture array, to ensure textures go into the
		right array.
		However you'll see here that both decalDiffuseId & decalNormalId are the same.
		This is because normal maps, being TEXTURE_TYPE_NORMALS and of a different format
		(RG8_SNORM instead of RGBA8_UNORM) will end up in different arrays anyway.
		So the reasons they will end up in different pools anyway is because:
		1. One uses TEXTURE_TYPE_DIFFUSE, the other TEXTURE_TYPE_NORMALS
		2. One uses RGBA8_UNORM, the other RG8_SNORM

		For example if you want to load diffuse decals into one array, and textures for
		area lights into a different array, then you would use different pool IDs:
		decalDiffuseId = 1;
		areaLightsId = 2;
		*/

		// Attention: Not required anymore?
#if 0
		 const Ogre::uint32 decalDiffuseId = 1;
		 const Ogre::uint32 decalNormalId = 1;

		 Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		 textureManager->reservePoolId(decalDiffuseId, 512u, 512u, 8u, 10u, Ogre::PFG_RGBA8_UNORM_SRGB);
		 textureManager->reservePoolId(decalNormalId, 512u, 512u, 8u, 10u, Ogre::PFG_RG8_SNORM);

		/*
		Create a blank diffuse & normal map textures, so we can use index 0 to "disable" them
		if we want them disabled for a particular Decal.
		This is not necessary if you intend to have all your decals using both diffuse
		and normals.
		*/
		 Ogre::uint8 *blackBuffer = reinterpret_cast<Ogre::uint8*>(
			 OGRE_MALLOC_SIMD(512u * 512u * 4u,
				 Ogre::MEMCATEGORY_RESOURCE));
		 memset(blackBuffer, 0, 512u * 512u * 4u);
		 Ogre::Image2 blackImage;
		 blackImage.loadDynamicImage(blackBuffer, 512u, 512u, 1u, Ogre::TextureTypes::Type2D, Ogre::PFG_RGBA8_UNORM_SRGB, true);

		 blackImage.generateMipmaps(false, Ogre::Image2::FILTER_NEAREST);
		 Ogre::TextureGpu *decalTexture = 0;

		 decalTexture = textureManager->createOrRetrieveTexture("decals_disabled_diffuse", Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::AutomaticBatching |
			 Ogre::TextureFlags::ManualTexture,
			 Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, decalDiffuseId);

		 decalTexture->setResolution(blackImage.getWidth(), blackImage.getHeight());
		 decalTexture->setNumMipmaps(blackImage.getNumMipmaps());
		 decalTexture->setPixelFormat(blackImage.getPixelFormat());
		 decalTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);

		 blackImage.uploadTo(decalTexture, 0, decalTexture->getNumMipmaps() - 1u);

		 blackImage.freeMemory();
		 blackBuffer = reinterpret_cast<Ogre::uint8*>(OGRE_MALLOC_SIMD(512u * 512u * 2u, Ogre::MEMCATEGORY_RESOURCE));
		 memset(blackBuffer, 0, 512u * 512u * 2u);
		 blackImage.loadDynamicImage(blackBuffer, 512u, 512u, 1u, Ogre::TextureTypes::Type2D,
			 Ogre::PFG_RG8_SNORM, true);
		 blackImage.generateMipmaps(false, Ogre::Image2::FILTER_NEAREST);
		 decalTexture = textureManager->createOrRetrieveTexture("decals_disabled_normals",
			 Ogre::GpuPageOutStrategy::Discard,
			 Ogre::TextureFlags::AutomaticBatching |
			 Ogre::TextureFlags::ManualTexture,
			 Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, decalNormalId);

		 decalTexture->setResolution(blackImage.getWidth(), blackImage.getHeight());
		 decalTexture->setNumMipmaps(blackImage.getNumMipmaps());
		 decalTexture->setPixelFormat(blackImage.getPixelFormat());
		 decalTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);

		 blackImage.uploadTo(decalTexture, 0, decalTexture->getNumMipmaps() - 1u);

		 /*
			 Now actually create the decals into the array with the pool ID we desire.
		 */
		 textureManager->createOrRetrieveTexture("floor_diffuse.PNG", Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::Diffuse,
			 Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, decalDiffuseId);

		 textureManager->createOrRetrieveTexture("floor_bump.PNG", Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::NormalMap,
			 Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, decalNormalId);
#endif
	}

	void Core::refreshHlms(bool useFog, bool useWind, bool useTerra, bool useOcean)
	{
		Ogre::String dataFolder = "../../media/";
		Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();

		Ogre::String shaderSyntax = "GLSL";
		if (renderSystem->getName() == "OpenGL ES 2.x Rendering Subsystem")
		{
			shaderSyntax = "GLSLES";
		}
		if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
		{
			shaderSyntax = "HLSL";
		}
		else if (renderSystem->getName() == "Metal Rendering Subsystem")
		{
			shaderSyntax = "Metal";
		}


#if 0
		Ogre::HlmsOcean* hlmsOcean = nullptr;
#endif

		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();

		//For retrieval of the paths to the different folders needed
		Ogre::String mainFolderPath;
		Ogre::StringVector libraryFoldersPaths;
		Ogre::StringVector::const_iterator libraryFolderPathIt;
		Ogre::StringVector::const_iterator libraryFolderPathEn;

		Ogre::ArchiveManager& archiveManager = Ogre::ArchiveManager::getSingleton();

		// HlmsPbs
		{
			Ogre::HlmsPbs::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			Ogre::Archive* archivePbs = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);

			//Get the library archive(s)
			Ogre::ArchiveVec archivePbsLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive *archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archivePbsLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}

			if (true == useFog)
			{
				Ogre::Archive* archiveFog = archiveManager.load(dataFolder + "Hlms/Fog/Any", "FileSystem", true);
				archivePbsLibraryFolders.push_back(archiveFog);
			}
			Ogre::Hlms* hlmsPbs = hlmsManager->getHlms(Ogre::HLMS_PBS);
			hlmsPbs->reloadFrom(archivePbs, &archivePbsLibraryFolders);
		}

		// HlmsTerra
		if (useTerra)
		{
			// Note: Terra uses HLMS_USER3

			//Get the path to all the subdirectories used by HlmsTerra
			Ogre::HlmsTerra::getDefaultPaths(mainFolderPath, libraryFoldersPaths);

			Ogre::Archive* archiveTerra = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);

			Ogre::ArchiveVec archiveTerraLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive* archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archiveTerraLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}

			//Add Terra's piece files that customize the PBS implementation.
			//These pieces are coded so that they will be activated when
			//we set the HlmsPbsTerraShadows listener and there's an active Terra
			//(see Tutorial_TerrainGameState::createScene01)
			Ogre::Hlms* hlmsPbs = hlmsManager->getHlms(Ogre::HLMS_PBS);
			Ogre::Archive* archivePbs = hlmsPbs->getDataFolder();
			Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
			libraryPbs.push_back(Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows", "FileSystem", true));
			hlmsPbs->reloadFrom(archivePbs, &libraryPbs);
		}

		// HlmsWind
		if (true == useWind)
		{
			// Note: HlmsWind uses HLMS_USER0
			HlmsWind::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			
			Ogre::ArchiveVec archive;
 
			for (Ogre::String str : libraryFoldersPaths)
			{
				Ogre::Archive* archiveLibrary = Ogre::ArchiveManager::getSingleton().load(dataFolder + str, "FileSystem", true);
				archive.push_back(archiveLibrary);
			}
 
			Ogre::Archive* archivePbs = Ogre::ArchiveManager::getSingleton().load(dataFolder + mainFolderPath, "FileSystem", true);
			Ogre::Hlms* hlmsPbs = hlmsManager->getHlms(Ogre::HLMS_PBS);
			hlmsPbs->reloadFrom(archivePbs, &archive);
		}

#if 0
		// Ocean
		{
			// Note: Ocean uses HLMS_USER2
			//// Register Ocean
			//// https://forums.ogre3d.org/viewtopic.php?f=25&t=93592&hilit=ocean

			//// Note a the moment only OpenGL available
			//Ogre::Archive* archiveOcean = Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/GLSL", "FileSystem", true);
			//hlmsOcean = OGRE_NEW Ogre::HlmsOcean(archiveOcean, &libraryPbs);
			//hlmsManager->registerHlms(hlmsOcean);
			//WorkspaceModule::getInstance()->setHlmsOcean(hlmsOcean);
			//hlmsOcean->setDebugOutputPath(false, false);

			//Ogre::Archive* archiveLibraryCustom = Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/GLSL/Custom", "FileSystem", true);
			//libraryPbs.push_back(archiveLibraryCustom);
			//hlmsPbs->reloadFrom(archivePbs, &libraryPbs);

			/*
			when you have a reflection probe, set it
			Ogre::HlmsOcean* hlmsOcean = static_cast<Ogre::HlmsOcean*>( Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms( Ogre::HLMS_USER2 ) );
			hlmsOcean->setEnvProbe( probeTexture );
			*/

// Attention Debug and check this, if all pathes are correct
			//Create & Register HlmsTerra
			//Get the path to all the subdirectories used by HlmsTerra
			Ogre::HlmsOcean::getDefaultPaths(mainFolderPath, libraryFoldersPaths);
			Ogre::Archive* archiveOcean = archiveManager.load(dataFolder + mainFolderPath, "FileSystem", true);

			Ogre::ArchiveVec archiveOceanLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while (libraryFolderPathIt != libraryFolderPathEn)
			{
				Ogre::Archive *archiveLibrary = archiveManager.load(dataFolder + *libraryFolderPathIt, "FileSystem", true);
				archiveOceanLibraryFolders.push_back(archiveLibrary);
				++libraryFolderPathIt;
			}

			//Create and register the terra Hlms
			hlmsOcean = OGRE_NEW Ogre::HlmsOcean(archiveOcean, &archiveOceanLibraryFolders);
			hlmsManager->registerHlms(hlmsOcean);

			//Add Terra's piece files that customize the PBS implementation.
			//These pieces are coded so that they will be activated when
			//we set the HlmsPbsTerraShadows listener and there's an active Terra
			//(see Tutorial_TerrainGameState::createScene01)
			Ogre::Hlms* hlmsPbs = hlmsManager->getHlms(Ogre::HLMS_PBS);
			Ogre::Archive* archivePbs = hlmsPbs->getDataFolder();
			Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
			libraryPbs.push_back(Ogre::ArchiveManager::getSingletonPtr()->load(dataFolder + "Hlms/Ocean/" + shaderSyntax + "/Custom", "FileSystem", true));
			hlmsPbs->reloadFrom(archivePbs, &libraryPbs);

		}
#endif
	}

	void Core::initMyGui(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::String& logName)
	{
		this->myGuiOgrePlatform = new MyGUI::Ogre2Platform();
		// Attention: Must be initilialized for all groups and not only for essential, else only images from essential are known!
		this->myGuiOgrePlatform->initialise(this->renderWindow, sceneManager, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, logName);

		this->myGui = new MyGUI::Gui();
		this->myGui->initialise("MyGUI_Core.xml");

		// MyGUI::ResourceManager::getInstancePtr()->load("FrameworkMicroFont.xml");
		// MyGUI::FontManager::getInstancePtr()->setDefaultFont("MicroFont_11");

		MyGUI::ResourceManager::getInstancePtr()->load("FrameworkFonts.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("NOWA_Font.xml");
		MyGUI::FontManager::getInstancePtr()->setDefaultFont("NOWA_10_Font");
		MyGUI::ResourceManager::getInstancePtr()->load("EditorLanguage.xml");
		MyGUI::LanguageManager::getInstancePtr()->loadUserTags("EditorLanguageEnglishTag.xml");
		MyGUI::LanguageManager::getInstancePtr()->loadUserTags("EditorLanguageGermanTag.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_PointerImages.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_Pointers.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("DemoPointersW32.xml");
		
		// MyGUI::ResourceManager::getInstancePtr()->load("FrameworkMicroFont.xml");
		
		// MyGUI::ResourceManager::getInstancePtr()->setDefaultFont("DejaVuSansFont_13");
		

		if (0 == this->optionLanguage)
		{
			MyGUI::LanguageManager::getInstance().setCurrentLanguage("English");
		}
		else if (1 == this->optionLanguage)
		{
			MyGUI::LanguageManager::getInstance().setCurrentLanguage("German");
		}
		// Later add method to choose skin
		// MyGUI_BlueWhiteSkins.xml
		// MyGUI_BlackBlueSkins.xml
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_BlackBlueSkins.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_Skin_BlackTheme.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_Template_BlackTheme.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_DarkTemplate.xml");
		MyGUI::ResourceManager::getInstancePtr()->load("MyGUI_DarkSkin.xml");
	}

	void Core::setSceneManagerForMyGuiPlatform(Ogre::SceneManager* sceneManager)
	{
		this->myGuiOgrePlatform->getRenderManagerPtr()->setSceneManager(sceneManager);
	}

	void Core::setCurrentWorldPath(const Ogre::String& currentWorldPath)
	{
		this->currentWorldPath = currentWorldPath;
		size_t found = currentWorldPath.find_last_of("/\\");
		if (Ogre::String::npos != found)
		{
			this->currentProjectPath = currentWorldPath.substr(0, currentWorldPath.find_last_of("/\\"));
			this->worldName = this->currentWorldPath.substr(found + 1, this->currentWorldPath.size() - found - 7);
		}

	}

	Ogre::String Core::getCurrentWorldPath(void) const
	{
		return this->currentWorldPath;
	}

	Ogre::String Core::getCurrentProjectPath(void) const
	{
		return this->currentProjectPath;
	}
	
	void Core::setProjectName(const Ogre::String& projectName)
	{
		this->projectName = projectName;
	}
		
	Ogre::String Core::getProjectName(void) const
	{
		return this->projectName;
	}

	Ogre::String Core::getWorldName(void) const
	{
		return this->worldName;
	}

	void Core::setCurrentWorldBounds(const Ogre::Vector3& mostLeftNearPosition, const Ogre::Vector3& mostRightFarPosition)
	{
		this->mostLeftNearPosition = mostLeftNearPosition;
		this->mostRightFarPosition = mostRightFarPosition;

		// Send event and set bounds for current world
		boost::shared_ptr<EventDataBoundsUpdated> eventDataBoundsUpdated(boost::make_shared<EventDataBoundsUpdated>(this->mostLeftNearPosition, this->mostRightFarPosition));
		AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataBoundsUpdated);
	}

	Ogre::Vector3 Core::getCurrentWorldBoundLeftNear(void)
	{
		return this->mostLeftNearPosition;
	}

	Ogre::Vector3 Core::getCurrentWorldBoundRightFar(void)
	{
		return this->mostRightFarPosition;
	}

	bool Core::getIsWorldBeingDestroyed(void) const
	{
		return true == AppStateManager::getSingletonPtr()->getIsShutdown() || true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying();
	}

	bool Core::getIsGame(void) const
	{
		return this->isGame;
	}

	PluginFactory* Core::getPluginFactory(void) const
	{
		return this->pluginFactory;
	}

	Ogre::Camera* Core::createCameraWithMenuSettings(Ogre::SceneManager* sceneManager, const Ogre::String& strCameraName)
	{
		Ogre::Camera *camera = sceneManager->createCamera(strCameraName);
		// set query flags to unused mask
		camera->setQueryFlags(0 << 0);

		camera->setNearClipDistance(0.01f);
		//camera->setAutoAspectRatio(true);
		camera->setLodBias(this->optionLODBias);
		// Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(0);
		// Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(static_cast<Ogre::TextureFilterOptions>(this->optionTextureFiltering));
		// Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(this->optionAnisotropyLevel);

		return camera;
	}

	void Core::setMenuSettingsForCamera(Ogre::Camera* camera)
	{
		// camera->setNearClipDistance(0.01f);
// to be removed
		// camera->setFarClipDistance(this->optionViewRange);
		//camera->setAutoAspectRatio(true);
		camera->setLodBias(this->optionLODBias);
		// Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(0);
		// Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering((Ogre::TextureFilterOptions)this->optionTextureFiltering);
		// Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(this->optionAnisotropyLevel);
	}

	void Core::setPolygonMode(unsigned short mode)
	{
		Ogre::HlmsPbs* hlmsPbs =  dynamic_cast<Ogre::HlmsPbs*>(NOWA::Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getHlms(Ogre::HLMS_PBS));
		if (nullptr != hlmsPbs)
		{
			Ogre::Hlms::HlmsDatablockMap::const_iterator itor = hlmsPbs->getDatablockMap().begin();
			Ogre::Hlms::HlmsDatablockMap::const_iterator end = hlmsPbs->getDatablockMap().end();
			while (itor != end)
			{
				HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(itor->second.datablock);
				Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
				macroblock.mPolygonMode = static_cast<Ogre::PolygonMode>(mode);

				const Ogre::HlmsMacroblock* finalMacroblock = root->getHlmsManager()->getMacroblock(macroblock);
				datablock->setMacroblock(finalMacroblock);

				++itor;
			}
		}
	}

	void Core::changeWindowTitle(const Ogre::String& title)
	{
		const char* charTitle = title.c_str();

#ifdef WIN32
		HWND hWnd;
		this->renderWindow->getCustomAttribute("WINDOW", &hWnd);
		//LPCWSTR titleFoo = TEXT(title);
		SetWindowText(hWnd, title.c_str());
#else
		// FIXME
		//    XStoreName(display, frame_window, charTitle);
#endif
	}

	Ogre::String Core::getApplicationName(void) const
	{
		return this->title;
	}

	void Core::setWindowPosition(int x, int y)
	{
		boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);

		RECT desktop;
		// Get a handle to the desktop window
		const HWND hDesktop = GetDesktopWindow();
		// Get the size of screen to the variable desktop
		GetWindowRect(hDesktop, &desktop);
		// The top left corner will have coordinates (0,0) and the bottom right corner will have coordinates (horizontal, vertical)
		// int horizontal = desktop.right;
		// int vertical = desktop.bottom;

		HWND handle;
		NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &handle);
		MoveWindow(handle, x, y, NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getHeight(), true);
	}

	void Core::getDesktopSize(int& width, int& height)
	{
		RECT desktop;
		// Get a handle to the desktop window
		const HWND hDesktop = GetDesktopWindow();
		// Get the size of screen to the variable desktop
		GetWindowRect(hDesktop, &desktop);
		// The top left corner will have coordinates (0,0) and the bottom right corner will have coordinates (horizontal, vertical)
		width = desktop.right;
		height = desktop.bottom;
	}

	void Core::moveWindowToTaskbar(void)
	{
		if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 0)
		{
			boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);
		}
		HWND handle;
		NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &handle);
		ShowWindow(handle, SW_FORCEMINIMIZE);
		//AnimateWindow(handle, 200, AW_BLEND);
	}

	size_t Core::getWindowHandle(void) const
	{
		size_t hWnd;
		NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &hWnd);
		return hWnd;
	}

	void Core::moveWindowToFront(void)
	{
		if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 0)
		{
			boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);
		}

		HWND handle;
		NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &handle);
		SetWindowPos(handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
	}

	void Core::windowResized(Ogre::Window* renderWindow)
	{
		if (renderWindow == this->renderWindow)
		{
			if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 0)
			{
				boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
				AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);
			}

			unsigned int width, height;
			int left, top;
			renderWindow->getMetrics(width, height, left, top);

			const OIS::MouseState& ms = InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
			ms.width = width;
			ms.height = height;
		}
	}

	void Core::windowMoved(Ogre::Window* renderWindow)
	{
		if (renderWindow == this->renderWindow)
		{
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
			this->renderWindow->requestResolution(renderWindow->getWidth(), renderWindow->getHeight());
#endif
			this->renderWindow->windowMovedOrResized();

			if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 0)
			{
				boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);
			}
			// this->root->getRenderSystem()->validateDevice(true);
		}
	}

	bool Core::windowClosing(Ogre::Window* renderWindow)
	{
		if (renderWindow == this->renderWindow)
		{
			if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 0)
			{
				boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);
			}
		}
		// this->root->getRenderSystem()->validateDevice(true);
		return false;
	}

	void Core::windowClosed(Ogre::Window* renderWindow)
	{
		if (renderWindow == this->renderWindow)
		{
		// Attention: Is this necessary?
#if 0
		// this->root->getRenderSystem()->validateDevice(true);

			// Only close for window that created OIS (the main window in these demos)
			if (this->inputManager)
			{
				this->inputManager->destroyInputObject(this->mouse);
				this->inputManager->destroyInputObject(this->keyboard);

				OIS::InputManager::destroyInputSystem(this->inputManager);
				this->inputManager = nullptr;
			}
#endif
		}
	}

	void Core::windowFocusChange(Ogre::Window* renderWindow)
	{
		if (renderWindow == this->renderWindow)
		{
			this->renderWindow->setFocused(renderWindow->isFocused());

			if (AppStateManager::getSingletonPtr() && AppStateManager::getSingletonPtr()->getAppStatesCount() > 0)
			{
				if (nullptr != NOWA::AppStateManager::getSingletonPtr()->getEventManager())
				{
					boost::shared_ptr<EventDataWindowChanged> eventDataWindowChanged(new EventDataWindowChanged());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWindowChanged);
				}
			}
			// this->root->getRenderSystem()->validateDevice(true);
		}
	}

	bool Core::checkStorage(const DWORDLONG diskSpaceNeeded, DWORDLONG& thisDiscSpace)
	{
		ULARGE_INTEGER uliFreeBytesAvailableToCaller;
		ULARGE_INTEGER uliTotalNumberOfBytes;
		ULARGE_INTEGER uliTotalNumberOfFreeBytes;

		::GetDiskFreeSpaceEx(nullptr, &uliFreeBytesAvailableToCaller, &uliTotalNumberOfBytes, &uliTotalNumberOfFreeBytes);

		thisDiscSpace = static_cast<__int64>(uliFreeBytesAvailableToCaller.QuadPart / (1024 * 1024));
		if (thisDiscSpace < diskSpaceNeeded)
		{
			// if you get here you do ott have enough disk space!
			// throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[Core] Not enough physical space.\n", "NOWA");
			return false;
		}
		return true;
	}

	//
	// CheckMemory							- Chapter 5, page 139
	//
	bool Core::checkMemory(const DWORDLONG physicalRAMNeeded, const DWORDLONG virtualRAMNeeded, DWORDLONG& thisRAMPhysical, DWORDLONG& thisRAMVirtual)
	{
		bool success = true;

		MEMORYSTATUSEX status;
		GlobalMemoryStatusEx(&status);

		MEMORYSTATUSEX statex;
		// call the function with the correct size
		statex.dwLength = sizeof(statex);

		GlobalMemoryStatusEx(&statex);

		//_tprintf(TEXT("There is  %*ld percent of memory in use.\n"),
		//	WIDTH, statex.dwMemoryLoad);
		//_tprintf(TEXT("There are %*I64d total KB of physical memory.\n"),
		//	WIDTH, statex.ullTotalPhys / DIV);
		//_tprintf(TEXT("There are %*I64d free  KB of physical memory.\n"),
		//	WIDTH, statex.ullAvailPhys / DIV);
		//_tprintf(TEXT("There are %*I64d total KB of paging file.\n"),
		//	WIDTH, statex.ullTotalPageFile / DIV);
		//_tprintf(TEXT("There are %*I64d free  KB of paging file.\n"),
		//	WIDTH, statex.ullAvailPageFile / DIV);
		//_tprintf(TEXT("There are %*I64d total KB of virtual memory.\n"),
		//	WIDTH, statex.ullTotalVirtual / DIV);
		//_tprintf(TEXT("There are %*I64d free  KB of virtual memory.\n"),
		//	WIDTH, statex.ullAvailVirtual / DIV);

		//// Show the amount of extended memory available.

		//_tprintf(TEXT("There are %*I64d free  KB of extended memory.\n"),
		//	WIDTH, statex.ullAvailExtendedVirtual / DIV);

		thisRAMPhysical = statex.ullAvailPhys / (1024 * 1024);
		if (thisRAMPhysical < physicalRAMNeeded)
		{
			// you do not have enough physical memory. Tell the player to go get a real 
			// computer and give this one to his mother. 
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: CheckMemory Failure: Not enough physical memory.");
			success = false;
		}

		thisRAMVirtual = statex.ullAvailVirtual / (1024 * 1024);
		// Check for enough free memory.
		if (thisRAMVirtual < virtualRAMNeeded)
		{
			// you don’t have enough virtual memory available. 
			// Tell the player to shut down the copy of Visual Studio running in the
			// background, or whatever seems to be sucking the memory dry.
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: CheckMemory Failure: Not enough virtual memory.");
			success = false;
		}

		char *buff = new char[(unsigned int)virtualRAMNeeded];
		if (buff)
		{
			delete[] buff;
		}
		else
		{
			// even though there is enough memory, it isn’t available in one 
			// block, which can be critical for games that manage their own memory
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core]: CheckMemory Failure: Not enough contiguous available memory.");
			success = false;
		}
		return success;
	}

	DWORD Core::readCPUSpeed()
	{
		DWORD BufSize = sizeof(DWORD);
		DWORD dwMHz = 0;
		DWORD type = REG_DWORD;
		HKEY hKey;

		// open the key where the proc speed is hidden:
		long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
			0, KEY_READ, &hKey);

		if (lError == ERROR_SUCCESS)
		{
			// query the key:
			RegQueryValueEx(hKey, "~MHz", NULL, &type, (LPBYTE)&dwMHz, &BufSize);
		}
		return dwMHz;
	}
#ifdef WIN32
	int Core::getCurrentThreadCount()
	{
		// First determine the id of the current process
		DWORD const id = GetCurrentProcessId();

		// Then get a process list snapshot.
		HANDLE const snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

		// Initialize the process entry structure.
		PROCESSENTRY32 entry = { 0 };
		entry.dwSize = sizeof(entry);

		// Get the first process info.
		BOOL  ret = true;
		ret = Process32First(snapshot, &entry);
		while (ret && entry.th32ProcessID != id)
		{
			ret = Process32Next(snapshot, &entry);
		}
		CloseHandle(snapshot);
		return ret ? entry.cntThreads : -1;
	}
#endif

	bool Core::isOnlyInstance(LPCTSTR title)
	{
		// Find the window.  If active, set and return false
		// Only one game instance may have this mutex at a time...

		HANDLE handle = CreateMutex(NULL, TRUE, title);

		// Does anyone else think 'ERROR_SUCCESS' is a bit of an oxymoron?
		if (GetLastError() != ERROR_SUCCESS)
		{
			HWND hWnd = FindWindow(title, NULL);
			if (hWnd)
			{
				// An instance of your game is already running.
				ShowWindow(hWnd, SW_SHOWNORMAL);
				SetFocus(hWnd);
				SetForegroundWindow(hWnd);
				SetActiveWindow(hWnd);
				return false;
			}
		}
		return true;
	}

	// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

	void PrintProcessNameAndID( DWORD processID )
	{
		TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

		// Get a handle to the process.

		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
									   PROCESS_VM_READ,
									   FALSE, processID );

		// Get the process name.

		if (NULL != hProcess )
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
				 &cbNeeded) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, 
								   sizeof(szProcessName)/sizeof(TCHAR) );
			}
		}

		// Print the process name and identifier.

		// _tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--------------->: " + Ogre::String(szProcessName));

		// Release the handle to the process.

		CloseHandle( hProcess );
	}

	bool Core::isProcessRunning(const TCHAR* const executableName)
	{
#if 0
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (!Process32First(snapshot, &entry))
		{
			CloseHandle(snapshot);
			return false;
		}

		do
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--------------->: " + Ogre::String(entry.szExeFile));

			if (!_tcsicmp(entry.szExeFile, executableName))
			{
				CloseHandle(snapshot);
				return true;
			}
		} while (Process32Next(snapshot, &entry));

		CloseHandle(snapshot);
		return false;
#endif
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;

		if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		{
			return 1;
		}


		// Calculate how many process identifiers were returned.

		cProcesses = cbNeeded / sizeof(DWORD);

		// Print the name and process identifier for each process.

		for ( i = 0; i < cProcesses; i++ )
		{
			if( aProcesses[i] != 0 )
			{
				PrintProcessNameAndID( aProcesses[i] );
			}
		}

		return false;
	}

	Ogre::String Core::getSaveGameDirectory(const Ogre::String& saveName)
	{
#ifdef WIN32
		HWND hWnd;
		this->renderWindow->getCustomAttribute("WINDOW", &hWnd);
		
		HRESULT hr;
		static char saveGameDirectory[MAX_PATH];
		TCHAR userDataPath[MAX_PATH];

		hr = SHGetSpecialFolderPath(hWnd, userDataPath, CSIDL_APPDATA, true);

		strcpy(saveGameDirectory, userDataPath);
		strcat(saveGameDirectory, "\\");
		strcat(saveGameDirectory, saveName.data());

		// Does our directory exist?
		if (0xffffffff == GetFileAttributes(saveGameDirectory))
		{
			if (SHCreateDirectoryEx(hWnd, saveGameDirectory, 0) != ERROR_SUCCESS)
			{
				return nullptr;
			}
		}

		strcat(saveGameDirectory, "\\");
		
		return saveGameDirectory;
#endif

		return "";
	}
	
	Ogre::String Core::getSaveFilePathName(const Ogre::String& saveName, const Ogre::String& fileEnding)
	{
		Ogre::String saveFilePathName;
		
		// Ogre::String applicationFilePathName = this->getApplicationFilePathName();
		// Ogre::String applicationName = this->getFileNameFromPath(applicationFilePathName);

		Ogre::String applicationName = this->projectName + "\\";

		size_t dotPos = applicationName.find(".");
		if (Ogre::String::npos != dotPos)
		{
			applicationName = applicationName.substr(0, dotPos);
		}
		
		Ogre::String saveUserFolder = this->getSaveGameDirectory(applicationName);
		if (false == saveUserFolder.empty())
		{
				this->createFolders(saveUserFolder);

				bool hasEnding = saveName.size() >= fileEnding.size() && 0 == saveName.compare(saveName.size() - fileEnding.size(), fileEnding.size(), fileEnding);
				saveFilePathName = saveUserFolder + saveName;
				if (false == hasEnding)
				{
					saveFilePathName += fileEnding;
				}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] Could not get user data folder: " + Ogre::String(saveUserFolder));
			throw Ogre::Exception(Ogre::Exception::ERR_FILE_NOT_FOUND, "[Core] Could not get user data folder: " + Ogre::String(saveUserFolder), "NOWA");
		}
		return saveFilePathName;
	}

	std::pair<bool, Ogre::String> Core::removePartsFromString(const Ogre::String& source, const Ogre::String& parts)
	{
		bool success = false;
		size_t n = parts.length();
		Ogre::String target = source;
		for (size_t i = target.find(parts); i != Ogre::String::npos; i = target.find(parts))
		{
			target.erase(i, n);
		}
		success = target.length() < source.length();

		return std::make_pair(success, target);
	}

	void Core::copyTextToClipboard(const Ogre::String& text)
	{
		HWND hWnd;
		this->renderWindow->getCustomAttribute("WINDOW", &hWnd);
		OpenClipboard(hWnd);
		EmptyClipboard();
		HGLOBAL globalAlloc = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
		if (!globalAlloc)
		{
			CloseClipboard();
			return;
		}
		memcpy(GlobalLock(globalAlloc), text.c_str(), text.size() + 1);
		GlobalUnlock(globalAlloc);
		SetClipboardData(CF_TEXT, globalAlloc);
		CloseClipboard();
		GlobalFree(globalAlloc);
	}

	Ogre::String Core::getApplicationFilePathName(void)
	{
		HINSTANCE handleInstance = GetModuleHandle(0);
		
		Ogre::String result;
		char temp[255 + 1] = "";
		memset(temp, 0, sizeof(temp));
		::GetModuleFileName(handleInstance, &temp[0], sizeof(temp) / sizeof(unsigned short) - 1);
		result = &temp[0];
		return result;
	}

	Ogre::String Core::getDirectoryNameFromFilePathName(const Ogre::String& filePathName)
	{
		size_t pos = filePathName.find_last_of("\\/");
		return (std::string::npos == pos) ? "" : filePathName.substr(0, pos);
	}

	Ogre::String Core::getRootFolderName(void)
	{
		Ogre::String applicationFilePathName = this->replaceSlashes(this->getApplicationFilePathName(), false);
		Ogre::String applicationFolder = this->getDirectoryNameFromFilePathName(applicationFilePathName);
		Ogre::String rootFolder;

		size_t found = applicationFolder.find("/bin/");
		if (found != Ogre::String::npos)
		{
			rootFolder = applicationFolder.substr(0, found);
		}
		else
		{
			return Ogre::String();
		}

		return rootFolder;
	}

	Ogre::String Core::replaceSlashes(const Ogre::String& filePath, bool useBackwardSlashes)
	{
		char separator = useBackwardSlashes ? L'\\' : L'/';
		Ogre::String resultPath = filePath;
		unsigned int i = 0;
		while (i < resultPath.size())
		{
			if (resultPath[i] == L'\\' || resultPath[i] == L'/')
			{
				resultPath[i] = separator;
			}
			i++;
		}
		return resultPath;
	}

	std::vector<Ogre::String> Core::getSceneFileNames(const Ogre::String& resourceGroupName, const Ogre::String& projectName)
	{
		std::vector<Ogre::String> sceneNames;

		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(resourceGroupName)[0];

		// Project is always: "Projects/ProjectName/SceneName.scene"
		// E.g.: "Projects/Plattformer/Level1.scene", "Projects/Plattformer/Level2.scene", "Projects/Plattformer/Level3.scene"
		Ogre::String searchPath = projectPath + "/" + projectName + "/*.scene";

		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// read all (real) files in current folder
				// , delete '!' read other 2 default folder . and ..
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					sceneNames.push_back(fd.cFileName);
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}

		return sceneNames;
	}

	std::vector<Ogre::String> Core::getFilePathNamesInProject(const Ogre::String& projectName, const Ogre::String& pattern)
	{
		std::vector<Ogre::String> fileNames;

		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath("Project")[0];

		// Project is always: "Projects/ProjectName/SceneName.scene"
		// E.g.: "Projects/Plattformer/Level1.scene", "Projects/Plattformer/Level2.scene", "Projects/Plattformer/Level3.scene"
		Ogre::String searchPath = projectPath + "/" + projectName + "/" + pattern;

		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// read all (real) files in current folder
				// , delete '!' read other 2 default folder . and ..
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					fileNames.push_back(projectPath + "/" + projectName + "/" + fd.cFileName);
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}

		return fileNames;
	}

	std::vector<Ogre::String> Core::getSceneSnapshotsInProject(const Ogre::String& projectName)
	{
		std::vector<Ogre::String> fileNames;

		Ogre::String saveGameProjectPath = this->getSaveGameDirectory(projectName);
		Ogre::String searchPath = saveGameProjectPath + "*.scene";

		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// read all (real) files in current folder
				// , delete '!' read other 2 default folder . and ..
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					fileNames.push_back(fd.cFileName);
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}

		return fileNames;
	}

	std::vector<Ogre::String> Core::getSaveNamesInProject(const Ogre::String& projectName)
	{
		std::vector<Ogre::String> fileNames;

		Ogre::String saveGameProjectPath = this->getSaveGameDirectory(projectName);
		Ogre::String searchPath = saveGameProjectPath + "*.sav";

		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// read all (real) files in current folder
				// , delete '!' read other 2 default folder . and ..
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					fileNames.push_back(fd.cFileName);
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}

		return fileNames;
	}

	std::vector<Ogre::String> Core::getFilePathNames(const Ogre::String& resourceGroupName, const Ogre::String& folder, const Ogre::String& pattern, bool recursive)
	{
		std::vector<Ogre::String> fileNames;

		Ogre::String rootPath;
		if (false == Core::getSingletonPtr()->getSectionPath(resourceGroupName).empty())
		{
			rootPath = Core::getSingletonPtr()->getSectionPath(resourceGroupName)[0];
		}
		Ogre::String searchPath;

		if (true == folder.empty())
		{
			searchPath = rootPath + "/" + pattern;
		}
		else if(false == rootPath.empty())
		{
			searchPath = rootPath + "/" + folder + "/" + pattern;
		}
		else
		{
			searchPath = folder + "/" + pattern;
		}

		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				// read all (real) files in current folder
				// , delete '!' read other 2 default folder . and ..
				if (false == folder.empty() && false == recursive)
				{
					if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (false == rootPath.empty())
						{
							fileNames.push_back(rootPath + "/" + folder + "/" + fd.cFileName);
						}
						else
						{
							fileNames.push_back(folder + "/" + fd.cFileName);
						}
					}
				}
				else
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						if (Ogre::String(fd.cFileName) == "." || Ogre::String(fd.cFileName) == "..")
						{
							continue;
						}
						if (false == rootPath.empty())
						{
							fileNames.push_back(rootPath + "/" + fd.cFileName);
						}
						else
						{
							fileNames.push_back(folder + "/" + fd.cFileName);
						}
					}
				}
			} while (::FindNextFile(hFind, &fd));
			::FindClose(hFind);
		}

		return fileNames;
	}

	void Core::openFolder(const Ogre::String& filePathName)
	{
		Ogre::String path = NOWA::Core::getSingletonPtr()->getAbsolutePath(filePathName);
		ShellExecute(0, "open", path.data(), 0, 0, SW_SHOWMAXIMIZED);
	}

	void Core::displayError(LPCTSTR errorDesc, DWORD errorCode)
	{
		TCHAR errorMessage[1024] = TEXT("");

		DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS
			| FORMAT_MESSAGE_MAX_WIDTH_MASK;

		FormatMessage(flags,
			NULL,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorMessage,
			sizeof(errorMessage) / sizeof(TCHAR),
			NULL);

		Ogre::String strError;
		strError += "Error : " + Ogre::String(errorDesc) + "\n";
		strError += "Code    = " + Ogre::StringConverter::toString(errorCode) + "\n";
		strError += "Message = " + Ogre::String(errorMessage) + "\n";

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, strError);
	}

	bool Core::createFolder(const Ogre::String& path)
	{
		if (CreateDirectory(path.c_str(), nullptr) || ERROR_ALREADY_EXISTS == GetLastError())
			return true;
		else
			return false;
	}

	void Core::createFolders(const Ogre::String& path)
	{
		Ogre::String internalPath = path;
		std::replace(internalPath.begin(), internalPath.end(), '\\', '/');

		// create all the directories, that are missing for the given path
		size_t pos = 0;
		while (pos < internalPath.size())
		{
			pos = internalPath.find(R"(/)", pos);
			if (Ogre::String::npos == pos)
				break;
			Ogre::String subPath = internalPath.substr(0, pos);
			this->createFolder(subPath);
			pos++;
		}
	}

	Ogre::String Core::getFileNameFromPath(const Ogre::String& filePath)
	{
		size_t i = filePath.find_last_of("/\\");
		if (i != Ogre::String::npos)
		{
			return filePath.substr(i + 1, filePath.length() - i);
		}
		else
		{
			i = filePath.rfind('/', filePath.length());
			if (i != Ogre::String::npos)
			{
				return filePath.substr(i + 1, filePath.length() - i);
			}
		}

		return filePath;
	}
	
	Ogre::String Core::getProjectNameFromPath(const Ogre::String& filePath)
	{
		// Project is always: "projects/projectName/sceneName.scene"
		Ogre::String tempProjectName = filePath;
		// Go back twice to get the project name
		size_t found = tempProjectName.find_last_of("/\\");
		
		if (Ogre::String::npos == found)
			return "";

		size_t found2 = tempProjectName.find_last_of(".scene");
		if (Ogre::String::npos != found2)
		{
			// Get rid of everything after the project name
			tempProjectName.erase(found, Ogre::String::npos);
			// Get rid of everything before the project name
			found = tempProjectName.find_last_of("/\\");
			tempProjectName = tempProjectName.substr(found + 1, tempProjectName.size());
			return tempProjectName;
		}
		else
		{
			tempProjectName = tempProjectName.substr(found + 1, tempProjectName.size());
		}
		
		return tempProjectName;
	}

#if 0
	Ogre::String Core::getProjectNameFromPath(const Ogre::String& filePath)
	{
		// Project is always: "projects/projectName/sceneName/sceneName.scene"
		Ogre::String tempProjectName = filePath;
		// Go back twice to get the project name
		size_t found = tempProjectName.find_last_of("/\\");

		if (Ogre::String::npos == found)
			return "";

		// Get rid of everything after the project name
		tempProjectName.erase(found, Ogre::String::npos);
		found = tempProjectName.find_last_of("/\\");

		// Just projectName/sceneName -> found it
		if (Ogre::String::npos == found)
			return tempProjectName;

		tempProjectName = tempProjectName.substr(0, found);

		// Get rid of everything before the project name
		found = tempProjectName.find_last_of("/\\");
		tempProjectName = tempProjectName.substr(found + 1, tempProjectName.size() - 1);

		return tempProjectName;
	}

#endif

	Ogre::String Core::getAbsolutePath(const Ogre::String& relativePath)
	{
		Ogre::String result;
		long retval = 0;
		char buffer[1024] = {};
		char** lppPart = { nullptr };
		// Retrieve the full path name for a file. The file does not need to exist.
		retval = GetFullPathNameA(relativePath.c_str(), 1024, buffer, lppPart);
		if (0 == retval)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Could not get absolute path from relative path: " + relativePath);
			return result;
		}
		result = buffer;
		std::replace(result.begin(), result.end(), '\\', '/');
		return result;
	}

	Ogre::String Core::encode64(const Ogre::String& text, bool cipher)
	{
		Ogre::String result;
		if (true == cipher)
			result = this->encrypt(text, this->cryptKey);
		else
			result = text;

		using namespace boost::archive::iterators;
		using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
		auto tmp = std::string(It(std::begin(result)), It(std::end(result)));
		return tmp.append((3 - result.size() % 3) % 3, '=');
	}
	
	Ogre::String Core::decode64(const Ogre::String& text, bool cipher)
	{
		using namespace boost::archive::iterators;
		using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
		Ogre::String result;

		if (true == cipher)
		{
			result = this->decrypt(boost::algorithm::trim_right_copy_if(std::string(It(std::begin(text)), It(std::end(text))), [](char c)
			{
				return c == '\0';
			}), this->cryptKey);
		}
		else
		{
			result = boost::algorithm::trim_right_copy_if(std::string(It(std::begin(text)), It(std::end(text))), [](char c)
			{
				return c == '\0';
			});
		}

		return result;
	}

	Ogre::String Core::encrypt(const Ogre::String& text, int key)
	{
		char strKey = (char)key; //Any char will work
		Ogre::String result = text;
    
		for (int i = 0; i < text.size(); i++)
		{
			result[i] = text[i] ^ strKey;
		}
    
		return result;
	}

	Ogre::String Core::decrypt(const Ogre::String& text, int key)
	{
		char strKey = (char)key; //Any char will work
		Ogre::String result = text;
    
		for (int i = 0; i < text.size(); i++)
		{
			result[i] = text[i] ^ strKey;
		}
    
		return result;
	}

	void Core::setCryptKey(int key)
	{
		this->cryptKey = key;
	}

	int Core::getCryptKey(void) const
	{
		return this->cryptKey;
	}

	bool Core::getUseEntityType(void) const
	{
		return this->useEntityType;
	}

	void Core::setUseEntityType(bool useEntityType)
	{
		this->useEntityType = useEntityType;
	}

	void Core::encodeAllFiles(void)
	{
		std::vector<Ogre::String> fileNames = this->getFilePathNamesInProject(this->projectName, "*.*");
		for (auto filePathName : fileNames)
		{
			std::fstream inFile(filePathName);
			Ogre::String luaInFileContent;
			if (true == inFile.good())
			{
				DWORD dwFileAttributes	= GetFileAttributes(filePathName.data());
				if(0xFFFFFFFF != dwFileAttributes)
				{
					if (dwFileAttributes & FileFlag)
					{
						inFile.close();
						continue;
					}
				}

				Ogre::String inFileContent = std::string{ std::istreambuf_iterator<char>{inFile}, std::istreambuf_iterator<char>{} };
				inFile.close();

				Ogre::String encodedContent = this->encode64(inFileContent, true);

				std::ofstream outFile(filePathName);
				if (true == outFile.good())
				{
					outFile << encodedContent;
					outFile.close();

					dwFileAttributes |= FileFlag;
					SetFileAttributes(filePathName.data(), dwFileAttributes);
				}
			}
		}

		this->projectEncoded = true;

		boost::shared_ptr<EventDataProjectEncoded> eventDataProjectEncoded(new EventDataProjectEncoded(this->projectEncoded));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectEncoded);
	}

	bool Core::decodeAllFiles(int key)
	{
		std::vector<Ogre::String> fileNames = this->getFilePathNamesInProject(this->projectName, "*.*");

		// Check on one scene file, if after decoding a xml tag can be found (with correct key decoded)
		for (auto filePathName : fileNames)
		{
			// If its a scene and the decoded content has no xml tag, then a wrong key must have been used
			size_t foundScene = filePathName.find(".scene");
			if (foundScene != std::wstring::npos)
			{
				std::fstream inFile(filePathName);
				Ogre::String inFileContent;
				if (true == inFile.good())
				{
					DWORD dwFileAttributes = GetFileAttributes(filePathName.data());
					if (0xFFFFFFFF != dwFileAttributes)
					{
						if (dwFileAttributes & FileFlag)
						{
							dwFileAttributes &= ~FileFlag;
							SetFileAttributes(filePathName.data(), dwFileAttributes);
						}
						else
						{
							inFile.close();
							continue;
						}
					}

					Ogre::String inFileContent = std::string{ std::istreambuf_iterator<char>{inFile}, std::istreambuf_iterator<char>{} };
					inFile.close();

					Ogre::String decodedContent = this->decode64(inFileContent, true);

					size_t foundTag = decodedContent.find("<?xml");
					if (foundTag != std::wstring::npos)
					{
						break;
					}
					else
					{
						return false;
					}
				}
			}
		}

		for (auto filePathName : fileNames)
		{
			std::fstream inFile(filePathName);
			Ogre::String inFileContent;
			if (true == inFile.good())
			{
				DWORD dwFileAttributes	= GetFileAttributes(filePathName.data());
				if(0xFFFFFFFF != dwFileAttributes)
				{
					if (dwFileAttributes & FileFlag)
					{
						dwFileAttributes &= ~FileFlag;
						SetFileAttributes(filePathName.data(), dwFileAttributes);
					}
					else
					{
						inFile.close();
						continue;
					}
				}

				Ogre::String inFileContent = std::string{ std::istreambuf_iterator<char>{inFile}, std::istreambuf_iterator<char>{} };
				inFile.close();

				Ogre::String decodedContent = this->decode64(inFileContent, true);

				std::ofstream outFile(filePathName);
				if (true == outFile.good())
				{
					outFile << decodedContent;
					outFile.close();
				}
			}
		}

		this->projectEncoded = false;
		boost::shared_ptr<EventDataProjectEncoded> eventDataProjectEncoded(new EventDataProjectEncoded(this->projectEncoded));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectEncoded);
		return true;
	}
	
	void Core::createApplicationIcon(unsigned short iconResourceId)
	{
		// Add a window icon
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		HWND hwnd;
		this->renderWindow->getCustomAttribute("WINDOW", (void*)&hwnd);
		LONG iconID = (LONG)LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(iconResourceId));
#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64
		SetClassLong(hwnd, GCLP_HICON, iconID);
#else
		SetClassLong(hwnd, GCL_HICON, iconID);
#endif
#endif
	}

	void Core::dumpNodes(Ogre::Node* pNode, Ogre::String padding)
	{
		//create the scene tree
		Ogre::LogManager::getSingletonPtr()->logMessage(padding + "SceneNode: " + pNode->getName()
			+ " derived position: " + Ogre::StringConverter::toString(pNode->_getDerivedPosition())
			+ " local position: " + Ogre::StringConverter::toString(pNode->getPosition())
			+ " derived orientation: " + Ogre::StringConverter::toString(pNode->_getDerivedOrientation())
			+ " local orientation: " + Ogre::StringConverter::toString(pNode->getOrientation())
			+ " derived scale: " + Ogre::StringConverter::toString(pNode->_getDerivedScale())
			+ " local scale: " + Ogre::StringConverter::toString(pNode->getScale())
			);

		Ogre::SceneNode::ObjectIterator objectIt = ((Ogre::SceneNode*) pNode)->getAttachedObjectIterator();
		auto nodeIt = pNode->getChildIterator();
		while (objectIt.hasMoreElements())
		{
			//go through all scenenodes in the scene
			Ogre::MovableObject* movableObject = objectIt.getNext();
			//add them to the tree as parents
			Ogre::LogManager::getSingletonPtr()->logMessage(padding + "Type: " + movableObject->getMovableType() + ": "
				+ movableObject->getName() + ": query flags: " + Ogre::StringConverter::toString(movableObject->getQueryFlags())
				+ " size: " + Ogre::StringConverter::toString(movableObject->getWorldAabb().getSize() * movableObject->getParentNode()->getScale()));
		}
		while (nodeIt.hasMoreElements())
		{
			//go through all objects recursive that are attached to the scenenodes
			this->dumpNodes(nodeIt.getNext(), "  " + padding);
		}
	}

	void Core::destroyScene(Ogre::SceneManager*& sceneManager)
	{
		if (sceneManager)
		{
			this->myGuiOgrePlatform->getRenderManagerPtr()->setSceneManager(nullptr);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Destroying all cameras");
			// delete the cameras
			sceneManager->destroyAllCameras();
			/*sceneManager->destroyAllLights();
			sceneManager->destroyAllAnimations();
			sceneManager->destroyAllAnimationStates();
			sceneManager->destroyAllBillboardChains();
			sceneManager->destroyAllBillboardSets();
			sceneManager->destroyAllInstancedGeometry();*/
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Clearing scene");
			sceneManager->clearScene(true);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] Destroying SceneManager");
			this->root->destroySceneManager(sceneManager);
			sceneManager = nullptr;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[Core] SceneManager destruction finised");
		}
	}

	// void Core::eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList* parameters)
	// {
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-----> [Core] event: " + eventName + " param value: " + parameters->find(eventName) );
	// }

	void Core::loadCustomConfiguration(const char *strFilename)
	{
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* XMLRoot;
		std::ifstream fp;
		fp.open(strFilename, std::ios::in | std::ios::binary);
		// if the config file does not exist, a new one will be created with default values
		if (fp.fail())
		{
			this->saveCustomConfiguration();
			return;
		}
		Ogre::DataStreamPtr stream(OGRE_NEW Ogre::FileStreamDataStream(strFilename, &fp, false));
		char* customConfig = XMLDoc.allocate_string(stream->getAsString().data());
		// parse the document
		try
		{
			XMLDoc.parse<0>(customConfig);
		} catch (rapidxml::parse_error& error)
		{
			Ogre::String errorMessage = "[Core] Could not load config file: '" + Ogre::String(strFilename) + "' because of parse errors: " 
				+ Ogre::String(error.what()) + " location: " + Ogre::String(error.where<char>());
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, errorMessage);
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, errorMessage + "\n", "NOWA");
		}
		XMLRoot = XMLDoc.first_node("Main-Configuration");
		// config file corrupt?
		if (nullptr == XMLRoot)
		{
			this->saveCustomConfiguration();
			return;
		}

		unsigned int displayRate = getScreenRefreshRate();

		// retrieve core configuration
		rapidxml::xml_node<>* pSubElement = XMLRoot->first_node("Core");
		if (nullptr != pSubElement)
		{
			if (pSubElement->first_attribute("RAM"))
			{
				this->requiredRAMPhysicalMB = Ogre::StringConverter::parseInt(pSubElement->first_attribute("RAM")->value());
			}
			if (pSubElement->first_attribute("VRAM"))
			{
				this->requiredRAMVirtualMB = Ogre::StringConverter::parseInt(pSubElement->first_attribute("VRAM")->value());
			}
			if (pSubElement->first_attribute("DiscSpace"))
			{
				this->requiredDiscSpaceMB = Ogre::StringConverter::parseInt(pSubElement->first_attribute("DiscSpace")->value());
			}
			if (pSubElement->first_attribute("CPUSpeed"))
			{
				this->requiredCPUSpeedMhz = Ogre::StringConverter::parseInt(pSubElement->first_attribute("CPUSpeed")->value());
			}
			if (pSubElement->first_attribute("Language"))
			{
				this->optionLanguage = Ogre::StringConverter::parseInt(pSubElement->first_attribute("Language")->value());
			}
			if (pSubElement->first_attribute("Border"))
			{
				 this->borderType = pSubElement->first_attribute("Border")->value();
				 if ("none" != this->borderType && "fixed" != this->borderType)
				 {
					this->borderType = "none";
				 }
			}
			if (pSubElement->first_attribute("UserData"))
			{
				 this->cryptKey = Ogre::StringConverter::parseInt(this->decode64(pSubElement->first_attribute("UserData")->value(), false));
			}
			if (pSubElement->first_attribute("UseEntityType"))
			{
				this->useEntityType = Ogre::StringConverter::parseBool(pSubElement->first_attribute("UserData")->value());
			}
		}
		// retrieve graphics configuration
		pSubElement = XMLRoot->first_node("Graphics");
		if (nullptr != pSubElement)
		{
			// Quality level controls via MyGui all other attributes here!
			if (pSubElement->first_attribute("QualityLevel"))
			{
				this->optionQualityLevel = Ogre::StringConverter::parseInt(pSubElement->first_attribute("QualityLevel")->value());
			}
			if (pSubElement->first_attribute("LODBias"))
			{
				this->optionLODBias = Ogre::StringConverter::parseReal(pSubElement->first_attribute("LODBias")->value());
			}
			if (pSubElement->first_attribute("TextureFiltering"))
			{
				this->optionTextureFiltering = Ogre::StringConverter::parseInt(pSubElement->first_attribute("TextureFiltering")->value());
			}
			if (pSubElement->first_attribute("AnisotropyLevel"))
			{
				this->optionAnisotropyLevel = Ogre::StringConverter::parseInt(pSubElement->first_attribute("AnisotropyLevel")->value());
			}
			if (pSubElement->first_attribute("DesiredFramesUpdates"))
			{
				this->optionDesiredFramesUpdates = Ogre::StringConverter::parseInt(pSubElement->first_attribute("DesiredFramesUpdates")->value());
			}
			if (pSubElement->first_attribute("DesiredSimulationUpdates"))
			{
				this->optionDesiredSimulationUpdates = Ogre::StringConverter::parseInt(pSubElement->first_attribute("DesiredSimulationUpdates")->value());
			}
			if (pSubElement->first_attribute("RenderDistance"))
			{
				this->globalRenderDistance = Ogre::StringConverter::parseInt(pSubElement->first_attribute("RenderDistance")->value());
			}
		}
		// retrieve audio configuration
		pSubElement = XMLRoot->first_node("Audio");
		if (nullptr != pSubElement)
		{
			if (pSubElement->first_attribute("SoundVolume"))
			{
				this->optionSoundVolume = Ogre::StringConverter::parseInt(pSubElement->first_attribute("SoundVolume")->value());
			}
			if (pSubElement->first_attribute("MusicVolume"))
			{
				this->optionMusicVolume = Ogre::StringConverter::parseInt(pSubElement->first_attribute("MusicVolume")->value());
			}
		}
		// retrieve log configuration
		pSubElement = XMLRoot->first_node("Log");
		if (pSubElement)
		{
			if (pSubElement->first_attribute("Level"))
			{
				this->optionLogLevel = Ogre::StringConverter::parseInt(pSubElement->first_attribute("Level")->value());
				if (this->optionLogLevel < 1) this->optionLogLevel = 1;
				if (this->optionLogLevel > 3) this->optionLogLevel = 3;
				Ogre::LogManager::getSingletonPtr()->setLogDetail((Ogre::LoggingLevel)this->optionLogLevel);
			}
		}
		// retrieve server configuration
		pSubElement = XMLRoot->first_node("Server");
		if (pSubElement)
		{
			if (pSubElement->first_attribute("StartAsServer"))
			{
				this->startAsServer = Ogre::StringConverter::parseBool(pSubElement->first_attribute("StartAsServer")->value());
			}
			if (pSubElement->first_attribute("AreaOfInterest"))
			{
				this->optionAreaOfInterest = Ogre::StringConverter::parseInt(pSubElement->first_attribute("AreaOfInterest")->value());
			}
			if (pSubElement->first_attribute("PacketSendRate"))
			{
				this->optionPacketSendRate = Ogre::StringConverter::parseInt(pSubElement->first_attribute("PacketSendRate")->value());
			}
		}

		// retrieve client configuration
		pSubElement = XMLRoot->first_node("Client");
		{
			if (pSubElement)
			{
				if (pSubElement->first_attribute("PlayerColor"))
				{
					this->optionPlayerColor = Ogre::StringConverter::parseInt(pSubElement->first_attribute("PlayerColor")->value());
				}
				if (pSubElement->first_attribute("InterpolationRate"))
				{
					this->optionInterpolationRate = Ogre::StringConverter::parseInt(pSubElement->first_attribute("InterpolationRate")->value());
				}
				if (pSubElement->first_attribute("PacketsPerSecond"))
				{
					this->optionPacketsPerSecond = Ogre::StringConverter::parseInt(pSubElement->first_attribute("PacketsPerSecond")->value());
				}
			}
		}
		// retrieve lua script configuration
		pSubElement = XMLRoot->first_node("LuaScript");
		if (pSubElement)
		{
			if (pSubElement->first_attribute("Usage"))
			{
				this->optionUseLuaScript = Ogre::StringConverter::parseBool(pSubElement->first_attribute("Usage")->value());
			}
			if (pSubElement->first_attribute("Console"))
			{
				this->optionUseLuaConsole = Ogre::StringConverter::parseBool(pSubElement->first_attribute("Console")->value());
			}
			if (pSubElement->first_attribute("GroupName"))
			{
				this->optionLuaGroupName = pSubElement->first_attribute("GroupName")->value();
				if (this->optionLuaGroupName.empty())
				{
					this->optionLuaGroupName = "Lua";
				}
			}
			if (pSubElement->first_attribute("LuaApiPath"))
			{
				this->optionLuaApiFilePath = pSubElement->first_attribute("LuaApiPath")->value();
			}
		}

		// Retrieve key mapping configuration 0
		pSubElement = XMLRoot->first_node("KeyMapping0");
		if (pSubElement)
		{
			// rapidxml::xml_node<>* propertyElement = propertyElement->next_sibling("property");
			unsigned short i = 0;
			rapidxml::xml_node<>* propertyElement = pSubElement->first_node("Key");
			while (nullptr != propertyElement)
			{
				if (propertyElement->first_attribute("value"))
				{
					OIS::KeyCode keyCode = static_cast<OIS::KeyCode>(Ogre::StringConverter::parseInt(propertyElement->first_attribute("value")->value()));
					if (OIS::KC_UNASSIGNED != keyCode)
					{
						InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->remapKey(static_cast<InputDeviceModule::Action>(i), keyCode);
					}
				}
				i++;
				propertyElement = propertyElement->next_sibling("Key");
			}
		}

		// Retrieve button mapping configuration 0
		pSubElement = XMLRoot->first_node("ButtonMapping0");
		if (pSubElement)
		{
			// rapidxml::xml_node<>* propertyElement = propertyElement->next_sibling("property");
			unsigned short i = 0;
			rapidxml::xml_node<>* propertyElement = pSubElement->first_node("Button");
			while (nullptr != propertyElement)
			{
				if (propertyElement->first_attribute("value"))
				{
					InputDeviceModule::JoyStickButton button = static_cast<InputDeviceModule::JoyStickButton>(Ogre::StringConverter::parseInt(propertyElement->first_attribute("value")->value()));
					if (InputDeviceModule::JoyStickButton::BUTTON_NONE != button)
					{
						InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->remapButton(static_cast<InputDeviceModule::Action>(i), button);
					}
				}
				i++;
				propertyElement = propertyElement->next_sibling("Button");
			}
		}

		// Retrieve key mapping configuration 1
		pSubElement = XMLRoot->first_node("KeyMapping1");
		if (pSubElement)
		{
			// rapidxml::xml_node<>* propertyElement = propertyElement->next_sibling("property");
			unsigned short i = 0;
			rapidxml::xml_node<>* propertyElement = pSubElement->first_node("Key");
			while (nullptr != propertyElement)
			{
				if (propertyElement->first_attribute("value"))
				{
					OIS::KeyCode keyCode = static_cast<OIS::KeyCode>(Ogre::StringConverter::parseInt(propertyElement->first_attribute("value")->value()));
					if (OIS::KC_UNASSIGNED != keyCode)
					{
						InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->remapKey(static_cast<InputDeviceModule::Action>(i), keyCode);
					}
				}
				i++;
				propertyElement = propertyElement->next_sibling("Key");
			}
		}

		// Retrieve button mapping configuration 1
		pSubElement = XMLRoot->first_node("ButtonMapping1");
		if (pSubElement)
		{
			// rapidxml::xml_node<>* propertyElement = propertyElement->next_sibling("property");
			unsigned short i = 0;
			rapidxml::xml_node<>* propertyElement = pSubElement->first_node("Button");
			while (nullptr != propertyElement)
			{
				if (propertyElement->first_attribute("value"))
				{
					InputDeviceModule::JoyStickButton button = static_cast<InputDeviceModule::JoyStickButton>(Ogre::StringConverter::parseInt(propertyElement->first_attribute("value")->value()));
					if (InputDeviceModule::JoyStickButton::BUTTON_NONE != button)
					{
						InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->remapButton(static_cast<InputDeviceModule::Action>(i), button);
					}
				}
				i++;
				propertyElement = propertyElement->next_sibling("Button");
			}
		}

		fp.close();
	}

	void Core::saveCustomConfiguration(void)
	{
		Ogre::String strXMLConfigName;

		if (true == this->customConfigName.empty())
		{
			strXMLConfigName = "defaultConfig.xml";
		}
		else
		{
			strXMLConfigName = this->customConfigName;
		}

		rapidxml::xml_document<> XMLDoc;

		//XML Deklaration
		rapidxml::xml_node<> *pDeclaration = XMLDoc.allocate_node(rapidxml::node_declaration);
		pDeclaration->append_attribute(XMLDoc.allocate_attribute("version", "1.0"));
		pDeclaration->append_attribute(XMLDoc.allocate_attribute("encoding", "utf-8"));
		XMLDoc.append_node(pDeclaration);

		/*rapidxml::xml_node<> *pNodeRoot = XMLDoc.allocate_node(rapidxml::node_element, "CustomConfiguration");
		pNodeRoot->append_attribute(XMLDoc.allocate_attribute("ViewRange", Ogre::StringConverter::toString(this->optionViewRange).c_str()));
		XMLDoc.append_node(pNodeRoot);*/

		std::ofstream outfile(strXMLConfigName.c_str());
		// Document-Object
		outfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		// Main-Configuration
		outfile << "<Main-Configuration>\n";
		// Core-Configuration
		outfile << "<Core RAM=\"" << Ogre::StringConverter::toString(this->requiredRAMPhysicalMB).c_str() << "\""
			<< " VRAM=\"" << Ogre::StringConverter::toString(this->requiredRAMVirtualMB).c_str() << "\""
			<< " DiscSpace=\"" << Ogre::StringConverter::toString(this->requiredDiscSpaceMB).c_str() << "\""
			<< " CPUSpeed=\"" << Ogre::StringConverter::toString(this->requiredCPUSpeedMhz).c_str() << "\""
			<< " Language=\"" << Ogre::StringConverter::toString(this->optionLanguage) << "\""
			<< " Border=\"" << this->borderType << "\""
			<< " UserData=\"" << this->encode64(Ogre::StringConverter::toString(this->cryptKey), false) << "\""
			<< " UseEntityType=\"" << Ogre::StringConverter::toString(this->useEntityType) << "\""
			<< "/>\n";
		// Graphics-Configuration
		outfile << "<Graphics QualityLevel=\"" << Ogre::StringConverter::toString(this->optionQualityLevel) << "\""
			<< " LODBias=\"" << Ogre::StringConverter::toString(this->optionLODBias).c_str() << "\""
			<< " TextureFiltering=\"" << Ogre::StringConverter::toString(this->optionTextureFiltering).c_str() << "\""
			<< " AnisotropyLevel=\"" << Ogre::StringConverter::toString(this->optionAnisotropyLevel).c_str() << "\""
			<< " DesiredFramesUpdates=\"" << Ogre::StringConverter::toString(this->optionDesiredFramesUpdates).c_str() << "\""
			<< " DesiredSimulationUpdates=\"" << Ogre::StringConverter::toString(this->optionDesiredSimulationUpdates).c_str() << "\""
			<< " RenderDistance=\"" << Ogre::StringConverter::toString(this->globalRenderDistance).c_str() << "\""
			<< "/>\n";
		// Audio-Configuration
		outfile << "<Audio SoundVolume=\"" << Ogre::StringConverter::toString(this->optionSoundVolume).c_str() << "\""
			<< " MusicVolume=\"" << Ogre::StringConverter::toString(this->optionMusicVolume).c_str() << "\""
			<< "/>\n";
		// Log-Configuration
		outfile << "<Log Level=\"" << Ogre::StringConverter::toString(this->optionLogLevel).c_str() << "\""
			<< "/>\n";
		// Server-Configuration
		outfile << "<Server StartAsServer=\"" << Ogre::StringConverter::toString(this->startAsServer).c_str() << "\""
			<< " AreaOfInterest=\"" << Ogre::StringConverter::toString(this->optionAreaOfInterest).c_str() << "\""
			<< " PacketSendRate=\"" << Ogre::StringConverter::toString(this->optionPacketSendRate).c_str() << "\""
			<< "/>\n";
		// Client-Configuration
		outfile << "<Client PlayerColor=\"" << Ogre::StringConverter::toString(this->optionPlayerColor).c_str() << "\""
			<< " InterpolationRate=\"" << Ogre::StringConverter::toString(this->optionInterpolationRate).c_str() << "\""
			<< " PacketsPerSecond=\"" << Ogre::StringConverter::toString(this->optionPacketsPerSecond).c_str() << "\""
			<< "/>\n";
		// LuaScript-Configuration
		outfile << "<LuaScript Usage=\"" << Ogre::StringConverter::toString(this->optionUseLuaScript).c_str() << "\""
			<< " Console=\"" << Ogre::StringConverter::toString(this->optionUseLuaConsole).c_str() << "\""
			<< " GroupName=\"" << this->optionLuaGroupName.c_str() << "\""
			<< " LuaApiPath=\"" << this->optionLuaApiFilePath << "\""
			<< "/>\n";

		{
			// KeyMapping-Configuration 0
			outfile << "<KeyMapping0>\n";

			for (unsigned short i = 0; i < InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getKeyMappingCount(); i++)
			{
				outfile << "<Key value=\"" << Ogre::StringConverter::toString(InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(static_cast<InputDeviceModule::Action>(i))) << "\" />\n";
			}
			outfile << "</KeyMapping0>\n";

			// ButtonMapping-Configuration 0
			outfile << "<ButtonMapping0>\n";

			for (unsigned short i = 0; i < InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getButtonMappingCount(); i++)
			{
				outfile << "<Button value=\"" << Ogre::StringConverter::toString(InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(static_cast<InputDeviceModule::Action>(i))) << "\" />\n";
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "--->button: " + NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedButtonAction(static_cast<InputDeviceModule::Action>(i)));
			}
			outfile << "</ButtonMapping0>\n";

			// KeyMapping-Configuration 1
			outfile << "<KeyMapping1>\n";

			for (unsigned short i = 0; i < InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getKeyMappingCount(); i++)
			{
				outfile << "<Key value=\"" << Ogre::StringConverter::toString(InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedKey(static_cast<InputDeviceModule::Action>(i))) << "\" />\n";
			}
			outfile << "</KeyMapping1>\n";

			// ButtonMapping-Configuration 1
			outfile << "<ButtonMapping1>\n";

			for (unsigned short i = 0; i < InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getButtonMappingCount(); i++)
			{
				outfile << "<Button value=\"" << Ogre::StringConverter::toString(InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(static_cast<InputDeviceModule::Action>(i))) << "\" />\n";
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "--->button: " + NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedButtonAction(static_cast<InputDeviceModule::Action>(i)));
			}
			outfile << "</ButtonMapping1>\n";
		}

		outfile << "</Main-Configuration>\n";

		outfile.close();
	}

	Ogre::StringVector Core::getSectionPath(const Ogre::String& strSection)
	{
		Ogre::String secName;
		Ogre::StringVector archName;
		Ogre::ConfigFile cf;
		// load the resources
		cf.load(this->resourcesName);
		// go through all sections and subsections
		Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
		while (seci.hasMoreElements())
		{
			secName = seci.peekNextKey();
			Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
			if (secName == strSection)
			{
				Ogre::ConfigFile::SettingsMultiMap::const_iterator i;
				for (i = settings->cbegin(); i != settings->cend(); ++i)
				{
					archName.emplace_back(i->second);
				}
			}
		}
		return archName;
	}

	bool Core::keyPressed(const OIS::KeyEvent& keyEventRef)
	{
		if (keyEventRef.key == OIS::KC_SYSRQ)
		{
			// Screenshot

			Ogre::Image2 img;
			Ogre::TextureGpu* texture = renderWindow->getTexture();
			img.convertFromTexture(texture, 0u, texture->getNumMipmaps() - 1u);

			std::time_t t = std::time(0);   // get time now
			std::tm* now = std::localtime(&t);
			Ogre::String date = Ogre::StringConverter::toString(now->tm_year + 1900) + "-" + Ogre::StringConverter::toString(now->tm_mon + 1) + "-" +
				Ogre::StringConverter::toString(now->tm_mday) + "_" + Ogre::StringConverter::toString(now->tm_min) + "_" + Ogre::StringConverter::toString(now->tm_sec);

			img.save(date + ".png", 0u, texture->getNumMipmaps());

			renderWindow->setWantsToDownload(true);
			renderWindow->setManualSwapRelease(true);
			renderWindow->performManualRelease();

			return true;
		}

		if (this->optionUseLuaConsole && !NOWA::LuaConsole::getSingletonPtr()->isVisible())
		{
			// Show fps statistics
			if (keyEventRef.key == OIS::KC_F12)
			{
				this->info->setVisible(!this->info->getVisible());
				this->root->getRenderSystem()->setMetricsRecordingEnabled(this->info->getVisible());
			}
		}
		if (this->optionUseLuaConsole && keyEventRef.key == OIS::KC_GRAVE)
		{
			if (NOWA::LuaConsole::getSingletonPtr())
				NOWA::LuaConsole::getSingletonPtr()->setVisible(!NOWA::LuaConsole::getSingletonPtr()->isVisible());
			return true;
		}
		if (this->optionUseLuaConsole && NOWA::LuaConsole::getSingletonPtr()->isVisible())
		{
			if (NOWA::LuaConsole::getSingletonPtr())
				NOWA::LuaConsole::getSingletonPtr()->injectKeyPress(keyEventRef);
		}
		else
		{
			// Normal non-console key handling.
			switch (keyEventRef.key)
			{
			case OIS::KC_ESCAPE:
			case OIS::KC_Q:
				return true;
			default:
				break;
			}
		}
		return true;
	}

	bool Core::keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		return true;
	}

	bool Core::mouseMoved(const OIS::MouseEvent& evt)
	{
		return true;
	}

	bool Core::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		return true;
	}

	bool Core::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		return true;
	}
	
	bool Core::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		return true;
	}
		
	bool Core::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}
	
	bool Core::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}
	
	/*bool Core::povMoved(const OIS::JoyStickEvent& evt, int pov)
	{
		
	}*/
	
	void Core::update(Ogre::Real dt)
	{
		MyGUI::Gui::getInstancePtr()->_injectFrameEntered(dt);

		if (this->optionUseLuaConsole)
		{
			LuaConsole::getSingletonPtr()->update(dt);
		}

		ProcessManager::getInstance()->updateProcesses(dt);
		// allow event queue to process for up to 20 ms
		AppStateManager* appStateManager = AppStateManager::getSingletonPtr();
		if (appStateManager->getAppStatesCount() > 0)
		{
			appStateManager->getEventManager()->update(20);
		}
	}

	void Core::updateFrameStats(Ogre::Real dt)
	{
		// Get frames statistics
		if (this->info && true == this->info->getVisible())
		{
			// Update each passed second
			this->timeSinceLastStatisticUpdate += dt;
			if (this->timeSinceLastStatisticUpdate > 1.0f)
			{
				this->timeSinceLastStatisticUpdate -= 1.0f;
				try
				{
					const Ogre::FrameStats* frameStats = this->root->getFrameStats();

					Ogre::Real avgTime = frameStats->getRollingAverage();
					Ogre::Real avgFps = frameStats->getRollingAverageFps();
					char m[32];
					sprintf(m, "%.2fms - %.2ffps", avgTime, avgFps);
					this->info->change("FPS", Ogre::String(m));
					this->info->change("Best FPS", Ogre::StringConverter::toString(frameStats->getBestTime()) + " ms");
					this->info->change("Worst FPS", Ogre::StringConverter::toString(frameStats->getWorstTime()) + " ms");
					this->info->change("Face Count", Ogre::StringConverter::toString(this->root->getRenderSystem()->getMetrics().mFaceCount));
					this->info->change("Batch Count", Ogre::StringConverter::toString(this->root->getRenderSystem()->getMetrics().mBatchCount));
					this->info->change("Draw Count", Ogre::StringConverter::toString(this->root->getRenderSystem()->getMetrics().mDrawCount));
					this->info->change("Instance Count", Ogre::StringConverter::toString(this->root->getRenderSystem()->getMetrics().mInstanceCount));
					this->info->change("Vertex Count", Ogre::StringConverter::toString(this->root->getRenderSystem()->getMetrics().mVertexCount));
					this->info->update();
				}
				catch (...)
				{
				}

#if 0
				// Test joystick count if maybe connection is gone etc.
				unsigned short joystickCount = this->inputManager->getNumberOfDevices(OIS::OISJoyStick);
				if (joystickCount < this->joyStick.size())
				{
					// What todo here??
					// this->joyStick[0]->queryInterface
					// this->joyStick.erase(this->joyStick)
				}
#endif
			}
		}
	}

	std::set<Ogre::String> Core::getAllAvailableTextureNames(std::vector<Ogre::String>& filters)
	{
		if (true == filters.empty())
			filters = { "png", "jpg", "bmp", "tga", "gif", "tif", "dds" };

		std::set<Ogre::String> textureNames;

		for (auto& resourceGroupName : this->resourceGroupNames)
		{
			if ("Skies" == resourceGroupName)
				continue;
			// Ogre::StringVector extensions = Ogre::Codec::getExtensions();
			// for (Ogre::StringVector::iterator itExt = extensions.begin(); itExt != extensions.end(); ++itExt)
			for (auto& filter : filters)
			{
				Ogre::StringVectorPtr names = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames(resourceGroupName, "*." + filter/**itExt*/);
				for (Ogre::StringVector::iterator itName = names->begin(); itName != names->end(); ++itName)
				{
					textureNames.emplace(*itName);
				}
			}
		}

		return textureNames;
	}

	Ogre::String Core::getResourceFilePathName(const Ogre::String& resourceName)
	{
		if (true == resourceName.empty())
		{
			return "";
		}

		Ogre::Archive* archive = nullptr;
		try
		{
			Ogre::String resourceGroupName = Ogre::ResourceGroupManager::getSingletonPtr()->findGroupContainingResource(resourceName);
			archive = Ogre::ResourceGroupManager::getSingletonPtr()->_getArchiveToResource(resourceName, resourceGroupName);
		}
		catch (...)
		{
			return "";
		}

		if (nullptr != archive)
		{
			return archive->getName();
		}
		return "";
	}

	Ogre::String Core::readContent(const Ogre::String& filePathName, unsigned int startOffset, unsigned int size)
	{
		std::ifstream fin(filePathName);
		if (true == fin.good())
		{
			fin.seekg(startOffset);
			// char buffer[41];
			std::vector<char> buffer(size + 1);
			fin.read(&buffer[0], size);
			buffer[size] = 0;
			std::istringstream iss(&buffer[0]);
			return iss.str();
		}
		return "";
	}

	bool Core::isProjectEncoded(void) const
	{
		return this->projectEncoded;
	}

	Ogre::String Core::getCurrentDateAndTime(const Ogre::String& format) const
	{
		time_t     now = time(0);
		struct tm  tStruct;
		char       buf[80];
		tStruct = *localtime(&now);
		// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
		// for more information about date/time format
		strftime(buf, sizeof(buf), format.data(), &tStruct);

		Ogre::String dateAndTime = Ogre::StringUtil::replaceAll(buf, ":", "_");

		return dateAndTime;
	}

	void Core::setCurrentSaveGameName(const Ogre::String& saveGameName)
	{
		this->saveGameName = saveGameName;
	}

	Ogre::String Core::getCurrentSaveGameName(void) const
	{
		return this->saveGameName;
	}

	void Core::setTightMemoryBudget()
	{
		if (nullptr != this->root)
		{
			Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();
			Ogre::TextureGpuManager* textureGpuManager = renderSystem->getTextureGpuManager();

			textureGpuManager->setStagingTextureMaxBudgetBytes(8u * 1024u * 1024u);
			textureGpuManager->setWorkerThreadMaxPreloadBytes(8u * 1024u * 1024u);
			textureGpuManager->setWorkerThreadMaxPerStagingTextureRequestBytes(4u * 1024u * 1024u);

			Ogre::TextureGpuManager::BudgetEntryVec budget;
			textureGpuManager->setWorkerThreadMinimumBudget(budget);
		}
	}
	//-----------------------------------------------------------------------------------
	void Core::setRelaxedMemoryBudget(void)
	{
		if (nullptr != this->root)
		{
			Ogre::RenderSystem *renderSystem = this->root->getRenderSystem();

			Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS && \
OGRE_PLATFORM != OGRE_PLATFORM_ANDROID && \
OGRE_ARCH_TYPE != OGRE_ARCHITECTURE_32
			textureGpuManager->setStagingTextureMaxBudgetBytes(256u * 2048u * 2048u);
#else
			textureGpuManager->setStagingTextureMaxBudgetBytes(128u * 1024u * 1024u);
#endif
			textureGpuManager->setWorkerThreadMaxPreloadBytes(256u * 2048u * 2048u);
			textureGpuManager->setWorkerThreadMaxPerStagingTextureRequestBytes(64u * 2048u * 2048u);

			textureGpuManager->setWorkerThreadMinimumBudget(this->defaultBudget);
		}
	}

	void Core::switchFullscreen(bool bFullscreen, unsigned int monitorId, unsigned int width, unsigned int height)
	{
		if (0 == width || 0 == height)
		{
			width = NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth();
			height = NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getHeight();
		}

		if (true == bFullscreen)
		{
			NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->requestFullscreenSwitch(bFullscreen, false, monitorId, width, height, 0, 0);
		}
		else
		{
			NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->requestFullscreenSwitch(bFullscreen, true, monitorId, width, height, 0, 0);
		}
	}

	Ogre::String Core::dumpGraphicsMemory(void)
	{
		//NOTE: Some of these routines to retrieve memory may be relatively "expensive".
        //So don't call them every frame in the final build for the user.

		Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

		Ogre::String outText;

        //Note that VaoManager::getMemoryStats gives us a lot of info that we don't show on screen!
        //Dumping to a Log is highly recommended!
        Ogre::VaoManager::MemoryStatsEntryVec memoryStats;
        size_t freeBytes;
        size_t capacityBytes;
		bool outIncludingTextures = true;
        vaoManager->getMemoryStats( memoryStats, capacityBytes, freeBytes, 0, outIncludingTextures );

        const size_t bytesToMb = 1024u * 1024u;
        char tmpBuffer[256];
        Ogre::LwString text( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        text.clear();
        text.a( "\n\nGPU buffer pools (meshes, const, texture, indirect & uav buffers): ",
                (Ogre::uint32)((capacityBytes - freeBytes) / bytesToMb), "/",
                (Ogre::uint32)(capacityBytes / bytesToMb), " MB" );
        outText += text.c_str();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();
        size_t textureBytesCpu, textureBytesGpu, usedStagingTextureBytes, availableStagingTextureBytes;
        textureGpuManager->getMemoryStats( textureBytesCpu, textureBytesGpu,
                                           usedStagingTextureBytes, availableStagingTextureBytes );

        text.clear();
        text.a( "\nGPU StagingTextures. In use: ",
                (Ogre::uint32)(usedStagingTextureBytes / bytesToMb), " MB. Available: ",
                (Ogre::uint32)(availableStagingTextureBytes / bytesToMb), " MB. Total:",
                (Ogre::uint32)((usedStagingTextureBytes + availableStagingTextureBytes) / bytesToMb) );
        outText += text.c_str();

        text.clear();
        text.a( "\nGPU Textures:\t", (Ogre::uint32)(textureBytesGpu / bytesToMb), " MB" );
        text.a( "\nCPU Textures:\t", (Ogre::uint32)(textureBytesCpu / bytesToMb), " MB" );
        text.a( "\nTotal GPU:\t",
                (Ogre::uint32)((capacityBytes + textureBytesGpu +
                                usedStagingTextureBytes + availableStagingTextureBytes) /
                               bytesToMb), " MB" );
        outText += text.c_str();

		return outText;
	}

	unsigned int Core::getScreenRefreshRate(void)
	{
		DEVMODE Screen;
		Screen.dmSize = sizeof(DEVMODE);

		// TODO: Will just work for the main monitor
		for (int i = 0; EnumDisplaySettings(NULL, i, &Screen); i++)
		{
			return Screen.dmDisplayFrequency;
		}
		return 0;
	}

	bool Core::processMeshMagick(const Ogre::String& meshName, const Ogre::String& parameters)
	{
		Ogre::String rootFolder = this->getRootFolderName();
		if (true == rootFolder.empty())
		{
			return false;
		}

		// "Material/SOLID/TEX/case1.png"
		Ogre::String meshResourceFolderName = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource(meshName);

		Ogre::FileInfoListPtr fileInfoList = Ogre::FileInfoListPtr(Ogre::ResourceGroupManager::getSingletonPtr()->findResourceFileInfo(meshResourceFolderName, meshName));
		if (fileInfoList->empty())
		{
			return false;
		}

		// "D:\Ogre\GameEngineDevelopment\media/models/Objects"
		Ogre::String sourceFolder = fileInfoList->at(0).archive->getName();
		sourceFolder = rootFolder + "/" + sourceFolder.substr(6, sourceFolder.length());

		// "D:\Ogre\GameEngineDevelopment\media\models\Objects\Case1.mesh"
		Ogre::String sourceFile = sourceFolder + "/" + meshName;

		sourceFile = this->replaceSlashes(sourceFile, false);

		Ogre::String destFile = sourceFile + "_backup";

		// Makes a backup of the file
		CopyFile(sourceFile.data(), destFile.data(), TRUE);

#if 0
		STARTUPINFO startupInfo;
		memset(&startupInfo, 0, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags = STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION processInformation;
#endif

#if 0
		Ogre::String param;
		param += "-smooth ";
		param += "-relsize ";
		param += "0.35 ";
		param += "-cut_surface ";
		param += "\"";
		param += datablockName;
		param += "\" ";
		param += "-random ";
		param += "\"";
		param += sourceFile;
		param += "\" ";
		param += "\"";
		param += destinationFolder;
		param += "\"";
#endif

		Ogre::String applicationFolder = rootFolder + "/development/MeshUtils/1.4.1";

		// Copies the file to the targed folder
		Ogre::String destinationFilePathName = applicationFolder + "/" + meshName;
		CopyFile(sourceFile.data(), destinationFilePathName.data(), TRUE);

		Ogre::String meshMagickFilePathName = applicationFolder + "\\MeshMagick.exe";
		meshMagickFilePathName = this->replaceSlashes(meshMagickFilePathName, true);

		Ogre::String tempParameters = meshMagickFilePathName + " " + parameters;
		tempParameters += " in=" + sourceFile;
		tempParameters += " out=" + sourceFile + "_rotated.mesh";

		// 90/0/1/0

		Ogre::String meshMagickCallerFilePathName = applicationFolder + "\\MeshMagickCaller.exe";
		meshMagickCallerFilePathName = this->replaceSlashes(meshMagickCallerFilePathName, true);

#if 0
		DWORD creationFlags = DETACHED_PROCESS /*NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP*/;
		BOOL result = ::CreateProcess(
			meshMagickCallerFilePathName.data(), // Path to executable
			tempParameters.data(),            // Command line
			NULL,            // Process handle not inheritable
			NULL,            // Thread handle not inheritable
			FALSE,           // Set handle inheritance to FALSE
			creationFlags,               // No creation flags
			NULL,            // Use parent's environment block
			"", // Set the working directory
			&startupInfo,             // Pointer to STARTUPINFO structure
			&processInformation);

		if (result)
		{
			// Wait for external application to finish
			WaitForSingleObject(processInformation.hProcess, INFINITE);

			DWORD exitCode = 0;
			// Get the exit code.
			result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

			// Close the handles.
			CloseHandle(processInformation.hProcess);
			CloseHandle(processInformation.hThread);

			if (FALSE == result)
			{
				// Could not get exit code.
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Coure] Could not get exit code from OgreMeshMagick.exe.");

				return false;
			}
		}
		else
		{
			Ogre::String lastError = Ogre::StringConverter::toString(GetLastError());
			Core::getSingletonPtr()->displayError("Could not create process for OgreMeshMagick.exe", GetLastError());

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] Cannot continue, because the OgreMeshMagick detected an error during mesh operation: "
															+ meshName + ".");

			// Sent event with feedback
			boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{MeshOperationFail}"));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
			return false;
		}
#endif
#if 1
		// 90/0/1/0

		SHELLEXECUTEINFO shRun = { 0 };
		shRun.cbSize = sizeof(SHELLEXECUTEINFO);
		shRun.fMask = SEE_MASK_NOCLOSEPROCESS;
		shRun.hwnd = NULL;
		shRun.lpVerb = NULL;
		shRun.lpFile = meshMagickCallerFilePathName.data();
		shRun.lpParameters = tempParameters.data();
		shRun.nShow = SW_SHOW;
		shRun.hInstApp = NULL;

		if (!ShellExecuteEx(&shRun))
		{
			Ogre::String lastError = Ogre::StringConverter::toString(GetLastError());
			Core::getSingletonPtr()->displayError("Could not create process for OgreMeshMagick.exe", GetLastError());

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] Cannot continue, because the OgreMeshMagick detected an error during mesh operation: "
															+ meshName + ".");

			// Sent event with feedback
			boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{MeshOperationFail}"));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
			return false;
		}

		WaitForSingleObject(shRun.hProcess, INFINITE);
		CloseHandle(shRun.hProcess);
#endif

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		applicationFolder = this->replaceSlashes(applicationFolder, true);

		// Set up the environment block to use the old OgreMain.dll path
		std::string env = "PATH=" + applicationFolder + ";" + getenv("PATH");
		LPSTR envBlock = const_cast<char*>(env.c_str());

		// Create the process
		if (!CreateProcess(NULL, const_cast<char*>(tempParameters.c_str()), NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, envBlock, NULL, &si, &pi))
		{
			Ogre::String lastError = Ogre::StringConverter::toString(GetLastError());
			Core::getSingletonPtr()->displayError("Could not create process for OgreMeshMagick.exe", GetLastError());

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Core] Cannot continue, because the OgreMeshMagick detected an error during mesh operation: "
															+ meshName + ".");

			// Sent event with feedback
			boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{MeshOperationFail}"));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
			return false;
		}

		// Wait for the process to complete
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return true;

		return true;
	}

	template <typename T, size_t MaxNumTextures>
	void Core::unloadTexturesFromUnusedMaterials(Ogre::HlmsDatablock *datablock, std::set<Ogre::TextureGpu*>& usedTex, std::set<Ogre::TextureGpu*>& unusedTex)
	{
		OGRE_ASSERT_HIGH(dynamic_cast<T*>(datablock));
		T *derivedDatablock = static_cast<T*>(datablock);

		for (size_t texUnit = 0; texUnit < MaxNumTextures; ++texUnit)
		{
			//Check each texture from the material
			Ogre::TextureGpu* tex = derivedDatablock->getTexture(texUnit);
			if (nullptr != tex)
			{
				//If getLinkedRenderables is empty, then the material is not in use,
				//and thus so is potentially the texture
				if (!datablock->getLinkedRenderables().empty())
				{
					usedTex.insert(tex);
				}
				else
				{
					unusedTex.insert(tex);
				}
			}
		}
	}
	
	void Core::unloadTexturesFromUnusedMaterials(void)
	{
		Ogre::HlmsManager* hlmsManager = this->root->getHlmsManager();

		std::set<Ogre::TextureGpu*> usedTex;
		std::set<Ogre::TextureGpu*> unusedTex;

		//Check each material from each Hlms (except low level) to see if their material is
		//currently in use. If it's not, then its textures may be not either
		for (size_t i = Ogre::HLMS_PBS; i < Ogre::HLMS_MAX; ++i)
		{
			Ogre::Hlms *hlms = hlmsManager->getHlms(static_cast<Ogre::HlmsTypes>(i));

			if (hlms)
			{
				const Ogre::Hlms::HlmsDatablockMap& datablocks = hlms->getDatablockMap();

				Ogre::Hlms::HlmsDatablockMap::const_iterator itor = datablocks.begin();
				Ogre::Hlms::HlmsDatablockMap::const_iterator end = datablocks.end();

				while (itor != end)
				{
					if (i == Ogre::HLMS_PBS)
					{
						unloadTexturesFromUnusedMaterials<Ogre::HlmsPbsDatablock, Ogre::NUM_PBSM_TEXTURE_TYPES>(itor->second.datablock, usedTex, unusedTex);
					}
					else if (i == Ogre::HLMS_UNLIT)
					{
						unloadTexturesFromUnusedMaterials<Ogre::HlmsUnlitDatablock, Ogre::NUM_UNLIT_TEXTURE_TYPES>(itor->second.datablock, usedTex, unusedTex);
					}

					++itor;
				}
			}
		}

		//Unload all unused textures, unless they're also in the "usedTex" (a texture may be
		//set to a material that is currently unused, and also in another material in use)
		std::set<Ogre::TextureGpu*>::const_iterator itor = unusedTex.begin();
		std::set<Ogre::TextureGpu*>::const_iterator end = unusedTex.end();

		while (itor != end)
		{
			if (usedTex.find(*itor) == usedTex.end())
				(*itor)->scheduleTransitionTo(Ogre::GpuResidency::OnStorage);

			++itor;
		}
	}

	void Core::unloadUnusedTextures(void)
	{
		Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();

		Ogre::TextureGpuManager* textureGpuManager = renderSystem->getTextureGpuManager();

		const Ogre::TextureGpuManager::ResourceEntryMap& entries = textureGpuManager->getEntries();

		Ogre::TextureGpuManager::ResourceEntryMap::const_iterator itor = entries.begin();
		Ogre::TextureGpuManager::ResourceEntryMap::const_iterator end = entries.end();

		while (itor != end)
		{
			const Ogre::TextureGpuManager::ResourceEntry& entry = itor->second;

			const Ogre::vector<Ogre::TextureGpuListener*>::type& listeners = entry.texture->getListeners();

			bool canBeUnloaded = true;

			Ogre::vector<Ogre::TextureGpuListener*>::type::const_iterator itListener = listeners.begin();
			Ogre::vector<Ogre::TextureGpuListener*>::type::const_iterator enListener = listeners.end();

			while (itListener != enListener)
			{
				//We must use dynamic_cast because we don't know if it's safe to cast
				Ogre::HlmsDatablock *datablock = dynamic_cast<Ogre::HlmsDatablock*>(*itListener);
				if (datablock)
					canBeUnloaded = false;

				canBeUnloaded &= (*itListener)->shouldStayLoaded(entry.texture);

				++itListener;
			}

			if (entry.texture->getTextureType() != Ogre::TextureTypes::Type2D ||
				!entry.texture->hasAutomaticBatching() ||
				!entry.texture->isTexture() ||
				entry.texture->isRenderToTexture() ||
				entry.texture->isUav())
			{
				//This is likely a texture internal to us (i.e. a PCC probe)
				//Note that cubemaps loaded from file will also fall here, at
				//least the way I wrote the if logic.
				//
				//You may have to customize this further
				canBeUnloaded = false;
			}

			if (canBeUnloaded)
				entry.texture->scheduleTransitionTo(Ogre::GpuResidency::OnStorage);

			++itor;
		}
	}

	void Core::initMiscParamsListener(Ogre::NameValuePairList& params)
	{
		//The default parameters may be fine, but they may be not for you.
        //Monitor what's your average and worst case consumption,
        //and then set these parameters accordingly.
        //
        //In particular monitor the output of VaoManager::getMemoryStats
        //to select the proper defaults best suited for your application.
        //A value of 0 bytes is a valid input.

        //Used by GL3+ & Metal
        params["VaoManager::CPU_INACCESSIBLE"] =					Ogre::StringConverter::toString( 8u * 1024u * 1024u );
        params["VaoManager::CPU_ACCESSIBLE_DEFAULT"] =				Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::CPU_ACCESSIBLE_PERSISTENT"] =			Ogre::StringConverter::toString( 8u * 1024u * 1024u );
        params["VaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT"] =	Ogre::StringConverter::toString( 4u * 1024u * 1024u );

        //Used by D3D11
        params["VaoManager::VERTEX_IMMUTABLE"] =					Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::VERTEX_DEFAULT"] =						Ogre::StringConverter::toString( 8u * 1024u * 1024u );
        params["VaoManager::VERTEX_DYNAMIC"] =						Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::INDEX_IMMUTABLE"] =						Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::INDEX_DEFAULT"] =						Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::INDEX_DYNAMIC"] =						Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::SHADER_IMMUTABLE"] =					Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::SHADER_DEFAULT"] =						Ogre::StringConverter::toString( 4u * 1024u * 1024u );
        params["VaoManager::SHADER_DYNAMIC"] =						Ogre::StringConverter::toString( 4u * 1024u * 1024u );
	}

	void Core::minimizeMemory(Ogre::SceneManager* sceneManager)
	{
		setTightMemoryBudget();
		unloadTexturesFromUnusedMaterials();
		unloadUnusedTextures();

		sceneManager->shrinkToFitMemoryPools();

		Ogre::RenderSystem* renderSystem = this->root->getRenderSystem();
		Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();
		vaoManager->cleanupEmptyPools();
	}

	void Core::createCustomTextures(void)
	{
		Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
		Ogre::TextureGpu* tex = textureManager->createTexture("HalftoneVolume", Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type3D);
		tex->setResolution(64u, 64u, 64u);
		tex->setPixelFormat(Ogre::PFG_R8_UNORM);

		{
			tex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
			tex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			Ogre::StagingTexture* stagingTexture = textureManager->getStagingTexture(64u, 64u, 64u, 1u, tex->getPixelFormat());
			stagingTexture->startMapRegion();
			Ogre::TextureBox texBox = stagingTexture->mapRegion(64u, 64u, 64u, 1u, tex->getPixelFormat());
			size_t height = tex->getHeight();
			size_t width = tex->getWidth();
			size_t depth = tex->getDepth();

			for (size_t z = 0; z < depth; ++z)
			{
				for (size_t y = 0; y < height; ++y)
				{
					Ogre::uint8 *data = reinterpret_cast<Ogre::uint8*>(texBox.at(0, y, z));
					for (size_t x = 0; x < width; ++x)
					{
						float fx = 32 - (float)x + 0.5f;
						float fy = 32 - (float)y + 0.5f;
						float fz = 32 - ((float)z) / 3 + 0.5f;
						float distanceSquare = fx * fx + fy * fy + fz * fz;
						data[x] = 0x00;
						if (distanceSquare < 1024.0f)
							data[x] += 0xFF;
					}
				}
			}

			stagingTexture->stopMapRegion();
			stagingTexture->upload(texBox, tex, 0, 0, 0, true);
			tex->notifyDataIsReady();

			textureManager->removeStagingTexture(stagingTexture);
			stagingTexture = 0;
		}

		Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();

		tex = textureManager->createTexture("DitherTex", Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D);
		tex->setResolution(renderWindow->getWidth(), renderWindow->getHeight());
		tex->setPixelFormat(Ogre::PFG_R8_UNORM);

		{
			tex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
			tex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			Ogre::StagingTexture* stagingTexture = textureManager->getStagingTexture(tex->getWidth(), tex->getHeight(), tex->getDepth(), tex->getNumSlices(), tex->getPixelFormat());
			stagingTexture->startMapRegion();
			Ogre::TextureBox texBox = stagingTexture->mapRegion(tex->getWidth(), tex->getHeight(), tex->getDepth(), tex->getNumSlices(), tex->getPixelFormat());
			size_t height = tex->getHeight();
			size_t width = tex->getWidth();

			for (size_t y = 0; y < height; ++y)
			{
				Ogre::uint8 *data = reinterpret_cast<Ogre::uint8*>(texBox.at(0, y, 0));
				for (size_t x = 0; x < width; ++x)
					data[x] = Ogre::Math::RangeRandom(64.0, 192);
			}

			stagingTexture->stopMapRegion();
			stagingTexture->upload(texBox, tex, 0, 0, 0, true);
			tex->notifyDataIsReady();

			textureManager->removeStagingTexture(stagingTexture);
			stagingTexture = 0;
		}
	}

	HlmsBaseListenerContainer* Core::getBaseListenerContainer(void) const
	{
		return this->baseListenerContainer;
	}

}; // namespace end
