var winston = require('winston');
winston.emitErrs = true;

var logger = new winston.Logger({
    transports: [
        new winston.transports.Console({
            level: 'info',
            handleExceptions: true,
            json: false,
            colorize: true,
            timestamp: true,
        })
    ],
    exitOnError: false
});

module.exports = logger;

// redirect morgan http logger
module.exports.stream = {
    write: function(message, encoding){
        logger.info(message);
    }
};

var verbose = true;

function Log(msg,level){

    if (verbose){
       logger.log(level || 'info',msg);
    }
}

module.exports.Log = Log;