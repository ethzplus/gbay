#ifndef __VECTOR_H__
#define __VECTOR_H__

//#include "gdal/gdal.h"
#include <gdal.h>

#define MAX_LEN 256

// to parse column names in attribute tables for likelihoods (lu_t0__s1, lu_t0__s2, etc)
#define REGEX_VECTOR_COLUMNS "^(\\w+)__[s|S]([1-9][0-9]*)$"

// typedef struct {

// 	char nodename[MAX_LEN];
// 	float *findings;

// } feature_node_findings_t;   // as read from the DBF


typedef struct {

	GDALDatasetH hDataset;
	char proj[MAX_LEN];
	char filename[MAX_LEN];
	//feature_node_findings_t* feature_node_finding;
	char nodename[MAX_LEN];   // <--- this makes no sense, now the nodes are stored in the colmuns of the attribute table. So every vector file could contain likelihoods fr many nodes
	OGRLayerH hLayer;
	OGRFeatureDefnH hFDefn;
	OGRFieldDefnH hFieldDefn;
	int n_fields;
	int n_features;
} vector_t;




int loadVector(char *dirname, vector_t* vector, int printinfo);
int checkSameVectorMetaInfo(vector_t* vectors, int len);
int writeSHP(char *filename, GDALDatasetH refDataset, void** data, int n_states);
int getNodeNamesFromVector(vector_t vector, char*** nodenames, int verbose);
#endif