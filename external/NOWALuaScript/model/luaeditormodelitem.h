#ifndef LUAEDITORMODELITEM_H
#define LUAEDITORMODELITEM_H

#include <QObject>

class LuaEditorModelItem : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString filePathName READ getFilePathName NOTIFY filePathNameChanged)

    Q_PROPERTY(QString content READ getContent WRITE setContent NOTIFY contentChanged)

    Q_PROPERTY(QString title READ getTitle NOTIFY titleChanged)
public:
    explicit LuaEditorModelItem(QObject* parent = Q_NULLPTR);

    virtual ~LuaEditorModelItem();

    QString getFilePathName() const;

    void setFilePathName(const QString& filePathName);

    QString getContent() const;

    void setContent(const QString& content);

    QString getTitle() const;

    void setTitle(const QString& title);

    bool getHasChanges(void) const;

    void setHasChanges(bool hasChanges);

    void restoreContent(void);

Q_SIGNALS:
    void filePathNameChanged();

    void contentChanged();

    void titleChanged();

    void hasChangesChanged();

private:
    QString filePathName;
    QString content;
    QString oldContent;
    QString title;
    bool hasChanges;
    bool firstTimeContent;
};

#endif // LUAEDITORMODELITEM_H
