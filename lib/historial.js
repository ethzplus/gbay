var fs = require('fs');
var config = require(__base + './config.js');
var filename = config.historial_filename; 

module.exports = {
	append: function (msg){
		fs.appendFile(filename, new Date().toUTCString() +': ' +msg, (err) => {
			if (err) throw err;
		});
	}
}