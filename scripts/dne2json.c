/*
	Parse a .dne file passed as a argument and print nodes information 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h> 
#include <errno.h>

#define REGEX_NODE_HEADER "^node (\\w+) \\{\r?\n$"   // support for Windows '\r'
#define REGEX_NODE_ATTR "^\t(title|kind|discrete|parents|belief|states) = ([^;]+);"
#define REGEX_NODE_END "^\t\\};\r?\n$"

#define REGEX_NODE_VISUAL_HEADER "^\tvisual (\\w+) \\{\r?\n$"
#define REGEX_NODE_VISUAL_ATTR "^\t\t(center|height) = ([^;]+);"
#define REGEX_NODE_VISUAL_END "^\t\t\\};\r?\n$"

#define MATCHES_MAX 3
#define MAX_BUFFER 32768 // 32K
#define MAX_LINE 256

void printHelp(char* command){
	printf("Usage: %s DNE_FILE\n", command);
}

// convert an .dne array into JSON.    (aaa, bbb, ccc) --> ["aaa", "bbb", "ccc"]
// string returned should be freed afterwards
char* convert_strings_array(char* src){

	int i;
	char* dst=NULL;;

	if (src[0] != '('){
		fprintf(stderr, "convert_array: First character of src is not '('\n");
		return NULL;
	}
	if (src[strlen(src)-1] != ')'){
		fprintf(stderr, "convert_array: Last charactaer of src is not ')'\n");
		return NULL;
	}
	dst = malloc(sizeof(char)*MAX_LINE);
	if (!dst){
		perror("malloc");
		return NULL;
	}

	sprintf(dst, "[");

	if (src[1] == ')' ){
		sprintf(dst+1, "]");
		return dst;
	}
	else{
		sprintf(dst+1, "\"");
	}

	for(i =1; i<strlen(src); i++){

		switch(src[i]){

			case ',':
				sprintf(dst+strlen(dst),"\",");
				break;
			case ')':
				sprintf(dst+strlen(dst),"\"]");
				break;
			case ' ':
				sprintf(dst+strlen(dst)," \"");
				break;
			default:
				sprintf(dst+strlen(dst),"%c", src[i]);
				break;
		}

		if(strlen(dst) > MAX_LINE -2){
			fprintf(stderr, "convert_strings_array: Line too long : %s\n", src);
			free(dst):
			return NULL;
		}
	}

	return dst;
}

int main(int argc, char* argv[]){
	
	FILE* fd;
	char *line = NULL;
	char *converted = NULL;  // to transform arrays into JSON syntax
	char aux[MAX_LINE];

	size_t len = 0;
	ssize_t read;

	int first_node=1; 
	int in_node=0;
	int in_attr = 0; // to check if commas should be written
	int in_visual = 0; 
	int is_array=0;

	char output_buffer[MAX_BUFFER];

	regex_t regex_node_header;
	regex_t regex_node_attr;
	regex_t regex_node_end;
	regex_t regex_node_visual_header;
	regex_t regex_node_visual_attr;
	regex_t regex_node_visual_end;

	regmatch_t matches[MATCHES_MAX+1];

	if (argc < 2){
		printHelp(argv[0]);
		return 0;
	}
	fd = fopen(argv[1], "r");
	if (!fd){
		fprintf(stderr, "Error: fopen: %s\n", argv[1]);
	}
	
	if (regcomp(&regex_node_header, REGEX_NODE_HEADER, REG_EXTENDED)) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODE_HEADER);
	    return -1;;
	}
	if (regcomp(&regex_node_attr, REGEX_NODE_ATTR, REG_EXTENDED)) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODE_ATTR);
	    return -1;;
	}
	if (regcomp(&regex_node_end, REGEX_NODE_END, REG_EXTENDED)) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODE_END);
	    return -1;;
	}
	if (regcomp(&regex_node_visual_header, REGEX_NODE_VISUAL_HEADER, REG_EXTENDED)) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODE_VISUAL_HEADER);
	    return -1;;
	}
	if (regcomp(&regex_node_visual_attr, REGEX_NODE_VISUAL_ATTR, REG_EXTENDED)) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODE_VISUAL_ATTR);
	    return -1;;
	}
	if (regcomp(&regex_node_visual_end, REGEX_NODE_VISUAL_END, REG_EXTENDED)) {
	    fprintf(stderr, "Could not compile regex: %s\n", REGEX_NODE_VISUAL_END);
	    return -1;;
	}
	
	sprintf(output_buffer, "[");

	while ((read = getline(&line, &len, fd)) != -1) {

        //printf("Retrieved line of length %zu :\n", read);
        //printf("%s\n", line);

        if (!regexec(&regex_node_header, line, MATCHES_MAX, matches, 0)){
		
			if(!first_node)
				sprintf(output_buffer + strlen(output_buffer), ",");

			sprintf(output_buffer + strlen(output_buffer), "{\n\t\"name\": \"");
			strncpy(output_buffer + strlen(output_buffer), line+matches[1].rm_so,  matches[1].rm_eo - matches[1].rm_so);
			sprintf(output_buffer + strlen(output_buffer), "\",");

			first_node=0;
			in_node = 1;
			in_attr = 0;
		}
		else if (in_node && !regexec(&regex_node_attr, line, MATCHES_MAX, matches, 0)){ 

			//printf("%s matches node attr\n", line);

			if (in_attr)
				sprintf(output_buffer + strlen(output_buffer),",\n\t\"");
			else
				sprintf(output_buffer + strlen(output_buffer),"\n\t\"");

			strncpy(output_buffer + strlen(output_buffer), line+matches[1].rm_so,  matches[1].rm_eo - matches[1].rm_so);
			sprintf(output_buffer + strlen(output_buffer), "\" : ");

			if (line[matches[2].rm_so] == '('){
				memset(aux, '\0', MAX_LINE);
				strncpy(aux, line+matches[2].rm_so,  matches[2].rm_eo - matches[2].rm_so);
				converted = convert_strings_array(aux);
				
				if (!converted){
					if (line)
				    	free(line);
					fclose(fd);
					return -1;
				}

				strcat(output_buffer, converted);
				free(converted);
			}
			else{

				if (line[matches[2].rm_so] == '\"'){  // To avoid double double quotes
					matches[2].rm_so++;
					matches[2].rm_eo--;
				}

				sprintf(output_buffer + strlen(output_buffer), "\"");
				strncpy(output_buffer + strlen(output_buffer), line+matches[2].rm_so,  matches[2].rm_eo - matches[2].rm_so);
				sprintf(output_buffer + strlen(output_buffer), "\"");
			}

			in_attr = 1;
		}
		else if (in_node && !regexec(&regex_node_end, line, MATCHES_MAX, matches, 0)){
			sprintf(output_buffer + strlen(output_buffer), "\n}");
			in_node = 0;
			in_attr = 0;
		}
		else if (in_attr && !regexec(&regex_node_visual_header, line, MATCHES_MAX, matches, 0)){
			sprintf(output_buffer + strlen(output_buffer), ",\n\t\"visual\":{");
		}
		else if (in_attr && !regexec(&regex_node_visual_attr, line, MATCHES_MAX, matches, 0)){

			if (in_visual)
				sprintf(output_buffer + strlen(output_buffer),",\n\t\t\"");
			else
				sprintf(output_buffer + strlen(output_buffer),"\n\t\t\"");

			strncpy(output_buffer + strlen(output_buffer), line+matches[1].rm_so,  matches[1].rm_eo - matches[1].rm_so);
			sprintf(output_buffer + strlen(output_buffer), "\" : ");

			if (line[matches[2].rm_so] == '('){
				sprintf(output_buffer + strlen(output_buffer), "[");
				matches[2].rm_so++;
				matches[2].rm_eo--;
				is_array=1;
			}
			else{
				is_array=0;
			}

			strncpy(output_buffer + strlen(output_buffer), line+matches[2].rm_so,  matches[2].rm_eo - matches[2].rm_so);
			
			if (is_array)
				sprintf(output_buffer + strlen(output_buffer), "]");

			in_visual = 1;
		}
		else if (in_visual && !regexec(&regex_node_visual_end, line, MATCHES_MAX, matches, 0)){
			sprintf(output_buffer + strlen(output_buffer), "\n\t}");
			in_visual = 0;
		}

		if (strlen(output_buffer) > (MAX_BUFFER - 128)){

			printf("File is too big. Exiting.\n");
			if (line)
				free(line);
			fclose(fd);
			return -1;
		}
    }

	sprintf(output_buffer + strlen(output_buffer), "]\n");
    printf("%s\n",output_buffer); 
    if (line)
    	free(line);
	fclose(fd);
	return 0;
}