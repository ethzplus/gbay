var express = require('express');
var router = express.Router();
var async = require('async');
var bodyParser = require('body-parser');
var multer  = require('multer');
var Log = require(__base + 'lib/logger.js').Log;
var path = require('path');
var fs = require('fs');
var sanitize_filename = require("sanitize-filename");
var archiver = require('archiver');
var decompress = require('decompress');
var nodemailer = require('nodemailer');
var command = require(__base + 'lib/command.js');
var historial = require(__base + 'lib/historial.js');
var config = require(__base + './config.js');
var rasterdir = config.rasterdir;
var vectordir = config.vectordir;
var py_modules_dir = config.py_modules_dir;
var command_run_inference = config.command_run_inference;
var command_get_info = config.command_get_info;
var command_vectornodes = config.command_vectornodes;
var output_dir = config.output_dir;
var bn_commands_dir = config.bn_commands_dir;

var mail_transporter = nodemailer.createTransport(config.mail_settings);

var storage = multer.diskStorage({
  destination: function (req, file, cb) {

    if (file.fieldname == 'py_module')
        cb(null, py_modules_dir);
    else if (file.fieldname == 'vector')
        cb(null, vectordir);
    else{
        var userdir = path.join(__base, rasterdir, sanitize_filename(req.session.id));

        if (!fs.existsSync(userdir)){
            fs.mkdir(userdir, null, function(err){
                cb(err, userdir);
            })
        }
        else
            cb(null, userdir);
    }
  },

  filename: function (req, file, cb) {
    if (file.fieldname == 'py_module')
        cb(null, sanitize_filename('_' + req.session.id +'_'+ file.originalname)  ); // cause it is 'imported' to not import a submodule
    else
        cb(null, file.originalname);
  }

});

var upload = multer({ storage: storage });

var multer_post_params = upload.fields([
    { name: 'by_network', maxCount: config.max_BN_files },
    { name: 'raster', maxCount: config.max_raster_files },
    { name: 'vector', maxCount: 1 },
    { name: 'py_module', maxCount: 1 },
    { name: 'raster_dbf', maxCount: config.max_raster_files },
]);


function sendNotificationEmail(email, code, signal, session_id, command_info){

    if (signal){
        var subject = 'GBAY could not process your data';
        var content_html = 'The process ended unexpectedlly by signal '+signal;
    }
    else if (code && code !== 0) {
        var subject = 'GBAY could not process your data';
        var content_html = 'Internal server error.';
    }
    else{
        var result_link = config.url + '/outputViaMail?q=' + session_id;
        var subject = 'GBAY process is finished';
        var content_html = 'Click on the link to get your results: ' + '<a href="'+result_link+'">Get the results</a>';

        if(command_info)
            content_html += '<p>Command summary: </p><pre>' + JSON.stringify(command_info, null, 4) + '</pre>'; 
        content_html += '<p>Evidences values for discrete nodes are the index of the evidence state. First state is number zero.</p>'
    }

    mail_transporter.sendMail({
        from: config.mailfrom,
        to: email,
        subject: subject,
        html: content_html,                
        }, function(err) {
            if (err)
                Log(err && err.stack, 'error');
        }
    );
}

function exitCodeResponse(code, signal, req, res){

    if (!res){
        Log('exitCodeResponse: not sending response. User probably disconnected', 'warn');
        return;
    }

    if (signal)
        res.sendStatus(204);
    else if (code && code !== 0) 
        res.sendStatus(500);  
    else
        res.sendStatus(200);
}

function sendOutputFile(req, res, id){

    var output_dir = path.join(__base, config.output_dir, sanitize_filename(id));
    var files = fs.readdirSync(output_dir);
    if (files.length == 1){
        return res.download(path.join(output_dir, files[0]));
    }
    else{
        for (var i =0; i < files.length; i++){
            if (files[i].slice(-4) == '.zip')
                return res.download(path.join(output_dir, files[i]), 'output.zip');
        }
    }
    return res.sendStatus(404);
}

function sanitize(string){
    return string.replace(/[^0-9a-zA-Z_\-=\[\]\,\.]/g, '');
}

