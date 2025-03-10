/*!
	@file
	@author		Albert Semenov, modified by Lukas Kalinowski
	@date		09/2008
*/
#include "NOWAPrecompiled.h"
#include "OpenSaveFileDialogExtended.h"

#include "FileSystemInfo/FileSystemInfo.h"

OpenSaveFileDialogExtended::OpenSaveFileDialogExtended()
	: tools::Dialog("OpenSaveFileDialog.layout"),
	mWindow(nullptr),
	mListFiles(nullptr),
	mEditFileName(nullptr),
	mButtonUp(nullptr),
	mCurrentFolderField(nullptr),
	mButtonOpenSave(nullptr),
	mFileMask("*.*"),
	mFolderMode(false)
{
	assignWidget(mListFiles, "ListFiles");
	assignWidget(mEditFileName, "EditFileName");
	assignWidget(mEditSearchName, "EditSearchName");
	assignWidget(mButtonUp, "ButtonUp");
	assignWidget(mCurrentFolderField, "CurrentFolder");
	assignWidget(mButtonOpenSave, "ButtonOpenSave");

	mListFiles->eventListChangePosition += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyListChangePosition);
	mListFiles->eventListSelectAccept += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyListSelectAccept);
	mListFiles->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyMouseButtonDoubleClick);
	mListFiles->eventMouseButtonClick += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyMouseButtonDoubleClick);
	mEditFileName->eventEditSelectAccept += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyEditSelectAccept);
	mButtonUp->eventMouseButtonClick += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyUpButtonClick);
	mCurrentFolderField->eventComboAccept += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyDirectoryComboAccept);
	mCurrentFolderField->eventComboChangePosition += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyDirectoryComboChangePosition);
	mButtonOpenSave->eventMouseButtonClick += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyMouseButtonClick);
	mEditSearchName->eventEditTextChange += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyEditTextChange);
	

	mWindow = mMainWidget->castType<MyGUI::Window>();
	mWindow->eventWindowButtonPressed += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyWindowButtonPressed);
	mWindow->eventKeyButtonPressed += MyGUI::newDelegate(this, &OpenSaveFileDialogExtended::notifyWindowKeyButtonPressed);

	mCurrentFolder = common::getSystemCurrentFolder();

	mMainWidget->setVisible(false);

	update();
}

void OpenSaveFileDialogExtended::notifyWindowButtonPressed(MyGUI::Window* _sender, const std::string& _name)
{
	if (_name == "close")
		eventEndDialog(this, false);
}

void OpenSaveFileDialogExtended::notifyWindowKeyButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode key, MyGUI::Char c)
{
	for (size_t i = 0; i < mListFiles->getItemCount(); i++)
	{
		MyGUI::UString itemText = mListFiles->getItem(i);
		if (itemText[1] == c)
		{
			mListFiles->setItemSelect(i);
			mListFiles->setScrollPosition(i * 19); // 21 line height
			accept();
			break;
		}
	}
}

void OpenSaveFileDialogExtended::notifyEditSelectAccept(MyGUI::EditBox* _sender)
{
	accept();
}

void OpenSaveFileDialogExtended::notifyMouseButtonClick(MyGUI::Widget* _sender)
{
	accept();
}

void OpenSaveFileDialogExtended::notifyMouseButtonDoubleClick(MyGUI::Widget* _sender)
{
	accept();
}

void OpenSaveFileDialogExtended::notifyUpButtonClick(MyGUI::Widget* _sender)
{
	upFolder();
}

void OpenSaveFileDialogExtended::setDialogInfo(const MyGUI::UString& _caption, const MyGUI::UString& _button, const MyGUI::UString& _buttonUp, bool _folderMode)
{
	mFolderMode = _folderMode;
	mWindow->setCaption(_caption);
	mButtonOpenSave->setCaption(_button);
	mButtonUp->setCaption(_buttonUp);
	mEditFileName->setVisible(!_folderMode);
}

void OpenSaveFileDialogExtended::notifyListChangePosition(MyGUI::ListBox* _sender, size_t _index)
{
	this->autoCompleteSearch.reset();
	mEditSearchName->setCaption("");

	if (_index == MyGUI::ITEM_NONE)
	{
		mEditFileName->setCaption("");
	}
	else
	{
		common::FileInfo info = *_sender->getItemDataAt<common::FileInfo>(_index);

		if (L".." == info.name)
		{
			upFolder();
			return;
		}

		if (!info.folder)
		{
			mEditFileName->setCaption(info.name);
			mFilePathName = mCurrentFolder + "/" + info.name;
		}
		else
		{
			// Remove *
			if (false == mFileMask.empty())
			{
				std::wstring extension = mFileMask.substr(1, mFileMask.size() - 1);
				size_t found = info.name.find(mFileMask);
				if (std::wstring::npos == found)
				{
					mCurrentFolder = mCurrentFolder + "/" + info.name;
					update();
				}
			}
		}
	}
}

void OpenSaveFileDialogExtended::notifyListSelectAccept(MyGUI::ListBox* _sender, size_t _index)
{
	if (_index == MyGUI::ITEM_NONE) return;

	common::FileInfo info = *_sender->getItemDataAt<common::FileInfo>(_index);
	if (info.folder)
	{
		if (info.name == L"..")
		{
			upFolder();
		}
		else
		{
			mCurrentFolder = common::concatenatePath(mCurrentFolder.asWStr(), info.name);
			update();
		}
	}
	else
	{
		accept();
	}
}

