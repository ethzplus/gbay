var use_session = false;
var input_files = [];
var linkings = [];
var linking = false;
var python_script; 

function fake_div(x,y){ 
    return {
        position: function(){ return {left: x, top: y} },
        outerHeight: function(){return 0},
        outerWidth: function(){return 0},
    }
}

function addFileToNode(node, file){

    var extension = file.name.slice(-4);

    if ( (node.prop('id') != 'vectordrop') && (extension != '.tif')){
        displayInfoPopup('Raster files must have ".tif" extension');
        return;
    }
    else if ( (node.prop('id') == 'vectordrop') && (extension != '.zip')){
        displayInfoPopup('Vector files must have ".zip" extension');
        return;
    }
    else if ( (node.prop('id') == 'vectordrop') && (extension == '.zip') ) {
        displayVectorNodes(file);
        return;
    }

    node.addClass('input_node');
    node.find('.node_filename').html(file.name);
    node.children('.node_filename_div').css('display','flex');
    input_files.push({
        nodename: node.prop('id'),
        file: file
    })
    node.find('.uploadGISfile').hide();
}

window.onload = function () {
  
    initNetworkGUI();

    // Continuous set value buttons

    $(".node li.setValueButton").click(function () {
        if(!linking)
            $(this).children('.valueSetter').show();
    });
    $(".contValueCancel").click(function (e) {
        e.stopPropagation();
        if(!linking){
            var node = $(this).closest('.node');
            node.find('.valueSetter').hide();
        }
    });
    $(".contValueAccept").click(function (e) {
        e.stopPropagation();
        if(!linking){
            var node = $(this).closest('.node');
            setContValue(node);     
        }
    });
    $(".resetValueButton").click(function () {
        if(!linking){
            var node = $(this).closest('.node');
            node.find(".nodeStates li").removeClass('isEvidence');
            resetContValue(node);    
        }
    });

    // Discrete likelihood buttons

    $(".node li.setLikelihoodButton").click(function () {
        if(!linking){
            var node = $(this).closest('.node');

            node.find('.likelihoodSetter').show();
    
        }
    });
    $(".likelihoodCancel").click(function (e) {
        e.stopPropagation();
        if(!linking){
            var node = $(this).closest('.node');
            node.find('.likelihoodSetter').hide();
        }
    });
    $(".likelihoodAccept").click(function (e) {
        e.stopPropagation();
        if(!linking){
            var node = $(this).closest('.node');
            setLikelihood(node);     
        }
    });
    $(".resetLikelihoodButton").click(function () {
        if(!linking){
            var node = $(this).closest('.node');
            resetLikelihood(node);    
        }
    });

    $(".node li.targetNodeButton").click(function () {
        if(!linking)
            $(this).closest('.node').toggleClass('targetNode');
    })

    $(".node li.pythonNodeButton").click(function () {
        if(!linking)
            $(this).closest('.node').toggleClass('pythonNode');
    })

    $(".nodeStates li").click(function () {
        if (!linking){
            $(this).siblings('li').removeClass('isEvidence');
            $(this).toggleClass('isEvidence');

            var node = $(this).closest('.node');

            if (node.data('nodetype') == "CONTINUOUS_TYPE") {

                if ($(this).hasClass('isEvidence')){
                    var value = parseFloat($(this).html());

                    if ($(this).next('li').length > 0)
                        value = (value + parseFloat($(this).next('li').html())) / 2;
 
                    setContValue(node,value);
                }
                else{
                    resetContValue(node);
                }  
            }
            else{
                resetLikelihood(node)
            }
        }
    });

    $('.node').on('dragover', function (e) {
        e.preventDefault();
        e.stopPropagation();
    });

    $('.node').on('dragenter', function (e) {
        e.preventDefault();
        e.stopPropagation();
    });

    $('.node').on('drop', function (e) {

        if (e.originalEvent.dataTransfer && e.originalEvent.dataTransfer.files.length) {
            e.preventDefault();
            e.stopPropagation();

            addFileToNode($(this).closest('.node'), e.originalEvent.dataTransfer.files[0]);
        }
    });

    $('.node .file-upload').on('change', function(){
        addFileToNode($(this).closest('.node'), this.files[0])
    });

    // Linking
    $('.linkNodeButton').click(function () {

        if (linking){

            if ($('.linking_src').prop('id') == $(this).parents('.node').prop('id')){
                $('.linking_src').removeClass('linking_src');
                linking = false;
                $('.tmp_linking').remove(); // to remove the arrow
            }
        }
        else{

            // check if there is already a link --> delete it

            var node = $(this).parents('.node');
            var linking_undo = false;

            for (var i =0; i < linkings.length; i++){

                if ( (linkings[i][0] == node.prop('id')) || (linkings[i][1] == node.prop('id')) ){
                    disconnectLinkingNodes($('#'+linkings[i][0]), $('#'+linkings[i][1]));
                    linkings.splice(i,1);
                    linking_undo = true;
                }
            }
            if (linking_undo){
                linking = false;
            }
            else{
                node.addClass('linking_src');
                linking = true;
            }
        }
    })

    $('.node').click(function (e) {

        if (linking && $(this).hasClass('linking_dst')){

            var linking_src = $('.linking_src');
            var linking_dst = $(this);

            linking_src.removeClass('linking_src');

            linkings.push([linking_src.prop('id'), linking_dst.prop('id')]);

            connectLinkingNodes(linking_src, linking_dst);

            $('.tmp_linking').remove();
            $('.node').removeClass('linking_dst');

            linking = false;
        }
    });

    document.onmousemove = handleMouseMove;

    function handleMouseMove(event) {
       if (linking){
            var pos_x = event.pageX - $('#network').offset().left;
            var pos_y = event.pageY - $('#network').offset().top;

            $('.tmp_linking').remove();
            connect($('.linking_src'), fake_div(pos_x, pos_y), 'tmp_linking');
       }
    }

    $('.node').on('mouseenter', function () {
        if(!linking){
            $(this).find('.nodeMenu').show();
        }
        if (linking && (!$(this).hasClass('linking_src'))){    
            $(this).addClass('linking_dst');
        }
    });
    $('.node').on('mouseleave', function () {
        $('.nodeMenu').hide();
        $('.node').removeClass('linking_dst');
    });

    if (use_session) {
        for (key in sessionStorage) {
            $('select[data-node_fullname="' + key + '"]').val(sessionStorage[key]);
        }
    }

    $('#submitForm').click(function (e) {
        if ($('.targetNode').length == 0) {
            displayInfoPopup('Please select at least one target node.');
            return;
        }

        if ($('.input_node').length == 0) {
            displayInfoPopup('Please add at least one raster/vector to a node.');
            return;
        }

        formData = new FormData(document.getElementById('blumap_form'));
        formData.append('by_network', $('#network').data('by_network'));

        for (var i = 0; i < input_files.length; i++) {

            if (input_files[i].file.name.slice(-3) == 'zip'){
                formData.append('vector', input_files[i].file);

                $('.vector_node').each(function (idx, elem) {    // to be able to upload a vector file and then cancel some of the nodes
                    if (! $(this).hasClass('input_node'))
                        formData.append('disabled_vectornodes', $(this).prop('id'));
                })
            }
            else{
                formData.append('raster', input_files[i].file);
                formData.append('filename_nodename', input_files[i].file.name + '=' + input_files[i].nodename);
            }
        }

        $('.isEvidence').each(function (idx, elem) {

            var node = $(this).parents('.node');

            if (node.data('nodetype') == 'DISCRETE_TYPE')
                formData.append('evidence', node.prop('id') + '=' + $(this).index());
            else
                formData.append('evidence', node.prop('id') + '=' + parseFloat(node.find('.stateValue').html()));
        });

        $('.hasLikelihood').each(function (idx, elem) {

            var node = $(this);
            formData.append('likelihood', node.prop('id') + '=' + JSON.stringify(node.data('likelihood')));
        });

        $('.pythonNode').each(function (idx, elem) {
            formData.append('inter_input_node', $(this).prop('id'));
        });

        $('.targetNode').each(function (idx, elem) {
            formData.append('target_node[]', $(this).prop('id'));
        });

        for (var i = 0; i < linkings.length; i++) {
            formData.append('linking', linkings[i][0] + '=' + linkings[i][1]);
        };

        // When python script was loaded using loadConfig (from a zipped file, unable to set the input[type='file'] manually so it will be overwritten

        if (!formData.get('py_module') && python_script)
            formData.set('py_module', python_script);   

        $('.output').empty();

        var url = $('#blumap_form').prop('action');

        $.ajax({
            url: url,
            type: 'POST',
            data: formData,
            processData: false,
            contentType: false,
            timeout: 0, // forever

            complete: function () {
                
                if (use_session) {

                    $('input[type="text"], input[type="number"]').each(function () {
                        sessionStorage.setItem(this.name, this.value);
                    })

                    $('select').each(function () {
                        sessionStorage.setItem($(this).data('node_fullname'), this.value);
                    })
                }
            },

            error: function (jqXHR, textStatus, errorThrown) {
                console.log('Error: ' + jqXHR.responseText);
                $('.output').append('<span class="stderr">' + jqXHR.responseText + '</span>');
            },

            success: function (res, textStatus, jqXHR) {

                if (jqXHR.status == 200) {
                    if (url == window.location.origin + "/run_inference") {
                        $('.downloader').remove();
                        $("body").append("<iframe class='downloader' src='" + 'output' + "' style='display: none;' ></iframe>");
                    }


                } else
                    console.log('Received status code: ' + jqXHR.status);
            },
        });
        e.preventDefault();
    });

    $('.popup-close, #popup-container').click(function () {
        $('#popup-container').hide();
    })

    // Using AJAX instead of normal link to prevent loosing the websocket when changing location
    $('.help_btn').click(function () {
        $('.output').empty();
        $.get("command_help");
    });

    var socket = io();

    socket.on('cmd_stdout', function (data) {
        console.log(data);
        data = data.replace('WARNING -', '<span class="cmd_warning">WARNING: ').replace('- WARNING', '</span');
        $('.output').append(data);
        $('.output').scrollTop($(".output")[0].scrollHeight);
    });

    socket.on('cmd_stderr', function (data) {
        $('.output').append('<span class="stderr">' + data + '</span>');
        $('.output').scrollTop($(".output")[0].scrollHeight);
        displayInfoPopup('ERROR: '+  $('.output .stderr').html());
    });

    socket.on('child_close', function (data) {
        console.log(data);
        $('body').append('<img src="' + data.src + '">');
    });

    $('.rasters input[type="file"]').each(function () {

        if ($(this).val())
            $('.rasterSelector').append('<option value="' + $(this).val() + '">' + $(this).val() + '</option>')
    })

    $('.rasters input[type="file"]').change(updateRasterSelector);
    $('#showInterInfoBtn').click(showInterInfo);

    $('.clearInputFileBtn').click(function () {
        $(this).siblings('input[type="file"]').val('');
        $(this).siblings('.node_filename').empty();
        $(this).parents('.node_filename_div').hide();
        $(this).parents('.node').removeClass('input_node');
        var nodename = $(this).parents('.node').prop('id');
        input_files = input_files.filter(function (e) {
            return e.nodename != nodename
        });

        $(this).parents('.node').find('.uploadGISfile').show();
    });

    $('.selectable').click(function () {
        $(this).toggleClass('selected')
    });

    $('#saveConfig').click(saveConfig);
    $("#loadConfig").change(loadConfig);
    $('#showDetails').click(displayConsolePopup);

    $('#clearPyModuleBtn').click(function () {

        $('input[name="py_module"]').val('');
        $('#pyModuleInputDiv').show();
        $('#pyModuleInfoDiv').hide();

        python_script = null;

    });

    $('.inputfile[name="py_module"]').change(function(){
        if ($('.inputfile[name="py_module"]')[0].files){
            showPythonModuleName($('.inputfile[name="py_module"]')[0].files[0].name);
        }
        else{
            $('#pyModuleInputDiv').show();
            $('#pyModuleInfoDiv').hide();

        }
    })
}

