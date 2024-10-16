import QtQuick
import QtQuick.Controls
// import QtQuick.Controls.Material
// import QtQuick.Layouts

import NOWALuaScript

LuaEditorQml
{
    id: root;

    // Is done in C++
    // width: parent.width - 12;
    // height: 800;
    focus: true;

    Component.onCompleted:
    {

        console.debug("----> LuaEditorQml LOADED");
    }

    onVisibleChanged:
    {
        if (visible)
        {
            luaEditor.forceActiveFocus();
        }
    }

    property string textColor: "white";
    property bool hasChanges: false;

    property int leftPadding: 4;
    property int topPadding: 4;

    Rectangle
    {
        anchors.fill: parent
        color: "darkslategrey";
        border.color: "darkgrey";
        border.width: 2;
    }

    function checkSyntax()
    {
        LuaScriptQmlAdapter.checkSyntax(root.model.filePathName, luaEditor.originalText);
    }

    Flickable
    {
        id: flickable;
        width: parent.width;
        height: parent.height;
        // anchors.fill: parent;

        contentWidth: luaEditor.contentWidth;  // Content width is equal to the width of the TextEdit
        contentHeight: luaEditor.contentHeight;  // Content height is equal to the height of the TextEdit
        clip: true;  // Clip content that exceeds the viewable area
        // Dangerous!
        // interactive: false; // Prevent flickable from capturing input events

        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragAndOvershootBounds

        ScrollBar.vertical: ScrollBar
        {
            width: 20;
            x: flickable.width - 12;
            policy: ScrollBar.AlwaysOn;
        }

        // MouseArea
        // {
        //     anchors.fill: parent;
        //     propagateComposedEvents: true;

        //     onClicked:
        //     {
        //         // Ensure luaEditor receives focus on click
        //         luaEditor.forceActiveFocus();
        //     }
        // }

        // Synchronize scroll position
        function ensureVisible(r)
        {
             if (contentX >= r.x)
                 contentX = r.x;
             else if (contentX + width <= r.x + r.width)
                 contentX = r.x + r.width - width;
             if (contentY >= r.y)
                 contentY = r.y;
             else if (contentY+height <= r.y + r.height)
                 contentY = r.y + r.height - height;
        }

        Row
        {
            width: flickable.width;

            // Line Number View
            TextEdit
            {
                id: lineNumbersEdit;
                objectName: "lineNumbersEdit";

                leftPadding: root.leftPadding;
                topPadding: root.topPadding;

                readOnly: true;
                width: 50;
                wrapMode: TextEdit.NoWrap;
                color: textColor;
                font.family: luaEditor.font.family;
                font.pixelSize: luaEditor.font.pixelSize;

                // Keep line numbers updated
                Component.onCompleted: updateLineNumbers();
                onTextChanged: updateLineNumbers();

                function updateLineNumbers()
                {
                    var lineCount = luaEditor.text.split("\n").length;
                    var lineText = "";
                    for (var i = 1; i <= lineCount; i++)
                    {
                        lineText += i + "\n";
                    }
                    lineNumbersEdit.text = lineText;
                }

                onCursorRectangleChanged:
                {
                    flickable.ensureVisible(cursorRectangle);
                }
            }

            // Lua Code Editor
            TextEdit
            {
                id: luaEditor;
                objectName: "luaEditor";

                width: parent.width - 12;
                height: parent.height;
                leftPadding: root.leftPadding;
                topPadding: root.topPadding;

                textFormat: TextEdit.PlainText;
                wrapMode: TextEdit.Wrap;
                focus: true;
                activeFocusOnPress: true; // Enable focus on press for text selection
                color: textColor;
                selectByMouse: true;
                persistentSelection: true;
                selectedTextColor: "black";
                selectionColor: "cyan";

                property string originalText: "";
                property int errorLine: -1;

                onTextChanged:
                {
                    checkSyntaxTimer.restart();
                    luaEditor.originalText = luaEditor.text;
                    clearHighlight();
                    lineNumbersEdit.updateLineNumbers();  // Sync line numbers on text change
                    root.hasChanges = true;

                    // Sets new changes to model
                    root.model.content = luaEditor.text;
                }

                onCursorRectangleChanged:
                {
                    flickable.ensureVisible(cursorRectangle);
                }

                // MouseArea
                // {
                //     anchors.fill: parent;
                //     propagateComposedEvents: true;

                //     onClicked:
                //     {
                //         // Ensure luaEditor receives focus on click
                //         luaEditor.forceActiveFocus();
                //     }
                // }

                Connections
                {
                    target: LuaScriptQmlAdapter;

                    function onSignal_syntaxCheckResult(filePathName, valid, line, start, end, message)
                    {
                        if (filePathName === root.model.filePathName)
                        {
                            if (!valid)
                            {
                                luaEditor.errorLine = line;
                                highlightErrorLine(line);
                            }
                            else
                            {
                                luaEditor.errorLine = -1;
                                clearHighlight();
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle
    {
        id: errorHighlightRect;
        color: "red";
        height: 2;
        visible: false;
    }

    Timer
    {
        id: checkSyntaxTimer;
        interval: 2000;
        repeat: false;
        onTriggered:
        {
            LuaScriptQmlAdapter.checkSyntax(root.model.filePathName, luaEditor.originalText);
        }
    }

    FontMetrics
    {
        id: fontMetrics;
        font: luaEditor.font;
    }

    function highlightErrorLine(line)
    {
        errorHighlightRect.visible = true;

        var lineHeight = luaEditor.font.pixelSize * 1.25;
        var offsetY = (line - 1) * lineHeight;

        var textLine = luaEditor.text.split('\n')[line - 1];
        var boundingRect = fontMetrics.boundingRect(textLine);

        errorHighlightRect.width = boundingRect.width;
        errorHighlightRect.x = lineNumbersEdit.x + lineNumbersEdit.width + root.leftPadding;// Align with the TextEdit
        errorHighlightRect.y = offsetY + lineHeight - flickable.contentY + root.topPadding;  // Adjust for the flickable's scroll position + 5 = padding 4 + 1
    }

    // function highlightErrorLine(line)
    // {
    //     // Calculate the position of the line in the text
    //     let lines = luaEditor.text.split('\n');
    //     let start = 0;
    //     for (let i = 0; i < line - 1; i++)
    //     {
    //         start += lines[i].length + 1;  // +1 for the newline character
    //     }
    //     let end = start + lines[line - 1].length;

    //     // Apply highlight using selection
    //     luaEditor.select(start, end);
    //     luaEditor.selectedText.color = "red";
    // }

    // function highlightErrorWord(line, word)
    // {
    //     let lines = luaEditor.text.split('\n');
    //     let start = 0;
    //     for (let i = 0; i < line - 1; i++)
    //     {
    //         start += lines[i].length + 1;  // +1 for the newline character
    //     }

    //     // Find the word within the line
    //     let wordStart = lines[line - 1].indexOf(word);
    //     if (wordStart !== -1)
    //     {
    //         let startPos = start + wordStart;
    //         let endPos = startPos + word.length;

    //         // Apply highlight
    //         luaEditor.select(startPos, endPos);
    //         luaEditor.textCursor.selectText(startPos, endPos);
    //         luaEditor.textCursor.selectedText.color = "red";
    //         luaEditor.textCursor.selectedText.underline = TextEdit.UnderlineStyle.Wavy;  // Wavy underline
    //     }
    // }

    function clearHighlight()
    {
        errorHighlightRect.visible = false;
    }

    ListView
    {
        id: suggestionsList;
        width: parent.width;
        height: parent.height * 0.2;
        model: luaSuggestionsModel;

        visible: false;

        delegate: Item
        {
            width: parent.width;
            height: 30;

            Text
            {
                text: model.name;
                font.pixelSize: 18;
            }
        }

        ListModel
        {
            id: luaSuggestionsModel;
        }

        Connections
        {
            target: LuaScriptQmlAdapter;

            function onSignal_intellisenseReady(filePathName, suggestions)
            {
                if (filePathName === root.model.filePathName)
                {
                    luaSuggestionsModel.clear();
                    for (var i = 0; i < suggestions.length; i++)
                    {
                        luaSuggestionsModel.append({"name": suggestions[i]});
                    }
                }
            }
        }

        Connections
        {
            target: NOWALuaEditorModel;

            function onSignal_luaScriptAdded(filePathName, content)
            {
                if (filePathName === root.model.filePathName)
                {
                    luaEditor.text = content;
                }
            }
        }
    }
}
