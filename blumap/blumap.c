#include <stdio.h>
#include <stdlib.h>  
#include <string.h> 
#include <libgen.h> 
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h> 
#include <signal.h>
#include <gdal.h>
#include "raster.h"
#include "vector.h"
#include "by_network.h"
#include "file_writer.h"
#include "embed_python.h"
#include "execution.h"
#include "dbf.h"
#include "statistics.h"
#include "blumap.h"

#define python_script_wrapper "python_c"
#define MAX_PYNODE 10

int numPlaces(int n);
int nodeIsNODATA(node_t* node, struct Raster* rasters, int rasterfile_n, int offset);
int checkPyNodes(net_info_t* net_info, pyNode_t** py_node_list, int len, int total_pixels);
int loadPyNodeData(node_t* node, pyNode_t* pyNode, int is_no_data);
int storeInitialLikelihood(struct Likelihood* likelihood, char* likelihood_str, regex_t* regex);
int vector_node_is_enabled(char* nodename, char** vector_nodes_disabled, int vector_nodes_disabled_n);

time_t  now=0, last=0, start=0;
int total_pixels, offset;
int it_count = 0;

void printInfo(){
	time_t diff = now - start;
	now = time(NULL);
	printf("Process finished in %ldm %lds -- %ld/s\n",diff/60, diff%60, (total_pixels*(it_count-1)+offset)/(diff?diff:1)); //prevent division by zero
}

void signalHandler(int dummy){
	printInfo();
	exit(-1);
}


void printHelp(char *command){

	printf("\nNAME:\n\t%s\n",command);
	printf("\nSYNOPSIS:\n\t%s [OPTIONS] -b BAYESIAN_NETWORK TARGET_NODE_NAMES [RASTER_FILE] ...\n",command);
	printf("\nDESCRIPTION:\n\tApply bayesian inference using GIS files (raster or vector) as inputs (run one per each cell/feature). It will produce a GIS file of the same type with target node beliefs, plus an additional file with the entropy in case the target node is type 'Discrete', or expected values, std deviations, mean, quantile and custom standard deviation in case it is 'Continuous'.\n");
	printf("\n\tIf a raster file is supplied, raster mode will be enabled. Ignoring all vector files.\n");
	printf("\n\tIn case of RASTER_MODE, %s will try to match every input file with a network node by name. In case a suitable node is found, it will use the raster cell value as a finding (evidence or likelihood) for that node. It will then use the bayesian network to calculate the output for every cell in the rasters. In case the of --dbf option is supplied, the raster values will be interpreted according to the raster attribute table (RAT) passed as an argument. This RAT should have only three columns: value, count and another column with the name of the state of the node to be applied for that value \n",command);

	printf("\n\tRasters with one band will be considered 'Node evidences' which values should be in the range [1..node_states] whereas rasters with more than one band will be considered 'Node likelihoods' with values in the range [0..100]. IMPORTANT: Rasters value 'NODATA' is considered UNKNOWN and will reset the node findings\n");
	printf("\n\tIn case of VECTOR_MODE, %s will try to match every column in the attribute table with a network node by name. In case a suitable node is found it will use the attribute value as an evidence for the node state. In case of soft evidence, the columns in the attribute table should be named like the name of the node, adding __s1, __s2, etc for every state (Example: lu_t0__s0, lu_t0__s1, ...). In addition, some nodes from the attribute table can be disabled using the '-z' option. It will then use the bayesian network to calculate the output for every feature in the vectors. IMPORTANT: Attribute value '0' (or every state in case of soft evidence being 0) will be considered UNKNOWN and will reset the node findings\n",command);

	printf("\n\tTARGET_NODE_NAMES is a list of comma-separated TARGET_NODEs\n");
	printf("\n\tIn case of RASTER_MODE. The output will be a raster with one band for each state belief, plus an additional band with the index of the state with the highest likelihood.\n");
	printf("\n\tIn case of VECTOR_MODE. The output will be a shapefile with one attribute (column of the attribute table) for each state belief, plus an additional atribute with the index of the state with the highest likelihood.\n");
	printf("\n\tIn case a TARGET_NODE is type 'Continuous' it will include two additional bands/attributes, for the expected value, and the standard deviation.\n");
	printf("\n\tIn RASTER_MODE the output raster file will be in 'GeoTIFF' format.\n");
	printf("\n\tIn VECTOR_MODE the output vector file will be in 'ESRI Shapefile' format.\n");
	printf("\n\tThe output file[s] will be named as the node name they correspond.\n");
	printf("\nOPTIONS:\n");
	printf("\t-h, --help\t\t\tPrint this help\n");
	printf("\t-d, --output_dir path\t Absolute path for output files.\n");
	printf("\t-o, --output_suffix suffix\t Use this suffix to be added to the output file.\n");
	printf("\t-e, --evidence evidence\t\t Give a explicit evidence to one of the nodes. This value will be used for each feature / raster cell. The evidence is in the form 'node_name=value'. Value will be considered as a state index for DISCRETE nodes, and a real value for CONTINUOUS nodes. In case a raster file already exists with the same node name, this evidence will be ignored. In case a vector file includes a column for a node with this name, the evidence will be applied.\n");
	printf("\t-k, --likelihood likelihood\t\t Give a explicit likelihood to one of the nodes. This likelihood will be used for each raster cell. The likelihood is in the form 'node_name=[state1,state2,..,staten]', with no spaces. In case a raster file already exists with the same node name, this likelihood will be ignored. . In case a vector file includes a column for a node with this name, the evidence will be applied.\n");
	//printf("\t-n, --nodata nodatavalue\tUse this value as NODATA for the result raster file (Default: '%d')\n",NO_DATA_VALUE);
	printf("\t-l, \t--link node_linking\t\t\t. Use result beliefs of one node as the CPT for a second node to be run the next iteration. It has only sense when used with -i option. The format of node_linking is 'node_src=node_dst'..\n");
	printf("\t-i, \t--iterations number_iterations\t Run the network 'number_iterations' times.\n");
	printf("\t-t, \t--target-type nodetype\t\t Force target node to be considered as nodetype regarding the type or result. Possible nodetypes are 'CONTINUOUS_TYPE' and 'DISCRETE_TYPE'\n");

	printf("\t-b, \t--bayes bayes_network[:network_period]. At least one bayes network must be provided. More than one bayesian network will be used together with '-r' option. In case a network_period is supplied, the network will be applied every 'network_period' iterations.\n");
	printf("\t-m, \t--py_module python_module. Use python module to customize node likelihoods every iteration.\n");
	printf("\t-n, \t--py_input input_node. The belief vector of this nodes will be passed to the python module as arguments.\n");
	printf("\t-s, \t--shapefile shapefile. Vector file. The column names of the attribute table should match node names in the network. This vector files will be IGNORED in case a raster file is passed as an argument.\n");
	printf("\t-v, --verbose\t Print debug information.\n");
	printf("\t-vv, --extra_verbose\t Print debug information plus single cells input and output.\n");
	printf("\t-x, --save-exec-info\t Save pre-cached executions info in file %s.\n", LOG_EXEC_FILENAME);
	printf("\t-f, --dbf dbf_filename\t To interpret raster values using the attribute table 'dbf_filename'.\n");
	printf("\t-y, --no-saving\t Not to save executions.\n");
	printf("\t-z, --disable-vectornode\t Disable the node specified by this argument. By default it uses every node stored in the vector attribute table/.\n");
	printf("\nRETURN VALUE:\n");
	printf("\t%d: if OK\n", OK);
	printf("\t%d: if ERROR \n", ERR);
	printf("\n");
}

int bytes2str(int bytes, char* res){
	if (bytes > 1024){
		if (bytes/1024 > 1024)
			return sprintf(res, "%d MB", bytes / 1024 / 1024);
		else
			return sprintf(res, "%d KB", bytes / 1024);	
	}
	else
		return sprintf(res, "%d B", bytes); 
}

void timeControl(time_t* start, time_t* now, time_t *last, int offset, int total_pixels){
	*now = time(NULL);
	time_t diff;

	if (*last == 0)
		*last = time(NULL);

	if ( (*now - *last) > DEF_TIMING ){
		*last = *now;
		diff = *now - *start;
		printf("- %d/%d %d%% %ld/s\n", offset, total_pixels, (offset*100)/total_pixels, offset/diff);
	} 
}
/*
	values: array of values to be scaled
	n: number values
	range_values: range of values of the first argument (i.e. 10 means form 0 to 10)
	maximun: maximun value to return in the scaled array (i.e. 200 means form 0 to 200)
	no_data: value inside values that means NODATA and will be directly represented as no_data_scaled
	no_data_scaled: vale to transform no_data value to

	returns: array of scaled values
*/
unsigned char* scale(unsigned char* values, int n, int range_values, int maximun, int no_data, int no_data_scaled ){

	int i;
	unsigned char* scaled;

	if (maximun < range_values)
		return NULL;
	scaled = (unsigned char*)malloc(sizeof(char)*n);

	// do not scale in case of NO_DATA, just print white

	for (i=0; i < n; i++){
		if (values[i] == no_data)
			scaled[i] = no_data_scaled;
		else
			scaled[i] = values[i]*maximun/range_values;
	}

	return scaled;
}


/*
	Split comma-separated 'target_nodename' in several strings.
	It does not modify target_nodename.
	target_nodenames must be freed afterwards.
*/
int getMultipleTargetNodenames(char *target_nodename, char*** target_nodenames){

	char* nodename;
	int i = 0;
	const char s[2] = ",";

	char *target_nodename_c = strndup(target_nodename, MAX_NODENAME_LEN * MAX_TARGETS);

	nodename = strtok(target_nodename_c, s);
   
	while( (nodename != NULL) && (i < MAX_TARGETS)) {

		*target_nodenames = (char**)realloc(*target_nodenames, sizeof(char*)*(i+1));

		//printf("%s (%d)\n", nodename, strlen(nodename) );

		(*target_nodenames)[i] = strndup(nodename, MAX_NODENAME_LEN);
		
		nodename = strtok(NULL, s);

		i++;
   }

   free(target_nodename_c);
   return i;
}


