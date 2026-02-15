#ifndef DOT_SCENE_IMPORT_MODULE_H
#define DOT_SCENE_IMPORT_MODULE_H

#include "modules/OgreRecastModule.h"
#include "modules/GraphicsModule.h"
#include "utilities/rapidxml.hpp"
#include "main/Events.h"

namespace NOWA
{
	/**
	* @class DotSceneImportModule
	* @brief This class is responsible for loading an external virtual environment
	*/
	class EXPORTED DotSceneImportModule
	{
	public:
		friend class MiniMapModule;

		/**
		* @class IsceneLoaderCallback
		* @brief This interface can be implemented to react each time a scenenode or entity will be loaded
		*/
		class EXPORTED IsceneLoaderCallback
		{
		public:
			virtual ~IsceneLoaderCallback()
			{
			}

			/**
			* @brief		Called when a scene node gets loaded
			* @param[in]	node		The scene node to react on
			*/
			virtual void onPostLoadSceneNode(Ogre::SceneNode* node)
			{
			}

			// Param: Ogre::MovableObject*
			/**
			* @brief		Called when an entity gets loaded
			* @param[in]	movableObject		The movable object to react on
			* @note			An entity or sub entity can be manipulated. In order to manipulate a sub movable object query the movable object over a name etc. and get the appropriate sub movable object index
			*/
			virtual void onPostLoadMovableObject(Ogre::MovableObject* movableObject)
			{
			}

			// Param: greNewt::Body
			//        OgreNewt (Physics settings) to manipulate
			// virtual void onPostLoadOgreNewtBody(PhysicsObject* pPhysicsObject) { }
		};
	public:
		/**
		 * @brief		Initializes thedot scene import module with minimal data
		 * @Note		Only for usage if just snippets of XML must be loaded
		 */
		DotSceneImportModule(Ogre::SceneManager* sceneManager);

		/**
		* @brief		Initializes the dot scene import module
		* @param[in]	sceneManager	The ogre scene manager to use
		* @param[in]	mainCamera		The main camera to use.
		* @param[in]	ogreNewt		The ogre newt physics to use			Hence, when a main camera is not specified here, a new one will be created from the given virtial environment with setting provided there.
		*/
		DotSceneImportModule(Ogre::SceneManager* sceneManager, Ogre::Camera* mainCamera, OgreNewt::World* ogreNewt);

		virtual ~DotSceneImportModule();

		/**
		* @brief		Parses a scene XML to create the virtual environment for the game engine
		* @param[in]	projectName			The project name
		* @param[in]	sceneName			The scene name
		* @param[in]	resourceGroupName	The group name which leads to the scene name. The group name must be declared in a resource cfg file.
		*									FileSystem=../../media/Projects/JumpNRun1
		* @param[in]	sunLight			The outside configured sun light to use for terrain shading. If the sun light does not exist, when a terrain gets created a default configured sun light
		*									will be used.
		* @param[in]	sceneLoaderCallback	The scene loader callback that can be used to react when objects are loaded.
		* @note								A newly created scene loader callback heap pointer must be passed for the IsceneLoaderCallback. It will be deleted internally,
		*									if the object is not required anymore.
		* @param[in]	showProgress		If set to true, the loading progress will be shown, else nothing will be shown which loads the virtual environment faster.
		* @return		success				True, if scene could be parsed, else false
		*/
		bool parseScene(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName, Ogre::Light* sunLight = nullptr,
			IsceneLoaderCallback* sceneLoaderCallback = nullptr, bool showProgress = true);

		/**
		* @brief		Parses a scene XML to create the virtual environment for the game engine
		* @param[in]	projectName				The project name
		* @param[in]	sceneName				The scene name
		* @param[in]	savedGameFilePathName	The whole file path name of the saved game location.
		* @param[in]	resourceGroupName		The group name which leads to the scene name. The group name must be declared in a resource cfg file.
		* @param[in]	crypted					If a saved game snapshot shall be parsed, the user can say, whether everything is crypted and needs to be decoded.
		* @param[in]	showProgress			If set to true, the loading progress will be shown, else nothing will be shown which loads the virtual environment faster.
		* @return		success					True, if scene could be parsed, else false
		*/
		bool parseSceneSnapshot(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName, const Ogre::String& savedGameFilePathName, bool crypted = false, bool showProgress = false);

		std::vector<unsigned long> parseGroup(const Ogre::String& fileName, const Ogre::String& resourceGroupName);

		/**
		* @brief		Parses a game object manually
		* @param[in]	name The game object name to parse.
		* @note			This function can called when e.g. a game object should be loaded again, or delayed or whatever.
		*/
		bool parseGameObject(const Ogre::String& name);

		/**
		* @brief		Parses all game objects that are controlled by a certain client
		* @param[in]	controlledByClientID The id of the client.
		* @note			This function can called when e.g. a game objects should be loaded again, or delayed or whatever.
		*/
		bool parseGameObjects(unsigned int controlledByClientID);

