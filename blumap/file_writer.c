#include <stdio.h>
#include <stdlib.h>  //atoi
#include <jpeglib.h> // to write intermidiate results
#include "file_writer.h"

/*
	write grayscale image jpeg image with 100% quality
	It will choose colors according to the range of states
	Refer to: https://github.com/Windower/libjpeg/blob/master/libjpeg.txt
*/
int writeJPEG(char *filename, char* image_buffer, int image_width, int image_height){

	FILE * outfile;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];	/* pointer to a single row */

	int row_stride;			/* physical row width in buffer */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
	    fprintf(stderr, "can't open %s\n", filename);
	    exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = image_width; 	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 1;	/* # of color components per pixel */
	cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality (&cinfo, 100, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = image_width * 1;//1 byte per pixel (greyscale)	/* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
	    row_pointer[0] = (JSAMPROW)&image_buffer[cinfo.next_scanline * row_stride];
	    jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return 0;
}


int writeBinFile(char** buffer, int n_bands, int x_size, int y_size){

	FILE* fd;
	int err, i;
	int n_pixels = x_size * y_size;
	//int size = n_bands * n_pixels;

	fd = (FILE*)fopen("tmp.bin", "wb+");
	if (!fd){
		fprintf(stderr, "writeBinFile: fopen: error\n");
		return -1;
	}

	for (i =0; i < n_bands; i++){
		err = fwrite(buffer[i], 1, n_pixels , fd);
		if (err < n_pixels){
			fprintf(stderr, "writeBinFile: fwrite: error\n");
			return -1;
		}
	}

	fclose(fd);

	return 0;

}
