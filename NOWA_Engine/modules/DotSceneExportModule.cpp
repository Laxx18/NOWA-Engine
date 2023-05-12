#include "NOWAPrecompiled.h"
#include "DotSceneExportModule.h"
#include "gameobject/GameObjectController.h"
#include "OgreNewtModule.h"
#include "OgreRecastModule.h"
#include "LuaScriptModule.h"
#include "utilities/XMLConverter.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "Terra.h"
#include "OgreForwardPlusBase.h"
#include "OgreForward3D.h"
#include "OgreForwardClustered.h"
#include "res/resource.h"
#include "gameobject/LuaScriptComponent.h"

namespace
{
	Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
	{
		size_t startPos = 0;
		while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
		{
			str.replace(startPos, from.length(), to);
			startPos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DotSceneExportModule::DotSceneExportModule()
		: sceneManager(nullptr),
		ogreNewt(nullptr),
		mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY))
	{

	}

	DotSceneExportModule::DotSceneExportModule(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt, const ProjectParameter& projectParameter)
		: sceneManager(sceneManager),
		ogreNewt(ogreNewt),
		projectParameter(projectParameter),
		mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY))
	{

	}

	DotSceneExportModule::DotSceneExportModule(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt)
		: sceneManager(sceneManager),
		ogreNewt(ogreNewt),
		mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY))
	{

	}

	DotSceneExportModule::~DotSceneExportModule()
	{

	}

	void DotSceneExportModule::exportScene(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& worldResourceGroupName, bool crypted)
	{
		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(worldResourceGroupName)[0];
		// Announce the current world path to core
		Core::getSingletonPtr()->setCurrentWorldPath(projectPath + "/" + projectName + "/" + sceneName + ".scene");

		// Project is always: "projects/projectName/sceneName.scene"
		Ogre::String filePath = projectPath + "/" + projectName;
		Ogre::String filePathName = filePath + "/" + sceneName + ".scene";

		// Maybe create a folder
		Core::getSingletonPtr()->createFolders(filePathName);

		if (filePathName.empty())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneExportModule] Error: Could not export scene, because there is no such group name resource: " + worldResourceGroupName);
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneExportModule] Error: Could not export scene, because there is no such group name resource: " + worldResourceGroupName + "\n", "NOWA");
		}
		
		xml_document<> doc;
		xml_node<>* decl = doc.allocate_node(node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(decl);

		// Scene element
		{
			xml_node<>* sceneXML = doc.allocate_node(node_element, "scene");
			sceneXML->append_attribute(doc.allocate_attribute("formatVersion", NOWA_DOT_SCENE_FILEVERSION_STR));
			sceneXML->append_attribute(doc.allocate_attribute("generator", "NOWA-Engine"));
			sceneXML->append_attribute(doc.allocate_attribute("sceneName", XMLConverter::ConvertString(doc, sceneName)));

			// ResourceLocations element
			// Is no more required
#if 1
			xml_node<>* resourceLocationsXML = doc.allocate_node(node_element, "resourceLocations");
			this->exportResourceLocations(resourceLocationsXML, doc);
			sceneXML->append_node(resourceLocationsXML);
#endif

			// Environment element
			xml_node<>* environmentXML = doc.allocate_node(node_element, "environment");
			this->exportEnvironment(environmentXML, doc);
			sceneXML->append_node(environmentXML);

			doc.append_node(sceneXML);

			//// MainCamera element
			//{
			//	xml_node<>* cameraXML = doc.allocate_node(node_element, "camera");
			//	this->exportMainCamera(cameraXML, doc);
			//	sceneXML->append_node(cameraXML);
			//}

			// OgreNewt element
			{
				xml_node<>* ogreNewtXML = doc.allocate_node(node_element, "OgreNewt");
				this->exportOgreNewt(ogreNewtXML, doc);
				sceneXML->append_node(ogreNewtXML);
			}

			// OgreRecast element
			{
				if (nullptr != AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
				{
					xml_node<>* ogreRecastXML = doc.allocate_node(node_element, "OgreRecast");
					this->exportOgreRecast(ogreRecastXML, doc);
					sceneXML->append_node(ogreRecastXML);
				}
			}

			//// Light element
			//{
			//	xml_node<>* lightXML = doc.allocate_node(node_element, "light");
			//	this->exportMainLight(lightXML, doc);
			//	sceneXML->append_node(lightXML);
			//}

			// Nodes element
			{
				xml_node<>* nodesXML = doc.allocate_node(node_element, "nodes");
				// Export local game objects (false)
				this->exportSceneNodes(nodesXML, doc, false, filePath);
				sceneXML->append_node(nodesXML);
			}
			
			// Export all global game objects in projectName/global.scene separately
			this->exportGlobalScene(worldResourceGroupName, projectName);

			// Bounds element (after bounds have been calculated for each object
			{
				xml_node<>* boundsXML = doc.allocate_node(node_element, "bounds");
				boundsXML->append_attribute(doc.allocate_attribute("mostLeftNearPosition", XMLConverter::ConvertString(doc, this->mostLeftNearPosition)));
				boundsXML->append_attribute(doc.allocate_attribute("mostRightFarPosition", XMLConverter::ConvertString(doc, this->mostRightFarPosition)));
				environmentXML->append_node(boundsXML);
			}

			// Send event and set bounds for current world
			boost::shared_ptr<EventDataBoundsUpdated> eventDataBoundsUpdated(boost::make_shared<EventDataBoundsUpdated>(this->mostLeftNearPosition, this->mostRightFarPosition));
			AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataBoundsUpdated);
			Core::getSingletonPtr()->setCurrentWorldBounds(this->mostLeftNearPosition, this->mostRightFarPosition);
		}

		std::stringstream stream;
		stream << doc;

		if (true == crypted)
		{
			stream << Core::getSingletonPtr()->encode64(stream.str(), true);
		}

		std::ofstream file;
		file.open(filePathName);
		file << stream.str();

		file.close();
	}

	void DotSceneExportModule::saveSceneSnapshot(const Ogre::String& filePathName, bool crypted)
	{
		Ogre::String tempFilePathName = Core::getSingletonPtr()->removePartsFromString(filePathName, ".sav").second;
		// Maybe create a folder
		Core::getSingletonPtr()->createFolders(tempFilePathName);

		if (tempFilePathName.empty())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneExportModule] Error: Could not save scene snapshot, because the file path name is empty.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneExportModule] Error: Could not save scene snapshot, because the file path name is empty.\n", "NOWA");
		}

		xml_document<> doc;
		xml_node<>* decl = doc.allocate_node(node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(decl);

		// Scene element
		{
			xml_node<>* sceneXML = doc.allocate_node(node_element, "scene");
			sceneXML->append_attribute(doc.allocate_attribute("formatVersion", NOWA_DOT_SCENE_FILEVERSION_STR));
			sceneXML->append_attribute(doc.allocate_attribute("generator", "NOWA-Engine"));
			sceneXML->append_attribute(doc.allocate_attribute("projectName", XMLConverter::ConvertString(doc, Core::getSingletonPtr()->getProjectName())));
			sceneXML->append_attribute(doc.allocate_attribute("sceneName", XMLConverter::ConvertString(doc, Core::getSingletonPtr()->getWorldName())));

			// ResourceLocations element
			// Is no more required
#if 1
			xml_node<>* resourceLocationsXML = doc.allocate_node(node_element, "resourceLocations");
			this->exportResourceLocations(resourceLocationsXML, doc);
			sceneXML->append_node(resourceLocationsXML);
#endif

			// Environment element
			xml_node<>* environmentXML = doc.allocate_node(node_element, "environment");
			this->exportEnvironment(environmentXML, doc);
			sceneXML->append_node(environmentXML);

			doc.append_node(sceneXML);

			//// MainCamera element
			//{
			//	xml_node<>* cameraXML = doc.allocate_node(node_element, "camera");
			//	this->exportMainCamera(cameraXML, doc);
			//	sceneXML->append_node(cameraXML);
			//}

			// OgreNewt element
			{
				xml_node<>* ogreNewtXML = doc.allocate_node(node_element, "OgreNewt");
				this->exportOgreNewt(ogreNewtXML, doc);
				sceneXML->append_node(ogreNewtXML);
			}

			// OgreRecast element
			{
				if (nullptr != AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
				{
					xml_node<>* ogreRecastXML = doc.allocate_node(node_element, "OgreRecast");
					this->exportOgreRecast(ogreRecastXML, doc);
					sceneXML->append_node(ogreRecastXML);
				}
			}

			//// Light element
			//{
			//	xml_node<>* lightXML = doc.allocate_node(node_element, "light");
			//	this->exportMainLight(lightXML, doc);
			//	sceneXML->append_node(lightXML);
			//}

			// Nodes element
			{
				xml_node<>* nodesXML = doc.allocate_node(node_element, "nodes");
				// Export local game objects (true)! Also global!
				this->exportSceneNodes(nodesXML, doc, true, tempFilePathName);
				sceneXML->append_node(nodesXML);
			}

			// Bounds element (after bounds have been calculated for each object
			{
				xml_node<>* boundsXML = doc.allocate_node(node_element, "bounds");
				boundsXML->append_attribute(doc.allocate_attribute("mostLeftNearPosition", XMLConverter::ConvertString(doc, this->mostLeftNearPosition)));
				boundsXML->append_attribute(doc.allocate_attribute("mostRightFarPosition", XMLConverter::ConvertString(doc, this->mostRightFarPosition)));
				environmentXML->append_node(boundsXML);
			}
		}

		std::stringstream stream;
		stream << doc;

		if (true == crypted)
		{
			stream << Core::getSingletonPtr()->encode64(stream.str(), true);
		}

		std::ofstream file;
		file.open(tempFilePathName);
		file << stream.str();

		file.close();
	}

	void DotSceneExportModule::copyScene(const Ogre::String& oldSeneName, const Ogre::String& newProjectName, const Ogre::String& newSceneName, const Ogre::String& worldResourceGroupName)
	{
		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(worldResourceGroupName)[0];

		// Project is always: "projects/projectName/sceneName.scene"
		Ogre::String filePath = projectPath + "/" + newProjectName;
		Ogre::String filePathName = filePath + "/" + newSceneName + ".scene";

		// Maybe create a folder
		Core::getSingletonPtr()->createFolders(filePathName);

		if (true == filePathName.empty())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneExportModule] Error: There is no such group name resource: " + worldResourceGroupName);
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[WorldLoader] Error: There is no such group name resource: " + worldResourceGroupName + "\n", "NOWA");
		}

		for (auto it = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cbegin(); it != AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cend(); ++it)
		{
			GameObjectPtr gameObjectPtr = it->second;
			if (nullptr != gameObjectPtr)
			{
				GameObjectComponents* components = gameObjectPtr->getComponents();
				for (auto& it = components->begin(); it != components->end(); ++it)
				{
					auto luaScriptCompPtr = boost::dynamic_pointer_cast<LuaScriptComponent>(std::get<COMPONENT>(*it));
					if (nullptr != luaScriptCompPtr)
					{
						if (false == gameObjectPtr->getGlobal())
						{
							Ogre::String scriptFilePathName = filePath + "/" + luaScriptCompPtr->getScriptFile();
							size_t found = luaScriptCompPtr->getScriptFile().find(oldSeneName);
							if (Ogre::String::npos != found)
							{
								Ogre::String newName = replaceAll(luaScriptCompPtr->getScriptFile(), oldSeneName, newSceneName);
								luaScriptCompPtr->setScriptFile(newName, LuaScriptComponent::WRITE_XML);
								AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScript(luaScriptCompPtr->getScriptFile(), newName, false);
							}
						}
					}
				}
			}
		}
		
		xml_document<> doc;
		xml_node<>* decl = doc.allocate_node(node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(decl);

		// Scene element
		{
			xml_node<>* sceneXML = doc.allocate_node(node_element, "scene");
			sceneXML->append_attribute(doc.allocate_attribute("formatVersion", NOWA_DOT_SCENE_FILEVERSION_STR));
			sceneXML->append_attribute(doc.allocate_attribute("generator", "NOWA-Engine"));

			// ResourceLocations element
			// Is no more required
#if 1
			xml_node<>* resourceLocationsXML = doc.allocate_node(node_element, "resourceLocations");
			this->exportResourceLocations(resourceLocationsXML, doc);
			sceneXML->append_node(resourceLocationsXML);
#endif

			// Environment element
			xml_node<>* environmentXML = doc.allocate_node(node_element, "environment");
			this->exportEnvironment(environmentXML, doc);
			sceneXML->append_node(environmentXML);

			doc.append_node(sceneXML);

			//// MainCamera element
			//{
			//	xml_node<>* cameraXML = doc.allocate_node(node_element, "camera");
			//	this->exportMainCamera(cameraXML, doc);
			//	sceneXML->append_node(cameraXML);
			//}

			// OgreNewt element
			{
				xml_node<>* ogreNewtXML = doc.allocate_node(node_element, "OgreNewt");
				this->exportOgreNewt(ogreNewtXML, doc);
				sceneXML->append_node(ogreNewtXML);
			}

			// OgreRecast element
			{
				if (nullptr != AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
				{
					xml_node<>* ogreRecastXML = doc.allocate_node(node_element, "OgreRecast");
					this->exportOgreRecast(ogreRecastXML, doc);
					sceneXML->append_node(ogreRecastXML);
				}
			}

			//// Light element
			//{
			//	xml_node<>* lightXML = doc.allocate_node(node_element, "light");
			//	this->exportMainLight(lightXML, doc);
			//	sceneXML->append_node(lightXML);
			//}

			// Nodes element
			{
				xml_node<>* nodesXML = doc.allocate_node(node_element, "nodes");
				// Export local game objects (false)
				this->exportSceneNodes(nodesXML, doc, false, filePath);
				sceneXML->append_node(nodesXML);
			}

			// Bounds element (after bounds have been calculated for each object
			{
				xml_node<>* boundsXML = doc.allocate_node(node_element, "bounds");
				boundsXML->append_attribute(doc.allocate_attribute("mostLeftNearPosition", XMLConverter::ConvertString(doc, this->mostLeftNearPosition)));
				boundsXML->append_attribute(doc.allocate_attribute("mostRightFarPosition", XMLConverter::ConvertString(doc, this->mostRightFarPosition)));
				environmentXML->append_node(boundsXML);
			}

			// Send event and set bounds for current world
			boost::shared_ptr<EventDataBoundsUpdated> eventDataBoundsUpdated(boost::make_shared<EventDataBoundsUpdated>(this->mostLeftNearPosition, this->mostRightFarPosition));
			AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataBoundsUpdated);
			Core::getSingletonPtr()->setCurrentWorldBounds(this->mostLeftNearPosition, this->mostRightFarPosition);
		}

		std::ofstream file;
		file.open(filePathName);
		file << doc;
		file.close();
	}

	void DotSceneExportModule::exportGlobalScene(const Ogre::String& worldResourceGroupName, const Ogre::String& projectName, bool crypted)
	{
		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(worldResourceGroupName)[0];
		// Project is always: "projects/projectName/global.scene"
		Ogre::String filePath = projectPath + "/" + projectName;
		Ogre::String filePathName = filePath + "/global.scene";
		// Maybe create a folder
		Core::getSingletonPtr()->createFolders(filePathName);// Maybe create a folder

		if (projectPath.empty())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneExportModule] Error: The project path is empty!");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[WorldLoader] Error: The project path is empty!\n", "NOWA");
		}

		xml_document<> doc;
		xml_node<>* decl = doc.allocate_node(node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(decl);

		// Scene element
		{
			xml_node<>* sceneXML = doc.allocate_node(node_element, "scene");
			sceneXML->append_attribute(doc.allocate_attribute("formatVersion", NOWA_DOT_SCENE_FILEVERSION_STR));
			sceneXML->append_attribute(doc.allocate_attribute("generator", "NOWA-Engine"));

			doc.append_node(sceneXML);

			// Nodes element
			{
				xml_node<>* nodesXML = doc.allocate_node(node_element, "nodes");
				// Export global game objects (true)
				this->exportSceneNodes(nodesXML, doc, true, filePath);
				sceneXML->append_node(nodesXML);
			}
		}

		std::stringstream stream;
		stream << doc;

		if (true == crypted)
		{
			stream << Core::getSingletonPtr()->encode64(stream.str(), true);
		}

		std::ofstream file;
		file.open(filePathName);
		file << stream.str();

		file.close();
	}

	void DotSceneExportModule::exportEnvironment(xml_node<>* environmentXML, xml_document<>& doc)
	{
		std::hash<Ogre::String> hash;
		// Use id, to get generic object property?? See ogitor
		// SceneManager
		{
			xml_node<>* sceneManagerXML = doc.allocate_node(node_element, "sceneManager");
			sceneManagerXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, this->sceneManager->getName())));
			sceneManagerXML->append_attribute(doc.allocate_attribute("id", XMLConverter::ConvertString(doc, static_cast<unsigned int>(hash(this->sceneManager->getName())))));
			sceneManagerXML->append_attribute(doc.allocate_attribute("type", XMLConverter::ConvertString(doc, this->sceneManager->getTypeName())));
			environmentXML->append_node(sceneManagerXML);
		}

		// ambient
		{
			xml_node<>* colorAmbientXML = doc.allocate_node(node_element, "ambient");

			// AmbientLightUpperHemisphere
			{
				xml_node<>* colorUpperHemisphereXML = doc.allocate_node(node_element, "ambientLightUpperHemisphere");

				colorUpperHemisphereXML->append_attribute(doc.allocate_attribute("r", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightUpperHemisphere().r)));
				colorUpperHemisphereXML->append_attribute(doc.allocate_attribute("g", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightUpperHemisphere().g)));
				colorUpperHemisphereXML->append_attribute(doc.allocate_attribute("b", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightUpperHemisphere().b)));
				// colourUpperHemisphereXML->append_attribute(doc.allocate_attribute("a", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightUpperHemisphere().a)));
				// Note a is used for envmapscale in ogre
				colorAmbientXML->append_node(colorUpperHemisphereXML);
			}
			// AmbientLightLowerHemisphere
			{
				xml_node<>* colorLowerHemisphereXML = doc.allocate_node(node_element, "ambientLightLowerHemisphere");

				colorLowerHemisphereXML->append_attribute(doc.allocate_attribute("r", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightLowerHemisphere().r)));
				colorLowerHemisphereXML->append_attribute(doc.allocate_attribute("g", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightLowerHemisphere().g)));
				colorLowerHemisphereXML->append_attribute(doc.allocate_attribute("b", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightLowerHemisphere().b)));
				colorLowerHemisphereXML->append_attribute(doc.allocate_attribute("a", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightLowerHemisphere().a)));
				colorAmbientXML->append_node(colorLowerHemisphereXML);
			}
			// HemisphereDir
			{
				xml_node<>* hemisphereDirXML = doc.allocate_node(node_element, "hemisphereDir");

				hemisphereDirXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightHemisphereDir().x)));
				hemisphereDirXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightHemisphereDir().y)));
				hemisphereDirXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightHemisphereDir().z)));
				colorAmbientXML->append_node(hemisphereDirXML);
			}
			// EnvmapScale
			{
				xml_node<>* envmapScaleXML = doc.allocate_node(node_element, "envmapScale");
				// Note a is used for envmapscale in ogre
				envmapScaleXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->sceneManager->getAmbientLightUpperHemisphere().a)));
				colorAmbientXML->append_node(envmapScaleXML);
			}

			environmentXML->append_node(colorAmbientXML);
		}

		// shadows
		{
			xml_node<>* shadows = doc.allocate_node(node_element, "shadows");

			// shadowFarDistance
			{
				xml_node<>* shadowFarDistanceXML = doc.allocate_node(node_element, "shadowFarDistance");

				shadowFarDistanceXML->append_attribute(doc.allocate_attribute("distance", XMLConverter::ConvertString(doc, this->sceneManager->getShadowFarDistance())));
				shadows->append_node(shadowFarDistanceXML);
			}
			// shadowDirectionalLightExtrusionDistance
			{
				xml_node<>* shadowDirectionalLightExtrusionDistanceXML = doc.allocate_node(node_element, "shadowDirectionalLightExtrusionDistance");

				shadowDirectionalLightExtrusionDistanceXML->append_attribute(doc.allocate_attribute("distance", XMLConverter::ConvertString(doc, this->sceneManager->getShadowDirectionalLightExtrusionDistance())));
				shadows->append_node(shadowDirectionalLightExtrusionDistanceXML);
			}
			// shadowDirLightTextureOffset
			{
				xml_node<>* shadowDirLightTextureOffsetXML = doc.allocate_node(node_element, "shadowDirLightTextureOffset");

				shadowDirLightTextureOffsetXML->append_attribute(doc.allocate_attribute("offset", XMLConverter::ConvertString(doc, this->sceneManager->getShadowDirLightTextureOffset())));
				shadows->append_node(shadowDirLightTextureOffsetXML);
			}
			// shadowColor
			{
				xml_node<>* shadowColorXML = doc.allocate_node(node_element, "shadowColor");
				shadowColorXML->append_attribute(doc.allocate_attribute("r", XMLConverter::ConvertString(doc, this->sceneManager->getShadowColour().r)));
				shadowColorXML->append_attribute(doc.allocate_attribute("g", XMLConverter::ConvertString(doc, this->sceneManager->getShadowColour().g)));
				shadowColorXML->append_attribute(doc.allocate_attribute("b", XMLConverter::ConvertString(doc, this->sceneManager->getShadowColour().b)));
				// Note a is used for envmapscale in ogre
				shadows->append_node(shadowColorXML);
			}
			// shadowQuality
			{
				xml_node<>* shadowQualityXML = doc.allocate_node(node_element, "shadowQuality");
				shadowQualityXML->append_attribute(doc.allocate_attribute("index", XMLConverter::ConvertString(doc, static_cast<unsigned char>(WorkspaceModule::getInstance()->getShadowQuality()))));
				// Note a is used for envmapscale in ogre
				shadows->append_node(shadowQualityXML);
			}
			// ambientLightMode
			{
				xml_node<>* ambientLightModeXML = doc.allocate_node(node_element, "ambientLightMode");
				ambientLightModeXML->append_attribute(doc.allocate_attribute("index", XMLConverter::ConvertString(doc, static_cast<unsigned char>(WorkspaceModule::getInstance()->getAmbientLightMode()))));
				// Note a is used for envmapscale in ogre
				shadows->append_node(ambientLightModeXML);
			}

			environmentXML->append_node(shadows);
		}
		
		// Light Forward
		{
			Ogre::ForwardPlusBase* forwardPlusBase = this->sceneManager->getForwardPlus();
			if (nullptr != forwardPlusBase)
			{
				unsigned short forwardMode = 0;
				if (Ogre::ForwardPlusBase::MethodForward3D == this->sceneManager->getForwardPlus()->getForwardPlusMethod())
				{
					forwardMode = 1;
				}
				else if (Ogre::ForwardPlusBase::MethodForwardClustered == this->sceneManager->getForwardPlus()->getForwardPlusMethod())
				{
					forwardMode = 2;
				}


				xml_node<>* lightFoward = doc.allocate_node(node_element, "lightFoward");

				// forwardMode
				{
					xml_node<>* nodeXML = doc.allocate_node(node_element, "forwardMode");
					nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, forwardMode)));
					lightFoward->append_node(nodeXML);
				}

				if (0 != forwardMode)
				{
					// lightWidth
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "lightWidth");

						if (1 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::Forward3D*>(forwardPlusBase)->getWidth())));
						else
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getWidth())));
						lightFoward->append_node(nodeXML);
					}
					// lightHeight
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "lightHeight");

						if (1 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::Forward3D*>(forwardPlusBase)->getHeight())));
						else
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getHeight())));
						lightFoward->append_node(nodeXML);
					}
					// numLightSlices
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "numLightSlices");

						if (1 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, static_cast<Ogre::Forward3D*>(forwardPlusBase)->getNumSlices())));
						else
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, static_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getNumSlices())));
						lightFoward->append_node(nodeXML);
					}
					// lightsPerCell
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "lightsPerCell");

						if (1 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::Forward3D*>(forwardPlusBase)->getLightsPerCell())));
						else
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getLightsPerCell())));
						lightFoward->append_node(nodeXML);
					}
					// decalsPerCell
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "decalsPerCell");

						if (2 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getDecalsPerCell())));
						lightFoward->append_node(nodeXML);
					}
					// cubemapProbesPerCell
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "cubemapProbesPerCell");

						// There is no proberty for cubemapProbesPerCell in Ogre yet
						if (2 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, 10)));
						lightFoward->append_node(nodeXML);
					}
					// minLightDistance
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "minLightDistance");

						if (1 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::Forward3D*>(forwardPlusBase)->getMinDistance())));
						else
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getMinDistance())));
						lightFoward->append_node(nodeXML);
					}
					// maxLightDistance
					{
						xml_node<>* nodeXML = doc.allocate_node(node_element, "maxLightDistance");

						if (1 == forwardMode)
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::Forward3D*>(forwardPlusBase)->getMaxDistance())));
						else
							nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, dynamic_cast<Ogre::ForwardClustered*>(forwardPlusBase)->getMaxDistance())));
						lightFoward->append_node(nodeXML);
					}
					environmentXML->append_node(lightFoward);
				}
			}

			// Main Parameter
			{
				xml_node<>* mainParameterXML = doc.allocate_node(node_element, "mainParameter");

				// ignoreGlobalScene
				{
					xml_node<>* nodeXML = doc.allocate_node(node_element, "ignoreGlobalScene");
					nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, this->projectParameter.ignoreGlobalScene)));
					mainParameterXML->append_node(nodeXML);
				}

				// renderDistance
				{
					xml_node<>* nodeXML = doc.allocate_node(node_element, "renderDistance");

					nodeXML->append_attribute(doc.allocate_attribute("distance", XMLConverter::ConvertString(doc, NOWA::Core::getSingletonPtr()->getGlobalRenderDistance())));
					mainParameterXML->append_node(nodeXML);
				}

				// useV2Item
				{
					xml_node<>* nodeXML = doc.allocate_node(node_element, "useV2Item");
					nodeXML->append_attribute(doc.allocate_attribute("value", XMLConverter::ConvertString(doc, this->projectParameter.useV2Item)));
					mainParameterXML->append_node(nodeXML);
				}

				environmentXML->append_node(mainParameterXML);
			}
		}

		// SkyBox
		// TODO: When setting sky box in scene, all skybox, dome, plane parameters must be stored separatly, because its not possible to get all the parameters via scene manager
