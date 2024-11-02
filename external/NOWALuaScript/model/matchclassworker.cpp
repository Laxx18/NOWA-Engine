#include "matchclassworker.h"
#include "luaeditormodelitem.h"
#include "apimodel.h"

#include <QRegularExpression>

MatchClassWorker::MatchClassWorker(LuaEditorModelItem* luaEditorModelItem, bool forConstant, const QString& currentText,  const QString& textAfterKeyword, int cursorPosition, int mouseX, int mouseY)
    : luaEditorModelItem(luaEditorModelItem),
    currentText(currentText),
    forConstant(forConstant),
    typedAfterKeyword(textAfterKeyword),
    cursorPosition(cursorPosition),
    mouseX(mouseX),
    mouseY(mouseY),
    isProcessing(false),
    isStopped(true)
{

}

void MatchClassWorker::setParameters(bool forConstant, const QString& currentText, const QString& textAfterKeyword, int cursorPos, int mouseX, int mouseY)
{
    this->forConstant = forConstant;
    this->currentText = currentText;
    this->typedAfterKeyword = textAfterKeyword;
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

    if (true == this->forConstant)
    {
        if (!ApiModel::instance()->getIsIntellisenseShown() && this->typedAfterKeyword.isEmpty())
        {
            // Extract text from the start up to the cursor position
            QString textBeforeCursor = this->currentText.left(this->cursorPosition);

            // Check if we should stop before processing further
            if (this->isStopped)
            {
                this->isProcessing = false; // Reset the flag before exiting
                return; // Exit if stopping
            }

            bool validIdentifier = false;
            QString matchedIdentifier = this->luaEditorModelItem->extractClassBeforeDot(textBeforeCursor, this->cursorPosition);

            if (false == matchedIdentifier.isEmpty() && true == ApiModel::instance()->isValidClassName(matchedIdentifier))
            {
                this->matchedClassName = matchedIdentifier;
                validIdentifier = true;
            }
            else
            {
                // Nothing matched, so maybe the dot is called for a function, not for a class. Handle that case

                this->handleMethods();

                matchedIdentifier = this->matchedClassName;
                if (false == matchedIdentifier.isEmpty() && true == ApiModel::instance()->isValidClassName(matchedIdentifier))
                {
                    validIdentifier = true;
                }
            }

            if (true == validIdentifier)
            {
                this->handleConstantsResults();
                return;
            }
            else
            {
                this->isProcessing = false;
                return;
            }
        }

        // Check if we should stop before processing matched methods
        if (!this->typedAfterKeyword.isEmpty())
        {
            this->handleConstantsResults();
            return;
        }
    }

    QString matchedMethodName;

    // Only re-match variables if the menu is not shown and user is not typing something after colon, because else the auto completion would not work, because each time variables would be re-detected
    // But user typed already the next expression, which would no more match!
    if (!ApiModel::instance()->getIsIntellisenseShown() && this->typedAfterKeyword.isEmpty())
    {
        if (false == this->forConstant)
        {
            matchedMethodName = this->handleMethods();

            if (false == this->matchedClassName.isEmpty())
            {
                Q_EMIT signal_deliverData(this->matchedClassName, matchedMethodName, this->typedAfterKeyword, this->cursorPosition, this->mouseX, this->mouseY);
            }

            // Check if we should stop before triggering the menu
            if (this->isStopped)
            {
                this->isProcessing = false; // Reset the flag before exiting
                return; // Exit if stopping
            }

            // Now you can trigger a method that opens the context menu in QML with the methods
            ApiModel::instance()->showIntelliSenseMenu(false, this->matchedClassName, mouseX, mouseY);
            return;
        }
        else
        {
            this->isProcessing = false;
            return;
        }
    }

    // Check if we should stop before processing matched methods
    if (!this->typedAfterKeyword.isEmpty() && false == this->forConstant)
    {
        // Ensure we check again for stop condition
        if (this->isStopped)
        {
            this->isProcessing = false; // Reset the flag before exiting
            return; // Exit if stopping
        }

        ApiModel::instance()->processMatchedMethodsForSelectedClass(this->matchedClassName, this->typedAfterKeyword);
        this->typedAfterKeyword.clear();

        // Emit signal directly since the data structure has already been updated
        Q_EMIT ApiModel::instance()->signal_showIntelliSenseMenu(false, mouseX, mouseY);
    }

    // Reset the processing flag before exiting
    this->isProcessing = false;
}

QString MatchClassWorker::handleMethods(void)
{
    QString matchedMethodName;

    QRegularExpression trimmer(R"(^[^\w]+)");

    this->luaEditorModelItem->detectVariables();

    // Extract text from the start up to the cursor position
    QString textBeforeCursor = this->currentText.left(this->cursorPosition);

    // Check if we should stop before processing further
    if (this->isStopped)
    {
        this->isProcessing = false; // Reset the flag before exiting
        return matchedMethodName; // Exit if stopping
    }

    QString matchedPostIdentifier = this->luaEditorModelItem->extractWordBeforeColon(textBeforeCursor, this->cursorPosition);
    QString matchedPostMethodName = this->luaEditorModelItem->extractMethodBeforeColon(textBeforeCursor, this->cursorPosition);

    int lastNewlineIndex = textBeforeCursor.lastIndexOf('\n', this->cursorPosition - 1);

    // Special case:
    // physicsActiveComponent:applyForce(Vector3.UNIT_Y * 200);
    // typed:
    // physicsActiveComponent:applyForce(Vector3: * 200);
    // Nothing should match! because Vector3 only constants can be listed
    // Note, there are cases, which are un-matchable, without corrupting other matches like:
    // if (a == Vector3) then
    QString matchedCurrentIdentifier = this->luaEditorModelItem->extractClassBeforeDot(textBeforeCursor, this->cursorPosition);
    if (true == ApiModel::instance()->isValidClassName(matchedCurrentIdentifier) && matchedPostIdentifier.contains("("))
    {
        return "";
    }

    // Remove any non-identifier characters from the beginning of the strings
    matchedPostIdentifier.remove(trimmer);
    matchedPostMethodName.remove(trimmer);

    // Check for empty matched class
    if (matchedPostIdentifier.isEmpty())
    {
        this->isProcessing = false; // Reset the flag before exiting
        return matchedMethodName; // Exit if no matched class
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
                    return matchedMethodName;
                }
            }
        }
        else
        {
            // Nothing found, type unknown
            this->matchedClassName.clear();
        }
    }
    return matchedMethodName;
}

void MatchClassWorker::handleConstantsResults(void)
{
    // Ensure we check again for stop condition
    if (this->isStopped)
    {
        this->isProcessing = false; // Reset the flag before exiting
        return; // Exit if stopping
    }

    // Set the selected class name in ApiModel for constant lookup
    ApiModel::instance()->setSelectedClassName(this->matchedClassName);

    ApiModel::instance()->processMatchedConstantsForSelectedClass(this->matchedClassName, this->typedAfterKeyword);
    this->typedAfterKeyword.clear();

    // Only show intellisense, if not shown already and if nothing has been typed so far to list all constants
    // Note: processMatchedConstantsForSelectedClass makes the auto completion and internally a signal is send to qml to show the shrinked list, so showIntelliSenseMenu must not be called, because it would overwrite the shrinked list
    // with all constants
    if (!ApiModel::instance()->getIsIntellisenseShown() && this->typedAfterKeyword.isEmpty())
    {
        ApiModel::instance()->showIntelliSenseMenu(true, this->matchedClassName, mouseX, mouseY);
    }
    this->isProcessing = false;
}
