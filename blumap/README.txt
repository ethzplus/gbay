gBay uses a bayesian network and GIS files to produce a new GIS file with information about a network node.

The application will set network nodes findings from GIS data for every cell or feature in the GIS file, depending on the type of input (raster or vector)


Description:
-----------

BLUMAP will iterate over every GIS element, reading its value and applying to the corresponding node in the network. The user may upload as many GIS files as desired.

- Raster mode: 
  -----------
In case of just one band, the value for every element in this band will be considered as a node 'hard evidence'. In case of many bands (there should be the same number of raster bands as node states), the value for every element will be considered as a node 'likelihood' or 'soft evidence'.

The value for a node likelihood will be taken from the bands in the same order, that is, first band means first node state likelihood, and so on. 

- Vector mode:
 ------------
The inputs are read from the attribute table. In case a column name matches a node name, it will be considered as a node 'hard evidence'. In order to input a node 'soft evidence' the column names must be the node name, followed by two underscores, an 's' (from state) and the number of the state. It is expected to find as many columns named in this way as the number ofstates of the node.

Example: node 'lu_t0', with three states   --> columns: lu_t0__s1, lu_t0__s2, lu_t0__s3


The value for a node hard evidence has a different meaning depending on the type of node it is applied. 

	- Discrete node: Value is the state of the node. Ranging from 1 to node_states.
	- Continuous node: Value is the real value of the node.

BLUMAP will iterate over each GIS file element, reading its value and setting the network node finding. In case of NODATA, the node findings will be reset. After all GIS elements have been read and a finding has been set up for their nodes, a network inference will be run, and the information for the target node for that element (raster cell of vector feature) will be stored. 

When all elements have been iterated over, the application will write a file with the stored results.


Output:
------

The output will have the same name as the target node, and it will contain the node beliefs, plus the number of the state with the maximum value. Thus, the output file will have node_states+1 different bands (rasters) / attribute table columns (vector).

	- Raster mode: the bands will match the node states. That is, the first band will have the values of the first state of the target node for all the study area, and so on.

	- Vector mode: the result file will have an attribute table with target_node_states+1 columns, each value matching the target node state value. Plus one with the number of the state the maximum value.

A second GIS file will be returned with some additional statistics. Depending on the target node type, the output file will be different:

	- Target node is 'Discrete': The statistics file will contain one band / column with the entropy.
	
	- Target node is 'Continuous': The statistics file will contain 4 bands: 

		1. Entropy
		2. Mean
		3. Quantile
		4. Standard deviation (customized function)


Additional features:
--------------------

- Set node evidences directly:

	In case a GIS file matches a node with an evidence set up this way. The raster file will take precedence and the fixed evidence will be ignored.

- Multiple inference / Linking nodes:

	The user can define any number of 'source' nodes so their belief vectors will be used as another 'target' nodes likelihoods for the next iteration. It only makes sense when more than 1 iteration is selected.

- Set node findings via python scripts.

	The user can modify any network node likelihood by applying a python script and selecting which nodes it needs as an input. This script will be run at the end of every iteration.

	