#if 0
		{
			xml_node<>* skyBoxXML = doc.allocate_node(node_element, "skyBox");
			Ogre::SceneNode* skyBoxNode = this->sceneManager->getSkyBoxNode();
			bool active = false;
			Ogre::String materialName;
			if (nullptr != skyBoxNode)
			{
				auto object = skyBoxNode->getAttachedObject(0);
				if (nullptr != object)
				{
					active = object->isVisible();
				}

			}
			Ogre::Real distance = this->sceneManager->getSkyBoxGenParameters().skyBoxDistance;
			skyBoxXML->append_attribute(doc.allocate_attribute("active", XMLConverter::ConvertString(doc, active)));
			skyBoxXML->append_attribute(doc.allocate_attribute("distance", XMLConverter::ConvertString(doc, distance)));
			skyBoxXML->append_attribute(doc.allocate_attribute("material", materialName));
			environmentXML->append_node(skyBoxXML);
		}
#endif	

		// Fog
		{
			Ogre::String fogModes[] = { "none", "exp", "exp2", "linear" };
			xml_node<>* fogXML = doc.allocate_node(node_element, "fog");
			fogXML->append_attribute(doc.allocate_attribute("mode", XMLConverter::ConvertString(doc, fogModes[this->sceneManager->getFogMode()])));
			fogXML->append_attribute(doc.allocate_attribute("start", XMLConverter::ConvertString(doc, this->sceneManager->getFogStart())));
			fogXML->append_attribute(doc.allocate_attribute("end", XMLConverter::ConvertString(doc, this->sceneManager->getFogEnd())));
			fogXML->append_attribute(doc.allocate_attribute("density", XMLConverter::ConvertString(doc, this->sceneManager->getFogDensity())));
			environmentXML->append_node(fogXML);
		}

		// Here no editor camera so far, InitViewCamera
