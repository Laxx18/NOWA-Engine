#include "luaeditormodelitem.h"
#include "apimodel.h"

#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

LuaEditorModelItem::LuaEditorModelItem(QObject* parent)
    : QObject{parent},
    hasChanges(false),
    firstTimeContent(true),
    worker(Q_NULLPTR),
    thread(Q_NULLPTR)
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
    if (this->content == content && this->oldContent == this->content)
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

void LuaEditorModelItem::openProjectFolder()
{
    // Extract the folder path from the full file path
    QFileInfo fileInfo(this->filePathName);
    QString folderPath = fileInfo.absolutePath();

    // Open the folder in the system's file explorer
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

#if 0
void LuaEditorModelItem::matchClass(const QString& currentText, int cursorPosition, int mouseX, int mouseY)
{
    this->setContent(currentText);
    // TODO: Later in own thread
    this->detectVariables();

    // Extract text from the start up to the cursor position
    QString textBeforeCursor = this->content.left(cursorPosition);

    QString matchedClass = this->extractWordBeforeColon(textBeforeCursor, cursorPosition);
    if (true == matchedClass.isEmpty())
    {
        return;
    }

    // Now you can trigger a method that opens the context menu in QML with the methods
    ApiModel::instance()->showIntelliSenseMenu(matchedClass, mouseX, mouseY);
}
#endif

QString LuaEditorModelItem::getFilePathName() const
{
    return this->filePathName;
}

QString LuaEditorModelItem::extractWordBeforeColon(const QString& currentText, int cursorPos)
{
    // Start from the cursor position and move backwards to find the last word before a colon
    int startPos = cursorPos - 1;

    // Iterate backwards from the cursor position until a delimiter is found
    while (startPos >= 0)
    {
        QChar currentChar = currentText[startPos];

        // Stop if a delimiter (":", "=", space, or line break) is found
        if (currentChar == ':' || currentChar == '=' || currentChar == ' ' || currentChar == '\n')
        {
            break;
        }

        startPos--;
    }

    // Extract the word between the delimiter and the cursor position
    QString wordBeforeColon = currentText.mid(startPos + 1, cursorPos - (startPos + 1)).trimmed();

    // Now handle case-sensitive class recognition
    // Remove "get" prefix and brackets if found
    if (wordBeforeColon.startsWith("get"))
    {
        wordBeforeColon = wordBeforeColon.mid(3);  // Remove the "get"
    }

    // Remove any trailing parentheses (e.g., from function calls like "Controller()")
    if (wordBeforeColon.endsWith("()"))
    {
        wordBeforeColon.chop(2);  // Remove the last two characters "()"
    }

    return wordBeforeColon;
}

QString LuaEditorModelItem::extractMethodBeforeColon(const QString& currentText, int cursorPos)
{
    // Start from the cursor position and move backwards to find the last word before a colon
    int startPos = cursorPos - 1;

    // Iterate backwards from the cursor position until a delimiter is found
    while (startPos >= 0)
    {
        QChar currentChar = currentText[startPos];

        // Stop if a delimiter (":", "=", space, or line break) is found
        if (currentChar == ':' || currentChar == '=' || currentChar == ' ' || currentChar == '\n')
        {
            break;
        }

        startPos--;
    }

    // Extract the word between the delimiter and the cursor position
    QString methodBeforeColon = currentText.mid(startPos + 1, cursorPos - (startPos + 1)).trimmed();

    // Remove any trailing parentheses (e.g., from function calls like "Controller()")
    if (methodBeforeColon.endsWith("()"))
    {
        methodBeforeColon.chop(2);  // Remove the last two characters "()"
    }

    return methodBeforeColon;
}

#if 0
void LuaEditorModelItem::detectVariables(void)
{
    this->variableMap.clear();
    QStringList lines = this->content.split('\n');

    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();

        // Detect local variables
        QRegularExpression localVarRegex(R"(local\s+(\w+)\s*=)");
        QRegularExpressionMatch match = localVarRegex.match(line);

        if (match.hasMatch())
        {
            QString varName = match.captured(1);
            this->variableMap[varName] = LuaVariableInfo{varName, "", i + 1, "local"};
        }

        // Detect global assignments
        QRegularExpression globalVarRegex(R"((\w+)\s*=)");
        match = globalVarRegex.match(line);

        if (match.hasMatch() && !line.startsWith("local"))
        {
            QString varName = match.captured(1);
            this->variableMap[varName] = LuaVariableInfo{varName, "", i + 1, "global"};
        }

        // Parameter variables are not of interest, because no type can be determined anyway, cast... must be used
        // // Detect function parameters
        // QRegularExpression functionParamRegex(R"(function\s*\((.*)\))");
        // match = functionParamRegex.match(line);

        // if (match.hasMatch())
        // {
        //     QString paramList = match.captured(1).trimmed();  // Get the parameter list inside parentheses
        //     QStringList paramNames = paramList.split(',', Qt::SplitBehaviorFlags::SkipEmptyParts);

        //     for (const QString& param : paramNames)
        //     {
        //         QString paramName = param.trimmed();

        //         if (!paramName.isEmpty())
        //         {
        //             // Add each parameter to the variable map as a local variable (inside function scope)
        //             this->variableMap[paramName] = LuaVariableInfo{paramName, "", i + 1, "parameter"};
        //         }
        //     }
        // }

        // Now handle the detection of "cast" and assignment of type
        int castIndex = line.indexOf("cast");
        if (castIndex != -1)
        {
            // We found the word "cast", so now we need to extract the class name and variable
            int classNameStart = castIndex + 4;  // "cast" is 4 characters long

            // Find the start of the class name (skip the parentheses)
            int openParenIndex = line.indexOf('(', classNameStart);
            int closeParenIndex = line.indexOf(')', openParenIndex);

            QString className = line.mid(classNameStart, openParenIndex - classNameStart).trimmed();

            // Now go backward to find the variable before the "="
            int equalsIndex = line.lastIndexOf('=', castIndex);
            QString varName = "";

            if (equalsIndex != -1)
            {
                // Find the variable name before the "=" by iterating backward
                int varNameEnd = equalsIndex - 1;

                while (varNameEnd >= 0 && line[varNameEnd].isSpace())
                {
                    varNameEnd--;
                }

                int varNameStart = varNameEnd;
                while (varNameStart >= 0 && (line[varNameStart].isLetterOrNumber() || line[varNameStart] == '_'))
                {
                    varNameStart--;
                }

                varName = line.mid(varNameStart + 1, varNameEnd - varNameStart).trimmed();
            }

            if (!varName.isEmpty() && !className.isEmpty())
            {
                // Assign the detected class name as the type for the variable
                this->variableMap[varName] = LuaVariableInfo{varName, className, i + 1, "cast"};
            }
        }

        // Handle method call assignments and infer variable types
        QRegularExpression assignmentRegex(R"((\w+)\s*=\s*(\w+):(\w+)\(\))");
        match = assignmentRegex.match(line);

        if (match.hasMatch())
        {
            QString leftVar = match.captured(1);  // The variable being assigned
            QString objectVar = match.captured(2);  // The object being called
            QString methodName = match.captured(3);  // The method being invoked

            // Check if the objectVar already exists in the variableMap and has a type assigned
            if (this->variableMap.contains(objectVar) && !this->variableMap[objectVar].type.isEmpty())
            {
                QString objectClass = this->variableMap[objectVar].type;

                // Set the selected class name in ApiModel for method lookup
                ApiModel::instance()->setSelectedClassName(objectClass);

                // Fetch the methods for this class
                QVariantList methods = ApiModel::instance()->getMethodsForSelectedClass();

                // Find the method in the class's methods and get its return type
                for (const QVariant& methodVariant : methods)
                {
                    QVariantMap methodMap = methodVariant.toMap();
                    QString currentMethodName = methodMap["name"].toString();

                    if (currentMethodName == methodName)
                    {
                        QString returnType = methodMap["returns"].toString();

                        // Assign the return type to the leftVar
                        if (!returnType.isEmpty())
                        {
                            this->variableMap[leftVar] = LuaVariableInfo{leftVar, returnType, i + 1, "inferred"};
                        }

                        break;
                    }
                }
            }
        }
    }


    // Now we have a map of variables that can be queried when `:` is pressed
}

