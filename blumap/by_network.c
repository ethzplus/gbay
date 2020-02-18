#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json/json.h>
#include <libgen.h> // basename
#include <errno.h>
#include <math.h> // roundf

#include "by_network.h"

environ_ns* env;

int checkError();
int normalize(float** likelihood, int len);

int initBayesNetworkEnv(char* license, int verbose){

	int res;
	char mesg[MESG_LEN_ns];

	env = NewNeticaEnviron_ns (license, NULL, NULL);
	
	res = InitNetica2_bn (env, mesg);

	if (verbose){
		printf ("%s\n", mesg);
		//GetNeticaVersion_bn (env, &version);
	    //printf("Version of Netica running: %s\n", version);
	}

	return res;
}

int closeBayesNetworkEnv(int verbose){

	int res;
	char mesg[MESG_LEN_ns];
	
	res = CloseNetica_bn (env, mesg);

	if (verbose)
		printf("%s\n", mesg);

	return res;
}


net_bn* readNet(char* filename){

	net_bn* net = ReadNet_bn (NewFileStream_ns( filename, env, NULL), NO_WINDOW );

	if (checkError() != NET_OK) {
		return NULL;
	} 

	return net;
}

int CompileNet(net_bn* net){

	CompileNet_bn(net);
		
	if (checkError() != NET_OK) {
		return NET_ERR;
	} 

	return NET_OK;
}


int checkError(){
		
	report_ns* err;

	err = GetError_ns (env, ERROR_ERR, NULL);

	if (err){
		fprintf (stderr, "checkError: Error %d %s\n", ErrorNumber_ns (err), ErrorMessage_ns (err));
		return NET_ERR;
	}
	else
		return NET_OK;

}  

void closeNet(net_bn* net){ return DeleteNet_bn(net); }

void closeAllNets(net_info_t* nets_info, int n){

	int i;
	
	for (i=0; i < n; i++)
		closeNet(nets_info[i].net);

	return;
}

void PrintNodeList (const nodelist_t* nodelist){
    
    int i, numnodes = LengthNodeList_bn (nodelist);

    printf("There are %d nodes: ", numnodes );

    for (i = 0;  i < numnodes;  ++i){
	    if (i != 0) 
	    	printf (", ");

	    printf ("%s", GetNodeName_bn (NthNode_bn (nodelist, i))) ;
    }
    printf ("\n");

}



/* 
	If node is DISCRETE: value should be the node state, in the range [0..node_states-1] 
	If node is CONTINUOUS: value is the real value
*/
int setNodeFinding(node_t* node, float value){



	if (node){

		RetractNodeFindings_bn (node); // if node has aleady a finding, it will crash if not retracted

		if (getNodeType( node ) ==  DISCRETE_TYPE ){


			//printf("setNodeFinding: DISCRETE_TYPE\n");

			if ( (value >= getNodeNumberStates(node)) || (value < 0) ){
				fprintf(stderr, "setNodeFinding: Finding state %.2f does not exist for node %s (%s)\n", value, getNodeName(node), getNodeTitle(node));
				return NET_ERR;
			}

			EnterFinding_bn (node, (int)value); // ignore decimal part
			
		}

		else if (getNodeType( node ) ==  CONTINUOUS_TYPE ){

			EnterNodeValue_bn (node,  (double) value );
		}

		if (checkError() == NET_ERR){
			fprintf(stderr,"Error: setNodeFinding: node: %s -> (int: %d, float: %.2f)\n", getNodeName(node), (int)value,  value );
			return NET_ERR;
		}


		else
			return NET_OK;
	}

	return NET_ERR;

}

int resetNodeFindings(node_t* node){
	if (node){
		RetractNodeFindings_bn(node);
		return checkError();
	}

	return NET_ERR;
}


node_t* getNodeInNet(net_bn* net, char *nodename){

	if (!net || !nodename){
		fprintf(stderr, "getNodeInNet: Incorrect arguments\n");
		return NULL;
	}

	return GetNodeNamed_bn (nodename, net );

}

node_t* getNodeByName(char *nodename, net_info_t* nets, int n_networks){

	int i;
	node_t* node;

	for (i=0; i < n_networks; i++){
		node = getNodeInNet(nets[i].net, nodename);
		if (node)
			return node; 
	}
	return NULL;
}


int getNodeStateValueStr(node_t* node, int state, char* res_str){

	const level_bn* node_levels = NULL;
	nodetype_bn aux_node_type = -1;

	aux_node_type = getNodeType(node);

	if (aux_node_type == DISCRETE_TYPE){
		sprintf(res_str,"%s" , GetNodeStateName_bn(node, state));
		return NET_OK;
	}

	else if (aux_node_type == CONTINUOUS_TYPE){

		node_levels = GetNodeLevels_bn(node);

		if(node_levels[state] == -179589723762469127732510952207590308605129343313275008770386039446431745642627425141063683838909027637604304328082199480672580063537989127481301993246688101530711167026574742509809712389667739758184948408983133542388500252948482578554330667991490432841517561889231746889776235673861781532496270511661792624640.0)
		 	sprintf(res_str,"-Inf");
		else

			sprintf(res_str,"%.2f", node_levels[state]);

		return NET_OK;
	}
	return NET_ERR;
}