void OpenSaveFileDialogExtended::notifyEditTextChange(MyGUI::Widget* _sender)
{
	if (_sender == mEditSearchName)
	{
		MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(_sender);

		update();

		Ogre::String searchText = editBox->getOnlyText();
		if (true == searchText.empty())
			return;

		this->autoCompleteSearch.reset();

		// Add all registered comopnents to the search
		for (size_t i = 0; i < mListFiles->getItemCount(); i++)
		{
			MyGUI::UString itemText = mListFiles->getItem(i);
			this->autoCompleteSearch.addSearchText(itemText);
		}

		// Get the matched results
		auto& matchedFileNames = this->autoCompleteSearch.findMatchedItemWithInText(searchText);

		for (size_t j = 1; j < mListFiles->getItemCount();)
		{
			MyGUI::UString itemText = mListFiles->getItem(j);

			int index = -1;
			// 0 index is [..] folder back
			
			for (size_t i = 0; i < matchedFileNames.getResults().size(); i++)
			{
				Ogre::String fileName = matchedFileNames.getResults()[i].getMatchedItemText();

				if (Ogre::String(itemText) != fileName)
				{
					index = j;
				}
				else
				{
					index = -1;
					break;
				}
			}

			if (-1 != index)
				mListFiles->removeItemAt(index);
			else
				j++; // Only iterate if nothing has been removed, because removement is taken in place in the loop
		}
	}
}

void OpenSaveFileDialogExtended::accept()
{
	this->autoCompleteSearch.reset();
	mEditSearchName->setCaption("");

	if (!mFolderMode)
	{
		mFileName = mEditFileName->getOnlyText();
		if (!mFileName.empty())
			eventEndDialog(this, true);
	}
	else
	{
		if (mListFiles->getIndexSelected() != MyGUI::ITEM_NONE)
		{
			common::FileInfo info = *mListFiles->getItemDataAt<common::FileInfo>(mListFiles->getIndexSelected());
			if (!common::isParentDir(info.name.c_str()))
				mCurrentFolder = common::concatenatePath(mCurrentFolder.asWStr(), info.name);
		}
		eventEndDialog(this, true);
	}
}

void OpenSaveFileDialogExtended::upFolder()
{
	this->autoCompleteSearch.reset();
	mEditSearchName->setCaption("");

	size_t index = mCurrentFolder.find_last_of(L"\\/");
	if (index != std::string::npos)
	{
		mCurrentFolder = mCurrentFolder.substr(0, index);
	}
	update();
}

void OpenSaveFileDialogExtended::setCurrentFolder(const MyGUI::UString& _folder)
{
	mCurrentFolder = _folder.empty() ? MyGUI::UString(common::getSystemCurrentFolder()) : _folder;

	update();
}

void OpenSaveFileDialogExtended::update()
{
	if (mCurrentFolder.empty())
		mCurrentFolder = "/";
	mCurrentFolderField->setCaption(mCurrentFolder);

	mListFiles->removeAllItems();

	// add all folders first
	common::VectorFileInfo infos;
	getSystemFileList(infos, mCurrentFolder, L"*.*");

	for (common::VectorFileInfo::iterator item = infos.begin(); item != infos.end(); ++item)
	{
		if ((*item).folder)
			mListFiles->addItem(L"[" + (*item).name + L"]", *item);
	}

	if (!mFolderMode)
	{
		// add files by given mask
		infos.clear();
		getSystemFileList(infos, mCurrentFolder, mFileMask);

		for (common::VectorFileInfo::iterator item = infos.begin(); item != infos.end(); ++item)
		{
			if (!(*item).folder)
				mListFiles->addItem((*item).name, *item);
		}
	}

	MyGUI::InputManager::getInstancePtr()->setMouseFocusWidget(this->mMainWidget);
}

void OpenSaveFileDialogExtended::setFileName(const MyGUI::UString& _value)
{
	mFileName = _value;
	mEditFileName->setCaption(_value);
}

void OpenSaveFileDialogExtended::setFileMask(const MyGUI::UString& _value)
{
	mFileMask = _value;
	update();
}

const MyGUI::UString& OpenSaveFileDialogExtended::getFileMask() const
{
	return mFileMask;
}

void OpenSaveFileDialogExtended::onDoModal()
{
	update();

	MyGUI::IntSize windowSize = mMainWidget->getSize();
	MyGUI::IntSize parentSize = mMainWidget->getParentSize();

	mMainWidget->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

void OpenSaveFileDialogExtended::onEndModal()
{
}

void OpenSaveFileDialogExtended::notifyDirectoryComboAccept(MyGUI::ComboBox* _sender, size_t _index)
{
	setCurrentFolder(_sender->getOnlyText());
}

const MyGUI::UString& OpenSaveFileDialogExtended::getCurrentFolder() const
{
	return mCurrentFolder;
}

const MyGUI::UString& OpenSaveFileDialogExtended::getFileName() const
{
	return mFileName;
}

const MyGUI::UString& OpenSaveFileDialogExtended::getFilePathName() const
{
	return mFilePathName;
}

const MyGUI::UString& OpenSaveFileDialogExtended::getMode() const
{
	return mMode;
}

void OpenSaveFileDialogExtended::setMode(const MyGUI::UString& _value)
{
	mMode = _value;
}

void OpenSaveFileDialogExtended::setRecentFolders(const VectorUString& _listFolders)
{
	mCurrentFolderField->removeAllItems();

	for (VectorUString::const_iterator item = _listFolders.begin(); item != _listFolders.end(); ++item)
		mCurrentFolderField->addItem((*item));
}

void OpenSaveFileDialogExtended::notifyDirectoryComboChangePosition(MyGUI::ComboBox* _sender, size_t _index)
{
	if (_index != MyGUI::ITEM_NONE)
		setCurrentFolder(_sender->getItemNameAt(_index));
}