#if 0
		// Main Camera
		{
	
			xml_node<>* cameraXML = doc.allocate_node(node_element, "camera");
			cameraXML->append_attribute(doc.allocate_attribute("name", this->mainCamera->getName()));
			cameraXML->append_attribute(doc.allocate_attribute("viewMode", "0")); // is that just for editor?
			cameraXML->append_attribute(doc.allocate_attribute("polyMode", XMLConverter::ConvertString(doc, this->mainCamera->getPolygonMode())));
			cameraXML->append_attribute(doc.allocate_attribute("fov", XMLConverter::ConvertString(doc, this->mainCamera->getFOVy())));
			environmentXML->append_node(cameraXML);

			// Clipping
			{
				xml_node<>* clippingXML = doc.allocate_node(node_element, "clipping");
				clippingXML->append_attribute(doc.allocate_attribute("near", XMLConverter::ConvertString(doc, this->mainCamera->getNearClipDistance())));
				clippingXML->append_attribute(doc.allocate_attribute("far", XMLConverter::ConvertString(doc, this->mainCamera->getFarClipDistance())));
				cameraXML->append_node(clippingXML);
			}

			// Position
			{
				xml_node<>* positionXML = doc.allocate_node(node_element, "position");
				positionXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedPosition().x)));
				positionXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedPosition().y)));
				positionXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedPosition().z)));
				cameraXML->append_node(positionXML);
			}

			// Rotation
			{
				xml_node<>* rotationXML = doc.allocate_node(node_element, "rotation");
				rotationXML->append_attribute(doc.allocate_attribute("qw", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().w)));
				rotationXML->append_attribute(doc.allocate_attribute("qx", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().x)));
				rotationXML->append_attribute(doc.allocate_attribute("qy", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().y)));
				rotationXML->append_attribute(doc.allocate_attribute("qz", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().z)));
				cameraXML->append_node(rotationXML);
			}
		}