function showPythonModuleName(name){
    $('#PyModuleName').html(name);
    $('#pyModuleInputDiv').hide();
    $('#pyModuleInfoDiv').show();
}

function displayVectorNodes(file){

    formData = new FormData();
    formData.append('vector', file);

    $.ajax({
        url: window.location.origin + "/get_vector_nodes",
        type: 'POST',
        data: formData,
        processData: false,
        contentType: false,

        error: function (jqXHR, textStatus, errorThrown) {

            console.log('displayVectorNodes: ' + jqXHR.status);
            console.log('Error: ' + errorThrown);
            console.log('Error: ' + textStatus);
            console.log('Error: ' + jqXHR.responseText);
            $('.output').append('<span class="stderr">' + jqXHR.responseText + '</span>');
        },

        success: function (res, textStatus, jqXHR) {

            if (jqXHR.status == 200) {   
                try{
                    vector_nodes = JSON.parse(res);
                }
                catch(e){
                    console.log(e)
                     displayInfoPopup('Error parsing vector file:' + e);
                    return;
                }
                input_files.push({
                    nodename: '',   
                    file: file
                })

                $('.vectornode').find('.node_filename').html(file.name);
                $('.vectornode').children('.node_filename_div').css('display','flex');

                for (var i =0; i < vector_nodes.length; i++){

                    if($('#'+vector_nodes[i])){
                        node = $('#'+vector_nodes[i]);

                        node.addClass('input_node');
                        node.addClass('vector_node');
                        node.find('.node_filename').html(file.name);
                        node.children('.node_filename_div').css('display','flex');
                    }
                    else
                        console.warn('Node from vector' +vector_nodes[i] + ' was not found in the network.');
                }
            } else
                console.log('Received status code: ' + jqXHR.status);
        },
    });
}

