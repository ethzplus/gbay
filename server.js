var fs = require('fs');

var express = require('express');
var app = express();

var http = require('http');


//var webserver = http.Server(app);

var config = require('./config.js');
var path = require('path');
var favicon = require('serve-favicon');
var morgan = require('morgan');  // HTTP request logger middleware 
var logger = require('./lib/logger.js');
var Log = logger.Log;
var bodyParser = require('body-parser');
var cookieParser = require('cookie-parser');
var pug = require('pug');
var expressSession = require('express-session');

var sharedsession = require("express-socket.io-session");

var compress = require('compression');
var helmet = require('helmet');

global.__base = __dirname + '/';   // prevent submodules to "require(../../../)"

var io;
var session = expressSession({
    secret: 'Hoy he visto un lindo gatito',
    name: 'blumap.sid',
    resave: true,
    saveUninitialized: true,

    //store: new MongoStore({ mongooseConnection: mongoose.connection })
}); 

var users = {}; // to store users id by session id

function initApp(){

    Log("BLUMAP starting");

    Log("Node "+process.version);
   
    Log("Configuring express...");

    app.use(helmet());
    app.use(compress());

    app.set('views', './views');
    app.set('view engine', 'pug');

    app.use(express.static(path.join(__dirname, '/public')));
    //app.use(express.static(path.join(__dirname, config.output_dir)));

    app.use(morgan('combined', { "stream": logger.stream }));

    app.use(bodyParser.json()); // for parsing application/json
    app.use(bodyParser.urlencoded({ extended: true })); // parse application/x-www-form-urlencoded
    app.use(cookieParser());

    app.use(session);
    var flash = require('connect-flash');
    app.use(flash());

    var routes = require('./routes/index')(users);
    app.use('/', routes);
    var server = http.Server(app)

    server.timeout = config.server_timeout; // 30 min (prevent 502 Bad Gateway)

    io = require("socket.io")(server);
    io.use(sharedsession(session));

    io.on("connection", function(socket) {
        
        Log('A client connected: socket.id: ' + socket.id + ' session.id: '+ socket.handshake.session.id);

        users[socket.handshake.session.id] = { socket: socket };

        socket.on('disconnect', function () {
            Log('A client disconnected: socket.id: ' + socket.id + ' session.id: '+ socket.handshake.session.id);

            var user = users[socket.handshake.session.id];

            if (user){
                delete user;
            }
        });        
    });  
    Log("Reading Netica license from "+config.netica_license_filename);
    fs.readFile(config.netica_license_filename, 'utf8', function(err, license){
        if (err)
            Log(err,'error');
        else
            config.netica_license = license;
    });

    Log("Web server starting...");

    server.listen(config.listen_port, function(){
        Log('Express HTTP server listening on port :'+config.listen_port);
    });
}

initApp();




