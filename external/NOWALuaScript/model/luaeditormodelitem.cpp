#include "model/luaeditormodelitem.h"

LuaEditorModelItem::LuaEditorModelItem(QObject* parent)
    : QObject{parent},
      hasChanges(false),
      firstTimeContent(true)
{

}

LuaEditorModelItem::~LuaEditorModelItem()
{

}

void LuaEditorModelItem::setFilePathName(const QString& filePathName)
{
    if (this->filePathName == filePathName)
    {
        return;
    }
    this->filePathName = filePathName;

    Q_EMIT filePathNameChanged();
}

QString LuaEditorModelItem::getContent() const
{
    return this->content;
}

void LuaEditorModelItem::setContent(const QString& content)
{
    if (this->content == content)
    {
        this->setHasChanges(false);
        return;
    }
    this->content = content;

    if (false == this->firstTimeContent)
    {
        this->setHasChanges(true);
    }
    else
    {
        this->oldContent = this->content;
    }

    if (this->content == this->oldContent)
    {
        this->setHasChanges(false);
    }

    this->firstTimeContent = false;

    Q_EMIT contentChanged();
}

QString LuaEditorModelItem::getTitle() const
{
    return this->title;
}

void LuaEditorModelItem::setTitle(const QString& title)
{
    if (this->title == title)
    {
        return;
    }
    this->title = title;

    Q_EMIT titleChanged();
}

bool LuaEditorModelItem::getHasChanges() const
{
    return this->hasChanges;
}

void LuaEditorModelItem::setHasChanges(bool hasChanges)
{
    this->hasChanges = hasChanges;

    QString tempTitle = this->title;

    if (false == tempTitle.isEmpty() && true == tempTitle.endsWith('*'))
    {
        tempTitle.chop(1);  // Removes the last character if it's '*'
    }

    if (true == this->hasChanges)
    {
        this->setTitle(tempTitle + "*");
    }
    else
    {
        this->setTitle(tempTitle);
    }

    Q_EMIT hasChangesChanged();
}

void LuaEditorModelItem::restoreContent()
{
    // If lua script has been saved, restore the content, so that it has no mor changes;
    this->oldContent = this->content;
    this->setContent(this->oldContent);

    QString tempTitle = this->title;

    if (false == tempTitle.isEmpty() && true == tempTitle.endsWith('*'))
    {
        tempTitle.chop(1);  // Removes the last character if it's '*'
    }
    this->setTitle(tempTitle);
}

QString LuaEditorModelItem::getFilePathName() const
{
    return this->filePathName;
}
