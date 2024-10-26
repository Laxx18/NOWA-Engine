import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

import NOWALuaScript

Rectangle
{
    id: root;
    color: "white";
    visible: false;
    width: 400; // Set a fixed width for the menu
    height: 400; // classNameRect.height + descriptionRect.height + inheritsRect.height + detailsArea.height + contentWrapper.height;

    QtObject
    {
        id: p;

        property int itemHeight: 24;
        property int textMargin: 12;
        property string backgroundColor: "darkslategrey";
        property string backgroundHoverColor: "steelblue";
        property string backgroundClickColor: "darkblue";
        property string textColor: "white";
        property string borderColor: "grey";
    }

    Column
    {
        anchors.fill: parent

        // Header section - fixed items
        Rectangle
        {
            id: classNameRect;
            height: p.itemHeight;
            width: root.width;
            color: p.backgroundColor;
            border.color: p.borderColor;

            Row
            {
                anchors.fill: parent;
                anchors.margins: 4;
                spacing: 4;

                Text
                {
                    id: baseText;
                    color: p.textColor;
                    text: "Class Name: " + NOWAApiModel.selectedClassName;
                    verticalAlignment: Text.AlignVCenter;
                    width: root.width - 12;
                    wrapMode: Text.NoWrap;
                }
            }
        }

        Rectangle
        {
            id: descriptionRect;
            height: p.itemHeight * 2;
            width: root.width;
            color: p.backgroundColor;
            border.color: p.borderColor;

            Row
            {
                anchors.fill: parent;
                anchors.margins: 4;
                spacing: 4;

                Text
                {
                    id: baseText2;
                    color: p.textColor;
                    text: "Description: " + NOWAApiModel.classDescription;
                    verticalAlignment: Text.AlignVCenter;
                    width: root.width - 12;
                    wrapMode: Text.Wrap;
                }
            }
        }

        Rectangle
        {
            id: inheritsRect;
            height: p.itemHeight;
            width: root.width;
            color: p.backgroundColor;
            border.color: p.borderColor;

            Row
            {
                anchors.fill: parent;
                anchors.margins: 4;
                spacing: 4;

                Text
                {
                    id: baseText3;
                    color: p.textColor;
                    text: "Inherits: " + NOWAApiModel.classInherits;
                    verticalAlignment: Text.AlignVCenter;
                    width: root.width - 12;
                    wrapMode: Text.NoWrap;
                }
            }
        }

        Rectangle
        {
            id: detailsArea;
            height: Math.max(details.height, 100);
            width: root.width;
            color: p.backgroundColor;
            border.color: p.borderColor;

            Row
            {
                anchors.fill: parent;
                anchors.margins: 4;
                spacing: 4;

                Text
                {
                    id: details;
                    color: p.textColor;
                    text: "Details: ";
                    verticalAlignment: Text.AlignVCenter;
                    width: root.width - 12;
                    wrapMode: Text.Wrap;
                }
            }
        }

        // Divider
        Rectangle
        {
            width: root.width;
            height: 2;
            color: "white";
            opacity: 0.5;
        }

        Flickable
        {
            id: flickable;
            width: root.width;
            height: root.height - contentWrapper.height - 5 * p.itemHeight // Leave space for header
            clip: true; // Clip content that exceeds the viewable area
            contentWidth: contentWrapper.width
            contentHeight: repeaterContent.height

            ScrollBar.vertical: ScrollBar
            {
                id: verticalScrollBar;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                hoverEnabled: true;
                active: pressed || hovered;
                policy: flickable.contentHeight > flickable.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff;
            }

            ScrollBar.horizontal: ScrollBar
            {
                height: 20;
                anchors.bottom: parent.bottom;

                // Make sure the scrollbar is positioned correctly
                anchors.left: flickable.left;
                anchors.right: flickable.right;
                hoverEnabled: true;
                active: pressed || hovered;
                policy: flickable.contentWidth > flickable.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff;

            }

            Column
            {
                id: contentWrapper;
                width: 0; // Will be calculated (max function)
                height: 0; // Will be calculated (max function)

                // Repeater for class methods
                Repeater
                {
                    id: repeaterContent;
                    model: NOWAApiModel.methodsForSelectedClass;

                    delegate: Rectangle
                    {
                        id: itemContainer;

                        height: p.itemHeight;
                        width: contentWrapper.width;

                        property int rowTextWidth: internalText.contentWidth;
                        property int rowTextHeight: p.itemHeight;

                        color: itemMouseArea.pressed ? p.backgroundClickColor : (itemMouseArea.containsMouse ? p.backgroundHoverColor : p.backgroundColor);
                        border.color: itemMouseArea.pressed ? p.backgroundHoverColor : p.borderColor;

                        property int clickCount: 0;
                        property real lastClickTime: 0;
                        property real doubleClickThreshold: 300; // Time in milliseconds for double-click detection

                        Row
                        {
                            anchors.fill: parent;
                            anchors.leftMargin: 6;
                            spacing: 4;

                            Text
                            {
                                id: internalText;
                                color: p.textColor;
                                text: modelData.returns + " " + modelData.name + modelData.args;
                                verticalAlignment: Text.AlignVCenter;
                            }
                        }

                        MouseArea
                        {
                            id: itemMouseArea;

                            anchors.fill: parent;
                            hoverEnabled: true;

                            onClicked:
                            {
                                let currentTime = Date.now();
                                // Check if this click is within the double click threshold
                                if (currentTime - lastClickTime <= doubleClickThreshold)
                                {
                                    clickCount = 0; // Reset click count
                                    // Handle double-click action here
                                    // For example, you can open a detailed view or perform an action.
                                    root.visible = false;
                                    NOWAApiModel.isShown = false;
                                    NOWALuaEditorModel.sendTextToEditor(modelData.name);
                                }
                                else
                                {
                                    clickCount++;
                                    // Single click logic, e.g., update details text
                                    hoverTimer.restart();
                                }

                                // Update last click time
                                lastClickTime = currentTime;
                            }

                            Timer
                            {
                                id: hoverTimer
                                interval: 300 // 300 ms delay
                                repeat: false

                                onTriggered:
                                {
                                    details.text = "Details: " + modelData.description + "\n" + modelData.returns + " " + modelData.name + modelData.args;
                                }
                            }

                            // Track hover and press state for visual feedback
                            onPressed:
                            {
                                parent.color = p.backgroundClickColor;
                            }

                            onReleased:
                            {
                                parent.color = itemMouseArea.containsMouse ? p.backgroundHoverColor : p.backgroundColor;
                                hoverTimer.stop();
                            }

                            // Set the hovered state dynamically
                            onEntered:
                            {
                                parent.color = p.backgroundHoverColor;
                            }

                            onExited:
                            {
                                parent.color = p.backgroundColor;
                            }
                        }
                    }
                }
            }
        }
    }

    function open(x, y)
    {
        calculateMaxWidth();

        let pHeight = parent.height;
        let rHeight = root.height;
        // Parent is the main application
        const availableSpaceBelow = pHeight - y;
        if (availableSpaceBelow < rHeight)
        {
            root.y = y - rHeight - 100; // Position above
        }
        else
        {
            root.y = y; // Position below
        }

        let pY = root.y;
        root.x = Math.min(x, parent.width - root.width); // Ensure it stays within width
        root.visible = true;

        NOWAApiModel.isShown = true;
    }

    function calculateMaxWidth()
    {
        var maxWidth = 0;
        var maxHeight = 0;
        var items = contentWrapper.children;

        for (var i = 0; i < items.length; i++)
        {
            if (items[i].hasOwnProperty("rowTextWidth"))
            {
                maxWidth = Math.max(maxWidth, items[i].rowTextWidth);
            }

            if (items[i].hasOwnProperty("rowTextHeight"))
            {
                maxHeight += items[i].rowTextHeight;
            }
        }

        var finalWidth = maxWidth + p.textMargin;
        if (finalWidth < root.width)
        {
            finalWidth = root.width;
        }

        var finalHeight = maxHeight + p.textMargin;

        contentWrapper.width = finalWidth;
        flickable.contentHeight = finalHeight;
    }
}
