

var ui_nodes = [
    {
        name: "bottomline",
        title: "title_bottomline",
        kind: "NATURE",
        discrete: true,
        parents: ["box1" , "box2"] ,
        states: [ "state1", "state2", "state3" ],
        position: [200, 200],
        
        probs : [
        //  state1  state2  state3  // box1 box2
            [0.5,   0.4,    0.1,    'state1',   's1'],
            [0.3,   0.3,    0.4,    'state1',   's2'],
            [0.2,   0.4,    0.4,    'state2',   's1'],
            [0.1,   0.1,    0.8,    'state2',   's2']

        ]
    },
    {
        name: "box1",
        title: "Superbox1",
        kind: "NATURE",
        discrete: true,
        parents: [] ,
        states: [ "state1", "state2"],
        probs : [
        //  state1  state2
            [0.6, 0.4]
        ],
        position: [300, 500]
    },
    {
        name: "box2",
        title: "Superbox2",
        kind: "DECISION",
        discrete: true,
        parents: [] ,
        states: [ "s1", "s2"],
        probs : [
        //  s1  s2
            [0.3, 0.7]
        ],
        position: [400, 200]
    },
    {
        name: "box3",
        title: "Superbox3",
        kind: "NATURE",
        discrete: false,
        parents: ["bottomline","box2"] ,
        states: [ "d1", "d2"],
        levels: [0, 0.3, 0.6],
        position: [600, 50],
        probs : [
        //  d1  d2  // bottomline box2
            [0.5,   0.5,    'state1',   's1'],
            [0.3,   0.7,    'state1',   's2'],
            [0.2,   0.8,    'state2',   's1'],
            [0.1,   0.9,    'state2',   's2'],
            [0.2,   0.8,    'state3',   's1'],
            [0.9,   0.1,    'state3',   's2']
        ]

    },
    {
        name: "box4",
        title: "Superbox4",
        kind: "NATURE",
        discrete: true,
        parents: ["box1","box2", "box3"] ,
        states: [ "aaa", "bbb"],
        position: [800, 300],
        probs : [
        //  aaa  bbb  // box1 box2 box3
            [0.5,   0.5,    'state1',   's1',   'd1'],
            [0.3,   0.7,    'state1',   's1',   'd2'],
            [0.2,   0.8,    'state1',   's2',   'd1'],
            [0.1,   0.9,    'state1',   's2',   'd2'],
            [0.5,   0.5,    'state2',   's1',   'd1'],
            [0.3,   0.7,    'state2',   's1',   'd2'],
            [0.2,   0.8,    'state2',   's2',   'd1'],
            [0.1,   0.9,    'state2',   's2',   'd2'],
            
        ]
    }
]

var ui_node_selected;
var node_connecting;

$(document).ready(function() {
    // connect($('#div1'), $('#div2'));
    // connect($('#div2'), $('#div3'));
    // connect($('#div3'), $('#div1'));

    // $(".box").draggable({
    //     containment: '#chart',
    //     drag: function(event, ui) {
    //         $('.connectingLine').remove();
    //         connect($('#div1'), $('#div2'));
    //         connect($('#div2'), $('#div3'));
    //         connect($('#div3'), $('#div1'));
    //     }
    // });

    //$(".selectable").mousedown(selectNode);

    $('body').on('click', '.selectable', selectNode);

    $('#node_info input[type!=button]').val('');
    $('#node_info select').val('');
    $('#node_info #node_probs').empty();

    //$('#nodemenu_btn').click(displayNodeMenu);
    $('#new_node_btn').click(createNode);
    $('#delete_node_btn').click(deleteSelectedNode);
    $(document).keypress(handleKeyPress);

    //$('#apply_node_btn').click(applySelectedNode);

    $('#node_info input, #node_info select').on('input', applySelectedNode);

   // $('#chart').mouseup(function(){displayNodes(ui_nodes);});

    $('#connect_node_btn').click(startConnecting);
    $('#msgDialog .sub').click(function(){$('#msgDialog').hide()});
    $('#probs_node_btn').click(displayProbTable);
    $('#save_net_btn').click(saveNetwork);
    $('#load_net_btn').click(displayLoadMenu);


    //$('.node_btn').hide();

    displayNodes(ui_nodes);

     $.get('net01_simple.dne', function(data){
        dne2json(data);
    })

});




