#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h> 
#include <sys/types.h>

#define MAX_NODES 128
#define MAX_STATES 128

typedef struct {
	char* nodename;
	float* likelihood;
	int node_states_n;
} node_likelihood_t;

int main(){

	regex_t regex;
	regmatch_t matches[3];

	char regex_str[] = "^(\\w*)__[s|S]([1-9][0-9]*)$";

	int ret, i, j;
	char buffer[128];
	char *fieldname;

	char *fieldnames[11] = { "lu_t0__s1", "lu_t0__s2", "lu_t0__s3", "lu_t0__s4", "lu_t0__s5", "species", "node42__s1", "node42__s2", "node42__s3", "node42__s4", "node42__s5", };
	float fieldvalues[11] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 2.0, 3.0, 4.0, 5.0, 6.0};
	int state;

	node_likelihood_t node_likelihoods[MAX_NODES];
	int node_likelihoods_n = 0;


	ret = regcomp(&regex, regex_str, REG_EXTENDED);
	if (ret) {
	    fprintf(stderr, "Could not compile regex\n");
	    exit(1);
	}



	for (i=0; i < 11; i++){

		fieldname = fieldnames[i];

		//printf("Fieldname: %s\n", fieldname);

		// check if it is a likelihood. It has "__s" in the name
		ret = regexec(&regex, fieldname, 3, matches, 0);

		if (!ret){

			strncpy(buffer, fieldname+matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
			buffer[matches[1].rm_eo - matches[1].rm_so] = '\0';

			//printf("nodename: %s\n", buffer);


			for (j=0; j < node_likelihoods_n; j++){
				if (!strncmp(buffer, node_likelihoods[j].nodename, strlen(buffer)))
					break;
			}

			if( j == node_likelihoods_n){
				node_likelihoods[j].nodename = strdup(buffer);
				node_likelihoods[j].likelihood = (float*)malloc(sizeof(float)*MAX_NODES);
				node_likelihoods[j].node_states_n = 0;
				node_likelihoods_n++;
			}




			strncpy(buffer, fieldname+matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
			buffer[matches[2].rm_eo - matches[2].rm_so] = '\0';
			

			state = atoi(buffer);
			//printf("state: %s (%d)\n", buffer, state);

			if (state > MAX_STATES){
				fprintf(stderr, "State out of range");
				exit(1);
			}
			else{
				node_likelihoods[j].likelihood[state] = fieldvalues[state];
				node_likelihoods[j].node_states_n++;
			}
		}

		else if (ret == REG_NOMATCH) {  // no match. IT IS A FINDING    
		    printf("Finding: %s\n", fieldname);
		}

		else{   // error

			regerror(ret, &regex, buffer, sizeof(buffer));
		    fprintf(stderr, "Regex match failed: %s\n", buffer);
		    exit(1);
		}
	}


	for (i=0; i < node_likelihoods_n; i++ ){

		node_likelihoods[i].likelihood = (float*)realloc(node_likelihoods[i].likelihood, sizeof(float)*node_likelihoods[i].node_states_n);

		printf("Nodename: %s: [",node_likelihoods[i].nodename);

		for (j=0; j < node_likelihoods[i].node_states_n; j++ ){
			printf("%f ", node_likelihoods[i].likelihood[j]);
		}

		printf("]\n");
	
	}








	regfree(&regex);


	return 0;
}
