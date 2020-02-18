#include <stdio.h>
#include <string.h>
//#include "gdal/gdal.h"
#include <gdal.h>
//#include "gdal/ogr_core.h"
//#include "gdal/ogr_srs_api.h"

//#include "gdal/cpl_conv.h" /* for CPLMalloc() */

#include "raster.h"


GDALDatasetH loadRaster(char *filename, int printinfo){

	GDALDatasetH hDataset;
	GDALDriverH hDriver;
	double adfGeoTransform[6];

	//printf("loadRaster: %s\n", filename);

	GDALAllRegister();

	hDataset = GDALOpen( filename, GA_ReadOnly );
    if( hDataset == NULL ){
      // printf( "Error loading raster file: %s\n", filename);
       return NULL;
    }

    if (printinfo){

		hDriver = GDALGetDatasetDriver( hDataset );
		printf( "Driver: %s/%s\n",
		        GDALGetDriverShortName( hDriver ),
		        GDALGetDriverLongName( hDriver ) );

		printf( "Size is %dx%dx%d\n",
		        GDALGetRasterXSize( hDataset ), 
		        GDALGetRasterYSize( hDataset ),
		        GDALGetRasterCount( hDataset ) );

		if( GDALGetProjectionRef( hDataset ) != NULL )
		    printf( "Projection is `%s'\n", GDALGetProjectionRef( hDataset ) );

		if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None ){
		    printf( "Origin = (%.6f,%.6f)\n",
		            adfGeoTransform[0], adfGeoTransform[3] );
		    printf( "Pixel Size = (%.6f,%.6f)\n",
		            adfGeoTransform[1], adfGeoTransform[5] );
		}
	}

	return hDataset;

}






void closeGdalRaster(GDALDatasetH hDataset){
	GDALClose(hDataset);
}


GDALRasterBandH getRasterBand(GDALDatasetH hDataset, int band, int printinfo){

	GDALRasterBandH hBand;
	int nBlockXSize, nBlockYSize;
	int bGotMin, bGotMax;
	double adfMinMax[2];
	hBand = GDALGetRasterBand( hDataset, band );

	if (printinfo){

		GDALGetBlockSize( hBand, &nBlockXSize, &nBlockYSize );

		printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
		        nBlockXSize, nBlockYSize,
		        GDALGetDataTypeName(GDALGetRasterDataType(hBand)),
		        GDALGetColorInterpretationName(
		            GDALGetRasterColorInterpretation(hBand)) );

		adfMinMax[0] = GDALGetRasterMinimum( hBand, &bGotMin );
		adfMinMax[1] = GDALGetRasterMaximum( hBand, &bGotMax );
		if( ! (bGotMin && bGotMax) )
		    GDALComputeRasterMinMax( hBand, TRUE, adfMinMax );
		printf( "Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1] );
		if( GDALGetOverviewCount(hBand) > 0 )
		    printf( "Band has %d overviews.\n", GDALGetOverviewCount(hBand));
		if( GDALGetRasterColorTable( hBand ) != NULL )
		    printf( "Band has a color table with %d entries.\n", 
		             GDALGetColorEntryCount(
		                 GDALGetRasterColorTable( hBand ) ) );
	}

	return hBand;

}




/*
	Read raster bands into pafScanbuf. If 'bands' is zero, it will read all bands.

	This function will reserve memory for pafScanbuf that should be freed afterwards. It will read band by 
	band using GDALRasterIO storing the values consequently in pafScanbuf so values are in the form:
 	[ band1cell1, band1cell2, ...,  badn1celln, band2cell1, ...]

 	It is expected that values are in the range [0..100]. Anyway it reads to a buffer of type GDT_Float32

	RETURNS: number of bytes read. 
*/
int readRasterData(float **pafScanbuf, GDALDatasetH hDataset, int bands, int debug){

	//float *pafScanbuf;
	int   nXSize;
	int   nYSize;
	int i, err;

	int n_bands;
	int dataType;

	GDALRasterBandH hBand;
	//GDALRasterAttributeTableH hRAT;
/*
	float enrico_conversion[8] = {5.0, 7.0, 11.0, 2.0, 1.0, 6.0, 4.0, 3.0};

	printf("\n\n\n****************************************\nreadRasterData: USING ENRICO CONVERSION!!!!\n****************************************************\n\n\n\n");
*/


/*
1	5
2	7
3	11
4	2
5	1
6	6
7	4
8	3
*/




	/*
enum  	GDALDataType {
  GDT_Unknown = 0, GDT_Byte = 1, GDT_UInt16 = 2, GDT_Int16 = 3,
  GDT_UInt32 = 4, GDT_Int32 = 5, GDT_Float32 = 6, GDT_Float64 = 7,
  GDT_CInt16 = 8, GDT_CInt32 = 9, GDT_CFloat32 = 10, GDT_CFloat64 = 11,
  GDT_TypeCount = 12
}
*/
	
	if (bands > 0)
		n_bands = bands;
	else
		n_bands = GDALGetRasterCount( hDataset );

	hBand = getRasterBand(hDataset, 1, 0);

	dataType = GDALGetRasterDataType(hBand);

	if (dataType == GDT_Unknown){
		fprintf(stderr,"Band data type is unknown\n");
		return -1;
	}

	if (dataType == GDT_Float64 || dataType == GDT_CFloat64){
		fprintf(stderr,"Band data type is too big. Please convert it to a different data type.\n");
		return -1;
	}



	nXSize = GDALGetRasterBandXSize( hBand );
	nYSize = GDALGetRasterBandYSize( hBand );

	//*pafScanbuf = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize*n_bands);
	*pafScanbuf = (float *) malloc(sizeof(float)*nXSize*nYSize*n_bands);
	if (!(*pafScanbuf)){
		fprintf(stderr,"readRasterData: Error allocating memory");
		return -1;
	}

	for (i = 0; i < n_bands; i++){

		hBand = getRasterBand(hDataset, i+1, 0);
/*
		hRAT = GDALGetDefaultRAT(hBand);

		if (!hRAT){
			printf("readRasterData: The raster has no attribute table\n");
		}
		else
			GDALRATDumpReadable(hRAT, NULL);
*/


	

		err = GDALRasterIO( hBand, GF_Read, 0, 0, nXSize, nYSize, 
		              *pafScanbuf + nXSize*nYSize*i , nXSize, nYSize,  GDT_Float32, 
		              0, 0 );



		if(err){
			CPLError(CE_Failure, err, "readRasterData: Error calling GDALRasterIO");
		}
	}
	
	//printf("readRasterData: first cell: %f\n", *pafScanbuf[0]);

	return sizeof(GDT_Float32)*nXSize*nYSize*n_bands;

}

