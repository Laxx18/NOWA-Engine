#ifndef CORE_H
#define CORE_H

/*#define WIN32_LEAN_AND_MEAN
#define _USING_V110_SDK71_
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#include <windows.h>*/

// stupid microsoft error with std::min/max therefore undef it
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

// #define OGRE_DEBUG_MODE 0

#include "OIS.h"

#include "OgreOverlay.h"
#include "OgreOverlayElement.h"
#include "OgreOverlayManager.h"
#include "OgreOverlaySystem.h"
#include "OgreTextureGpuManager.h"
#include "OgreTextureGpuManagerListener.h"

#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"
#include "MessageBox/MessageBox.h"
#include "Base/StatisticInfo.h"
#include "EngineResourceListener.h"
#include "shader/HlmsBaseListenerContainer.h"
#include "utilities/rapidxml.hpp"

#include "defines.h"

// Factories and modules
#include "PluginFactory.h"

// Used custom plugins
// #include <FaderPlugin.h>
#include <boost/thread.hpp>

namespace NOWA
{
	class Fader;
	class AnimationBlender;

	/**
	* @class CoreConfiguration
	* @brief		Sets the core configuration
	* @param[in]	wndTitle			The window title, to be shown (useful if there are due to network capabilities more windows open to distinguish the windows)
	* @param[in]	graphicsConfigName	The configuration cfg filename to get all necessary graphics settings. If not specified, default is the ogre.cfg file.
	*									If the file does not exist, a new window appears to set the graphics settings
	* @param[in]	resourcesName		The resources cfg filename, where all media resources are linked. Default is the "resources.cfg" file in the release/debug folder.
	* @param[in]	customConfigName	The custom configuration xml filename for custom configuration
	* @param[in]	startProjectName	The optional start project file name, that is project name and scene name. For example: "MyProject1/MyScene1.scene"
	* @param[in]	customParams		The optional custom string parameters for customizing the window: 
	        parentWindowHandle       -> parentHwnd
			externalWindowHandle     -> externalHandle
			border : none, fixed
			outerDimensions
			monitorIndex
			useFlipMode
			vsync
			vsyncInterval
			alwaysWindowedMode
			enableDoubleClick
			left
			top

	*/
	struct CoreConfiguration
	{
		CoreConfiguration()
			: wndTitle("Application"),
			graphicsConfigName("ogre.cfg"),
			resourcesName("resources.cfg"),
			customConfigName("defaultConfig.xml"),
			isGame(true)
		{

		}

		Ogre::String wndTitle;
		Ogre::String graphicsConfigName;
		Ogre::String customConfigName;
		Ogre::String resourcesName;
		Ogre::String startProjectName;
		Ogre::NameValuePairList customParams;
		bool isGame;
	};

	class ResourceLoadingListenerImpl : public Ogre::ResourceLoadingListener
	{
	public:
		ResourceLoadingListenerImpl();

		virtual ~ResourceLoadingListenerImpl();
	protected:

		/** This event is called when a resource beings loading. */
		virtual Ogre::SharedPtr<Ogre::DataStream> resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource) override;

		/// Gets called when a groupless manager (like TextureGpuManager) wants to check if there's
		/// a resource with that name provided by this listener.
		/// This function is called from main thread.
		virtual bool grouplessResourceExists(const Ogre::String& name) override;

		/// Gets called when a groupless manager (like TextureGpuManager) loads a resource.
		/// WARNING: This function is likely going to be called from a worker thread.
		virtual Ogre::SharedPtr<Ogre::DataStream> grouplessResourceLoading(const Ogre::String& name) override;

		/// Similar to resourceStreamOpened, gets called when a groupless manager has already
		/// opened a resource and you may want to modify the stream.
		/// If grouplessResourceLoading has been called, then this function won't.
		/// WARNING: This function is likely going to be called from a worker thread.
		virtual Ogre::SharedPtr<Ogre::DataStream> grouplessResourceOpened(const Ogre::String& name, Ogre::Archive* archive, Ogre::SharedPtr<Ogre::DataStream>& dataStream) override;

