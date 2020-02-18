configuration = {
	
	listen_port: 9500,
	server_timeout : 18000000, // 30 min (prevent 502 Bad Gateway)
	rasterdir: 'rasterdir/',
	vectordir: 'vectordir/',
	py_modules_dir: 'thirdparty/',
	bn_commands_dir : 'blumap/',
	command_run_inference : 'blumap',
	command_get_info : 'net2json',
	command_vectornodes : 'vectornodes',
	output_dir : 'results',
	max_BN_files : 10,
	max_raster_files: 16,
	gui: {
		evidences: 15,
	},
	tmp_dir : '/tmp/blumap', // directory to decompress vector files
	// Application URL. Used for the link sent via e-mail for user verification. Change it for local developments
	//url: 'http://localhost:9500',
	url: 'http://gbay.ethz.ch',
	amilfrom: 'MAILFROM'
	mail_settings : {
	    host: 'HOST',
	    auth: {
	        user: 'USER',
	        pass: 'PASSWORD'
	    }
	},
	historial_filename: 'history.log',
	readme: 'docs/README.txt', // file to be included in the .zip file with all the iutputs
	netica_license_filename: 'PATH_TO_LICENSE',
}

module.exports = configuration;