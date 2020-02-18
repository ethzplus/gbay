#ifndef __BY_NETWORK_H__
#define __BY_NETWORK_H__

#include "Netica.h"

#define NET_OK 0
#define NET_ERR -1
#define NET_NODE_ALL_ZERO -2
#define NET_UNDEFINED_VALUE -1   // to be returned when getting a value from a continuous node is undefined
#define MAX_BUFF 512
#define PRINT_BASIC 0
#define PRINT_PRETTY 1
#define MYCHKERR  { if (checkError()!=NET_OK) { exit(closeNet(net));}}

typedef net_bn net_t;
typedef node_bn node_t;
typedef nodelist_bn  nodelist_t;

typedef struct {
	net_t* net;
	char filename[MAX_BUFF];
	unsigned int time_period;
} net_info_t;

int initBayesNetworkEnv(char* license, int verbose);
int closeBayesNetworkEnv(int verbose);
net_bn* readNet(char* filename);
int CompileNet(net_bn* net);
void closeNet(net_bn* net);
void closeAllNets(net_info_t* nets_info, int n);
void PrintNodeList (const nodelist_bn* nodes);
int getNodeStateValueStr(node_t* node, int state, char* res);
int setNodeFinding(node_t* node,  float value);
int resetNodeFindings(node_t* node);
node_t* getNodeInNet(net_bn* net, char *nodename);
node_t* getNodeByName(char *nodename, net_info_t* nets, int n_networks);
int getNodeType(node_t* node);
int getNodeNumberStates(node_t* node);
const char* getNodeTitle(node_t* node);
const nodelist_t* getNetNodes(net_bn* net); 
char* getNodeName( const node_bn* node);
int isLeafNode(node_t* node);
int numParents(node_t* node);
float* getNodeBeliefs(node_t* node);
double getNodeExpectedValue(node_t* node, double* std_dev);
int enterNodeLikelihood(node_t* node, float* likelihood);
int lengthNodeList(const nodelist_t* nodelist);
int checkNetFindings(net_bn* net);
char* getNetName(net_t* net);
int printNetworkInfo(net_info_t net_info);
int printNetworkInfoJSON(FILE* fd, net_info_t net_info);
net_info_t* getNetByFileName(net_info_t* nets, int n, char* filename);
int printNode(node_t* node, int mode);
double GetNodeValueEntered(node_t* node );

#endif