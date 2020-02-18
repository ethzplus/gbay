/*
	It will create a geoTIFF file with the same metadata as the first argument, and as many bands as second parameter, where values for the same pixels in different bands add to 1

	Optional argument: -o outputfile
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "raster.h"

#define OUTPUT_FILENAME "output.tiff"

int printPixel(char *filename, int offset){

	float* data;
	int size, i;
	float* likelihood;

	GDALDatasetH hDataset;

	hDataset = loadRaster(filename,1);

	size = readRasterData(&data, hDataset, 0, 1);

	likelihood = (float*)malloc(sizeof(float) * GDALGetRasterCount( hDataset ));
	if (!likelihood){ 
		fprintf(stderr, "ERROR: printPixel: malloc\n");
		if (data) free(data);
		return -1;
	}

	for (i = 0; i < GDALGetRasterCount( hDataset ); i++)
		likelihood[i] = data[offset + GDALGetRasterXSize(hDataset)* GDALGetRasterYSize(hDataset)*i] / 100.0;

	for (i =0; i < GDALGetRasterCount( hDataset ); i++)
		printf("%f ", likelihood[i]);

	free(likelihood);

	return 0;

}





void printData(float **data, int bands, int limit){

	int i,j;

	for (i=0; i < bands; i++ ){
		printf("Band %d: ", i);
		for (j=0; j < limit; j++ ){
			printf("%f ", data[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

int main(int argc, char*argv[]){

	GDALDatasetH refDataset;	
	float **data_array;
	int err, bands, total_pixels;
	int i, j, k, rest;
	char* filename = OUTPUT_FILENAME;
	char c;


	while ((c = getopt (argc, argv, "ho:")) != -1){
		switch (c){
			case 'h':
				printf("Usage: %s NBANDS [-o OUTPUT_FILENAME]\n", argv[0]);
				return 0;
			case 'o':
				filename = optarg;
				break;
			default:
				printf("Unrecognized option %c\n", c);

		}

	}

	if (argc<3){
		printf("Usage: %s refDataset NBANDS [-o OUTPUT_FILENAME]\n", argv[0]);
		return -1;
	}

	refDataset = loadRaster(argv[optind], 1);
	if (!refDataset){
		fprintf(stderr, "loadRaster error\n");
		return -1;
	}

	total_pixels = GDALGetRasterXSize( refDataset ) * GDALGetRasterYSize( refDataset ) ;

	printf("\nTotal pixels: %d\n", total_pixels);

	bands = atoi(argv[optind+1]);

	data_array = (float**)CPLMalloc(sizeof(float*) * bands);
	if (!data_array){
		fprintf(stderr,"ERROR: data_array: Error allocating memory. Trying to reserve %ld bytes\n", sizeof(float*) * bands);
		return -1;
	}

	for (i =0; i < bands; i++)
		data_array[i] = (float*)CPLMalloc(sizeof(float)*total_pixels);

	srand(time(NULL));

	for (j =0; j < total_pixels; j++){

		printf("Cell %d\n",j);

		for (i =0; i < bands; i++){
			if (i==0){
				// data_array[i][j] = roundf( ((rand() % 100) / 100.0) *100) /100; // 2 decimals
				data_array[i][j] = rand() % 100; 
			}
			else {

				rest = 100;

				for (k =0; k < i; k++)
					rest -= data_array[k][j];

				if (i==bands-1)
					data_array[i][j] = rest;
				else{
					if (rest == 0)
						data_array[i][j] = 0;
					else
						data_array[i][j] = rand() % rest;
				}
			}
		}

		for (i =0; i < bands; i++)
			printf("%f\n",data_array[i][j]);

		//printf("cell %d: %f\n",j,  data_array[0][i] + data_array[1][i] + data_array[2][i]);
	}

	printData(data_array, bands, 10);

	err = writeGeoTIFF(filename, refDataset, (void**)data_array, GDT_Float32, bands, 255);
	if (err == -1){
		fprintf(stderr,"ERROR: writeGeoTiff\n");
	}





	printf("pixel 342: ");
	printPixel(filename, 342);





	return 0;

}