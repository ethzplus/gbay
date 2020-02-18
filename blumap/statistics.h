#ifndef __STATISTICS_H__
#define __STATISTICS_H__

double entropy(unsigned char* vector, int len);
double mean(unsigned char* v, int len, double* breaks, int n_breaks);
double quantile(unsigned char* v, int len, double* breaks, int n_breaks, int q);
double variance(unsigned char* v, int len, double* breaks, int n_breaks);
double custom_std_dev(unsigned char* v, int len, double* breaks, int n_breaks);

#endif