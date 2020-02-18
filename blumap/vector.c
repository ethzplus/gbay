#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <regex.h> 
#include <sys/stat.h> // mkdir
#include <sys/types.h> 
#include <gdal.h>


#include "vector.h"
#include "dbf.h"
//#include "gdal/ogr_srs_api.h"
//#include "gdal/ogr_srs_api.h"

//const char* driver = "FileGDB";

int loadVectorFeaturesData(char *filename, vector_t* vector, int printinfo);

// filename is the directory containing .shp .prj, .dbx, etc
int loadVector(char *filename, vector_t* vector, int printinfo){

	int n_fields;
	int n_features;
	int i = 0;

	GDALAllRegister();

	vector->hDataset = GDALOpenEx(filename, GDAL_OF_VECTOR | GDAL_OF_VERBOSE_ERROR, /*&driver*/ NULL, NULL, NULL );
	if( !vector->hDataset )	{
		fprintf(stderr,"Error opening file: %s (%d)\n", filename, printinfo);
	    return -1;
	}

	if (printinfo){

		printf("loadVector: %d layers. Only the first layer will be read. The rest will be ignored!\n", GDALDatasetGetLayerCount(vector->hDataset));

		vector->hLayer = GDALDatasetGetLayer(vector->hDataset, 0);
		vector->hFDefn = OGR_L_GetLayerDefn(vector->hLayer);
		vector->n_fields = OGR_FD_GetFieldCount(vector->hFDefn);
		vector->n_features = OGR_L_GetFeatureCount(vector->hLayer, 0);

		printf("loadVector: %d fields\n", vector->n_fields);
		printf("loadVector: %d features\n", vector->n_features);
	}

	strcpy(vector->filename,filename);

	return 0;
}





/*
	It checks the projection and number of features.
	TODO: check feature coordinates
*/
int checkSameVectorMetaInfo(vector_t* vectors, int len){

	OGRLayerH hLayer, hLayer_base;
	int n_features_base;
	int i;

	printf("checkSameVectorMetaInfo - NOT CHECKING SAME PROJECTION (empty string) -\n");

	if (!vectors)
		return 0;

	hLayer_base = GDALDatasetGetLayer(vectors[0].hDataset, 0);
	n_features_base = OGR_L_GetFeatureCount(hLayer_base, 0);

	for (i=1; i < len; i++){
/*
		if(!strcmp(vectors[0].proj, vectors[i].proj)){
			fprintf(stderr,"checkSameVectorMetaInfo: Projections mismatch\n");
			return -1;
		}
*/
		hLayer = GDALDatasetGetLayer(vectors[i].hDataset, 0);

		if (n_features_base != OGR_L_GetFeatureCount(hLayer, 0)){
			fprintf(stderr,"checkSameVectorMetaInfo: Feature number mismatch\n");
			return -1;
		}
	}

	return 0;

}



/*
	It will create a shapefile with an attribute table with a column for every state in n_states

	data is a char array [state][feature]
*/
int writeSHP(char *dirname, GDALDatasetH refDataset, void** data, int n_states){

    GDALDriverH *hDriver;
    GDALDatasetH hDS;
    OGRLayerH hLayer, hLayer_ref;
    OGRFieldDefnH hFieldDefn = NULL;
    OGRFeatureH hFeature = NULL, hFeature_ref = NULL;
    const char *pszDriverName = "ESRI Shapefile";
    //const char *pszDriverName = "FileGDB";
    char column_name[3];
    int i,j,feature_counter;
    char shapefile[256];
    int err;


     OGRGeometryH hGeometry;


	//printf("\nwriteSHP: Creating directory: %s\n", dirname);

	err = mkdir(dirname, 0777);
	if (err){

		if (errno != EEXIST){
			perror("writeSHP: mkdir");
			return -1;
		}
		else
			printf(" - writeSHP: directory already existed\n");
	}

	GDALAllRegister();

    hDriver = (GDALDriverH*) GDALGetDriverByName(pszDriverName);
    if( hDriver == NULL ){
        printf("writeSHP: %s driver not available.\n", pszDriverName);
        return -1;
    }

    sprintf(shapefile, "%s/%s.shp", dirname, basename(dirname));

    //printf("\nwriteSHP: Creating shapefile: %s\n", shapefile);

	hDS = GDALCreate( hDriver, shapefile, 0, 0, 0, GDT_Unknown, NULL );
	if( hDS == NULL )	{
	    printf("writeSHP: Creation of output file failed.\n");
	    return -1;
	}

    hLayer_ref = GDALDatasetGetLayer(refDataset, 0);

	hLayer = GDALDatasetCreateLayer( hDS, "output", OGR_L_GetSpatialRef(hLayer_ref), OGR_L_GetGeomType(hLayer_ref), NULL );
	if( hLayer == NULL ){
	    printf("writeSHP: Layer creation failed.\n");
	    return -1;
	}

	// Create attribute table
	for (i=0; i < n_states; i++){

		if (i < n_states-1)
			sprintf(column_name, "s%d", i);
		else
			sprintf(column_name, "max");

		hFieldDefn = OGR_Fld_Create(column_name, OFTInteger);
		OGR_Fld_SetWidth( hFieldDefn, 8);
		if( OGR_L_CreateField( hLayer, hFieldDefn, TRUE ) != OGRERR_NONE ){
		    printf( "writeSHP: Creating %s field failed.\n",column_name );
		    return -1;
		}
		OGR_Fld_Destroy(hFieldDefn);
		hFieldDefn = NULL;

	}

	
	OGR_L_ResetReading(hLayer_ref); 	

	// copy features form reference
	for (i =0; i < OGR_L_GetFeatureCount(hLayer_ref, 0); i++){

		hFeature = OGR_F_Create( OGR_L_GetLayerDefn( hLayer ) );

		// write attribute table

		for (j=0; j < n_states; j++){

			if (j < n_states-1)
				sprintf(column_name, "s%d", j);
			else
				sprintf(column_name, "max");
			
			//printf("- %s: %d\n", column_name, ((char**)data)[j][i] ) ;
			OGR_F_SetFieldInteger( hFeature, OGR_F_GetFieldIndex(hFeature, column_name), ((char**)data)[j][i] );
		}


		hFeature_ref = OGR_L_GetNextFeature(hLayer_ref);
	

		/* Just for debugging */ 
		/*
		if (i==0){
			printf("writeSHP TESTING\n");
			hGeometry = OGR_F_GetGeometryRef(hFeature_ref);
			if(hGeometry) 
				printf("%s\n",OGR_G_ExportToJson(hGeometry));
			else
				printf("It is NULL\n");
		}
	*/



		if (OGR_F_SetGeometry( hFeature, OGR_F_GetGeometryRef(hFeature_ref)) != OGRERR_NONE ){
		    printf( "writeSHP: OGR_F_SetGeometry failed.\n");
		    return -1;
		}

		if (OGR_L_CreateFeature(hLayer, hFeature) != OGRERR_NONE ){
		    printf( "writeSHP: OGR_L_CreateFeature failed.\n");
		    return -1;
		}

		OGR_F_Destroy(hFeature);
		OGR_F_Destroy(hFeature_ref);

	}

	GDALClose( hDS );

	return 0;

}	  