int getNodeType(node_t* node){ return GetNodeType_bn(node); }
int getNodeNumberStates(node_t* node){ return GetNodeNumberStates_bn(node); }
const nodelist_t* getNetNodes(net_bn* net){ return GetNetNodes_bn(net); } 
int lengthNodeList (const nodelist_t* nodelist) { return LengthNodeList_bn(nodelist); }
char* getNodeName ( const node_t* node) { return (char*)GetNodeName_bn(node); }
const char* getNodeTitle(node_t* node){ return GetNodeTitle_bn(node); }
int numParents(node_t* node){ return LengthNodeList_bn (GetNodeParents_bn (node));}
int isLeafNode(node_t* node){return (numParents(node) == 0);}
double getNodeExpectedValue(node_t* node, double* std_dev) { return GetNodeExpectedValue_bn (node, std_dev, NULL, NULL ); }
char* getNetName(net_t* net){ return (char*)GetNetName_bn(net); }

float* getNodeBeliefs(node_t* node) { 

	float* beliefs = (float*)GetNodeBeliefs_bn(node);

	if (checkError()==NET_ERR){
		fprintf(stderr,"Error: getNodeBeliefs: node: %s\n", getNodeName(node) );
			return NULL;
	}

	return beliefs;

}

// It tries to match directly with just basename
net_info_t* getNetByFileName(net_info_t* nets, int n, char* filename){ 

	int i;

	char buff[MAX_BUFF];

	for (i =0; i < n; i ++){

		if (!strncmp(nets[i].filename, filename, strlen(nets[i].filename)))
			return &(nets[i]);

		else{
			strncpy(buff, nets[i].filename, MAX_BUFF);
			if (!strncmp(basename(buff), filename, MAX_BUFF))
				return &(nets[i]);
		}
	}

	return NULL;

}

int enterNodeLikelihood(node_t* node, float* likelihood){ 

	int i;
	prob_bn* prior_probs;
	float *adjusted_likelihood = NULL;
	float sum=0;
	char buff[MAX_BUFF];
	state_bn* parent_states = NULL;
	int num_parents = numParents(node);
	int n_states = getNodeNumberStates(node);
	float normalized[64];

	resetNodeFindings(node);

	if (!isLeafNode(node)){

		adjusted_likelihood = (float*)malloc (n_states * sizeof(float));
		if (!adjusted_likelihood){
		   	fprintf (stderr, "Error: enterNodeLikelihood: malloc\n");
		   	return NET_ERR;
		}

		prior_probs = GetNodeBeliefs_bn((node_bn*)node);

		for (i = 0; i < n_states; i ++){

			if (prior_probs[i] == 0)
				adjusted_likelihood[i] =0;
			else
				adjusted_likelihood[i]= likelihood[i] / (prior_probs[i] * 100);
		}

		for (i=0; i < getNodeNumberStates(node); i++){
			sum += adjusted_likelihood[i];
		}

		if (sum > 0)
			EnterNodeLikelihood_bn(node, (prob_bn*)adjusted_likelihood);  // normalizes likelihood
		else{
			return NET_NODE_ALL_ZERO;
		}

		free(adjusted_likelihood);

		if (checkError()==NET_ERR){
			fprintf(stderr,"Error: enterNodeLikelihood: node: %s\n", getNodeName(node) );


			// check the sum in order to normalize it
			for (i=0; i < n_states; i++){
				//printf("DEBUG:enterNodeLikelihood %f\n", adjusted_likelihood[i]);
				sum += likelihood[i];
			}
			
			return NET_ERR;
		}
		return NET_OK;
	}
	
	else{

		

		if (num_parents > 0){
			parent_states = (state_bn*)malloc(num_parents* sizeof (state_bn));
		    if (!parent_states){
		    	fprintf (stderr, "Error: enterNodeLikelihood: malloc\n");
		    	return NET_ERR;
		    }

			for (i =0; i < num_parents; i ++){
				parent_states[i] = EVERY_STATE;
			}
		}

		SetNodeProbs_bn(node, parent_states, (prob_bn*)likelihood); // Does NOT normalize likelihood

		if (parent_states)
			free(parent_states);
		
		if (checkError()==NET_ERR){
			fprintf(stderr,"Error: enterNodeLikelihood: node: %s\n", getNodeName(node) );

			return NET_ERR;
		}

		return NET_OK;
	}
}

int checkNetFindings(net_bn* net){

	const nodelist_bn* nodelist = GetNetNodes_bn(net);
	int numnodes = lengthNodeList(nodelist);
	node_bn *node;
	int i;

	state_bn state;

	for (i = 0;  i < numnodes;  i++){
		node = NthNode_bn (nodelist, i);
		state = GetNodeFinding_bn(node);
		if (state != NO_FINDING)
			printf("checkNetFindings: Finding for node %s: %d\n", getNodeName(node), state);
	}
	return NET_OK;

}

