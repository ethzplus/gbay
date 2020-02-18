#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "execution.h"

int sameCharArray(char** array1, char** array2, int len);
int sameFloatArray(float* array1, float* array2, int len);
int sameFloatArrayArray(float** array1, float** array2, int array_len, int* array_content_len);

// init to the maximun of nodes that will be needed
int initExecution(Execution_t* execution, int input_node_number, int output_node_number){

	int i;

	if (!execution)
		return -1;

	execution->nodes_len = 0;
	execution->output_nodes_len = 0;
	execution->nodenames = NULL;
	execution->findings = NULL;
	execution->findings_len = NULL;

	if (input_node_number){
		execution->nodenames = (char**)malloc(sizeof(char*)*input_node_number);
		if (!execution->nodenames)
			return -1;

		execution->findings = (float**)malloc(sizeof(float*)*input_node_number);
		if (!execution->findings)
			return -1;

		execution->findings_len = (int*)malloc(sizeof(int)*input_node_number);
		if (!execution->findings_len)
			return -1;

		for (i=0; i < input_node_number; i++){
			execution->nodenames[i] = NULL;
			execution->findings[i] = NULL;
		}		
	}

	// multiple target node 
	execution->target_nodenames = (char**)malloc(sizeof(char*)*output_node_number);
	if (!execution->target_nodenames)
		return -1;

	execution->beliefs_m = (float**)malloc(sizeof(float*)*output_node_number);
	if (!execution->beliefs_m)
		return -1;

	execution->beliefs_len_m = (int*)malloc(sizeof(int)*output_node_number);
	if (!execution->beliefs_len_m)
		return -1;

	execution->expected_value_m = (float*)malloc(sizeof(float)*output_node_number);
	if (!execution->expected_value_m)
		return -1;

	execution->std_dev_m = (float*)malloc(sizeof(float)*output_node_number);
	if (!execution->std_dev_m)
		return -1;

	for (i=0; i < output_node_number; i++){
		execution->beliefs_m[i] = NULL;
		execution->target_nodenames[i] = NULL;
		execution->beliefs_len_m[i] = -1;
		execution->expected_value_m[i] = -1;
		execution->std_dev_m[i] = -1;
	}		

	return 0;
}

int E_initInput(Execution_t* execution, char* nodename, int inputs_len ){

	int i,j;

	for( i=0; i<execution->nodes_len && execution->nodenames[i]; i++ ){
		
		// To avoid having the same node several times 
		if (!strncmp(execution->nodenames[i], nodename, strlen(execution->nodenames[i]))){
		
			printf("E_initInput: Skipping node %s. It is already initialized (input raster and linking destination)\n", nodename);

			return 0;
		}
	}

	execution->nodenames[i] = strdup(nodename);

	execution->findings[i] = (float*)malloc(sizeof(float)*inputs_len);
	if (!execution->findings[i]){
		fprintf(stderr,"E_initInput: Error: %s %d\n", nodename, inputs_len);
		perror("E_initInput: malloc");
		return -1;
	}

	for (j=0; j < inputs_len; j++)
		execution->findings[i][j] = -1;

	execution->findings_len[i] = inputs_len;

	execution->nodes_len++;

	return 0;

}

int E_initOutput(Execution_t* execution, char* nodename, int outputs_len ){

	int i;

	for( i=0; i < execution->output_nodes_len && execution->target_nodenames[i]; i++ ){

		//printf("- E_initOutput: %s %s\n", execution->target_nodenames[i], nodename);

		// To avoid having the same nodes several times 
		if (!strncmp(execution->target_nodenames[i], nodename, strlen(execution->target_nodenames[i]))){

			printf("\tSkipping same node %s, already initialized (output raster / linking node)\n", nodename);
			return 0;
		}

	}

	execution->target_nodenames[i] = strdup(nodename);
	execution->beliefs_len_m[i] = outputs_len;
	execution->beliefs_m[i] = (float*)malloc(sizeof(float)*outputs_len);
	if (!execution->beliefs_m[i]){
		perror("E_initOutput: malloc");
		return -1;
	}
	
	execution->output_nodes_len++;
	return 0;
}

// just reset the values to reuse the execution for the next cell
int resetExecution(Execution_t* execution){

	int i,j;

	if (!execution){
		return -1;
	}

	for (i=0; i <execution->nodes_len; i++){
		
		for (j=0; j < execution->findings_len[i]; j++){
			execution->findings[i][j] = -1;
		}

		execution->findings_len[i] = 0;
	}

	for (i=0; i <execution->output_nodes_len; i++){
		for (j=0; j < execution->beliefs_len_m[i]; j++)
			execution->beliefs_m[i][j] = -1;

		execution->beliefs_len_m[i] = 0;
		
	}
	return 0;
}