module.exports = function(users){

    function checkRequest(req, res, next){
        try{

            if (!users[req.session.id] ){
                Log('Session not recognized "'+req.session.id+'"', 'error')
                res.status(500).send(' - Server has restarted. Please reload the page. - ');
                return;
            }
            else if (!users[req.session.id].socket){
                Log('No socket to send chunk data for session "'+req.session.id+'"', 'error')
                res.status(500).send(' - Server has restarted. Please reload the page. - ');
                return;
            }
            else
                next();
        }
        catch(err){
            res.status(500).send(err);
        }
    }

    router.get('/outputViaMail', function(req, res) {
        Log(req.query);
        return sendOutputFile(req, res, req.query.q);
    });

    // get output file. In case of multiple target nodes they were already zipped in processOutputFiles
    router.get('/output', checkRequest, function(req, res) {
        return sendOutputFile(req, res, req.session.id);
    });

    router.get('/iterations', checkRequest, function(req, res) {
        Log('Sending file: '+'/dev/shm/blumap_'+req.session.id+'_it_'+req.query.it+'.jpg');
        res.sendFile('/dev/shm/blumap_'+req.session.id+'_it_'+req.query.it+'.jpg', function(error){
            if(error)
                Log(error.message, 'error');
            else
                Log('File transfered successfully.');
        })
    });
    
    router.get('/command_help', checkRequest, function(req, res) {
        Log(path.join(__base, bn_commands_dir));
        command.run_buffered({
            user: users[req.session.id], 
            command: command_run_inference, 
            args: ['--help'],
            cwd: path.join(__base, bn_commands_dir),
            // callback: function (code, signal) {
            //     exitCodeResponse(code, signal, req, res)
            // }
            callback: function (err, stdout, stderr){
                if (err)
                    res.status(500).send(stderr);
                else
                    res.send(stdout);
            }
        });
    });

    // get node names from vector attribute table using the command vectornodes
    router.post('/get_vector_nodes', multer_post_params, function(req, res) {
        
        Log(' *** get_vector_nodes ***');
        Log(req.body);

        // GDB files: no need to decompress while using OpenFileGDB driver

        if (!req.files['vector']){
            res.status(500).send('ERROR: No vector file received.');
            return;
        }

        if (path.extname(req.files['vector'][0].originalname) != '.zip'){
            res.status(500).send('ERROR: The file has no .zip extension.');
            return;
        }

        var file = req.files['vector'][0];
        var size_str = ' ('+parseInt(file.size/1024) +' KB)';
        Log("Vector file: "+file.originalname + size_str);

        command.run_buffered({
            user: users[req.session.id], 
            command: path.join(__base, bn_commands_dir, command_vectornodes), 
            args: [file.path],
            cwd: __base + bn_commands_dir,
            callback: function (err, stdout, stderr) {
                if (err){       

                    res.status(500).send(stderr+'Zipped .gdb directories (with .gdb.zip extension), must contain a .gdb directory at their first level.');
                    Log(stderr, 'error');

                }
                else{
                    res.send(stdout);
                }
            }
        });
    });

    router.post('/get_net_info', multer_post_params, function(req, res) {

        Log(' *** get_net_info ***');
        Log(req.body);
        
        var stdout_array = [];

        async.eachLimit(req.files['by_network'], 10, function(file, callback){

            Log(path.join(__base, bn_commands_dir, command_get_info) + ' ' + file.path + '\n');
            command.run_buffered({
                user: users[req.session.id], 
                command: path.join(__base, bn_commands_dir, command_get_info), 
                //args: [path.join(__base, file.path)],
                args: [file.path],

                env: { 
                    PATH: path.join (__base, bn_commands_dir), 
                    NETICA_LIC:config.netica_license
                },
                
                callback: function (err, stdout, stderr) {
                    try{
                        if (!err)
                            stdout_array.push(JSON.parse(stdout));
                    }
                    catch(err){
                        Log(err, 'error');
                    }

                    callback(stderr);
                },
            });
        }, function (err){
            if (err)         
                res.status(500).send(err);
            else{
                stdout_array[0].nodes = stdout_array[0].nodes.filter(function(e){ return (e.name.slice(0,4) != 'NOTE') && (e.name.slice(0,5) != 'TITLE')});
                res.render('network_view', { 
                    network: stdout_array[0], 
                    mode: req.body.mode
                });
            }
        });
    });

    router.post('/run_inference', checkRequest, multer_post_params, function(req, res) {
        Log(' *** run_inference ***');
        Log(JSON.stringify(req.body, null, 4));
      
        if (req.files['by_network']){
            var size_str = ' ('+parseInt(req.files['by_network'][0].size/1024) +' KB)';
            Log("BN network: " + req.files['by_network'][0].originalname + size_str);
        }
        else if (req.body.by_network){      // in case of newer version 2 step
            if (typeof req.body.by_network == 'string')
                req.body.by_network = [req.body.by_network];

            for  (var i=0; i< req.body.by_network.length; i++){
                //var stats = fs.statSync(rasterdir + req.body.by_network[i]);
                var stats = fs.statSync(path.join(__base, rasterdir, sanitize_filename(req.session.id), req.body.by_network[i])); //same place where it was sotred in multer destination
                var size_str = ' ('+ stats.size/1024 +' KB)';
                Log("BN network: " + req.body.by_network[i] + size_str);
            }
        }
        else
            Log("No bayesian netowrk file supplied", "error");

        var vectors = [];

        // for rasters 
        if (typeof req.body['filename_nodename'] == 'string' )
            req.body['filename_nodename'] = [req.body['filename_nodename']];

        var filename_nodename = [];
        if (req.body['filename_nodename']){
            for  (var i=0; i< req.body['filename_nodename'].length; i++)
                filename_nodename.push(req.body['filename_nodename'][i].split('='));
        }

        if (!req.files['vector'])
            req.files['vector'] = [];

        for (var i=0; i< req.files['vector'].length; i++){
            var size_str = ' ('+parseInt(req.files['vector'][i].size/1024) +' KB)';
            Log("Vector file: "+req.files['vector'][i].originalname + size_str);
            vectors.push('-s', path.join(__base, req.files['vector'][i].path ));
        }

        var disabled_vectornodes = [];
        if (req.body['disabled_vectornodes']){ // to only use this nodes in case the vector file contains more
            if (typeof req.body['disabled_vectornodes'] == 'string' )
                req.body['disabled_vectornodes'] = [req.body['disabled_vectornodes']];
            for (var i =0; i < req.body['disabled_vectornodes'].length; i++){
                var vector_nodename = sanitize(req.body['disabled_vectornodes'][i]);
                disabled_vectornodes.push('-z', vector_nodename)
            }
        }

        var evidences = [];
        var linkings = [];
        var likelihoods = [];

        if (req.body['evidence']){
            if (typeof req.body['evidence'] == 'string' )
                req.body['evidence'] = [req.body['evidence']];
            for (var i =0; i < req.body['evidence'].length; i++){
                var evidence = sanitize(req.body['evidence'][i]);
                evidences.push('-e', evidence);
            }          
        }
        if (req.body['likelihood']){
            if (typeof req.body['likelihood'] == 'string' )
                req.body['likelihood'] = [req.body['likelihood']];
            for (var i =0; i < req.body['likelihood'].length; i++){
                var likelihood = sanitize(req.body['likelihood'][i]);
                likelihoods.push('-k', likelihood);
            }          
        }
        if (req.body['linking']){
            if (typeof req.body['linking'] == 'string' )
                req.body['linking'] = [req.body['linking']];
            for (var i =0; i < req.body['linking'].length; i++){
                var linking = sanitize(req.body['linking'][i]);
                linkings.push('-l', linking)
            }
        }
        Log('Explicit evidences: '+JSON.stringify(evidences));
        Log('Node linking: '+JSON.stringify(linkings));
       
        var args = [];
        args = args.concat(['-d', path.join(__base, output_dir, sanitize_filename(req.session.id))]);
        args = args.concat(evidences);
        args = args.concat(likelihoods);
        args = args.concat(linkings);
        args = args.concat(vectors);
        args = args.concat(disabled_vectornodes);

        if (req.body.iterations)
            args = args.concat(['-i', req.body.iterations]);

        if (req.files['py_module']){
           args = args.concat(['-m', req.files['py_module'][0].filename.slice(0,-3)]); // remove the extension '.py'
        }

        if (req.body['inter_input_node']){

            if (typeof req.body['inter_input_node'] == 'string' )
                req.body['inter_input_node'] = [req.body['inter_input_node']];
            for  (var i=0; i< req.body['inter_input_node'].length; i++)
                args = args.concat(['-n', sanitize(req.body['inter_input_node'][i])]);
        }

        if (req.files['raster_dbf']){
            for (var i=0; i< req.files['raster_dbf'].length; i++){
                args = args.concat(['-f', path.join(__base, req.files['raster_dbf'][i].path ) ]) ;
            }
        }

        if (req.files['by_network']){
           args = args.concat(['-b', req.files['by_network'][0].path]);
        }

        else if (req.body.by_network){

            for  (var i=0; i< req.body.by_network.length; i++){
                args = args.concat(['-b', path.join(__base, rasterdir, sanitize_filename(req.session.id), req.body.by_network[i]) + ':1']);
            }
        }

        // Multiple target node support
     
        if (Array.isArray(req.body['target_node']))
            args = args.concat([req.body['target_node'].join(',')]);  // comma separated
        else
            args = args.concat([req.body['target_node']]);

        if (req.files['raster']){
           
            for (var i=0; i< req.files['raster'].length; i++){
                for (var j=0; j< filename_nodename.length; j++){
                    if (req.files['raster'][i].originalname == filename_nodename[j][0]){
                        args = args.concat(req.files['raster'][i].path + '=' + sanitize(filename_nodename[j][1]));
                        filename_nodename.splice(j, 1);
                        break;
                    }
                }
            }
        }

        Log('Command: '+ command_run_inference);
        Log('Args: '+ JSON.stringify(args));

        if (req.body.notifEmail)
            Log('Notification email will be sent to: '+ req.body.notifEmail);

        Log(command_run_inference + ' ' + args.join(' ') + '\n');

        var vector_mode = !req.files['raster'] || (req.files['raster'].length == 0)
        Log('Vector mode: '+ vector_mode);

        historial.append((req.body.notifEmail?'('+req.body.notifEmail+') ':'')+command_run_inference + ' ' + args.join(' ') + ': ');

        command.run_live({
            user: users[req.session.id], 
            command: command_run_inference, 
            args: args, 
            cwd: path.join(__base, bn_commands_dir),
            env: {
                PYTHONPATH : path.join(__base, bn_commands_dir) +':'+ path.join(__base, py_modules_dir),
                NETICA_LIC: config.netica_license
            },
            callback: function (code, signal, cmd_output) {
                
                if(req.body.notifEmail){
                    sendNotificationEmail(req.body.notifEmail, code, signal, req.session.id, req.body);
                    historial.append(req.body.notifEmail);
                }
                historial.append(': '+ code + '\n');

                if (code ==0){
                    processOutputFiles(req.session.id, vector_mode, req.body, cmd_output, function(err){ 
                        if (err)
                            Log(err, 'error');
                        exitCodeResponse(code || err, signal, req, res)
                    });
                }
                else{
                    exitCodeResponse(code);
                }
            }
        });
    });
    return router;
}

