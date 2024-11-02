#include "matchmethodworker.h"
#include "luaeditormodelitem.h"
#include "apimodel.h"

#include <QDebug>

MatchMethodWorker::MatchMethodWorker(LuaEditorModelItem* luaEditorModelItem, const QString& matchedClassName, const QString& typedAfterColon, int cursorPosition, int mouseX, int mouseY)
    : luaEditorModelItem(luaEditorModelItem),
    matchedClassName(matchedClassName),
    typedAfterKeyword(typedAfterColon),
    cursorPosition(cursorPosition),
    mouseX(mouseX),
    mouseY(mouseY),
    isProcessing(false),
    isStopped(true)
{

}

void MatchMethodWorker::setParameters(const QString& matchedClassName, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY)
{
    this->matchedClassName = matchedClassName;
    this->typedAfterKeyword = textAfterColon;
    this->cursorPosition = cursorPos;
    this->mouseX = mouseX;
    this->mouseY = mouseY;
}

void MatchMethodWorker::stopProcessing(void)
{
    this->isStopped = true;
}

void MatchMethodWorker::process(void)
{
    this->isProcessing = true;
    this->isStopped = false;

    ApiModel::instance()->showMatchedFunctionMenu(this->mouseX, this->mouseY);

    // Trim `typedAfterKeyword` to remove characters after the method name
    QString trimmedMethodName = this->typedAfterKeyword;
    int bracketIndex = trimmedMethodName.indexOf('(');
    if (bracketIndex != -1)
    {
        trimmedMethodName = trimmedMethodName.left(bracketIndex);
    }

    // Get the method details from ApiModel
    const auto& methodDetails = ApiModel::instance()->getMethodDetails(this->matchedClassName, trimmedMethodName);

    if (true == methodDetails.isEmpty())
    {
        this->isProcessing = false;
        QMetaObject::invokeMethod(ApiModel::instance(), "signal_noHighlightFunctionParameter",
                                  Qt::QueuedConnection);
    }

    if (!methodDetails.isEmpty() && !this->isStopped)
    {
        QString returns = methodDetails["returns"].toString();
        QString methodName = methodDetails["name"].toString();
        QString args = methodDetails["args"].toString();  // args should be formatted as "(type1 param1, type2 param2, ...)"
        QString description = methodDetails["description"].toString();

        // Combine method signature
        QString fullSignature = returns + " " + methodName + args;

        // Calculate cursor position within typed arguments
        int cursorPositionInArgs = this->typedAfterKeyword.length();

        // Strip the enclosing parentheses from `args`
        QString strippedArgs = args.mid(1, args.length() - 2);

        // Split arguments by comma and keep track of indices
        QStringList params = strippedArgs.split(", ");

        // Count commas in `typedAfterKeyword` to determine the parameter index
        int commaCount = this->typedAfterKeyword.count(',');

        // Calculate base index just after the method name and opening parenthesis
        int currentParamStart = returns.length() + 1 + methodName.length() + 1;

        // Determine the start and end index for the parameter to be highlighted
        for (int i = 0; i < params.size(); ++i)
        {
            int paramLength = params[i].length();
            int currentParamEnd = currentParamStart + paramLength;

            // Highlight if the cursor position aligns with the parameter
            if (i == commaCount)  // Check if this is the current parameter based on commas typed
            {
                ApiModel::instance()->closeIntellisense();

                QMetaObject::invokeMethod(ApiModel::instance(), "signal_highlightFunctionParameter",
                                          Qt::QueuedConnection, Q_ARG(QString, fullSignature),
                                          Q_ARG(QString, description),
                                          Q_ARG(int, currentParamStart),
                                          Q_ARG(int, currentParamEnd));
                break;
            }

            // Move to the start of the next parameter (including comma and space)
            currentParamStart = currentParamEnd + 2;
        }
    }

    this->isProcessing = false;
}












