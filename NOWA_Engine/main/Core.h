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

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	/** 
	* @class Core
	* @brief Central singleton that manages engine core systems (Ogre, window, resources, input, plugins, etc.)
	*
	* Detailed responsibilities:
	* - Manage and expose `Ogre::Root`, render window and overlay systems.
	* - Provide input callbacks (keyboard, mouse, joystick) via OIS listeners.
	* - Offer path and resource helper utilities (absolute/relative paths, resource resolution).
	* - Load and save custom configuration and manage runtime options.
	* - Manage HLMS setup, texture preloading/unloading and memory budgets.
	* - Provide plugin factory access and engine-specific listeners.
	*
	* Notes:
	* - Core is a singleton accessible via getSingleton() / getSingletonPtr().
	* - Many methods are platform dependent (file system, clipboard, process checks).
	*/
	class EXPORTED Core : public Ogre::Singleton<Core>, public Ogre::WindowEventListener, public Ogre::DefaultTextureGpuManagerListener,
		OIS::KeyListener, OIS::MouseListener, OIS::JoyStickListener
	{
	public:
		friend class AppStateManager;
		friend class AppState;
		friend class DotSceneImportModule;
		friend class DotSceneExportModule;
		friend class FollowCamera2D; // Bounds function
	public:

		/** Constructor. Performs lightweight initialization; heavy subsystems are initialized in `initialize()`. */
		Core();
		/** Destructor. Releases allocated engine resources. */
		~Core();

		/**
		* @brief Return reference to the global Core singleton.
		* @note Required to export a singleton across DLL boundaries.
		* @return Reference to the single Core instance.
		*/
		static Core& getSingleton(void);

		/**
		* @brief Return pointer to the global Core singleton.
		* @return Pointer to the single Core instance.
		*/
		static Core* getSingletonPtr(void);

		/**
		 * @brief Initialize the engine core (graphics, audio, input, GUI, plugins).
		 * @param coreConfiguration Configuration used for initialization (window title, config filenames, custom params).
		 * @return true on success, false otherwise.
		 */
		bool initialize(const CoreConfiguration& coreConfiguration);

		/* -------------------
		 * Input listeners (OIS)
		 * ------------------- */

		/**
		 * @brief Handle key press events forwarded by OIS.
		 * @param keyEventRef Event data for the key press.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

		/**
		 * @brief Handle key release events forwarded by OIS.
		 * @param keyEventRef Event data for the key release.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

		/**
		 * @brief Handle mouse movement events.
		 * @param evt Mouse movement event containing position and relative motion.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

		/**
		 * @brief Handle mouse button press events.
		 * @param evt Mouse event information.
		 * @param id Identifier of the mouse button pressed.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		/**
		 * @brief Handle mouse button release events.
		 * @param evt Mouse event information.
		 * @param id Identifier of the mouse button released.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
		
		/**
		 * @brief Handle joystick axis movement events.
		 * @param evt Joystick event containing axis values.
		 * @param axis Index of the axis that moved.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;
		
		/**
		 * @brief Handle joystick button press events.
		 * @param evt Joystick event information.
		 * @param button Index of the pressed button.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;
		
		/**
		 * @brief Handle joystick button release events.
		 * @param evt Joystick event information.
		 * @param button Index of the released button.
		 * @return true if other listeners should also process the event; false to consume it.
		 */
		virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;
		
		/**
		 * @brief (Optional) Handle joystick POV (hat) changes.
		 * @note Currently commented out; enable if POV handling is required.
		 */
		// virtual bool povMoved(const OIS::JoyStickEvent& evt, int pov);

		/* -------------------
		 * Configuration and runtime utilities
		 * ------------------- */

		/**
		 * @brief Load a custom XML configuration file that contains user or UI settings.
		 * @param strFilename Path to the XML configuration file.
		 */
		void loadCustomConfiguration(const char *strFilename);

		/**
		 * @brief Save the current custom configuration (user settings) to disk.
		 */
		void saveCustomConfiguration(void);

		/**
		 * @brief Change the application window title during runtime.
		 * @param title New window title.
		 */
		void changeWindowTitle(const Ogre::String &title);

		/**
		 * @brief Return the current application name (window title or configured app name).
		 * @return Application name as an `Ogre::String`.
		 */
		Ogre::String getApplicationName(void) const;

		/**
		 * @brief Set the native window position.
		 * @param x X coordinate in pixels.
		 * @param y Y coordinate in pixels.
		 */
		void setWindowPosition(int x, int y);

		/**
		 * @brief Query the desktop resolution.
		 * @param[out] width Desktop width in pixels.
		 * @param[out] height Desktop height in pixels.
		 */
		void getDesktopSize(int& width, int& height);

		/**
		 * @brief Minimize or move the application window to the taskbar.
		 */
		void moveWindowToTaskbar(void);

		/**
		 * @brief Return the native window handle (platform-specific, e.g. HWND on Windows).
		 * @return Native handle represented as a size_t.
		 */
		size_t getWindowHandle(void) const;

		/* -------------------
		 * Window events (Ogre::WindowEventListener)
		 * ------------------- */

		/**
		 * @brief Called when the render window is resized.
		 * @param renderWindow Pointer to the resized Ogre::Window.
		 */
		virtual void windowResized(Ogre::Window* renderWindow) override;

		/**
		 * @brief Called when the render window is closed. Used to detach input systems or perform cleanup.
		 * @param renderWindow Pointer to the closing Ogre::Window.
		 */
		virtual void windowClosed(Ogre::Window* renderWindow) override;

		/**
		 * @brief Called when the render window is moved.
		 * @param renderWindow Pointer to the moved Ogre::Window.
		 */
		virtual void windowMoved(Ogre::Window* renderWindow) override;

        /**
		 * @brief Called when the render window is about to close (can be used to veto closing).
		 * @param renderWindow Pointer to the Ogre::Window.
		 * @return true to allow closing, false to cancel.
		 */
        virtual bool windowClosing(Ogre::Window* renderWindow) override;

        /**
		 * @brief Called when the render window gains or loses focus.
		 * @param renderWindow Pointer to the Ogre::Window whose focus changed.
		 */
        virtual void windowFocusChange(Ogre::Window* renderWindow) override;

		/**
		 * @brief Bring the application window to the foreground (top of Z-order).
		 */
		void moveWindowToFront(void);

		/**
		 * @brief Check available disk storage against a required amount.
		 * @param diskSpaceNeeded Required space in bytes (DWORDLONG).
		 * @param[out] thisDiscSpace Filled with available disk space on the target drive.
		 * @return true if there is at least `diskSpaceNeeded` available, false otherwise.
		 */
		bool checkStorage(const DWORDLONG diskSpaceNeeded, DWORDLONG& thisDiscSpace);

		/**
		 * @brief Read and return CPU clock speed in MHz (platform dependent).
		 * @return CPU clock speed in MHz, or 0 on failure.
		 */
		DWORD readCPUSpeed();

		/**
		 * @brief Check physical and virtual memory availability.
		 * @param physicalRAMNeeded Required physical RAM in bytes.
		 * @param virtualRAMNeeded Required virtual RAM in bytes.
		 * @param[out] thisRAMPhysical Actual available physical RAM in bytes.
		 * @param[out] thisRAMVirtual Actual available virtual RAM in bytes.
		 * @return true if both physical and virtual requirements are met, false otherwise.
		 */
		bool checkMemory(const DWORDLONG physicalRAMNeeded, const DWORDLONG virtualRAMNeeded, DWORDLONG& thisRAMPhysical, DWORDLONG& thisRAMVirtual);
		
		/**
		 * @brief Create or register an application icon resource.
		 * @param iconResourceId Resource identifier for the icon (platform dependent).
		 */
		void createApplicationIcon(unsigned short iconResourceId);

#ifdef WIN32
		/**
		 * @brief Return the number of threads currently used by the application.
		 * @return Thread count (platform-specific).
		 */
		int getCurrentThreadCount(void);
#endif

		/**
		 * @brief Check whether this process is the only running instance with the given title.
		 * @param title Window title or unique identifier used to detect other instances.
		 * @return true if no other instance is running, false otherwise.
		 */
		bool isOnlyInstance(LPCTSTR title);

		/**
		 * @brief Check whether a process with the specified executable name is running.
		 * @param executableName Executable name (platform dependent string type).
		 * @return true if a process with that executable name is found, false otherwise.
		 */
		bool isProcessRunning(const TCHAR* const executableName);

		/* -------------------
		 * Save / filesystem helpers
		 * ------------------- */

		/**
		* @brief Build a save-file path inside the user directory and ensure necessary directories exist.
		* @param saveName Base save name (folder or file) to append to user save directory.
		* @param fileEnding Optional file extension to append if not present (default ".sav").
		* @return Full path to the save file (including extension).
		*/
		Ogre::String getSaveFilePathName(const Ogre::String& saveName, const Ogre::String& fileEnding = ".sav");

		/**
		* @brief Create a single folder on disk.
		* @param path Folder path to create.
		* @return true if the folder was created or already exists, false on error.
		*/
		bool createFolder(const Ogre::String& path);
		
		/**
		* @brief Create all directories required for the provided path (recursive).
		* @param path Folder path to create recursively.
		*/
		void createFolders(const Ogre::String& path);

		/**
		* @brief Extract file name from a full path.
		* @param filePath Full file path.
		* @return File name component (with extension) or empty string if none.
		*/
		Ogre::String getFileNameFromPath(const Ogre::String& filePath);
		
		/**
		* @brief Extract the project name from a project scene path.
		* @param filePath Scene or project file path (e.g. "../media/projects/blub/scene1/scene1.scene").
		* @return Project name (e.g. "blub") or empty string on failure.
		*/
		Ogre::String getProjectNameFromPath(const Ogre::String& filePath);

		/**
		* @brief Extract the scene name from a full scene path.
		* @param filePath Scene file path (e.g. "../media/projects/blub/scene1/scene1.scene").
		* @return Scene name (e.g. "scene1") or empty string on failure.
		*/
		Ogre::String getSceneNameFromPath(const Ogre::String& filePath);

		/**
		* @brief Extract the project root directory from a scene path.
		* @param filePath Full scene or resource path.
		* @return Project root folder path (e.g. "../media/projects/blub") or empty string if not applicable.
		*/
		Ogre::String getProjectFilePathNameFromPath(const Ogre::String& filePath);

		/**
		* @brief Extract the scene folder path from a scene file path.
		* @param filePath Full scene file path.
		* @return Scene folder path (e.g. "../media/projects/blub/scene1").
		*/
		Ogre::String getSceneFilePathNameFromPath(const Ogre::String& filePath);

		/**
		* @brief List all scene folders for a given project path.
		* @param projectFilePathName Project folder path that contains scene subfolders.
		* @return Vector of scene folder names found in the project.
		*/
		std::vector<Ogre::String> getSceneFoldersInProject(const Ogre::String& projectFilePathName);

		/**
		* @brief List all scene filenames for a given project.
		* @param projectFilePathName Project folder path to search scenes in.
		* @return Vector of scene file names (e.g. "scene1.scene", "scene2.scene").
		*/
		std::vector<Ogre::String> getSceneFileNamesInProject(const Ogre::String& projectFilePathName);

		/**
		* @brief Convert a relative project path into an absolute filesystem path, if possible.
		* @param relativePath Project-relative path to resolve.
		* @return Absolute path if resolution succeeds; empty string on failure.
		*/
		Ogre::String getAbsolutePath(const Ogre::String& relativePath);

		/**
		* @brief Remove all characters from `parts` out of `source`.
		* @param source Original string to sanitize.
		* @param parts Characters to remove from the source string.
		* @return pair<success, resultingString>. `success` is true if any character was removed.
		*/
		std::pair<bool, Ogre::String> removePartsFromString(const Ogre::String& source, const Ogre::String& parts);

		/**
		 * @brief Copy the provided text to the system clipboard.
		 * @param text Text to copy.
		 */
		void copyTextToClipboard(const Ogre::String& text);

		/**
		 * @brief Return the application's executable path (including filename) or working directory.
		 * @return Full path to the application binary or an empty string if unavailable.
         */
		Ogre::String getApplicationFilePathName(void);

		// --- Path utilities ---

		/**
		 * @brief Return true if the given path is absolute for the current platform.
		 * @param path Path to evaluate.
		 * @return true when `path` is absolute, false otherwise.
		 */
		bool isAbsolutePath(const Ogre::String& path);

		/**
		 * @brief Join two path segments and normalize separators.
		 * @param a First path segment.
		 * @param b Second path segment.
		 * @return Joined path with appropriate platform separators and no duplicate separators.
		 */
		Ogre::String joinPath(const Ogre::String& a, const Ogre::String& b);

		/**
		 * @brief Normalize all separators in `p` to forward slashes ('/').
		 * @param p Input path.
		 * @return Path normalized to forward slashes.
		 */
		Ogre::String normalizeSlashesForward(const Ogre::String& p);

		/**
		 * @brief Return the directory of the module (.dll / .so) that contains the given address.
		 * @param address Any address known to be inside the target module (e.g. function pointer).
		 * @return Absolute directory path of the module or empty string on failure.
		 */
		Ogre::String getModuleDirectoryFromAddress(const void* address);

		/**
		 * @brief Resolve a tool relative path against the module directory of `address`.
		 * @param address Address inside the module used as base.
		 * @param relativeToolPath Relative path to a tool inside the module structure.
		 * @return Absolute resolved path to the tool.
		 */
		Ogre::String resolveToolPathFromModule(const void* address, const Ogre::String& relativeToolPath);

		// --- Exec / pipes ---

		/**
		 * @brief Execute a command line and capture stdout (and optionally stderr).
		 * @param commandLine Full command line to execute.
		 * @param[out] outStdout Captured standard output (and optionally stderr).
		 * @param[out] exitCode Exit code returned by the process (-1 on failure).
		 * @param captureStderr If true, also capture and append stderr to outStdout.
		 * @return true if the process could be started and output captured, false otherwise.
		 */
		bool execAndCaptureStdout(const Ogre::String& commandLine, Ogre::String& outStdout, int& exitCode, bool captureStderr = true);

		/**
		 * @brief Return the resources folder path used by the engine.
		 * @return Absolute path to the engine resources directory.
		 */
		Ogre::String getResourcesFilePathName(void);

		/**
		 * @brief Extract directory component from a full file path.
		 * @param filePathName Absolute or relative file path.
		 * @return Directory portion without trailing separator, or empty string if not present.
		 */
		Ogre::String getDirectoryNameFromFilePathName(const Ogre::String& filePathName);

		/**
		 * @brief Return the logical root folder name used by the engine for projects or resources.
		 * @return Name of the root folder.
		 */
		Ogre::String getRootFolderName(void);

		/**
		 * @brief Replace path separators in `filePath` with backslashes or forward slashes.
		 * @param filePath Input path string.
		 * @param useBackwardSlashes true to use '\' as separator, false to use '/'.
		 * @return Path with replaced separators.
		 */
		Ogre::String replaceSlashes(const Ogre::String& filePath, bool useBackwardSlashes);

		/**
		 * @brief List scene filenames found within a resource group and project directory.
		 * @param resourceGroupName Ogre resource group to search.
		 * @param projectName Project subfolder or identifier.
		 * @return List of scene file names (not necessarily full paths).
		 */
		std::vector<Ogre::String> getSceneFileNames(const Ogre::String& resourceGroupName, const Ogre::String& projectName);

		/**
		 * @brief Find files inside a project matching a wildcard pattern.
		 * @param projectName Project folder or identifier.
		 * @param pattern Wildcard pattern for filenames (e.g. "*.scene").
		 * @return Matching file path names.
		 */
		std::vector<Ogre::String> getFilePathNamesInProject(const Ogre::String& projectName, const Ogre::String& pattern);

		/**
		 * @brief Return available scene snapshot filenames for a project (used for quick save/load thumbnails).
		 * @param projectName Project identifier or folder.
		 * @return Vector of snapshot file names.
		 */
		std::vector<Ogre::String> getSceneSnapshotsInProject(const Ogre::String& projectName);

		/**
		 * @brief Return available savegame names for a project.
		 * @param projectName Project identifier or folder.
		 * @return Vector of save game names (base names, no full paths).
		 */
		std::vector<Ogre::String> getSaveNamesInProject(const Ogre::String& projectName);

		/**
		 * @brief List files in a resource group/folder matching a pattern.
		 * @param resourceGroupName Ogre resource group.
		 * @param folder Folder inside the resource group.
		 * @param pattern Filename pattern with wildcards.
		 * @param recursive Search recursively when true.
		 * @return List of matching file paths.
		 */
		std::vector<Ogre::String> getFilePathNames(const Ogre::String& resourceGroupName, const Ogre::String& folder, const Ogre::String& pattern, bool recursive = false);

		/**
		 * @brief Extract the Ogre resource group associated with a resource path.
		 * @param path Resource path or filename.
		 * @return The resource group name if found; otherwise an empty string.
		 */
		Ogre::String extractResourceGroupFromPath(const Ogre::String& path);

		/**
		 * @brief Open the given folder in the system file explorer.
		 * @param filePathName Path to open or select in the file explorer.
		 */
		void openFolder(const Ogre::String& filePathName);

		/**
		 * @brief Display a platform-specific error message (dialog) with an error code.
		 * @param errorDesc Human readable error description.
		 * @param errorCode Platform-specific error code.
		 */
		void displayError(LPCTSTR errorDesc, DWORD errorCode);

		/**
		 * @brief Return the plugin factory instance used for plugin registration and management.
		 * @return Pointer to the `PluginFactory`.
		 */
		PluginFactory* getPluginFactory(void) const;

		/**
		 * @brief Return configured paths for a named section in the engine config (e.g. "Audio").
		 * @param strSection Section name in the configuration.
		 * @return Vector of paths associated with the given section.
		 */
		Ogre::StringVector getSectionPath(const Ogre::String& strSection);

		/**
		* @brief Apply user-configured menu quality settings to a camera (LOD bias, mipmaps, filtering, anisotropy).
		* @param camera Camera to apply the settings to.
		*/
		void setMenuSettingsForCamera(Ogre::Camera* camera);

		/**
		 * @brief Set polygon rendering mode for all geometry (solid, wireframe, points).
		 * @param mode 3=Solid, 2=Wireframe, 1=Points.
		 */
		void setPolygonMode(unsigned short mode);

		/**
		 * @brief Dump a hierarchical tree of scene nodes and attached movable objects to the log.
		 * @param pNode Root node to dump.
		 * @param padding String used for indentation in the log.
		 */
		void dumpNodes(Ogre::Node* pNode, Ogre::String padding = "   ");

		/**
		 * @brief Destroy a scene: remove all cameras, movable objects and destroy the scene manager.
		 * @param sceneManager Reference to the pointer to the scene manager to destroy (will be set to nullptr).
		 */
		void destroyScene(Ogre::SceneManager*& sceneManager);

		/**
		 * @brief Return the pointer to the internal `Ogre::Root` instance.
		 * @return `Ogre::Root*`
		 */
		Ogre::Root* getOgreRoot(void) const { return this->root; }

		/**
		 * @brief Return the internal `Ogre::Window` used for rendering.
		 * @return `Ogre::Window*`
		 */
		Ogre::Window* getOgreRenderWindow(void) const { return this->renderWindow; }

		/**
		 * @brief Return the overlay system used by the engine.
		 * @return `Ogre::v1::OverlaySystem*`
		 */
		Ogre::v1::OverlaySystem* getOverlaySystem(void) const { return this->overlaySystem; }

		/**
		 * @brief Get the active engine resource listener, falling back to the default listener if none set.
		 * @return Pointer to the active `EngineResourceListener`.
		 */
		EngineResourceListener* getEngineResourceListener(void) const { return this->engineResourceListener != nullptr ? this->engineResourceListener : this->defaultEngineResourceListener; }

		/**
		 * @brief Assign a custom engine resource listener.
		 * @param engineResourceListener Listener instance to use.
		 */
		void setEngineResourceListener(EngineResourceListener* engineResourceListener) { this->engineResourceListener = engineResourceListener; }

		/**
		 * @brief Reset the engine resource listener to the built-in default.
		 */
		void resetEngineResourceListener(void) { this->engineResourceListener = this->defaultEngineResourceListener; }

		/**
		 * @brief Configure MyGUI's Ogre2 platform with a specific scene manager.
		 * @param sceneManager Scene manager to be used by MyGUI platform for resource creation.
		 */
		void setSceneManagerForMyGuiPlatform(Ogre::SceneManager* sceneManager);

		/**
		 * @brief Return the compositor workspace used for MyGUI rendering.
		 * @return Pointer to the MyGUI compositor workspace.
		 */
		Ogre::CompositorWorkspace* getMyGuiWorkspace(void) const { return this->myGuiWorkspace; };

		/**
		 * @brief Return the currently used graphics configuration filename.
		 * @return Graphics config filename as `Ogre::String`.
		 */
		Ogre::String getGraphicsConfigName(void) const { return this->graphicsConfigName; }

		/**
		 * @brief Set the resources configuration filename to be used by the engine.
		 * @param resourcesName New resources config filename.
		 */
		void setResourcesName(const Ogre::String& resourcesName) { this->resourcesName = resourcesName; }

		/**
		 * @brief Return the current resources config filename.
		 * @return Resources filename as `Ogre::String`.
		 */
		Ogre::String getResourcesName(void) const { return this->resourcesName; }

		/**
		 * @brief Return a set with all currently registered Ogre resource group names.
		 * @return Set of registered resource group names.
		 */
		std::set<Ogre::String> getResourcesGroupNames(void) const { return this->resourceGroupNames; }
		
		/**
		 * @brief Set the current project name used by the engine.
		 * @param projectName Project name to set.
		 */
		void setProjectName(const Ogre::String& projectName);
		
		/**
		 * @brief Get current project name.
		 * @return Project name as `Ogre::String`.
		 */
		Ogre::String getProjectName(void) const;

		/**
		 * @brief Get current scene name.
		 * @return Scene name as `Ogre::String`.
		 */
		Ogre::String getSceneName(void) const;

		/**
		 * @brief Get current scene file path.
		 * @return Scene path as `Ogre::String`.
		 */
		Ogre::String getCurrentScenePath(void) const;

		/**
		 * @brief Get current project folder path.
		 * @return Project path as `Ogre::String`.
		 */
		Ogre::String getCurrentProjectPath(void) const;

		/**
		 * @brief Return the axis-aligned bounding minimum corner (left/near) of the current scene.
		 * @return Minimum corner as `Ogre::Vector3`.
		 */
		Ogre::Vector3 getCurrentSceneBoundLeftNear(void);

		/**
		 * @brief Return the axis-aligned bounding maximum corner (right/far) of the current scene.
		 * @return Maximum corner as `Ogre::Vector3`.
		 */
		Ogre::Vector3 getCurrentSceneBoundRightFar(void);

		/**
		 * @brief Check whether the current scene is in the process of being destroyed.
		 * @return true if scene destruction is in progress; false otherwise.
		 */
		bool getIsSceneBeingDestroyed(void) const;

		/**
		 * @brief Check whether the engine is running in game mode (not editor).
		 * @return true when the engine is running as a game; false when running as an editor or tools.
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
		 * @brief Get the desired simulation updates (tick count). Default value 30 ticks per second.
		 * @return The desired simulation updates
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

		void setOptionLanguage(int optionLanguage) { this->optionLanguage = optionLanguage; }

		int getOptionLanguage(void) const { return this->optionLanguage; }

		void setOptionQualityLevel(int optionQualityLevel)  { this->optionQualityLevel = optionQualityLevel; }
		
		int getOptionQualityLevel(void) const { return this->optionQualityLevel; }

		void setGlobalRenderDistance(Ogre::Real globalRenderDistance) { this->globalRenderDistance = globalRenderDistance; }

		Ogre::Real getGlobalRenderDistance(void) const { return this->globalRenderDistance; }

		inline Ogre::Timer* getOgreTimer(void) const { return this->timer; }

		/**
		 * @brief Get names of all available texture resources optionally filtered by extensions.
		 * @param filters Vector with extension filters (e.g. {"png","jpg"}). Empty -> return all supported textures.
		 * @return Sorted set of texture names.
		 */
		std::set<Ogre::String> getAllAvailableTextureNames(std::vector<Ogre::String>& filters);

		/**
		 * @brief Determine the version of a mesh (useful to check if v1 -> v2 conversion needed).
		 * @param meshName Name of the mesh resource.
		 * @return Pair<canBeConvertedToV2, versionString>.
		 */
		std::pair<bool, Ogre::String> getMeshVersion(const Ogre::String& meshName);

		/**
		 * @brief Resolve a resource name to its file path on disk if possible.
		 * @param resourceName Resource file name including extension (e.g. "arrow.png").
		 * @return Absolute or relative path to the resource or empty string if unknown.
		 */
		Ogre::String getResourceFilePathName(const Ogre::String& resourceName);

		/**
		 * @brief Read a section of a file into a string.
		 * @param filePathName Path to the file to read.
		 * @param startOffset Byte offset to start reading from.
		 * @param size Number of bytes to read.
		 * @return Content read as an `Ogre::String` or empty on failure.
		 */
		Ogre::String readContent(const Ogre::String& filePathName, unsigned int startOffset, unsigned int size);

        /**
		 * @brief Load a mesh from an absolute file path and register it with Ogre using the provided VaoManager.
		 * @param absolutePath Absolute path to the mesh file.
		 * @param meshName Name to assign to the loaded mesh resource.
		 * @param vaoManager Ogre VaoManager used for GPU resources.
		 * @return Mesh pointer on success, null pointer on failure.
		 */
        Ogre::MeshPtr loadMeshFromAbsolutePath(const Ogre::String& absolutePath, const Ogre::String& meshName, Ogre::VaoManager* vaoManager);

		/**
		 * @brief Attempt to minimize memory usage for the supplied scene manager (release caches, free unused buffers).
		 * @param sceneManager Scene manager to operate on.
		 */
		void minimizeMemory(Ogre::SceneManager* sceneManager);
		
		/**
		 * @brief Encode a string to base64 and optionally apply a symmetric cipher using `cipher` flag.
		 * @param text Plain text to encode.
		 * @param cipher When true, additionally apply engine-specific cipher before encoding.
		 * @return Base64 (and possibly ciphered) encoded string.
		 */
		Ogre::String encode64(const Ogre::String& text, bool cipher);

		/**
		 * @brief Decode a base64 (and possibly ciphered) string.
		 * @param text Encoded string to decode.
		 * @param cipher When true, decrypt after base64 decoding using engine-specific cipher.
		 * @return Decoded plain text.
		 */
		Ogre::String decode64(const Ogre::String& text, bool cipher);

		/**
		 * @brief Set the integer cryptographic key used for the engine's simple cipher operations.
		 * @param key Numeric key.
		 */
		void setCryptKey(int key);

		/**
		 * @brief Return the currently configured cryptographic key.
		 * @return Integer crypt key.
		 */
		int getCryptKey(void) const;

		/**
		 * @brief Query whether the engine uses entity types by default when creating game objects.
		 * @return true if entity type is used by default; false to use item type.
		 */
		bool getUseEntityType(void) const;

		/**
		 * @brief Set whether game objects are created via entity type or item type by default.
		 * @param useEntityType true to use entity type by default.
		 */
		void setUseEntityType(bool useEntityType);

		/**
		 * @brief Encode all project-related files using the configured crypt key (used for obfuscation).
		 */
		void encodeAllFiles(void);

		/**
		 * @brief Decode all project files using the provided key.
		 * @param key Numeric key to use for decoding.
		 * @return true if decoding succeeded for all files, false otherwise.
		 */
		bool decodeAllFiles(int key);
		
		/**
		 * @brief Check whether the currently loaded project is stored in encoded form.
		 * @return true if project files are encoded; false otherwise.
		 */
		bool isProjectEncoded(void) const;

		/**
		 * @brief Return current date/time formatted using the provided `format` string.
		 * @param format C-style format string (default "%Y_%m_%d_%X").
		 * @return Formatted date/time string.
		 */
		Ogre::String getCurrentDateAndTime(const Ogre::String& format = "%Y_%m_%d_%X") const;

		/**
		 * @brief Set the global save game name used by application states to reference saved state.
		 * @param saveGameName Save name to set.
		 */
		void setCurrentSaveGameName(const Ogre::String& saveGameName);

		/**
		 * @brief Get the global save game name previously set.
		 * @return Save name string or empty if not set.
		 */
		Ogre::String getCurrentSaveGameName(void) const;

		/**
		 * @brief Return the HLMS base listener container used to register custom HLMS listeners.
		 * @return Pointer to `HlmsBaseListenerContainer`.
		 */
		HlmsBaseListenerContainer* getBaseListenerContainer(void) const;

		/**
		 * @brief Setup HLMS (High-Level Material System). Registers HLMS types on first call and reloads PBS libraries.
		 * @param useFog When true, include fog piece files in the PBS library.
		 */
		void setupHlms(bool useFog = false);

		/**
		 * @brief Tighten GPU memory budget to reduce memory usage (unload caches, limit residency).
		 * Useful for low-memory scenarios.
		 */
		void setTightMemoryBudget(void);

		/**
		 * @brief Relax GPU memory budget to allow more textures and buffers to remain resident.
		 * Useful for high-performance scenarios.
		 */
		void setRelaxedMemoryBudget(void);

		/**
		 * @brief Switch fullscreen/windowed state, selecting a monitor and resolution.
		 * @param bFullscreen true to switch to fullscreen; false to switch to windowed.
		 * @param monitorId ID of the target monitor.
		 * @param width Desired fullscreen width (ignored in windowed mode).
		 * @param height Desired fullscreen height (ignored in windowed mode).
		 */
		void switchFullscreen(bool bFullscreen, unsigned int monitorId, unsigned int width, unsigned int height);

		/**
		 * @brief Dump current graphics memory usage and related stats to a string for diagnostic purposes.
		 * @return Diagnostic string containing GPU/texture memory usage information.
		 */
		Ogre::String dumpGraphicsMemory(void);

		/**
		 * @brief Return the internal logger name used by the engine.
		 * @return Logger name string.
		 */
		Ogre::String getLogName(void) const { return this->logName; }

		/**
		 * @brief Query the current screen refresh rate in Hz.
		 * @return Refresh rate in Hz, or 0 on failure.
		 */
		unsigned int getScreenRefreshRate(void);

		/**
		* @brief Run MeshMagick operations on a mesh (rotation, resize, transform, origin, axes).
		* @see https://wiki.ogre3d.org/MeshMagick
		* @param meshName Mesh resource name (without path).
		* @param parameters Command line parameters for MeshMagick (excluding mesh name).
		* @return true if operation succeeded, false on error.
		*/
		bool processMeshMagick(const Ogre::String& meshName, const Ogre::String& parameters);
		
		/**
		* @brief Preload textures required for the specified scene to reduce hitches during scene startup.
		* @param sceneName Scene identifier or file name.
		*/
        void preLoadSceneTextures(const Ogre::String& sceneName);

		/**
		 * @brief Return names of all plugins known to the engine (registered or configured).
		 * @return Vector with all plugin names (may include plugins not present on disk).
		 */
		std::vector<Ogre::String> getAllPluginNames(void);

		/**
		 * @brief Return names of all plugins currently available on disk and loadable.
		 * @return Vector with available plugin names.
		 */
		std::vector<Ogre::String> getAvailablePluginNames(void);

		/**
		 * @brief Record the thread id used for rendering to allow render-thread checks.
		 * @param id Thread id considered to be the render thread.
		 */
		void setRenderThreadId(std::thread::id id);

		/**
		 * @brief Check whether the current code is running in the render thread.
		 * @return true if the caller is executing in the stored render thread id.
		 */
		bool isInRenderThread(void) const;

		/**
		 * @brief Show or hide the MyGUI pointer (cursor) rendered by MyGUI.
		 * @param visible true to make the pointer visible, false to hide it.
		 */
		void setMyGuiPointerVisible(bool visible);
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

    public:
        // Static crash handling — callable before Core singleton exists (e.g. from WinMain)
        static LONG WINAPI vectoredExceptionHandler(EXCEPTION_POINTERS* exceptionInfo);

        static void printStackTrace(const Ogre::String& errorMessage, const Ogre::String& title);

        static void installCrashHandlers(void);

    private:
        struct TexturePreloadConfig
        {
            std::vector<Ogre::String> resourceGroups;
            std::vector<Ogre::String> explicitTextures; // Specific textures to force-load
            std::vector<Ogre::String> excludePatterns;  // e.g., "*_normal.png" for normals you don't need yet
            bool preloadNormalMaps = true;
            bool preloadMetalnessRoughness = true;
            size_t maxConcurrentLoads = 8; // Parallel loading limit
        };
	private:
		void update(Ogre::Real dt);
		void updateFrameStats(Ogre::Real dt);
		void cleanPluginsCfg(bool isDebug);
		void registerHlms(void);
		void initMyGui(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::String& logName);
		void setCurrentScenePath(const Ogre::String& currentScenePath);
		void setCurrentSceneBounds(const Ogre::Vector3& mostLeftNearPosition, const Ogre::Vector3& mostRightFarPosition);

		void loadHlmsDiskCache(void);

		void saveHlmsDiskCache(void);

		void preLoadTexturesImpl(const TexturePreloadConfig& config);

		void preLoadTextures(const TexturePreloadConfig& config);

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
		std::thread::id renderThreadId;
		
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
		Ogre::String currentScenePath;
		Ogre::String currentProjectPath;
		Ogre::String projectName;
		Ogre::String sceneName;
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
