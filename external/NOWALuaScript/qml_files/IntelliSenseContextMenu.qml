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
        property bool forConstant: false;
        property int currentIndex: 0;  // Track selected index
    }

    Connections
    {
        target: LuaScriptQmlAdapter;

        // Handles relayed keys from LuaEditor, because normally this context menu gets no focus for key inputs
        function onSignal_relayKeyPress(key)
        {
            if (key === Qt.Key_Down)
            {
                p.currentIndex = Math.min(p.currentIndex + 1, repeaterContent.count - 1);
                keyHoverTimer.restart();
            }
            else if (key === Qt.Key_Up)
            {
                p.currentIndex = Math.max(p.currentIndex - 1, 0);
                keyHoverTimer.restart();
            }
            else if (key === Qt.Key_Tab && p.currentIndex >= 0)
            {
                var selectedIdentifier;
                if (!p.forConstant)
                {
                    selectedIdentifier = NOWAApiModel.methodsForSelectedClass[p.currentIndex];
                }
                else
                {
                    selectedIdentifier = NOWAApiModel.constantsForSelectedClass[p.currentIndex];
                }

                NOWALuaEditorModel.sendTextToEditor(selectedIdentifier.name);
            }
        }
    }

    Timer
    {
        id: keyHoverTimer;
        interval: 300; // 300 ms delay
        repeat: false;

        onTriggered:
        {
            var selectedIdentifier;
            var content = "";
            if (!p.forConstant)
            {
                selectedIdentifier = NOWAApiModel.methodsForSelectedClass[p.currentIndex];
                if (selectedMethod.description)
                {
                    content = "Details: " + selectedIdentifier.description + "\n" + selectedIdentifier.returns + " " + selectedIdentifier.name + selectedIdentifier.args;
                }
                else
                {
                    content = "Details: " + selectedIdentifier.returns + " " + selectedIdentifier.name + selectedIdentifier.args;
                }
            }
            else
            {
                selectedIdentifier = NOWAApiModel.constantsForSelectedClass[p.currentIndex];
                if (selectedMethod.description)
                {
                    content = "Details: Constant: " + selectedIdentifier.description + "\n" + selectedIdentifier.name;
                }
                else
                {
                    content = "Details: Constant: " + selectedIdentifier.name;
                }
            }



            details.text = content;
        }
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
                    id: classNameText;
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
                    id: descriptionText;
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
                    id: inheritsText;
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

                    // Sets a model either for constants or for methods
                    model: !p.forConstant ? NOWAApiModel.methodsForSelectedClass : NOWAApiModel.constantsForSelectedClass;

                    delegate: Rectangle
                    {
                        id: itemContainer;

                        height: p.itemHeight;
                        width: contentWrapper.width;

                        property int rowTextWidth: internalText.contentWidth;
                        property int rowTextHeight: p.itemHeight;

                        property bool isHovered: false;  // Track hover state
                        property bool isPressed: false;  // Track pressed state

                        color: isHovered || index === p.currentIndex || isPressed ? p.backgroundHoverColor : p.backgroundColor;
                        border.color: isPressed || isHovered ? p.backgroundHoverColor : p.borderColor;

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
                                textFormat: Text.RichText;
                                // text: modelData.returns + " " + modelData.name + modelData.args;
                                text:
                                {
                                    var nameText = modelData.name;
                                    var preMatch;
                                    var matchText;
                                    var postMatch;

                                    if (!p.forConstant)
                                    {
                                        if (modelData.startIndex >= 0 && modelData.endIndex > modelData.startIndex)
                                        {
                                            preMatch = nameText.substring(0, modelData.startIndex);
                                            matchText = nameText.substring(modelData.startIndex, modelData.endIndex + 1);
                                            postMatch = nameText.substring(modelData.endIndex + 1);
                                            // #FFDAB9 = Peach Puff
                                            nameText = preMatch + "<b><span style='color:#FFDAB9;'>" + matchText + "</span></b>" + postMatch;
                                        }
                                        return modelData.returns + " " + nameText + modelData.args;
                                    }
                                    else
                                    {
                                        if (modelData.startIndex >= 0 && modelData.endIndex > modelData.startIndex)
                                        {
                                            preMatch = nameText.substring(0, modelData.startIndex);
                                            matchText = nameText.substring(modelData.startIndex, modelData.endIndex + 1);
                                            postMatch = nameText.substring(modelData.endIndex + 1);
                                            // #FFDAB9 = Peach Puff
                                            nameText = preMatch + "<b><span style='color:#FFDAB9;'>" + matchText + "</span></b>" + postMatch;
                                        }
                                        return nameText;
                                    }
                                }

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
                                p.currentIndex = index;

                                let currentTime = Date.now();
                                // Check if this click is within the double click threshold
                                if (currentTime - itemContainer.lastClickTime <= doubleClickThreshold)
                                {
                                    clickCount = 0; // Reset click count
                                    // Handle double-click action here
                                    // For example, you can open a detailed view or perform an action.
                                    root.visible = false;
                                    NOWAApiModel.isIntellisenseShown = false;
                                    NOWALuaEditorModel.sendTextToEditor(modelData.name);
                                }
                                else
                                {
                                    clickCount++;
                                    // Single click logic, e.g., update details text
                                    hoverTimer.restart();
                                }

                                // Update last click time
                                itemContainer.lastClickTime = currentTime;
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
                                isPressed = true;
                            }

                            onReleased:
                            {
                                isPressed = false;
                                hoverTimer.stop();
                            }

                            // Set the hovered state dynamically
                            // onEntered:
                            // {
                            //     p.currentIndex = index;
                            //     isHovered = true;
                            // }

                            onExited:
                            {
                                isHovered = false;
                            }
                        }
                    }
                }
            }
        }
    }

    function open(forConstant, x, y)
    {
        p.forConstant = forConstant;
        p.currentIndex = 0;

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

        NOWAApiModel.isIntellisenseShown = true;
    }

    function close()
    {
        NOWAApiModel.isIntellisenseShown = false;
        root.visible = false;
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
