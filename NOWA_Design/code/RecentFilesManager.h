#ifndef RECENT_FILES_MANAGER
#define RECENT_FILES_MANAGER

#include "MyGUI.h"

class RecentFilesManager : public MyGUI::Singleton<RecentFilesManager>
{
public:
	static const unsigned int maxRecentFiles = 5;

	typedef std::vector<MyGUI::UString> VectorUString;

	RecentFilesManager();
	virtual ~RecentFilesManager();

	void initialise();
	void shutdown();

	void addRecentFile(const MyGUI::UString& fileName);
	void removeRecentFile(const MyGUI::UString& fileName);
	void removeRecentFile(unsigned short index);
	void setActiveFile(const MyGUI::UString& fileName);
	const VectorUString& getRecentFiles() const;
	void saveSettings(void);
private:
	void checkArray(VectorUString& array, size_t maxElements);
private:
	VectorUString recentFiles;
};

#endif
