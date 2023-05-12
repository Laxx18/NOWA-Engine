#include "NOWAPrecompiled.h"
#include "DeployResourceModule.h"
#include "main/Core.h"
#include "modules/LuaScriptApi.h"
#include "main/AppStateManager.h"

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
	}

	DeployResourceModule::~DeployResourceModule()
	{
		this->taggedResourceMap.clear();
	}

	void DeployResourceModule::destroyContent(void)
	{
		this->taggedResourceMap.clear();
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

	void DeployResourceModule::deploy(const Ogre::String& applicationName, const Ogre::String& jsonFilePathName)
	{
		// Go or create path + "applicationName/media" folder
		// Create resourceFolder if necessary
		// Go through all used resources and their current location an copy them to the resourceFolder
		// Either get the location from resourcePtr or from the resourcePath String
		// If its a mesh or plane, read also the location of the datablock
		// Zip everything
		// Copy all mandatory resources to "applicationName/media/..." folder
		// Create in "applicationName/bin" an applicationName.cfg file an add the zip resource + all mandatory Ogre resources

		// jsonFilePathName must be in the form: name.material.json, often .material is missing, so check that

		Ogre::String filename = jsonFilePathName;

		size_t materialIndex = jsonFilePathName.find(".material.json");
		if (Ogre::String::npos == materialIndex)
		{
			Ogre::String json;
			const size_t jsonIndex = jsonFilePathName.rfind(".json");
			if (std::string::npos != jsonIndex)
			{
				filename = jsonFilePathName.substr(0, jsonIndex) + ".material.json";
			}
			else
			{
				filename += ".material.json";
			}
		}

		Ogre::String directory;
		const size_t lastSlashIndex = filename.rfind('\\');
		if (std::string::npos != lastSlashIndex)
		{
			directory = filename.substr(0, lastSlashIndex);
		}

		// 1. First save all data blocks to have at least all sampler, macro and blend blocks gathered
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

		// Write new content
		std::ofstream ofs(filename);
		ofs << content;
		ofs.close();
	}

	bool DeployResourceModule::createCPlusPlusProject(const Ogre::String& projectName, const Ogre::String& worldName)
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
			content = replaceAll(content, "World1", worldName);

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
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Warning: Plugin component: '" + componentName + "' does already exist. Please specify a different name, or delete the project.");
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

	bool DeployResourceModule::createSceneInOwnState(const Ogre::String& projectName, const Ogre::String& worldName)
	{
		Ogre::String destinationFolder = "../../" + projectName + "/";
		Ogre::String destinationHeaderFilePathName = destinationFolder + "code/" + worldName + "State.h";
		Ogre::String destinationImplFilePathName = destinationFolder + "code/" + worldName + "State.cpp";

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

			content = replaceAll(content, "GameState", worldName + "State");
			Ogre::String makroName = worldName;
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

			content = replaceAll(content, "GameState", worldName + "State");
			content = replaceAll(content, "ProjectTemplate", projectName);
			content = replaceAll(content, "World1", worldName);

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

	bool DeployResourceModule::createAndStartExecutable(const Ogre::String& projectName, const Ogre::String& worldName)
	{
		this->createCPlusPlusProject(projectName, worldName);

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

	bool DeployResourceModule::createProjectBackup(const Ogre::String& projectName, const Ogre::String& worldName)
	{
		auto filePathNames = NOWA::Core::getSingletonPtr()->getFilePathNames("", "../../media/Projects/backup/", "*.*");
		for (auto filePathName : filePathNames)
		{
			size_t found = filePathName.find_last_of("/\\");
			if (Ogre::String::npos == found)
				continue;

			filePathName = filePathName.substr(found + 1, filePathName.length() - found);
			Ogre::String pattern = "_backup_";
			size_t found1 = filePathName.find(pattern);
			if (found1 != Ogre::String::npos)
			{
				Ogre::String worldNamePattern = filePathName.substr(0, filePathName.size() - pattern.size());
				this->sceneBackupMap[projectName].push_back(filePathName);
			}
			
		}

		Ogre::String tempWorldName = worldName;
		size_t found = tempWorldName.find(".scene");
		if (found != Ogre::String::npos)
		{
			tempWorldName = tempWorldName.substr(0, tempWorldName.size() - 6);
		}

		Ogre::String destinationFolder = "../../media/Projects/backup/" + projectName;
		Core::getSingletonPtr()->createFolders(destinationFolder + "/");
		Ogre::String sourceFolder = "../../media/Projects/" + projectName;

		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char strTempTime[1000];
		strftime(strTempTime, sizeof(strTempTime), "%Y_%m_%d_%H_%M_%S", timeinfo);
		Ogre::String strTime = strTempTime;

		Ogre::String destinationFilePathName = destinationFolder + "/" + tempWorldName + "_backup_" + strTime + ".scene";
		Ogre::String sourceFilePathName = sourceFolder + "/" + tempWorldName + ".scene";
		CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

		std::ifstream ifs(destinationFilePathName);
		if (false == ifs.good())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create project backup failed, because the file: " + destinationFilePathName + " cannot be opened.");
			return false;
		}
		else
		{
			this->sceneBackupMap[projectName].push_back(destinationFilePathName);
		}

		return true;
	}

} // namespace end