#endif

#if 0
void LuaEditorModelItem::detectVariables(void)
{
    this->variableMap.clear();
    QStringList lines = this->content.split('\n');

    // First Pass: Detect all variables
    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();

        // Detect local variables
        QRegularExpression localVarRegex(R"(local\s+(\w+)\s*=)");
        QRegularExpressionMatch match = localVarRegex.match(line);

        if (match.hasMatch())
        {
            QString varName = match.captured(1);
            this->variableMap[varName] = LuaVariableInfo{varName, "", i + 1, "local"};
        }

        // Detect global assignments
        QRegularExpression globalVarRegex(R"((\w+)\s*=)");
        match = globalVarRegex.match(line);

        if (match.hasMatch() && !line.startsWith("local"))
        {
            QString varName = match.captured(1);
            this->variableMap[varName] = LuaVariableInfo{varName, "", i + 1, "global"};
        }
    }

    // Second Pass: Infer variable types based on assignments and method calls
    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();

        // Handle the detection of "cast" and assignment of type
        int castIndex = line.indexOf("cast");
        if (castIndex != -1)
        {
            // We found the word "cast", so now we need to extract the class name and variable
            int classNameStart = castIndex + 4;  // "cast" is 4 characters long

            // Find the start of the class name (skip the parentheses)
            int openParenIndex = line.indexOf('(', classNameStart);
            int closeParenIndex = line.indexOf(')', openParenIndex);

            QString className = line.mid(classNameStart, openParenIndex - classNameStart).trimmed();

            // Now go backward to find the variable before the "="
            int equalsIndex = line.lastIndexOf('=', castIndex);
            QString varName = "";

            if (equalsIndex != -1)
            {
                // Find the variable name before the "=" by iterating backward
                int varNameEnd = equalsIndex - 1;

                while (varNameEnd >= 0 && line[varNameEnd].isSpace())
                {
                    varNameEnd--;
                }

                int varNameStart = varNameEnd;
                while (varNameStart >= 0 && (line[varNameStart].isLetterOrNumber() || line[varNameStart] == '_'))
                {
                    varNameStart--;
                }

                varName = line.mid(varNameStart + 1, varNameEnd - varNameStart).trimmed();
            }

            if (!varName.isEmpty() && !className.isEmpty())
            {
                // Assign the detected class name as the type for the variable
                this->variableMap[varName] = LuaVariableInfo{varName, className, i + 1, this->variableMap[varName].scope};
            }
        }

        // Handle method call assignments and infer variable types
        QRegularExpression assignmentRegex(R"((\w+)\s*=\s*(\w+):(\w+)\(\))");
        QRegularExpressionMatch match = assignmentRegex.match(line);

        if (match.hasMatch())
        {
            QString leftVar = match.captured(1);  // The variable being assigned
            QString objectVar = match.captured(2);  // The object being called
            QString methodName = match.captured(3);  // The method being invoked

            // Check if the objectVar already exists in the variableMap
            if (this->variableMap.contains(objectVar) && !this->variableMap[objectVar].type.isEmpty())
            {
                // Set the selected class name in ApiModel for method lookup
                ApiModel::instance()->setSelectedClassName(this->variableMap[objectVar].type);

                // Fetch the methods for this class
                QVariantList methods = ApiModel::instance()->getMethodsForSelectedClass();

                // Find the method in the class's methods and get its return type
                for (const QVariant& methodVariant : methods)
                {
                    QVariantMap methodMap = methodVariant.toMap();
                    QString currentMethodName = methodMap["name"].toString();

                    if (currentMethodName == methodName)
                    {
                        // Removes the brackets around the return type
                        QString returnType = methodMap["returns"].toString().remove(QRegularExpression("[()]"));

                        // Assign the return type to the leftVar
                        if (!returnType.isEmpty())
                        {
                            this->variableMap[leftVar] = LuaVariableInfo{leftVar, returnType, i + 1, this->variableMap[leftVar].scope};
                        }

                        break;
                    }
                }
            }
        }

        // Handle method chain assignment (e.g., variable = object:method1():method2())
        QRegularExpression assignmentChainRegex(R"((\w+)\s*=\s*(\w+(:\w+\(\))+))");
        match = assignmentChainRegex.match(line);
        if (match.hasMatch())
        {
            QString assignedVar = match.captured(1);  // Variable on the left-hand side
            QString objectVar = match.captured(2);    // Object with method chain on the right-hand side

            // Resolve the type of the object variable through method chain
            QString resolvedType = resolveMethodChainType(objectVar);
            if (!resolvedType.isEmpty())
            {
                this->variableMap[assignedVar] = LuaVariableInfo{assignedVar, resolvedType, i + 1, this->variableMap[assignedVar].scope};
            }
        }

        // Handle simple assignments (no colons) (e.g., temp2 = temp), so temp2 should exactly be of type temp
        QRegularExpression simpleAssignmentRegex(R"((\w+)\s*=\s*(\w+)\s*;?)");
        QRegularExpressionMatch simpleMatch = simpleAssignmentRegex.match(line);

        if (simpleMatch.hasMatch())
        {
            QString leftVar = simpleMatch.captured(1);  // Variable being assigned
            QString rightVar = simpleMatch.captured(2);  // Variable from which to inherit type

            // Ensure this is a "naked" assignment (no colons involved)
            if (!line.contains(":") && this->variableMap.contains(rightVar))
            {
                // Inherit the type from the right-hand variable
                QString rightVarType = this->variableMap[rightVar].type;

                if (!rightVarType.isEmpty())
                {
                    this->variableMap[leftVar] = LuaVariableInfo{leftVar, rightVarType, i + 1, this->variableMap[leftVar].scope};
                }
            }
        }
    }
}
#endif