function dne2json(dne_string){

    var str = dne_string.replace(/;/g,',').replace(/\(/g,'[').replace(/\)/g,']').replace(/=/g,':');

    // remove from '//' to the end of the line
    while (str.indexOf('\/\/') != -1){
        var begin = str.indexOf('\/\/');
        var end = str.indexOf('\n', begin);
        var str = str.slice(0, begin) + str.slice(end+1);
    }

    var lines = dne_string.split('\n');

    var name_line = lines.find(function(line) { return line.slice(0,4) == 'bnet' } )
    var net_name = name_line.slice(5, name_line.indexOf('{')-1);
    console.log('Net name: ' + net_name);




    var node_line = lines.find(function(line) { return line.slice(0,4) == 'node' } )
    while (str.indexOf('\nnode') != -1){

        var begin = str.indexOf('\nnode');

        var node_name = str.slice(begin+5, str.indexOf('{', begin)-1);

        var node_data = str.slice(str.indexOf('{', begin-2), str.indexOf('\t},\n')) ;

        console.log('Node name: ' + node_name);
        console.log('Node data: ' + node_data);
       
        break;

        var str = str.slice(0, begin) + str.slice(str.indexOf('},\n\n') + 4);
    }

    //console.log(str);


    // console.log('str: '+str);

}




function loadNetwork(){

    var file = document.getElementById("bn_input").files[0];
    if (file) {
        var reader = new FileReader();
        reader.readAsText(file, "UTF-8");
        reader.onload = function (evt) {
            // displayDialog(evt.target.result);
            console.log(evt.target.result);

            var json = dne2json(evt.target.result);

        }
        reader.onerror = function (evt) {
            displayDialog("Error reading file");
        }
    }

}

function displayLoadMenu(){

    var msg = ('<input type="file" id="bn_input" required><input type="button" value="Submit" onclick="loadNetwork()">');
    displayDialog(msg);

}

function displayProbTable(){

    if(!ui_node_selected)
        return;
    else{

        var div_container = $('<div id="node_probs"></div>');
        div_container.append(node_probs2html(ui_node_selected));
        displayDialog(div_container);

        $('#node_probs td').on('input',modifyProbs);    
    }
}


