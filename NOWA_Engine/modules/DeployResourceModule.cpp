#include "NOWAPrecompiled.h"
#include "DeployResourceModule.h"
#include "main/Core.h"
#include "modules/LuaScriptApi.h"
#include "main/AppStateManager.h"
#include "utilities/rapidxml.hpp"

#include <filesystem>
#include <regex>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

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
	DeployResourceModule::DeployResourceModule()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DeployResourceModule] Module created");

		this->hwndNOWALuaScript = 0;

		this->deleteLuaRuntimeErrorXmlFiles("../../external/NOWALuaScript/bin");

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DeployResourceModule::handleLuaError), NOWA::EventDataPrintLuaError::getStaticEventType());
	}

	DeployResourceModule::~DeployResourceModule()
	{
		this->taggedResourceMap.clear();
	}

	void DeployResourceModule::destroyContent(void)
	{
		this->taggedResourceMap.clear();
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DeployResourceModule::handleLuaError), NOWA::EventDataPrintLuaError::getStaticEventType());
	}

	Ogre::String DeployResourceModule::getCurrentComponentPluginFolder(void) const
	{
		return this->currentComponentPluginFolder;
	}

	bool DeployResourceModule::checkIfInstanceRunning(void)
	{
// #if defined(_WIN32)
#if 0
		// Try to create a named mutex
		this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");
		if (0 != this->hwndNOWALuaScript)
		{
			return true;
		}
#else
		Ogre::String filePath = "../../external/NOWALuaScript/bin/NOWALuaScript.running";

		struct stat buffer;
		bool fileExists = (stat(filePath.c_str(), &buffer) == 0);

		return fileExists;
#endif
		return false;
	}

	bool DeployResourceModule::sendFilePathToRunningInstance(const Ogre::String& filePathName)
	{
// #if defined(_WIN32)
#if 0
		this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");  // Replace with the correct window title
		if (this->hwndNOWALuaScript)
		{
			SetForegroundWindow(this->hwndNOWALuaScript);
			SetForegroundWindow(this->hwndNOWALuaScript);

			// Prepare a custom message identifier
			std::string messageId = "LuaScriptPath";
			std::string message = messageId + "|" + filePathName;  // Combine message ID and file path

			COPYDATASTRUCT cds;
			cds.dwData = 1;  // Optional identifier
			cds.cbData = message.size() + 1;  // Size of the message (including null terminator)
			cds.lpData = (void*)message.c_str();  // Pointer to the message

			SendMessage(this->hwndNOWALuaScript, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
			return true;
		}
		else
		{
			// If somehow mutex does exist, even it should not, release the mutex and create process again
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to find running instance of NOWALuaScript.");
			return false;
		}
#else
#if defined(_WIN32)
		this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");  // Replace with the correct window title
		if (this->hwndNOWALuaScript)
		{
			SetForegroundWindow(this->hwndNOWALuaScript);
			SetForegroundWindow(this->hwndNOWALuaScript);
		}
#endif

		bool success = false;
		// Sends the file path name to the to be opened lua script file
		// Create an XML document
		rapidxml::xml_document<> doc;

		// Create and append the root node
		rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Message");
		doc.append_node(root);

		// Create and append the message ID node
		char* messageId = doc.allocate_string("LuaScriptPath");
		rapidxml::xml_node<>* idNode = doc.allocate_node(rapidxml::node_element, "MessageId", messageId);
		root->append_node(idNode);

		// Create and append the file path node
		char* filePath = doc.allocate_string(filePathName.c_str());
		rapidxml::xml_node<>* pathNode = doc.allocate_node(rapidxml::node_element, "FilePath", filePath);
		root->append_node(pathNode);

		// Write the XML to a file
		std::ofstream outFile("../../external/NOWALuaScript/bin/lua_script_data.xml"); // Use a platform-specific path if necessary
		if (outFile.is_open())
		{
			outFile << doc; // Print the XML content to the file
			outFile.close();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: XML file created successfully at ../../external/NOWALuaScript/bin/lua_script_data.xml");
			success = true;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to write to ../../external/NOWALuaScript/bin/lua_script_data.xml");
			success = false;
		}

		// Clear the document (optional)
		doc.clear();
		return success;
#endif
	}

	void DeployResourceModule::handleLuaError(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataPrintLuaError> castEventData = boost::static_pointer_cast<NOWA::EventDataPrintLuaError>(eventData);
		// Create an XML document
		rapidxml::xml_document<> doc;

		// Create and append the root node <Message>
		rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Message");
		doc.append_node(root);

		// Create and append the <MessageId> node with the value "LuaRuntimeErrors"
		Ogre::String id = "LuaRuntimeErrors" + Ogre::StringConverter::toString(castEventData->getLine());
		char* messageId = doc.allocate_string("LuaRuntimeErrors");
		rapidxml::xml_node<>* idNode = doc.allocate_node(rapidxml::node_element, "MessageId", messageId);
		root->append_node(idNode);

		// Allocate and append the <FilePath> node with the script file path from castEventData
		Ogre::String filePathStr = castEventData->getScriptFilePathName();
		char* filePath = doc.allocate_string(filePathStr.c_str());
		rapidxml::xml_node<>* pathNode = doc.allocate_node(rapidxml::node_element, "FilePath", filePath);
		root->append_node(pathNode);

		// Format the line and start/end attributes for the <error> node
		int line = castEventData->getLine();
		Ogre::String lineStr = std::to_string(line);
		char* lineAttr = doc.allocate_string(lineStr.c_str());

		// Set the error message from castEventData
		Ogre::String errorMsg = castEventData->getErrorMessage();
		char* errorMsgText = doc.allocate_string(errorMsg.c_str());

		if (true == errorMsg.empty())
		{
			if (false == this->checkIfInstanceRunning())
			{
				return;
			}
		}

		// Create and append the <error> node with line attribute and error message text
		rapidxml::xml_node<>* errorNode = doc.allocate_node(rapidxml::node_element, "error", errorMsgText);
		errorNode->append_attribute(doc.allocate_attribute("line", lineAttr));
		errorNode->append_attribute(doc.allocate_attribute("start", "-1"));
		errorNode->append_attribute(doc.allocate_attribute("end", "-1"));
		root->append_node(errorNode);

		std::hash<Ogre::String> hash;
		unsigned int hashNumber = static_cast<unsigned int>(hash(filePathStr));
		Ogre::String outFilePathName = "../../external/NOWALuaScript/bin/lua_script_data" + Ogre::StringConverter::toString(hashNumber) + ".xml";

		// Write the XML document to a file
		std::ofstream outFile(outFilePathName); // Adjust the file path as needed
		if (outFile.is_open())
		{
			outFile << doc;
			outFile.close();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[NOWA-Engine]: XML file created successfully at: " + outFilePathName);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[NOWA-Engine]: Failed to write to: " + outFilePathName);
		}

		// Clear the document to free memory
		doc.clear();
	}

	bool DeployResourceModule::openNOWALuaScriptEditor(const Ogre::String& filePathName)
	{
		// Check if the process is already running (pseudo-implementation)
		bool instanceRunning = this->checkIfInstanceRunning(); // Implement platform-specific instance check

		if (instanceRunning)
		{
			return this->sendFilePathToRunningInstance(filePathName);
		}

#if defined(_WIN32)
		// No instance running, so start NOWALuaScript.exe
		STARTUPINFOA si = { sizeof(STARTUPINFOA) };
		PROCESS_INFORMATION pi;

		std::string command = "../../external/NOWALuaScript/bin/NOWALuaScript.exe \"" + filePathName + "\"";

		if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			// Get the window handle of NOWALuaScript
			this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");
			

			// Start a thread to monitor the process
			std::thread([this, processHandle = pi.hProcess]()
						{
							this->monitorProcess(processHandle);
						}).detach();

			if (this->hwndNOWALuaScript != NULL)
			{
				// Bring the window to the foreground
				SetForegroundWindow(this->hwndNOWALuaScript);
				SetForegroundWindow(this->hwndNOWALuaScript);
			}
			// No need to wait for the process here; we handle it in the thread
			CloseHandle(pi.hThread); // We can close the thread handle, we don't need it

			return true; // Return true when the process is started
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to start NOWALuaScript.exe.");
		}