void LuaEditorModelItem::detectVariables(void)
{
    this->variableMap.clear();
    QStringList lines = this->content.split('\n');

    // First Pass: Detect all variables
    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();

        // Detect local variables
        QRegularExpression localVarRegex(R"(local\s+(\w+)\s*=)");
        QRegularExpressionMatch match = localVarRegex.match(line);

        if (match.hasMatch())
        {
            QString varName = match.captured(1);
            this->variableMap[varName] = LuaVariableInfo{varName, "", i + 1, "local"};
        }

        // Detect global assignments
        QRegularExpression globalVarRegex(R"((\w+)\s*=)");
        match = globalVarRegex.match(line);

        if (match.hasMatch() && !line.startsWith("local"))
        {
            QString varName = match.captured(1);
            this->variableMap[varName] = LuaVariableInfo{varName, "", i + 1, "global"};
        }
    }

    // Second Pass: Infer variable types based on assignments and method calls
    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();

        // Split the line into multiple statements using semicolon ";"
        QStringList statements = line.split(';', Qt::SkipEmptyParts);

        // Process each statement separately
        for (const QString& statement : statements)
        {
            QString trimmedStatement = statement.trimmed();

            // Handle the detection of "cast" and assignment of type
            int castIndex = trimmedStatement.indexOf("cast");
            if (castIndex != -1)
            {
                // We found the word "cast", so now we need to extract the class name and variable
                int classNameStart = castIndex + 4;  // "cast" is 4 characters long

                // Find the start of the class name (skip the parentheses)
                int openParenIndex = trimmedStatement.indexOf('(', classNameStart);
                int closeParenIndex = trimmedStatement.indexOf(')', openParenIndex);

                QString className = trimmedStatement.mid(classNameStart, openParenIndex - classNameStart).trimmed();

                // Now go backward to find the variable before the "="
                int equalsIndex = trimmedStatement.lastIndexOf('=', castIndex);
                QString varName = "";

                if (equalsIndex != -1)
                {
                    // Find the variable name before the "=" by iterating backward
                    int varNameEnd = equalsIndex - 1;

                    while (varNameEnd >= 0 && trimmedStatement[varNameEnd].isSpace())
                    {
                        varNameEnd--;
                    }

                    int varNameStart = varNameEnd;
                    while (varNameStart >= 0 && (trimmedStatement[varNameStart].isLetterOrNumber() || trimmedStatement[varNameStart] == '_'))
                    {
                        varNameStart--;
                    }

                    varName = trimmedStatement.mid(varNameStart + 1, varNameEnd - varNameStart).trimmed();
                }

                if (!varName.isEmpty() && !className.isEmpty())
                {
                    // Assign the detected class name as the type for the variable
                    this->variableMap[varName] = LuaVariableInfo{varName, className, i + 1, this->variableMap[varName].scope};
                }
            }

            // Handle method call assignments and infer variable types
            QRegularExpression assignmentRegex(R"((\w+)\s*=\s*(\w+):(\w+)\(\))");
            QRegularExpressionMatch match = assignmentRegex.match(trimmedStatement);

            if (match.hasMatch())
            {
                QString leftVar = match.captured(1);  // The variable being assigned
                QString objectVar = match.captured(2);  // The object being called
                QString methodName = match.captured(3);  // The method being invoked

                // Check if the objectVar already exists in the variableMap
                if (this->variableMap.contains(objectVar) && !this->variableMap[objectVar].type.isEmpty())
                {
                    // Set the selected class name in ApiModel for method lookup
                    ApiModel::instance()->setSelectedClassName(this->variableMap[objectVar].type);

                    // Fetch the methods for this class
                    QVariantList methods = ApiModel::instance()->getMethodsForSelectedClass();

                    // Find the method in the class's methods and get its return type
                    for (const QVariant& methodVariant : methods)
                    {
                        QVariantMap methodMap = methodVariant.toMap();
                        QString currentMethodName = methodMap["name"].toString();

                        if (currentMethodName == methodName)
                        {
                            // Removes the brackets around the return type
                            QString returnType = methodMap["returns"].toString().remove(QRegularExpression("[()]"));

                            // Assign the return type to the leftVar
                            if (!returnType.isEmpty())
                            {
                                this->variableMap[leftVar] = LuaVariableInfo{leftVar, returnType, i + 1, this->variableMap[leftVar].scope};
                            }

                            break;
                        }
                    }
                }
            }

            // Handle method chain assignment (e.g., variable = object:method1():method2())
            QRegularExpression assignmentChainRegex(R"((\w+)\s*=\s*(\w+(:\w+\(\))+))");
            match = assignmentChainRegex.match(trimmedStatement);
            if (match.hasMatch())
            {
                QString assignedVar = match.captured(1);  // Variable on the left-hand side
                QString objectVar = match.captured(2);    // Object with method chain on the right-hand side

                // Resolve the type of the object variable through method chain
                QString resolvedType = resolveMethodChainType(objectVar);
                if (!resolvedType.isEmpty())
                {
                    this->variableMap[assignedVar] = LuaVariableInfo{assignedVar, resolvedType, i + 1, this->variableMap[assignedVar].scope};
                }
            }

            // Handle simple assignments (no colons) (e.g., temp2 = temp), so temp2 should exactly be of type temp
            QRegularExpression simpleAssignmentRegex(R"((\w+)\s*=\s*(\w+)\s*;?)");
            QRegularExpressionMatch simpleMatch = simpleAssignmentRegex.match(trimmedStatement);

            if (simpleMatch.hasMatch())
            {
                QString leftVar = simpleMatch.captured(1);  // Variable being assigned
                QString rightVar = simpleMatch.captured(2);  // Variable from which to inherit type

                // Ensure this is a "naked" assignment (no colons involved)
                if (!trimmedStatement.contains(":") && this->variableMap.contains(rightVar))
                {
                    // Inherit the type from the right-hand variable
                    QString rightVarType = this->variableMap[rightVar].type;

                    if (!rightVarType.isEmpty())
                    {
                        this->variableMap[leftVar] = LuaVariableInfo{leftVar, rightVarType, i + 1, this->variableMap[leftVar].scope};
                    }
                }
            }
        }
    }
}