function saveNetwork(){


    var dne_visual = `
visual V1 {
    defdispform = BELIEFBARS;
    nodelabeling = TITLE;
    NodeMaxNumEntries = 50;
    nodefont = font {shape= "Arial"; size= 9;};
    linkfont = font {shape= "Arial"; size= 9;};
    windowposn = (26, 26, 1212, 509);
    scrollposn = (152, 0);
    resolution = 72;
    magnification = 0.857373;
    drawingbounds = (16062, 8035);
    showpagebreaks = FALSE;
    usegrid = TRUE;
    gridspace = (6, 6);
    NodeSet Node {BuiltIn = 1; Color = 0x00E1E1E1;};
    NodeSet Nature {BuiltIn = 1; Color = 0x00F8EED2;};
    NodeSet Deterministic {BuiltIn = 1; Color = 0x00D3CAA6;};
    NodeSet Finding {BuiltIn = 1; Color = 0x00C8C8C8;};
    NodeSet Constant {BuiltIn = 1; Color = 0x00FFFFFF;};
    NodeSet ConstantValue {BuiltIn = 1; Color = 0x00FFFFB4;};
    NodeSet Utility {BuiltIn = 1; Color = 0x00FFBDBD;};
    NodeSet Decision {BuiltIn = 1; Color = 0x00DEE8FF;};
    NodeSet Documentation {BuiltIn = 1; Color = 0x00F0FAFA;};
    NodeSet Title {BuiltIn = 1; Color = 0x00FFFFFF;};
    NodeSet Game {Color = 0x0080FFFF;};
    NodeSet expert {Color = 0x00FFFFC6;};
    NodeSet workInProgress {Color = 0x00FCFCFC;};
    NodeSet Actor {Color = 0x00C5D7B9;};
    NodeSet Telecoupling {Color = 0x00EFA0A0;};
    NodeSet Flow {Color = 0x00FF80FF;};
    };
`; 


    var date = new Date();
    var str= '// ~->[DNET-1]->~\n';
    str += '// File created by somebody using Blumap 0.1 on '+ date.toString()+'\n';
    str += 'bnet ' + $('#net_name').val() + ' {\n';
    str += 'autoupdate = TRUE;\n';
    str += 'whenchanged = '+date.getTime()+ ';\n\n';

    str += dne_visual + '\n';

    for (var i=0 ; i<ui_nodes.length; i++){
        var node = ui_nodes[i];
        str += 'node ' + node.name + ' {\n';
        str += '\tkind = ' + node.kind + ';\n';
        str += '\tdiscrete = ' + (node.discrete?'TRUE':'FALSE') + ';\n';

        if (node.states){
            str += '\tstates = (';
            for (var j =0; j < node.states.length; j++){
                str += node.states[j];
                if (j < node.states.length-1)
                    str += ', ';
            }
            str += ');\n';
        }

        if (node.levels){
            str += '\tlevels = (';
            for (var j =0; j < node.levels.length; j++){
                str += node.levels[j];
                if (j < node.levels.length-1)
                    str += ', ';
            }
            str += ');\n';
        }

        str += '\tparents = (';
        for (var j =0; j < node.parents.length; j++){
            str += node.parents[j];
            if (j < node.parents.length-1)
                str += ', ';
        }

        str += ');\n';

        if (node.probs){

            str += '\tprobs = \n\t\t// ';

            for (var j =0; j < node.states.length; j++)
                str += node.states[j] + '\t';
            
            str += '//\t';

            for (var j =0; j < node.parents.length; j++)
                str += node.parents[j] + '\t';
            
            str += '\n\t\t(';
            
            for (var j =0; j < node.probs.length; j++){
                var prob = node.probs[j];

                var sep = ',\t';

                for (k =0; k < prob.length; k++){

                    if (k == node.states.length)
                        sep = '\t';


                    if ( (j == node.probs.length -1 ) && (k == node.states.length-1) )
                        str += prob[k] + ');\t';
                    else
                        str += prob[k] + sep;
                    
                    if (k == node.states.length-1)
                        str += '//\t';
                }

                if (j == node.probs.length-1)
                    str += ';\n';
                else
                    str += '\n\t\t';

            }


        }

        str += '\ttitle = ' + node.title + ';\n';
        str += '\tvisual V1 {\n';
        str += '\t\tcenter = ('+node.position[0]+', '+node.position[1] + ');\n'
        str += '\t\theight = '+ parseInt($('#'+node.name).css('height').slice(0,-2)) +';\n'; // TESTING
        str += '\t\t};\n';
        str += '\t};\n';
    }
    str += '};\n';
    
    console.log(str);

    //triggerDownload($('#net_name').val() +'.dne', str);
    
}

function triggerDownload(filename, content){
    var link = document.createElement('a');
    link.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(content));
    link.setAttribute('download', filename);
    link.style.display = 'none';
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

function startConnecting(){

    if (node_connecting){
        node_connecting = false;
        $('#message').html('');
    }
    else if(ui_node_selected){
        node_connecting = ui_node_selected;
        $('#message').html(' - Click on another node -');
    }
}
    