Execution_t* findExecution(Execution_t** executions, int executions_len, Execution_t execution){

	int index;

	for (index=0; index < executions_len; index++){

		if ((executions[index]->nodes_len == execution.nodes_len)){

			if (sameCharArray(executions[index]->nodenames, execution.nodenames, executions[index]->nodes_len) && sameFloatArrayArray(executions[index]->findings, execution.findings, executions[index]->nodes_len, executions[index]->findings_len))
			 	return executions[index];
		}
	}

	return NULL;
}





int sameCharArray(char** array1, char** array2, int len){

	int i;
	for (i=0; i < len; i++){

		if (!array2[i])
			return 0;

		if (strncmp(array1[i], array2[i], strlen(array1[i])))
			return 0;
	}
	return 1;
}

int sameFloatArray(float* array1, float* array2, int len){
	
	int i;
	for (i=0; i < len; i++){
		if (array1[i] != array2[i])
			return 0;
	}
	return 1;
}

int sameFloatArrayArray(float** array1, float** array2, int array_len, int* array_content_len){
	
	int i;
	for (i=0; i < array_len; i++){
		if (!sameFloatArray(array1[i], array2[i], array_content_len[i]))
			return 0;
	}
	return 1;
}


int E_addNodeFinding(Execution_t* execution, char* nodename, float* findings, int findings_len){


	int i;

	if (!execution){
		return -1;
	}

	for( i=0; i<execution->nodes_len && execution->nodenames[i]; i++ ){
		if (!strncmp(execution->nodenames[i], nodename, strlen(execution->nodenames[i]))){

			execution->findings_len[i] = findings_len;

			memcpy(execution->findings[i], findings, sizeof(float)*findings_len);
			return 0;
		}
	}

	fprintf(stderr, "E_addNodeFinding: Error: no node name '%s' found in execution.\n", nodename);

	return -1;
}

// Same as E_setNodeFinding but findings is an array of pointers to data
int E_addNodeFinding_p(Execution_t* execution, char* nodename, float** findings, int findings_len){

	int i, j;

	if (!execution){
		return -1;
	}

	for( i=0; i<execution->nodes_len && execution->nodenames[i]; i++ ){

		if (!strncmp(execution->nodenames[i], nodename, strlen(execution->
			nodenames[i]))){

			execution->findings_len[i] = findings_len;

			for (j=0; j < findings_len; j++)
				execution->findings[i][j] = *(findings[j]);
			return 0;
		}
	}
	fprintf(stderr, "E_addNodeFinding_p: Error: no node name '%s' found in execution.\n", nodename);
	return -1;
}

void freeExecution(Execution_t* execution){

	int i;

	for (i =0; i < execution->nodes_len; i++){

		if(execution->nodenames[i]){
			free(execution->nodenames[i]);
			execution->nodenames[i] = NULL;
		}

		if(execution->findings[i]){
			free(execution->findings[i]);
			execution->findings[i] = NULL;
		}
		if (execution->findings_len){
			free(execution->findings_len);
			execution->findings_len = NULL;
		}
	}

	for (i=0; i < execution->output_nodes_len; i++){
		if (execution->beliefs_m[i]) free(execution->beliefs_m[i]);
		if (execution->target_nodenames[i]) free(execution->target_nodenames[i]);
	}


	if (execution->nodenames) free(execution->nodenames);
	if (execution->target_nodenames) free(execution->target_nodenames);
	if (execution->findings) free(execution->findings);
	if (execution->findings_len) free(execution->findings_len);

	if (execution->beliefs_m) free(execution->beliefs_m);
	if (execution->beliefs_len_m) free(execution->beliefs_len_m);
	if (execution->expected_value_m) free(execution->expected_value_m);
	if (execution->std_dev_m) free(execution->std_dev_m);

	if (execution) free(execution);
	
}


/*
If beliefs is NULL, it means NODATA, and it will write -1 in every node state
*/
int E_setTargetBeliefs(Execution_t* execution, char* nodename, float* beliefs, int beliefs_len/*, float expected_value, float std_dev*/){

	int i,j;

	for( i=0; i < execution->output_nodes_len && execution->target_nodenames[i]; i++ ){

		if (!strncmp(execution->target_nodenames[i], nodename, strlen(execution->target_nodenames[i]))){

			if(beliefs == NULL){ // NODATA 

				for(j=0; j < beliefs_len; j++)
					execution->beliefs_m[i][j] = -1;
			}
			else{
				memcpy(execution->beliefs_m[i], beliefs, sizeof(float)*beliefs_len);
			}

			execution->beliefs_len_m[i] = beliefs_len;

			return 0;
		}

	}

	fprintf(stderr, "E_setTargetBeliefs: Error: no target node name '%s' found in execution.\n", nodename);

	return -1;

}
void printExecutionMetadata(Execution_t* execution){

	int i;

	if (!execution){
		printf("printExecutionMetadata: execution is NULL\n");
		return;
	}

	printf("printExecutionMetadata: nodes_len: %d\n", execution->nodes_len);
	for (i =0; i < execution->nodes_len; i++){
		printf("printExecutionMetadata: findings_len[%d]: %d\n", i, execution->findings_len[i]);
	}

	printf("printExecutionMetadata: output_nodes_len: %d\n", execution->output_nodes_len);
	for (i =0; i < execution->output_nodes_len; i++){
		printf("printExecutionMetadata: beliefs_len_m[%d]: %d\n", i, execution->beliefs_len_m[i]);
	}

	//printf("printExecutionMetadata: linking_n: %d\n", execution->linking_n);

	return;

}