/*
    In case there are more than one output file => zip them
    In vector mode all files in run are shapefiles directories
    report is an object (req.body) with all the command arguments
*/
function processOutputFiles(session_id, vector_mode, report, cmd_output, callback){

    var output_dir = path.join(__base, config.output_dir, sanitize_filename(session_id));
    var files = fs.readdirSync(output_dir);
    
    Log('processOutputFiles: output_dir:' +output_dir);
   
    if (vector_mode || (files.length > 1)){

        var filename = path.join(output_dir, sanitize_filename(session_id) + '.zip');
        var output = fs.createWriteStream(filename);
        var archive = archiver('zip', { zlib: { level: 9 } });

        // listen for all archive data to be written
        output.on('close', function() {
            Log(archive.pointer() + ' total bytes');
            Log('archiver has been finalized and the output file descriptor has closed: ' + filename);
            callback();
        });

        // good practice to catch this error explicitly
        archive.on('error', function(err) {
            Log(err,'error');
            callback(err);
            //throw err;
        });

        // pipe archive data to the file
        archive.pipe(output);

        for (var i =0; i < files.length; i++){
        
            Log("processOutputFiles: "+files[i]);

            if (!(files[i].slice(-4) == '.zip')){

                if(vector_mode)
                    archive.directory(path.join(output_dir, files[i]), files[i]);
                else
                    archive.file(path.join(output_dir, files[i]), {name: files[i]});
            }
        }
        archive.file(path.join(__base, config.readme),  {name: 'README.txt'});
        report.date = new Date().toLocaleString();
        archive.append(JSON.stringify(report, null, 4),  {name: 'report.txt'});
        archive.append(cmd_output,  {name: 'output.txt'});
        archive.finalize();
    }
    else
        callback();
}


