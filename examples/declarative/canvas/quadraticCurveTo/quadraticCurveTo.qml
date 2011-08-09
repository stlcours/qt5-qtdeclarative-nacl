import QtQuick 2.0
import "../contents"
Item {
  id:container
  width:360
  height:600

  Column {
    spacing:5
    anchors.fill:parent
    Text { font.pointSize:25; text:"Quadratic Curve"; anchors.horizontalCenter:parent.horizontalCenter}

    Canvas {
      id:canvas
      width:360
      height:360
      property string strokeStyle:"steelblue"
      property string fillStyle:"yellow"
      property int lineWidth:lineWidthCtrl.value
      property bool fill:true
      property bool stroke:true
      property real alpha:alphaCtrl.value
      property real scaleX : scaleXCtrl.value
      property real scaleY : scaleYCtrl.value
      property real rotate : rotateCtrl.value
      smooth:true
      renderTarget:Canvas.Image
      threadRendering:false

      onLineWidthChanged:requestPaint();
      onFillChanged:requestPaint();
      onStrokeChanged:requestPaint();
      onAlphaChanged:requestPaint();
      onScaleXChanged:requestPaint();
      onScaleYChanged:requestPaint();
      onRotateChanged:requestPaint();
      Behavior on scaleX { SpringAnimation { spring: 2; damping: 0.2; loops:Animation.Infinite } }
      Behavior on scaleY { SpringAnimation { spring: 2; damping: 0.2; loops:Animation.Infinite} }
      Behavior on rotate { SpringAnimation { spring: 2; damping: 0.2; loops:Animation.Infinite} }

      onPaint: {
          var ctx = canvas.getContext('2d');
          ctx.reset();
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          ctx.globalAlpha = canvas.alpha;
          ctx.strokeStyle = canvas.strokeStyle;
          ctx.fillStyle = canvas.fillStyle;
          ctx.lineWidth = canvas.lineWidth;
          ctx.scale(canvas.scaleX, canvas.scaleY);
          ctx.rotate(canvas.rotate);
          ctx.beginPath();
          ctx.moveTo(75,25);
          ctx.quadraticCurveTo(25,25,25,62.5);
          ctx.quadraticCurveTo(25,100,50,100);
          ctx.quadraticCurveTo(50,120,30,125);
          ctx.quadraticCurveTo(60,120,65,100);
          ctx.quadraticCurveTo(125,100,125,62.5);
          ctx.quadraticCurveTo(125,25,75,25);
          ctx.closePath();
          if (canvas.fill)
             ctx.fill();
          if (canvas.stroke)
             ctx.stroke();


          ctx.fillStyle="green";
          ctx.font = "Bold 15px";

         // ctx.fillText("QML酷毙了", 30, 60); //BUG(in Linux):can't show multiple Chinese characters correctly.
          ctx.fillText("QML酷   毙   了   !", 35, 65);
      }
    }

    Rectangle {
        id:controls
        width:360
        height:160
        Column {
          spacing:3
          Slider {id:lineWidthCtrl; width:300; height:30; min:1; max:10; init:2; name:"Line width"}
          Slider {id:scaleXCtrl; width:300; height:30; min:0.1; max:10; init:1; name:"ScaleX"}
          Slider {id:scaleYCtrl; width:300; height:30; min:0.1; max:10; init:1; name:"ScaleY"}
          Slider {id:rotateCtrl; width:300; height:30; min:0; max:Math.PI*2; init:0; name:"Rotate"}
          Slider {id:alphaCtrl; width:300; height:30; min:0; max:1; init:1; name:"Alpha"}
        }
    }
  }
}
