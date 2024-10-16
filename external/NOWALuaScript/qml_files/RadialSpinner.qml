import QtQuick

Rectangle
{
    property real spinnerValue: 0;

    color: "transparent";

    function updateSpinner()
    {
        radialSpinner.requestPaint();
    }

    Canvas
    {
        id: radialSpinner;

        width: 100;
        height: 100;

        onPaint:
        {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // Draw background circle
            ctx.beginPath();
            ctx.arc(width / 2, height / 2, width / 2 - 5, 0, 2 * Math.PI);
            ctx.lineWidth = 10;
            ctx.strokeStyle = "#FFFFFF";
            ctx.stroke();

            // Draw progress arc
            var endAngle = 2 * Math.PI * (spinnerValue / 100);
            ctx.beginPath();
            ctx.arc(width / 2, height / 2, width / 2 - 5, 0, endAngle);
            ctx.lineWidth = 10;
            ctx.strokeStyle = "#dc143c";
            ctx.stroke();
        }
    }
}
