import os
import sys
import json
import signal
import base64
from node_utils import *
from binascii import unhexlify

from osgeo import gdal
from osgeo import ogr

from gdalconst import *
import importlib

import logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s', stream=sys.stdout)
logging.info('python_c.py: Script started')

thirdparty_timeout = 1200 

INDEX_NAME =0
INDEX_STATES = 1
INDEX_TYPE = 2
INDEX_DATA =3


# All defined in node_utils

#PY_NODATA_VALUE = 255  # Same as in embed_python.h. To communicate a NODATA value from C and to C the likelihood has to have this value for every state.
#PY_DISCRETE = 0   # Same as in embed_python.h
#PY_CONTINUOUS = 1 # Same as in embed_python.h

WORKING_DIRECTORY = './sandbox'

def timeout_handler(signum, frame):
	raise Exception("Timeout")


def main():

	new_nodes_data = [{'name' : 'water', 'data': [bytearray('\x04\x60'), bytearray([30,20,50]), bytearray([33,33,34])]}]

	print validResultData(new_nodes_data)



def likelihoodStrMeansNODATA(likelihood): # likelihood is in the string format, directly passed from C 
	for i in likelihood:
		if (ord(i) != PY_NODATA_VALUE):
			return False
	return True


# Returns the length of the first not empty list in node['data']
def guessNumberStates(node):
	for cell_data in node["data"]:
		if (cell_data):
			return len(cell_data)

"""
	This function will be called from C code

	It will import the module 'py_module_filename' and call the function 'process' that should be implemented in it., passing as arguments:

		- reference dataset			
		- list of python dictionaries in the form: 
			{ name: nodename,
			  states: number of node states 
			  <bytearray>data: two dimensional array with node data per raster cell: [cell][state]
			}
		- iteration number


	node_list_src should be and array of elements [string, int, string ] : [nodename, states, bin_data]
		- 'data' should be a python string with length equal to rastersize * states

"""


def run(py_module_filename, geo_ref_filename, node_list_src, iteration, node_list_dst):

	filename, file_extension = os.path.splitext(geo_ref_filename)
	
	if file_extension in [".tif", ".tiff"]:
		dataset = gdal.Open( geo_ref_filename, GA_ReadOnly )

	elif file_extension == ".shp" :
		driver = ogr.GetDriverByName('ESRI Shapefile')
		dataset = driver.Open(geo_ref_filename, 0) # 0 means read-only. 1 means writeable.

	else:
		print 'python_c.py: run: Unknown raster extension', file_extension
		dataset = None
		# return []

	if dataset is None:
	    print 'python_c.py: Could not open %s' % (geo_ref_filename)
	    # return []


	# print "run: node_list_src: ", node_list_src

	nodes_data = []

	for node in node_list_src:

		new_node = {}

		new_node["name"] = node[INDEX_NAME]
		new_node["states"] = node[INDEX_STATES]
		# new_node["data"] = bytearray(node[2])
		new_node["type"] = node[INDEX_TYPE]
		
		if (new_node["type"] == PY_DISCRETE):

			new_node["data"] = [];

			for i in range ( len(node[INDEX_DATA])/new_node["states"] ):
				#new_node["data"].append(bytearray((node[2][i*new_node["states"]:(i+1)*new_node["states"]])))
				new_node["data"].append((node[INDEX_DATA][i*new_node["states"]:(i+1)*new_node["states"]]))

				if (likelihoodStrMeansNODATA(new_node["data"][i])):
					new_node["data"][i] = [] # for NODATA pass an empty list

			data_decimal = []
			for i in range (len(new_node["data"])):
				data_decimal.append(map(ord,list(new_node["data"][i])))
			new_node["data"] = data_decimal

		else:
			new_node["data"] = node[INDEX_DATA]


		nodes_data.append(new_node)


	# print "run: process input: ", nodes_data


	signal.signal(signal.SIGALRM, timeout_handler)
	signal.alarm(thirdparty_timeout);

	if (not os.environ['PYTHONPATH']):
		print "python_c.py: PYTHONPATH environment does not exist"


	print "python_c.py: run: PYTHONPATH: ", os.environ['PYTHONPATH']

	old_cwd =  os.getcwd()
	# print os.getcwd()

	# print py_module_filename
	# print os.path.join(os.getcwd(),py_module_filename)

	os.chdir(WORKING_DIRECTORY);

	my_env = os.environ
	os.environ={} # prevent third-party module to get access to the environment

	# print os.getcwd()

	try:

		print "python_c.py: Import", py_module_filename # Two times to see debug messages in case of error

		# redirect stdout
		# sys.stdout = open('thirdparty_stdout.txt', 'w')

		print "python_c.py: ********** PYTHON STDOUT begin **********"

		thirdparty = importlib.import_module(py_module_filename)
		res_nodes = thirdparty.process(dataset, nodes_data, iteration)
		
		print "python_c.py: Process function has finished."

	except Exception, exc: 

   		print "\npython_c.py: ERROR: Exception in supplied python script! :",exc

   		# print "python_c.py: ERROR: Maybe module", py_module_filename, "could not be imported" 

   		signal.alarm(0)

   		# old reference 
	   	os.environ = my_env 
   		os.chdir(old_cwd);

   		return -1


   	# old reference 
   	os.environ = my_env 
   	os.chdir(old_cwd);

   	signal.alarm(0)

   	# print "run: process output: ", res_nodes

   	if (not(validResultData(res_nodes, dataset.RasterXSize * dataset.RasterYSize, True))):
   		return -1

	for res_node in res_nodes:

		new_res_node = []
		new_res_node.append(res_node["name"])

		if (res_node["type"] == PY_DISCRETE):
			n_states = guessNumberStates(res_node)
		
			new_res_node.append('')

			# to speed up the process a bit
			cell_nodata_str = '';
			for i in range(n_states):
				cell_nodata_str += chr(PY_NODATA_VALUE)

			likelihoods = [] # creating a list with all likelihoods in string type and then joining them is a much faster solution than just concatenatic one by one
		

			for idx, cell_data in enumerate(res_node["data"]):
				#print(cell_data)
				#new_res_node[1] += str(cell_data)

				if (not cell_data): # empty likelihood, meaning NODATA for that cell
					# new_res_node[1] += cell_nodata_str
					likelihoods.append(cell_nodata_str);
					
					
				else:	
					# for state_likelihood in cell_data:
					# 	new_res_node[1] += chr(state_likelihood)

					#new_res_node[1].join(map(chr,cell_data))
					
					likelihoods.append(''.join(map(chr,cell_data)))
					

				# if ((iteration == 0) and (idx == 0)):
				# 	print("python_c debug: cell 0: cell_data: ", cell_data)
				# 	print("python_c debug: cell 0: new_res_node[1]: ", new_res_node[1])
				# 	for i in range(8):
				#  		print("-> ",new_res_node[1][i], ord(new_res_node[1][i]))

				# if (idx%2000 == 0):
				# 	print idx

			
			new_res_node[1] += ''.join(likelihoods);

		elif (res_node["type"] == PY_CONTINUOUS):
			new_res_node.append(res_node["data"])

		else:
			print "python_c.py: res_node with wrong 'type' key. It should be 'PY_DISCRETE' (",PY_DISCRETE, ") or 'PY_CONTINUOUS' (", PY_CONTINUOUS, ")" 


		node_list_dst.append(new_res_node)

	# print "python_c: run: node_list_dst: ", node_list_dst

	return len(node_list_dst)



if __name__ == "__main__":
    main()
