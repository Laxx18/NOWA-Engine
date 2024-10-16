import QtQuick
import QtQuick.Controls

import QtQuick.Controls.Material

import NOWALuaScript

Rectangle
{
    id: root

    width: parent.width;
    height: 40;
    color: "darkslategrey";

    // property string filePathName;

    Flickable
    {
        id: flickable;
        width: parent.width;
        height: parent.height;
        // anchors.fill: parent;

        contentWidth: detailsText.contentWidth;  // Content width is equal to the width of the TextEdit
        contentHeight: detailsText.contentHeight;  // Content height is equal to the height of the TextEdit
        clip: true;  // Clip content that exceeds the viewable area
        // Dangerous!
        // interactive: false; // Prevent flickable from capturing input events

        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.DragAndOvershootBounds

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

        ScrollBar.vertical: ScrollBar
        {
            width: 20;
            x: flickable.width - 12;
            // policy: ScrollBar.AlwaysOn;
        }

        TextEdit
        {
            id: detailsText;

            padding: 8;

            textFormat: TextEdit.PlainText;
            wrapMode: TextEdit.Wrap;
            focus: true;
            readOnly: true;
            activeFocusOnPress: true; // Enable focus on press for text selection
            selectByMouse: true;
            persistentSelection: true;

            selectedTextColor: "black";
            selectionColor: "cyan";

            onCursorRectangleChanged:
            {
                flickable.ensureVisible(cursorRectangle);
            }

            Connections
            {
                target: LuaScriptQmlAdapter;

                function onSignal_syntaxCheckResult(filePathName, valid, line, start, end, message)
                {
                    // if (filePathName === root.filePathName)
                    // {
                        if (!valid)
                        {
                            detailsText.text = "Error at line: " + line + " Details: " + message;
                        }
                        else
                        {
                            detailsText.text = message;
                        }
                    // }
                }
            }
        }
    }
}
