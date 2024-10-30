#include "matchclassworker.h"
#include "luaeditormodelitem.h"
#include "apimodel.h"

#include <QRegularExpression>

MatchClassWorker::MatchClassWorker(LuaEditorModelItem* luaEditorModelItem, const QString& currentText,  const QString& typedAfterColon, int cursorPosition, int mouseX, int mouseY)
    : luaEditorModelItem(luaEditorModelItem),
    currentText(currentText),
    typedAfterColon(typedAfterColon),
    cursorPosition(cursorPosition),
    mouseX(mouseX),
    mouseY(mouseY),
    isProcessing(false),
    isStopped(true)
{

}

void MatchClassWorker::setParameters(const QString& currentText, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY)
{
    this->currentText = currentText;
    this->typedAfterColon = textAfterColon;
    this->cursorPosition = cursorPos;
    this->mouseX = mouseX;
    this->mouseY = mouseY;
}

void MatchClassWorker::stopProcessing(void)
{
    this->isStopped = true;
}

void MatchClassWorker::process(void)
{
    // Set the flag to indicate processing has started
    this->isProcessing = true;
    this->isStopped = false; // Reset the stop flag

    // Set the content for processing
    this->luaEditorModelItem->setContent(this->currentText);

    QRegularExpression trimmer(R"(^[^\w]+)");

    QString matchedMethodName;

    // Only re-match variables if the menu is not shown and user is not typing something after colon
    if (!ApiModel::instance()->getIsIntellisenseShown() && this->typedAfterColon.isEmpty())
    {
        this->luaEditorModelItem->detectVariables();

        // Extract text from the start up to the cursor position
        QString textBeforeCursor = this->currentText.left(this->cursorPosition);

        // Check if we should stop before processing further
        if (this->isStopped)
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if stopping
        }

        QString matchedPostIdentifier = this->luaEditorModelItem->extractWordBeforeColon(textBeforeCursor, this->cursorPosition);
        QString matchedPostMethodName = this->luaEditorModelItem->extractMethodBeforeColon(textBeforeCursor, this->cursorPosition);

        // Remove any non-identifier characters from the beginning of the strings
        matchedPostIdentifier.remove(trimmer);
        matchedPostMethodName.remove(trimmer);

        // Check for empty matched class
        if (matchedPostIdentifier.isEmpty())
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if no matched class
        }

        const auto& luaVariableInfo = this->luaEditorModelItem->getClassForVariableName(matchedPostIdentifier);

        if (!luaVariableInfo.type.isEmpty())
        {
            this->matchedClassName = luaVariableInfo.type;
        }
        else if (true == ApiModel::instance()->isValidClassName(matchedPostIdentifier))
        {
            this->matchedClassName = matchedPostIdentifier;
        }
        else
        {
            // No variable info found, take prior color into account

            // Find the last newline before the cursor position
            int lastNewlineIndex = textBeforeCursor.lastIndexOf('\n', this->cursorPosition - 1);

            // Extract the current line up to the cursor position
            QString currentLineText = (lastNewlineIndex != -1)
                                          ? textBeforeCursor.mid(lastNewlineIndex + 1).trimmed()
                                          : textBeforeCursor.trimmed(); // If no newline, take from start

            // Find the last colon in the current line
            int lastColonIndex = currentLineText.lastIndexOf(':');

            // If a colon is found in the current line, proceed with extraction
            if (lastColonIndex != -1)
            {
                QString textBeforeLastColon = currentLineText.left(lastColonIndex).trimmed();
                QString matchedPreIdentifier = this->luaEditorModelItem->extractWordBeforeColon(textBeforeLastColon, lastColonIndex);
                matchedMethodName = this->luaEditorModelItem->extractMethodBeforeColon(currentLineText, lastColonIndex);

                // Remove any non-identifier characters from the beginning of the strings
                matchedPreIdentifier.remove(trimmer);
                matchedMethodName.remove(trimmer);

                QString resultClassName = ApiModel::instance()->getClassForMethodName(matchedPreIdentifier, matchedMethodName);
                if (true == resultClassName.isEmpty())
                {
                    // Handling this case: otherGameObject:getAiFlockingComponent():getOwner(): -> to get GameObject which is the owner, determined by AiFlockingComponent and then the return type of the Method getOwner
                    resultClassName = ApiModel::instance()->getClassForMethodName(matchedPreIdentifier, matchedPostMethodName);
                    matchedMethodName = matchedPostMethodName;
                }
                else if (false == ApiModel::instance()->isValidMethodName(matchedPreIdentifier, matchedMethodName))
                {
                    matchedMethodName = matchedPostIdentifier;
                }

                // Check if the class name is valid (e.g., not void)
                if (true == ApiModel::instance()->isValidClassName(resultClassName))
                {
                    this->matchedClassName = resultClassName;
                }
                else if (true == ApiModel::instance()->isValidClassName(matchedPreIdentifier))
                {
                    this->matchedClassName = matchedPreIdentifier;
                }
                else
                {
                    const auto& luaVariableInfo = this->luaEditorModelItem->getClassForVariableName(matchedPreIdentifier);

                    if (!luaVariableInfo.type.isEmpty())
                    {
                        this->matchedClassName = luaVariableInfo.type;
                    }
                    else
                    {
                        this->isProcessing = false;
                        return;
                    }
                }
            }
            else
            {
                // Nothing found, type unknown
                this->matchedClassName.clear();
            }
        }

        if (false == this->matchedClassName.isEmpty())
        {
            Q_EMIT signal_deliverData(this->matchedClassName, matchedMethodName, this->typedAfterColon, this->cursorPosition, this->mouseX, this->mouseY);
        }

        // Check if we should stop before triggering the menu
        if (this->isStopped)
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if stopping
        }

        // Now you can trigger a method that opens the context menu in QML with the methods
        ApiModel::instance()->showIntelliSenseMenu(this->matchedClassName, mouseX, mouseY);
        return;
    }

    // Check if we should stop before processing matched methods
    if (!this->typedAfterColon.isEmpty())
    {
        // Ensure we check again for stop condition
        if (this->isStopped)
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if stopping
        }

        ApiModel::instance()->processMatchedMethodsForSelectedClass(this->matchedClassName, this->typedAfterColon);
        this->typedAfterColon.clear();

        // Emit signal directly since the data structure has already been updated
        Q_EMIT ApiModel::instance()->signal_showIntelliSenseMenu(mouseX, mouseY);
    }

    // Reset the processing flag before exiting
    this->isProcessing = false;
}