QString LuaEditorModelItem::resolveMethodChainType(const QString& objectVar)
{
    // Split object:method chain by ":" but also handle chains that are broken by spaces
    QStringList parts = objectVar.split(QRegularExpression(":|\\s"), Qt::SkipEmptyParts);

    if (parts.size() < 2)
    {
        return "";
    }

    QString baseVar = parts.first();  // The base variable before any ":"
    QString currentType = this->variableMap.contains(baseVar) ? this->variableMap[baseVar].type : "";

    if (currentType.isEmpty())
    {
        return "";  // No known type for the base variable
    }

    // Iterate through each method in the chain until a space is encountered
    for (int i = 1; i < parts.size(); ++i)
    {
        QString part = parts[i];

        // Stop if we encounter a space (indicating a new chain starts)
        if (part.contains(' '))
        {
            break;
        }

        QString methodName = part;

        // Remove parentheses "()" from the method name if present
        if (methodName.endsWith("()"))
        {
            methodName = methodName.mid(0, methodName.size() - 2);
        }

        // Now get the methods available for the current type
        ApiModel::instance()->setSelectedClassName(currentType);
        QVariantList methods = ApiModel::instance()->getMethodsForSelectedClass();

        bool methodFound = false;

        for (const QVariant& methodVariant : methods)
        {
            QVariantMap methodMap = methodVariant.toMap();
            if (methodMap["name"].toString() == methodName)
            {
                // Method found, update the type for the next method in the chain
                currentType = methodMap["returns"].toString().remove('(').remove(')');
                methodFound = true;
                break;
            }
        }

        if (!methodFound)
        {
            return "";  // If a method in the chain is not found, stop the resolution
        }
    }

    return currentType;  // The resolved type of the last method in the chain
}