/*
	Checks there is a projection (could be different) and same size only. Pixel size in case of rotated rasters has to be calculated.
*/
int checkSameRasterMetaInfo(GDALDatasetH* hDatasets, int n){

	int i;
	int size_x , size_y, count;
	double origin_x, origin_y, pixelsize_x, pixelsize_y;
	//char *hDriver;
	const char *proj;
	double  adfGeoTransform[6];

	OGRSpatialReferenceH spatialRef, tmpRef;

	if( n < 2 )
		return 0;

	//hDriver = (char*)GDALGetDatasetDriver( hDatasets[0] );
	size_x = GDALGetRasterXSize( hDatasets[0] );
	size_y = GDALGetRasterYSize( hDatasets[0] );
	//count = GDALGetRasterCount( hDatasets[0] );
	proj = GDALGetProjectionRef( hDatasets[0] );

	if( GDALGetGeoTransform( hDatasets[0], adfGeoTransform ) == CE_None ){
		origin_x = adfGeoTransform[0];
		origin_y = adfGeoTransform[3];
		pixelsize_x = adfGeoTransform[1];
		pixelsize_y = adfGeoTransform[5];
	}
	else{
		fprintf(stderr, "Error in GDALGetGeoTransform.\n");
		return -1;
	}

	spatialRef = OSRNewSpatialReference(GDALGetProjectionRef( hDatasets[0] ));
	if (!spatialRef){
		fprintf(stderr, "Error in OSRNewSpatialReference.\n");
		return -1;
	}	


	//printf("EPSG: %s\n", OSRGetAttrValue(spatialRef, "PROJCS|AUTHORITY", 1));

	for(i = 1; i < n; i++){

		if (size_x != GDALGetRasterXSize( hDatasets[i])){
			fprintf(stderr, "Raster X size does not match.\n");
			return -1;
		}

		if (size_y != GDALGetRasterYSize( hDatasets[i])){
			fprintf(stderr, "Raster Y size does not match.\n");
			return -1;
		}

		// "WGS_1984_UTM_zone_39S" and "WGS 84 / UTM zone 39S" would not match
		// if (strcmp(proj, GDALGetProjectionRef( hDatasets[i] ) )){
		// 	fprintf(stderr, "Raster ProjectionRef does not match\n --%s--\n--%s--\n", proj, GDALGetProjectionRef( hDatasets[i] ));
		// 	return -1;
		// }



		tmpRef = OSRNewSpatialReference(GDALGetProjectionRef( hDatasets[i] ));
		if (!tmpRef){
			fprintf(stderr, "Error in OSRNewSpatialReference.\n");
			OSRDestroySpatialReference(spatialRef);
			return -1;
		}

		if (!OSRGetAttrValue(spatialRef, "PROJCS|AUTHORITY", 0) || !OSRGetAttrValue(tmpRef, "PROJCS|AUTHORITY", 0)){
			fprintf(stderr, "Dataset with no PROJCS|AUTHORITY field.\n");
			OSRDestroySpatialReference(spatialRef);
			OSRDestroySpatialReference(tmpRef);
			return -1;
		}

/*
		if (strcmp(OSRGetAttrValue(spatialRef, "PROJCS|AUTHORITY", 0), OSRGetAttrValue(tmpRef, "PROJCS|AUTHORITY", 0)) != 0 ){
			fprintf(stderr, "PROJCS|AUTHORITY does not match (%s - %s)\n", OSRGetAttrValue(spatialRef, "PROJCS|AUTHORITY", 0), OSRGetAttrValue(tmpRef, "PROJCS|AUTHORITY", 0));
			OSRDestroySpatialReference(spatialRef);
			OSRDestroySpatialReference(tmpRef);
			return -1;
		}

		if (strcmp(OSRGetAttrValue(spatialRef, "PROJCS|AUTHORITY", 1), OSRGetAttrValue(tmpRef, "PROJCS|AUTHORITY", 1)) != 0 ){
			fprintf(stderr, "PROJCS|AUTHORITY EPSG does not match (%s - %s)\n", OSRGetAttrValue(spatialRef, "PROJCS|AUTHORITY", 1), OSRGetAttrValue(tmpRef, "PROJCS|AUTHORITY", 1));
			OSRDestroySpatialReference(spatialRef);
			OSRDestroySpatialReference(tmpRef);
			return -1;
		}

		if( GDALGetGeoTransform( hDatasets[i], adfGeoTransform ) == CE_None ){
			
			if (origin_x != adfGeoTransform[0]){
				fprintf(stderr, "Origin X does not match\n");
				OSRDestroySpatialReference(spatialRef);
				OSRDestroySpatialReference(tmpRef);
				return -1;
			}
			if (origin_y != adfGeoTransform[3]){
				fprintf(stderr, "Origin Y does not match\n");
				OSRDestroySpatialReference(spatialRef);
				OSRDestroySpatialReference(tmpRef);
				return -1;
			}
			if (pixelsize_x != adfGeoTransform[1]){
				fprintf(stderr, "Pixel Size Y does not match\n");
				OSRDestroySpatialReference(spatialRef);
				OSRDestroySpatialReference(tmpRef);
				return -1;
			}
			if (pixelsize_y != adfGeoTransform[5]){
				fprintf(stderr, "Pixel Size Y does not match\n");
				OSRDestroySpatialReference(spatialRef);
				OSRDestroySpatialReference(tmpRef);
				return -1;
			}

		}
		else{
			fprintf(stderr, "Error in GDALGetGeoTransform\n");
			OSRDestroySpatialReference(spatialRef);
			OSRDestroySpatialReference(tmpRef);
			return -1;
		}
*/

	}

	OSRDestroySpatialReference(spatialRef);
	OSRDestroySpatialReference(tmpRef);

	return 0;

}