int printNetworkInfo(net_info_t net_info){

	net_t* net = net_info.net;
	nodelist_t* nodelist = (nodelist_t*)getNetNodes(net); 
	int i, j;
	node_t* node;

	char buff[MAX_BUFF];

	for (i =0; i < lengthNodeList(nodelist); i++){
		
		node = NthNode_bn (nodelist, i);

		printf("%s: (",getNodeName(node));

		if (getNodeType(node) == DISCRETE_TYPE)
			printf("D):\t[");
		else if (getNodeType(node) == CONTINUOUS_TYPE)
			printf("C):\t[");
		else 
			printf("U):\t[");

		for (j =0; j < getNodeNumberStates(node); j++){
			getNodeStateValueStr(node, j, buff);
			printf("%s",buff);
			if(j <  getNodeNumberStates(node)-1)
				printf(", "); 
		}	
		printf("]\n");
	}
	return 0;
}


int printNetworkInfoJSON(FILE* fd, net_info_t net_info){

	net_t* net = net_info.net;
	nodelist_t* nodelist = (nodelist_t*)getNetNodes(net); 
	int i, j;
	node_t *node, *parent;
	double x, y;
	nodelist_bn* parents;

	char buff[MAX_BUFF];
	double val;


	json_object *jobj = json_object_new_object();

	json_object_object_add(jobj,"net_filename", json_object_new_string(net_info.filename));
	json_object_object_add(jobj,"net_name", json_object_new_string(getNetName(net)));

	json_object *jnodes = json_object_new_array();

	for (i =0; i < lengthNodeList(nodelist); i++){
		
		node = NthNode_bn (nodelist, i);

		json_object *jnode = json_object_new_object();

		json_object_object_add(jnode,"name", json_object_new_string(getNodeName(node)));

		json_object *jstates = json_object_new_array();
	
		for (j =0; j < getNodeNumberStates(node); j++){
		
			getNodeStateValueStr(node, j, buff);

			json_object_array_add(jstates,json_object_new_string(buff));
		}	
		json_object_object_add(jnode, "states", jstates);

		switch(getNodeType(node)){

			case DISCRETE_TYPE: 
				json_object_object_add(jnode, "type", json_object_new_string("DISCRETE_TYPE"));
				break;
			case CONTINUOUS_TYPE:
				json_object_object_add(jnode, "type", json_object_new_string("CONTINUOUS_TYPE"));
				break;
			default:
				json_object_object_add(jnode, "type", json_object_new_string("UNKNOWN_TYPE"));
		}


		/* visual information */
		GetNodeVisPosition_bn(node, NULL, &x, &y);
		json_object *jvisual = json_object_new_object();
		json_object_object_add(jvisual, "x", json_object_new_double(x));
		json_object_object_add(jvisual, "y", json_object_new_double(y));
		json_object_object_add(jnode, "visual", jvisual);

		/* parents */
		json_object *jparents = json_object_new_array();
		parents = (nodelist_t*)GetNodeParents_bn (node);
		for (j =0; j < lengthNodeList(parents); j++){
			parent = NthNode_bn (parents, j);
			json_object_array_add(jparents,json_object_new_string(getNodeName(parent)));
		}
		json_object_object_add(jnode, "parents", jparents);

		json_object_array_add(jnodes, jnode);
	}

	json_object_object_add(jobj,"nodes", jnodes);

	fprintf (fd, "%s\n",json_object_to_json_string(jobj));

	json_object_put(jobj);
	
	return 0;
}

int printNode(node_t* node, int mode){

	int i;
	char state_str[MAX_BUFF];

	int type = getNodeType(node);
	float *beliefs = getNodeBeliefs(node);

	double std_dev;
	double expected_value;

	if (type == CONTINUOUS_TYPE){
		expected_value = getNodeExpectedValue (node, &std_dev );
	}

	if (beliefs == NULL){
		fprintf(stderr, "Error: Getting beliefs of node '%s'\n", getNodeName(node));
		return -1;
	}
	
	if(mode == PRINT_BASIC){
		printf(" (%s (%s)[ ", getNodeName(node), (type == DISCRETE_TYPE)?"d":(type == CONTINUOUS_TYPE)?"c":"u"  );
		for (i = 0; i< getNodeNumberStates(node); i++)
			printf("%d ", (int)(beliefs[i] * 100));
		printf("] ");
		if (type == CONTINUOUS_TYPE){
			printf("(%.2f - %.2f) ",expected_value, std_dev );
		}
	}

	else if(mode == PRINT_PRETTY){

		printf("\n--------------------------------\n");
		printf("|       %-23s|\n",  getNodeName(node));
		printf("--------------------------------\n");


		for (i = 0; i< getNodeNumberStates(node); i++){
			getNodeStateValueStr(node, i, state_str);
			printf("|%-28s%2d|\n", state_str, (int)(beliefs[i] * 100));
		}
		printf("--------------------------------\n");
	}

	return 0;
}

double GetNodeValueEntered(node_t* node ){ 

	double val = GetNodeValueEntered_bn(node);

	if (val == UNDEF_DBL)
		return NET_UNDEFINED_VALUE;
	else
		return val;
}