function handleKeyPress(e){
    // console.log(e.key);
    // console.log($(':focus'));

    if ($(':focus').length == 0){

        if (e.key == 'Escape'){
            displayNodeInfo();
            $('#'+ui_node_selected.name).removeClass('selected');
            ui_node_selected = '';
            node_connecting = false;
            $('#message').html('');
            $('#msgDialog').hide();
        }
        else if (e.key == 'd')
            deleteSelectedNode();
        else if (e.key == 'n')
            createNode();
        else if (e.key == 'c')
            startConnecting();
        else if (e.key == 's')
            saveNetwork();
    }
}

function displayDialog(msg){

    $('#dialogContent').html(msg);
    $('#msgDialog').show();
}

function createNode(){

    if (!$('#new_node_name').val()){
        displayDialog('Please provide a name for the new node');
        return;
    }

    if (ui_nodes.find(function(node){ return (node.name == $('#new_node_name').val()) })){
        displayDialog('node "'+$('#new_node_name').val()+'" already exists. Please select a different name.');
        return;
    }


    var states = [];

    // for (var i =0; i < $('#node_new input[name="new_node_state"]').length; i++){
    //     var state = $('#node_new input[name="new_node_state"]').eq(i).val();
    //     if (state)
    //         states.push(state);
    // }


    var states_lines = $('#node_new textarea[name="new_node_states"]').val().split('\n');
    for(var i = 0;i < states_lines.length;i++){
        if (states_lines[i])
            states.push(states_lines[i]);
    }

    var new_node = {
        name: $('#new_node_name').val(),
        kind: $('#new_node_kind').val(),
        discrete: $('#new_node_discrete').val() == 'Discrete',
        parents: [] ,
        states: states,
        position: [0, 0]
    }

    ui_nodes.push(new_node);
    displayNodes([new_node]);

    createProbabilityTable(new_node);


    $('#' + new_node.name).click();
}




function connect(div1, div2) { // draw a line connecting elements from div1 to div2

    if (!div1.length || !div2.length){
        console.log('connect: wrong parameters');
        return;
    }

    var off1 = div1.offset();
    var off2 = div2.offset();
    // box1
    var x1 = off1.left + div1.outerWidth() / 2;
    var y1 = off1.top + div1.outerHeight() / 2;
    // box2
    var x2 = off2.left + div2.outerWidth() / 2;
    var y2 = off2.top + div2.outerHeight() / 2;
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

    //console.log(angle);
    // make hr
    var htmlLine = "<div class='connectingLine' data-source='"+ div1.prop('id') +"' data-target='"+ div2.prop('id') +"' style='left:" + cx + "px; top:" + cy + "px; width:" + length + "px; -moz-transform:rotate(" + angle + "deg); -webkit-transform:rotate(" + angle + "deg); -o-transform:rotate(" + angle + "deg); -ms-transform:rotate(" + angle + "deg); transform:rotate(" + angle + "deg);'><div class='arrowhead' style='margin-left:" + arrowmargin + "'/></div>";

    //document.body.innerHTML += htmlLine;
    $('body').after(htmlLine);

    return $(htmlLine);
}



function selectNode(){

    if (!($(this).hasClass('selected'))) {

        var name = $(this).prop('id');
        var node = ui_nodes.find(function(e){return e.name == name});

        $('.node_btn').show();

        if (node_connecting){

            if (!node.parents.find(function(parent_name){return parent_name == ui_node_selected.name})){
                connect($('#' + ui_node_selected.name), $(this));    
                node.parents.push(ui_node_selected.name);

                node.probs = createProbabilityTable(node);
            }
            else
                console.log('Node '+ ui_node_selected.name +' was already a parent of '+ name);

            node_connecting = false;            
            $('#message').html('');
        }
        else{
            $('#chart').find('.selected').removeClass('selected');
            $(this).addClass('selected');
            displayNodeInfo(node);
            ui_node_selected = node;
        }
    }
}

