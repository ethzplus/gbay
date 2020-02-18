#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "execution.h"

int main(){

	Execution_t ex, ex2, ex3;
	Execution_t* executions, *aux;

	float node0_beliefs[] = {6.0, 4.0, 5.0};
	float node1_beliefs[] = {2.0};
	float node2_beliefs[] = {1.0, 2.0, 6.0, 1.0};
	float nodeT_beliefs[] = {0.3, 0.7};
	
	initExecution(&ex, 3);
	E_addNodeFinding(&ex, "node0", node0_beliefs, 3);
	E_addNodeFinding(&ex, "node1", node1_beliefs, 1);
	E_addNodeFinding(&ex, "node2", node2_beliefs, 4);
	E_setTargetBeliefs(&ex, nodeT_beliefs, 2);

	executions = &ex;

	initExecution(&ex2, 3);
	E_addNodeFinding(&ex2, "node0", node0_beliefs, 3);
	E_addNodeFinding(&ex2, "node1", node1_beliefs, 1);
	E_addNodeFinding(&ex2, "node2", node1_beliefs, 4);

	printExecution(executions);
	printf("\n");
	printExecution(&ex);
	printf("\n");


	aux = findExecution(&executions, 1, ex);

	if (aux)
		printExecution(aux);
	else
		printf("Not found\n");

	copyExecution(&ex3, &ex);

freeExecution(&ex);
freeExecution(&ex2);

	

freeExecution(&ex3);

	printf("\n");

	return 0;

}