/*
	Refer to comments of setFindingsFromVectors

	It reads the columns from the attribute table and differenciates betweeen findings and likelihoods to get the number of possible nodes. WARNING - there could be columns that are not nodes in the network (Area, id and so on))

	*nodenames should be freed afterwards

	Returns the number of nodes

	This function should return a JSON when called from the webgui. Avoid any debug message!

*/
int getNodeNamesFromVector(vector_t vector, char*** nodenames, int verbose){

	int i, j;
	int iField;
	OGRLayerH hLayer;
	OGRFeatureDefnH hFDefn;
	OGRFieldDefnH hFieldDefn;
	int n_features, n_fields, ret;
	char *fieldname;
	regmatch_t matches[3];
	char buffer[128];
	regex_t regex; 	// to parse column names in attribute tables for likelihoods (lu_t0__s1, lu_t0__s2, etc)

	hLayer = GDALDatasetGetLayer(vector.hDataset, 0);
	hFDefn = OGR_L_GetLayerDefn(hLayer);
	n_fields = OGR_FD_GetFieldCount(hFDefn);
	n_features = OGR_L_GetFeatureCount(hLayer, 0);
	
	char* likelihood_nodenames[256];
	int likelihoods_n=0;

	char* finding_nodenames[256];
	int findings_n=0;






	ret = regcomp(&regex, REGEX_VECTOR_COLUMNS, REG_EXTENDED);
	if (ret) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_VECTOR_COLUMNS);
	    return -1;
	}

	if (verbose)
		printf("Vector mode: Using regular expression to match node findings/likelihoods in the attribute table: %s\n", REGEX_VECTOR_COLUMNS);

	for (iField =0; iField < n_fields; iField++){

		hFieldDefn = OGR_FD_GetFieldDefn( hFDefn, iField );
		fieldname = (char*)OGR_Fld_GetNameRef(hFieldDefn);

		// check if it is a likelihood. It has "__s" in the name
		ret = regexec(&regex, fieldname, 3, matches, 0);

		if (!ret){   // It is likelihood (soft evidence)

			if(verbose)
				printf("getNodeNamesFromVector: likelihood (%s)\n", fieldname);

			strncpy(buffer, fieldname+matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
				buffer[matches[1].rm_eo - matches[1].rm_so] = '\0';

			for (j=0; j < likelihoods_n; j++){
				if (!strncmp(buffer, likelihood_nodenames[j], strlen(buffer)))
					break;
			}

			if( j == likelihoods_n){
				likelihood_nodenames[j] = strdup(buffer);
				likelihoods_n++;
				
			}

		}
		else if (ret == REG_NOMATCH) {  // no match. It is a finding (hard evidence)
			
			if(verbose)
				printf("getNodeNamesFromVector: finding (%s)\n", fieldname);
			
			finding_nodenames[findings_n] = strdup(fieldname);
			findings_n++;
		
		}
		else{ 
			regerror(ret, &regex, buffer, sizeof(buffer));
		    fprintf(stderr, "getNodeNumberFromVector: Regex match failed: %s\n", buffer);
		    regfree(&regex);
		    return -1;
		}
	}


	*nodenames = (char**)malloc(sizeof(char*) * (findings_n + likelihoods_n));
	for (i=0; i < findings_n; i++){

		if(verbose)
			printf("getNodeNamesFromVector: copying finding: %s\n", finding_nodenames[i]);

		(*nodenames)[i] = strdup(finding_nodenames[i]);

		free(finding_nodenames[i]);
	}
	for (i=0; i < likelihoods_n; i++){

		if(verbose)
			printf("getNodeNamesFromVector: copying likelihood: %s\n", likelihood_nodenames[i]);

		(*nodenames)[findings_n+i] = strdup(likelihood_nodenames[i]);

		free(likelihood_nodenames[i]);
	}

	regfree(&regex);

	return (findings_n + likelihoods_n);

}
