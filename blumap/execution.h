#ifndef __EXECUTION__
#define __EXECUTION__
#define LOG_EXEC_FILENAME "exec_log.txt"

typedef struct {
	
	// input
	char** nodenames;
	int nodes_len;

	float** findings;
	int* findings_len;

	// output
	int output_nodes_len;
	char** target_nodenames;
	float** beliefs_m;
	int* beliefs_len_m;
	float* expected_value_m;
	float* std_dev_m;
} Execution_t;

int initExecution(Execution_t* execution, int node_number, int output_node_number);
void freeExecution(Execution_t* execution);
int E_initInput(Execution_t* execution, char* nodename, int inputs_len );
int E_initOutput(Execution_t* execution, char* nodename, int outputs_len );
Execution_t* findExecution(Execution_t** executions, int executions_len, Execution_t execution);
int E_addNodeFinding(Execution_t* execution, char* nodename, float* findings, int findings_len);
int E_addNodeFinding_p(Execution_t* execution, char* nodename, float** findings, int findings_len);
//int E_addOutputLinking(Execution_t* execution, float* beliefs_linking, int beliefs_linking_len);
int E_setTargetBeliefs(Execution_t* execution, char* nodename, float* beliefs, int beliefs_len/*, float expected_value, float std_dev*/);
void printExecutionVerbose(Execution_t* execution, int cell_offset, int cell_total, int iteration, int iteration_total);
void printExecution(Execution_t* execution);
//void copyExecution(Execution_t* dst, Execution_t* src);
void printExecutionMetadata(Execution_t* execution);
int logExecution(Execution_t* execution, int cell_offset, int iteration);
Execution_t* copyExecution(Execution_t* execution);
int resetExecution(Execution_t* execution);

#endif