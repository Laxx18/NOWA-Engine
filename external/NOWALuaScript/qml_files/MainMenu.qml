import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

import NOWALuaScript

MenuBar
{
    id: mainMenu;
    focus: true;

    Component.onCompleted:
    {
        mainMenu.forceActiveFocus();
    }

    Rectangle
    {
        anchors.fill: parent;
        color: "darkslategrey";
    }

    FileDialog
    {
       id: fileOpenDialog;
       currentFolder: Qt.resolvedUrl("../../media/projects");

       nameFilters: ["Lua files (*.lua)"];

       onAccepted:
       {
           var selectedFilePath = fileOpenDialog.selectedFile.toString();

           if (Qt.platform.os === "windows")
           {
               selectedFilePath = selectedFilePath.replace("file:///", "");
           }
           else
           {
               selectedFilePath = selectedFilePath.replace("file://", "/");
           }

           NOWALuaEditorModel.requestAddLuaScript(selectedFilePath);
       }
    }

    FileDialog
    {
       id: apiFileOpenDialog;
       currentFolder: Qt.resolvedUrl("../bin/Release");

       nameFilters: ["Lua Api File (*.lua)"];

       onAccepted:
       {
           var selectedFilePath = apiFileOpenDialog.selectedFile.toString();

           if (Qt.platform.os === "windows")
           {
               selectedFilePath = selectedFilePath.replace("file:///", "");
           }
           else
           {
               selectedFilePath = selectedFilePath.replace("file://", "/");
           }
           // false -> parseSilent: Give feedback if it could be parsed
           LuaScriptQmlAdapter.requestSetLuaApi(selectedFilePath, false);
       }
    }

    // Message dialog for unsaved changes prompt
    MessageDialog
    {
        id: messageDialog;
        visible: false;
        // Prompt user to save changes
        text: "Lua Api parse result";
        informativeText: "";
        buttons: MessageDialog.Ok;

        // icon: StandardIcon.Warning;
        onAccepted:
        {
            messageDialog.visible = false;
        }
    }

    Connections
    {
        target: LuaScriptQmlAdapter;

        function onSignal_luaApiPreparationResult(parseSilent, success, message)
        {
            if (!parseSilent)
            {
                messageDialog.visible = true;
                messageDialog.informativeText = message;
            }
        }
    }

    Menu
    {
        title: qsTr("File");

        Action
        {
            text: qsTr("Open...");
            shortcut: "Ctrl+O";
            icon.name: "document-open";  // Common icon for "Open"
            onTriggered: fileOpenDialog.open();
        }

        Action
        {
            text: qsTr("Save");
            shortcut: "Ctrl+S";
            enabled: NOWALuaEditorModel.hasChanges;
            icon.name: "document-save";  // Common icon for "Save"
            onTriggered: NOWALuaEditorModel.requestSaveLuaScript();
        }

        Action
        {
            text: qsTr("Exit");
            shortcut: "Ctrl+Q";
            icon.name: "application-exit";  // Common icon for "Exit"
            onTriggered: Qt.quit();
        }
    }

    Menu
    {
        title: qsTr("Edit");

        Action
        {
            text: qsTr("Undo");
            shortcut: "Ctrl+Z";
            icon.name: "edit-undo";

            onTriggered:
            {
                NOWALuaEditorModel.undo();
            }
        }

        Action
        {
            text: qsTr("Redo");
            shortcut: "Ctrl+Y";
            icon.name: "edit-redo";

            onTriggered:
            {
                NOWALuaEditorModel.redo();
            }
        }

        Action
        {
            text: qsTr("Copy");
            shortcut: StandardKey.Copy;
            icon.name: "edit-copy";
            onTriggered: console.log("Copy action triggered");
        }

        Action
        {
            text: qsTr("Paste");
            shortcut: StandardKey.Paste;
            icon.name: "edit-paste";
            onTriggered: console.log("Paste action triggered");
        }

        Action
        {
            text: qsTr("Cut");
            shortcut: StandardKey.Cut;
            icon.name: "edit-cut";
            onTriggered: console.log("Cut action triggered");
        }

        Action
        {
            text: qsTr("Select All");
            shortcut: StandardKey.SelectAll;
            icon.name: "edit-select-all";  // If available, for "Select All"
            onTriggered: console.log("Select All action triggered");
        }

        Action
        {
            text: qsTr("Shift Lines");
            icon.name: "Tab";
            onTriggered: NOWALuaEditorModel.addTabToSelection();
        }

        Action
        {
            text: qsTr("Unshift Lines");
            icon.name: "text-comment";  // Might need to check availability
            onTriggered: NOWALuaEditorModel.removeTabFromSelection();
        }

        Action
        {
            text: qsTr("Comment");
            icon.name: "text-comment";  // Might need to check availability
            onTriggered: NOWALuaEditorModel.commentLines();
        }

        Action
        {
            text: qsTr("Uncomment");
            icon.name: "text-comment";  // Might need to check availability
            onTriggered: NOWALuaEditorModel.unCommentLines();
        }
    }

    Menu
    {
        title: qsTr("Lua");

        Action
        {
            text: qsTr("Run Script");
            icon.name: "system-run";  // Common icon for "Run"
            onTriggered: console.log("Run Lua script triggered");
        }

        Action
        {
            text: qsTr("Open Lua Script Folder");
            shortcut: "Ctrl+L";
            icon.name: "folder-open";  // Common icon for opening folders
            onTriggered: NOWALuaEditorModel.openProjectFolder();
        }

        Action
        {
            text: qsTr("Open Lua Api File");
            // icon.name: "document-save";  // Common icon for "Save"
            onTriggered: apiFileOpenDialog.open();
        }
    }

    Menu
    {
        title: qsTr("Help");

        Action
        {
            text: qsTr("About");
            icon.name: "help-about";  // Common icon for "About"

            onTriggered: console.log("About Lua script editor triggered")
        }
    }
}
