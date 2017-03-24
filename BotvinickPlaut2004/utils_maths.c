// #include "bp.h"
#include <stdlib.h>
#include <math.h>

double random_normal(double mean, double sd)
{
    // This generates a random integer with given mean and standard deviation

    double r1 = rand() / (double) RAND_MAX;
    double r2 = rand() / (double) RAND_MAX;

    return(mean + sd * sqrt(-2 * log(r1)) * cos(6.2831853 * r2));
}

double random_uniform(double low, double high)
{
    double r = rand() / (double) RAND_MAX;

    return(low + (r * (high - low)));
}

int random_int(int n)
{
    // This generates a random integer in the range [0, n)

    return(n * (rand() / (double) RAND_MAX));
}

double squared(double input)
{
    return(input * input);
}

static double a_log_b(double a, double b)
{
    return(a * log(b));
}

/* New??
static double a_log_a_over_b(double a, double b)
{
    return(a * log(a / b));
}
*/

double sigmoid(double input)
{
    return(1.0 / (1.0 + exp(-1.0 * input)));
}

double sigmoid_derivative(double input)
{
    double y = sigmoid(input);
    return(y * (1.0 - y));
}

double vector_sum_square_difference(int w, double *v1, double *v2)
{
    double r = 0.0;
    while (w-- > 0) {
        r += squared(v1[w] - v2[w]);
    }
    return(r);
}

double vector_rms_difference(int w, double *v1, double *v2)
{
    double r = 0.0;
    int i;

    for (i = 0; i < w; i++) {
        r += squared(v1[i] - v2[i]);
    }
    return(sqrt(r / (double) w));
}

double vector_cross_entropy(int w, double *desired, double *actual)
{
    double r = 0.0;
    while (w-- > 0) {
        r -= a_log_b(desired[w], actual[w]) + a_log_b(1-desired[w], 1-actual[w]);
// Alternative calculation sometimes used ... CHECK?
//        r -= a_log_a_over_b(desired[w], actual[w]) + a_log_a_over_b(1-desired[w], 1-actual[w]);
    }
    return(r);
}

double vector_set_variability(int n, int l, double *v)
{
    /* Variability in a set of n vectors, each with l components,   */
    /* located from address v.                                      */

    double variability = 0.0;
    int i, j, count = 0;

    if (n > 1) {
        for (i = 0; i < n-1; i++) {
            for (j = i+1; j < n; j++) {
                variability += sqrt(vector_sum_square_difference(l, &v[i*l], &v[j*l]));
                count++;
            }
        }
    }
    return(variability / (double) count);
}

/******************************************************************************/
