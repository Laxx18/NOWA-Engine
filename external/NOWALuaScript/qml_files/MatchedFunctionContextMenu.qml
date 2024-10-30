import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

import NOWALuaScript

Popup
{
    id: root;

    width: methodDetails.width + 20;
    height: Math.max(100, dataColumn.height + 20);

    QtObject
    {
        id: p;

        property string backgroundColor: "darkslategrey";
        property string textColor: "white";
        property string borderColor: "grey";
    }

    background: Rectangle
    {
        color: p.backgroundColor;
        border.color: p.borderColor;
        border.width: 2;
        radius: 5;
    }

    Column
    {
        id: dataColumn;
        spacing: 8;
        width: root.width;
        anchors.margins: 8;

        Text
        {
            id: methodDetails;
            color: p.textColor;
            textFormat: Text.RichText;
        }

        Text
        {
            id: methodDescription;
            color: p.textColor;
            textFormat: Text.RichText;
            width: methodDetails.width;
            wrapMode: Text.Wrap;
        }
    }

    function open(x, y)
    {
        let pHeight = parent.height;
        let rHeight = root.height;
        const availableSpaceBelow = pHeight - y;
        if (availableSpaceBelow < rHeight)
        {
            root.y = y - rHeight - 100;
        }
        else
        {
            root.y = y;
        }

        root.x = Math.min(x, parent.width - root.width);
        root.visible = true;

        NOWAApiModel.isMatchedFunctionShown = true;
    }

    function close()
    {
        methodDescription.text = "";
        methodDetails.text = "";
        root.visible = false;
        NOWAApiModel.isMatchedFunctionShown = false;
    }

    Connections
    {
        target: NOWAApiModel;

        // React to the signal for highlighting a function parameter
        function onSignal_highlightFunctionParameter(fullSignature, description, startIndex, endIndex)
        {
            // Set the description text
            methodDescription.text = "<b>Description: </b>" + description;

            // Format the `fullSignature` text to highlight parameter with bold tags
            let highlightedText = fullSignature.substring(0, startIndex) +
                                  "<b><span style='color:#FFDAB9;'>" + fullSignature.substring(startIndex, endIndex) + "</span></b>" +
                                  fullSignature.substring(endIndex);

            // Update `methodDetails` text with the formatted HTML content
            methodDetails.text = highlightedText;
        }

        function onSignal_noHighlightFunctionParameter()
        {
            close();
        }
    }
}
