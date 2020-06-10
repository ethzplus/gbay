# gBAY
<!-- Last modification date: 04.06.20 17:29 -->

## Table of contents
* [Introduction](#introduction)
* [Requirements](#requirements)
* [Setup](#setup)
* [Using gBay](#using-gbay)
* [Running gBay from the command-line](#running-gbay-from-the-command-line)
* [Advanced use](#Advanced use)
* [License](#license)
* [Source Code blumap](#blumap)
* [Source Code GUI](#gui)



## Introduction<a name="introduction"></a>

Bayesian networks (BNs) are a tool to represent socio-ecological systems, as they can take into account both qualitative and quantitative data, while the associated uncertainties are explicitly shown and propagated through the network. Furthermore, relationships between variables in the BN are represented graphically, creating a transparent model structure that can facilitate communication with stakeholders. Because of these advantages, BNs are increasingly used to model land-use change and ecosystem services.

gBay is a tool that links BNs to spatial (raster or vector) data. For each pixel (or polygon), the values in the input data are used as evidence in the network, and inference is performed to obtain the posterior probability distribution of the target nodes. Then, the posterior distributions of the target nodes are written into a new spatial file.

gBay supports quasi-dynamic BNs – running the network over multiple time steps, where the output of one time step is an input to the next, which can be used to reflect feedback loops. Advanced users can also modify spatial data directly in gBay using Python. This allows users to take into account spatial interactions such as neighbourhood effects.

## Technical background and requirements<a name="requirements"></a>
This project has been developed and tested in x86_64 GNU/Linux (Ubuntu 16.04 LTS)

The gBAY project consist on two separate parts:

* The command-line tool named **blumap**.
* A GUI (graphical user interface) based on NodeJS called **gBay**.

When using the front-end (GUI), all input data will be stored in the filesystem, and all the settings will be translated into command-line arguments that will be run in a separate process. All output files will be stored in a directory that will be compressed and send back to the user.

To be able to use all the functionalities of gBAY you should build BLUMAP and start the GUI.

Requirements:
* NodeJS >= 7.4.0
* Netica-C API

## Setup<a name="setup"></a>

### Setting up a server
First, a future manager of the platform has to install a linux server. Follow instructions on https://ubuntu.com/download/server
<!-- You need to install Ubuntu 16.04 as stated in the previous section (newer versions should also work) following the instructions in their webpage-->

<!-- When the platform is running, it will open a port to listen for HTTP connections. That means if you set it up in your computer using the default settings (listening on port 9500, you could open a browser and type in the address bar: http://localhost:9500 and it will show you the platform)-->
<!-- Then, in case you want to make it publicly accessible, you will need to set up a web server that forwards the traffic from whatever domain you have registered to the local port 9500-->

<!-- An example : 
1.- We need a virtual server to run gBAY, for that I apply for a virtual server from ETH using a form where you specify which operating system you would like to have, how many CPU's, disk space, memory and some other things. Once it is done I receive the address of this server, IP or hostname. In our case it is 'irl-plus-s-005.ethz.ch
2. Then you'd need to register a domain name, like "gbay.ethz.ch". For that, we have registered the domain 'gbay.ethz.ch' using the Netcenter tool to register .ethz.ch domains, and I set this domain name as an alias for our server called 'irl-plus-s-005.ethz.ch'
3. Then I could log in the virtual server, for example using the program 'ssh' to establish a remote connection and install gBAY as explained. Once it is done, the local port 9500 is open. Now, HTTP uses the port 80 by default, so you need a web server that forwards everything coming to the domain name 'gbay.ethz.ch' to the port 80, to the local port 9500. To do that we use nginx web server, but you could another one, like apache for example. In case you want to use HTTPS, the port is the 443 and the web server should be configured accordingly.
<!-- This part does not belong to the software documentation. It could be applied to any other web applciation.-->




### Setting-up blumap
BLUMAP is written in C and it includes a Makefile to facilitate the compilation and linking process.

* libgdal-dev (https://gdal.org/)

    In Ubuntu you can install it by typing in a console: `'sudo apt-get install libgdal-dev'`

* python-gdal (https://gdal.org/python/)

    In Ubuntu you can install it by typing in a console: `'sudo apt-get install python-gdal'`

* python-dev 

    In Ubuntu you can install it by typing in a console: `'sudo apt-get install python-dev'`


* Netica-C API (https://norsys.com/netica_c_api.htm)

The programs to be built are:

* **blumap**: This is the main application that runs bayesian inference using GIS data. It is the only one needed if a GUI is not necessary.
* **net2json**<a name="net2json"></a>: This program translates a .dne file into JSON format.  It is needed for the GUI to visually represent the network in the browser.
* **vectornodes**: This program will read a vector file and print out the node names found in the attribute table. It is needed for the GUI to present the user with the network filled nodes when a vector file is uploaded.

To build blumap you only need to do (***where and how?***): <!-- the commands are the next lines: "cd blumap" and then "make"-->
```
cd blumap
make
```

Inside the blumap directory there are all the source files and the Makefile required to build the command-line applications needed for the project. 

When the programs 'blumap', 'net2json' and 'vectornodes' are built, you are ready to run the front-end. It is recommended since it adds many functionalities to the application, such as being able to save runs, or notify via e-mail when specially long inference is run, but if you are not interested and just want to use the command-line tool, you can jump to the section [Running gBay from the command-line](#running-gbay-from-the-command-line)



### Setting-up the frontend

The GUI consists of a NodeJS application that will start a local web server listening by default in the port 9500.

Before starting the application it is needed to install all its dependencies using the command

* `npm install`

To start the server simply run:

* `npm start`

Many settings for this application are defined in the file 'config.js' such as the listening port, paths, url,  etc.

There are many settings that can be configured changing the values in `'config.js'` such as the paths to several files, the port where the app listens for connections, the mail settings, etc.

If setting up gBay in a public server, please consider to take some [precautions](#warning).

Once started it will display an HTML form where a Bayesian network file should be uploaded. Once is it uploaded, the user will be presented with a graphical representation of the network (internally the command ['net2json'](#net2json) is used).

This display is also a form that will be submitted to the server at the moment the button 'Run' is clicked.

The web server will then take care of:

   * Store all files into temporary directories.
   * Create an unique output directory.
   * Translate the form fileds into command line arguments to be supplied to the blumap command.
   * Spawn a new 'blumap' process with all the arguments
   * Pipe the blumap standard output and standard error to a websocket to send back live log information to the user (The checkbox 'Show console' in the GUI will display this information). Useful for debugging.
   * When the process is finished, it will compress all the output files and send it back as the POST response.

In case of a very long run, an email could be supplied in the GUI so when the process is finished an email will be sent with a link to retrieve the output.

Currently only one process is allowed per user session to prevent a user to spawn many processes at the same time and thus potentially blocking the server.


## Using gBay<a name="using-gbay"></a>

### 1. Upload a BN

In the first step, the BN we want to run should be uploaded in the **.dne** or **.net** format (as created in Netica or another BN software). This is done by using “choose file” and selecting the network file, or dragging and dropping the network file into the application. Then, we click “Proceed”. 

Once a network has been uploaded, it is visualized in the GUI, displaying the nodes with their states (in case of continuous nodes, the upper bound of each interval is displayed), and the links between them. The nodes can be moved around. 

### 2. Configure the run

1. Select target nodes

    When hovering with your mouse above a node, additional options appear. A node can be selected as a target node, which means that the output of running the network will contain the posterior probability distribution of this node. When we select a node as a target node, a target icon will appear in its upper right corner. You may want to save the configuration of the run (including the information about which are the target nodes, etc.), in case you will run it again later. You can also upload a previously saved configuration. 

2. Set non-spatial evidence

    By clicking on the state of a node, we set that state as hard evidence, which is then shown in bold. This is useful for non-spatial evidences, for nodes which have the same state over the whole study area, such as an agricultural policy. It is also possible to set soft evidence hovering with the mouse and clicking 'Set likelihood' and setting the likelihood per each state manually.

### 3. Spatial inputs

1. Raster

    In the raster mode of gBay, the input data as well as outputs are in raster (.tif) format. The input rasters correspond to nodes that we have data on. We can upload input rasters by simply dragging the .tif file from our file explorer to the corresponding node in gBay. When data is added to a node, the colour of the node changes, and the name of the .tif file is displayed. 

    The input rasters should all have the same extent, cell size, and spatial reference (coordinate system).

    When the input data represents **hard evidence** (we know the state of the node at each pixel, with 100% certainty, e.g. we know that the land cover of a pixel is forest), then the input raster has one band, where the value of each pixel corresponds to the number of a state of the node. For continuous nodes, the value of the pixel represents the real value (e.g. forest cover of 75%). 

    When we use **soft evidence** (a probability distribution for each pixel), the input raster should have as many bands as the number of states of the corresponding node. Each band represents the probability of a state (e.g. the probability that the land cover of a pixel is forest). The values of all bands should sum up to 100.

2. Vector

    When using vector data (zipped **.shp** files or **.gdb** ESRI geodatabase files), the input nodes are represented in the attribute table of the dataset.

    For **hard evidence**, the attribute table should have a column corresponding to the name of the input node, with values corresponding to the states of the node. A value of 0 means NODATA

    For **soft evidence**, each state of the input node should be represented by a column of the attribute table, with values representing the probabilities of the states (which should sum up to 100). The column names should be the node name, followed by two underscores, an 's' (from state) and the number of the state. If all values are 0, it means NODATA.

    Example: to set soft evidence on node 'lu_t0', with three states, the attribute table should contain the columns:

    `lu_t0__s1, lu_t0__s2, lu_t0__s3`

    The vector file can be added to the network by dragging it to the box labelled **“Vector file”**. Please note that gBay does not modify the geometry of the vector file, but simply performs inference on each object, using information from the attribute table.

### 4. Run the bayesian inference

You can set manual evidence for the nodes byt clicking on the state of the node to set hard evidence, or by hovering on the node and selecting 'Set likelihood' and entering the likelihood for each state of the node .

Once you have set up the network, selected the target nodes, and uploaded the spatial inputs, you can click on **“Run”** to run the network. gBay will use your spatial data to perform inference in the BN for each pixel or feature, and produce an output of the results.

Any potential **errors or warnings** will appear as pop-ups. If you want to see the progress of the processing, select the option to “Show console”.

If you have a complex network and are running it with a **large** spatial dataset, this may take some time. In this case, you should enter your **email** address, where you will receive a notification when the process is completed, along with a link where you can download the data. 

![alt text](images/gbay_basic.png "gBay basic use")

For more advanced uses of gBAY, jump to [Advanced use](#advanced-use)

###  5. Outputs

1. Raster

    The output of a gBay run is a posterior probability distribution raster of each target node (named target_node.tif). This raster has one band for each state of the target node, where the value represents the probability of the state. In addition, the last band of the raster represents the most probable state.

    In addition, for each target node, some metrics of the posterior probability are calculated and stored in an additional raster file called target_node_stats.tif. For discrete target nodes, this file contains one band with Shannon’s evenness index of the posterior probability distribution: 

    `J = H'/Hmax, where H' = - SUMi(pi*ln(pi) with pi=ni/N , Hmax = ln(N), pi is the probability of state i and N is the number of states.`
    (Compared to Evenness Index: number of individuals N = number of species S, so S not used in formula)

    The index is a standardized measure of entropy (can be compared between nodes with different numbers of states) and expresses uncertainty. It has values between 0 and 1, where 1 denotes a uniform distribution between all possible states (maximum uncertainty), and 0 denotes complete certainty that the output node is in a specific state.

    If the target node is continuous, the stats output contains six bands with the following values: 

    	1. Evenness index
    	2. Mean
    	3. Median
    	4. Standard deviation

**Overview of gBay spatial inputs and outputs** 

Input format:

* **Raster**: A .tif file per input node

	* **Input values:**

    	* Hard evidence: Value = node state (discrete nodes) or continuous value (continuous nodes)
    	* Soft evidence: One band per state. value = probability of state 

	* **Output:**

    	* target.tif: One band per state. Value = probability of state, plus an additional band with value = most likely state
    	* target_stats.tiff with bands:
    		1. Evenness index

    		      Only for continous nodes:

    		2. Mean
    		3. Median
    		4. Standard deviation

* **Vector**: One .shp file or geodatabase .gdb (It reads the attribute table)

	* **Input values:**
    	* Hard evidence: Column of attribute table with same name as input node: Value = node state (discrete nodes) or continuous value (continuous nodes)
    	* Soft evidence: Columns with node name and state number in the form (lu_t0__s1, lu_t0__s2, lu_t0__s3, ...):  Value = probability of state

	* **Output:**

        * target.shp with same geometry as input: Attribute table with a column for each state of the target node: Value = probability of state, plus an additional column with value = most likely state.


## Running gBay from the command-line<a name="running-gbay-from-the-command-line"></a>

It is possible to run gBAY from the command line by entering in the 'blumap' directory and running './blumap' with several arguments

Run: `'./blumap --help'` to get a list of all the possible arguments.

The most basic run of blumap need a bayesian network, a list of target nodes and at least one raster file to fill a node. This should be as follows:
`'./blumap -b bayesian_network.dne target_nodenade raster.tiff'`


## Advanced use<a name="advanced-use"></a>

### Multiple iteration

It is possible to run inference in gBAY several times while copying the likelihood of one node into another one. To do that you could set the number of iterations in the input filed in the GUI, and, by hovering on a node, click on 'target' and then click on another node. Be aware the number of the states should be the same.

In the command line you get the same results using '-i number_iterations' and '-l source_node=target_node'

### Intermidiate processing

There is the possibility to modify the node likelihoods using this feature. It could be use in cases such as:

* Changing evidences over time (e.g. modifying the evidence on a node describing agricultural policy, which changes between different time steps).
* Implementing boundary conditions (e.g. limiting the number of pixels that can change from “forest” to “agriculture” under a specific land use change scenario).
* Calculating neighbourhood values (e.g. calculating the percentage of forest cover in neighbouring pixels within a specified distance).
* To set up a node likelihood at the start of the application according to some other nodes data.

To do that you can upload a python script using the 'Intermidiate processing' section in the GUI and selecting any nodes in the GUI which this script will refer to when it runs by hoverin in the ndoe and clicking on Python.

In the command-line you can do the same using the '-m python_module' and '-n python_input_node' arguments. 

There is a file 'blumap/instructions_thirdparty.txt' where the use is explained. Basically you should implement a function with the following prototype:
```python
process(GDALDatasetH dataset, list nodes_data, int iteration)
```
* dataset: It contains geographical information of the region being used by the current gBay run. gBay uses GDAL to operate with GIS files 
* iteration: 0 means before the first network inference. It is commonly used to set nodes beliefs depending on another nodes. 
* nodes_data: Data about the nodes we selected as input python nodes. It is a list of python dictionaries with the following keys: 
	* name: Node name 
	* states: Number of states of the node
	* data: Likelihood / Values of the node ordered by cell, in case of raster, or feature in case of vector. 

This function should return a list with the same structure as nodes_data: It must be a list of dictionaries. In the case that only one node is modified, the list will have one element. The applicattion will copy this data into the node likelihood for the next iteration.

In case the output is not valid, an error message will be displayed in the console and the intrmidiate processing will be ignored. 

Be aware that the intermidiate processing could return a list with the correct format but still be wrong from the bayesian network perspective, such as setting an impossible likelihhod to a node given their parent states. In that case an error from Netica will be displayed.

We provide a python file called "node_utils.py" with many helper funcions to work with this nodes structure.

It is important to set the PYTHONPATH environment variable to the location of 'python_c.py', 'node_utils.py' and the path where the third-party python script is located.

<a name="warning"></a>
**Be careful when setting up gBAY in a public server: If you do not protect or isolate the 'blumap' process, a malicious attacker could run python code without restrictions using the Intermidiate processing functionality. It is recommended to use some security measures to prevent that, such installing AppArmor and restricting the 'blumap' CPU use, networking capabilities, etc**

## License<a name="license"></a>

[GPL-3.0](https://www.gnu.org/licenses/gpl-3.0.en.html).


## Source code documentation blumap<a name="blumap"></a>

It uses many standard linux C libraries, and some sources detailed here:

* **raster.c** and **raster.h**: Used to manage raster data. Functions included:

    * `GDALDatasetH loadRaster(char *filename, int printinfo)`: Loads a raster file given its filename, optionally printing some information about it.

    * `void closeGdalRaster(GDALDatasetH hDataset)` : Closes a raster file

    * `GDALRasterBandH getRasterBand(GDALDatasetH hDataset, int band, int printinfo)`: Read the band 'band' and return it, optionally printing some information about it.
    * `int readRasterData(float **pafScanbuf, GDALDatasetH hDataset, int bands, int debug)`: Read raster bands into pafScanbuf.

        If 'bands' is zero, it will read all bands. This function will reserve memory for pafScanbuf that should be freed afterwards. It will read band by band using GDALRasterIO, storing the values consequently in pafScanbuf so values are in the form: [ band1cell1, band1cell2, ...,  badn1celln, band2cell1, ...]

        Returns the number of bytes read.

    * `int checkSameRasterMetaInfo(GDALDatasetH* hDatasets, int n)`: Checks if the datasets have the same projection  and same size. To prevent the application of starting in case they mismatch.
    * `GDALDatasetH copyRaster(char *pszSrcFilename, char *pszDstFilename)`: Copy a raster by filename and return the dataset of the new copy
    * `int writeGeoTIFF(char *filename, GDALDatasetH refDataset, void** data, int n_bands, int nodata, GDALDataType dataType)`: Write a geoTiff with the data passed as argument. Used to create the result tiff for a target node.

* **vector.c** and **vector.h**: Used to manage general vector data. Functions included:

    In vector.h the structure `'vector_t'` is defined to manage vector nodes data as well as the regular expression to parse the column names in the vector attribute table. Function included:

    * `int loadVector(char *dirname, vector_t* vector, int printinfo)`: Loads a vector file given its filename, optionally printing some information about it.
    * `int checkSameVectorMetaInfo(vector_t* vectors, int len)`: Checks if the datasets have the same projection and the same number of features.
    * `int writeSHP(char *filename, GDALDatasetH refDataset, void** data, int n_states)`: It will create a shapefile with an attribute table with a column for every state in n_states. data is a char array [state][feature]
    * `int getNodeNamesFromVector(vector_t vector, char*** nodenames, int verbose)`: It reads the columns from the attribute table differentitaing between findings and likelihoods to get the number of possible nodes. It will store the the names in 'nodenames' and returns the number of nodes. Nodenames should be freed afterwards.

        Avoid printing out anything else in this function since the output will be used from the GUI to present the user with the nodes that have matched a column name from the attribute table.

* **dbf.c** and **dbf.h**: Used to manage vector data in GDB format. Functions included:
    * `int DBF_Open(char* filename, DBF* dbf)`: Opens a DBF file
    * `void DBF_Print(DBF dbf)`: Prints the DBF filename, the node name it matched and all entries
    * `void DBF_Free(DBF dbf)`: frees the DBF struct
    * `int DBF_getValueforStateName(DBF dbf, char* state_name)`: Gets the value for a given state name.

* **bn_network.c** and **bn_network.h**: Utilities to manage bayesian networks in Netica format. All the functions defined in bn_network.h extend their equivalent from the Netica C-API, adding some error checking and additional functionalities. Refer to the [Netica C-API](https://norsys.com/onLineAPIManual/index.html) for further documentation. Functions included:

	* `int initBayesNetworkEnv(char* license, int verbose)`: Initialize the Netica environment. This is the first function that should be called when using the rest of the functions defined here. You should supply a valid license.
	* `int closeBayesNetworkEnv(int verbose)`: Close the environment
	* `net_bn* readNet(char* filename)`: Read a BN from a valid .dne or .neta file.
	* `int CompileNet(net_bn* net)`: Compile the network. It is necessary to make inferences.
	* `void closeNet(net_bn* net)`: Close the network
	* `void closeAllNets(net_info_t* nets_info, int n)`: Close all the networks.

        This is a legacy function intended to make use of many networks at a single run. It is not used anymore.

	* `void PrintNodeList (const nodelist_bn* nodes)`: Print node list information,
	* `int getNodeStateValueStr(node_t* node, int state, char* res)`: Get the valueof a given state for the node
	* `int setNodeFinding(node_t* node,  float value)`: 
        * If node is DISCRETE: value should be the node state, in the range [0..node_states-1].
        * If node is CONTINUOUS: value is the real value.
	* `int resetNodeFindings(node_t* node)`: Reset the node findings
	* `node_t* getNodeInNet(net_bn* net, char *nodename)`: Get a node given its name
	* `node_t* getNodeByName(char *nodename, net_info_t* nets, int n_networks)`: Get a node given its name in the network list. not used anymore
	* `int getNodeType(node_t* node)`: Returns DISCRETE_TYPE if node is discrete, or CONTINUOUS_TYPE if continuous type
	* `int getNodeNumberStates(node_t* node)`: Get the node number of states.
	* `const char* getNodeTitle(node_t* node)`: Get the node title.
	* `const nodelist_t* getNetNodes(net_bn* net);`: Get a list of all nodes in a network.
	* `char* getNodeName( const node_bn* node)`: Get the node name
	* `int isLeafNode(node_t* node)`: Returns 1 if node is leaf (it has no parents), 0 otherwise.
	* `int numParents(node_t* node)`: Returns the number of nodes that are direct parents of the node.
	* `float* getNodeBeliefs(node_t* node)`: Get an array of values with the node actual likelihood.
	* `double getNodeExpectedValue(node_t* node, double* std_dev)`: Get the node expected value
	* `int enterNodeLikelihood(node_t* node, float* likelihood)`: Enter a likelihood for a node
	* `int lengthNodeList(const nodelist_t* nodelist)`: Get the length of a nodelist_t
	* `int checkNetFindings(net_bn* net)`: Print out all findings of the nodes in the network
	* `char* getNetName(net_t* net)`: Get the name of a bayesian network.
	* `int printNetworkInfo(net_info_t net_info)`: Print out information about all the nodes in the network, together with the node types and likelihoods.

        Used when the verbose option ('-v' or '-w' is passed in the command-line). It is meant for debugging purposes. Try not to use it since it slows down the process.

	* `int printNetworkInfoJSON(FILE* fd, net_info_t net_info)`: Prints the network in JSON format.

        Intended to be used with net2json so the GUI can easily parse a JSON and present the network to the user.

	* `net_info_t* getNetByFileName(net_info_t* nets, int n, char* filename)`: Get a network from a network list by filename
	* `int printNode(node_t* node, int mode)`: Print out all the information for a node.It is meant for debugging purposes. Try not to use it since it slows down the process.
	* `double GetNodeValueEntered(node_t* node )`: Get the value entered for a node.

* **statistics.c** and **statistics.h**: Utilities to run statistics to include in the output files in case of cotinuous nodes. Functions included:

    * `double entropy(unsigned char* vector, int len)`:
	* `double mean(unsigned char* v, int len, double* breaks, int n_breaks)`:
	* `double quantile(unsigned char* v, int len, double* breaks, int n_breaks, int q)`:
	* `double variance(unsigned char* v, int len, double* breaks, int n_breaks)`:
	* `double custom_std_dev(unsigned char* v, int len, double* breaks, int n_breaks)`:

* **execution.c** and **execution.h**: Functions to be able to cache some inputs and outputs of network inferences and compare them afterwards to potentially increase the speed of the whole blumap process.

    It defines the struct Execution_t to save data about executions.

   	* `int initExecution(Execution_t* execution, int node_number, int output_node_number)` : Initilaise execution for a node
	* `void freeExecution(Execution_t* execution)`: Free the execution
	* `int E_initInput(Execution_t* execution, char* nodename, int inputs_len )`: Initialise execution input node
	* `int E_initOutput(Execution_t* execution, char* nodename, int outputs_len )`: Initialise execution output node
	* `Execution_t* findExecution(Execution_t** executions, int executions_len, Execution_t execution)`: Try to find an Execution_t matching the provided Execution_t inputs 
	* `int E_addNodeFinding(Execution_t* execution, char* nodename, float* findings, int findings_len)`: Add a finding for a node finding into a given Execution_t
	* `int E_addNodeFinding_p(Execution_t* execution, char* nodename, float** findings, int findings_len)`:  Same as E_setNodeFinding but findings is an array of pointers to data
	* `int E_setTargetBeliefs(Execution_t* execution, char* nodename, float* beliefs, int beliefs_len)`:_ Set target beliefs for a node.
	* `void printExecutionVerbose(Execution_t* execution, int cell_offset, int cell_total, int iteration, int iteration_total)`: To print verbose Execution data. For debugging purposes.
	* `void printExecution(Execution_t* execution)`:To print Execution data. For debugging purposes.
	* `void printExecutionMetadata(Execution_t* execution)`: T
	* `int logExecution(Execution_t* execution, int cell_offset, int iteration)`: To print Execution data into LOG_EXEC_FILENAME as defined in execution.h. 
	* `Execution_t* copyExecution(Execution_t* execution)`: Copy an Execution_t
	* `int resetExecution(Execution_t* execution)`: Reset the Execution_t


* **embed_python.c** and **embed_python.h**: Together with `'python_c.py'` these files provide the functionality to run intermidiate processing using a python file as described in the [README](README.md)

    Here it is defined the `struct pyNode_t` which be used to translate a node data from C to Python. It is a representation of the node beliefs for each cell. There is a pyNode for each of the nodes select by the user as a python input node.

    The following functions are defined:

    * `int initPythonEnv(char *python_script_name)`:  Initialize python environment.
	* `void finishPythonEnv(void)`: Finish the python environe.ment.
    * `pyNode_t* createPyNode(char* nodename, int node_states, void* data, int data_len, int node_type)`
	* `void freePyNode(pyNode_t* py_node_info)`: Free the node
	* `int pyAddData(pyNode_t* py_node_info, char *buff, int len)`: For DISCRETE nodes: to add the beliefs of the node.
	* `int pyAddValue(pyNode_t* pyNode, double val)`: For CONTINUOUS nodes: to add the value of the node
	* `void resetPyNode(pyNode_t* pyNode)`: Restet the pyNode. This function does not free the memory, just reset the length so it is possible to reuse it.
	* `float getValueFromPyNode(pyNode_t* pyNode, int cell)`: Returns the value for a given cell in the pyNode which node is CONTINUOUS.
	* `float getStateValueFromPyNode(pyNode_t* pyNode, int cell, int state)`:  Return the state likelihood of a DISCRETE node for a given cell and state
	* `void getBeliefsFromPyNode(pyNode_t* pyNode, int cell, float** beliefs)` : Fill the 'beliefs' array with the likelihood of a pyNode for a given cell for a DISCRETE node.
	* `void printPyNode(pyNode_t* pyNode, int cell)`: Print information for a pyNode. For debugging purposes
	* `int third_party_python_script(char* python_script, char* py_third_party, char* raster_ref_filename, pyNode_t* src_node_list[], int len, pyNode_t* dst_node_list[], long iteration, int debug)`:

        This function is the one that will run the python script supplied by the user and will pass the pyNodes filled with their beliefs, and a list of destination pyNode_t's whit the likelihoods of the nodes the script has modified.

        This function takes care of converting data from C to Python. It will basically use functions defined in 'Python.h' to create a python tuple with five elements:

        * The third-party script.
        * An input raster filename.
        * The input node list.
        * The number of the current iteration
        * The list of output nodes

        Afterwards, it will import the file python_c.py, initialize the Dataset and call the 'run' function
passing all the arguments. When this function returns it will check the result for errors and in case everything is OK it will fill 'dst_node_list' and return its length.

* **c_python.py**: This module defines the function:

       `def run(py_module_filename, geo_ref_filename, node_list_src, iteration, node_list_dst)`

       This function takes care of converting the data passed from 'third_party_python_script' to a more user friendly python dictionary so each node has the following keys:

       * name: Node name
       * states: Node numebr of states.
       * type: Node type (PY_DISCRETE or PY_CONTINUOUS).
       * data: node beliefs/values arraged by cell.

       It will also open 'geo_ref_filename' using python to supply the 'process' function defined by the users in their third-party script 'py_module_filename' with all the relevant information: a GDAL reference dataset, the number of the current iteration and a list of the nodes the user selected as input nodes.

       It will unset the environment before running so the third-party script does not have access to the environment. And set it back later.

       It will try then to import the python script, run the 'process' function, get back the results, make some error checking (like the likelihoods does not add up to 100), convert back the data so it is understandable by ythe function 'third_party_python_script', put it in node_list_dst and return the length of the list.

       As explained in the [README](README.md): **Be careful when using third-party scripts from unknown sources since a malicious attacker could potentially run python code without restrictions using this functionality. It is recommended to use some security measures to prevent that, such installing AppArmor and restricting the 'blumap' CPU use, networking capabilities, etc**

## Source Code documentation GUI<a name="gui"></a>

The most relevant source files are displayed here:

* **server.js**: Main gBAY module. It takes care of setting up the web server, handle the sessions, the websockets, check that the Netica license is found, and start all the other modules
* **routes/index.js**: Takes care of handling all the HTTP requests, to translate the POST fileds into command line arguments, sanitizing the user inputs.
* **lib/command.js**: It exports two ways of running a command:
    
    * run_buffered: For 'net2json' and 'vectornodes', to run them and send back their output in one chunk
    * run_live: For 'blumap', to run and send via websocket the standard output as soon as a chunk arrives, so the user can see live what is happening using the console.

* **config.js**: Default configuration parameters.