#ifndef DOT_SCENE_EXPORT_MODULE_H
#define DOT_SCENE_EXPORT_MODULE_H

#include "utilities/rapidxml_print.hpp"

#include "defines.h"
#include "main/ProjectParameter.h"

class Ogre::SceneManager;
class Ogre::Light;
class Ogre::Viewport;
class Ogre::Camera;
class Ogre::SceneNode;
class Ogre::Node;
class Ogre::v1::Entity;
class Ogre::Item;
class OgreNewt::World;

namespace NOWA
{
	class GameObject;

	class EXPORTED DotSceneExportModule
	{
	public:
		DotSceneExportModule();

		DotSceneExportModule(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt, const ProjectParameter& projectParameter);

		DotSceneExportModule(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt);

		~DotSceneExportModule();

		void exportScene(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& sceneResourceGroupName, bool crypted = false);

		void saveSceneSnapshot(const Ogre::String& filePathName, bool crypted = false);

		void copyScene(const Ogre::String& oldSeneName, const Ogre::String& newSceneFilePathName, const Ogre::String& sceneResourceGroupName);

		void exportGlobalScene(const Ogre::String& sceneResourceGroupName, const Ogre::String& projectName, bool crypted = false);

		void exportResourceLocations(rapidxml::xml_node<>* resourcesXML, rapidxml::xml_document<>& doc);

		void exportEnvironment(rapidxml::xml_node<>* environmentXML, rapidxml::xml_document<>& doc);

		void exportOgreNewt(rapidxml::xml_node<>* xmlNode, rapidxml::xml_document<>& doc);

		void exportOgreRecast(rapidxml::xml_node<>* xmlNode, rapidxml::xml_document<>& doc);
		// void exportMainLight(rapidxml::xml_node<>* lightXML, rapidxml::xml_document<>& doc);
		// void exportMainCamera(rapidxml::xml_node<>* cameraXML, rapidxml::xml_document<>& doc);
		void exportSceneNodes(rapidxml::xml_node<>* nodesXML, rapidxml::xml_document<>& doc, bool exportGlobalGameObjects, bool recursive = true);

		void exportNode(Ogre::Node* ogreNode, rapidxml::xml_node<>* nodesXML, rapidxml::xml_document<>& doc, bool exportGlobalGameObject, bool recursive = true);

		void exportEntity(GameObject* gameObject, Ogre::v1::Entity* entity,  rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc);

		void exportItem(GameObject* gameObject, Ogre::Item* item, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc);

		void exportManualObject(GameObject* gameObject, Ogre::ManualObject* manualObject, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc);

		void exportTerra(GameObject* gameObject, Ogre::Terra* terra, rapidxml::xml_node<>* nodeXML, rapidxml::xml_document<>& doc);

		void exportGroup(const std::vector<unsigned long>& gameObjectIds, const Ogre::String& fileName, const Ogre::String& resourceGroupName);

		void setAdditionalMeshResources(const std::vector<Ogre::String>& additionalMeshResources);

	private:
		void setIdsForGameObjects(const std::vector<std::pair<unsigned long, bool>>& idsList, const std::vector<unsigned long>& gameObjectIds);
		void calculateBounds(const Ogre::Aabb& worldAAB);
	private:
		Ogre::SceneManager* sceneManager;
		OgreNewt::World* ogreNewt;
		Ogre::Vector3 mostLeftNearPosition;
		Ogre::Vector3 mostRightFarPosition;
		ProjectParameter projectParameter;

		std::vector<Ogre::String> additionalMeshResources;
	};

}; //namespace end

#endif
