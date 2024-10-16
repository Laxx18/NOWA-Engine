import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

import NOWALuaScript

ApplicationWindow
{
    visible: true;
    minimumWidth: 800;
    minimumHeight: 800;
    title: "NOWALuaScript";

    flags: Qt.Window | Qt.WindowFullscreenButtonHint
    visibility: "Maximized"

    Material.theme: Material.Yellow;
    Material.primary: Material.BlueGrey;
    Material.accent: Material.LightGreen;
    Material.foreground: "#FFFFFF";
    Material.background: "#1E1E1E";

    menuBar: MainMenu
    {

    }

    header: ToolBar
    {
        Rectangle
        {
            anchors.fill: parent;
            color: "darkslategrey";
        }

        TabBar
        {
            id: tabBar;

            width: parent.width;

            onCurrentIndexChanged:
            {
               NOWALuaEditorModel.currentIndex = currentIndex;
               // Get the LuaEditor for the current index
               let luaEditor = luaEditorContainer.itemAt(currentIndex);
               if (luaEditor)
               {
                   luaEditor.checkSyntax();
               }
            }

            Repeater
            {
                model: NOWALuaEditorModel;

                delegate: TabButton
                {
                    text: model.title;
                    font.bold: true;
                    font.pixelSize: 16;

                    onClicked: tabBar.currentIndex = index;

                    Button
                    {
                        text: "x";
                        font.pixelSize: 14;
                        anchors.right: parent.right;
                        anchors.rightMargin: 8;
                        anchors.bottomMargin: 4;

                        onClicked:
                        {
                            closeTab(index);
                        }
                    }
                }
            }

            // Button to add new LuaEditor tab
            // TabButton
            // {
            //     text: "+";
            //     font.pixelSize: 18;
            //     onClicked:
            //     {
            //         if (NOWALuaEditorModel.count() < 10) // Limit to 10 editors
            //         {
            //             // TODO Via menu and fileopendialog set the filepath name and what about a caption?
            //             NOWALuaEditorModel.requestAddLuaScript("New Lua Script" + (NOWALuaEditorModel.count() + 1))
            //             tabBar.openTabs++;
            //         }
            //     }
            // }
        }
    }

    footer: DetailsArea
    {
        // filePathName: NOWALuaEditorModel.getLuaScript(tabBar.currentIndex).filePathName;
    }

    StackLayout
    {
        // NOTE: Do not change this names, as they are used for object matching in C++
        id: luaEditorContainer;
        objectName: "luaEditorContainer";

        width: parent.width;
        currentIndex: tabBar.currentIndex;

        // Repeater
        // {
        //     // NOTE: Do not change this names, as they are used for object matching in C++
        //     id: luaEditorRepeater;
        //     objectName: "luaEditorRepeater";

        //     model: NOWALuaEditorModel;

            // delegate: LuaEditor
            // {
            //     id: luaEditorDelegate;
            //     filePathName: NOWALuaEditorModel.getLuaScript(index).filePathName;

            //     onHasChangesChanged:
            //     {
            //         tabBar.unsavedChanges[index] = true;
            //     }
            // }
       // }
    }

    // Function to close the tab
    function closeTab(index)
    {
        // if (tabBar.unsavedChanges[index])
        if (NOWALuaEditorModel.hasChanges)
        {
            messageDialog.visible = true;
        }
        else
        {
            // Close the tab without confirmation
            removeTab(index);
        }
    }

    // Remove tab and corresponding LuaEditor
    function removeTab(index)
    {
        NOWALuaEditorModel.requestRemoveLuaScript(index);
    }

    // Message dialog for unsaved changes prompt
    MessageDialog
    {
        id: messageDialog;
        visible: false;
        // Prompt user to save changes
        text: "Unsaved Changes";
        informativeText: "You have unsaved changes. Do you really want to close this tab?";
        buttons: MessageDialog.Yes | MessageDialog.No;

        // icon: StandardIcon.Warning;
        onAccepted:
        {
            // Close the tab if accepted
            removeTab(tabBar.currentIndex);
        }
    }

    Connections
    {
        target: LuaScriptQmlAdapter;

        function onSignal_changeTab(newTabIndex)
        {
            tabBar.currentIndex = newTabIndex; // Change the current tab index
        }
    }
}