#endif
		// UserData here
		// -> Recast config
		// -> OgreNewt world size
		
	}

	void DotSceneExportModule::exportResourceLocations(rapidxml::xml_node<>* resourcesXML, rapidxml::xml_document<>& doc)
	{
#if 0
		auto& resourceLocations = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
		for (auto& resourceGroupName : resourceLocations)
		{
			// A resource group name can have several pathes
			for (auto& path : Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(resourceGroupName))
			{
				xml_node<>* resourceLocationXML = doc.allocate_node(node_element, "resourceLocation");
				
				resourceLocationXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, resourceGroupName)));
				resourceLocationXML->append_attribute(doc.allocate_attribute("type", XMLConverter::ConvertString(doc, path->archive->getType())));
				resourceLocationXML->append_attribute(doc.allocate_attribute("path", XMLConverter::ConvertString(doc, path->archive->getName())));
				
				resourcesXML->append_node(resourceLocationXML);
			}
		}
#endif

		for (std::vector<Ogre::String>::const_iterator it = this->additionalMeshResources.cbegin(); it != this->additionalMeshResources.cend(); ++it)
		{
			for (auto& path : Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(*it))
			{
				xml_node<>* resourceLocationXML = doc.allocate_node(node_element, "resourceLocation");

				resourceLocationXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, *it)));
				resourceLocationXML->append_attribute(doc.allocate_attribute("type", XMLConverter::ConvertString(doc, path->archive->getType())));
				resourceLocationXML->append_attribute(doc.allocate_attribute("path", XMLConverter::ConvertString(doc, path->archive->getName())));

				resourcesXML->append_node(resourceLocationXML);
			}
		}
	}

	//void DotSceneExportModule::exportMainLight(xml_node<>* lightXML, xml_document<>& doc)
	//{
	//	if (nullptr != this->mainLight)
	//	{
	//		std::hash<Ogre::String> hash;
	//		// MainLight
	//		{
	//			Ogre::String lightType[] = { "directional", "point", "spot", "vpl" };
	//			unsigned int id = hash(this->mainLight->getName());

	//			lightXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, this->mainLight->getName())));
	//			lightXML->append_attribute(doc.allocate_attribute("id", XMLConverter::ConvertString(doc, id)));
	//			lightXML->append_attribute(doc.allocate_attribute("type", XMLConverter::ConvertString(doc, lightType[this->mainLight->getType()])));
	//			lightXML->append_attribute(doc.allocate_attribute("castShadows", XMLConverter::ConvertString(doc, this->mainLight->getCastShadows())));

	//			// Position
	//			{
	//				xml_node<>* positionXML = doc.allocate_node(node_element, "position");
	//				positionXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->mainLight->getParentSceneNode()->_getDerivedPositionUpdated().x)));
	//				positionXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, this->mainLight->getParentSceneNode()->_getDerivedPositionUpdated().y)));
	//				positionXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, this->mainLight->getParentSceneNode()->_getDerivedPositionUpdated().z)));
	//				lightXML->append_node(positionXML);
	//			}

	//			// DirectionVector
	//			{
	//				xml_node<>* directionVectorXML = doc.allocate_node(node_element, "directionVector");
	//				directionVectorXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->mainLight->getDirection().x)));
	//				directionVectorXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, this->mainLight->getDirection().y)));
	//				directionVectorXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, this->mainLight->getDirection().z)));
	//				lightXML->append_node(directionVectorXML);
	//			}

	//			// ColourDiffuse
	//			{
	//				xml_node<>* colourDiffuseXML = doc.allocate_node(node_element, "colourDiffuse");
	//				colourDiffuseXML->append_attribute(doc.allocate_attribute("r", XMLConverter::ConvertString(doc, this->mainLight->getDiffuseColour().r)));
	//				colourDiffuseXML->append_attribute(doc.allocate_attribute("g", XMLConverter::ConvertString(doc, this->mainLight->getDiffuseColour().g)));
	//				colourDiffuseXML->append_attribute(doc.allocate_attribute("b", XMLConverter::ConvertString(doc, this->mainLight->getDiffuseColour().b)));
	//				colourDiffuseXML->append_attribute(doc.allocate_attribute("a", XMLConverter::ConvertString(doc, this->mainLight->getDiffuseColour().a)));
	//				lightXML->append_node(colourDiffuseXML);
	//			}

	//			// ColourSpecular
	//			{
	//				xml_node<>* colourSpecularXML = doc.allocate_node(node_element, "colourSpecular");
	//				colourSpecularXML->append_attribute(doc.allocate_attribute("r", XMLConverter::ConvertString(doc, this->mainLight->getSpecularColour().r)));
	//				colourSpecularXML->append_attribute(doc.allocate_attribute("g", XMLConverter::ConvertString(doc, this->mainLight->getSpecularColour().g)));
	//				colourSpecularXML->append_attribute(doc.allocate_attribute("b", XMLConverter::ConvertString(doc, this->mainLight->getSpecularColour().b)));
	//				colourSpecularXML->append_attribute(doc.allocate_attribute("a", XMLConverter::ConvertString(doc, this->mainLight->getSpecularColour().a)));
	//				lightXML->append_node(colourSpecularXML);
	//			}
	//			// here with attuentation etc.
	//		}
	//	}
	//}

	void DotSceneExportModule::exportOgreNewt(xml_node<>* xmlNode, xml_document<>& doc)
	{
		assert(this->ogreNewt != nullptr);
		if (nullptr != this->ogreNewt)
		{
			// OgreNewt
			{
				// Main parameters
				{
					Ogre::String plattformArchitectureDescription;
					xmlNode->append_attribute(doc.allocate_attribute("solverModel", XMLConverter::ConvertString(doc, this->ogreNewt->getSolverModel())));
					xmlNode->append_attribute(doc.allocate_attribute("multithreadSolverOnSingleIsland", XMLConverter::ConvertString(doc, this->ogreNewt->getMultithreadSolverOnSingleIsland())));
					xmlNode->append_attribute(doc.allocate_attribute("broadPhaseAlgorithm", XMLConverter::ConvertString(doc, this->ogreNewt->getBroadPhaseAlgorithm())));
					xmlNode->append_attribute(doc.allocate_attribute("threadCount", XMLConverter::ConvertString(doc, this->ogreNewt->getThreadCount())));
					xmlNode->append_attribute(doc.allocate_attribute("desiredFps", XMLConverter::ConvertString(doc, this->ogreNewt->getDesiredFps())));
					xmlNode->append_attribute(doc.allocate_attribute("defaultLinearDamping", XMLConverter::ConvertString(doc, this->ogreNewt->getDefaultLinearDamping())));

					xml_node<>*  dampingXML = doc.allocate_node(node_element, "defaultAngularDamping");
					dampingXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->ogreNewt->getDefaultAngularDamping().x)));
					dampingXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, this->ogreNewt->getDefaultAngularDamping().y)));
					dampingXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, this->ogreNewt->getDefaultAngularDamping().z)));
					xmlNode->append_node(dampingXML);

					xml_node<>* gravityXML = doc.allocate_node(node_element, "globalGravity");
					gravityXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, AppStateManager::getSingletonPtr()->getOgreNewtModule()->getGlobalGravity().x)));
					gravityXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, AppStateManager::getSingletonPtr()->getOgreNewtModule()->getGlobalGravity().y)));
					gravityXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, AppStateManager::getSingletonPtr()->getOgreNewtModule()->getGlobalGravity().z)));
					xmlNode->append_node(gravityXML);
				}
			}
		}
	}

	void DotSceneExportModule::exportOgreRecast(rapidxml::xml_node<>* xmlNode, rapidxml::xml_document<>& doc)
	{
		// Ogre Recast
		{
			// Main parameters
			{
				OgreRecast* recast = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast();
				if (nullptr != recast)
				{
					xmlNode->append_attribute(doc.allocate_attribute("CellSize", XMLConverter::ConvertString(doc, recast->getConfigParams().getCellSize())));
					xmlNode->append_attribute(doc.allocate_attribute("CellHeight", XMLConverter::ConvertString(doc, recast->getConfigParams().getCellHeight())));
					xmlNode->append_attribute(doc.allocate_attribute("AgentMaxSlope", XMLConverter::ConvertString(doc, recast->getConfigParams().getAgentMaxSlope())));
					xmlNode->append_attribute(doc.allocate_attribute("AgentMaxClimb", XMLConverter::ConvertString(doc, recast->getConfigParams().getAgentMaxClimb())));
					xmlNode->append_attribute(doc.allocate_attribute("AgentHeight", XMLConverter::ConvertString(doc, recast->getConfigParams().getAgentHeight())));
					xmlNode->append_attribute(doc.allocate_attribute("AgentRadius", XMLConverter::ConvertString(doc, recast->getConfigParams().getAgentRadius())));
					xmlNode->append_attribute(doc.allocate_attribute("EdgeMaxLen", XMLConverter::ConvertString(doc, recast->getConfigParams().getEdgeMaxLen())));
					xmlNode->append_attribute(doc.allocate_attribute("EdgeMaxError", XMLConverter::ConvertString(doc, recast->getConfigParams().getEdgeMaxError())));
					xmlNode->append_attribute(doc.allocate_attribute("RegionMinSize", XMLConverter::ConvertString(doc, recast->getConfigParams().getRegionMinSize())));
					xmlNode->append_attribute(doc.allocate_attribute("RegionMergeSize", XMLConverter::ConvertString(doc, recast->getConfigParams().getRegionMergeSize())));
					xmlNode->append_attribute(doc.allocate_attribute("VertsPerPoly", XMLConverter::ConvertString(doc, recast->getConfigParams().getVertsPerPoly())));
					xmlNode->append_attribute(doc.allocate_attribute("DetailSampleDist", XMLConverter::ConvertString(doc, recast->getConfigParams().getDetailSampleDist())));
					xmlNode->append_attribute(doc.allocate_attribute("DetailSampleMaxError", XMLConverter::ConvertString(doc, recast->getConfigParams().getDetailSampleMaxError())));
					xmlNode->append_attribute(doc.allocate_attribute("KeepInterResults", XMLConverter::ConvertString(doc, recast->getConfigParams().getKeepInterResults())));

					xml_node<>*  pointExtendsXML = doc.allocate_node(node_element, "PointExtends");
					pointExtendsXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, recast->getPointExtents().x)));
					pointExtendsXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, recast->getPointExtents().y)));
					pointExtendsXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, recast->getPointExtents().z)));
					xmlNode->append_node(pointExtendsXML);
				}
			}
		}
	}

	//void DotSceneExportModule::exportMainCamera(rapidxml::xml_node<>* cameraXML, rapidxml::xml_document<>& doc)
	//{
	//	std::hash<Ogre::String> hash;
	//	// Main Camera
	//	{
	//		Ogre::String projectionTypes[] = { "perspective", "orthgraphic" };
	//		cameraXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, this->mainCamera->getName())));
	//		cameraXML->append_attribute(doc.allocate_attribute("id", XMLConverter::ConvertString(doc, hash(this->mainCamera->getName()))));
	//		cameraXML->append_attribute(doc.allocate_attribute("fov", XMLConverter::ConvertString(doc, this->mainCamera->getFOVy())));
	//		cameraXML->append_attribute(doc.allocate_attribute("projectionType", XMLConverter::ConvertString(doc, projectionTypes[this->mainCamera->getProjectionType()])));

	//		// Clipping
	//		{
	//			xml_node<>* clippingXML = doc.allocate_node(node_element, "clipping");
	//			clippingXML->append_attribute(doc.allocate_attribute("near", XMLConverter::ConvertString(doc, this->mainCamera->getNearClipDistance())));
	//			clippingXML->append_attribute(doc.allocate_attribute("far", XMLConverter::ConvertString(doc, this->mainCamera->getFarClipDistance())));
	//			cameraXML->append_node(clippingXML);
	//		}

	//		// Position
	//		{
	//			xml_node<>* positionXML = doc.allocate_node(node_element, "position");
	//			positionXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedPosition().x)));
	//			positionXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedPosition().y)));
	//			positionXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedPosition().z)));
	//			cameraXML->append_node(positionXML);
	//		}

	//		// Rotation
	//		{
	//			xml_node<>* orientationXML = doc.allocate_node(node_element, "rotation");
	//			orientationXML->append_attribute(doc.allocate_attribute("qw", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().w)));
	//			orientationXML->append_attribute(doc.allocate_attribute("qx", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().x)));
	//			orientationXML->append_attribute(doc.allocate_attribute("qy", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().y)));
	//			orientationXML->append_attribute(doc.allocate_attribute("qz", XMLConverter::ConvertString(doc, this->mainCamera->getDerivedOrientation().z)));
	//			cameraXML->append_node(orientationXML);
	//		}
	//	}
	//}

	void DotSceneExportModule::exportSceneNodes(xml_node<>* nodesXML, xml_document<>& doc, bool exportGlobalGameObjects, const Ogre::String& filePath)
	{
		// Recursive call
		this->exportNode(this->sceneManager->getRootSceneNode(), nodesXML, doc, exportGlobalGameObjects, filePath);
	}

	void DotSceneExportModule::exportNode(Ogre::Node* ogreNode, xml_node<>* nodesXML, xml_document<>& doc, bool exportGlobalGameObject, const Ogre::String& filePath, bool recursive)
	{
// Attention: Check this! As ragdoll will store lots of nodes without name (bones)
		Ogre::String nodeName = ogreNode->getName();
		if (true == nodeName.empty())
		{
			return;
		}

		GameObject* gameObject = nullptr;
		try
		{
			gameObject = Ogre::any_cast<GameObject*>(ogreNode->getUserObjectBindings().getUserAny());
		}
		catch (...)
		{

		}

		// Attention: Scene Type must be stored too!!!
		std::hash<Ogre::String> hash;
		auto nodeIt = ogreNode->getChildIterator();
		if (true == recursive)
		{
			if (nullptr != gameObject)
			{
				// 2 cases:
				// 1: If only local game objects should be exported (exportGlobalGameObject = false) and it is a global game object (global = true), do not export this one
				// 2: If only global game objects should be exported (exportGlobalGameObject = true) and it is a local game object (global = false), do not export this one
				bool isGlobal = gameObject->getGlobal();
				if (exportGlobalGameObject != isGlobal)
				{
					while (nodeIt.hasMoreElements())
					{
						//go through all objects recursive that are attached to the scenenodes
						this->exportNode(nodeIt.getNext(), nodesXML, doc, exportGlobalGameObject, filePath, recursive);
					}
					return;
				}
			}

			// If a node has no game object, do not export the node, but the next one
			if (nullptr == gameObject)
			{
				while (nodeIt.hasMoreElements())
				{
					//go through all objects recursive that are attached to the scenenodes
					this->exportNode(nodeIt.getNext(), nodesXML, doc, exportGlobalGameObject, filePath, recursive);
				}
				return;
			}
		}

		Ogre::SceneNode::ObjectIterator objectIt = ((Ogre::SceneNode*)ogreNode)->getAttachedObjectIterator();

		// Node
		{
			xml_node<>* nodeXML = doc.allocate_node(node_element, "node");
			nodeXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ogreNode->getName())));

			nodeXML->append_attribute(doc.allocate_attribute("id", XMLConverter::ConvertString(doc, static_cast<unsigned int>(hash(ogreNode->getName())))));
			nodesXML->append_node(nodeXML);

			// Special case with camera, as the scene node does not move along with the camera, so the position/orientation would always be wrong
			Ogre::Camera* mainCamera = nullptr;
			if ("MainCamera" == ogreNode->getName())
			{
				auto movableObject = static_cast<Ogre::SceneNode*>(ogreNode)->getAttachedObject(0);
				mainCamera = dynamic_cast<Ogre::Camera*>(movableObject);
				if (nullptr == mainCamera)
				{
					if (static_cast<Ogre::SceneNode*>(ogreNode)->numAttachedObjects() > 1)
					{
						auto movableObject = static_cast<Ogre::SceneNode*>(ogreNode)->getAttachedObject(1);
						mainCamera = dynamic_cast<Ogre::Camera*>(movableObject);
					}
				}
			}
			// Position
			{
				Ogre::Vector3 position = Ogre::Vector3::ZERO;
				if (nullptr != mainCamera)
				{
					position = mainCamera->getDerivedPosition();
				}
				else
				{
					position = ogreNode->_getDerivedPositionUpdated();
				}

				xml_node<>* positionXML = doc.allocate_node(node_element, "position");
				positionXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, position.x)));
				positionXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, position.y)));
				positionXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, position.z)));
				nodeXML->append_node(positionXML);
			}

			// Rotation
			{
				Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
				if (nullptr != mainCamera)
				{
					orientation = mainCamera->getDerivedOrientation();
				}
				else
				{
					orientation = ogreNode->_getDerivedOrientationUpdated();
				}

				xml_node<>* rotationXML = doc.allocate_node(node_element, "rotation");
				rotationXML->append_attribute(doc.allocate_attribute("qw", XMLConverter::ConvertString(doc, orientation.w)));
				rotationXML->append_attribute(doc.allocate_attribute("qx", XMLConverter::ConvertString(doc, orientation.x)));
				rotationXML->append_attribute(doc.allocate_attribute("qy", XMLConverter::ConvertString(doc, orientation.y)));
				rotationXML->append_attribute(doc.allocate_attribute("qz", XMLConverter::ConvertString(doc, orientation.z)));
				nodeXML->append_node(rotationXML);
			}

			// Scale
			{
				xml_node<>* scaleXML = doc.allocate_node(node_element, "scale");
				Ogre::Vector3 scale = ogreNode->_getDerivedScaleUpdated();
				scaleXML->append_attribute(doc.allocate_attribute("x", XMLConverter::ConvertString(doc, scale.x)));
				scaleXML->append_attribute(doc.allocate_attribute("y", XMLConverter::ConvertString(doc, scale.y)));
				scaleXML->append_attribute(doc.allocate_attribute("z", XMLConverter::ConvertString(doc, scale.z)));
				nodeXML->append_node(scaleXML);
			}

			if (nullptr != gameObject)
			{
				bool foundType = false;
				Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
				if (nullptr != entity)
				{
					foundType = true;
					this->exportEntity(gameObject, entity, nodeXML, doc, filePath);
				}
				Ogre::Item* item = gameObject->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					foundType = true;
					this->exportItem(gameObject, item, nodeXML, doc, filePath);
				}
				if (false == foundType)
				{
					Ogre::ManualObject* manualObject = gameObject->getMovableObject<Ogre::ManualObject>();
					if (nullptr != manualObject)
					{
						foundType = true;
						this->exportManualObject(gameObject, manualObject, nodeXML, doc, filePath);
					}
				}
				if (false == foundType)
				{
					Ogre::Terra* terra = gameObject->getMovableObject<Ogre::Terra>();
					if (nullptr != terra)
					{
						foundType = true;
						this->exportTerra(gameObject, terra, nodeXML, doc, filePath);
					}
				}
			}
		}
