#include "backend/luascript.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QStack>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

LuaScript::LuaScript(const QString& filePathName, QObject* parent)
    : QObject(parent),
      filePathName(filePathName)
{
    this->lua = lua_open();  // Create a new Lua state
    luaL_openlibs(this->lua);     // Load standard Lua libraries
}

LuaScript::~LuaScript()
{
    if (nullptr != this->lua)
    {
        lua_close(this->lua);  // Close the Lua interpreter when done
    }
}

bool LuaScript::loadScriptFromFile(const QString& filePathName)
{
    QFile file(filePathName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file:" << filePathName;
        return false;
    }

    QString luaCode = file.readAll();
    file.close();

    // this->checkSyntax(luaCode);  // Perform a syntax check when loading the script

    // Parse the Lua script and update internal data structures for intellisense
    qDebug() << "Lua script loaded from file:" << filePathName;

    // Must be replaced, else addTab, indent does not work correctly, as it messes up with \t
    luaCode.replace("\t", "    ");

    this->content = luaCode;

    Q_EMIT signal_luaScriptLoaded(luaCode);

    return true;
}

QString LuaScript::getFilePathName() const
{
    return this->filePathName;
}

QString LuaScript::getContent(void) const
{
    return this->content;
}

#if 0
bool LuaScript::checkSyntax(const QString& luaCode)
{
    int result = luaL_loadstring(this->lua, luaCode.toStdString().c_str());

    int errorLine = 0;
    QString errorMessage;

    if (result == /*LUA_OK*/ 0)
    {
        Q_EMIT signal_syntaxCheckFinished(true, errorLine, errorMessage);
        return true;  // No syntax errors
    }
    else
    {
        const char* errorMsg = lua_tostring(this->lua, -1);
        QString errorStr = QString(errorMsg);
        QRegularExpression regex(R"(\[string\s\".*\"\]:(\d+):\s(.*))");
        QRegularExpressionMatch match = regex.match(errorStr);
        if (match.hasMatch())
        {
            errorLine = match.captured(1).toInt();  // Capture the line number
            errorMessage = match.captured(2);       // Capture the error message
        }
        Q_EMIT signal_syntaxCheckFinished(false, errorLine, errorMessage);
        return false;  // Syntax error found
    }
}
#endif

#include <QRegularExpression>

void LuaScript::checkSyntax(const QString& luaCode)
{
    // Attempt to load the Lua code
    int result = luaL_loadstring(this->lua, luaCode.toStdString().c_str());

    if (result != 0)
    {
        // Get the error message from the Lua stack
        const char* errorMsg = lua_tostring(this->lua, -1);
        QString errorString = QString::fromUtf8(errorMsg);

        // Extract line number and error message using QRegularExpression
        int errorLine = -1;

        // Not necessary and expensive
#if 0
        // Step 1: Extract keywords from error message for matching
        // Look for the part of the error message that might be relevant to the code
        QStringList keywords;
        QRegularExpression quoteRegex("\"([^\"]+)\"");  // Match content inside double quotes
        QRegularExpressionMatchIterator quoteIterator = quoteRegex.globalMatch(errorString);

        while (quoteIterator.hasNext())
        {
            QRegularExpressionMatch quoteMatch = quoteIterator.next();
            QString keyword = quoteMatch.captured(1).trimmed(); // Extract the matched group
            if (!keyword.isEmpty())
            {
                keywords.append(keyword); // Add to keywords list
            }
        }

        QStringList lines = luaCode.split('\n');
        int bestMatchLine = -1;
        int longestMatchLength = 0;  // Track the length of the longest match

        for (int i = 0; i < lines.size(); ++i)
        {
            // Check if any keyword matches the line (case-insensitive)
            for (const QString& keyword : keywords)
            {
                if (lines[i].contains(keyword, Qt::CaseInsensitive))
                {
                    // If a match is found, consider this line for the error
                    int matchLength = keyword.length();
                    if (matchLength > longestMatchLength)
                    {
                        longestMatchLength = matchLength;
                        bestMatchLine = i;
                    }
                }
            }
        }

        // If we found a best match, set the errorLine
        if (bestMatchLine != -1)
        {
            errorLine = bestMatchLine + 1; // Set the error line based on the luaCode
            qDebug() << "Error line found in luaCode: " << errorLine;
        }
#endif

        // Step 2: Check for "file:line:message" format
        QRegularExpression errorRegex(".*:([0-9]+):");
        QRegularExpressionMatch match = errorRegex.match(errorString);

        if (match.hasMatch())
        {
            errorLine = match.captured(1).toInt();
            qDebug() << "Error line set from 'file:line:message' to: " << errorLine;
        }

        // Step 3: Check for "at line X" in the error message
        errorRegex.setPattern("at line ([0-9]+)");
        match = errorRegex.match(errorString);

        if (match.hasMatch())
        {
            // Found "at line X", set this as the error line
            errorLine = match.captured(1).toInt();
            qDebug() << "Error line set from 'at line X' to: " << errorLine;
        }

        if (-1 != errorLine)
        {
            Q_EMIT signal_syntaxCheckResult(false, errorLine, 0, 0, errorString);
        }
        else
        {
            // Emit the syntax check result
            Q_EMIT signal_syntaxCheckResult(true, -1, 0, 0, "");
        }

        lua_pop(this->lua, 1); // Pop the error message from the stack
        return;
    }
    else
    {
        Q_EMIT signal_syntaxCheckResult(true, -1, 0, 0, "");
    }

    // If the code loads correctly, execute it
    result = lua_pcall(this->lua, 0, LUA_MULTRET, 0);
}


bool LuaScript::saveLuaScript(const QString& content)
{
    bool success = false;

    // Write the lua content to a file
    QFile file(this->filePathName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << content;
        file.close();
        success = true;
    }
    else
    {
        QString message = "Failed to save GraphML file: \n" + filePathName + " Error: " + file.errorString();
        qDebug() << message;
        success = false;
    }

    return success;
}
