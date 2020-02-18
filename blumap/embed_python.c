#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Python.h>
#include <unistd.h>
#include <libgen.h>  // dirname() and basename(

#include "embed_python.h"

int initPythonEnv(char *python_script_name){

    Py_SetProgramName(python_script_name);  // optional but recommended 
    Py_Initialize();
    return Py_IsInitialized();
}

void finishPythonEnv(){
     Py_Finalize();
}

// Should call checkPyNodes in blumap to check if node names belong to the network, and to set node_states.
// This is necessary before calling getStateValueFromPyNode with the dst_node_list nodes
int third_party_python_script(char* python_script, char* py_third_party, char* raster_ref_filename, pyNode_t* src_node_list[], int len, pyNode_t* dst_node_list[], long iteration, int debug){


    PyObject *pName, *pModule, *pFunc, *pNode, *pList, *raster_ref, *pThird, *pIteration;
    PyObject *pArgs, *pValue;
    PyObject *pList_res;

    PyObject *pList_values_cont;

    int i, j, dst_len;
    long ret;
    
    double *values_ret;
    int values_ret_len;

    int debug_i;

    char python_func[] = "run";


    char *py_third_party_dup = strdup(py_third_party);

    if (debug){

        printf("third_party_python_script: %s %s %s\n",python_script, py_third_party, raster_ref_filename);
        printf("third_party_python_script: Both %s and %s should be able to be imported. Check PYTHONPATH in case of import errors.\n", python_script, py_third_party);
        printf("third_party_python_script: PYTHONPATH is set to: %s\n", getenv("PYTHONPATH"));

    }

    pName = PyString_FromString(python_script);
    raster_ref = PyString_FromString(raster_ref_filename);
    pThird = PyString_FromString(py_third_party);
    pIteration = PyInt_FromLong(iteration);

    pList = PyList_New(len);

    for (i = 0; i < len; i++) {


        // data is a list of chars
        if (src_node_list[i]->type == PY_DISCRETE){
            
            pNode = Py_BuildValue("[s,i,i,s#]", src_node_list[i]->nodename, src_node_list[i]->node_states, src_node_list[i]->type, src_node_list[i]->data, src_node_list[i]->data_len);

        }

        // data is a list of floats
        else if (src_node_list[i]->type == PY_CONTINUOUS){

            pList_values_cont = PyList_New(src_node_list[i]->values_len);

            for (j=0; j< src_node_list[i]->values_len; j++)
                PyList_SET_ITEM(pList_values_cont, j, PyFloat_FromDouble(src_node_list[i]->values[j]));   

            pNode = Py_BuildValue("[s,i,i]", src_node_list[i]->nodename, src_node_list[i]->node_states, src_node_list[i]->type);

            PyList_Append(pNode, pList_values_cont);

        }

        if (!pNode) {
            Py_DECREF(pList);
            return -1;
        }

        PyList_SET_ITEM(pList, i, pNode);   
    }

    if (!pList)
        fprintf(stderr, "Py_BuildValue: error");

    pList_res = PyList_New(0);

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, python_func);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {

            pArgs = PyTuple_New(5);

            PyTuple_SetItem(pArgs, 0, pThird);
            PyTuple_SetItem(pArgs, 1, raster_ref);
            PyTuple_SetItem(pArgs, 2, pList);
            PyTuple_SetItem(pArgs, 3, pIteration);
            PyTuple_SetItem(pArgs, 4, pList_res);


            pValue = PyObject_CallObject(pFunc, pArgs);

            printf("third_party_python_script: Function finished\n");

            // Py_DECREF(pArgs);  // not anymore. We trust the final argument (pList_res) to be the result list
            if (pValue != NULL) {

                ret = PyInt_AsLong(pValue);

                //printf("third_party_python_script: Result of call: %ld\n", ret);

                if (ret == -1){
                    fprintf(stderr, "\nthird_party_python_script: ERROR EXECUTING SCRIPT. PLEASE CHECK ERROR MESSAGES ABOVE.\n\n");
                }

                dst_len = PyList_Size(pList_res);

                printf("third_party_python_script: Number of elements in returned list: : %d\n", dst_len);
                
                for (i = 0; i < dst_len; i++) {

                    pNode = PyList_GetItem(pList_res, i);

                    if (!pNode){
                        printf("third_party_python_script: PyList_GetItem (%d) error\n", i);
                        return -1;
                    }


                   

                    if (!(PyList_GetItem(pNode, 0)) || !(PyList_GetItem(pNode, 1))){
                        fprintf(stderr, "third_party_python_script: Result list: Malformed item\n");
                        return -1;
                    }

                    

                    // every item should be: [char* nodename, char* data]
                    if ( PyString_Check(PyList_GetItem(pNode, 1))){  // it is a string --> Discrete node
                        dst_node_list[i] = createPyNode(PyString_AsString(PyList_GetItem(pNode, 0) ), 0, PyString_AsString(PyList_GetItem(pNode, 1) ), PyString_Size(PyList_GetItem(pNode, 1)), PY_DISCRETE);

                        if (debug){
                            printf("third_party_python_script: Discrete: Size of data is %d\n", (int)PyString_Size(PyList_GetItem(pNode, 1) ));
                        }
                    }

                    // every item should be: [char* nodename, <python list>Array of float values ]
                    else if ( PyList_Check(PyList_GetItem(pNode, 1))){ // it is a list of floats --> Continuous node

                        values_ret_len = PyList_Size(PyList_GetItem(pNode, 1));

                        values_ret = (double*)malloc(sizeof(double) *values_ret_len );
                        if (!values_ret){
                            fprintf(stderr, "third_party_python_script: malloc values_ret\n");
                            return -1;
                        }

                        if (debug){
                            printf("third_party_python_script: Continuous: Size of data is %d\n", sizeof(float) * values_ret_len);
                        }

                        for (j=0; j< values_ret_len; j++){
                            values_ret[j] = PyFloat_AsDouble(PyList_GetItem(PyList_GetItem(pNode, 1), j)); 
                        }

                        dst_node_list[i] = createPyNode(PyString_AsString(PyList_GetItem(pNode, 0) ), 0, values_ret, values_ret_len, PY_CONTINUOUS);

                        free(values_ret);  // it is copied in createPyNode

                    }
                }

                Py_DECREF(pArgs);
                Py_DECREF(pValue);
            }
            else {

                Py_DECREF(pArgs);
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
                return - 1;
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", python_func);

            return -1;
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", python_script);
        return -1;
    }

    printf("third_party_python_script: Finished\n");

    return dst_len;
}

