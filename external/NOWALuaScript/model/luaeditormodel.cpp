#include "model/luaeditormodel.h"
#include "apimodel.h"

LuaEditorModel* LuaEditorModel::ms_pInstance = Q_NULLPTR;
QMutex LuaEditorModel::ms_mutex;

LuaEditorModel::LuaEditorModel(QObject* parent)
    : QAbstractListModel(parent),
    currentIndex(-1),
    hasChanges(false)
{

}

LuaEditorModel* LuaEditorModel::instance()
{
    QMutexLocker lock(&ms_mutex);

    if (ms_pInstance == Q_NULLPTR)
    {
        ms_pInstance = new LuaEditorModel();
    }
    return ms_pInstance;
}

QObject* LuaEditorModel::getSingletonTypeProvider(QQmlEngine* pEngine, QJSEngine* pScriptEngine)
{
    Q_UNUSED(pEngine)
    Q_UNUSED(pScriptEngine)

    return instance();
}

int LuaEditorModel::getCurrentIndex() const
{
    return this->currentIndex;
}

void LuaEditorModel::setCurrentIndex(int currentIndex)
{
    if (this->currentIndex == currentIndex)
    {
        return;
    }
    this->currentIndex = currentIndex;

    Q_EMIT currentIndexChanged();
}

int LuaEditorModel::getSize() const
{
    return this->luaScripts.size();
}

bool LuaEditorModel::getHasChanges(void)
{
    if (-1 == this->currentIndex)
    {
        return false;
    }

    LuaEditorModelItem* luaEditorModelItem = this->luaScripts.at(this->currentIndex);

    if (Q_NULLPTR != luaEditorModelItem)
    {
        return luaEditorModelItem->getHasChanges();
    }

    return false;
}

void LuaEditorModel::setHasChanges(bool hasChanges)
{
    if (this->hasChanges == hasChanges)
    {
        return;
    }
    this->hasChanges = hasChanges;

    Q_EMIT hasChangesChanged();
}

int LuaEditorModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return this->luaScripts.count();
}

QVariant LuaEditorModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= this->luaScripts.count())
    {
        return QVariant();
    }

    LuaEditorModelItem* item = this->luaScripts.at(index.row());

    if (role == FilePathNameRole)
    {
        return item->getFilePathName();
    }
    else if (role == TitleRole)
    {
        return item->getTitle();
    }

    return QVariant();
}

QHash<int, QByteArray> LuaEditorModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FilePathNameRole] = "filePathName";
    roles[TitleRole] = "title";
    return roles;
}

void LuaEditorModel::addLuaScript(LuaEditorModelItem* luaScriptModelItem)
{
    beginInsertRows(QModelIndex(), this->luaScripts.count(), this->luaScripts.count());
    this->luaScripts.append(luaScriptModelItem);
    endInsertRows();

    // Connection to update title, each time the content changed and so the title changed
    connect(luaScriptModelItem, &LuaEditorModelItem::titleChanged, this, [=]()
    {
        QModelIndex index = this->index(this->luaScripts.indexOf(luaScriptModelItem));
        Q_EMIT dataChanged(index, index, { TitleRole });
    });

    connect(luaScriptModelItem, &LuaEditorModelItem::hasChangesChanged, this, [=]()
    {
        this->setHasChanges(luaScriptModelItem->getHasChanges());
    });

    Q_EMIT signal_luaScriptAdded(luaScriptModelItem->getFilePathName(), luaScriptModelItem->getContent());
}

void LuaEditorModel::removeLuaScript(LuaEditorModelItem* luaEditorModelItem)
{
    int index = 0;
    for (const auto& currentLuaEditorModelItem : this->luaScripts)
    {
        if (currentLuaEditorModelItem == luaEditorModelItem)
        {
            break;
        }
        index++;
    }

    beginRemoveRows(QModelIndex(), index, index);
    delete this->luaScripts.takeAt(index);
    endRemoveRows();
}

