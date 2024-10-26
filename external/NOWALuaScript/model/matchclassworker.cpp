#include "matchclassworker.h"
#include "luaeditormodelitem.h"
#include "apimodel.h"

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

    // Only re-match variables if the menu is not shown and user is not typing something after colon
    if (!ApiModel::instance()->getIsShown() && this->typedAfterColon.isEmpty())
    {
        this->luaEditorModelItem->detectVariables();

        // Extract text from the start up to the cursor position
        QString textBeforeCursor = this->currentText.left(cursorPosition);

        // Check if we should stop before processing further
        if (this->isStopped)
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if stopping
        }

        QString matchedClass = this->luaEditorModelItem->extractWordBeforeColon(textBeforeCursor, cursorPosition);

        // Check for empty matched class
        if (matchedClass.isEmpty())
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if no matched class
        }

        const auto& luaVariableInfo = this->luaEditorModelItem->getClassForVariableName(matchedClass);

        if (!luaVariableInfo.type.isEmpty())
        {
            this->matchedClassName = luaVariableInfo.type;
        }
        else if (true == ApiModel::instance()->isValidClassName(matchedClass))
        {
            this->matchedClassName = matchedClass;
        }
        else
        {
            // No variable info found, take prior color into account

            // Find the last colon before the current position
            int lastColonIndex = textBeforeCursor.lastIndexOf(':', cursorPosition - 1);

            // If a colon is found, extract the text before it
            if (lastColonIndex != -1)
            {
                QString textBeforeLastColon = textBeforeCursor.left(lastColonIndex).trimmed();
                // Extract the class name from the text before the last colon
                QString matchedClass = this->luaEditorModelItem->extractWordBeforeColon(textBeforeLastColon, lastColonIndex);
                QString matchedMethod = this->luaEditorModelItem->extractMethodBeforeColon(textBeforeCursor, cursorPosition);

                QString resultClassName = ApiModel::instance()->getClassForMethodName(matchedClass, matchedMethod);
                if (false == resultClassName.isEmpty())
                {
                    this->matchedClassName = resultClassName;
                }
                else
                {
                    this->isProcessing = false;
                    return;
                }
            }
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