#if 0
		// MovableObject
		{
			while (objectIt.hasMoreElements())
			{
				// Go through all scenenodes in the scene
				Ogre::MovableObject* movableObject = objectIt.getNext();
			}
		}
#endif
		
		while (nodeIt.hasMoreElements())
		{
			//go through all objects recursive that are attached to the scenenodes
			this->exportNode(nodeIt.getNext(), nodesXML, doc, exportGlobalGameObject, filePath, recursive);
		}
	}

	void DotSceneExportModule::exportEntity(GameObject* gameObject, Ogre::v1::Entity* entity, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath)
	{
		// Entity
		{
			xml_node<>* entityXML = doc.allocate_node(node_element, "entity");
			entityXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, entity->getName())));

			// No 'missing.mesh' should be stored, hence check if a mesh could not be loaded and get its original name
			Ogre::String meshFile;
			if (false == gameObject->getOriginalMeshNameOnLoadFailure().empty())
				meshFile = gameObject->getOriginalMeshNameOnLoadFailure();
			else
				meshFile = entity->getMesh()->getName();

			// Only calculate bounds for usable world entities
			if ("LightDirectional.mesh" != meshFile && "LightPoint.mesh" != meshFile && "LightSpot.mesh" != meshFile && "Camera.mesh" != meshFile)
				this->calculateBounds(entity->getWorldAabbUpdated());

			entityXML->append_attribute(doc.allocate_attribute("meshFile", XMLConverter::ConvertString(doc, meshFile)));
			entityXML->append_attribute(doc.allocate_attribute("castShadows", XMLConverter::ConvertString(doc, entity->getCastShadows())));
			entityXML->append_attribute(doc.allocate_attribute("visible", XMLConverter::ConvertString(doc, entity->getVisible())));

			nodeXML->append_node(entityXML);

			// SubEntity
			{
				for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
				{
					Ogre::v1::SubEntity* subEntity = entity->getSubEntity(i);
					xml_node<>* subEntityXML = doc.allocate_node(node_element, "subentity");
					subEntityXML->append_attribute(doc.allocate_attribute("index", XMLConverter::ConvertString(doc, i)));
					subEntityXML->append_attribute(doc.allocate_attribute("datablockName", XMLConverter::ConvertString(doc, *subEntity->getDatablock()->getNameStr())));
					entityXML->append_node(subEntityXML);
				}
			}

			// UserData
			{
				xml_node<>* userDataXML = doc.allocate_node(node_element, "userData");
				gameObject->writeXML(userDataXML, doc, filePath);
				entityXML->append_node(userDataXML);
			}
		}
	}

	void DotSceneExportModule::exportItem(GameObject* gameObject, Ogre::Item* item, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath)
	{
		// Entity
		{
			xml_node<>* entityXML = doc.allocate_node(node_element, "item");
			entityXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, item->getName())));

			// No 'missing.mesh' should be stored, hence check if a mesh could not be loaded and get its original name
			Ogre::String meshFile;
			if (false == gameObject->getOriginalMeshNameOnLoadFailure().empty())
				meshFile = gameObject->getOriginalMeshNameOnLoadFailure();
			else
				meshFile = item->getMesh()->getName();

			// Only calculate bounds for usable world entities
			if ("LightDirectional.mesh" != meshFile && "LightPoint.mesh" != meshFile && "LightSpot.mesh" != meshFile && "Camera.mesh" != meshFile)
				this->calculateBounds(item->getWorldAabbUpdated());

			entityXML->append_attribute(doc.allocate_attribute("meshFile", XMLConverter::ConvertString(doc, meshFile)));
			entityXML->append_attribute(doc.allocate_attribute("castShadows", XMLConverter::ConvertString(doc, item->getCastShadows())));
			entityXML->append_attribute(doc.allocate_attribute("visible", XMLConverter::ConvertString(doc, item->getVisible())));

			nodeXML->append_node(entityXML);

			// SubItem
			{
				for (unsigned int i = 0; i < item->getNumSubItems(); i++)
				{
					Ogre::SubItem* subItem = item->getSubItem(i);
					xml_node<>* subEntityXML = doc.allocate_node(node_element, "subitem");
					subEntityXML->append_attribute(doc.allocate_attribute("index", XMLConverter::ConvertString(doc, i)));
					subEntityXML->append_attribute(doc.allocate_attribute("datablockName", XMLConverter::ConvertString(doc, *subItem->getDatablock()->getNameStr())));
					entityXML->append_node(subEntityXML);
				}
			}

			// UserData
			{
				xml_node<>* userDataXML = doc.allocate_node(node_element, "userData");
				gameObject->writeXML(userDataXML, doc, filePath);
				entityXML->append_node(userDataXML);
			}
		}
	}

	void DotSceneExportModule::exportManualObject(GameObject* gameObject, Ogre::ManualObject* manualObject, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath)
	{
		// ManualObject
		{
			xml_node<>* manualObjectXML = doc.allocate_node(node_element, "manualObject");
			manualObjectXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, manualObject->getName())));

			manualObjectXML->append_attribute(doc.allocate_attribute("castShadows", XMLConverter::ConvertString(doc, manualObject->getCastShadows())));
			manualObjectXML->append_attribute(doc.allocate_attribute("visible", XMLConverter::ConvertString(doc, manualObject->getVisible())));

			this->calculateBounds(manualObject->getWorldAabbUpdated());

			nodeXML->append_node(manualObjectXML);

			// UserData
			{
				xml_node<>* userDataXML = doc.allocate_node(node_element, "userData");
				gameObject->writeXML(userDataXML, doc, filePath);
				manualObjectXML->append_node(userDataXML);
			}
		}
	}

	void DotSceneExportModule::exportTerra(GameObject* gameObject, Ogre::Terra* terra, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath)
	{
		// Terra
		{
			xml_node<>* terraXML = doc.allocate_node(node_element, "terra");
			terraXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, terra->getName())));
			terraXML->append_attribute(doc.allocate_attribute("visible", XMLConverter::ConvertString(doc, terra->getVisible())));
			nodeXML->append_node(terraXML);

			this->calculateBounds(terra->getWorldAabbUpdated());

			// UserData
			{
				xml_node<>* userDataXML = doc.allocate_node(node_element, "userData");
				gameObject->writeXML(userDataXML, doc, filePath);
				terraXML->append_node(userDataXML);
			}
		}
	}

	void DotSceneExportModule::exportGroup(const std::vector<unsigned long>& gameObjectIds, const Ogre::String& fileName, const Ogre::String& worldResourceGroupName)
	{
		// First change all the id's and look for connection patterns, to set the same new ids, so that the connections remain still active
		// This is necessary, because if a group is later loaded into a scene, where the original GO's are located, the GO's will not be added, because their ids do have collision with the original GO ids

		/*
		 Example:
		 Slider1_0				1219939954
		 JointId=					686949549  -> PassiveSlider
		 TargetId=					1961461562 -> line

		 pillar_b_0				3407179954

		 pillar_b_1				1961461562

		 Slider1_1				2484267368
		 JointId=					2497072426 -> PassiveSlider
		 JointId=					1695101210 -> Pulley
		 JointPredecessorId=		2497072426
		 JointTargetId=				1686949549
		 TargetId=					1961461562 -> line
		*/

		// GO id, list of component ids, bool flag whether id has already been replaced
		// Make a flat list for easier replacement
		std::vector<std::pair<unsigned long, bool>> flatIdsList;

		for (size_t i = 0; i < gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectIds[i]);
			if (nullptr != gameObjectPtr)
			{
				std::vector<std::pair<Ogre::String, Variant*>>& attributes = gameObjectPtr->getAttributes();
				for (size_t j = 0; j < attributes.size(); j++)
				{
					Variant* gameObjectIdAttribute = attributes[j].second;
					if (true == gameObjectIdAttribute->getIsId())
					{
						if (gameObjectIdAttribute->getULong() > 0)
						{
							flatIdsList.push_back(std::make_pair(gameObjectIdAttribute->getULong(), false));

							GameObjectComponents* components = gameObjectPtr->getComponents();
							for (auto& it = components->begin(); it != components->end(); ++it)
							{
								std::vector<std::pair<Ogre::String, Variant*>>& attributes = std::get<COMPONENT>(*it)->getAttributes();
								for (size_t k = 0; k < attributes.size(); k++)
								{
									Variant* componentIdAttribute = attributes[k].second;
									if (true == componentIdAttribute->getIsId())
									{
										if (componentIdAttribute->getULong() > 0)
										{
											flatIdsList.push_back(std::make_pair(componentIdAttribute->getULong(), false));
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// Make a copy of the ids (later for setting back the original ids)
		std::vector<std::pair<unsigned long, bool>> origFlatIdsList = flatIdsList;

		size_t nextId = 0;
		for (size_t i = 0; i < flatIdsList.size(); i++)
		{
			unsigned long originalId = flatIdsList[i].first;
			unsigned long newId = NOWA::makeUniqueID();

			nextId = i + 1;
			if (nextId < flatIdsList.size())
			{
				for (size_t j = nextId; j < flatIdsList.size(); j++)
				{
					unsigned long currentId = flatIdsList[j].first;
					if (currentId == originalId && false == flatIdsList[j].second)
					{
						flatIdsList[j].first = newId;
						flatIdsList[j].second = true; // mark as replaced
					}
				}
			}
			if (false == flatIdsList[i].second)
			{
				flatIdsList[i].first = newId;
				flatIdsList[i].second = true; // mark as replaced
			}
		}

		Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(worldResourceGroupName);
		Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
		Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

		Ogre::String filePath;
		Ogre::String filePathName;
		for (; it != itEnd; ++it)
		{
			filePath = (*it)->archive->getName() + "/Groups";
			filePathName = filePath + "/" + fileName;
			size_t found = fileName.rfind(".group");
			if (Ogre::String::npos == found)
			{
				filePathName += ".group";
			}
			break;
		}

		// Set the new ids
		this->setIdsForGameObjects(flatIdsList, gameObjectIds);

		rapidxml::xml_document<> doc;
		xml_node<>* decl = doc.allocate_node(node_declaration);
		decl->append_attribute(doc.allocate_attribute("version", "1.0"));
		decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(decl);

		// Export also resource locations, so that when a group is loaded missing resources are declared
#if 1
		xml_node<>* resourceLocationsXML = doc.allocate_node(node_element, "resourceLocations");
		this->exportResourceLocations(resourceLocationsXML, doc);
		doc.append_node(resourceLocationsXML);
#endif

		rapidxml::xml_node<>* nodesXML = doc.allocate_node(rapidxml::node_element, "nodes");
		for (size_t i = 0; i < gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectIds[i]);
			if (nullptr != gameObjectPtr)
			{
				// Write all the game object data to stream (may also be a global game object)
				this->exportNode(gameObjectPtr->getSceneNode(), nodesXML, doc, gameObjectPtr->getGlobal(), filePath, false);
			}
		}
		doc.append_node(nodesXML);

		if (filePathName.empty())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneExportModule] Error: There is no such group name resource: " + worldResourceGroupName);
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[WorldLoader] Error: There is no such group name resource: " + worldResourceGroupName + "\n", "NOWA");
		}

		std::ofstream file;
		file.open(filePathName);
		file << doc;
		file.close();

		// Set the back the original ids
		this->setIdsForGameObjects(origFlatIdsList, gameObjectIds);

		for (size_t i = 0; i < gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectIds[i]);
			if (nullptr != gameObjectPtr)
			{
				GameObjectComponents* components = gameObjectPtr->getComponents();
				for (auto& it = components->begin(); it != components->end(); ++it)
				{
					auto luaScriptCompPtr = boost::dynamic_pointer_cast<LuaScriptComponent>(std::get<COMPONENT>(*it));
					if (nullptr != luaScriptCompPtr)
					{
						Ogre::String scriptFilePathName = filePath + "/" + luaScriptCompPtr->getScriptFile();
						for (size_t j = 0; j < origFlatIdsList.size(); j++)
						{
							AppStateManager::getSingletonPtr()->getLuaScriptModule()->replaceIdsInScript(scriptFilePathName,
								Ogre::StringConverter::toString(origFlatIdsList[j].first), Ogre::StringConverter::toString(flatIdsList[j].first));
						}
					}
				}
			}
		}
	}

	void DotSceneExportModule::setIdsForGameObjects(const std::vector<std::pair<unsigned long, bool>>& idsList, const std::vector<unsigned long>& gameObjectIds)
	{
		int indexCounter = 0;
		// Now map the flat list to the game object list with the replaced ids
		for (size_t i = 0; i < gameObjectIds.size(); i++)
		{
			GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectIds[i]);
			if (nullptr != gameObjectPtr)
			{
				std::vector<std::pair<Ogre::String, Variant*>>& attributes = gameObjectPtr->getAttributes();
				for (size_t j = 0; j < attributes.size(); j++)
				{
					Variant* gameObjectIdAttribute = attributes[j].second;
					if (true == gameObjectIdAttribute->getIsId())
					{
						if (gameObjectIdAttribute->getULong() > 0)
						{
							// Set the new id
							gameObjectIdAttribute->setReadOnly(false);
							gameObjectIdAttribute->setValue(idsList[indexCounter].first);
							gameObjectIdAttribute->setReadOnly(true);
							indexCounter++;

							GameObjectComponents* components = gameObjectPtr->getComponents();
							for (auto& it = components->begin(); it != components->end(); ++it)
							{
								std::vector<std::pair<Ogre::String, Variant*>>& attributes = std::get<COMPONENT>(*it)->getAttributes();
								for (size_t k = 0; k < attributes.size(); k++)
								{
									Variant* componentIdAttribute = attributes[k].second;
									if (true == componentIdAttribute->getIsId())
									{
										if (componentIdAttribute->getULong() > 0)
										{
											// Set the new id
											componentIdAttribute->setReadOnly(false);
											componentIdAttribute->setValue(idsList[indexCounter].first);
											componentIdAttribute->setReadOnly(true);
											indexCounter++;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void DotSceneExportModule::calculateBounds(const Ogre::Aabb& worldAAB)
	{
		Ogre::Vector3 minimum = worldAAB.getMinimum();
		Ogre::Vector3 maximum = worldAAB.getMaximum();

		// calc environment bounds
		if (minimum.x < this->mostLeftNearPosition.x)
		{
			this->mostLeftNearPosition.x = minimum.x;
		}
		if (minimum.y < this->mostLeftNearPosition.y)
		{
			this->mostLeftNearPosition.y = minimum.y;
		}
		if (minimum.z < this->mostLeftNearPosition.z)
		{
			this->mostLeftNearPosition.z = minimum.z;
		}

		if (maximum.x > this->mostRightFarPosition.x)
		{
			this->mostRightFarPosition.x = maximum.x;
		}
		if (maximum.y > this->mostRightFarPosition.y)
		{
			this->mostRightFarPosition.y = maximum.y;
		}
		if (maximum.z > this->mostRightFarPosition.z)
		{
			this->mostRightFarPosition.z = maximum.z;
		}
	}

	void DotSceneExportModule::setAdditionalMeshResources(const std::vector<Ogre::String>& additionalMeshResources)
	{
		this->additionalMeshResources = additionalMeshResources;
	}

} // namespace end