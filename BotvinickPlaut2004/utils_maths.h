
extern double squared(double input);
extern double sigmoid(double input);
extern double sigmoid_derivative(double input);

extern double random_uniform(double low, double high);
extern double random_normal(double mean, double sd);
extern int random_int(int n);

extern double vector_sum_square_difference(int w, double *v1, double *v2);
extern double vector_rms_difference(int w, double *v1, double *v2);
extern double vector_cross_entropy(int w, double *v1, double *v2);
extern double vector_set_variability(int n, int l, double *v);

