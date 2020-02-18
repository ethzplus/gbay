#ifndef __RASTER_H__
#define __RASTER_H__

//#include "gdal/gdal.h"
#include <gdal.h>

GDALDatasetH loadRaster(char *filename, int printinfo);
void closeGdalRaster(GDALDatasetH hDataset);

GDALRasterBandH getRasterBand(GDALDatasetH hDataset, int band, int printinfo);
//float* readRasterBandData(GDALRasterBandH hBand);
//float* readRasterData(GDALDatasetH hDataset, int bands, int debug);
int readRasterData(float **pafScanbuf, GDALDatasetH hDataset, int bands, int debug);

int checkSameRasterMetaInfo(GDALDatasetH* hDatasets, int n);

GDALDatasetH copyRaster(char *pszSrcFilename, char *pszDstFilename);

int writeGeoTIFF(char *filename, GDALDatasetH refDataset, void** datae, int n_bands, int nodata, GDALDataType dataType);

#endif