void LuaEditorModel::requestAddLuaScript(const QString& filePathName)
{
    Q_EMIT signal_requestAddLuaScript(filePathName);
}

void LuaEditorModel::requestRemoveLuaScript(int index)
{
    Q_EMIT signal_requestRemoveLuaScript(index);
}

void LuaEditorModel::requestSaveLuaScript(void)
{
    if (-1 == this->currentIndex)
    {
        qDebug() << "Error: Could not save file because there is no lua script.";
    }

    LuaEditorModelItem* luaEditorModelItem = this->luaScripts.at(this->currentIndex);

    if (Q_NULLPTR != luaEditorModelItem)
    {
        if (true == luaEditorModelItem->getHasChanges())
        {
            Q_EMIT signal_requestSaveLuaScript(luaEditorModelItem->getFilePathName(), luaEditorModelItem->getContent());
        }
    }
    else
    {
        qDebug() << "Error: Could not save file, because the corresponding lua editor model instance does not exist.";
    }
}

int LuaEditorModel::count(void)
{
    return this->luaScripts.size();
}

LuaEditorModelItem* LuaEditorModel::getEditorModelItem(int index) const
{
    if (index < 0 || index >= this->luaScripts.count())
    {
        return Q_NULLPTR;
    }

    return this->luaScripts.at(index);
}

void LuaEditorModel::commentLines()
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_commentLines();
    }
}

void LuaEditorModel::unCommentLines()
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_unCommentLines();
    }
}

void LuaEditorModel::addTabToSelection()
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_addTabToSelection();
    }
}

void LuaEditorModel::removeTabFromSelection()
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_removeTabFromSelection();
    }
}

void LuaEditorModel::breakLine()
{
    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_breakLine();
    }
}

void LuaEditorModel::searchInTextEdit(const QString& searchText, bool wholeWord, bool caseSensitive)
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_searchInTextEdit(searchText, wholeWord, caseSensitive);
    }
}

void LuaEditorModel::replaceInTextEdit(const QString& searchText, const QString& replaceText)
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_replaceInTextEdit(searchText, replaceText);
    }
}

void LuaEditorModel::clearSearch()
{
    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_clearSearch();
    }
}

void LuaEditorModel::undo()
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_undo();
    }
}

void LuaEditorModel::redo()
{
    if (true == ApiModel::instance()->getIsIntellisenseShown() || true == ApiModel::instance()->getIsMatchedFunctionShown())
    {
        return;
    }

    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        Q_EMIT luaEditorModelItem->signal_redo();
    }
}

void LuaEditorModel::openProjectFolder()
{
    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        luaEditorModelItem->openProjectFolder();
    }
}

void LuaEditorModel::sendTextToEditor(const QString& text)
{
    const auto& luaEditorModelItem = this->getEditorModelItem(this->currentIndex);
    if (Q_NULLPTR != luaEditorModelItem)
    {
        ApiModel::instance()->setSelectedMethodName(text);
        luaEditorModelItem->signal_sendTextToEditor(text);
    }
}

void LuaEditorModel::updateTitle(int row, const QString& newTitle)
{
    if (row < 0 || row >= this->luaScripts.count())
    {
        return;
    }

    LuaEditorModelItem* item = this->luaScripts.at(row);
    item->setTitle(newTitle);

    // Notify view about the data change for the TitleRole
    QModelIndex index = this->index(row);
    Q_EMIT dataChanged(index, index, { TitleRole });
}

void LuaEditorModel::luaScriptSaved()
{
    if (-1 == this->currentIndex)
    {
        qDebug() << "Error: Could not save file because there is no lua script.";
    }

    LuaEditorModelItem* luaEditorModelItem = this->luaScripts.at(this->currentIndex);

    if (Q_NULLPTR != luaEditorModelItem)
    {
        luaEditorModelItem->restoreContent();
    }
}
