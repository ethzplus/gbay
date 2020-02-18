#include <math.h>

int sum(unsigned char* v, int len, int start, int end);

double entropy(unsigned char* v, int len){
	
	int i;
	double hi, h=0, hmax, val;

	if (len < 1 || !v)
		return -1;
	else if (len == 1)
		return 0;

	for (i =0; i < len; i++){

		val = (double)v[i];
		hi = (val>0)?(val/100*log2(val/100)):0;
		h += hi;
	}
	hmax = log2(len);

	return (-1*h/hmax);
}

// #### 3. Mean of the probability distribution  ####
// # v is a vector of probabilities
// # breaks are the discretization breaks
// # length of the breaks should be length(v)+1
double mean(unsigned char* v, int len, double* breaks, int n_breaks){

	int i;
	double center, mean=0;

	if (!v || !breaks || (n_breaks!=(len+1)) )
		return -1;

	for (i =0; i < len; i++){
		center = (breaks[i] + breaks[i+1])/2;
		mean += (center*v[i]/100);
	}

	return mean;
}



int sum(unsigned char* v, int len, int start, int end){
	
	int i;
	int sum=0;
		
	if (!v || (start < 0) || (end > len))
		return -1;
	for (i =start; i < end; i++){
		sum += v[i];
	}
	return sum;
}

// #### 4. get  median (or quantiles) from distribution ####
// # v is a vector of probabilities
// # breaks are the discretization breaks
// # length of the breaks should be length(v)+1
// # q is the quantile in %
// #to get the median: q = 50
double quantile(unsigned char* v, int len, double* breaks, int n_breaks, int q){

	int i;
	int L;
	double c,h,f,quant;

	if (!v || !breaks || (n_breaks!=(len+1)) )
		return -1;

	for (i =0; i < len; i++){
	
		if(sum(v, len, 0, i) >= q )
			break;
		
    }

    L = breaks[i-1]; // the lower break of the interval
    c = (i>0)?sum(v, len, 0, i-1):0; // the cumulative freq of the previous class
   	h = breaks[i] - breaks[i-1]; // width of the interval
   	f = v[i-1]; // frequency of the interval
   	quant = L + h/f *(q-c); 
   

    // L = breaks[i]; // the lower break of the interval
    // c = (i>0)?sum(v, len, 0, i-1):0; // the cumulative freq of the previous class
   	// h = breaks[i+1] - breaks[i]; // width of the interval
   	// f = v[i]; // frequency of the interval
   	// quant = L + h/f *(q-c); 
   
    return quant;
}



// # v is a vector of probabilities
// # breaks are the discretization breaks
// # length of the breaks should be length(v)+1
double variance(unsigned char* v, int len, double* breaks, int n_breaks){

	int i;
	double center, mean=0, diff=0;

	for (i =0; i < len; i++){
		center = (breaks[i] + breaks[i+1])/2;
		mean += (center*v[i]/100);
	}
	for (i =0; i < len; i++){
		center = (breaks[i] + breaks[i+1])/2;
		diff += pow((center-mean), 2) * v[i] / 100;
	}
	return diff;
}

double custom_std_dev(unsigned char* v, int len, double* breaks, int n_breaks){
	return sqrt(variance(v, len, breaks, n_breaks));
}

