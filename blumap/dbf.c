#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include "dbf.h"

/* 
	Takes the node name from 'filename'. Every entry is supposed to be ""node_state_name:raster_value""
*/
int DBF_Open(char* filename, DBF* dbf){
	
	FILE *fd;
	char buffer[MAX_LINE];
	char str_aux[128];
	int dbf_c = 0;
	char state_name[MAX_STATE_NAME];
	int n, ret_val;

	fd = fopen(filename, "r");
	if (!fd){
		fprintf(stderr, "%s: Error\n ",__FUNCTION__);
		perror(filename);
		return DBF_ERR;
	}

	fclose(fd);

	if (strlen(filename) > MAX_DBF_FILENAME){
		fprintf(stderr, "%s: %s: filename too long\n", __FUNCTION__, filename);
		return DBF_ERR;
	}

	sprintf(buffer, "dbview -tb %s", filename);

	fd = popen(buffer, "r");
	if (fd == NULL) {
		fprintf(stderr, "popen error\n" );
		return DBF_ERR;
	}

	dbf->filename = strdup(basename(filename));

	n = sscanf(dbf->filename, "%128[^.]", str_aux);
	if (n != 1){
		perror("sscanf");
		return DBF_ERR;
	}
	dbf->nodename = strdup(str_aux);

	while (fgets(buffer, sizeof(buffer)-1, fd) != NULL) {

		n = sscanf(buffer, "%u:%*d:%[^:]:\n", &(dbf->entries[dbf_c].value), state_name);

		if (n != 2){
			fprintf(stderr, "%s: sscanf: error scanning dbview output for the given DBF file.\n", __FUNCTION__);

			ret_val = pclose(fd);
			fprintf(stderr, "%s: The exit status is: %d\n", __FUNCTION__, WEXITSTATUS(ret_val));

			return DBF_ERR;
		}

		dbf->entries[dbf_c].state_name = strdup(state_name);
		dbf_c++;
	}

	dbf->entries_n = dbf_c;

	pclose(fd);

	return dbf_c;
}

void DBF_Print(DBF dbf){
	unsigned int i;

	printf("DBF filename: %s\n", dbf.filename);
	printf("DBF nodename: %s\n", dbf.nodename);

	for (i=0; i < dbf.entries_n; i++){
		printf("%d : %s\n", dbf.entries[i].value, dbf.entries[i].state_name);
	}
}

void DBF_Free(DBF dbf){

	unsigned int i;

	if(dbf.filename)
		free(dbf.filename);
	if(dbf.nodename)	
		free(dbf.nodename);

	for (i=0; i < dbf.entries_n; i++)
		free(dbf.entries[i].state_name);
	
	return;
}

int DBF_getValueforStateName(DBF dbf, char* state_name){

	unsigned int i;

	for(i =0; i < dbf.entries_n; i++){
		if ( !strcmp(dbf.entries[i].state_name, state_name))
			return dbf.entries[i].value;
	}

	return DBF_ERR;

}