#else
		std::string command = "../../external/NOWALuaScript/bin/NOWALuaScript \"" + filePathName + "\" &";
		int result = std::system(command.c_str());
		if (result == 0)
		{
			return true;
		}
		else
		{
			std::cerr << "Failed to start NOWALuaScript on POSIX system." << std::endl;
			return false;
		}

#endif

		return false;
	}

	void DeployResourceModule::monitorProcess(HANDLE processHandle)
	{
		// Wait for the process to exit
		WaitForSingleObject(processHandle, INFINITE);

		// Cleanup actions or notifications can go here
		this->hwndNOWALuaScript = 0;
	}

	void DeployResourceModule::deleteLuaRuntimeErrorXmlFiles(const Ogre::String& directoryPath)
	{
		namespace fs = std::filesystem;

		try
		{
			// Iterate through the directory
			for (const auto& entry : fs::directory_iterator(directoryPath))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".xml")
				{
					DeleteFile(entry.path().string().c_str()); // Delete the file
				}
			}
		}
		catch (const fs::filesystem_error&)
		{
			
		}
		catch (const std::exception&)
		{
			
		}
	}

	DeployResourceModule* DeployResourceModule::getInstance()
	{
		static DeployResourceModule instance;

		return &instance;
	}

	void DeployResourceModule::tagResource(const Ogre::String& name, const Ogre::String& resourceGroupName, Ogre::String& path)
	{
		if (true == path.empty() && false == resourceGroupName.empty())
		{
			auto& locationList = Ogre::ResourceGroupManager::getSingletonPtr()->getResourceLocationList(resourceGroupName);
			for (auto it = locationList.cbegin(); it != locationList.cend(); ++it)
			{
				Ogre::String filePathName = (*it)->archive->getName() + "//" + name;
				
				std::ifstream file(filePathName.c_str());
				
				if (file.good())
				{
					// path = Core::getSingletonPtr()->getAbsolutePath(filePathName);
					path = filePathName;
					break;
				}
			}

			// auto resourcePathes = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(resourceGroupName);
			// Note this will only work, if a resource group does have just one item as path
			// path = resourcePathes.front()->archive->getName();
		}
		this->taggedResourceMap.emplace(name, std::make_pair(resourceGroupName, path));
	}

	void DeployResourceModule::removeResource(const Ogre::String& name)
	{
		const auto& it = this->taggedResourceMap.find(name);
		if (this->taggedResourceMap.cend() != it)
		{
			this->taggedResourceMap.erase(it);
		}
	}

	std::pair<Ogre::String, Ogre::String> DeployResourceModule::getPathAndResourceGroupFromDatablock(const Ogre::String& datablockName, Ogre::HlmsTypes type)
	{
		const Ogre::String* fileNamePtr = nullptr;
		const Ogre::String* resourceGroupPtr = nullptr;
		Ogre::Hlms* hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(type);
		if (nullptr != hlms)
		{
			Ogre::HlmsUnlitDatablock* unlitDataBlock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(hlms->getDatablock(datablockName));
			if (nullptr != unlitDataBlock)
			{
				unlitDataBlock->getFilenameAndResourceGroup(&fileNamePtr, &resourceGroupPtr);
			}
		}

		Ogre::String resourceGroup;
		if (nullptr != resourceGroupPtr)
			resourceGroup = *resourceGroupPtr;


		Ogre::String path;
		if (nullptr != fileNamePtr)
			path = *fileNamePtr;

		return std::make_pair(resourceGroup, path);
	}

	Ogre::String DeployResourceModule::getResourceGroupName(const Ogre::String& name) const
	{
		const auto& it = this->taggedResourceMap.find(name);
		if (this->taggedResourceMap.cend() != it)
		{
			return it->second.first;
		}
		return Ogre::String();
	}

	Ogre::String DeployResourceModule::getResourcePath(const Ogre::String& name) const
	{
		const auto& it = this->taggedResourceMap.find(name);
		if (this->taggedResourceMap.cend() != it)
		{
			return it->second.second;
		}
		return Ogre::String();
	}

	void DeployResourceModule::createConfigFile(const Ogre::String& configurationFilePathName, const Ogre::String& applicationName)
	{
		// Create and open the configuration file
		std::ofstream configFile(configurationFilePathName.c_str());

		if (configFile.is_open())
		{
			// Write the content to the file
			configFile << "# Resources required by the sample browser and most samples.\n\n";
			configFile << "[Essential]\n";
			configFile << "FileSystem=../../media/MyGUI_Media\n";
			configFile << "FileSystem=../../media/MyGUI_Media/images\n\n";

			configFile << "[General]\n";
			configFile << "FileSystem=../../media/2.0/scripts/materials/Common\n";
			configFile << "FileSystem=../../media/2.0/scripts/materials/Common/Any\n";
			configFile << "FileSystem=../../media/2.0/scripts/materials/Common/GLSL\n";
			configFile << "FileSystem=../../media/2.0/scripts/materials/Common/GLSLES\n";
			configFile << "FileSystem=../../media/2.0/scripts/materials/Common/HLSL\n";
			configFile << "FileSystem=../../media/2.0/scripts/materials/Common/Metal\n";
			configFile << "FileSystem=../../media/Hlms/Common/Any\n";
			configFile << "FileSystem=../../media/Hlms/Common/GLSL\n";
			configFile << "FileSystem=../../media/Hlms/Common/HLSL\n";
			configFile << "FileSystem=../../media/Hlms/Common/Metal\n";
			configFile << "FileSystem=../../media/Hlms/Compute\n";
			configFile << "FileSystem=../../media/Compute/Algorithms/IBL\n";
			configFile << "FileSystem=../../media/Compute/Tools/Any\n";
			configFile << "FileSystem=../../media/NOWA/PriorScripts\n";
			configFile << "FileSystem=../../media/NOWA/Scripts\n";
			configFile << "# Custom scripts for compositor effects etc.\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/Postprocessing\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/Postprocessing/GLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/Postprocessing/HLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/terra\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/terra/GLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/terra/HLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/terra/Metal\n";
			configFile << "#FileSystem=../../media/NOWA/Scripts/ocean/textures\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SMAA\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SMAA/GLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SMAA/HLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SMAA/Metal\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SSAO\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SSAO/GLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SSAO/HLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/SSAO/Metal\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/Distortion\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/HDR\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/HDR/GLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/HDR/HLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/HDR/Metal\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/GpuParticles\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/GpuParticles/HLSL\n";
			configFile << "FileSystem=../../media/NOWA/Scripts/GpuParticles/GLSL\n";
			configFile << "FileSystem=../../media/materials/textures\n\n";

			configFile << "[NOWA_Design]\n";
			configFile << "FileSystem=../../media/MyGUI_Media/NOWA_Design\n\n";

			configFile << "[Brushes]\n";
			configFile << "FileSystem=../../media/MyGUI_Media/brushes\n\n";

			configFile << "[Audio]\n";
			configFile << "FileSystem=../../media/Audio\n";
			configFile << "FileSystem=../../media/Audio/machinery\n";
			configFile << "FileSystem=../../media/Audio/music\n";
			configFile << "FileSystem=../../media/Audio/heavy_object\n";
			configFile << "FileSystem=../../media/Audio/rumble\n\n";

			configFile << "[Models]\n";
			configFile << "FileSystem=../../media/models\n";
			configFile << "[Backgrounds]\n";
			configFile << "FileSystem=../../media/Backgrounds\n\n";

			configFile << "[Projects]\n";
			configFile << "FileSystem=../../media/projects\n\n";

			// Insert the application name in the Project section
			configFile << "[Project]\n";
			configFile << "FileSystem=../../media/Projects/" << applicationName << "\n";
			configFile << "FileSystem=../../media/Projects/" << applicationName << "/media" << "\n\n";

			configFile << "[TerrainTextures]\n";
			configFile << "FileSystem=../../media/TerrainTextures\n";
			configFile << "[Skies]\n";
			configFile << "FileSystem=../../media/Skies\n\n";

			configFile << "[NOWA]\n";
			configFile << "FileSystem=../../media/fonts\n";
			configFile << "FileSystem=../../media/NOWA\n\n";

			configFile << "[Lua]\n";
			configFile << "FileSystem=../../media/lua\n\n";

			configFile << "[ParticleUniverse]\n";
			configFile << "FileSystem=../../media/ParticleUniverse/core\n";
			configFile << "FileSystem=../../media/ParticleUniverse/examples/materials\n";
			configFile << "FileSystem=../../media/ParticleUniverse/examples/models\n";
			configFile << "FileSystem=../../media/ParticleUniverse/examples/scripts\n";
			configFile << "FileSystem=../../media/ParticleUniverse/examples/textures\n\n";

			configFile << "[GpuParticles]\n";
			configFile << "FileSystem=../../media/ParticleSystems\n\n";

			configFile << "[Unlit]\n";
			configFile << "FileSystem=../../media/materials/unlit\n\n";

			configFile << "# Do not load this as a resource. It's here merely to tell the code where\n";
			configFile << "# the Hlms templates are located\n";
			configFile << "[Hlms]\n";
			configFile << "DoNotUseAsResource=../../media\n";

			// Close the file
			configFile.close();

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Configuration file created at: " + configurationFilePathName);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to create the configuration file.");
		}
	}

	void DeployResourceModule::deploy(const Ogre::String& applicationName, const Ogre::String& sceneName, const Ogre::String& projectFilePathName)
	{
		// Go or create path + "applicationName/media" folder
		// Create resourceFolder if necessary
		// Go through all used resources and their current location an copy them to the resourceFolder
		// Either get the location from resourcePtr or from the resourcePath String
		// If its a mesh or plane, read also the location of the datablock
		// Zip everything
		// Copy all mandatory resources to "[Project]" resource folder, e.g.:
		// [Project]
		// FileSystem = ../../media/Projects/QuatLax
		// FileSystem = ../../media/Projects/QuatLax/media
		// Creates in "applicationName/bin/resources" an applicationNameDeployed.cfg file and adds the zip resource + all mandatory Ogre resources
		// Now if application is started, in Core preLoadTextures is called for [Project] resource folder, which pre loads all textures at application start.
		// TODO: Copy all necessary dll files etc. to a given folder also, so that the application is closed and separated

		Ogre::String mediaFolderFilePathName = projectFilePathName + "/media";
		Core::getSingletonPtr()->createFolder(mediaFolderFilePathName);

		Ogre::String resoucesFilePathName = Core::getSingletonPtr()->getResourcesFilePathName();

		Ogre::String configurationFilePathName = resoucesFilePathName + "/" + applicationName + "Deployed.cfg";

		this->createConfigFile(configurationFilePathName, applicationName);

		// TODO: For preLoad, listen if there is a media folder inside the projectFilePathName and load the textures from there at application start
		// Read out during resources nowa engine start if there is a [Project] 

		// jsonFilePathName must be in the form: name.material.json, often .material is missing, so check that

		Ogre::String filename = mediaFolderFilePathName;
		Ogre::String mainFileName = mediaFolderFilePathName;

		size_t materialIndex = mediaFolderFilePathName.find(".material.json");
		if (Ogre::String::npos == materialIndex)
		{
			Ogre::String json;
			const size_t jsonIndex = mediaFolderFilePathName.rfind(".json");
			if (std::string::npos != jsonIndex)
			{
				filename = mediaFolderFilePathName.substr(0, jsonIndex) + ".material.json";
			}
			else
			{
				filename += "/" + sceneName + ".material.json";
			}
		}

		bool jsonAlreadyExisting = false;
		mainFileName += "/" + applicationName + ".material.json";
		std::ifstream ifsMain(mainFileName);
		if (false == ifsMain.good())
		{
			filename = mainFileName;
		}
		else
		{
			jsonAlreadyExisting = true;
			ifsMain.close();
		}

		Ogre::String directory;
		size_t lastSlashIndex = filename.rfind('\\');
		if (std::string::npos != lastSlashIndex)
		{
			directory = filename.substr(0, lastSlashIndex);
		}
		else
		{
			lastSlashIndex = filename.rfind('/');
			if (std::string::npos != lastSlashIndex)
			{
				directory = filename.substr(0, lastSlashIndex);
			}
		}

		// Save materials to the temporary file (this prevents overwriting the actual target file)
		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
		hlmsManager->saveMaterials(Ogre::HLMS_PBS, filename, nullptr, "");

		// Get the content
		std::ifstream ifs(filename);
		if (false == ifs.good())
		{
			return;
		}
		Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

		std::set<Ogre::String> existingDataBlocks;

		auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

		// Go through all game objects to get, which are really used in scene, because only these ones should be written
		for (auto& it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
		{
			Ogre::v1::Entity* entity = it->second->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				for (size_t i = 0; i < entity->getNumSubEntities(); i++)
				{
					auto dataBlock = entity->getSubEntity(i)->getDatablock();
					if (nullptr != dataBlock)
					{
						Ogre::String resourcePath = NOWA::DeployResourceModule::getInstance()->getResourcePath(entity->getMesh()->getName());
						if (false == resourcePath.empty())
						{
							Ogre::set<Ogre::String>::type savedTextures;
							// Save all textures, that are used (textures are copied to the given folder)
							dataBlock->saveTextures(directory, savedTextures, false, true, nullptr);

							// Copy all mesh and skeleton files to the given folder
							Ogre::String destination = directory + "\\" + entity->getMesh()->getName();
							CopyFile(resourcePath.data(), destination.data(), TRUE);

							if (true == entity->getMesh()->hasSkeleton())
							{
								Ogre::String destinationSkeletonFilePathName = directory + "\\" + entity->getMesh()->getSkeletonName();
								Ogre::String destinationRagFilePathName = directory + "\\" 
									+ entity->getMesh()->getSkeletonName().substr(0, entity->getMesh()->getSkeletonName().find(".skeleton")) + ".rag";
								Ogre::String sourceSkeletonPath;
								Ogre::String sourceRagPath;
								size_t meshIndex = resourcePath.find(".mesh");
								if (Ogre::String::npos != materialIndex)
								{
									sourceSkeletonPath = resourcePath.substr(0, meshIndex) + ".skeleton";
									sourceRagPath = resourcePath.substr(0, meshIndex) + ".rag";
									if (false == sourceSkeletonPath.empty())
									{
										CopyFile(sourceSkeletonPath.data(), destinationSkeletonFilePathName.data(), TRUE);
										CopyFile(sourceRagPath.data(), destinationRagFilePathName.data(), TRUE);
									}
								}
							}
						}

						// 2. Mark all data blocks, that are really used in the scene (created game objects)
						const Ogre::String* dataBlockName = dataBlock->getNameStr();
						auto it = existingDataBlocks.find(*dataBlockName);
						if (it == existingDataBlocks.cend())
						{
							if (nullptr != dataBlockName)
							{
								// Mark only the first occurence
								size_t foundDataBlock = content.find(*dataBlockName);
								if (Ogre::String::npos != foundDataBlock)
								{
									Ogre::String marker = "-->";
									content.insert(foundDataBlock, marker);
									existingDataBlocks.emplace(*dataBlockName);
								}
							}
						}
					}
				}
			}
			else
			{
				Ogre::Item* item = it->second->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					for (size_t i = 0; i < item->getNumSubItems(); i++)
					{
						auto dataBlock = item->getSubItem(i)->getDatablock();
						if (nullptr != dataBlock)
						{
							Ogre::String resourcePath = NOWA::DeployResourceModule::getInstance()->getResourcePath(item->getMesh()->getName());
							if (false == resourcePath.empty())
							{
								Ogre::set<Ogre::String>::type savedTextures;
								// Save all textures, that are used (textures are copied to the given folder)
								dataBlock->saveTextures(directory, savedTextures, false, true, nullptr);

								// Copy all mesh and skeleton files to the given folder
								Ogre::String destination = directory + "\\" + item->getMesh()->getName();
								CopyFile(resourcePath.data(), destination.data(), TRUE);

								if (true == item->getMesh()->hasSkeleton())
								{
									Ogre::String destinationSkeletonFilePathName = directory + "\\" + item->getMesh()->getSkeletonName();
									Ogre::String destinationRagFilePathName = directory + "\\"
										+ item->getMesh()->getSkeletonName().substr(0, item->getMesh()->getSkeletonName().find(".skeleton")) + ".rag";
									Ogre::String sourceSkeletonPath;
									Ogre::String sourceRagPath;
									size_t meshIndex = resourcePath.find(".mesh");
									if (Ogre::String::npos != materialIndex)
									{
										sourceSkeletonPath = resourcePath.substr(0, meshIndex) + ".skeleton";
										sourceRagPath = resourcePath.substr(0, meshIndex) + ".rag";
										if (false == sourceSkeletonPath.empty())
										{
											CopyFile(sourceSkeletonPath.data(), destinationSkeletonFilePathName.data(), TRUE);
											CopyFile(sourceRagPath.data(), destinationRagFilePathName.data(), TRUE);
										}
									}
								}
							}

							// 2. Mark all data blocks, that are really used in the scene (created game objects)
							const Ogre::String* dataBlockName = dataBlock->getNameStr();
							auto it = existingDataBlocks.find(*dataBlockName);
							if (it == existingDataBlocks.cend())
							{
								if (nullptr != dataBlockName)
								{
									// Mark only the first occurence
									size_t foundDataBlock = content.find(*dataBlockName);
									if (Ogre::String::npos != foundDataBlock)
									{
										Ogre::String marker = "-->";
										content.insert(foundDataBlock, marker);
										existingDataBlocks.emplace(*dataBlockName);
									}
								}
							}
						}
					}
				}
			}
		}

		// Go through all data block, remove the marker and the datablocks without a marker will be removed
		bool firstDataBlock = false;
		Ogre::String marker = "-->";
		// Its hard to find some patterns in json, because its just a bunch of {, '' etc.
		// But when json is investigated for hlms, a data block always starts by 2x tab and then ''. So this is the search sequence
		Ogre::String searchSequence = "\n\t\t\"";
		size_t foundStart = 0;
		size_t length = 0;
		size_t removeStart = 0;
		bool removeNextIteration = false;
		// Go through the whole formula description and search until there is no search sequence anymore
		while (foundStart < content.size())
		{
			// Find the eval start position
			length = foundStart;
			foundStart = content.find(searchSequence, foundStart);

			// No sequence found anymore exit
			if (Ogre::String::npos == foundStart)
				break;

			// Remove but at next iteration, in order to know how long the to be removed datablock is
			if (true == removeNextIteration)
			{
				// Remove data block
				size_t removeLength = foundStart - length + 4;
				content.erase(removeStart, removeLength);
				firstDataBlock = false;
				removeStart = 0;
				// Content will be adapted at iteration, so foundStart must also be decreased
				foundStart -= removeLength;
				removeNextIteration = false;
			}

			if (std::wstring::npos != foundStart)
			{
				size_t contentSize = content.size();
				if (foundStart >= contentSize)
				{
					break;
				}
				Ogre::String sub = content.substr(foundStart + searchSequence.size(), marker.size());
				// Found marker at position, remove marker
				if (sub == marker)
				{
					content.erase(foundStart + searchSequence.size(), marker.size());
					foundStart -= marker.size();
					firstDataBlock = true;
				}
				// Found macro, blend, sampler blocks, which are required, so skip, they are always at the begin, before any data block
				else if ((sub == "Sam" || sub == "Mac" || sub == "Ble") && false == firstDataBlock)
				{

				}
				else
				{
					removeNextIteration = true;
					removeStart = foundStart;
				}
			}
			else
			{
				break;
			}
			foundStart += searchSequence.size();

		} // end while

		// maybe some marker are still there, remove them
		content = replaceAll(content, marker, "");

		std::regex materialRegex(R"("([a-zA-Z0-9_/]+)__\d+")");

		// Replace matches with only the material name (without __digits)
		content = std::regex_replace(content, materialRegex, R"("$1")");

		content = this->removeDuplicateMaterials(content);

		// Write the cleaned content back to the file
		std::ofstream ofs(filename);
		ofs << content;
		ofs.close();


		if (true == jsonAlreadyExisting)
		{
			// Check if the target file already exists and append new unique materials
			this->appendToExistingJson(mainFileName, filename);

			// Now, remove the temporary file (clean up)
			DeleteFile(filename.c_str());
		}
	}

	Ogre::String DeployResourceModule::removeDuplicateMaterials(const Ogre::String& json)
	{
		// Parse the input JSON using RapidJSON
		rapidjson::Document document;
		document.Parse(json.c_str());

		// Check for parsing errors
		if (document.HasParseError())
		{
			std::cerr << "Error parsing JSON" << std::endl;
			return json;  // Return the original json if parsing fails
		}

		std::unordered_set<std::string> unique_names;  // To track unique second-level names

		// Check if the document is an object and remove duplicates at second level
		if (document.IsObject())
		{
			for (auto it = document.MemberBegin(); it != document.MemberEnd();)
			{
				if (it->value.IsObject())  // If value is an object at the second level
				{
					std::unordered_set<std::string> second_level_names;

					// Iterate through the second-level object and remove duplicates
					rapidjson::Value& second_level_object = it->value;
					for (auto sub_it = second_level_object.MemberBegin(); sub_it != second_level_object.MemberEnd();)
					{
						std::string sub_name = sub_it->name.GetString();

						if (second_level_names.find(sub_name) != second_level_names.end())
						{
							// If the name is a duplicate, erase the entry
							sub_it = second_level_object.EraseMember(sub_it);
						}
						else
						{
							// Otherwise, add the name to the set and move to the next item
							second_level_names.insert(sub_name);
							++sub_it;
						}
					}
				}

				++it;  // Move to the next member
			}
		}

		// Convert the modified document back to a prettified string
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);  // Accept the writer to pretty-print the document

		return buffer.GetString();  // Return the prettified and modified JSON as a string
	}

	void DeployResourceModule::appendToExistingJson(const Ogre::String& filename, const Ogre::String& tempFilename)
	{
		// Gets the main json (filename) and appends the tempFileName (currently exported scene resources json members) materials and checks for uniqueness
		// Load the original JSON from the file
		std::ifstream file(filename.c_str());
		if (!file.is_open())
		{
			std::cerr << "Failed to open file: " << filename << std::endl;
			return;
		}

		std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		// Load the temporary JSON from the temp file
		std::ifstream tempFile(tempFilename.c_str());
		if (!tempFile.is_open())
		{
			std::cerr << "Failed to open file: " << tempFilename << std::endl;
			return;
		}

		std::string tempFileContent((std::istreambuf_iterator<char>(tempFile)), std::istreambuf_iterator<char>());
		tempFile.close();

		// Parse both JSON documents using RapidJSON
		rapidjson::Document originalDoc;
		originalDoc.Parse(fileContent.c_str());

		rapidjson::Document tempDoc;
		tempDoc.Parse(tempFileContent.c_str());

		// Check for parse errors
		if (originalDoc.HasParseError() || tempDoc.HasParseError())
		{
			std::cerr << "Error parsing JSON files" << std::endl;
			return;
		}

		// Set to keep track of second-level names to ensure uniqueness
		std::unordered_set<std::string> secondLevelNames;

		// Add second-level names from the original document to the set
		if (originalDoc.IsObject())
		{
			for (auto it = originalDoc.MemberBegin(); it != originalDoc.MemberEnd(); ++it)
			{
				if (it->value.IsObject())  // Check if the value is an object at second level
				{
					for (auto sub_it = it->value.MemberBegin(); sub_it != it->value.MemberEnd(); ++sub_it)
					{
						secondLevelNames.insert(sub_it->name.GetString());
					}
				}
			}
		}

		// Iterate through the temp document and only add second-level members if they don't already exist
		if (tempDoc.IsObject())
		{
			for (auto it = tempDoc.MemberBegin(); it != tempDoc.MemberEnd(); ++it)
			{
				if (it->value.IsObject())  // Only process objects at the second level
				{
					rapidjson::Value& tempObject = it->value;

					// Iterate through the second-level members of tempObject
					for (auto sub_it = tempObject.MemberBegin(); sub_it != tempObject.MemberEnd(); ++sub_it)
					{
						std::string subName = sub_it->name.GetString();

						// Only add the second-level member if it doesn't already exist in the set
						if (secondLevelNames.find(subName) == secondLevelNames.end())
						{
							secondLevelNames.insert(subName);
							// Add the second-level member to the original document
							originalDoc.AddMember(sub_it->name, sub_it->value, originalDoc.GetAllocator());
						}
					}
				}
			}
		}

		// Convert the modified original document back to a string with pretty printing
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		originalDoc.Accept(writer);

		// Write the final JSON to the file
		std::ofstream outputFile(filename.c_str());
		if (outputFile.is_open())
		{
			outputFile << buffer.GetString();
			outputFile.close();
		}
		else
		{
			std::cerr << "Failed to write to file: " << filename << std::endl;
		}
	}

	void DeployResourceModule::saveTexturesCache(const Ogre::String& sceneFolderPathName)
	{
		// Note: Will not be used, because it will not work this way: If there would be a cache for for a project, the project must be loaded in any case, so 
		// Then the textures would be preLoaded. But I need it during starting the Engine! But at this time the user has not yet determined which scene to load.
		// So the only possible scenario is using deploy function, in which all resources are copied to a folder for a game. If the game is started this folder is used to preLoad the textures at game start not at scene load stage!
		std::ifstream ifs(sceneFolderPathName + "/cachedTextures.cache");
		Ogre::set<Ogre::String>::type savedTextures;

		auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

		for (auto& it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
		{
			Ogre::set<Ogre::String>::type tempTextures;
			Ogre::v1::Entity* entity = it->second->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				for (size_t i = 0; i < entity->getNumSubEntities(); i++)
				{
					auto dataBlock = entity->getSubEntity(i)->getDatablock();
					if (nullptr != dataBlock)
					{
						Ogre::String resourcePath = NOWA::DeployResourceModule::getInstance()->getResourcePath(entity->getMesh()->getName());
						if (!resourcePath.empty())
						{
							dataBlock->saveTextures(sceneFolderPathName, tempTextures, false, true, nullptr);
							savedTextures.insert(tempTextures.begin(), tempTextures.end());
						}
					}
				}
			}
			else
			{
				Ogre::Item* item = it->second->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					for (size_t i = 0; i < item->getNumSubItems(); i++)
					{
						auto dataBlock = item->getSubItem(i)->getDatablock();
						if (nullptr != dataBlock)
						{
							Ogre::String resourcePath = NOWA::DeployResourceModule::getInstance()->getResourcePath(item->getMesh()->getName());
							if (!resourcePath.empty())
							{
								dataBlock->saveTextures(sceneFolderPathName, tempTextures, false, true, nullptr);
								savedTextures.insert(tempTextures.begin(), tempTextures.end());
							}
						}
					}
				}
			}
		}

		ifs.close();

		// Save JSON-like file manually
		std::ofstream ofs(sceneFolderPathName + "/cachedTextures.cache");
		ofs << "{\n";
		bool first = true;
		for (const auto& texture : savedTextures)
		{
			if (!first) ofs << ",\n";
			first = false;
			ofs << "  \"" << texture << "\": \"" << NOWA::DeployResourceModule::getInstance()->getResourcePath(texture) << "\"";
		}
		ofs << "\n}";
		ofs.close();
	}

	void DeployResourceModule::loadTexturesCache(const Ogre::String& sceneFolderPathName)
	{
#if 0
		Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();

		Ogre::uint32 textureFlags = Ogre::TextureFlags::AutomaticBatching;
		Ogre::uint32 filters = Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps;

		// Really important: createOrRetrieveTexture when its created, its width/height is 0 etc. so the texture is just prepared
		// it will be filled with correct data when setDataBlock is called
		Ogre::TextureGpu* texture = textureManager->createOrRetrieveTexture(textureName,
			Ogre::GpuPageOutStrategy::SaveToSystemRam, textureFlags, Ogre::TextureTypes::Type2D,
			resourceGroupName, filters, 0u);

		// Check if its a valid texture
		if (nullptr != texture)
		{
			try
			{
				texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);

			}
			catch (const Ogre::Exception& exception)
			{

			}
		}

		textureManager->waitForStreamingCompletion();
