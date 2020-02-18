
var child_process = require('child_process');
var Log = require(__base + 'lib/logger.js').Log;
var config = require(__base + './config.js');
var command = config.command;

module.exports = {

    run_live : function(settings){ 

        var command = settings.command;
        var args = settings.args;
        var user = settings.user;
        var callback = settings.callback;

        var cwd = settings.cwd;
        var env = settings.env;

        if (!user)
            Log('run_live: No user', 'error');
        if (!command)
            Log('run_live: No command', 'error');
        if (!callback)
            Log('run_live: No callback', 'error');

        /* kill other process, if any */
        if (user.child)
            user.child.kill();

        user.child = child_process.spawn(command, args, {cwd: cwd, env: env });

        var cmd_output = '';

        Log('child process ('+user.child.pid +') started: '+command + ' ' + args.join(' '));

        user.child.stdout.on('data', function (chunk){
            var chunk_str = chunk.toString();
            Log(chunk_str);
            user.socket.emit('cmd_stdout', chunk_str);
            cmd_output += chunk_str;
        });

        user.child.stderr.on('data', function (chunk){
            var chunk_str = chunk.toString();
            Log(chunk_str, 'error');
            user.socket.emit('cmd_stderr', chunk_str);
            cmd_output += chunk_str;
        });

        user.child.on('close', function(code, signal){

            if (signal){
                Log('child process ('+this.pid+') terminated by signal '+signal,'warn');
                user.socket.emit('cmd_stderr','\nchild process ('+this.pid+') terminated by signal '+signal+'\n');
                cmd_output += ('Process erminated by signal '+signal);
            }
            else{
                Log('child process ('+this.pid+') exited with code '+code,((code==0)?'':'error'));            
                user.socket.emit((code==0)?'cmd_stdout':'cmd_stderr','\nchild process exited with code '+code+'\n');
                cmd_output += ('Process erminated with code '+code);
            }
            callback(code, signal, cmd_output);  
        });

    },

    // for get_net_info and get_vector_nodes
    run_buffered : function(settings){ 

        var command = settings.command;
        var args = settings.args;
        var user = settings.user;
        var callback = settings.callback;
        var env = settings.env;

        if (!user)
            Log('run_buffered: No user. Will operate without streaming.', 'warn');
        if (!command)
            Log('run_buffered: No command', 'error');
        if (!callback)
            Log('run_buffered: No callback', 'error');

        Log('run_buffered: '+ command + ' ' + args.join(' '));

        var child = child_process.execFile(command, args, {env: env}, function(err, stdout, stderr){
            if (stdout){
                Log(stdout.toString());
            }
            callback(err, stdout, stderr); 
        });

        if (user){
            if (user.childs)
                user.childs.push(child);
            else
                user.childs = [child]
        }
    }
}