/* update the GUI */
function connectLinkingNodes(node_src, node_dst) {
    node_src.find('.linkNodeButton').html('to: ' + node_dst.prop('id'));
    node_dst.find('.linkNodeButton').html('from: ' + node_src.prop('id'));
    connect(node_src, node_dst, 'linking');
    node_dst.data('linking_src', node_src.prop('id')); // to redraw the arrow when moving nodes
}

function disconnectLinkingNodes(node_src, node_dst) {
    node_src.find('.linkNodeButton').html('Link');
    node_dst.find('.linkNodeButton').html('Link');
    node_dst.removeData('linking_src'); // to redraw the arrow when moving nodes
    
    for (var i =0; i < $('.connectingLine.linking').length; i++){

        if( ($('.connectingLine.linking').eq(i).data('src') == node_src.prop('id')) && ($('.connectingLine.linking').eq(i).data('dst') == node_dst.prop('id'))){
            $('.connectingLine.linking').eq(i).remove();
            break;
        }
    }
}

function loadConfig(evt) {

    var file = evt.target.files[0];
    var reader = new FileReader();
    reader.onload = function(e){

        try{
            JSZip.loadAsync(e.target.result).then(function(zip) {

                var config_zip = zip.file("config.json")

                if(!config_zip){
                    displayInfoPopup('loadConfig: No config.json found in the file.');
                    return;
                }

                config_zip.async("string").then(function success(content) {
                    
                    try{
                        var config = JSON.parse(content);
                    } catch (e) {
                        displayInfoPopup('loadConfig:Error parsing config file:' + e);
                        return;
                    }

                    $('.isEvidence').removeClass('isEvidence');
                    $('.hasLikelihood').removeClass('hasLikelihood');
                    $('.node').removeClass('pythonNode');
                    $('.node').removeClass('targetNode');
                    $('.linking').remove();

                    //console.log(config)

                    linking = false;
                    linkings = config.linkings

                    for (var i = 0; i < config.evidences.length; i++){
            
                        if ($('#' + config.evidences[i][0]).data('nodetype') == 'DISCRETE_TYPE')
                            $('#' + config.evidences[i][0] + ' .nodeStates li').eq(config.evidences[i][1]).addClass('isEvidence');
                        else
                            setContValue($('#' + config.evidences[i][0]), config.evidences[i][1]);
                    }
                    for (var i = 0; i < config.likelihoods.length; i++)
                        setLikelihood($('#' + config.likelihoods[i][0]), config.likelihoods[i][1]);

                    for (var i = 0; i < config.inter_input_nodes.length; i++)
                        $('#' + config.inter_input_nodes[i]).addClass('pythonNode');

                    for (var i = 0; i < config.target_nodes.length; i++)
                        $('#' + config.target_nodes[i]).addClass('targetNode');

                    for (var i = 0; i < config.linkings.length; i++)
                        connectLinkingNodes($('#' + config.linkings[i][0]), $('#' + config.linkings[i][1]));

                    $('input[name=iterations]').val(config.iterations);

                    if (config.python_script_name){

                        zip.file("python_script").async("blob").then(function success(blob) {

                            var file = new File([blob], config.python_script_name, {type: 'text/x-python', lastModified: Date.now()});

                            python_script = file;
                            showPythonModuleName(file.name);

                            $('input[name="py_module"]').val('');
                        })
                    }
                    config.input_files.forEach(function(e){

                        zip.file(e[1]).async("blob").then(function success(blob) {
                            var file = new File([blob], e[1], {type: "image/tiff", lastModified: Date.now()});
                            addFileToNode($('#'+e[0]), file);

                        }, function (e) {
                            console.error(e)
                            displayInfoPopup('loadConfig: Error decompressing '+e[1]+': '+e);
                            return;
                        });
                    });

                }, function (e) {
                    console.error(e)
                    displayInfoPopup('loadConfig: Error decompressing config.json: '+e);
                    return;
                });

            }, function (e) {
                console.error(e)
                displayInfoPopup('loadConfig: Error decompressing ZIP file.');
                return;
            })
    
        } catch (e) {
           displayInfoPopup('Unsupported file format:' + e);
        }
    }
    reader.readAsArrayBuffer(file);
}

