function initNetworkGUI() {

    /******** Make nodes draggable ***********/

    $(".node").draggable({

        start: function (event, ui) {
            $(this).find('.nodeMenu').hide();
            
        },
        stop: function (event, ui) {
            $(this).find('.nodeMenu').css('display', '');
        },
        drag: function (event, ui) {
            //console.log('drag');
            $('.connectingLine').remove();
            drawConnections();
        }
    });


    /*****************************************/

    drawConnections();

}


/******** Draw arrows ***********/
function connect(div1, div2, extraClass) {
    var pos1 = div1.position();
    var pos2 = div2.position();
    // box1

    var x1 = pos1.left + div1.outerWidth() / 2;
    var y1 = pos1.top + div1.outerHeight() / 2;
    // box2
    var x2 = pos2.left + div2.outerWidth() / 2;
    var y2 = pos2.top + div2.outerHeight() / 2;

    // distance
    var length = Math.sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)));
    // center
    var cx = ((x1 + x2) / 2) - (length / 2);
    var cy = ((y1 + y2) / 2) - 0.5;
    // angle
    var angle = Math.atan2((y1 - y2), (x1 - x2)) * (180 / Math.PI);
    //arrowhead margin

    var halfHeight = div2.outerHeight() / 2;
    var halfWidth = div2.outerWidth() / 2;

    //var arrowmargin = Math.max(b / 2, a / 2);
    a = halfWidth * Math.tan(angle * Math.PI / 180);
    if (Math.abs(a) < halfHeight) {
        b = halfWidth;
    } else {
        a = halfHeight;
        b = halfHeight / Math.tan(angle * Math.PI / 180);
    }
    var c = Math.sqrt(a * a + b * b)
    var arrowmargin = c;

    var src = div1.prop?div1.prop('id'):'';
    var dst = div2.prop?div2.prop('id'):'';

    // make hr
    var htmlLine = "<div class='connectingLine " + extraClass + "' data-src='"+src+"' data-dst='"+dst+"' style='left:" + cx + "px; top:" + cy + "px; width:" + length + "px; -moz-transform:rotate(" + angle + "deg); -webkit-transform:rotate(" + angle + "deg); -o-transform:rotate(" + angle + "deg); -ms-transform:rotate(" + angle + "deg); transform:rotate(" + angle + "deg);'><div class='arrowhead " + extraClass + "' style='margin-left:" + arrowmargin + "px'/></div>";

    $('#network').append(htmlLine);
    
}

function drawConnections() {
    $('.node').each(function () {
        var myNode = $(this);
        var myParents = JSON.parse($(this).attr('data-parents'));
        if (myParents) {
            $(myParents).each(function (index, value) {
                connect($('#' + value), myNode);
            })
        }

        var mySource = $(this).data('linking_src');
        if (mySource)
            connect($('#' + mySource), myNode, 'linking');

    })
}
