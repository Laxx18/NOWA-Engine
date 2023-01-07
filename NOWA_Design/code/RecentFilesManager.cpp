#include "NOWAPrecompiled.h"
#include "RecentFilesManager.h"
#include "FileSystemInfo/FileSystemInfo.h"
#include "FileSystemInfo/pugixml.cpp"
// #include "FileSystemInfo/sigslot.cpp"
#include "FileSystemInfo/SettingsManager.cpp"

using namespace tools;

template <> RecentFilesManager* MyGUI::Singleton<RecentFilesManager>::msInstance = nullptr;
template <> const char* MyGUI::Singleton<RecentFilesManager>::mClassTypeName = "RecentFilesManager";

RecentFilesManager::RecentFilesManager()
{
}

RecentFilesManager::~RecentFilesManager()
{
}

void RecentFilesManager::initialise()
{
	// Init recent files manager
	new SettingsManager();

	// Load recent files
	SettingsManager::getInstance().loadUserSettingsFile("../resources/NOWA_Design_Settings.xml");
	this->recentFiles = SettingsManager::getInstance().getValueList<MyGUI::UString>("Files/RecentFile.List");
	
	checkArray(this->recentFiles, RecentFilesManager::maxRecentFiles);
}

void RecentFilesManager::shutdown()
{
	// Save recent files
	this->saveSettings();

	delete SettingsManager::getInstancePtr();
}

void RecentFilesManager::checkArray(VectorUString& array, size_t maxElements)
{
	for (size_t index = 0; index < array.size(); ++index)
	{
		array.erase(std::remove(array.begin() + index + 1, array.end(), array[index]), array.end());
	}
	while (array.size() > maxElements)
	{
		array.pop_back();
	}
}

void RecentFilesManager::addRecentFile(const MyGUI::UString& fileName)
{
	for (size_t i = 0; i < this->recentFiles.size(); i++)
	{
		if (this->recentFiles[i] == fileName)
		{
			return;
		}
	}

	this->recentFiles.insert(this->recentFiles.begin(), fileName);

	checkArray(this->recentFiles, RecentFilesManager::maxRecentFiles);

	SettingsManager::getInstance().setValueList<MyGUI::UString>("Files/RecentFile.List", this->recentFiles);
}

void RecentFilesManager::removeRecentFile(const MyGUI::UString& fileName)
{
	for (size_t i = 0; i < this->recentFiles.size(); i++)
	{
		if (this->recentFiles[i] == fileName)
		{
			this->recentFiles.erase(this->recentFiles.begin() + i);
		}
	}

	this->saveSettings();
}

void RecentFilesManager::removeRecentFile(unsigned short index)
{
	if (index < this->recentFiles.size())
		this->recentFiles.erase(this->recentFiles.begin() + index);

	this->saveSettings();
}

void RecentFilesManager::setActiveFile(const MyGUI::UString& fileName)
{
	// When the user clicks on a recent file, move that file to front and all other one below
	int indexNewRecentFile = -1;
	for (size_t i = 0; i < this->recentFiles.size(); i++)
	{
		if (this->recentFiles[i] == fileName)
		{
			indexNewRecentFile = i;
			break;
		}
	}
	if (-1 != indexNewRecentFile)
	{
		this->recentFiles.erase(this->recentFiles.begin() + indexNewRecentFile);
	}
	this->recentFiles.insert(this->recentFiles.begin(), fileName);

	this->saveSettings();
}

const RecentFilesManager::VectorUString& RecentFilesManager::getRecentFiles() const
{
	return this->recentFiles;
}

void RecentFilesManager::saveSettings(void)
{
	SettingsManager::getInstance().setValueList<MyGUI::UString>("Files/RecentFile.List", this->recentFiles);
	SettingsManager::getInstance().saveUserSettingsFile();
}
