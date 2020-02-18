#include <stdio.h>
#include <json/json.h>
#include "vector.h" 


int main(int argc, char* argv[]){

	int i, ret;
	vector_t vector;
	char **vector_nodenames = NULL; // node names read from the vector attribute table
	int vector_nodenames_n = 0;

	if (argc < 2){
		printf("Usage %s vector_filename\n", argv[0]);
	}

	ret = loadVector(argv[1], &vector, 0);
	if (ret !=0)
		return -1;

	vector_nodenames_n = getNodeNamesFromVector(vector, &vector_nodenames, 0);
	if (vector_nodenames_n < 0)
		return -1;

	json_object *jnodenames = json_object_new_array();

	for (i =0; i < vector_nodenames_n; i++){
		json_object_array_add(jnodenames,json_object_new_string(vector_nodenames[i]));
	}

	printf("%s\n",json_object_to_json_string(jnodenames));

	json_object_put(jnodenames);

	free(vector_nodenames);

	return 0;
}