#endif
	}

	bool DeployResourceModule::createCPlusPlusProject(const Ogre::String& projectName, const Ogre::String& sceneName)
	{
		{
			// Check if project already exists
			std::ifstream ifs("../../" + projectName + "/" + projectName + ".vcxproj");
			if (true == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Warning: Project: '" +  projectName + "/" + projectName + "' does already exist. Please specify a different name, or delete the project.");
				ifs.close();
				return true;
			}
		}

		////////////////////////////Copy to destination/////////////////////////////////////
		Core::getSingletonPtr()->createFolder("../../" + projectName);
		Ogre::String destinationFolder = "../../" + projectName + "/";
		Ogre::String sourceFolder = "../../media/NOWA/ProjectTemplate/";

		Ogre::String destinationFilePathName = destinationFolder + projectName + ".vcxproj";
		Ogre::String sourceFilePathName = sourceFolder + "ProjectTemplate.vcxproj";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");
		
			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			// Replace "ProjectTemplate" with project name
			content = replaceAll(content, "ProjectTemplate", projectName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + projectName + ".vcxproj.user";
		sourceFilePathName = sourceFolder + "ProjectTemplate.vcxproj.user";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			// Replace "ProjectTemplate" with project name
			content = replaceAll(content, "ProjectTemplate", projectName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + projectName + ".vcxproj.filters";
		sourceFilePathName = sourceFolder + "ProjectTemplate.vcxproj.filters";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			// Replace "ProjectTemplate" with project name
			content = replaceAll(content, "ProjectTemplate", projectName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		////////////////////////////code/////////////////////////////////////
		destinationFolder = "../../" + projectName + "/code";

		sourceFolder = "../../media/NOWA/ProjectTemplate/code";
		Core::getSingletonPtr()->createFolder(destinationFolder);

		destinationFilePathName = destinationFolder + "/GameState.cpp";
		sourceFilePathName = sourceFolder + "/GameState.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "ProjectTemplate", projectName);
			content = replaceAll(content, "Scene1", sceneName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + "/GameState.h";
		sourceFilePathName = sourceFolder + "/GameState.h";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/main.cpp";
		sourceFilePathName = sourceFolder + "/main.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/MainApplication.cpp";
		sourceFilePathName = sourceFolder + "/MainApplication.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "ProjectTemplate", projectName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + "/MainApplication.h";
		sourceFilePathName = sourceFolder + "/MainApplication.h";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/NOWAPrecompiled.cpp";
		sourceFilePathName = sourceFolder + "/NOWAPrecompiled.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/NOWAPrecompiled.h";
		sourceFilePathName = sourceFolder + "/NOWAPrecompiled.h";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		////////////////////////////res/////////////////////////////////////
		destinationFolder = "../../" + projectName + "/res";
		sourceFolder = "../../media/NOWA/ProjectTemplate/res";
		Core::getSingletonPtr()->createFolder(destinationFolder);

		destinationFilePathName = destinationFolder + "/NOWA.ico";
		sourceFilePathName = sourceFolder + "/NOWA.ico";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/Resource.aps";
		sourceFilePathName = sourceFolder + "/Resource.aps";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/resource.h";
		sourceFilePathName = sourceFolder + "/resource.h";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/Resource.rc";
		sourceFilePathName = sourceFolder + "/Resource.rc";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		////////////////////////config file/////////////////////////////////////

		destinationFilePathName = "../resources/" + projectName + ".cfg";
		sourceFolder = "../../media/NOWA/ProjectTemplate";
		sourceFilePathName = sourceFolder + "/ProjectTemplate.cfg";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		// TODO: Check if visual studio is available

		return true;
	}

	bool DeployResourceModule::createCPlusPlusComponentPluginProject(const Ogre::String& componentName)
	{
		{
			// Check if project already exists
			std::ifstream ifs("../../NOWA_Engine/plugins" + componentName + "/" + componentName + ".vcxproj");
			if (true == ifs.good())
			{
				this->currentComponentPluginFolder = "../../NOWA_Engine/plugins/" + componentName + "/code";;

				ifs.close();
				return true;
			}
		}

		////////////////////////////Copy to destination/////////////////////////////////////

		Core::getSingletonPtr()->createFolder("../../NOWA_Engine/plugins/" + componentName);
		Ogre::String destinationFolder = "../../NOWA_Engine/plugins/" + componentName  + "/";
		Ogre::String sourceFolder = "../../media/NOWA/PluginTemplate/";

		Ogre::String destinationFilePathName = destinationFolder + componentName + ".vcxproj";
		Ogre::String sourceFilePathName = sourceFolder + "PluginTemplate.vcxproj";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");
		
			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			// Replace "PluginTemplate" with component name
			content = replaceAll(content, "PluginTemplate", componentName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + componentName + ".vcxproj.user";
		sourceFilePathName = sourceFolder + "PluginTemplate.vcxproj.user";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			// Replace "PluginTemplate" with project name
			content = replaceAll(content, "PluginTemplate", componentName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + componentName + ".vcxproj.filters";
		sourceFilePathName = sourceFolder + "PluginTemplate.vcxproj.filters";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			// Replace "PluginTemplate" with project name
			content = replaceAll(content, "PluginTemplate", componentName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		////////////////////////////code/////////////////////////////////////
		destinationFolder = "../../NOWA_Engine/plugins/" + componentName + "/code";

		this->currentComponentPluginFolder = destinationFolder;

		sourceFolder = "../../media/NOWA/PluginTemplate/code";
		Core::getSingletonPtr()->createFolder(destinationFolder);

		destinationFilePathName = destinationFolder + "/" + componentName + ".cpp";
		sourceFilePathName = sourceFolder + "/PluginTemplate.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "PluginTemplate", componentName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + "/" + componentName + ".h";
		sourceFilePathName = sourceFolder + "/PluginTemplate.h";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "PluginTemplate", componentName);

			Ogre::String upperComponentName = componentName;
			std::transform(upperComponentName.begin(), upperComponentName.end(), upperComponentName.begin(), ::toupper);
			content = replaceAll(content, "PLUGIN_TEMPLATE_H", upperComponentName + "_H");

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + "/startDll.cpp";
		sourceFilePathName = sourceFolder + "/startDll.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "PluginTemplate", componentName);

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		destinationFilePathName = destinationFolder + "/NOWAPrecompiled.cpp";
		sourceFilePathName = sourceFolder + "/NOWAPrecompiled.cpp";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		destinationFilePathName = destinationFolder + "/NOWAPrecompiled.h";
		sourceFilePathName = sourceFolder + "/NOWAPrecompiled.h";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		// Add to plugins list (Debug)
		destinationFilePathName = "../Debug/plugins.cfg";

		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");
		
			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			size_t foundComponentName = content.find(componentName);
			if (Ogre::String::npos == foundComponentName)
			{
				content += "\nPlugin=plugins/" + componentName + "_d";
			}

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		// Add to plugins list (Release)
		destinationFilePathName = "../Release/plugins.cfg";



		{
			// Get the content
			std::ifstream ifs(destinationFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
				return false;
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");
		
			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			size_t foundComponentName = content.find(componentName);
			if (Ogre::String::npos == foundComponentName)
			{
				content += "\nPlugin=plugins/" + componentName;
			}

			// Write new content
			std::ofstream ofs(destinationFilePathName);
			ofs << content;
			ofs.close();
		}

		// TODO: Check if visual studio is available

		Ogre::String executableName = componentName + ".vcxproj";

		// Does not work
		bool alreadyRunning = Core::getSingletonPtr()->isProcessRunning(executableName.data());

		if (false == alreadyRunning)
		{
			Ogre::String command = "start ../../NOWA_Engine/plugins/" + componentName + "/" + executableName;
			system(command.data());
		}

		AppStateManager::getSingletonPtr()->exitGame();

		return true;
	}

	bool DeployResourceModule::createSceneInOwnState(const Ogre::String& projectName, const Ogre::String& sceneName)
	{
		Ogre::String destinationFolder = "../../" + projectName + "/";
		Ogre::String destinationHeaderFilePathName = destinationFolder + "code/" + sceneName + "State.h";
		Ogre::String destinationImplFilePathName = destinationFolder + "code/" + sceneName + "State.cpp";

		Ogre::String sourceFolder = "../../media/NOWA/ProjectTemplate/code/";
		Ogre::String sourceHeaderFilePathName = sourceFolder + "GameState.h";
		Ogre::String sourceImplFilePathName = sourceFolder + "GameState.cpp";

		// Check if class already exists
		{
			std::ifstream ifs(destinationImplFilePathName);
			if (true == ifs.good())
			{
				ifs.close();
				return true;
			}
		}

		CopyFile(sourceHeaderFilePathName.data(), destinationHeaderFilePathName.data(), TRUE);
		CopyFile(sourceImplFilePathName.data(), destinationImplFilePathName.data(), TRUE);

		// Replace complete class name in header
		{
			// Get the content
			std::ifstream ifs(destinationHeaderFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create scene in own state failed, because the file: " + destinationHeaderFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "GameState", sceneName + "State");
			Ogre::String makroName = sceneName;
			Ogre::StringUtil::toUpperCase(makroName);
			content = replaceAll(content, "GAME_STATE_H", makroName + "_STATE_H");

			// Write new content
			std::ofstream ofs(destinationHeaderFilePathName);
			ofs << content;
			ofs.close();
		}
		
		// Replace complete class name in implementation
		{
			// Get the content
			std::ifstream ifs(destinationImplFilePathName);
			if (false == ifs.good())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create scene in own state failed, because the file: " + destinationImplFilePathName + " cannot be opened.");
				return false;
			}

			Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			ifs.close();

			content = replaceAll(content, "GameState", sceneName + "State");
			content = replaceAll(content, "ProjectTemplate", projectName);
			content = replaceAll(content, "Scene1", sceneName);

			// Write new content
			std::ofstream ofs(destinationImplFilePathName);
			ofs << content;
			ofs.close();
		}

		return true;
	}

	void DeployResourceModule::openProject(const Ogre::String& projectName)
	{
		Ogre::String executableName = projectName + ".vcxproj";

		bool alreadyRunning = Core::getSingletonPtr()->isProcessRunning(executableName.data());

		if (false == alreadyRunning)
		{
			Ogre::String command = "start ../../" + projectName + "/" + executableName;
			system(command.data());
		}
	}

	void DeployResourceModule::openLog(void)
	{
		Ogre::String command = "start " + Core::getSingletonPtr()->getLogName();
		system(command.data());
	}

	bool DeployResourceModule::startGame(const Ogre::String& projectName)
	{
		Ogre::String executable;
		Ogre::String command;
		
#if _DEBUG
		executable = projectName + "_d.exe";
#else
		executable = projectName + ".exe";
#endif

		command = "start ./" + executable;
		std::ifstream ifs(executable);
		if (false == ifs.good())
		{
			return false;
		}

		system(command.data());
		return true;
	}

	bool DeployResourceModule::createAndStartExecutable(const Ogre::String& projectName, const Ogre::String& sceneName)
	{
		this->createCPlusPlusProject(projectName, sceneName);

		Ogre::String destinationFolder = "../../" + projectName;
		Ogre::String destinationFilePathName = destinationFolder + "/" + projectName + ".vcxproj";

		// TODO: check if msbuild is available
		// /WAIT 
		// Ogre::String command = "start /B cmd /c  msbuild " + destinationFilePathName + "/p:configuration=release /p:platform=win64 /t:rebuild";

		// Does not work
		Ogre::String command = "CD C:/Windows/Microsoft.NET/Framework64/v4.0.30319";
		system(command.data());

		command = "msbuild " + destinationFilePathName + "/p:configuration=release /p:platform=x64 /t:rebuild";
		// /p:PlatformToolset=v110_xp
		system(command.data());

		system("pause");

		// std::remove(destinationFolder.data());

//		command = "start " + projectName + ".exe";
//#if _DEBUG
//		command = "start ../Release/" + projectName + ".exe";
//#endif
//		system(command.data());

		return true;
	}

	bool DeployResourceModule::createLuaInitScript(const Ogre::String& projectName)
	{
		Ogre::String filePathName = Core::getSingletonPtr()->getSectionPath("Projects")[0];
		filePathName += "/" + projectName + "/" + "init.lua";
		std::ifstream ifs(filePathName);
		if (false == ifs.good())
		{
			std::ofstream ofs(filePathName);
			if (true == ofs.good())
			{
				ofs << LuaScriptApi::getInstance()->getLuaInitScriptContent();
				ofs.close();
				return true;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create init.lua file failed, because the file: " + filePathName + " cannot be created.");
				return false;
			}
		}
		return true;
	}

	bool DeployResourceModule::createProjectBackup(const Ogre::String& projectName, const Ogre::String& sceneName)
	{
		std::list<Ogre::String> sceneBackupList;
		auto filePathNames = NOWA::Core::getSingletonPtr()->getFilePathNames("", "../../media/Projects/backup/" + projectName, "*.*");
		for (auto filePathName : filePathNames)
		{
			sceneBackupList.push_back(filePathName);
		}

		// TODO: Not per project but in project 4 backup files per scene!
		// Only allows up to 4 backup files per project
		const unsigned char maxElements = 10;
		while (sceneBackupList.size() >= maxElements)
		{
			Ogre::String oldestBackup = sceneBackupList.front();
			std::remove(oldestBackup.data());
			sceneBackupList.pop_front();
		}

		Ogre::String tempSceneName = sceneName;
		size_t found = tempSceneName.find(".scene");
		if (found != Ogre::String::npos)
		{
			tempSceneName = tempSceneName.substr(0, tempSceneName.size() - 6);
		}

		Ogre::String destinationFolder = "../../media/Projects/backup/" + projectName;
		Ogre::String sourceFolder = "../../media/Projects/" + projectName;

		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char strTempTime[1000];
		strftime(strTempTime, sizeof(strTempTime), "%Y_%m_%d_%H_%M_%S", timeinfo);
		Ogre::String strTime = strTempTime;

		Ogre::String destinationFilePathName = destinationFolder + "/" + tempSceneName + "/" + tempSceneName + "_backup_" + strTime + ".scene";
		Ogre::String sourceFilePathName = sourceFolder + "/" + tempSceneName + "/" + tempSceneName + ".scene";

		Core::getSingletonPtr()->createFolders(destinationFilePathName);

		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		Ogre::String sourceGlobalSceneFilePathName = sourceFolder + "/" + "global.scene";
		Ogre::String destinationGlobalSceneFilePathName = destinationFolder + "/" + "global.scene";
		CopyFile(sourceGlobalSceneFilePathName.data(), destinationGlobalSceneFilePathName.data(), TRUE);

		Ogre::String sourceLuaInitSceneFilePathName = sourceFolder + "/" + "init.lua";
		Ogre::String destinationLuaInitSceneFilePathName = destinationFolder + "/" + "init.lua";
		CopyFile(sourceLuaInitSceneFilePathName.data(), destinationLuaInitSceneFilePathName.data(), TRUE);

		std::ifstream ifs(destinationFilePathName);
		if (false == ifs.good())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create project backup failed, because the file: " + destinationFilePathName + " cannot be opened.");
			return false;
		}

		return true;
	}

} // namespace end