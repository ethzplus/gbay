#include <gdal.h>
#include "by_network.h" 
#include "embed_python.h"
#include "vector.h"
#include "execution.h" 
#include "dbf.h"

#define NETICA_LICENSE_FILENAME "Netica.lic"
#define OK 0
#define ERR -1
#define TRUE 1
#define FALSE 0
#define MAX_RASTER_FILES 16
#define MAX_VECTOR_FILES 16
#define MAX_TARGETS 16
#define MAX_EV 256
#define MAX_LINK 256
#define MAX_NODENAME_LEN 256
#define MAX_EXECUTIONS 2048
#define MAX_NETWORKS 10
#define BUFF_LEN 256
#define MAX_NODES 256
#define MAX_NODE_STATES 64
#define REGEX_FILENAME_NODE "^([^=]+)=(\\w+)$"
#define REGEX_NODENAME_LIKELIHOOD "^([^=]+)=\\[([0-9]+)(,[0-9]+)*\\]$"  // nodename=[40,60] no spaces
#define LOG_EXEC_FILENAME "exec_log.txt"
#define MAX_FILENAME_LEN 255
#define NO_DATA_VALUE 255
#define DEF_TIMING 10 // time to print complete percentage while processing
#define OUTPUT_FILENAME_SUFFIX ".tiff"
#define CONT_FILENAME_SUFFIX "_stats.tiff"    // additional file to be produced when target node is continuous with mean, quantile and some other statistics applied to the beliefs
#define CONT_BANDS_N 4
#define CONT_NODATA -128

// in case the sum of the likelihoods is in between these numbers --> normalize it
#define NORMALIZE_MIN 0.95
#define NORMALIZE_MAX 1.05

#define VECTORNODE_HARD 0
#define VECTORNODE_SOFT 1

typedef struct Raster {
	GDALDatasetH hDataset;
	char nodename[MAX_NODENAME_LEN];
	float nodatavalue;
	float* data;
	net_t* nets[MAX_NETWORKS]; //list of nets with a node of this name
	int nets_n;
	char filename[BUFF_LEN];
} raster_t;

// Used when fixed evidence from command line
struct Evidence {
	net_info_t* net_info;
	char nodename[MAX_NODENAME_LEN];
	int nodeindex;
	float data;
};

// Used when set node likelihood from command line
struct Likelihood {
	char nodename[MAX_NODENAME_LEN];
	float data[MAX_NODE_STATES];
	int data_len;
};

struct Linking {
	net_t* net_src; // probably this is not needed. Just for debug.
	net_t* net_dst; // probably this is not needed. Just for debug.
	node_t* src;
	node_t* dst;
};


typedef struct raster_to_bayes{
	int value;
	net_info_t* net_info;
}raster_to_bayes_t ;

typedef enum {
	TYPE_RASTER,
	TYPE_VECTOR
} gis_type_t;

typedef struct {
	char* name;
	int type; // VECTORNODE_HARD or VECTORNODE_SOFT.
	int iField; //index of the field in the attribute table in case of hard evidence; number of states read in case of soft evidence
	int *iFields; // indexes per state in case of soft evidence
	int* data;

} vectornode_t;

int consistentRasters(struct Raster* rasters, int n);
int writeResultFile(char *filename, GDALDatasetH refDataset, void** data, GDALDataType dataType, int n_bands, int nodata);
int setFindingsFromRasters(net_bn* net, struct Raster* rasters, int rasterfile_n, int offset, int debug);
int writeTempFile(char** belief_array, int n_bands, int x_size, int y_size);
int setInitialEvidenceAllNetworks(net_info_t* nets_info, int n_networks, struct Evidence* evidences, int evidence_n);
int setInitialLikelihoods(net_info_t* nets_info, struct Likelihood* likelihoods, int likelihoods_n);
int enterLikeliHoodsFromPYNode(net_t* net , int cell, pyNode_t** pyNodes, int pyNodes_n);
int setFindingsFromVectors(net_bn* net, vector_t* vectors, int vectors_n, int feature_index, Execution_t* execution, int save_execution_only, char** vector_nodes_enabled, int vector_nodes_enabled_n, vectornode_t* vectornodes, int vectornodes_n, int debug);

// path_node is in the format path_to_file=nodename. Loads the raster file into 'raster'
//int loadRasterFileMatchingNode(char* path_node, net_info_t net_info, struct Raster* raster, regex_t* regex, int debug);
int loadGISPathMatchingNode(char* path_node, net_info_t net_info, void* gis, regex_t* regex, gis_type_t gis_type, int debug);
int loadEvidenceMatchingNode(char* evidence_name, float evidence_value, net_info_t net, struct Evidence* evidence, int debug);

// Looks for a node in nets that match the evidence. 
// Returns number of networks where linking is correct
int loadLinkingMatchingNode(char* linking_src, char* linking_dst, net_info_t* nets_info, int n_networks, struct Linking* linking, int debug);
void closeRaster(struct Raster raster);

// Returns a array of pointers to the inner data structure for a given pixel 
float** getPixelVector(struct Raster raster, int pixel, float **vector);
int applyDBFtoRasterData(DBF dbf, struct Raster raster, node_t* node, int debug);
void printLikelihood(float* likelihood, int len);
void printLikelihood_adv(float* likelihood, int len, FILE* fd, char* prefix);
int getVectornodesFromVector(net_bn* net, vector_t* vector, vectornode_t* vectornodes, regex_t* regex, int debug);
int readVectornodesData(net_bn* net, vector_t* vector, vectornode_t* vectornodes, int vectornodes_n, int debug);
