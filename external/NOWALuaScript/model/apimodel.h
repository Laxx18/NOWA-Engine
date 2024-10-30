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

    Q_PROPERTY(QString selectedMethodName READ getSelectedMethodName WRITE setSelectedMethodName NOTIFY selectedMethodNameChanged FINAL)

    Q_PROPERTY(QVariantList methodsForSelectedClass READ getMethodsForSelectedClass NOTIFY methodsForSelectedClassChanged)

    Q_PROPERTY(QVariantList constantsForSelectedClass READ getConstantsForSelectedClass NOTIFY constantsForSelectedClassChanged)

    Q_PROPERTY(bool isIntellisenseShown READ getIsIntellisenseShown WRITE setIsIntellisenseShown NOTIFY isIntellisenseShownChanged FINAL)

    Q_PROPERTY(bool isMatchedFunctionShown READ getIsMatchedFunctionShown WRITE setIsMatchedFunctionShown NOTIFY isMatchedFunctionShownChanged FINAL)

public:
    Q_INVOKABLE void showIntelliSenseMenu(const QString& wordBeforeColon, int mouseX, int mouseY);

    Q_INVOKABLE void showMatchedFunctionMenu(int mouseX, int mouseY);
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

    QString getSelectedMethodName() const;

    void setSelectedMethodName(const QString& selectedMethodName);

    QVariantList getMethodsForSelectedClass() const;

    QVariantList getConstantsForSelectedClass() const;

    void processMatchedMethodsForSelectedClass(const QString& selectedClassName, const QString& typedAfterColon);

    void processMatchedConstantsForSelectedClass(const QString& selectedClassName, const QString& typedAfterColon);

    QString getClassForMethodName(const QString& className, const QString& methodName);

    bool isValidClassName(const QString& className);

    bool isValidMethodName(const QString& className, const QString& methodName);

    void closeIntellisense(void);

    void closeMatchedFunction(void);

    QVariantMap getMethodDetails(const QString& selectedClassName, const QString& methodName);

    QString getClassType() const;

    void setClassType(const QString& classType);

    QString getClassDescription() const;

    void setClassDescription(const QString& classDescription);

    QString getClassInherits() const;

    void setClassInherits(const QString& classInherits);

    bool getIsIntellisenseShown() const;

    void setIsIntellisenseShown(bool isIntellisenseShown);

    bool getIsMatchedFunctionShown() const;

    void setIsMatchedFunctionShown(bool isMatchedFunctionShown);

    void setConstantsForSelectedClass(const QVariantList& constantsForSelectedClass);

Q_SIGNALS:
    void selectedClassNameChanged();

    void selectedMethodNameChanged();

    void methodsForSelectedClassChanged();

    void constantsForSelectedClassChanged();

    void classTypeChanged();

    void classDescriptionChanged();

    void classInheritsChanged();

    void signal_showIntelliSenseMenu(int mouseX, int mouseY);

    void signal_closeIntellisense();

    void signal_showMatchedFunctionMenu(int mouseX, int mouseY);

    void signal_highlightFunctionParameter(const QString& methodName, const QString& description, int startIndex, int endIndex);

    void signal_noHighlightFunctionParameter();

    void signal_closeMatchedFunctionMenu();

    void signal_getSelectedMethodDetails(const QString& description, const QString& returns, const QString& methodName, const QString& args);

    void isIntellisenseShownChanged();

    void isMatchedFunctionShownChanged();

private:
    static ApiModel* ms_pInstance;
    static QMutex ms_mutex;
private:
    void extracted();

    bool updateMethodsForSelectedClass(); // Helper function to update methods

    bool updateConstantsForSelectedClass(); // Helper function to update methods

    QVariantList getMethodsForClassName(const QString& className);

    QVariantList getConstantsForClassName(const QString& className);
private:
    QMap<QString, LuaScriptAdapter::ClassData> apiData;
    QString selectedClassName;
    QString selectedMethodName;
    QVariantList methodsForSelectedClass;
    QVariantList constantsForSelectedClass;

    QString classType;
    QString classDescription;
    QString classInherits;

    bool isIntellisenseShown;
    bool isMatchedFunctionShown;
};

#endif // APIMODEL_H