function saveConfig() {
    var config = {
        evidences: [],
        inter_input_nodes: [],
        target_nodes: [],
        linkings: [],
        iterations: 0,
        input_files: [],
        likelihoods: []
    }

    $('.isEvidence').each(function (idx, elem) {

        var node = $(this).parents('.node');

        if (node.data('nodetype') == 'DISCRETE_TYPE')
            config.evidences.push([node.prop('id'), $(this).index()]);
        else
            config.evidences.push([node.prop('id'), parseInt($(this).find('.contValue').html())]);
    });
    $('.pythonNode').each(function (idx, elem) {
        config.inter_input_nodes.push($(this).prop('id'));
    });
    $('.targetNode').each(function (idx, elem) {
        config.target_nodes.push($(this).prop('id'));
    }); 
    $('.hasLikelihood').each(function (idx, elem) {
        config.likelihoods.push([$(this).prop('id'), $(this).data('likelihood')]);
    });

    for (var i = 0; i < linkings.length; i++) {
        config.linkings.push([linkings[i][0], linkings[i][1]]);
    };
    config.iterations = parseInt($('input[name=iterations]').val());

    var zip = new JSZip();

    var py_module = $('.inputfile[name="py_module"]')[0].files[0] || python_script;
    if(py_module){
        config.python_script_name = py_module.name;
        zip.file('python_script', py_module);
    
    }

    for (var i =0; i < input_files.length; i++){

        zip.file(input_files[i].file.name, input_files[i].file); 
        config.input_files.push([input_files[i].nodename, input_files[i].file.name]);
    }

    zip.file("config.json", JSON.stringify(config));

    zip.generateAsync({type:"blob", compression: "DEFLATE"}).then(function (blob) { // 1) generate the zip file
        saveAs(blob, "gbay_run.zip");                          // 2) trigger the download
    }, function (err) {
        console.error(saveConfig)
        console.error(err);
    });
}

