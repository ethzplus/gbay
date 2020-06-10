# gBAY

## Table of contents
* [Introduction](#introduction)
* [Requirements](#requirements)
* [Setup](#setup)
* [Using gBay](#using-gbay)
* [Running gBay from the command-line](#running-gbay-from-the-command-line)
* [Advanced use](#Advanced use)
* [License](#license)

## Introduction<a name="introduction"></a>

Bayesian networks (BNs) are a powerful tool to represent complex socio-ecological systems, as they can take into account both qualitative and quantitative data, while the associated uncertainties are explicitly shown and propagated through the network. Furthermore, relationships between variables in the BN are represented graphically, creating a transparent model structure that can facilitate communication with stakeholders. Because of these advantages, BNs are increasingly used to model land use change and ecosystem services.

gBay is a toolbox that links BNs to spatial (raster or vector) data. For each pixel (or polygon), the values in the input data are used as evidence in the network, and inference is performed to obtain the posterior probability distribution of the target nodes. Then, the posterior distributions of the target nodes are written into a new spatial file.

gBay supports dynamic BNs – running the network over multiple time steps, where the output of one time step is an input to the next, which can be used to model feedback loops. Advanced users can also modify spatial data directly in gBay, which allows them to take into account spatial interactions such as neighbourhood effects.

## Requirements<a name="requirements"></a>
* NodeJS >= 7.4.0
* Netica-C API

## Setup<a name="setup"></a>

This software have been tested using Ubuntu 16.04 LTS.

This project contains two separate parts:

* A command-line tool named 'blumap'.
* A GUI (graphical user interface) based on NodeJS.

### Seting up blumap
Blumap is written in C and it includes a Makefile so the compilation and linking process is easy.

You will need to download several dependencies that are not installed by default in all systems such as:

* libgdal-dev
* python-gdal
* python-dev
* Netica-C API (https://norsys.com/netica_c_api.htm)

To build blumap you only need to do:
```
cd blumap
make
```
When the programs 'blumap', 'net2json' and 'vectornodes' are built, you are ready to run the front-end. It is recommended since it adds many functionalities to the application, such as being able to save runs, or notify via e-mail when specially long inference is run, but if you are not interested and just want to use the command-line tool, you can jump to the section [Running gBay from the command-line](#running-gbay-from-the-command-line)

### Setting up the frontend
* Run `'npm install'` to install all NodeJS dependencies.
* Run `'npm start'` to run the webserver.

There are many settings that can be configured changing the values in `'config.js'` such as the paths to several files, the port where the app listens for connections, the mail settings, etc.

If setting up gBAY in a public server, please consider to take some [precautions](#warning).

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

    `J = H/Hmax, where H = i=1Npilog2pi , Hmax = log2(N), pi is the probability of state i and N is the number of states.`

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