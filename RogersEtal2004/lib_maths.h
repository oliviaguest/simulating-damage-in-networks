#ifndef _lib_maths_h_

#define _lib_maths_h_

extern double random_uniform(double low, double high);
extern double random_normal(double mean, double sd);
extern int    random_int(int n);
extern double squared(double input);
extern double sigmoid_inverse(double input);
extern double sigmoid(double input);
extern double sigmoid_derivative(double input);
extern double vector_length(int n, double *a);
extern double vector_sum_square_difference(int w, double *v1, double *v2);
extern double vector_rms_difference(int w, double *v1, double *v2);
extern double vector_cross_entropy(int w, double *v1, double *v2);
extern double vector_soft_max(int w, double *v1, double *v2);
extern double vector_cosine(int n, double *a, double *b);
extern double vector_correlation(int n, double *a, double *b);
extern double euclidean_distance(int n, double *a, double *b);
extern double jaccard_distance(int n, double *a, double *b);
extern double vector_set_variability(int n, int l, double *v);

#endif
