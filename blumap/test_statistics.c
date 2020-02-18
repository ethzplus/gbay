#include <stdio.h>
#include "statistics.h"

//unsigned char vector[5] = {30,0,10,10,50};
//int len = 5;
//double breaks[6] = {0, 1.5, 3, 4.5, 6, 10};

unsigned char vector[8] = {1, 2, 6, 29, 35, 18, 5, 1};
//unsigned char vector[8] = {100, 0, 0, 0, 0, 0, 0, 0};
int len = 8;


double breaks[9] = {0, 1, 100, 200, 300, 400, 500, 600, 700};

int main(){
	

	int i;



	printf("float: %d\ndouble: %d\n", sizeof(float), sizeof(double));


	printf("[");
	for (i =0; i < len; i++){
		printf("%d", vector[i]);
		if (i< (len-1))
			printf(", ");
		else
			printf("]\n");
	}

	printf("entropy: %f\n", entropy(vector, len));
	printf("mean: %f\n", mean(vector, len, breaks, len+1));
	printf("quantile: %f\n", quantile(vector, len, breaks, len+1, 50));
	//printf("variance: %f\n", variance(vector, len, breaks, len+1));
	printf("custom_std_dev: %f\n", custom_std_dev(vector, len, breaks, len+1));
	
	return 0;

}