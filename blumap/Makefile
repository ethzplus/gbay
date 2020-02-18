OBJECT_FILES = blumap.o raster.o vector.o by_network.o file_writer.o embed_python.o execution.o dbf.o statistics.o
CC = gcc
CFLAGS = -g -Wall -Wextra -Wcomment -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter

TARGET = blumap

ARCH := $(shell getconf LONG_BIT)

ifeq ($(ARCH),64)
    LIB_PATH = -L Netica_API_504/lib/64bit
else
    LIB_PATH = -L Netica_API_504/lib
endif

LIB_PATH += $(shell pkg-config --libs json-c)

LIB_PATH += $(shell pkg-config --libs gdal)

CFLAGS_PYTHON = $(shell /usr/bin/python2.7-config --cflags)
LIB_PATH_PYTHON = $(shell /usr/bin/python2.7-config --ldflags)

LIB_PATH += $(LIB_PATH_PYTHON)


# INC_PATH = -I smile_linux_x86_gcc_3_4_6/
# LIB_PATH = -L smile_linux_x86_gcc_3_4_6/

GDAL_LIB = gdal

all: $(TARGET) net2json ranraster vectornodes

blumap: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(OBJECT_FILES) $(LIB_PATH) -l $(GDAL_LIB) -lnetica -lm -lrt -lnetica -lpthread -lstdc++ -ljpeg -o $@

net2json: net2json.o by_network.o
	$(CC) $(CFLAGS) net2json.o by_network.o $(LIB_PATH) -lnetica -lm -lrt -lnetica -lpthread -lstdc++ -o $@

bayescmd: bayescmd.o by_network.o embed_python.o
	$(CC) $(CFLAGS) bayescmd.o by_network.o embed_python.o $(LIB_PATH) -lnetica -lm -lrt -lnetica -lpthread -lstdc++ -o $@

vectornodes: vectornodes.o vector.o 
	$(CC) $(CFLAGS) vectornodes.o vector.o $(LIB_PATH) -l $(GDAL_LIB) -lstdc++ -o $@

ranraster: ranraster.o raster.o
	$(CC) $(CFLAGS) ranraster.o raster.o $(LIB_PATH) -l $(GDAL_LIB) -lm -o $@

ranraster.o: ranraster.c 
	$(CC) $(CFLAGS) -I gdal -c ranraster.c

blumap.o: blumap.c blumap.h
	$(CC) $(CFLAGS) $(shell pkg-config --cflags json-c) -I Netica_API_504/src -c blumap.c

raster.o: raster.c raster.h
	$(CC) $(CFLAGS) -I gdal -c raster.c

vector.o: vector.c vector.h
	$(CC) $(CFLAGS) -I gdal -c vector.c

by_network.o: by_network.c by_network.h
	$(CC) $(CFLAGS) -I Netica_API_504/src -c by_network.c

file_writer.o: file_writer.c file_writer.h
	$(CC) $(CFLAGS) -I gdal -c file_writer.c

net2json.o: net2json.c 
	$(CC) $(CFLAGS) -I Netica_API_504/src $(shell pkg-config --cflags json-c) -c net2json.c

bayescmd.o: bayescmd.c 
	$(CC) $(CFLAGS) -I Netica_API_504/src $(shell pkg-config --cflags json-c) -c bayescmd.c

vectornodes.o: vectornodes.c 
	$(CC) $(CFLAGS) -I $(shell pkg-config --cflags json-c) -c vectornodes.c

embed_python.o: embed_python.c embed_python.h
	$(CC) $(CFLAGS) $(CFLAGS_PYTHON) -c embed_python.c

execution.o: execution.c execution.h
	$(CC) $(CFLAGS) $(CFLAGS_PYTHON) -c execution.c

dbf.o: dbf.c dbf.h
	$(CC) $(CFLAGS) -c dbf.c

statistics.o: statistics.c statistics.h
	$(CC) $(CFLAGS) -c statistics.c

clean:
	rm -f $(TARGET) $(OBJECT_FILES) net2json.o net2json bayescmd.o bayescmd ranraster*~