pyNode_t* createPyNode(char* nodename, int node_states, void* data, int data_len, int node_type){

    int i;

    pyNode_t *pyNode;
    pyNode = (pyNode_t*)malloc(sizeof(pyNode_t));

    pyNode->nodename = strdup(nodename);
    
    if (!pyNode->nodename){
        fprintf(stderr, "createPyNode: strdup\n");
        return NULL;
    }

    pyNode->type = node_type;
    pyNode->node_states = node_states;
    pyNode->data_len = 0;
    pyNode->values_len = 0;

    pyNode->data=NULL;
    pyNode->values=NULL;

    if (node_type == PY_DISCRETE){

        pyNode->data = (unsigned char*)malloc(data_len);

        if (!pyNode->data){
            fprintf(stderr, "createPyNode: malloc\n");
            return NULL;
        }

        if (data){
            memcpy(pyNode->data, data, data_len);
            pyNode->data_len = data_len;
        }
    }
    else if (node_type == PY_CONTINUOUS){

        pyNode->values = (double*)malloc(sizeof(double)*data_len);

        if (!pyNode->values){
            fprintf(stderr, "createPyNode: malloc\n");
            return NULL;
        }

        if (data){
            memcpy(pyNode->values, (double*)data, sizeof(double)*data_len);
            pyNode->values_len = data_len;
        }
    }
   

    
    return pyNode;
}


void freePyNode(pyNode_t* pyNode){

    if (pyNode->nodename) free(pyNode->nodename);
    if (pyNode->data) free(pyNode->data);
    if (pyNode->values) free(pyNode->values);
    if (pyNode) free(pyNode);

}

// for discrete nodes
int pyAddData(pyNode_t* pyNode, char *buff, int len){

    memcpy(pyNode->data + pyNode->data_len, buff, len);
    pyNode->data_len += len;
    return pyNode->data_len;
}

// for continuous nodes
int pyAddValue(pyNode_t* pyNode, double val){

    pyNode->values[pyNode->values_len] = val;
    pyNode->values_len++;
    return pyNode->values_len;
}

void resetPyNode(pyNode_t* pyNode){

    pyNode->data_len = 0;
    pyNode->values_len = 0;

}

// to get vales of a CONTINUOUS node
float getValueFromPyNode(pyNode_t* pyNode, int cell){

    if (!pyNode->node_states){
        fprintf(stderr, "getValueFromPyNode: WARN node_states is zero. Should call checkPyNodes before calling this function\n");
        return -1;
    }
    if (pyNode->type == PY_DISCRETE){
        fprintf(stderr, "getValueFromPyNode: ERROR: The node is DISCRETE type. It has no 'Value'.\n");
        return -1;
    }
    else if (pyNode->type == PY_CONTINUOUS){
        return (float)(pyNode->values[cell]);
    }
    else{
        fprintf(stderr, "getValueFromPyNode: WARN pyNode with unknown type.\n");
        return -1;
    }

}

// to get state likelihood of a DISCRETE node
float getStateValueFromPyNode(pyNode_t* pyNode, int cell, int state){

    if (!pyNode->node_states){
        fprintf(stderr, "getStateValueFromPyNode: WARN node_states is zero. Should call checkPyNodes before calling this function\n");
        return -1;
    }

    if (pyNode->type == PY_DISCRETE)
        return (float)(pyNode->data[cell * pyNode->node_states + state]);
    else if (pyNode->type == PY_CONTINUOUS){
        fprintf(stderr, "getStateValueFromPyNode: ERROR: The node is CONTINUOUS type. pyNode does not store values per state.\n");
        return -1;
    }
    else{
        fprintf(stderr, "getStateValueFromPyNode: WARN pyNode with unknown type.\n");
        return -1;
    }
}


void getBeliefsFromPyNode(pyNode_t* pyNode, int cell, float** beliefs){

    int i;

    for (i=0; i < pyNode->data_len; i++)
        (*beliefs)[i] = pyNode->data[cell * pyNode->data_len + i] / 100.0;

    return;

}

void printPyNode(pyNode_t* pyNode, int cell){

    int i;
    
    if (pyNode->type == PY_DISCRETE){

        printf("printPyNode: %s: node_states: %d, data_len: %d, cell %d beliefs: [", pyNode->nodename, pyNode->node_states, pyNode->data_len, cell);

        for (i=0; i< pyNode->node_states; i++)
            //printf("%d%c", pyNode->data[cell * pyNode->node_states +i], (i<pyNode->node_states-1)?' ':']');
            printf("%d%c", (int)getStateValueFromPyNode(pyNode, cell, i), (i<pyNode->node_states-1)?' ':']');
        printf("\n");
    }
    else if (pyNode->type == PY_CONTINUOUS){

        printf("printPyNode: %s: node_states: %d, data_len: %d, cell %d value: %f\n", pyNode->nodename, pyNode->node_states, pyNode->data_len, cell, pyNode->values[cell]);

    }

    return;
}