GDALDatasetH copyRaster(char *pszSrcFilename, char *pszDstFilename){

	GDALDatasetH hSrcDS = GDALOpen( pszSrcFilename, GA_ReadOnly );
	GDALDatasetH hDstDS;

	const char *pszFormat = "GTiff";
    GDALDriverH hDriver = GDALGetDriverByName( pszFormat );

	hDstDS = GDALCreateCopy( hDriver, pszDstFilename, hSrcDS, FALSE, NULL, NULL, NULL );

	GDALClose(hSrcDS);

	return hDstDS;

}

int writeGeoTIFF(char *filename, GDALDatasetH refDataset, void** data, int n_bands, int nodata, GDALDataType dataType){

	GDALDatasetH  hDstDS;
    GDALRasterBandH hBand;
    double adfGeoTransform[6];
    int i, err, x_size, y_size;

	//printf("\nCreating file: %s\n",filename);

	x_size = GDALGetRasterBandXSize(getRasterBand(refDataset, 1, FALSE));
	y_size = GDALGetRasterBandYSize(getRasterBand(refDataset, 1, FALSE));

	//hDstDS = GDALCreate( GDALGetDriverByName("GTiff") , filename, x_size, y_size, n_bands, GDT_Byte , NULL);
	hDstDS = GDALCreate( GDALGetDriverByName("GTiff") , filename, x_size, y_size, n_bands, dataType , NULL);

	if (!hDstDS){
		fprintf(stderr,"ERROR: Error in GDALCreate '%s'\n",filename);
		return -1;
	}

	GDALGetGeoTransform( refDataset, adfGeoTransform );
	GDALSetGeoTransform( hDstDS, adfGeoTransform );


	GDALSetProjection( hDstDS, GDALGetProjectionRef( refDataset ) );
	
	// printf("Reference projection:\n%s\n", GDALGetProjectionRef( refDataset ));
	// printf("Result projection:\n%s\n", GDALGetProjectionRef( hDstDS ));

	for (i = 0; i< n_bands; i++){

		hBand = GDALGetRasterBand( hDstDS,  i + 1 );

		// printf(" - Using NODATA: %d for band %d\n", nodata, i );

		GDALSetRasterNoDataValue(hBand, nodata);

		err = GDALRasterIO( hBand, GF_Write, 0, 0, x_size, y_size, data[i], x_size, y_size, dataType, 0, 0 );   
		if (err == CE_Failure){
			fprintf(stderr, "Error in GDALRasterIO\n");
			return -1;
		}
	}	

	GDALClose( hDstDS );

	return 0;
}
