/*!
	@file
	@author		Albert Semenov, modified by Lukas Kalinowski
	@date		09/2008
*/
#ifndef OPEN_SAVE_FILE_DIALOG_EXTENDED_H_
#define OPEN_SAVE_FILE_DIALOG_EXTENDED_H_

#include <MyGUI.h>
#include "utilities/AutoCompleteSearch.h"
#include "OpenSaveFileDialog/Dialog.h"


class OpenSaveFileDialogExtended : public tools::Dialog
{
public:
	OpenSaveFileDialogExtended();

	void setDialogInfo(const MyGUI::UString& _caption, const MyGUI::UString& _button, const MyGUI::UString& _buttonUp, bool _folderMode = false);

	void setCurrentFolder(const MyGUI::UString& _value);
	const MyGUI::UString& getCurrentFolder() const;

	void setFileName(const MyGUI::UString& _value);
	const MyGUI::UString& getFileName() const;

	const MyGUI::UString& getFilePathName() const;

	const MyGUI::UString& getMode() const;
	void setMode(const MyGUI::UString& _value);

	typedef std::vector<MyGUI::UString> VectorUString;
	void setRecentFolders(const VectorUString& _listFolders);

	void setFileMask(const MyGUI::UString& _value);
	const MyGUI::UString& getFileMask() const;

protected:
	virtual void onDoModal();
	virtual void onEndModal();

private:
	void notifyWindowButtonPressed(MyGUI::Window* _sender, const std::string& _name);
	void notifyWindowKeyButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode key, MyGUI::Char c);
	void notifyDirectoryComboAccept(MyGUI::ComboBox* _sender, size_t _index);
	void notifyDirectoryComboChangePosition(MyGUI::ComboBox* _sender, size_t _index);
	void notifyListChangePosition(MyGUI::ListBox* _sender, size_t _index);
	void notifyListSelectAccept(MyGUI::ListBox* _sender, size_t _index);
	void notifyEditSelectAccept(MyGUI::EditBox* _sender);
	void notifyMouseButtonClick(MyGUI::Widget* _sender);
	void notifyMouseButtonDoubleClick(MyGUI::Widget* _sender);
	void notifyUpButtonClick(MyGUI::Widget* _sender);
	void notifyEditTextChange(MyGUI::Widget* _sender);

	void update();
	void accept();

	void upFolder();

private:
	MyGUI::Window* mWindow;
	MyGUI::ListBox* mListFiles;
	MyGUI::EditBox* mEditFileName;
	MyGUI::EditBox* mEditSearchName;
	MyGUI::Button* mButtonUp;
	MyGUI::ComboBox* mCurrentFolderField;
	MyGUI::Button* mButtonOpenSave;

	MyGUI::UString mCurrentFolder;
	MyGUI::UString mFileName;
	MyGUI::UString mFileMask;
	MyGUI::UString mFilePathName;

	MyGUI::UString mMode;
	bool mFolderMode;
	NOWA::AutoCompleteSearch autoCompleteSearch;
};

#endif // OPEN_SAVE_FILE_DIALOG_EXTENDED_H_