int main(int argc, char* argv[]){
   
	//struct sigaction act;
	int debug = FALSE;
	int extra_debug = FALSE;   // to print single cell information
	int debug_raster = FALSE;
	int log_executions = FALSE;
	Execution_t* executions[MAX_EXECUTIONS];
	Execution_t* ex=NULL, *ex_found=NULL;
	int executions_n = 0;
	unsigned int cached_counter = 0;   // to count how many executins were run with saved executions
	int res;
	struct stat statbuf;
	int rastercell_nodata;
	char* command_name = "blumap";
	int i=0, j=0, k=0, h=0;
	int err = OK;
	struct Raster rasters[MAX_RASTER_FILES];
	int rasterfile_n = 0;
	struct Evidence evidences[MAX_RASTER_FILES];
	int evidence_n = 0;
	int n_bands;
	int cont_nodata = CONT_NODATA;   //value for continuous node nodata value not to collide with any other value in the raster. Since expected value and std_dev are not calculated, it is not needed
	int cont_nodata_old;
	char str_aux[BUFF_LEN];
	char str_aux2[BUFF_LEN];
	char *ptr_aux;
	long lnum;
	char evidence_names[MAX_EV][MAX_NODENAME_LEN];
	float evidence_values[MAX_EV];
	int evidence_argc = 0;
	struct Likelihood likelihoods[MAX_EV];
	int likelihoods_n = 0;
	char *target_nodename;
	nodetype_bn target_node_type = 0;   // Netica.h: typedef enum {CONTINUOUS_TYPE=1, DISCRETE_TYPE, TEXT_TYPE} nodetype_bn;
	node_t *node, *target_node;
	const nodelist_t* nodelist;
	char *node_type_str = NULL;
	int x_size, y_size;
	GByte **belief_array = NULL;   // [state][pixel(raster) || feature(vector)]
	FILE* fd;
	int n_iterations = 1;
	char linking_src[MAX_LINK][MAX_NODENAME_LEN];
	char linking_dst[MAX_LINK][MAX_NODENAME_LEN];
	int linking_argc = 0;
	struct Linking linkings[MAX_RASTER_FILES];
	int linking_n = 0;
	/* If target node type is DISCRETE_TYPE, calculate beliefs */
	//const prob_bn* beliefs;
	float* beliefs = NULL;
	char **vector_nodenames = NULL; // node names read from the vector attribute table
	int vector_nodenames_n = 0;
	int target_nodenames_n =0;
	char **target_nodenames = NULL;  // multiple target nodes support
	nodetype_bn *target_nodes_types;   // Netica.h: typedef enum {CONTINUOUS_TYPE=1, DISCRETE_TYPE, TEXT_TYPE} nodetype_bn;
	node_t **target_nodes = NULL;
	GByte ***belief_array_m = NULL;   // [target_node][state][pixel(raster) || feature(vector)]
	GByte **max_prob_m = NULL;
	float** beliefs_m = NULL;
	double** expected_value_m = NULL;
	double** std_dev_m = NULL;
	double **cont_data;
	
	double *entropy_v = NULL;
	double *mean_v = NULL;
	double *quantile_v = NULL;
	double *custom_std_dev_v = NULL;
	GByte* beliefs_stats = NULL;
	const level_bn* levels = NULL;   // double*
	int n_states;

	// to store result beliefs of source node to be applied the next iteration
	float*** beliefs_linking = NULL;   //  [cell][linking_index]
	int n_networks=0;
	net_info_t nets_info[MAX_NETWORKS];
	net_info_t* net_info;
	net_t* net;
	int memcounter = 0;
	regex_t regex; 	// to parse column names in raster attribute tables for likelihoods (lu_t0__s1, lu_t0__s2, etc)
	regex_t regex_filename_node; // to match filenames with nodes (filename=nodename)
	regex_t regex_likelihood; 	// to parse
	pyNode_t* pyNodes[MAX_PYNODE];
	int pyNodes_src_n =0;
	pyNode_t* pyNodes_dst[MAX_PYNODE];
	int pyNodes_dst_n =0;
	char py_node_args[MAX_PYNODE][BUFF_LEN]; 
	int py_node_args_n = 0;
	char py_module_filename[BUFF_LEN];
	int py_module_filename_supplied = 0;
	int ret;
	char vector_filenames[MAX_VECTOR_FILES][BUFF_LEN];
	vector_t vectors[MAX_VECTOR_FILES];
	int vectorfile_n = 0;
	char vector_nodes_disabled[MAX_NODES][MAX_NODENAME_LEN];
	int vector_nodes_disabled_n = 0;
	char* vector_nodes_disabled_p[MAX_NODES];
	float *pixel_vector[256]; // to store cached executions
	OGRLayerH hLayer;

	static struct option long_options[] = { 
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{"vv", no_argument, NULL, 'w'},
		{"save-exec-info", no_argument, NULL, 'x'},
		{"output_dir", required_argument, NULL, 'd'},
		{"output_suffix", required_argument, NULL, 'o'},
		{"evidence", required_argument, NULL, 'e'},
		{"likelihood", required_argument, NULL, 'k'},
		//{"nodata",  required_argument, NULL, 'n'},
		{"link",  required_argument, NULL, 'l'},
		{"iterations",  required_argument, NULL, 'i'},
		{"target-type",  required_argument, NULL, 't'},
		{"bayes",  required_argument, NULL, 'b'},
		{"py_module",  required_argument, NULL, 'm'},
		{"py_input",  required_argument, NULL, 'n'},
		{"shapefile",  required_argument, NULL, 's'},
		{"dbf",  required_argument, NULL, 'f'},
		{"no-saving", no_argument, NULL, 'y'},
		{"disable-vectornode", required_argument, NULL, 'z'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	int c=0;
	char output_filename[MAX_FILENAME_LEN];
	char *output_filename_suffix = NULL;
	char *output_dir = NULL;
	short int dbf_n = 0;
	DBF dbfs[MAX_RASTER_FILES];
	int savingExecutions = TRUE;
	vectornode_t vectornodes[MAX_NODES];
	int vectornodes_n=0;

 
	signal(SIGINT, signalHandler);

	// JUST WHILE DEBUGGING TO NOT BUFFER STDOUT
	setbuf(stdout, NULL);

	ret = regcomp(&regex_likelihood, REGEX_NODENAME_LIKELIHOOD, REG_EXTENDED);
	if (ret) {
		fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODENAME_LIKELIHOOD);
		return ERR;
	}

	/* Detect the end of the options. */

	while (c!=-1){

		c = getopt_long (argc, argv, "hxyd:o:e:k:l:i:t:b:m:n:s:f:vwz:", long_options, &option_index);

		switch (c){

			case 'h': 
				printHelp(command_name);
				return OK;

			case 'v': 
				debug = TRUE;
				break;
			case 'w': 
				debug = TRUE;
				extra_debug = TRUE;
				break;

			case 'x': 
				log_executions = TRUE;
				unlink(LOG_EXEC_FILENAME);
				break;
			case 'y': 
				printf("Not saving executions\n");
				savingExecutions = FALSE;
				break;


			case 'd':

				output_dir = optarg;

				// check target directory exists already
				res = mkdir(output_dir, 0775);
				if (res != 0){
					if (errno == EEXIST){
						if (stat(output_dir, &statbuf) != 0){
							fprintf(stderr, "Error in stat: %s\n", output_dir);
							return ERR;
						}
						else{
							if (!S_ISDIR(statbuf.st_mode)){
								fprintf(stderr, "Output directory name already exists and it is not a directory: %s\n", output_dir);
								return ERR;
							}
						}
					}
					else{
						perror("mkdir");
						return ERR;
					}
				}
				break;
			
			case 'o':
				output_filename_suffix = optarg;
				break;
			
			case 'e':
				if (strlen(optarg) > MAX_NODENAME_LEN){
					fprintf(stderr,"ERROR: Evidence argument is too long '%s'\n",optarg);
					return ERR;
				}

				if (evidence_argc >= MAX_EV){
					fprintf(stderr,"ERROR: Too much explicit evidences\n");
					return ERR;
				} 
				if (sscanf(optarg, "%[^=]=%f", evidence_names[evidence_argc], &evidence_values[evidence_argc]) == 2 ){
						evidence_argc++;
				}
				else{
					fprintf(stderr,"ERROR: Wrong evidence '%s'\n",optarg);
					return ERR;
				}
				break;

			case 'k':
				if (strlen(optarg) > MAX_NODENAME_LEN){
					fprintf(stderr,"ERROR: Likelihood argument is too long '%s'\n",optarg);
					return ERR;
				}

				if (likelihoods_n >= MAX_EV){
					fprintf(stderr,"ERROR: Too much explicit likelihoods\n");
					return ERR;
				} 


				ret = storeInitialLikelihood(likelihoods+likelihoods_n, optarg, &regex_likelihood);
				if (ret != OK){
					fprintf(stderr,"ERROR: storeInitialLikelihood\n");
					return ERR;
				}

				likelihoods_n++;

				break;

			case 'l':
				if (strlen(optarg) > BUFF_LEN){
					fprintf(stderr,"ERROR: Linking argument too long '%s'\n",optarg);
					return ERR;
				}

				if (sscanf(optarg, "%[^=]=%s", linking_src[linking_argc], linking_dst[linking_argc]) == 2 )

					if (!strcmp(linking_src[linking_argc], linking_dst[linking_argc])){
						fprintf(stderr,"WARN: Linking will be ignored'%s'\n",optarg);
					}
					else
						linking_argc++;
				else{
					fprintf(stderr,"ERROR: Wrong linking '%s'\n",optarg);
					return ERR;
				}

				break;

			case 'i':
				n_iterations = atoi(optarg);
				break;

			case 't':

				if (!strcmp(optarg, "DISCRETE_TYPE")){
					target_node_type = DISCRETE_TYPE;
				}
				else if (!strcmp(optarg, "CONTINUOUS_TYPE")){
					target_node_type = CONTINUOUS_TYPE;
				}

				break;


			case 'b':

				i = sscanf(optarg, "%[^:]:%s",nets_info[n_networks].filename, str_aux);

				//if (sscanf(optarg, "%[^,],%s",bayes_networks_str[n_networks], str_aux) == 2 ){
				if (sscanf(optarg, "%[^:]:%s",nets_info[n_networks].filename, str_aux) == 2 ){

					lnum = strtol(str_aux, &ptr_aux, 10);

					if (ptr_aux == str_aux){     //if no characters were converted these pointers are equal
						fprintf(stderr, "ERROR: can't convert string to number\n");
						return ERR;
					}
					// If sizeof(int) == sizeof(long), we have to explicitly check for overflows
					if ((lnum == UINT_MAX || lnum == LONG_MIN) && errno == ERANGE){
						fprintf(stderr, "ERROR: number out of range for LONG\n");
						return ERR;
					}

					// Because strtol produces a long, check for overflow
					if ( (lnum > INT_MAX) || (lnum < INT_MIN) ){
						fprintf(stderr, "ERROR: number out of range for INT\n");
						return ERR;
					}

					nets_info[n_networks].time_period = (unsigned int)lnum;

				}
				else {
					strncpy(nets_info[n_networks].filename, optarg, MAX_BUFF);
					nets_info[n_networks].time_period = 1;
				}

				n_networks++;
				break;

			case 'm':

				strncpy(py_module_filename, optarg, BUFF_LEN);
				py_module_filename_supplied = 1 ;
				break;

			case 'n':
				strncpy(py_node_args[py_node_args_n], optarg, BUFF_LEN);
				py_node_args_n++;
				break;

			case 's':
				strncpy(vector_filenames[vectorfile_n], optarg, BUFF_LEN);
				vectorfile_n++;
				break;

			case 'f':
				printf("Using DBF to interpret raster values: %s\n", optarg);
				DBF_Open(optarg, dbfs + dbf_n);
				dbf_n++ ;
				break;

			case 'z':
				strncpy(vector_nodes_disabled[vector_nodes_disabled_n], optarg, MAX_NODENAME_LEN);
				vector_nodes_disabled_p[vector_nodes_disabled_n] = vector_nodes_disabled[vector_nodes_disabled_n];
				vector_nodes_disabled_n++;
				break;

			case '?':
				return ERR;
		}
	}

	if (!vectorfile_n && (argc - optind < 2)){
		printf("\nERROR: Missing argument\n");
		return OK;
	}

	target_nodename = argv[optind];
	target_nodenames_n = getMultipleTargetNodenames(target_nodename, &target_nodenames);
	target_nodes_types = (nodetype_bn *)malloc(sizeof(nodetype_bn) * target_nodenames_n );
	target_nodes = (node_t**)malloc(sizeof(node_t*) * target_nodenames_n );

	if (n_networks == 0){
		fprintf(stderr, "No bayesian networks provided\n");
		return OK;
	}
 
	ptr_aux = NULL;

	fd = fopen(NETICA_LICENSE_FILENAME, "r");
	if (!fd){
		fprintf(stderr,"Could not open file with Netica license '%s'. Running in limited mode.\n", NETICA_LICENSE_FILENAME);
		
	}
	else{
		if (fread(str_aux, BUFF_LEN, 1, fd))
			ptr_aux = str_aux;
	}
	fclose(fd);

	err = initBayesNetworkEnv(ptr_aux, 0);
	if (err < 0){
		fprintf(stderr, "Error initializing environment.\n");
		return err;
	}
	
	for (i =0; i<n_networks; i++){
		nets_info[i].net = readNet( nets_info[i].filename);
		if (nets_info[i].net == NULL) {
			return ERR;
		} 
	}
	

	j = 0; // flag to control message displayed only once 
	for (i=0; i < target_nodenames_n; i++){

		target_nodes[i] = getNodeInNet(nets_info[0].net, target_nodenames[i]);

		if (!target_nodes[i]){
			fprintf(stderr,"ERROR: Target node '%s' not found in the network '%s'\n",target_nodenames[i], GetNetName_bn(nets_info[0].net));
			return ERR;
		}

		target_nodes_types[i] = getNodeType(target_nodes[i]);

		switch (target_nodes_types[i]){

			case DISCRETE_TYPE:
				node_type_str = "DISCRETE_TYPE";
				n_bands = getNodeNumberStates(target_nodes[i]) + 1;
				break;
			case CONTINUOUS_TYPE:
				node_type_str = "CONTINUOUS_TYPE";
				//n_bands = 2;
				n_bands = getNodeNumberStates(target_nodes[i]) + 3;

				if (!j++) printf("Execution caching is disabled due to an input continuous node.\n");
				
				savingExecutions = FALSE;

				break;
			default:
				node_type_str = "UNKNOWN_TYPE";
				fprintf(stderr,"ERROR: Target node of unknown type\n");
				return ERR;
		}

		printf("Target node %d '%s' (%s) %s %d states.\n", i, target_nodenames[i], getNodeTitle(target_nodes[i]) ,node_type_str, getNodeNumberStates(target_nodes[i]));

		if (rasterfile_n){
			printf("Result raster file will have %d bands (%s).\n", n_bands, (target_nodes_types[i]==DISCRETE_TYPE)?"One for each target node state belief, plus one with the state of maximum probability":"First band: expected value. Second band: standard deviation.");
		}
		else if (vectorfile_n){
			printf("Result vector file will have %d columns in the attribute table (%s).\n", n_bands, (target_nodes_types[i]==DISCRETE_TYPE)?"One for each target node state belief, plus one with the state of maximum probability":"First column: expected value. Second column: standard deviation.");
		}

	}

	target_node = target_nodes[0];

	nodelist = getNetNodes(nets_info[0].net);
	
	ret = regcomp(&regex_filename_node, REGEX_FILENAME_NODE, REG_EXTENDED);
	if (ret) {
		fprintf(stderr, "Could not compile regex: %s\n", REGEX_FILENAME_NODE);
		return ERR;
	}

	for (i = 0;  i < vectorfile_n;  i++){
		ret = loadGISPathMatchingNode(vector_filenames[i], nets_info[0], vectors + i, &regex_filename_node, TYPE_VECTOR, debug);
		if (ret != OK){
			fprintf(stderr, "ERROR in loadGISPathMatchingNode. Exiting...\n");
			return ERR;
		}
	}

	// read raster files at the end of command line input
	for (i = optind + 1;  i < argc;  i++){

		ret = loadGISPathMatchingNode(argv[i], nets_info[0], (void*)(rasters + rasterfile_n), &regex_filename_node, TYPE_RASTER, debug && debug_raster);

		if (ret == OK){
			
			// apply transformation of raster values according to raster attribute table
			for (j=0; j < dbf_n; j++){

				if (!strcmp(rasters[rasterfile_n].nodename, dbfs[j].nodename)){
					printf("\nRaster nodename matches DBF nodename.\n");
					ret = applyDBFtoRasterData(dbfs[j], rasters[rasterfile_n], getNodeByName(dbfs[j].nodename, nets_info, n_networks), debug);	
					if (ret == DBF_ERR){
						fprintf(stderr, "applyDBFtoRasterData: error\n");
						return ERR;
					}
					break;
				}
			}
			rasterfile_n++;
		}

		else{

			fprintf(stderr, "ERROR in loadGISPathMatchingNode. Exiting...\n");
			return ERR;

		}
	}
   
	regfree(&regex_filename_node);
	regfree(&regex_likelihood);
	
	if (vectorfile_n ==0 && rasterfile_n == 0){
		fprintf(stderr,"ERROR: No vector or raster files supplied.\n");
		return ERR;
	}

	if (rasterfile_n){   // RASTER MODE

		if (!consistentRasters(rasters, rasterfile_n))
			return ERR;

		x_size = GDALGetRasterBandXSize(getRasterBand(rasters[0].hDataset, 1, FALSE));
		y_size = GDALGetRasterBandYSize(getRasterBand(rasters[0].hDataset, 1, FALSE));

		total_pixels = x_size * y_size;

	}

	else{   // VECTOR MODE

		if (checkSameVectorMetaInfo(vectors, vectorfile_n) != 0 ) {
			return ERR;
		}

		hLayer = GDALDatasetGetLayer(vectors[0].hDataset, 0);
		OGR_L_ResetReading(hLayer);

		total_pixels = OGR_L_GetFeatureCount(hLayer, 0);   // number of features (points, polygons, etc)

		ret = regcomp(&regex, REGEX_VECTOR_COLUMNS, REG_EXTENDED);
		if (ret) {
			fprintf(stderr, "Could not compile regex: %s\n", REGEX_VECTOR_COLUMNS);
			exit(1);
		}

		if (debug)
			printf("Vector mode: Using regular expression to match node findings/likelihoods in the attribute table: %s\n", REGEX_VECTOR_COLUMNS);


	}


	// check evidences    
	for (i = 0; i< evidence_argc ; i++){

		ret = loadEvidenceMatchingNode(evidence_names[i], evidence_values[i], nets_info[0], evidences + evidence_n, debug);

		if (ret == OK){ 

			// check if evidence name collides with raster 
			for (k = 0; k < rasterfile_n; k++){
				if (!strcmp(str_aux2, rasters[k].nodename)){
					printf("WARNING - This evidence collides with a raster file and will be ignored. - WARNING\n");
					break;
				}
			}

			if (k ==rasterfile_n )
				evidence_n++;
		}
	}

	// In case of vector mode, the manual evidences and likelihoods will be applied instead of the values found in the vector file
	for (i = 0; i< evidence_n ; i++){
		strncpy(vector_nodes_disabled[vector_nodes_disabled_n], evidences[i].nodename, MAX_NODENAME_LEN);
		vector_nodes_disabled_p[vector_nodes_disabled_n] = vector_nodes_disabled[vector_nodes_disabled_n];
		vector_nodes_disabled_n++;
	}
	for (i = 0; i< likelihoods_n ; i++){
		strncpy(vector_nodes_disabled[vector_nodes_disabled_n], likelihoods[i].nodename, MAX_NODENAME_LEN);
		vector_nodes_disabled_p[vector_nodes_disabled_n] = vector_nodes_disabled[vector_nodes_disabled_n];
		vector_nodes_disabled_n++;
	}

	// check linkings
	for (i = 0; i< linking_argc ; i++){

		ret = loadLinkingMatchingNode(linking_src[i], linking_dst[i], nets_info, n_networks, linkings + linking_n, debug);

		if (ret > 0){ 
			printf("Linking '%s=%s' successful.\n", linking_src[i], linking_dst[i]);
			linking_n++;
		}
	}

	for (i =0; i < n_networks; i++){

		ret = CompileNet(nets_info[i].net);
		
		if (ret != NET_OK) {
			return ERR;
		} 
	
		SetNetAutoUpdate_bn(nets_info[i].net, 0);
	}


	if (py_module_filename_supplied){

		ret = initPythonEnv(py_module_filename);
		if (ret){
			if (debug)
				printf("\nPython initialized\n");
		}
		else{
			fprintf(stderr, "\nERROR: Python environment was not initialized.\n");
			return -1;
		}

		printf("Third party module supplied: %s\n", py_module_filename);

		for (i=0; i < py_node_args_n; i++){

			node = getNodeByName(py_node_args[i], nets_info, n_networks);

			if (node){

				if (getNodeType(node)==DISCRETE_TYPE){
					pyNodes[pyNodes_src_n] = createPyNode(py_node_args[i], getNodeNumberStates(node),  NULL, total_pixels * getNodeNumberStates(node), PY_DISCRETE);
				}
				else if (getNodeType(node)==CONTINUOUS_TYPE){
					pyNodes[pyNodes_src_n] = createPyNode(py_node_args[i], getNodeNumberStates(node), NULL,total_pixels , PY_CONTINUOUS);
				}
			
				if (pyNodes[pyNodes_src_n])
					pyNodes_src_n++;
			}
		}

		for (i=0; i< pyNodes_src_n; i++)
			printf(" - input node: '%s'\n", pyNodes[i]->nodename);
		if (!pyNodes_src_n)
			printf("WARN: no input nodes were supplied for third party module!");
	}


	if (!rasterfile_n && vector_nodes_disabled_n){
		printf("Vector nodes disabled:\n");
		for (i =0; i < vector_nodes_disabled_n; i++){
			printf("- %s\n", vector_nodes_disabled[i]);
		}	
	}

	beliefs_linking = (float***)malloc(sizeof(float**)*total_pixels);
	if (!beliefs_linking){
		fprintf(stderr,"ERROR: beliefs_linking: malloc.\n");
		return ERR;
	}
	for (i = 0; i< total_pixels; i++){
		beliefs_linking[i] = (float**)malloc(sizeof(float*)*linking_n);
		if (!beliefs_linking[i]){
			fprintf(stderr,"ERROR: beliefs_linking: malloc.\n");
			return ERR;
		}
	}
	for (i = 0; i< total_pixels; i++){
		for (j = 0; j< linking_n; j++){
			beliefs_linking[i][j] = (float*)malloc(sizeof(float)*getNodeNumberStates(linkings[j].src));
			if (!beliefs_linking[i][j]){
				fprintf(stderr,"ERROR: beliefs_linking: malloc.\n");
				return ERR;
			}
		}
	}

	belief_array_m = (GByte***)malloc(sizeof(GByte**) * target_nodenames_n);
	if (!belief_array_m){
		fprintf(stderr,"ERROR: belief_array_m: Error allocating memory\n");
		return ERR;
	}
	max_prob_m = (GByte**)malloc(sizeof(GByte*) * target_nodenames_n);
	if (!max_prob_m){
		fprintf(stderr,"ERROR: max_prob_m: Error allocating memory\n");
		return ERR;
	}
	expected_value_m = (double**)malloc(sizeof(double*) * target_nodenames_n);
	if (!expected_value_m){
		fprintf(stderr,"ERROR: expected_value_m: Error allocating memory\n");
		return ERR;
	}
	std_dev_m = (double**)malloc(sizeof(double*) * target_nodenames_n);
	if (!std_dev_m){
		fprintf(stderr,"ERROR: std_dev_m: Error allocating memory\n");
		return ERR;
	}
	beliefs_m = (float**)malloc(sizeof(float*) * target_nodenames_n);
	if (!beliefs_m){
		fprintf(stderr,"ERROR: beliefs: Error allocating memory\n");
		return ERR;
	}

	for (i=0; i < target_nodenames_n; i++){
		
		target_node = getNodeInNet(nets_info[0].net, target_nodenames[i]); 
		
		belief_array_m[i] = (GByte**)malloc(sizeof(GByte*) * (getNodeNumberStates(target_node) + 1));
		if (!belief_array_m){
			fprintf(stderr,"ERROR: belief_array_m: Error allocating memory\n");
			return ERR;
		}

		for (j =0; j < (getNodeNumberStates(target_node) + 1); j++)
			belief_array_m[i][j] = (GByte*)malloc(sizeof(GByte)*total_pixels);

		max_prob_m[i] = (GByte*)malloc(sizeof(GByte)*total_pixels);


		if (target_nodes_types[i] == CONTINUOUS_TYPE) {
			expected_value_m[i] = (double*)malloc(sizeof(double)*total_pixels);
			std_dev_m[i] = (double*)malloc(sizeof(double)*total_pixels);

			if (!expected_value_m[i] || !std_dev_m[i]){
				fprintf(stderr,"ERROR: expected_value_m: Error allocating memory\n");
				return ERR;
			}
		}
		else{
			expected_value_m[i] = NULL;
			std_dev_m[i] = NULL;
		}

		beliefs_m[i] = (float*)malloc(sizeof(float) * (getNodeNumberStates(target_node) + 1));
		if (!beliefs_m){
			fprintf(stderr,"ERROR: beliefs_m: Error allocating memory\n");
			return ERR;
		}

		memcounter += ((sizeof(float) * (getNodeNumberStates(target_node) + 1)));
	}


	if (debug){
		bytes2str(sizeof(float) * linking_n * total_pixels , (char*)&str_aux);
		printf("\nReserved %s for beliefs_linking\n", str_aux);

		bytes2str(2* sizeof(double)*total_pixels, (char*)&str_aux);
		printf("Reserved %s for expected_value and std_dev\n", str_aux);

		bytes2str(memcounter, (char*)&str_aux);
		printf("Reserved %s for beliefs\n", str_aux);
	}

	// Statistics
	entropy_v= (double*)malloc(sizeof(double)*total_pixels);
	if (!entropy_v){
		fprintf(stderr,"ERROR: entropy_v: Error allocating memory\n");
		return ERR;
	}
	
	if (rasterfile_n)
		printf("Total cells: %d (%d x %d)\n", total_pixels, x_size ,y_size);
	else if (vectorfile_n)
		printf("Total features: %d.\n", total_pixels);
	
	// Set expicit evidence findings

	err = setInitialEvidenceAllNetworks(nets_info, n_networks, evidences, evidence_n);	
	if (err){

		if (belief_array_m) free(belief_array_m);
		if (beliefs_m) free(beliefs_m);

		for (i = 0;  i < rasterfile_n;  ++i){
			closeRaster(rasters[i]);
		}

		for (i = 0;  i < target_nodenames_n;  ++i){
			if(target_nodenames[i]) free(target_nodenames[i]);
		}
		if(target_nodenames) free(target_nodenames);

		return ERR;
	}

	err = setInitialLikelihoods(nets_info, likelihoods, likelihoods_n);	
	if (err){

		if (belief_array_m) free(belief_array_m);
		if (beliefs_m) free(beliefs_m);

		for (i = 0;  i < rasterfile_n;  ++i){
			closeRaster(rasters[i]);
		}

		for (i = 0;  i < target_nodenames_n;  ++i){
			if(target_nodenames[i]) free(target_nodenames[i]);
		}
		if(target_nodenames) free(target_nodenames);

		return ERR;
	}

	if (debug){
		for (i = 0; i < rasterfile_n; i++)
			printf("Raster: '%s'. NODATA: %f\n", rasters[i].nodename, rasters[i].nodatavalue);
	}

	savingExecutions = (savingExecutions && (!py_module_filename_supplied));

	if (savingExecutions)
		ex = (Execution_t*)malloc(sizeof(Execution_t));
	else if (py_module_filename_supplied){
		printf("Python third party was supplied: No caching executions\n");
	}

	net_info = nets_info;
	net = net_info->net;

	if (savingExecutions){

		if (rasterfile_n){

			initExecution(ex, rasterfile_n  + linking_n, target_nodenames_n + linking_n);

			for (i=0; i < rasterfile_n; i++){
				node = getNodeInNet(net, rasters[i].nodename);
				E_initInput(ex, getNodeName(node), getNodeNumberStates(node));
			}
		}
		else if (vectorfile_n){
			
			vector_nodenames_n = getNodeNamesFromVector(vectors[0], &vector_nodenames, debug);

			initExecution(ex, vector_nodenames_n  + linking_n, target_nodenames_n + linking_n);

			for (i=0; i < vector_nodenames_n; i++){
				node = getNodeInNet(net, vector_nodenames[i]);
				
				if (node)
					E_initInput(ex, getNodeName(node), getNodeNumberStates(node));
				else{
					printf("WARNING - node %s not found in the network. - WARNING\n", vector_nodenames[i]);
				}
			}
		}

		for (i=0; i < target_nodenames_n; i++){
			E_initOutput(ex, target_nodenames[i], getNodeNumberStates(target_nodes[i]));
		}
		for (i=0; i < linking_n; i++){
			E_initInput(ex, getNodeName(linkings[i].dst), getNodeNumberStates(linkings[i].dst));
			E_initOutput(ex, getNodeName(linkings[i].src), getNodeNumberStates(linkings[i].src));
		}
	}
	
	it_count = 0;

	if (!rasterfile_n){
		vectornodes_n = getVectornodesFromVector(net, vectors, vectornodes, &regex, debug); 
		if (readVectornodesData(net, vectors, vectornodes, vectornodes_n, debug) != OK){
			return ERR;
		}
	}

	printf("\nProcess starting.\n");

	// Python could run before the first iteration, passing -1 as n_iteration
	// It is needed to run for all the cells once

	start = time(NULL);

	if (py_module_filename_supplied){

		for (offset = 0; offset < total_pixels; offset++){

			timeControl(&start, &now, &last, offset, total_pixels);

			if (rasterfile_n){

				rastercell_nodata = setFindingsFromRasters(net, rasters, rasterfile_n, offset, 0);

				if (rastercell_nodata == ERR){

					closeAllNets(nets_info, n_networks);

					for (i = 0;  i < rasterfile_n;  ++i){
						closeRaster(rasters[i]);
					}

					for (i = 0;  i < target_nodenames_n;  ++i){
						if(target_nodenames[i]) free(target_nodenames[i]);
					}
					if(target_nodenames) free(target_nodenames);
					
					return ERR;
				}
			}

			else if (vectorfile_n){

				ret = setFindingsFromVectors(net, vectors, vectorfile_n, offset, ex, 0, vector_nodes_disabled_p, vector_nodes_disabled_n, vectornodes, vectornodes_n , debug);

				if (ret == ERR){

					closeAllNets(nets_info, n_networks);

					for (i = 0;  i < rasterfile_n;  ++i){
						closeRaster(rasters[i]);
					}
					
					return ERR;
				}
			}

			for (i = 0; i< pyNodes_src_n; i++){
				loadPyNodeData(getNodeInNet(net, pyNodes[i]->nodename), pyNodes[i], nodeIsNODATA(getNodeInNet(net, pyNodes[i]->nodename), rasters, rasterfile_n, offset));
			}
		}

		pyNodes_dst_n = third_party_python_script(python_script_wrapper, py_module_filename, rasters[0].filename, pyNodes, pyNodes_src_n, pyNodes_dst, -1, debug);

		ret = checkPyNodes(net_info, pyNodes_dst, pyNodes_dst_n, total_pixels);
		if (ret !=0){
			return ERR;
		}

		for (i = 0; i< pyNodes_src_n; i++){
			resetPyNode(pyNodes[i]);
		}
	}

	if(py_module_filename_supplied){
		if (pyNodes_dst_n == -1)
			printf("Error executing python script\n");
		else
			printf("\n%d node%s modified before starting first iteration.\n\n", pyNodes_dst_n, (pyNodes_dst_n!=1)?"s were":" was");
	}

	while (it_count < n_iterations){

		for (offset = 0; offset < total_pixels; offset++){
		
			timeControl(&start, &now, &last, offset, total_pixels);

			if (extra_debug)
				printf("%s: %d/%d, IT: %d/%d : ", rasterfile_n?"CELL":"FEATURE", offset+1 , total_pixels, it_count+1, n_iterations);

			if (savingExecutions){

				resetExecution(ex);

				for (i=0; i< rasterfile_n; i++){

					getPixelVector(rasters[i], offset, pixel_vector);	
					E_addNodeFinding_p(ex, rasters[i].nodename, pixel_vector, GDALGetRasterCount(rasters[i].hDataset));

				}

				// store beliefs in the execution only, do nothing with the network nodes				
				if (vectorfile_n)
					setFindingsFromVectors(net, vectors, vectorfile_n, offset, ex, 1, vector_nodes_disabled_p, vector_nodes_disabled_n, vectornodes, vectornodes_n, extra_debug);

				if (it_count > 0){
		
					for (i = 0; i< linking_n; i++){

						if (beliefs_linking[offset][i][0] != -1) // not all NODATA
							E_addNodeFinding(ex, getNodeName(linkings[i].dst), beliefs_linking[offset][i], getNodeNumberStates(linkings[i].dst));
					}
				}
			
				ex_found = findExecution(executions, executions_n, *ex);

			}

			if (!ex_found){

				if (rasterfile_n){

					rastercell_nodata = setFindingsFromRasters(net, rasters, rasterfile_n, offset, extra_debug);

					if (rastercell_nodata == ERR){

						closeAllNets(nets_info, n_networks);

						for (i = 0;  i < rasterfile_n;  ++i){
							closeRaster(rasters[i]);
						}

						for (i = 0;  i < target_nodenames_n;  ++i){
							if(target_nodenames[i]) free(target_nodenames[i]);
						}
						if(target_nodenames) free(target_nodenames);
						
						return ERR;
					}
				}

				else if (vectorfile_n){

					ret = setFindingsFromVectors(net, vectors, vectorfile_n, offset, ex, 0, vector_nodes_disabled_p, vector_nodes_disabled_n, vectornodes, vectornodes_n, extra_debug);

					if (ret == ERR){

						closeAllNets(nets_info, n_networks);

						for (i = 0;  i < rasterfile_n;  ++i){
							closeRaster(rasters[i]);
						}
						
						return ERR;
					}

				}

				if (it_count > 0){
					
					for (i = 0; i< linking_n; i++){

						if (beliefs_linking[offset][i][0] != -1){

							ret = enterNodeLikelihood(linkings[i].dst, beliefs_linking[offset][i]);
							if (ret != NET_OK){

								sprintf(str_aux, "Error: Setting node from linkings: node_src: %s, offset: %d, likelihood: ", getNodeName(linkings[i].src), offset );
								//fprintf(stderr,"Error: setFindingsFromRasters: node: %s, offset: %d\n", getNodeName(node), offset );
								printLikelihood_adv(beliefs_linking[offset][i], getNodeNumberStates(node), stderr, str_aux);

								exit(-1);
							}

							if (extra_debug){

								printf("\nCELL: %d/%d, IT: %d/%d : Setting node '%s' likelihood from node %s beliefs: [",offset+1 , total_pixels, it_count+1, n_iterations, getNodeName(linkings[i].dst), getNodeName(linkings[i].src));

								for (k = 0; k < getNodeNumberStates(linkings[i].dst); k++)
									printf("%f ",beliefs_linking[offset][i][k]);

								printf("]");
							}
						}

						else{

							if (extra_debug){

								printf("\nCELL: %d/%d, IT: %d/%d : Setting node '%s' likelihood from node %s beliefs: [",offset+1 , total_pixels, it_count+1, n_iterations, getNodeName(linkings[i].dst), getNodeName(linkings[i].src));

								for (k = 0; k < getNodeNumberStates(linkings[i].dst); k++)
									printf("NODATA ");

								printf("]");
							}
						}		
					}
				}

				if (py_module_filename_supplied){

					// Set likelihood from pyNodes (third-party scripts) of last iteration

					for (i = 0; i< pyNodes_dst_n; i++){

						if (extra_debug){

							node = getNodeInNet(net, pyNodes_dst[i]->nodename);
							
							if (node){

								printf("\n* CELL: %d/%d, IT: %d/%d : Setting node '%s' %s from third-party script: [",offset+1 , total_pixels, it_count+1, n_iterations, pyNodes_dst[i]->nodename, (getNodeType(node) == DISCRETE_TYPE)?"likelihood":"value");


								if (getNodeType(node) == DISCRETE_TYPE){
									for (j =0; j < getNodeNumberStates(node); j++){
										printf("%d ", (int)getStateValueFromPyNode(pyNodes_dst[i], offset, j));
									}
								}
								else if (getNodeType(node) == CONTINUOUS_TYPE){
									printf("%.2f", getValueFromPyNode(pyNodes_dst[i], offset));
								}


								printf("]");
							}
						}

						enterLikeliHoodsFromPYNode(net, offset, pyNodes_dst, pyNodes_dst_n);
					}
				}

				if ((rasterfile_n != 0) && (rastercell_nodata == rasterfile_n)){ // all rasters are NODATA

					if (extra_debug)
						printf(" ->");

					for (k=0; k < target_nodenames_n; k++){ 

						target_node = target_nodes[k];  			

						if (!target_node){
							fprintf(stderr,"ERROR: Target node '%s' not found in the network '%s'\n",target_nodenames[i], GetNetName_bn(net));

							
							return ERR;
						}
						else{

							for (i = 0; i< getNodeNumberStates(target_node); i++)
								belief_array_m[k][i][offset] = NO_DATA_VALUE;
							
							max_prob_m[k][offset] = NO_DATA_VALUE;
							
							if (extra_debug){
								printf(" (%s: [ ", target_nodenames[k]);
								for (i = 0; i< getNodeNumberStates(target_node); i++)
									printf("NODATA ");
								printf("]) ");

								
							}
							if (savingExecutions){
								E_setTargetBeliefs(ex, target_nodenames[k], NULL, getNodeNumberStates(target_node));
							}
						}
					}
				}

				else{  // value is not NODATA

					if (extra_debug)
						printf(" ->"); 
					
					for (i=0; i < target_nodenames_n; i++){ 

						target_node = target_nodes[i];  			// in case of network changes 
						if (!target_node){
							fprintf(stderr,"ERROR: Target node '%s' not found in the network '%s'\n",target_nodenames[i], GetNetName_bn(net));
							return ERR;
						}

						switch (getNodeType(target_node)){
							case DISCRETE_TYPE:
								node_type_str = "DISCRETE_TYPE";
								n_bands = getNodeNumberStates(target_node) + 1;
								break;
							case CONTINUOUS_TYPE:
								node_type_str = "CONTINUOUS_TYPE";
								//n_bands = 2;
								n_bands = getNodeNumberStates(target_node) + 3;
								break;
							default:
								node_type_str = "UNKNOWN_TYPE";
								fprintf(stderr,"ERROR: Target node of unknown type\n");
								
								return ERR;
						}
	
						beliefs = getNodeBeliefs(target_nodes[i]);
						if (beliefs == NULL){
							fprintf(stderr, "Error: Getting beliefs of target node '%s'\n", getNodeName(target_nodes[i]));
							return ERR;
						}

						memcpy(beliefs_m[i], beliefs, sizeof(float) * getNodeNumberStates(target_nodes[i]));

						max_prob_m[i][offset] = 0;

						j=0;
						for (k = 0; k< getNodeNumberStates(target_nodes[i]); k++){
						
							belief_array_m[i][k][offset] = beliefs_m[i][k] * 100;		

							if (belief_array_m[i][k][offset] > max_prob_m[i][offset]){
								max_prob_m[i][offset] = belief_array_m[i][k][offset];
								j = k;
							}

						}
						
						max_prob_m[i][offset] = j+1;

						if (savingExecutions){

							E_setTargetBeliefs(ex, target_nodenames[i], beliefs_m[i], getNodeNumberStates(target_node));
						}

						if (extra_debug){
							printNode(target_nodes[i], PRINT_BASIC);
						}
					}
				} 

				if (savingExecutions){
					if (log_executions){
						logExecution(ex, offset, it_count);
					}
				}

				for (i = 0; i< pyNodes_src_n; i++){
					loadPyNodeData(getNodeInNet(net, pyNodes[i]->nodename), pyNodes[i], nodeIsNODATA(getNodeInNet(net, pyNodes[i]->nodename), rasters, rasterfile_n, offset));
				}

				for (i =0; i < linking_n; i++){

					beliefs = getNodeBeliefs(linkings[i].src);
					if (beliefs == NULL){
						fprintf(stderr, "Error: Storing beliefs of source linking node '%s'\n", getNodeName(linkings[i].src));
						return ERR;
					}

					if (rastercell_nodata == rasterfile_n){
						beliefs_linking[offset][i][0] = -1;
					}
					else{
						memcpy(beliefs_linking[offset][i], beliefs, sizeof(float) * getNodeNumberStates(linkings[i].src));
					}

					if (savingExecutions){
						ret = E_setTargetBeliefs(ex, getNodeName(linkings[i].src), beliefs, getNodeNumberStates(linkings[i].src));

						if (ret == -1){
							fprintf(stderr, "E_addOutputLinking: error\n");
						}
					}
				}

				if (savingExecutions){
					
					if ( executions_n  < MAX_EXECUTIONS){

						executions[executions_n] = copyExecution(ex);

						if (extra_debug){
							if (rasterfile_n)
								printf("\nCELL: ");
							else
								printf("\nFEATURE: ");
								
							printf("%d/%d, IT: %d/%d : ****** New execution saved ******\n",offset+1 , total_pixels, it_count+1, n_iterations);
							printExecutionVerbose(executions[executions_n], offset+1 , total_pixels, it_count+1, n_iterations);

							if (rasterfile_n)
								printf("\nCELL: ");
							else
								printf("\nFEATURE: ");
							
							printf("%d/%d, IT: %d/%d : *********************************\n",offset+1 , total_pixels, it_count+1, n_iterations);
						}
						executions_n++;
					}
				}
			} //if (!ex_found)


			else{ // ex_found

				cached_counter++;

				if (extra_debug){
					printExecution(ex_found);		
				}

				for (k=0; k < target_nodenames_n; k++){ 

					target_node = target_nodes[k];

					for (h=0; h < ex_found->output_nodes_len; h++){

						if (!strcmp(target_nodenames[k], ex_found->target_nodenames[h])){


							if (ex_found->beliefs_m[h][0] == -1){ 

								for (i = 0; i< getNodeNumberStates(target_node); i++)
									belief_array_m[k][i][offset] = NO_DATA_VALUE;
								
								max_prob_m[k][offset] = NO_DATA_VALUE;

							}
							else{

								max_prob_m[k][offset] = 0;
								j=0;
								for (i = 0; i< getNodeNumberStates(target_node); i++){
								
									belief_array_m[k][i][offset] = ex_found->beliefs_m[h][i] * 100;		

									if (belief_array_m[k][i][offset] > max_prob_m[k][offset]){
										max_prob_m[k][offset] = belief_array_m[k][i][offset];
										j = i;
									}	

								}
								max_prob_m[k][offset] = j+1;
							}
						}
					}
				}	

				for (i =0; i < linking_n; i++){
					for (j=0; j < ex_found->output_nodes_len; j++){
						if (!strcmp(linking_src[i], ex_found->target_nodenames[j])){
							if (ex_found->beliefs_m[j][0] == -1){
								beliefs_linking[offset][i][0] = -1;
							}
							else{

								beliefs = ex_found->beliefs_m[j];

								memcpy(beliefs_linking[offset][i], beliefs, sizeof(float) * getNodeNumberStates(linkings[i].src));
			
							}
						}
					}
				}
			} // execution was found
			
			if (extra_debug)
					printf("\n");
		} // for (offset = 0; offset < total_pixels; offset++)

		if (py_module_filename_supplied){ 

			pyNodes_dst_n = third_party_python_script(python_script_wrapper, py_module_filename, rasters[0].filename, pyNodes, pyNodes_src_n, pyNodes_dst, it_count, debug);

			ret = checkPyNodes(net_info, pyNodes_dst, pyNodes_dst_n, total_pixels);
			if (ret !=0){
				return ERR;
			}
			for (i = 0; i< pyNodes_src_n; i++){
				resetPyNode(pyNodes[i]);
			}
			for (i = 0; i< pyNodes_dst_n; i++){

				for (h=0; h < target_nodenames_n; h++){ 

					target_node = target_nodes[h];

					if (!strcmp(pyNodes_dst[i]->nodename, target_nodenames[h])){

						printf("WARN - Third party script is modifying a target node: %s. Cell 0: [", pyNodes_dst[i]->nodename);

						for (offset = 0; offset < total_pixels; offset++){

							max_prob_m[h][offset] = 0;
							j=0;

							if (pyNodes_dst[i]-> type == PY_CONTINUOUS){

								if (getValueFromPyNode(pyNodes_dst[i], offset) == PY_NODATA_VALUE){
									for (k = 0; k< getNodeNumberStates(target_node); k++)
										beliefs[k] = PY_NODATA_VALUE;
								}
								else{

									enterLikeliHoodsFromPYNode(net, offset, pyNodes_dst + i, 1);

									beliefs = getNodeBeliefs(target_node);

									if (offset==0){
										printf("DEBUG: Continuous node: entering %f\n", getValueFromPyNode(pyNodes_dst[i], offset));		
										printLikelihood(beliefs, getNodeNumberStates(target_node));
										printf("DEBUG: Continuous node: -------\n");
									}
								}
							}

							for (k = 0; k< getNodeNumberStates(target_node); k++){

								if (pyNodes_dst[i]-> type == PY_DISCRETE)
									belief_array_m[h][k][offset] = (int)getStateValueFromPyNode(pyNodes_dst[i], offset, k); 	

								else if (pyNodes_dst[i]-> type == PY_CONTINUOUS){
									belief_array_m[h][k][offset] = beliefs[k];
								}

								else{
									fprintf(stderr,"ERROR: pyNode with unknown type\n");
									return ERR;
								}

								if (belief_array_m[h][k][offset] == PY_NODATA_VALUE){
									belief_array_m[h][k][offset] = NO_DATA_VALUE;
									max_prob_m[h][offset] = NO_DATA_VALUE;
								}

								if (belief_array_m[h][k][offset] > max_prob_m[h][offset]){
									max_prob_m[h][offset] = belief_array_m[h][k][offset];
									j = k;
								}
							}

							if (max_prob_m[h][offset] != NO_DATA_VALUE)
								max_prob_m[h][offset] = j+1;


							if (offset==0){
								for (k = 0; k< getNodeNumberStates(target_node); k++){
									//printf("%d ", belief_array_m[h][k][offset]);
								
									if ( belief_array_m[h][k][offset] == NO_DATA_VALUE)
										printf("NODATA ");
									else
										printf("%d ", belief_array_m[h][k][offset]);

								}
							}

						}
						printf("]\n");
					}
				}

				//check if the pyNode is also a linking node source, in which case the target should be updated, not that node itself
				for (j = 0; j< linking_n; j++){
					if (!strcmp(pyNodes_dst[i]->nodename, getNodeName(linkings[j].src))) {

						free(pyNodes_dst[i]->nodename);
						pyNodes_dst[i]->nodename = strdup(getNodeName(linkings[j].dst));
						printf("DEBUG: a pyNode was modified due to a linking (%s --> %s)\n", getNodeName(linkings[j].src), getNodeName(linkings[j].dst));
						printPyNode(pyNodes_dst[i], 0);
						
						break;	
					}
				}
			}
		} 

		if (debug)
			printf("\n--- Finished iteration %d/%d ---\n", it_count+1, n_iterations);

		// END THIRD-PARTY 

		it_count++;

		if (n_iterations !=1 && linking_n == 0 && savingExecutions){ // does not make any sense to iterate again with the same input data 
			printf("WARN: No linkings and no python module supplied: Skipping iterations\n");
			break;
		}
	} // while (it_count < n_iterations)

	printInfo();
	
	for (i=0; i < pyNodes_src_n; i++)
		freePyNode(pyNodes[i]);

	for (i=0; i < pyNodes_dst_n; i++)
		freePyNode(pyNodes_dst[i]);

	
	for (i=0; i < target_nodenames_n; i++){

		memset(output_filename, '\0', MAX_FILENAME_LEN);

		if (output_dir){ // output dir is a directory (checked while getopt)
			snprintf(output_filename, MAX_FILENAME_LEN, "%s/", output_dir);
		}

		snprintf(output_filename+strlen(output_filename), MAX_FILENAME_LEN - (output_dir?strlen(output_dir):0), "%s", target_nodenames[i]);

		if (output_filename_suffix)
			snprintf(output_filename+strlen(output_filename), MAX_FILENAME_LEN - strlen(output_filename) -1 - (output_dir?strlen(output_dir):0), "_%s", output_filename_suffix);
		
		if (rasterfile_n)
			snprintf(output_filename + strlen(output_filename), MAX_FILENAME_LEN - strlen(output_filename) - 1 - (output_dir?strlen(output_dir):0) - (output_filename_suffix?strlen(output_filename_suffix):0), OUTPUT_FILENAME_SUFFIX);	

		n_bands = getNodeNumberStates(target_nodes[i]) + 1;

		for (j =0; j< total_pixels; j++){
			belief_array_m[i][getNodeNumberStates(target_nodes[i])][j] = max_prob_m[i][j];
		}

		if (rasterfile_n){
			err = writeGeoTIFF(output_filename, rasters[0].hDataset, (void*)belief_array_m[i], n_bands, NO_DATA_VALUE, GDT_Byte );
			if (err == ERR){
				fprintf(stderr, "Error in writeGeoTIFF.\n" );
			}
		}
	
		else if (vectorfile_n){

			err = writeSHP(output_filename, vectors[0].hDataset, (void*)belief_array_m[i], n_bands);
			if (err == ERR){
				fprintf(stderr, "Error in writeSHP.\n" );
			}
		}

		// calculate entropy, and in case node is continuous: mean, quantile and std_dev in another way

		n_states = getNodeNumberStates(target_nodes[i]);

		beliefs_stats = (GByte*)malloc(sizeof(GByte)*n_states);
		if (!beliefs_stats){
			fprintf(stderr,"ERROR: beliefs_stats: Error allocating memory\n");
			return ERR;
		}

		if (target_nodes_types[i] == CONTINUOUS_TYPE){
			mean_v = (double*)malloc(sizeof(double)*total_pixels);
			quantile_v = (double*)malloc(sizeof(double)*total_pixels);
			custom_std_dev_v = (double*)malloc(sizeof(double)*total_pixels);

			if (!mean_v || !quantile_v || !custom_std_dev_v){
				fprintf(stderr,"ERROR: mean, quantile, custom_std_dev: Error allocating memory\n");
				return ERR;
			}
			levels = GetNodeLevels_bn(target_nodes[i]);
		}
		
		// calculate statistics

		for (j =0; j< total_pixels; j++){

			for (k =0; k< n_states; k++)
				beliefs_stats[k] = belief_array_m[i][k][j];

			if (beliefs_stats[0] == NO_DATA_VALUE)
				entropy_v[j] = NO_DATA_VALUE;
			else
				entropy_v[j] = entropy(beliefs_stats, n_states);

			if (target_nodes_types[i] == CONTINUOUS_TYPE){

				if (beliefs_stats[0] == NO_DATA_VALUE){
					entropy_v[j] = cont_nodata;
					mean_v[j] = cont_nodata;
					quantile_v[j] = cont_nodata;
					custom_std_dev_v[j] = cont_nodata;
				}
				else{

					mean_v[j] = mean(beliefs_stats, n_states, (double*)levels, n_states+1);
					quantile_v[j] = quantile(beliefs_stats, n_states, (double*)levels, n_states+1, 50);
					custom_std_dev_v[j] = custom_std_dev(beliefs_stats, n_states, (double*)levels, n_states+1);
				}
			}
		}

		// write statistics into a separate file
		sprintf(output_filename + strlen(output_filename) - strlen(OUTPUT_FILENAME_SUFFIX), CONT_FILENAME_SUFFIX);

		if (target_nodes_types[i] == DISCRETE_TYPE){
			if (rasterfile_n){
				err = writeGeoTIFF(output_filename, rasters[0].hDataset, (void*)(&entropy_v), 1, NO_DATA_VALUE, GDT_Float64 );
				if (err == ERR){
					fprintf(stderr, "Error in writeGeoTIFF.\n" );
				}
			}
			else if (vectorfile_n){
				err = writeSHP(output_filename, vectors[0].hDataset, (void*)(&entropy_v), 1);
				if (err == ERR){
					fprintf(stderr, "Error in writeSHP.\n" );
				}
			}
		}
		
		else if (target_nodes_types[i] == CONTINUOUS_TYPE) {
			cont_data = (double**)malloc(sizeof(double*)* CONT_BANDS_N);
			cont_data[0] = entropy_v;
			cont_data[1] = mean_v;
			cont_data[2] = quantile_v;
			cont_data[3] = custom_std_dev_v;

			if (rasterfile_n){

				// expected value and standard deviation
				err = writeGeoTIFF(output_filename, rasters[0].hDataset, (void*)cont_data, CONT_BANDS_N, cont_nodata, GDT_Float64 );
				if (err == ERR){
					fprintf(stderr, "Error in writeGeoTIFF.\n" );
				}
			}
			else if (vectorfile_n){
				err = writeSHP(output_filename, vectors[0].hDataset, (void*)cont_data, CONT_BANDS_N);

				if (err == ERR){
					fprintf(stderr, "Error in writeSHP.\n" );
				}
			}

			free(mean_v);
			free(quantile_v);
			free(custom_std_dev_v);
			free(cont_data);
		}
	}

	if(debug){
		printf("Total cached executions saved: %d\n", executions_n);
		printf("Total cached executions uses: %d\n", cached_counter);
	}

	for (i = 0;  i < rasterfile_n;  i++){
		closeRaster(rasters[i]);
	}

	for (i = 0;  i < target_nodenames_n;  i++){

		target_node = getNodeInNet(nets_info[0].net, target_nodenames[i]);

		for (j =0; j < (getNodeNumberStates(target_node) + 1); j++){

			if (belief_array_m[i][j]) free(belief_array_m[i][j]);
		}
		if (belief_array_m[i]) free(belief_array_m[i]);
		if (max_prob_m[i]) free(max_prob_m[i]);
		if (beliefs_m[i]) free(beliefs_m[i]);
		if (target_nodenames[i]) free(target_nodenames[i]);
		if (expected_value_m[i]) free(expected_value_m[i]);
		if (std_dev_m[i]) free(std_dev_m[i]);
	}

	for (i = 0;  i < vectornodes_n;  i++){
		free (vectornodes[i].data);
	}

	if (target_nodes_types) free(target_nodes_types);
	if (belief_array_m) free(belief_array_m);
	if (max_prob_m) free(max_prob_m);
	if (expected_value_m) free(expected_value_m);
	if (std_dev_m) free(std_dev_m);
	if (beliefs_m) free(beliefs_m);
	if (target_nodenames) free(target_nodenames);
	if (target_nodes) free(target_nodes);
	
	for (i = 0; i< total_pixels; i++){
		if(beliefs_linking[i]) free(beliefs_linking[i]);
	}

	if(beliefs_linking) free(beliefs_linking);
	if(entropy_v) free(entropy_v);
	
	regfree(&regex);

	if (!rasterfile_n){
		regfree(&regex_filename_node);

		for (i=0; i < vector_nodenames_n; i++)
			free(vector_nodenames[i]);
		free(vector_nodenames);
	}

	for (i=0; i < dbf_n; i++)
		DBF_Free(dbfs[i]);

	if (savingExecutions){
		for (i=0; i < executions_n; i++){
			freeExecution(executions[i]);
		}
		freeExecution(ex);
	}

	closeAllNets(nets_info, n_networks);
	closeBayesNetworkEnv(0);
	finishPythonEnv();
	return OK;
}

int consistentRasters(struct Raster* rasters, int n){

	GDALDatasetH* datasets;
	int i, res;

	datasets = malloc(sizeof(GDALDatasetH)*n);

	for(i=0; i< n; i++)
		datasets[i] = rasters[i].hDataset;
	
	res = checkSameRasterMetaInfo(datasets, n);

	if (datasets) free(datasets);

	if (res == 0)
		return TRUE;
	else
		return FALSE;
}


void printLikelihood_adv(float* likelihood, int len, FILE* fd, char* prefix){
	int i;

	fprintf(fd, "%s[", prefix);
	for (i =0; i < len; i++)
		fprintf(fd, "%f ", likelihood[i]);
	fprintf(fd, "]\n");
}

void printLikelihood(float* likelihood, int len){
	int i;

	printf("[");
	for (i =0; i < len; i++)
		printf("%f ", likelihood[i]);
	printf("]");
}

int setFindingsFromRasters(net_bn* net, struct Raster* rasters, int rasterfile_n, int offset, int debug){

	int i, j, err=NET_OK;
	node_t* node;
	char str_aux[BUFF_LEN];
	int rastercell_nodata = 0;
	float* data;
	float* likelihood = NULL;
	int n_bands;
	int   nXSize;
	int   nYSize;
	int ret, max_index;
	float likelihood_total=0;
	float finding;
	GDALRasterBandH hBand;

	for (j = 0; j < rasterfile_n; j++){

		node = getNodeInNet(net, rasters[j].nodename);

		data = rasters[j].data;

		if (data[offset] != rasters[j].nodatavalue){ // skip NODATA

			n_bands = GDALGetRasterCount(rasters[j].hDataset);

			if ( n_bands == 1){   // ONLY ONE BAND -> HARD EVIDENCE

				finding = data[offset];

				// raster: from 1 to node_states. Node: from 0 to node_states-1
				if (getNodeType(node) == DISCRETE_TYPE)    
					finding -= 1;
				
				err = setNodeFinding(node, finding);

				if (debug){
					if (getNodeType(node) == DISCRETE_TYPE)
						getNodeStateValueStr(node, finding, str_aux);
					else 
						sprintf(str_aux, "_");

					printf("(%s: %.2f (%s) ) ", rasters[j].nodename, (getNodeType(node) == DISCRETE_TYPE)?finding+1:finding, str_aux);
				}
			}

			else{  // SOFT EVIDENCE

				if (getNodeNumberStates(node) != n_bands){
					printf("WARN: setFindingsFromRasters: soft evidence: number of raster bands != number of node states.\n");
				}

				if (getNodeNumberStates(node) > MAX_NODE_STATES){
					printf("ERROR: setFindingsFromRasters: soft evidence: node %s has too much bands (%d). Max is %d.\n", rasters[j].nodename, getNodeNumberStates(node), MAX_NODE_STATES);
				}
				
				if (!likelihood){
					likelihood = (float*)malloc(sizeof(float) * MAX_NODE_STATES);
					if (!likelihood){ 
						fprintf(stderr, "ERROR: setFindingsFromRasters: malloc\n");

						if (data) free(data);
						return ERR;
					}
				}

				hBand = getRasterBand(rasters[j].hDataset, 1, 0);
				nXSize = GDALGetRasterBandXSize( hBand );
				nYSize = GDALGetRasterBandYSize( hBand );

				for (i = 0; i < getNodeNumberStates(node); i++){
					likelihood[i] = data[offset + nXSize*nYSize*i] /100.0 ;
					//likelihood[i] = data[offset + nXSize*nYSize*i];
				}

				// Check if likelihood is correct
				max_index =0;
				for (i = 0; i < getNodeNumberStates(node); i++){
					likelihood_total += likelihood[i];

					if (likelihood[i] >= likelihood[max_index])
						max_index=i;
				}

				if ((likelihood_total < NORMALIZE_MAX) && (likelihood_total > NORMALIZE_MIN) ){

					if (debug)
						printf("WARN. Likelihood for node %s does not add to 100 for cell %d (total was %d). Normalized.\n", rasters[j].nodename, offset, (int)(likelihood_total*100));
					likelihood[max_index] += (1-likelihood_total);
				}
				
				if (debug){
					printf("(%s: ", rasters[j].nodename);
					printLikelihood(likelihood, getNodeNumberStates(node));
					printf(") ");
				}

				
				ret = enterNodeLikelihood(node, likelihood);
				if (ret == NET_ERR){
					sprintf(str_aux, "Error: setFindingsFromRasters: node: %s, offset: %d, likelihood: ", getNodeName(node), offset );
					printLikelihood_adv(likelihood, getNodeNumberStates(node), stderr, str_aux);
					exit(-1);
				}
				else if (ret==NET_NODE_ALL_ZERO){
					sprintf(str_aux, "WARN: setFindingsFromRasters: The likelihood to be applied consisted on all zeros (in case of the node not being a leaf node, it means the likelihood divided by the prior probabilities of the node) for node: %s, offset: %d, likelihood: ", getNodeName(node), offset );
					printLikelihood_adv(likelihood, getNodeNumberStates(node), stderr, str_aux);
					resetNodeFindings(node);   // RESET ALL FINDINGS FROM PREVIOUS CELL

					if (debug){
						printf(" (%s: NODATA) ", rasters[j].nodename);				
					}
					rastercell_nodata++;		
				}
			}

			if (err != NET_OK){
				//if (data) free(data);
				if (likelihood)	free(likelihood);
				return ERR;
			}
		}

		else{
			resetNodeFindings(node);   // RESET ALL FINDINGS FROM PREVIOUS CELL
			if (debug){
				printf(" (%s: NODATA) ", rasters[j].nodename);				
			}
			rastercell_nodata++;
		}

		fflush(stdout);
	} 

	if (likelihood) free(likelihood);

	return rastercell_nodata;
}

int vector_node_is_enabled(char *nodename, char** vector_nodes_disabled, int vector_nodes_disabled_n){
	int i;

	for (i=0; i < vector_nodes_disabled_n; i++)
		if (!strcmp(nodename, vector_nodes_disabled[i]))
			return 0;
	return 1;
}

/*
	The attribute table values range is [0...100]. For hard evidence, a value 0 means NO VALUE and will reset the node findings. For soft evidence, all values must be zero so it means NO VALUE.

	The attribute value can be also an string that should match as state of the node, meaning hard evidence. In case it is not found, it will reset the node findings.

	It will try to match the column name with a regular expression:
		- columnname ---> columnvalue is considered a finding for the 'columname' node
		- collumname__s1 --> columnvalue is considered the probability for the first stae of 'columname' node. It is expected to find as many __s* as number of states of the node. Values are from 0 to 100

	It stores findings and likelihoods in the Execution_t for later saving

	save_execution_only is used for storing the findings in the execution for later search in execution collection without actually setting the node findings

	vector_nodes_disabled: Nodes that would be found in the vector file but should not be used (because they were disabled in the GUI or have manual evidence set)

*/

int setFindingsFromVectors(net_bn* net, vector_t* vectors, int vectors_n, int feature_index, Execution_t* ex, int save_execution_only, char** vector_nodes_disabled, int vector_nodes_disabled_n, vectornode_t* vectornodes, int vectornodes_n, int debug){

	int i, j, err;
	node_t* node;
	char str_aux[BUFF_LEN];
	float finding, state_n;
	char* likelihood_nodenames[256];
	float* likelihood_values[256];
	int likelihoods_n =0;
	int k, ret, state;
	char *finding_str, state_p[256];
	float soft_evidence_sum = 0;
	int max_index;

	if (debug)
		printf("setFindingsFromVectors: ");

	for (i=0; i < vectornodes_n; i++){

		if (vector_node_is_enabled(vectornodes[i].name, vector_nodes_disabled, vector_nodes_disabled_n)){
			node = getNodeInNet(net, vectornodes[i].name);

			if (vectornodes[i].type == VECTORNODE_HARD){

				finding = vectornodes[i].data[feature_index] - 1 ;

				if (finding == -1){
					if (debug){
						printf("(%s: NODATA) ", vectornodes[i].name);				
					}
					if (!save_execution_only)
						resetNodeFindings(node);   // RESET ALL FINDINGS FROM PREVIOUS CELL
					state_n = -1.0;
					if (ex)
						E_addNodeFinding(ex, vectornodes[i].name, &state_n, 1);
				}
				else{
					if (!save_execution_only){
						err = setNodeFinding(node, finding);
						if (err != NET_OK){
							return ERR;
						}
					}
					if (ex)
						E_addNodeFinding(ex, vectornodes[i].name, &finding, 1);
					if (debug){	
						getNodeStateValueStr(node, finding, str_aux);
						printf("(%s: %.2f (%s) )", vectornodes[i].name, finding, str_aux);
					}
				}
			}
			else if (vectornodes[i].type == VECTORNODE_SOFT){
				for (k=0; k < likelihoods_n; k++){
					if (!strncmp(vectornodes[i].name, likelihood_nodenames[k], strlen(vectornodes[i].name)))
						break;
				}

				if( k == likelihoods_n){
					likelihood_nodenames[k] = strdup(vectornodes[i].name);
					likelihood_values[k] = (float*)malloc(sizeof(float)*vectornodes[i].iField);
					likelihoods_n++;
				}

				for (j =0; j < vectornodes[i].iField; j++){
					likelihood_values[k][j] = vectornodes[i].data[feature_index * vectornodes[i].iField + j] / 100.0;
				}
			}
		}
		else{
			if (!save_execution_only)
				resetNodeFindings(node);   // RESET ALL FINDINGS FROM PREVIOUS CELL
		}
	}

	// apply all likelihoods found for the feature
	for (k=0; k < likelihoods_n; k++){

		node = getNodeInNet(net, likelihood_nodenames[k]);

		if (debug){	
			printf(" (%s: [", likelihood_nodenames[k]);

			for (i = 0; i < getNodeNumberStates(node); i++)
				printf("%f ",likelihood_values[k][i]);

			printf("]");
		}

		max_index = 0;
		soft_evidence_sum = 0;
		for (i = 0; i < getNodeNumberStates(node); i++){

			if (likelihood_values[k][i] >= likelihood_values[k][max_index])  // to check if likelihood is correct
				max_index=i;

			soft_evidence_sum += likelihood_values[k][i];
		}

		if (!save_execution_only){

			if ((int)soft_evidence_sum == 0){   // all states are zero --> NODATA
				if (!save_execution_only)
					resetNodeFindings(node);   // RESET ALL FINDINGS FROM PREVIOUS CELL
			}
			else{
				// Check if likelihood is correct
				if ((soft_evidence_sum < NORMALIZE_MAX) && (soft_evidence_sum > NORMALIZE_MIN) ){

					if (debug)
						printf("WARN. Likelihood for node %s does not add to 100 (total was %d). Normalized.\n", getNodeName(node), (int)(soft_evidence_sum*100));
					likelihood_values[k][max_index] += (1-soft_evidence_sum);
				}

				ret = enterNodeLikelihood(node, likelihood_values[k]);
				if (ret != NET_OK){
					sprintf(str_aux, "Error: setFindingsFromVectors: node: %s, likelihood: ", getNodeName(node));
					printLikelihood_adv(likelihood_values[k], getNodeNumberStates(node), stderr, str_aux);
					exit(-1);
				}
			}
		}
		if (ex)
			E_addNodeFinding(ex, likelihood_nodenames[k], likelihood_values[k], getNodeNumberStates(node));
	}
	if (debug)
		printf("\n");
	
	return OK;
}



int setInitialEvidenceAllNetworks(net_info_t* nets_info, int n_networks, struct Evidence* evidences, int evidence_n){

	int i, j, err=NET_OK;
	char str_aux[BUFF_LEN];
	node_t* node;
	net_t* net;

	for (i=0; i < n_networks; i++){
		for (j = 0; j < evidence_n; j++){
			net = nets_info[i].net;
			node = getNodeInNet(net, evidences[j].nodename);
			if (node){

				if (getNodeType(node) == DISCRETE_TYPE){
					getNodeStateValueStr(node, evidences[j].data, str_aux);
					printf(" - Entering finding %.2f (%s) to node %s (%s) in network (%d):'%s'.\n", evidences[j].data, str_aux, evidences[j].nodename, getNodeTitle(node), i, getNetName(net) );
					err = setNodeFinding(node, evidences[j].data);	
				}

				else{
					sprintf(str_aux,"%.2f", evidences[j].data); 

					// set the mean between the states
					printf(" - Entering value %.2f to node %s (%s) in network (%d):'%s'.\n", evidences[j].data, evidences[j].nodename, getNodeTitle(node), i, getNetName(net) );

					EnterNodeValue_bn (node, evidences[j].data);			
				}

				if (err==NET_ERR)
					return ERR;
			}
		}
	}

	return OK;
}

int setInitialLikelihoods(net_info_t* nets_info, struct Likelihood* likelihoods, int likelihoods_n){

	int i, j, ret;
	char str_aux[BUFF_LEN];
	node_t* node;
	net_t* net;

	net = nets_info[0].net;

	for (i = 0; i < likelihoods_n; i++){

		node = getNodeInNet(net, likelihoods[i].nodename);

		if (node){

			if (getNodeType(node) == CONTINUOUS_TYPE){
				printf("setInitialLikelihoods: Error: Trying to set likelihood on a continuous node '%s'\n", likelihoods[i].nodename);
				return ERR;
			}

			if (getNodeNumberStates(node) != likelihoods[i].data_len){
				printf("setInitialLikelihoods: Error: Likelihood length (%d) is different than the number of states of the node '%s' (%d)\n",  likelihoods[i].data_len, likelihoods[i].nodename, getNodeNumberStates(node) );
				return ERR;
			}

			printf(" - Entering likelihood ["); 
			for (j =0; j < likelihoods[i].data_len; j++){
				printf("%d", (int)(100*likelihoods[i].data[j]));

				if( j == likelihoods[i].data_len -1)
					printf("] to node %s\n", likelihoods[i].nodename);
				else
					printf(", ");
			}

			ret = enterNodeLikelihood(node, likelihoods[i].data);
			if (ret != NET_OK){
				return ret;
			}
		}
	}
	return OK;
}


int enterLikeliHoodsFromPYNode(net_t* net, int cell, pyNode_t* pyNodes[], int pyNodes_n){

	int i, j, states;
	node_t* node;
	float* likelihood = NULL;
	int value;
	int ret;
	char str_aux[MAX_BUFF];
	float cont_value;
	int is_no_data;

	for (i=0; i < pyNodes_n; i++){

		node = getNodeInNet(net, pyNodes[i]->nodename);
		if (!node){
			printf("WARN - enterLikeliHoodsFromPYNode: node '%s' not found in network '%s' and will be ignored\n", pyNodes[i]->nodename, getNetName(net));
		}
		else{

			if (pyNodes[i]->type == PY_DISCRETE){

				states = getNodeNumberStates(node);

				likelihood = (float*)realloc(likelihood, sizeof(float)* states);
				if (!likelihood){
					fprintf(stderr, "ERROR: enterLikeliHoodsFromPYNode: realloc\n");
					return ERR;
				}
				is_no_data = 1;
				for (j =0; j < states; j ++){

					value = (int)getStateValueFromPyNode(pyNodes[i], cell, j);
					likelihood[j] = value / 100.0;
					is_no_data = (is_no_data && (value == PY_NODATA_VALUE));  // check if all values are PY_NODATA_VALUE
				}
				if (is_no_data){
					resetNodeFindings(node);
				}
				else{
					ret = enterNodeLikelihood(node, likelihood);
					if (ret != NET_OK){
						sprintf(str_aux, "Error: enterLikeliHoodsFromPYNode: node: %s, offset: %d, likelihood: ", getNodeName(node), cell );
						printLikelihood_adv(likelihood, getNodeNumberStates(node), stderr, str_aux);
						exit(-1);
					
					}
				}
			}
			else if (pyNodes[i]->type == PY_CONTINUOUS){
				cont_value = getValueFromPyNode(pyNodes[i], cell);
				if (cont_value == (float)PY_NODATA_VALUE){
					resetNodeFindings(node);
				}
				else{
					setNodeFinding(node, cont_value);
				}
			}
		}
	}

	if (likelihood)
		free(likelihood);

	return OK;

}

/*
	Fills up 'gis' information with the contents of the raster file and the node name from 'path_node'

	In case 'gis_type' is TYPE_RASTER, gis is suppose to be of type raster_t
	In case 'gis_type' is TYPE_Vector, gis is suppose to be of type vector_t

*/

int loadGISPathMatchingNode(char* path_node, net_info_t net_info, void* gis, regex_t* regex, gis_type_t gis_type, int debug){

	char str_aux[BUFF_LEN];
	int ret;
	int size;
	char path[BUFF_LEN];
	char nodename[MAX_NODENAME_LEN];
	regmatch_t matches[3];
	node_t* node;

	// for vector files, multiple node names are the columns of the attribute table
	// do not need to have filename=nodename. Filename is enough
	if (gis_type == TYPE_VECTOR){
		ret = loadVector(path_node, (vector_t*)gis, 1);
		if (ret){
			fprintf(stderr,"ERROR: loadGISPathMatchingNode: loading vector '%s'\n", path_node);
			return ERR;
		}
		return OK;
	}

	ret = regexec(regex, path_node, 3, matches, 0);

	if (ret==0){

		if ((matches[1].rm_eo - matches[1].rm_so) > BUFF_LEN){
			printf("loadGISPathMatchingNode: File path is too long\n");
			return ERR;
		}

		strncpy(path, path_node+matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
		path[matches[1].rm_eo - matches[1].rm_so] = '\0';

		if ((matches[2].rm_eo - matches[2].rm_so) > MAX_NODENAME_LEN){
			printf("loadGISPathMatchingNode: Nodename is too long\n");
			return ERR;
		}

		strncpy(nodename, path_node+matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
		nodename[matches[2].rm_eo - matches[2].rm_so] = '\0';

		node = getNodeInNet(net_info.net, nodename);

		if (!node){
			fprintf(stderr,"ERROR: loadGISPathMatchingNode: Node %s not found in the network\n", nodename);
			return ERR;
		}

		if (gis_type == TYPE_RASTER){

			((raster_t*)gis)->hDataset = loadRaster( path, debug);
			
			if (((raster_t*)gis)->hDataset){


				if ((GDALGetRasterCount(((raster_t*)gis)->hDataset) > 1) && (GDALGetRasterCount(((raster_t*)gis)->hDataset) != getNodeNumberStates(node))){
					fprintf(stderr,"ERROR: loadGISPathMatchingNode: Number of raster bands (%d) should be either one, for hard evidence, or the same as the number of states of the node (%d), for soft evidence -- %s\n", GDALGetRasterCount(((raster_t*)gis)->hDataset),  getNodeNumberStates(node), getNodeName(node));
					return ERR;
				}

				((raster_t*)gis)->nodatavalue = (float)GDALGetRasterNoDataValue(getRasterBand(((raster_t*)gis)->hDataset, 1, FALSE), NULL);
			
				strncpy(((raster_t*)gis)->nodename, getNodeName(node), MAX_NODENAME_LEN);
					
				((raster_t*)gis)->nets_n = 1; 
				((raster_t*)gis)->nets[0] = net_info.net;

				strncpy(((raster_t*)gis)->filename, path, BUFF_LEN);

				size = readRasterData(&(((raster_t*)gis)->data), ((raster_t*)gis)->hDataset, 0, debug);

				if (size == -1){
					fprintf(stderr,"ERROR: loadGISPathMatchingNode: Error reading raster data (%s)\n", ((raster_t*)gis)->nodename);
					return ERR;
				}

				if(debug){
					printf("Raster file '%s' will be used to fill node %s (%s) likelihood. Data size: %d\n", path, getNodeName(node), getNodeTitle(node), size);
				}
			}
			else{
				fprintf(stderr,"ERROR: loadGISPathMatchingNode: loading raster '%s'\n", path);
				return ERR;
			}
		}

		else if (gis_type == TYPE_VECTOR){

			ret = loadVector(path, (vector_t*)gis, 1);

			if (ret){
				if (debug)
					fprintf(stderr,"ERROR: loadGISPathMatchingNode: loading vector '%s'\n", path);
				return ERR;
			}
			strncpy(((vector_t*)gis)->nodename, getNodeName(node), MAX_NODENAME_LEN);
		}

		else{
			fprintf(stderr,"ERROR: loadGISPathMatchingNode: Unknown gis_type '%d'\n", gis_type);
			return ERR;
		}
	}
	else if (ret == REG_NOMATCH){
		printf("loadGISPathMatchingNode: path_node is wrong. Expecting 'filename=nodename'. Regex: '%s' <- '%s'\n", REGEX_FILENAME_NODE, path_node);
		return ERR;
	}
	else{ 
		regerror(ret, regex, str_aux, sizeof(str_aux));
		fprintf(stderr, "getNodeNumberFromVector: Regex match failed: %s\n", str_aux);
		return ERR;
	}
	return OK;

}

int loadEvidenceMatchingNode(char* evidence_name, float evidence_value, net_info_t net_info, struct Evidence* evidence, int debug){

	node_t* node;
	char str_aux[MAX_BUFF];
	char str_aux2[MAX_BUFF];

	node = getNodeInNet(net_info.net, evidence_name);

	if (node){
		strncpy(evidence->nodename, evidence_name, MAX_NODENAME_LEN);
		evidence->data = evidence_value;
	}
	else{
		printf("WARNING - Evidence '%s' does not match any node in the network '%s' and will be ignored. - WARNING\n", evidence_name, getNetName(net_info.net) );	
		return ERR;
	}
	return OK;
}

int loadLinkingMatchingNode(char* linking_src, char* linking_dst, net_info_t* nets_info, int n_networks, struct Linking* linking, int debug){

	node_t* node;
	net_t* net;
	int k;
	int counter = 0;  // to mark if linking is right
	int valid_linking;

	for (k=0; k < n_networks; k++){

		net = nets_info[k].net;

		node = getNodeInNet(net, linking_src);

		valid_linking = OK;

		if (node){
			
			if (debug)
				printf("Linking source '%s' matches node '%s' (%s) in network '%s'.\n", linking_src, getNodeName(node), getNodeTitle(node), getNetName(net));

			linking->src = getNodeInNet(net, linking_src);
			linking->net_src = net;

			node = getNodeInNet(net, linking_dst);

			if (node){

				if (debug)
					printf("Linking destination '%s' matches node '%s' (%s) in network %s.\n", linking_dst, getNodeName(node), getNodeTitle(node), getNetName(net));

				linking->dst = getNodeInNet(net, linking_dst);
				linking->net_dst = net;

				if (getNodeNumberStates(linking->src) != getNodeNumberStates(linking->dst) ){

					fprintf(stderr,"ERROR:  - WARNING - Linking nodes have different number of states '%s=%s'. Linking will be ignored.\n",
						getNodeName(linking->src),getNodeName(linking->dst));

					valid_linking = ERR;
				}
				if (valid_linking == OK){
					counter++;
				}
			}
			else
				printf("WARNING - Linking destination '%s' does not match any node in the network and will be ignored. - WARNING\n", linking_dst);
		}
		else
			printf("WARNING - Linking source '%s' does not match any node in the network and will be ignored. - WARNING\n", linking_src); 	
	}
	return counter;
}


void closeRaster(struct Raster raster){
	if (raster.data)
		free(raster.data);
	closeGdalRaster(raster.hDataset);
}


float** getPixelVector(struct Raster raster, int pixel, float **vector){

	GDALRasterBandH hBand;
	int  nXSize, nYSize, n_bands, i;
	
	hBand = getRasterBand(raster.hDataset, 1, 0);
	n_bands = GDALGetRasterCount(raster.hDataset);
	nXSize = GDALGetRasterBandXSize( hBand );
	nYSize = GDALGetRasterBandYSize( hBand );

	for (i = 0; i < n_bands; i++){
		vector[i] = &(raster.data[pixel + nXSize*nYSize*i]);
	}
	return vector;

}

/*

It modifies all raster.data values to the right node state values according to the DBF

*/
int applyDBFtoRasterData(DBF dbf, struct Raster raster, node_t* node, int debug){

	GDALRasterBandH hBand;

	int i;
	int nXSize;
	int nYSize;
	int node_states;
	int *conv_table;
	int dbf_value;
	char str_aux[256];
	
	hBand = getRasterBand(raster.hDataset, 1, 0);
	nXSize = GDALGetRasterBandXSize( hBand );
	nYSize = GDALGetRasterBandYSize( hBand );
	node_states = getNodeNumberStates(node);

	conv_table = (int*)malloc(sizeof(int) * node_states);

	if(debug){
		printf("applyDBFtoRasterData: DBF: \n");
		DBF_Print(dbf);
	}

	for (i=0; i < node_states; i++){

		getNodeStateValueStr(node, i, str_aux);
		dbf_value = DBF_getValueforStateName(dbf, str_aux);
		if (dbf_value == -1){
			fprintf(stderr, "applyDBFtoRasterData: WARNING - state name '%s' does not match any DBF entry\n", str_aux);

			conv_table[i] = i;

		}
		else
			conv_table[i] = dbf_value; 
	}

	if (debug){
		printf("applyDBFtoRasterData: Using DBF to convert raster values for node %s: ", getNodeName(node));
		printf("Conversion table: [");

		for (i=0; i < node_states; i++)
			printf("%d ", conv_table[i]);

		printf("]\n");
	}

	for (i = 0; i < nXSize*nYSize; i++){

		if (raster.data[i] != raster.nodatavalue){
			
			if (raster.data[i] > (node_states -1)){
				fprintf(stderr, "applyDBFtoRasterData: WARNING - raster data value '%f' is bigger than the total number of states of the node. Not applying DBF value.\n", raster.data[i]);
			}
			else{
				raster.data[i] = conv_table[(int)(raster.data[i])];
			}
		}
	}
	free(conv_table);
	return OK;
}

/*
	Returns TRUE if the pixel corresponding to that node/raster is NODATA, or if each pixel in every raster is NODATA 
*/
int nodeIsNODATA(node_t* node, struct Raster* rasters, int rasterfile_n, int offset){
	
	int rastercell_nodata = 0;
	int i;
	for (i = 0; i < rasterfile_n; i++){
		if (!strcmp(getNodeName(node),rasters[i].nodename)){
			return (rasters[i].data[offset] == rasters[i].nodatavalue);
		}
		if (rasters[i].data[offset] == rasters[i].nodatavalue)
			rastercell_nodata++;
	}
	return (rasterfile_n == rastercell_nodata);
}


int loadPyNodeData(node_t* node, pyNode_t* pyNode, int is_no_data){

	char py_node_beliefs_bytes[256]; 
	float* py_node_beliefs, *tmp;
	int i, n_states;
	int sum = 0;
	int rem;
	int max_index;
	float max;
	double val;  

	n_states = getNodeNumberStates(node);

	if (getNodeType(node) == DISCRETE_TYPE){

		if (is_no_data){
			for (i = 0; i< n_states; i++){
				py_node_beliefs_bytes[i] = (char)PY_NODATA_VALUE;   
			}
		}

		else{

			py_node_beliefs = getNodeBeliefs(node);

			for (i = 0; i< n_states; i++){
				py_node_beliefs_bytes[i] = (char)(py_node_beliefs[i]*100);
				sum += py_node_beliefs_bytes[i];
			}
			
			if (sum != 100){

				tmp = (float*)malloc(sizeof(float)* n_states);
				memcpy(tmp, py_node_beliefs, sizeof(float)* n_states);

				rem = 100 - sum;
				
				while(rem){

					max = tmp[0];
					max_index = 0;

					for (i = 1; i< n_states; i++){
						if(tmp[i] >= max ){
							max = tmp[i];
							max_index = i;
						}
					}

					py_node_beliefs_bytes[max_index]++;
					tmp[max_index] = 0;
					rem--;
				}
				free(tmp);
			}
		}
		pyAddData(pyNode, py_node_beliefs_bytes, n_states);

	}
	else if (getNodeType(node) == CONTINUOUS_TYPE){
	
		if (is_no_data)
			pyAddValue(pyNode, PY_NODATA_VALUE);

		else{
			val = GetNodeValueEntered(node);
			if (val != NET_UNDEFINED_VALUE){ // it was not set explicitely as hard evidence
				pyAddValue(pyNode, val);
			}
			else{
				pyAddValue(pyNode, PY_NODATA_VALUE);
			}
		}
	}

	return 0;

}

/* Checks that the pyNodes returns have names that are in the network,
* and that the state number is correct
* It also sets pyNode->node_states. 
*/
int checkPyNodes(net_info_t* net_info, pyNode_t** py_node_list, int len, int total_pixels){

	node_t* node;
	int i,j;

	for (i =0; i < len; i++){

		node = getNodeByName(py_node_list[i]->nodename, net_info, 1);
		if (!node){
			fprintf(stderr, "checkPyNodes: Error: node %s not found in the network\n", py_node_list[i]->nodename);
			return -1;
		}
		py_node_list[i]->node_states = getNodeNumberStates(node);
	
		if ((py_node_list[i]->type == PY_DISCRETE) && (py_node_list[i]->node_states != py_node_list[i]->data_len / total_pixels)){
			fprintf(stderr, "checkPyNodes: Error: node %s: data length (%d) should be the number of states of the node (%d) divided by total pixels (%d) for DISCRETE nodes\n", py_node_list[i]->nodename, py_node_list[i]->data_len, py_node_list[i]->node_states, total_pixels);
			return -1;
		}
		if ((py_node_list[i]->type == PY_CONTINUOUS) && (py_node_list[i]->values_len != total_pixels)){
			fprintf(stderr, "checkPyNodes: Error: node %s: values length (%d) should be the number of total pixels (%d) for CONTINUOUS nodes\n", py_node_list[i]->nodename, py_node_list[i]->data_len, total_pixels);
			return -1;
		}
	}
	return 0;
}

int numPlaces (int n) {
	if (n < 10) return 1;
	if (n < 100) return 2;
	if (n == 100) return 3;
	return ERR;
}

int storeInitialLikelihood(struct Likelihood* likelihood, char* likelihood_str, regex_t* regex){

	int i, err, ret, val, n;
	char buffer[128];
	regmatch_t matches[4];
	char* ptr;

	ret = regexec(regex, likelihood_str, 4, matches, 0);

	if(!ret){

		strncpy(likelihood->nodename, likelihood_str+matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
		strncpy(buffer, likelihood_str+matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);

		likelihood->data_len = 1;
		likelihood->data[0]= atof(buffer)/100;

		n = numPlaces(atoi(buffer));
		if(n == ERR){
			fprintf(stderr, "storeInitialLikelihood: Wrong value '%d'\n", atoi(buffer));
			return ERR;
		}

		ptr = likelihood_str + matches[2].rm_so + n+1;

		while(sscanf( ptr, "%d", &val ) == 1){

			likelihood->data[likelihood->data_len]= val/100.0;

			n = numPlaces(val);
			if(n == ERR){
				fprintf(stderr, "storeInitialLikelihood: Wrong value '%d'\n", val);
				return ERR;
			}

			ptr += n+1;

			likelihood->data_len++;
		}

	}
	else if (ret == REG_NOMATCH){
		fprintf(stderr, "storeInitialLikelihood: likelihood string is wrong. Expecting 'nodename=[s1,s2,...,sn]', with no spaces and no decimals. Regex: '%s' <- '%s'\n", REGEX_NODENAME_LIKELIHOOD, likelihood_str);
		return ERR;
	}
	else{ 
		regerror(ret, regex, buffer, sizeof(buffer));
		fprintf(stderr, "storeInitialLikelihood: Regex match failed: %s\n", buffer);
		return ERR;
	}

	return OK;
}

/*
Fills in the vectornodes struct so setFindingsFromVectors knows already what nodes to look for in the dataset and thus speed up a bit
*/
int getVectornodesFromVector(net_bn* net, vector_t* vector, vectornode_t* vectornodes, regex_t* regex, int debug){

	int i, j;
	int iField;
	OGRLayerH hLayer;
	OGRFeatureDefnH hFDefn;
	OGRFieldDefnH hFieldDefn;
	int n_features, n_fields, ret, state;
	char *fieldname;
	regmatch_t matches[3];
	char buffer[128];
	node_t* node;

	int vectornodes_n  =0;
	
	for (iField =0; iField < vector->n_fields; iField++){

		hFieldDefn = OGR_FD_GetFieldDefn( vector->hFDefn, iField );
		fieldname = (char*)OGR_Fld_GetNameRef(hFieldDefn);

		// check if it is a likelihood. It has "__s" in the name
		ret = regexec(regex, fieldname, 3, matches, 0);

		if (!ret){   // It is likelihood (soft evidence)

			strncpy(buffer, fieldname+matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
				buffer[matches[1].rm_eo - matches[1].rm_so] = '\0';

			for (j=0; j < vectornodes_n; j++){
				if (!strncmp(buffer, vectornodes[j].name, strlen(buffer)))
					break;
			}
			node = getNodeInNet(net, buffer);
			if (node){
				if( j == vectornodes_n){
					vectornodes[vectornodes_n].name = strdup(buffer);
					vectornodes[vectornodes_n].type = VECTORNODE_SOFT;
					vectornodes[vectornodes_n].iField = 0; 
					vectornodes[vectornodes_n].iFields = (int*)malloc(sizeof(int)*getNodeNumberStates(node));
					vectornodes_n++;
				}

				strncpy(buffer, fieldname+matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
				buffer[matches[2].rm_eo - matches[2].rm_so] = '\0';
				state = atoi(buffer);
				if (state > 256){
					fprintf(stderr, "getVectornodesFromVector: State out of range");
					return ERR;
				}
				vectornodes[vectornodes_n-1].iFields[state-1] = iField;
				vectornodes[vectornodes_n-1].iField++;
			}
		}
		else if (ret == REG_NOMATCH) {  // no match. It is a finding (hard evidence)
			node = getNodeInNet(net, fieldname);
			if (node){
				vectornodes[vectornodes_n].name = strdup(fieldname);
				vectornodes[vectornodes_n].type = VECTORNODE_HARD;
				vectornodes[vectornodes_n].iField = iField;
				vectornodes_n++;
			}
		}
		else{ 
			regerror(ret, regex, buffer, sizeof(buffer));
			fprintf(stderr, "getNodeNumberFromVector: Regex match failed: %s\n", buffer);
			regfree(regex);
			return -1;
		}
	}
	if(debug){
		for (i=0; i < vectornodes_n; i++){
			printf("getVectornodesFromVector: %s %s\n", vectornodes[i].name, vectornodes[i].type==VECTORNODE_HARD?"VECTORNODE_HARD":"VECTORNODE_SOFT");
		}
	}

	return vectornodes_n;
}

/*
	To read all data from a vector at once and store it in vectornodes->data.
	It reads whatever it is in the vector. Not adjusting anything.
	vectornodes->data should be freed at the end
*/
int readVectornodesData(net_bn* net, vector_t* vector, vectornode_t* vectornodes, int vectornodes_n, int debug){

	OGRFeatureH hFeature;
	OGRFieldDefnH hFieldDefn;
	node_t* node;
	int i, idx, state_n, feature_index = 0, c=0;
	char *finding_str, state_p[256];

	for (idx = 0; idx < vectornodes_n; idx++){

		if (vectornodes[idx].type == VECTORNODE_HARD){
			vectornodes[idx].data = (int*)malloc(sizeof(int)*vector->n_features);

			c += sizeof(int)*vector->n_features;
		}
		else if (vectornodes[idx].type == VECTORNODE_SOFT){
			vectornodes[idx].data = (int*)malloc(sizeof(int)* vector->n_features * getNodeNumberStates(getNodeInNet(net, vectornodes[idx].name)));

			c += sizeof(int)* vector->n_features * getNodeNumberStates(getNodeInNet(net, vectornodes[idx].name));
		}
		if (!vectornodes[idx].data){
			fprintf(stderr, "readVectornodesData: malloc");
			return ERR;
		}
	}

	OGR_L_ResetReading(vector->hLayer);

	while( (hFeature = OGR_L_GetNextFeature(vector->hLayer)) != NULL ){
		
		for (idx = 0; idx < vectornodes_n; idx++){

			node = getNodeInNet(net, vectornodes[idx].name);

			if (vectornodes[idx].type == VECTORNODE_HARD){

				hFieldDefn = OGR_FD_GetFieldDefn( vector->hFDefn, vectornodes[idx].iField );

				switch(OGR_Fld_GetType(hFieldDefn)){

					case OFTString:

						vectornodes[idx].data[feature_index] = 0; // in case of attribute == 0 --> reset node findings
						finding_str = (char*)OGR_F_GetFieldAsString( hFeature, vectornodes[idx].iField );

						for ( state_n=0 ; state_n < getNodeNumberStates(node) ; state_n++){

							getNodeStateValueStr(node, state_n, state_p);
							if (finding_str && !strncmp(finding_str, state_p, strlen(finding_str)) ){
								vectornodes[idx].data[feature_index] = state_n;
								break;
							}
						}
						break;

					case OFTInteger:
					case OFTInteger64:
					case OFTReal:

						vectornodes[idx].data[feature_index] = OGR_F_GetFieldAsInteger64( hFeature, vectornodes[idx].iField ) ;  // in case of attribute == 0 --> reset node findings

						break;

					default:
						fprintf(stderr, "readVectornodesData: field type is netither string nor integer\n");
						return ERR;
						break;
				}
			}
			else if (vectornodes[idx].type == VECTORNODE_SOFT){

				for (i = 0; i < vectornodes[idx].iField; i++ ){
					vectornodes[idx].data[feature_index*vectornodes[idx].iField + i] = OGR_F_GetFieldAsInteger64( hFeature, vectornodes[idx].iFields[i] );
				}
			}
		}

		feature_index++;
		OGR_F_Destroy(hFeature);
	}

	if (debug)
		printf("readVectornodesData: %d features were read. %d bytes allocated\n", feature_index, c);

	return OK;
}