LuaEditorModelItem::LuaVariableInfo LuaEditorModelItem::getClassForVariableName(const QString& variableName)
{
    LuaEditorModelItem::LuaVariableInfo luaVariableInfo;
    if (this->variableMap.contains(variableName))
    {
        luaVariableInfo = this->variableMap[variableName];
    }

    return luaVariableInfo;
}

void LuaEditorModelItem::startIntellisenseProcessing(const QString& currentText, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY)
{
    // If there's already a worker, interrupt its process
    if (Q_NULLPTR != this->worker)
    {
        // Stop the ongoing process
        this->worker->stopProcessing();

        // Optionally, you might want to wait for the finished signal before updating parameters
        // But this might block the UI; consider how you want to handle it
        // connect(worker, &MatchClassWorker::finished, this, [this, currentText, textAfterColon, cursorPos, mouseX, mouseY]() {
        //     worker->setParameters(currentText, textAfterColon, cursorPos, mouseX, mouseY);
        // });

        this->worker->setParameters(currentText, textAfterColon, cursorPos, mouseX, mouseY);

        // Emit a request to process in the worker thread
        Q_EMIT this->worker->signal_requestProcess();
    }
    else
    {
        // Create a new worker instance
        this->worker = new MatchClassWorker(this, currentText, textAfterColon, cursorPos, mouseX, mouseY);

        // Create and start a new thread for processing
        thread = QThread::create([this]
             {
                // Connect the signal to the process method
                QObject::connect(this->worker, &MatchClassWorker::signal_requestProcess, this->worker, &MatchClassWorker::process);
                this->worker->process();
             });

        QObject::connect(worker, &MatchClassWorker::finished, this, [this]() {
            // Cleanup and reset when finished
            delete worker; // Delete the worker instance when done
            worker = Q_NULLPTR; // Reset the worker pointer
            thread->quit();  // Quit the thread
        });

        QObject::connect(worker, &MatchClassWorker::finished, worker, &MatchClassWorker::deleteLater);
        QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);

        // Start processing
        thread->start();
    }
}

void LuaEditorModelItem::closeIntellisense()
{
    ApiModel::instance()->closeIntellisense();
}