function createProbabilityTable(node){

    var n_rows = parentStatesCombined(node);

    var node_probs = [];
    var parent_indexes = [];

    var str_debug;

    //console.log('createProbabilityTable: '+ node.name + ': parents: '+node.parents.length);

    for (var j=0; j < node.parents.length; j++)
        parent_indexes.push(0);

    for (var i=0; i < n_rows; i++){

        // str_debug = "row: "+i+': ';

        // for (var j=0; j < node.parents.length; j++)
        //     str_debug += parent_indexes[j] + ' ';

        // console.log(str_debug);



        var prob = [];

        for (var j=0; j < node.states.length; j++)
            prob.push(0);

        for (var j=0; j < node.parents.length; j++){

            var parent_name = node.parents[j];
            var parent_node = ui_nodes.find(function(e){return e.name == parent_name});
           
            prob.push(parent_node.states[parent_indexes[j]]);

        }
        node_probs.push(prob)

        var offset = parent_indexes.length-1;    
        parent_indexes[offset]++;
    
        while(offset && parent_indexes[offset] > parent_node.states.length-1){
            parent_indexes[offset] = 0;
            offset--;
            parent_indexes[offset]++;
            parent_name = node.parents[offset];
            parent_node = ui_nodes.find(function(e){return e.name == parent_name});
        }


    }

    return node_probs;
}


function deleteSelectedNode(){

    if (!ui_node_selected)
        return;

    var affected_nodes = []; //nodes whose parent was the deleted one

    ui_nodes = ui_nodes.filter(function(e){return e.name != ui_node_selected.name});

    // delete node form parent list
    for (var i =0; i < ui_nodes.length; i++){

        var index = ui_nodes[i].parents.indexOf(ui_node_selected.name);

        if (index != -1){
            ui_nodes[i].parents = ui_nodes[i].parents.filter(function(parent_name){return parent_name != ui_node_selected.name});
        
            affected_nodes.push(i);
        }
    }

    // fix probablity tables
    for (var i =0; i < affected_nodes.length; i++)
        ui_nodes[affected_nodes[i]].probs = createProbabilityTable(ui_nodes[affected_nodes[i]]);


    // html
    $('#'+ui_node_selected.name).remove();
    $('.connectingLine[data-target="'+ ui_node_selected.name +'"]').remove();
    $('.connectingLine[data-source="'+ ui_node_selected.name +'"]').remove();

    displayNodeInfo('');
    ui_node_selected = undefined;

    $('.node_btn').hide();
    
}

function applySelectedNode(){

    //if (confirm('Are you sure you want to apply changes to the node: '+ui_node_selected.name + '?')){

        if (ui_nodes.find(function (node){ return ((ui_node_selected.name != node.name)&&(node.name == $('#node_name').val())) })){
            displayDialog('This node already exists, please choose a different name');
            return;
        }


        //deleteSelectedNode(false);
        


       // ui_node_selected.name = $('#node_name').val();
        ui_node_selected.title = $('#node_title').val();
        ui_node_selected.kind = $('#node_kind').val();
        ui_node_selected.discrete = ($('#node_discrete').val() == 'Discrete');


        $('#'+ui_node_selected.name+' .header').html($('#node_name').val());
        $('#'+ui_node_selected.name).prop('id', $('#node_name').val());

        
        
        $('.connectingLine[data-target="'+ ui_node_selected.name +'"]').attr('data-target', $('#node_name').val());
        $('.connectingLine[data-source="'+ ui_node_selected.name +'"]').attr('data-source', $('#node_name').val());

        ui_node_selected.name = $('#node_name').val();

        //displayNodes([ui_node_selected]);

    //}
}

function displayNodeInfo(node){

    if (!node){
        $('#node_name').val('');
        $('#node_title').val('');
        $('#node_kind').val('');
        $('#node_discrete').val('Discrete');
        $('#node_probs').html('');
    }
    else{

        $('#node_name').val(node.name);
        $('#node_title').val(node.title);
        $('#node_kind').val(node.kind);

        if (node.discrete)
            $('#node_discrete').val('Discrete');
        else
            $('#node_discrete').val('Continuous');

        $('#node_probs').html(node_probs2html(node));
        $('#node_probs td').on('input',modifyProbs);
    }

}