void printExecutionVerbose(Execution_t* execution, int cell_offset, int cell_total, int iteration, int iteration_total){

	printf("CELL / FEATURE: %d/%d, IT: %d/%d : verbose:  ", cell_offset, cell_total, iteration, iteration_total);
	printExecution(execution);
	return;
}

// Be aware that rasters have 1 more to the value thas is applied to a node, only when it is EVIDENCE on a DISCRETE node

// FOR DISPLAYING REASONS IT WILL NOT PRINT NODE STATES WITH VALUE OF -1 (when node is discrete so it does not print the rest of the uninitialized states)
void printExecution(Execution_t* execution){

	int i,j;

	printf("** cached ** : %d nodes: ", execution->nodes_len);
	
	for (i =0; i < execution->nodes_len; i++){

		if (execution->nodenames[i]){

			printf("(%s: [", execution->nodenames[i]);

			for (j =0; j < execution->findings_len[i]; j++){
			
				if (execution->findings[i][j] != -1){

					//printf("(int: %d,float: %.2f) ", (int)(execution->findings[i][j]), execution->findings[i][j]);
					printf("%.2f ", execution->findings[i][j]);
				}

			}

			printf("])");
		}
	}

	printf(" ->");

	for (i =0; i < execution->output_nodes_len; i++){

		printf(" (%s: [", execution->target_nodenames[i]);

		for (j =0; j < execution->beliefs_len_m[i]; j++){
			
			if (execution->beliefs_m[i][j] == -1)
					printf("NODATA ");
			else
				printf("%.2f ", execution->beliefs_m[i][j]);
		}
		
		printf("])");
	}


}


/*
Writes information at the end of the file.
*/
int logExecution(Execution_t* execution, int cell_offset, int iteration){

	int i,j;
	FILE* fd;

	//printf("logExecution\n");

	if (!execution){
		printf("logExecution: incorrect arguments\n");
		return -1;
	}

	fd = fopen(LOG_EXEC_FILENAME, "a");
	if (!fd){
		perror("fopen");
	}

	fprintf(fd, "********\ncell: %d, iteration: %d\nINPUTS:\n", cell_offset, iteration);
	for (i =0; i < execution->nodes_len; i++){

/*
		It could happen that the execution is initialzed with more nodes_len
		than the actually used, an input raster file, and an input linking node are the same node:
		INPUTS:
			lu_t0: [0.34 0.05 0.04 0.00 0.50 0.06 0.00 0.00 0.00 0.00 0.00 ]
			(null): []
*/

		if (execution->nodenames[i]){
		
			fprintf(fd, "\t%s: [", execution->nodenames[i]);

			if (execution->findings_len[i] == 1){
				fprintf(fd, "%d", (int)(execution->findings[i][0]));
			}
			else{
				for (j =0; j < execution->findings_len[i]; j++)
					fprintf(fd, "%.2f ", execution->findings[i][j]);
			}
			fprintf(fd, "]\n");
		}
	}

	fprintf(fd, "\nOUTPUTS:\n");
	for (j =0; j < execution->output_nodes_len; j++){
		fprintf(fd, "\t%s: [", execution->target_nodenames[j]);

		for (i =0; i < execution->beliefs_len_m[j]; i++){

			if (execution->beliefs_m[j][0] != -1)
				fprintf(fd, "%d ", (int)(execution->beliefs_m[j][i] * 100));
			else
				fprintf(fd, "NODATA ");
		}
		fprintf(fd, "]\n");
	}

	fclose(fd);

	return 0;
}


Execution_t* copyExecution(Execution_t* src){

	int i, j;
	Execution_t* dst;

	dst = (Execution_t*)malloc(sizeof(Execution_t));
	if (!dst){
		fprintf(stderr, "copyExecution: malloc error\n");
		return NULL;
	}

	initExecution(dst, src->nodes_len, src->output_nodes_len);

	for (i=0; i < src->nodes_len; i++){

		E_initInput(dst, src->nodenames[i], src->findings_len[i]);
		
		for (j=0; j < src->findings_len[i]; j++){
			dst->findings[i][j] = src->findings[i][j];
		}
	}

	for (i=0; i < src->output_nodes_len; i++){

		E_initOutput(dst, src->target_nodenames[i], src->beliefs_len_m[i]);

		for (j=0; j < src->beliefs_len_m[i]; j++){
			dst->beliefs_m[i][j] = src->beliefs_m[i][j];
		}

		dst->expected_value_m[i] = src->expected_value_m[i];
		dst->std_dev_m[i] = src->std_dev_m[i];
	}

	return dst;
}