function showInterInfo() {
    var win = window.open('/instructions_thirdparty.txt')
    win.focus();
}

function printIntermidiateResults(iterations) {
    $('.iteration_img').remove();

    for (var i = 0; i < iterations; i++) {
        $('.intermidiate').append('<img class="iteration_img" alt="iteration_' + i + '" title="iteration_' + i + '" src="iterations?it=' + i + '">')
    }
}

function updateRasterSelector() {

    var rasters = [];

    $('.rasters input[type="file"]').each(function () {
        if ($(this).val())
            rasters.push($(this).val());
    });

    var value = $('.rasterSelector').val();
    $('.rasterSelector').empty();

    for (var i = 0; i < rasters.length; i++)
        $('.rasterSelector').append('<option value="' + rasters[i] + '">' + rasters[i] + '</option>');

    if (rasters.find(function (e) {
            return e === value
        }))
        $('.rasterSelector').val(value);
}

function displayInfoPopup(text) {
    
    $('#infoPopup .content').html(text);
    $('#popup-container').css('display', 'flex');
}

function displayConsolePopup() {

    if ($('#showDetails').prop('checked')){
        $('#consoleOutput').css('display', 'block');
    }
    else{
        $('#consoleOutput').css('display', 'none');
    }
}

// Argument 'value' in case of setting evidence by clicking on the li
function setContValue(node, value){

    resetContValue(node);
    value = value || node.find('.contValueInput').val()
    var states = node.find('.nodeStates li')
    states.removeClass('isEvidence');
        
    for (var i=0; i < states.length; i++){

        if ((parseFloat(states.eq(i).html()) <= value) && ((i == states.length-1) || (parseFloat(states.eq(i+1).html()) > value)) ){
            states.eq(i).html(states.eq(i).html() + '<span class="stateValue contValue">'+value+'</span>');
            states.eq(i).toggleClass('isEvidence');

            break;
        }
    }
    node.find('.valueSetter').hide();
    node.find('.setValueButton').hide();
    node.find('.resetValueButton').show();
}

function resetContValue(node){
    node.find('.stateValue').remove();
    node.find('.setValueButton').show();
    node.find('.resetValueButton').hide();
}

function resetLikelihood(node){
    node.find('.stateValue').remove();
    node.find('.setLikelihoodButton').show();
    node.find('.resetLikelihoodButton').hide();
    node.removeData('likelihood');
    node.removeClass('hasLikelihood')
}

function setLikelihood(node, likelihood){
    resetLikelihood(node);
    likelihood = likelihood || Array.from(node.find('.likelihoodInput').map(function(){return parseInt($(this).val())}));
    var states = node.find('.nodeStates li');
    states.removeClass('isEvidence');
    for (var i =0; i < likelihood.length; i++)
        states.eq(i).html(states.eq(i).html() + '<span class="stateValue">'+likelihood[i]+'</span>');

    node.data('likelihood', likelihood);
    node.addClass('hasLikelihood');
    node.find('.likelihoodSetter').hide();
    node.find('.setLikelihoodButton').hide();
    node.find('.resetLikelihoodButton').show();
}   