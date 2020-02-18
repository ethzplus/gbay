#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include "by_network.h"

#define NETICA_LIC_ENV "NETICA_LIC" // to get license from environment. When launched from blumap
#define NETICA_LICENSE_FILENAME "Netica.lic"
#define BUFF_LEN 256

int main(int argc, char* argv[]){

	net_t* net = NULL;
	net_info_t net_info;
	int err;
	FILE* fd;
	char* license = getenv(NETICA_LIC_ENV); 
	char str_aux[BUFF_LEN];
	

	if (argc < 2){
		fprintf(stderr, "%s what?\n", argv[0]);
		return -1;
	}


	if (!license){

	    fd = fopen(NETICA_LICENSE_FILENAME, "r");
	    if (!fd){
	    	//fprintf(stderr,"Could not open file with Netica license '%s'. Running in limited mode.\n", NETICA_LICENSE_FILENAME);
	    	
	    }
	    else{
	    	if (fgets(str_aux, BUFF_LEN, fd)){
	    		license = str_aux;
	    	}
	    	else{
	    		//fprintf(stderr,"fread error\n");
	    	}
	    }
	    fclose(fd);
	}

   	err = initBayesNetworkEnv(license, 0);// be careful. blumap expects stdout to be valid JSON. If verbose is enabled, BLUMAP will not work

   	if (err < 0)
   		return err;

	net_info.net = readNet(argv[1]);
	if (net_info.net == NULL)
		return -1;
	 
	strncpy(net_info.filename, basename(argv[1]), MAX_BUFF);

	printNetworkInfoJSON(stdout, net_info);

	closeNet(net);

	closeBayesNetworkEnv(0);

	return 0;
}

