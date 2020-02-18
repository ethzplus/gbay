#ifndef __EMBED_PYTHON_H__
#define __EMBED_PYTHON_H__


// This value will be passed for every state in a node likelihood to mean
// that the raster cell is NODATA
#define PY_NODATA_VALUE 255 // It must be the same as in python_c.py

#define PY_DISCRETE 0
#define PY_CONTINUOUS 1

typedef struct {
    char* nodename;
    int node_states; // not filled in when calling createPyNode in function 'third_party_python_script'
    int data_len;
    unsigned char *data;
    // for continuous nodes, to get the real values too
    int type;
   	
   	int values_len;
   	double *values; // to store the real values for continuous nodes
} pyNode_t;


/*
	Runs 'python_script'
*/
int third_party_python_script(char* python_script, char* py_third_party, char* raster_ref_filename, pyNode_t* src_node_list[], int len, pyNode_t* dst_node_list[], long iteration, int debug);

pyNode_t* createPyNode(char* nodename, int node_states, void* data, int data_len, int node_type);


void freePyNode(pyNode_t* py_node_info);

int pyAddData(pyNode_t* py_node_info, char *buff, int len);
int pyAddValue(pyNode_t* pyNode, double val);

void resetPyNode(pyNode_t* pyNode);
float getValueFromPyNode(pyNode_t* pyNode, int cell);
float getStateValueFromPyNode(pyNode_t* pyNode, int cell, int state);
void getBeliefsFromPyNode(pyNode_t* pyNode, int cell, float** beliefs);
void printPyNode(pyNode_t* pyNode, int cell);

int initPythonEnv(char *python_script_name);
void finishPythonEnv(void);


#endif