		/** This event is called when a resource stream has been opened, but not processed yet.
		@remarks
			You may alter the stream if you wish or alter the incoming pointer to point at
			another stream if you wish.
		*/
		virtual void resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::SharedPtr<Ogre::DataStream>& dataStream) override;

		/** This event is called when a resource collides with another existing one in a resource manager
		  */
		virtual bool resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager) override;

	};

	/** 
	* @class Core
	* @brief This singleton class is responsible for the maintainance of Ogre relevant settings and has also many helper functions
	*/

	class EXPORTED Core : public Ogre::Singleton<Core>, public Ogre::WindowEventListener, public Ogre::DefaultTextureGpuManagerListener,
		OIS::KeyListener, OIS::MouseListener, OIS::JoyStickListener /*, Ogre::RenderSystem::Listener*/
	{
	public:
		friend class AppStateManager;
		friend class AppState;
		friend class DotSceneImportModule;
		friend class DotSceneExportModule;
		friend class FollowCamera2D; // Bounds function
	public:

		Core();
		~Core();

		/**
		* @brief		Gets access to the singleton instance via reference
		* @note			Since this library gets exported, this two methods must be overwritten, in order to export this library as singleton too
		*/
		static Core& getSingleton(void);

		/**
		* @brief		Gets access to the singleton instance via pointer
		* @note			Since this library gets exported, this two methods must be overwritten, in order to export this library as singleton too
		*/
		static Core* getSingletonPtr(void);

		/**
		 * @brief		Initializes the core module (graphics, sound, etc.)
		 * @param[in]	coreConfiguration	The configuration parameters
		 * @see			CoreConfiguration
		 * @return		true	If the initialization was successful
		 */
		bool initialize(const CoreConfiguration& coreConfiguration);

		/**
		 * @brief		Actions on key pressed event.
		 * @see			OIS::KeyListener::keyPressed()
		 * @param[in]	keyEventRef		The key event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

		/**
		 * @brief		Actions on key released event.
		 * @see			OIS::KeyListener::keyReleased()
		 * @param[in]	keyEventRef		The key event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

		/**
		 * @brief		Actions on mouse moved event.
		 * @see			OIS::MouseListener::mouseMoved()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

		/**
		 * @brief		Actions on mouse pressed event.
		 * @see			OIS::MouseListener::mousePressed()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		/**
		 * @brief		Actions on mouse released event.
		 * @see			OIS::MouseListener::mouseReleased()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
		
		/**
		 * @brief		Actions on joyStick axis moved event.
		 * @see			OIS::JoyStickListener::axisMoved()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;
		
		/**
		 * @brief		Actions on joyStick button pressed event.
		 * @see			OIS::JoyStickListener::buttonPressed()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;
		
		/**
		 * @brief		Actions on joyStick button released event.
		 * @see			OIS::JoyStickListener::buttonReleased()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;
		
		/**
		 * @brief		Actions on joyStick pov moved event.
		 * @see			OIS::JoyStickListener::povMoved()
		 * @param[in]	evt		The joyStick event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		// virtual bool povMoved(const OIS::JoyStickEvent& evt, int pov);

		/*
		 * @brief A rendersystem-specific event occurred.
         * @param eventName The name of the event which has occurred
         * @param parameters A list of parameters that may belong to this event,
         * may be null if there are no parameters
         */
		// virtual void eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList* parameters = 0) override;

		/**
		 * @brief		Loads a custom configuration XML file for custom menue settings.
		 * @param[in]	strFilename		The filename for the XML file
		 */
		void loadCustomConfiguration(const char *strFilename);

		/**
		 * @brief		Stores the custom configuration XML file.
		 */
		void saveCustomConfiguration(void);

		/**
		 * @brief		Changes the window title during runtime. Usefull when using network capabilities and having several windows open, to adapt the window title.
		 * @param[in]	title		The new title to be shown
		 */
		void changeWindowTitle(const Ogre::String &title);

		/**
		 * @brief		Gets the application name
		 * @see			OIS::JoyStickListener::povMoved()
		 * @param[in]	evt		The joyStick event
		 * @return		true	affect actions
		 */
		Ogre::String getApplicationName(void) const;

		/**
		 * @brief		Sets the window position
		 * @param[in]	x		The x coordinate
		 * @param[in]	y		The y coordinate
		 */
		void setWindowPosition(int x, int y);

		/**
		 * @brief		Gets the desktop size
		 * @param[out]	width		The width of the desktop
		 * @param[out]	height		The height of the deskop
		 */
		void getDesktopSize(int& width, int& height);

		/**
		 * @brief	Moves the window to the taskbar
		 */
		void moveWindowToTaskbar(void);

		/**
		 * @brief		Gets window handle id for a scenario like embedding this window into another application.
		 * @return		hwnd	the handle window to get
		 */
		size_t getWindowHandle(void) const;

		// @brief Adjust mouse clipping area
		virtual void windowResized(Ogre::Window* renderWindow) override;

		// @brief Unattach OIS before window shutdown (very important under Linux)
		virtual void windowClosed(Ogre::Window* renderWindow) override;

		virtual void windowMoved(Ogre::Window* renderWindow) override;

        virtual bool windowClosing(Ogre::Window* renderWindow) override;

        virtual void windowFocusChange(Ogre::Window* renderWindow) override;

		/**
		 * @brief	Moves the window in z-order front
		 */
		void moveWindowToFront(void);

		bool checkStorage(const DWORDLONG diskSpaceNeeded, DWORDLONG& thisDiscSpace);

		DWORD readCPUSpeed();

		bool checkMemory(const DWORDLONG physicalRAMNeeded, const DWORDLONG virtualRAMNeeded, DWORDLONG& thisRAMPhysical, DWORDLONG& thisRAMVirtual);
		
		void createApplicationIcon(unsigned short iconResourceId);

#ifdef WIN32
		/**
		 * @brief		Gets the current thread count used in this application.
		 * @return		threadCount		
		 */
		int getCurrentThreadCount(void);
#endif

		/**
		 * @brief		Checks and gets whether this application is the only instance.
		 * @return		onlyOne True, if its the only one else false
		 */
		bool isOnlyInstance(LPCTSTR title);

		/**
		 * @brief						Checks if a process is running.
		 * @param[in] executableName	Name of process to check if is running
		 * @return True					if the process is running, or False if the process is not running
		 */
		bool isProcessRunning(const TCHAR* const executableName);

		/*
		* @brief		Gets the save file path name for this pc, which is in the user directory. Necessary directories will be created automatically.
		* @param[in] saveName	The save name to add as save file path name
		* @param[in] fileEnding	Adds the given file ending, if does not exist.
		* @return			The whole save file path name with (*.sav) ending
		*/
		Ogre::String getSaveFilePathName(const Ogre::String& saveName, const Ogre::String& fileEnding = ".sav");

		/*
		* @brief		Creates a folder for the given path
		* @param[in] path	The path to create the folder
		* @return			true If the folder could be created, else false
		*/
		bool createFolder(const Ogre::String& path);
		
		/*
		* @brief		Creates all folders for the given path
		* @param[in] path	The path to create the folders
		*/
		void createFolders(const Ogre::String& path);

		/*
		* @brief	Gets the file name from path
		* @param[in] filePath	The whole file path to extract the file name
		* @return fileName  The file name extracted out of the path
		*/
		Ogre::String getFileNameFromPath(const Ogre::String& filePath);
		
		/*
		* @brief	Gets the project name from path
		* @param[in] filePath	The whole file path to extract the project name
		* @return projectName  The project name extracted out of the path
		*/
		Ogre::String getProjectNameFromPath(const Ogre::String& filePath);

		/*
		* @brief	Gets absolute path from a relative path
		* @param[in] relativePath	The relative path to set
		* @return absolutePath  The absolute path to get
		* @note		The absolute path can only be constructed, if the relative path does exist within the project
		*/
		Ogre::String getAbsolutePath(const Ogre::String& relativePath);

		/*
		* @brief	Removes the given parts from a string
		* @param[in] source			The source string to operate on
		* @param[in] parts			The parts to remove
		* @return success, target	Success, if something has been removed and the target manipulated string.
		* @note		E.g. remove illegal chars from a string like []*()'
		*/
		std::pair<bool, Ogre::String> removePartsFromString(const Ogre::String& source, const Ogre::String& parts);

		/**
		 * @brief		Copies the given text to global clipboard for paste operations
		 * @param[in]	text	The text to copy
		 */
		void copyTextToClipboard(const Ogre::String& text);

		Ogre::String getApplicationFilePathName(void);

		Ogre::String getDirectoryNameFromFilePathName(const Ogre::String& filePathName);

		Ogre::String getRootFolderName(void);

		Ogre::String replaceSlashes(const Ogre::String& filePath, bool useBackwardSlashes);

		std::vector<Ogre::String> getSceneFileNames(const Ogre::String& resourceGroupName, const Ogre::String& projectName);

		std::vector<Ogre::String> getFilePathNamesInProject(const Ogre::String& projectName, const Ogre::String& pattern);

		std::vector<Ogre::String> getSceneSnapshotsInProject(const Ogre::String& projectName);

		std::vector<Ogre::String> getSaveNamesInProject(const Ogre::String& projectName);

		std::vector<Ogre::String> getFilePathNames(const Ogre::String& resourceGroupName, const Ogre::String& folder, const Ogre::String& pattern, bool recursive = false);

		void openFolder(const Ogre::String& filePathName);

		void displayError(LPCTSTR errorDesc, DWORD errorCode);

		/**
		 * @brief		Gets the factory for plugin managment and for registration of own plugins.
		 * @see			FaderPlugin as an example.
		 * @return		PluginFactory	The plugin factory
		 */
		PluginFactory* getPluginFactory(void) const;

		/**
		 * @brief		Gets a list with paths for a given section. 
		 * For example: "ParicleUniverse", would deliver: ../../media/ParticleUniverse/materials, ../../media/ParticleUniverse/models in the specified *.cfg file.
		 * @param[in]	mx				The current mouse x coordinate
		 * @param[in]	my				The current mouse y coordinate
		 * @param[out]	x				The resulting mouse x coordinate
		 * @param[out]	y				The resulting mouse y coordinate
		 * @return		list			A list of paths to the section
		 */
		Ogre::StringVector getSectionPath(const Ogre::String& strSection);

		/**
		 * @brief		Creates a camera with some menue settings like far/near clip distance etc.
		 * @param[in]	sceneManager	The current scene manager for Ogre graphics
		 * @param[in]	strCameraName	The camera name
		 * @return		camera			The camera with menue settings
		 */
		Ogre::Camera* createCameraWithMenuSettings(Ogre::SceneManager* sceneManager, const Ogre::String& strCameraName);

		/**
		* @brief		Sets the configured menu quality settings (LOD bias, Mipmaps, texture filtering, anisotropy)
		* @param[in]	camera		The camera to set the settings to
		*/
		void setMenuSettingsForCamera(Ogre::Camera* camera);

		/**
		 * @brief		Sets polygon mode for all loaded geometries.
		 * @param[in]	mode	The render mode to set (3 = Solid, 2 = Wireframe, 1 = Points)
		 */
		void setPolygonMode(unsigned short mode);

		/**
		 * @brief		Builds an tree of all scenenodes and movable object (e.g. Entity, lights etc.) hierarchically and prints it into the *.log file for debug purposes. 
		 * @param[in]	pNode			The node to dump all childnodes and objects that are attached to that node
		 * @param[in]	padding			The appereance of the hierarchy
		 */
		void dumpNodes(Ogre::Node* pNode, Ogre::String padding = "   ");

		/**
		 * @brief		Clears the whole scene including deletion all cameras and destroys the scenemanager
		 * @param[in]	sceneManager			The scene manager to destroy
		 */
		void destroyScene(Ogre::SceneManager*& sceneManager);

		Ogre::Root* getOgreRoot(void) const { return this->root; }

		Ogre::Window* getOgreRenderWindow(void) const { return this->renderWindow; }

		Ogre::v1::OverlaySystem* getOverlaySystem(void) const { return this->overlaySystem; }

		EngineResourceListener* getEngineResourceListener(void) const { return this->engineResourceListener != nullptr ? this->engineResourceListener : this->defaultEngineResourceListener; }

		void setEngineResourceListener(EngineResourceListener* engineResourceListener) { this->engineResourceListener = engineResourceListener; }

		void resetEngineResourceListener(void) { this->engineResourceListener = this->defaultEngineResourceListener; }

		void setSceneManagerForMyGuiPlatform(Ogre::SceneManager* sceneManager);

		Ogre::CompositorWorkspace* getMyGuiWorkspace(void) const { return this->myGuiWorkspace; };

		/// Is used for updating the GUI
		// Ogre::FrameEvent getFrameEvent(void) const { return this->frameEvent; }

		Ogre::String getGraphicsConfigName(void) const { return this->graphicsConfigName; }

		void setResourcesName(const Ogre::String& resourcesName) { this->resourcesName = resourcesName; }

		Ogre::String getResourcesName(void) const { return this->resourcesName; }

		std::set<Ogre::String> getResourcesGroupNames(void) const { return this->resourceGroupNames; }
		
		void setProjectName(const Ogre::String& projectName);
		
		Ogre::String getProjectName(void) const;

		Ogre::String getWorldName(void) const;

		Ogre::String getCurrentWorldPath(void) const;

		Ogre::String getCurrentProjectPath(void) const;

		Ogre::Vector3 getCurrentWorldBoundLeftNear(void);

		Ogre::Vector3 getCurrentWorldBoundRightFar(void);

		/**
		 * @brief		Gets whether the engine is used in a game and not in an editor.
		 * @return		True, if engine is used in a game, else false.
		 */
		bool getIsGame(void) const;

		void setOptionLODBias(Ogre::Real optionLODBias) { this->optionLODBias = optionLODBias; }

		Ogre::Real getOptionLODBias(void) const { return this->optionLODBias; }

		void setOptionTextureFiltering(int optionTextureFiltering) { this->optionTextureFiltering = optionTextureFiltering; }

		int	getOptionTextureFiltering(void) const { return this->optionTextureFiltering; }

		void setOptionAnisotropyLevel(int optionAnisotropyLevel) { this->optionAnisotropyLevel = optionAnisotropyLevel; }

		int	getOptionAnisotropyLevel(void) const { return this->optionAnisotropyLevel; }

		void setOptionSoundVolume(int optionSoundVolume) { this->optionSoundVolume = optionSoundVolume; }

		int	getOptionSoundVolume(void) const { return this->optionSoundVolume; }

		void setOptionMusicVolume(int optionMusicVolume) { this->optionMusicVolume = optionMusicVolume; }

		int getOptionMusicVolume(void) const { return this->optionMusicVolume; }

		/**
		 * @brief		Gets the desired frames updates for graphics rendering. Default value is the max monitor refresh rate (e.g. on a 144hz monitor 144 frames per second).
		 * @return		The desired frames updates
		 */
		unsigned int getOptionDesiredFramesUpdates(void) const { return this->optionDesiredFramesUpdates; }

		/**
		 * @brief		Gets the desired simulation updates (tick count). Default value 30 ticks per second.
		 * @return		The desired simulation updates
		 */
		unsigned int getOptionDesiredSimulationUpdates(void) const { return this->optionDesiredSimulationUpdates; }

		void setOptionLogLevel(char optionLogLevel) { this->optionLogLevel = optionLogLevel; }

		char getOptionLogLevel(void) const { return this->optionLogLevel; }

		void setShutdown(bool bShutdown) { this->bShutdown = bShutdown; }

		bool isShutdown(void) const { return this->bShutdown; }

		bool isStartedAsServer(void) const { return this->startAsServer; }

		void setOptionAreaOfInterest(unsigned int optionAreaOfInterest) { this->optionAreaOfInterest = optionAreaOfInterest; }
		
		unsigned int getOptionAreaOfInterest(void) const { return this->optionAreaOfInterest; }

		void setOptionPacketSendRate(unsigned int optionPacketSendRate) { this->optionPacketSendRate = optionPacketSendRate; }

		unsigned int getOptionPacketSendRate(void) const { return this->optionPacketSendRate; }

		void setOptionInterpolationRate(unsigned int optionInterpolationRate) { this->optionInterpolationRate = optionInterpolationRate; }

		unsigned int getOptionInterpolationRate(void) const { return this->optionInterpolationRate; }

		void setOptionPacketsPerSecond(unsigned int optionPacketsPerSecond) { this->optionPacketsPerSecond = optionPacketsPerSecond; }

		unsigned int getOptionPacketsPerSecond(void) const { return this->optionPacketsPerSecond; }

		void setOptionPlayerColor(unsigned int optionPlayerColor) { this->optionPlayerColor = optionPlayerColor; }

		unsigned int getOptionPlayerColor(void) const { return this->optionPlayerColor; }

		bool getOptionLuaConsoleUsed(void) const { return this->optionUseLuaConsole; }

		bool getOptionLuaScriptUsed(void) const { return this->optionUseLuaScript; }

		Ogre::String getOptionLuaGroupName(void) const { return this->optionLuaGroupName; }

		Ogre::String getOptionLuaApiFilePath(void) const { return this->optionLuaApiFilePath; }

		void setOptionLanguage(int optionLanguage) { this->optionLanguage = optionLanguage; }

		int getOptionLanguage(void) const { return this->optionLanguage; }

		void setOptionQualityLevel(int optionQualityLevel)  { this->optionQualityLevel = optionQualityLevel; }
		
		int getOptionQualityLevel(void) const { return this->optionQualityLevel; }

		void setGlobalRenderDistance(Ogre::Real globalRenderDistance) { this->globalRenderDistance = globalRenderDistance; }

		Ogre::Real getGlobalRenderDistance(void) const { return this->globalRenderDistance; }

		inline Ogre::Timer* getOgreTimer(void) const { return this->timer; }

		/**
		 * @brief		Gets all the available texture names
		 * @param[in]	filters			The filters to set for texture name extensions. When empty, all formats will be delivered. E.g. { "png", "jpg", "bmp", "tga", "gif", "tif", "dds" }
		 * @return		All texture names in alphabetical order
		 */
		std::set<Ogre::String> getAllAvailableTextureNames(std::vector<Ogre::String>& filters = std::vector<Ogre::String>());

		/**
		 * @brief		Gets the resource file path name.
		 * @param[in]	resourceName		The resource name with file ending to set. E.g. arrow.png.
		 * @return		Resource file location, or empty if cannot be determined.
		 */
		Ogre::String getResourceFilePathName(const Ogre::String& resourceName);

		/**
		 * @brief		Gets the read content from the given start offset and size.
		 * @param[in]	filePathName		The file path name to read the content
		 * @param[in]	startOffset			The start offset, where to start to read
		 * @param[in]	length				The read length up from the start offset.
		 * @return		The content as string, or empty, if file could not be found etc.
		 */
		Ogre::String readContent(const Ogre::String& filePathName, unsigned int startOffset, unsigned int size);

		void minimizeMemory(Ogre::SceneManager* sceneManager);
		
		/**
		 * @brief		Encodes the given text in base 64 and optionally also ciphers the text
		 * @param[in]	text			The text to encode
		 * @param[in]	cipher			Whether to cipher the text
		 * @return		encodedText		The not readable encoded text
		 */
		Ogre::String encode64(const Ogre::String& text, bool cipher);

		/**
		 * @brief		Decodes the given text in base 64 and optionally also de-ciphers the text
		 * @param[in]	text			The text to decode
		 * @param[in]	cipher			Whether to de-cipher the text
		 * @return		decodedText		The readable decoded text
		 */
		Ogre::String decode64(const Ogre::String& text, bool cipher);

		/**
		 * @brief		Sets the crypt key for all sipher operations (lua files, save game cryption)
		 * @param[in]	key			The key to set
		 */
		void setCryptKey(int key);

		/**
		* @brief		Gets the crypt key for all sipher operations (lua files, save game cryption)
		* @return		the key number
		*/
		int getCryptKey(void) const;

		/**
		* @brief		Gets whether by default game objects are created via entity type or item type.
		* @return		true if entity type is used, else item type.
		*/
		bool getUseEntityType(void) const;

		void setUseEntityType(bool useEntityType);

		void encodeAllFiles(void);

		bool decodeAllFiles(int key);
		
		/**
		* @brief		Gets whether the currently loaded project is encoded (not readable)
		* @return		true	if the project is encoded else false
		*/
		bool isProjectEncoded(void) const;

		/**
		* @brief		Gets the current date and time. The default format is in the form Year_Month_Day_Hour_Minutes_Seconds.
		* @param[in]	format		The optional format to set.
		* @return		dateAndTime The current date and time as string
		*/
		Ogre::String getCurrentDateAndTime(const Ogre::String& format = "%Y_%m_%d_%X") const;

		/**
		* @brief		Sets the current global save game name, which can be used for loading a game in an application state.
		* @param[in]	saveGameName		The save game name to set.
		*/
		void setCurrentSaveGameName(const Ogre::String& saveGameName);

		/**
		* @brief		Gets the current save game name, or empty string, if does not exist.
		* @return		saveGameName The current save game name to get
		*/
		Ogre::String getCurrentSaveGameName(void) const;

		/**
		* @brief		Gets the Hlms bas listener container, which can add concrete listeners for existing hlms pbs implementation, to be executed
		* @return		baseListenerContainer The base listener container to get
		*/
		HlmsBaseListenerContainer* getBaseListenerContainer(void) const;

		void refreshHlms(bool useFog, bool useWind, bool useTerra, bool useOcean);

		void setTightMemoryBudget(void);

		void setRelaxedMemoryBudget(void);

		void switchFullscreen(bool bFullscreen, unsigned int monitorId, unsigned int width, unsigned int height);

		Ogre::String dumpGraphicsMemory(void);

		Ogre::String getLogName(void) const { return this->logName; }

		unsigned int getScreenRefreshRate(void);

		/**
		* @brief		Processes the MeshMagick mesh operations like rotation, resize, transform, origin, axes.
		* @see			https://wiki.ogre3d.org/MeshMagick
		* @param[in]	meshName	The mesh name (without path) to set.
		* @param[in]	parameters	The operation parameters to set without the mesh name. E.g. 'transform -rotate=90/0/1/0'
		* @return		True		If the operation was successful, else false.
		*/
		bool processMeshMagick(const Ogre::String& meshName, const Ogre::String& parameters);
		
	public:

		enum PhysicsType
		{
			TERRAINTYPE = 0,
			PLAYERTYPE = 1,
			ENEMYTYYPE,
			STATICOBJECTTYPE,
			DYNAMICOBJECTTYPE,
			BOXTYPE,
			CAPSULETYPE,
			CHAMFERCYLINDERTYPE,
			CONETYPE,
			CONVEXHULLTYPE,
			CYLINDERTYPE,
			ELLIPSOIDTYPE,
			PYRAMIDTYPE,
			BULLETTYPE,
			BUILDINGINTERNTYPE,
			TREETYPE,
			DOORTYPE,
			PLATETYPE,
			BALLTYPE
		};

		//Ogre Type
		enum eQueryFlags
		{
			UNUSEDMASK = 1 << 0,
			GAMEOBJECTMASK = 1 << 1
		};
	private:
		void update(Ogre::Real dt);
		void updateFrameStats(Ogre::Real dt);
		void registerHlms(void);
		void initMyGui(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::String& logName);
		void setCurrentWorldPath(const Ogre::String& currentWorldPath);
		void setCurrentWorldBounds(const Ogre::Vector3& mostLeftNearPosition, const Ogre::Vector3& mostRightFarPosition);

		void loadHlmsDiskCache(void);

		void saveHlmsDiskCache(void);

		template <typename T, size_t MaxNumTextures>
		void unloadTexturesFromUnusedMaterials(Ogre::HlmsDatablock *datablock, std::set<Ogre::TextureGpu*>& usedTex, std::set<Ogre::TextureGpu*>& unusedTex);
		/// Unloads textures that are only bound to unused materials/datablock
		void unloadTexturesFromUnusedMaterials(void);

		/// Unloads textures that are currently not bound to anything (i.e. not even to a material)
		/// Use with caution, as it may unload textures that are used for something else
		/// and the code may not account for it.
		void unloadUnusedTextures(void);

		void initMiscParamsListener(Ogre::NameValuePairList& params);

		/**
		 * @brief		Gets the save game directory on this pc
		 * @param[in]	saveName	The save name
		 * @return		path		returns the path for including the save name
		 */
		Ogre::String getSaveGameDirectory(const Ogre::String& saveName);

		Ogre::String encrypt(const Ogre::String& text, int key);

		Ogre::String decrypt(const Ogre::String& text, int key);

		void createCustomTextures(void);
	private:
		Ogre::Root* root;
		Ogre::Window* renderWindow;
		Ogre::v1::OverlaySystem* overlaySystem;
		Ogre::CompositorWorkspace* myGuiWorkspace;
		PluginFactory* pluginFactory;
		Ogre::Timer* timer;
		
		Ogre::String writeAccessFolder;
		bool isGame;
		Ogre::String saveGameName;
		
		MyGUI::Gui* myGui;
		MyGUI::Ogre2Platform* myGuiOgrePlatform;
		EngineResourceListener* engineResourceListener;
		EngineResourceRotateListener* defaultEngineResourceListener;
		Ogre::String graphicsConfigName;
		Ogre::String resourcesName;
		Ogre::String customConfigName;
		Ogre::String title;
		Ogre::String currentWorldPath;
		Ogre::String currentProjectPath;
		Ogre::String projectName;
		Ogre::String worldName;
		std::set<Ogre::String> resourceGroupNames;

		Ogre::Vector3 mostLeftNearPosition;
		Ogre::Vector3 mostRightFarPosition;

		/// Menue options
		Ogre::Real optionLODBias;
		int	optionTextureFiltering;
		int	optionAnisotropyLevel;
		int optionQualityLevel;
		int	optionSoundVolume;
		int	optionMusicVolume;
		bool optionUseLuaConsole;
		bool optionUseLuaScript;
		Ogre::String optionLuaGroupName;
		Ogre::String optionLuaApiFilePath;
		unsigned int optionAreaOfInterest;
		unsigned int optionPacketSendRate;
		unsigned int optionInterpolationRate;
		unsigned int optionPacketsPerSecond;
		unsigned int optionPlayerColor;
		char optionLogLevel;
		unsigned int optionDesiredFramesUpdates;
		unsigned int optionDesiredSimulationUpdates;
		bool bShutdown;
		bool startAsServer;
		int optionLanguage;
		Ogre::String borderType;
		int requiredDiscSpaceMB;
		int requiredCPUSpeedMhz;
		int requiredRAMPhysicalMB;
		int requiredRAMVirtualMB;
		Ogre::Real globalRenderDistance;

		int cryptKey;
		bool projectEncoded;
		bool useEntityType;

		Ogre::String logName;

		diagnostic::StatisticInfo* info;
		Ogre::Real timeSinceLastStatisticUpdate;

		Ogre::ResourceLoadingListener* resourceLoadingListener;
		Ogre::TextureGpuManager::BudgetEntryVec defaultBudget;

		HlmsBaseListenerContainer* baseListenerContainer;
	};

}; //end namespace

template<> NOWA::Core* Ogre::Singleton<NOWA::Core>::msSingleton = 0;

#endif