		/**
		* @brief		Parses the bounds of the scene
		* @return		mostLeftNearVector	The most left near position (3x Ogre::Math::POS_INFINITY if so far never calculated)
		*				mostRightFarVector	The most right far position (3x Ogre::Math::NEG_INFINITY if so far never calculated)
		*/
		std::pair<Ogre::Vector3, Ogre::Vector3> parseBounds(void);

		/**
		 * @brief		Parses the exit directions for this scene and the target scene to reach for that exit direction
		 * @note		Each scene can have seveal exit directions.
		 * @return		a list of vector2 for the exit direction and the target scene name to reach
		 */
		std::vector<std::pair<Ogre::Vector2, Ogre::String>> parseExitDirectionsNextScenes(void);

		std::pair<bool, Ogre::Vector3> parseGameObjectPosition(unsigned long id);

		void processNodes(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent = nullptr, bool justSetValues = false);

		void processNode(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent = nullptr, bool justSetValues = false);

		void setMissingGameObjectIds(const std::vector<unsigned long>& missingGameObjectIds);

		Ogre::SceneManager* getSceneManager(void) const;

		Ogre::Camera* getMainCamera(void) const;

		/**
		 * @brief		Gets the main sun light (if specified in scene editor)
		 * @return		sunLight	The sun light to get
		 * @Note		Attention: If no sun light has been specified, nullptr will be returned
		 */
		Ogre::Light* getSunLight(void) const;

		/**
		 * @brief		Gets whole project parameter (lights, scene, ogre newt, recast etc.)
		 * @return		ProjectParameter	The project parameter to get
		 */
		const ProjectParameter& getProjectParameter(void) const;

		/**
		 * @brief		Sets whether dot scene import module will parse things as snapshot (nodes will not be re-created but reused etc.)
		 * @param[in]	bIsSnapshot	The flag to set.
		 * @note		Do not forget to reset the value after the parse operation.
		 */
		void setIsSnapshot(bool bIsSnapshot);

		/**
		 * @brief		Gets the parsed project and scene name. May be empty.
		 * @note		Useful if a snapshot has been created, in order to identify to which scene the snapshot belongs.
		 * @return		project, sceneName	The project and scene name to get
		 */
		std::pair<Ogre::String, Ogre::String> getProjectAndSceneName(const Ogre::String& filePathName, bool decrypt);
	protected:
		/**
		* @brief		Parses a scene XML to create the virtual environment for the game engine
		* @param[in]	sceneManager		The ogre scene manager to use
		* @param[in]	projectName			The project name
		* @param[in]	sceneName			The scene name
		* @param[in]	resourceGroupName	The group name which leads to the scene name. The group name must be declared in a resource cfg file.
		* @note			Only for MiniMapModule, because it knows, what it does
		*/
		DotSceneImportModule(Ogre::SceneManager* sceneManager, const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName);

		void postInitData(void);

		bool internalParseScene(const Ogre::String& filePathName, bool crypted = false);
		
		/**
		 * @brief		Parses a global scene to create the virtual environment for the game engine.
		 * @param[in]	crypted		Whether the scene is crypted and must be decoded first.
		 */
		bool parseGlobalScene(bool crypted = false);
	
		void processScene(rapidxml::xml_node<>* xmlRoot, bool justSetValues = false);
		void processResourceLocations(rapidxml::xml_node<>* xmlNode);
		void processEnvironment(rapidxml::xml_node<>* xmlNode);
		void processOgreNewt(rapidxml::xml_node<>* xmlNode);
		void processOgreRecast(rapidxml::xml_node<>* xmlNode);

		void findGameObjectId(rapidxml::xml_node<>*& propertyElement, unsigned long& missingGameObjectId);
		void processEntity(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues = false);
		void processItem(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues = false);
		void processTerra(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues = false);
		void processOcean(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues = false);
		void processPlane(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues = false);

		Ogre::MeshPtr loadMeshV2Optimized(const Ogre::String& meshName, const Ogre::String& itemName, const Ogre::String& originalMeshFile);
	private:
		void parseGameObjectDelegate(EventDataPtr eventData);
	protected:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* mainCamera;
		Ogre::String resourceGroupName;
		IsceneLoaderCallback* sceneLoaderCallback;

		//Physics
		OgreNewt::World* ogreNewt;

		Ogre::String scenePath;
		Ogre::String savedGameFilePathName;
		std::list<Ogre::Vector2> pages;
		int	pagesCount;
		bool needCollisionRebuild;
		Ogre::Light* sunLight;
		bool showProgress;
		bool forceCreation;
		bool bSceneParsed;
		std::vector<unsigned long> parsedGameObjectIds;
		
		ProjectParameter projectParameter;
		
		Ogre::Vector3 mostLeftNearPosition;
		Ogre::Vector3 mostRightFarPosition;

		std::vector<unsigned long> missingGameObjectIds;
		bool bIsSnapshot;
		bool bNewScene;
	};

}; // namespace end

#endif

