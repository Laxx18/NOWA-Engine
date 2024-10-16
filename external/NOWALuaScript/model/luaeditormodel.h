#ifndef LUAEDITORMODEL_H
#define LUAEDITORMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QMutex>

#include "luaeditormodelitem.h"

class LuaEditorModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_INVOKABLE void requestAddLuaScript(const QString& filePathName);

    Q_INVOKABLE void requestRemoveLuaScript(int index);

    Q_INVOKABLE void requestSaveLuaScript(void);

    Q_INVOKABLE int count(void);

    Q_INVOKABLE LuaEditorModelItem* getLuaScript(int index) const;
public:
    Q_PROPERTY(bool hasChanges READ getHasChanges NOTIFY hasChangesChanged FINAL)

    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
public:
    enum LuaScriptRoles
    {
        FilePathNameRole = Qt::UserRole + 1,
        TitleRole
    };

    explicit LuaEditorModel(QObject* parent = Q_NULLPTR);

    // Override required functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addLuaScript(LuaEditorModelItem* luaScriptModelItem);

    void removeLuaScript(LuaEditorModelItem* luaEditorModelItem);

    bool getHasChanges(void);

    void setHasChanges(bool hasChanges);

    void updateTitle(int row, const QString& newTitle);

    void luaScriptSaved(void);
public:
    /**
     * @brief instance is the getter used to receive the object of this singleton implementation.
     * @returns singleton instance of this
     */
    static LuaEditorModel* instance();

    /**
     * @brief The singleton type provider is needed by the Qt(-meta)-system to register this singleton instace in qml world
     * @param pEngine not used but needed by function base
     * @param pSriptEngine not used but needed by function base
     * @returns singleton instance of this
     */
    static QObject* getSingletonTypeProvider(QQmlEngine* pEngine, QJSEngine* pScriptEngine);

    int getCurrentIndex() const;

    void setCurrentIndex(int currentIndex);

    int getSize(void) const;

Q_SIGNALS:
    void signal_requestAddLuaScript(const QString& filePathName);

    void signal_requestRemoveLuaScript(int index);

    void signal_requestSaveLuaScript(const QString& filePathName, const QString& content);

    void signal_luaScriptAdded(const QString& filePathName, const QString& content);

    void hasChangesChanged();

    void currentIndexChanged();

private:
    static LuaEditorModel* ms_pInstance;
    static QMutex ms_mutex;
private:
    QList<LuaEditorModelItem*> luaScripts;
    int currentIndex;

    bool hasChanges;
};

#endif // LUAEDITORMODEL_H
