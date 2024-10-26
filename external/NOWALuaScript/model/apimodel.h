#ifndef APIMODEL_H
#define APIMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QMutex>

#include "luascriptadapter.h"

class ApiModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_PROPERTY(QString classType READ getClassType WRITE setClassType NOTIFY classTypeChanged FINAL)

    Q_PROPERTY(QString classDescription READ getClassDescription NOTIFY classDescriptionChanged FINAL)

    Q_PROPERTY(QString classInherits READ getClassInherits NOTIFY classInheritsChanged FINAL)

    Q_PROPERTY(QString selectedClassName READ getSelectedClassName WRITE setSelectedClassName NOTIFY selectedClassNameChanged FINAL)

    Q_PROPERTY(QVariantList methodsForSelectedClass READ getMethodsForSelectedClass NOTIFY methodsForSelectedClassChanged)

    Q_PROPERTY(bool isShown READ getIsShown WRITE setIsShown NOTIFY isShownChanged FINAL)
public:
    Q_INVOKABLE void showIntelliSenseMenu(const QString& wordBeforeColon, int mouseX, int mouseY);
public:
    explicit ApiModel(QObject* parent = Q_NULLPTR);

public:
    /**
     * @brief instance is the getter used to receive the object of this singleton implementation.
     * @returns singleton instance of this
     */
    static ApiModel* instance();

    /**
     * @brief The singleton type provider is needed by the Qt(-meta)-system to register this singleton instace in qml world
     * @param pEngine not used but needed by function base
     * @param pSriptEngine not used but needed by function base
     * @returns singleton instance of this
     */
    static QObject* getSingletonTypeProvider(QQmlEngine* pEngine, QJSEngine* pScriptEngine);
public:
    enum Roles
    {
        ClassNameRole = Qt::UserRole + 1,
        ClassTypeRole,
        ClassDescriptionRole,
        ClassInheritsRole
    };

    void setApiData(const QMap<QString, LuaScriptAdapter::ClassData>& apiData);

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QHash<int, QByteArray> roleNames() const override;

    QString getSelectedClassName() const;

    void setSelectedClassName(const QString& selectedClassName);

    QVariantList getMethodsForSelectedClass() const;

    void processMatchedMethodsForSelectedClass(const QString& selectedClassName, const QString& typedAfterColon);

    QString getClassForMethodName(const QString& className, const QString& methodName);

    bool isValidClassName(const QString& className);

    void closeIntellisense(void);

    QString getClassType() const;

    void setClassType(const QString& classType);

    QString getClassDescription() const;

    void setClassDescription(const QString& classDescription);

    QString getClassInherits() const;

    void setClassInherits(const QString& classInherits);

    bool getIsShown() const;

    void setIsShown(bool isShown);

Q_SIGNALS:
    void selectedClassNameChanged();

    void methodsForSelectedClassChanged();

    void classTypeChanged();

    void classDescriptionChanged();

    void classInheritsChanged();

    void signal_showIntelliSenseMenu(int mouseX, int mouseY);

    void signal_closeIntellisense();

    void isShownChanged();

private:
    static ApiModel* ms_pInstance;
    static QMutex ms_mutex;
private:
    void extracted();

    bool updateMethodsForSelectedClass(); // Helper function to update methods

    QVariantList getMethodsForClassName(const QString& className);

    QVariantMap getMethodDetails(const QString& selectedClassName, const QString& method);
private:
    QMap<QString, LuaScriptAdapter::ClassData> apiData;
    QString selectedClassName;
    QVariantList methodsForSelectedClass;

    QString classType;
    QString classDescription;
    QString classInherits;

    bool isShown;
};

#endif // APIMODEL_H