function parentStatesCombined(node){  // get parents states combinations number

    var total_states = 0;

    for (var i =0 ; i < node.parents.length; i ++){
 
        var parent_name = node.parents[i];
        var parent_node = ui_nodes.find(function(node){return node.name == parent_name });

        if (total_states == 0)
            total_states = parent_node.states.length;
        else
            total_states *= parent_node.states.length;

    }

    //console.log('parentStatesCombined: '+ node.name+ ': '+ total_states);

    return total_states
}

function modifyProbs(){

    var probs = $('#node_probs td[contenteditable="true"]');
    
    var rows = parentStatesCombined(ui_node_selected);
    var node_states = ui_node_selected.states.length;

    for (var i =0; i < probs.length; i ++){
        //console.log(i + ': ' + parseInt(i / (rows-1)) + '  ' + i % node_states );
        ui_node_selected.probs[parseInt(i / (rows-1))][i % node_states] = parseFloat($(probs[i]).html());
    }
}



function node_probs2html(node){

    if (!node.probs || node.probs.length == 0)
        return '';

    var table = $('<table></table>');

    var row = $('<tr></tr>');
    
    for (var i =0; i < node.states.length; i++)
        row.append('<th>'+ node.states[i] +'</th>');
    for (var i =0; i < node.parents.length; i++)
        row.append('<th>'+ node.parents[i] +'</th>');

    var thead = $('<thead></thead>')
    
    thead.append(row);
    table.append(thead);

    var tbody = $('<tbody></tbody>')

    for (var i =0; i < node.probs.length; i++){
        var row = $('<tr></tr>');

        for (var j =0; j < node.probs[i].length; j++){
        
            if (j < node.states.length)
                row.append('<td class="cell_editable" contenteditable="true">'+node.probs[i][j]+'</td>');
            else
                row.append('<td>'+node.probs[i][j]+'</td>');


        }
        tbody.append(row);
    }

    table.append(tbody);

    return table;

}



function displayNodes(nodes){

    // print nodes
    for (var i=0; i < nodes.length; i++){
        $("#chart").append(node2html(nodes[i]));
        $('#' + nodes[i].name).draggable({
            containment: '#chart',
            drag: dragNode,
        });
    }

    // print lines
    for (var i=0; i < nodes.length; i++){
        for (var j=0; j< nodes[i].parents.length; j++){
            connect($('#' + nodes[i].parents[j]), $('#' + nodes[i].name));    
        }
    
       
    }
}

function dragNode(event, ui) {

    var name = $(this).prop('id');

    $('.connectingLine[data-target="'+ name +'"]').remove();
    $('.connectingLine[data-source="'+ name +'"]').remove();

    var node = ui_nodes.find(function(e){return e.name == name});

    for (var i=0; i< node.parents.length; i++){
        connect($('#' + node.parents[i]), $('#' + name));    
    }
    for (var i=0; i< ui_nodes.length; i++){

        if (ui_nodes[i].parents.find(function(parent_name){return parent_name == name}))
            connect($('#' + name), $('#' + ui_nodes[i].name));    
    }


    node.position =  [$('#' + name).offset().left, $('#' + name).offset().top];


}

function node2html(node){
    
    var ui_node = $('#templates .box').clone();

    ui_node.prop('id', node.name);
    ui_node.find('.header').html(node.name);

    for (var i=0; i < node.states.length; i++){
        var ui_node_state = $('#templates .state').clone();
        ui_node_state.find('.state_name').html(node.states[i]);
        ui_node.find('.states').append(ui_node_state);
    }

    ui_node.css('left', node.position[0] + 'px');
    ui_node.css('top', node.position[1] + 'px');

    return ui_node;

}



