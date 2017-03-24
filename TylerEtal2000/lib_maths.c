/*******************************************************************************

    File:       lib_maths.c
    Contents:   Miscellaneous mathematical / vector functions.
    Author:     Rick Cooper
    Copyright (c) 2004 - 2016 Richard P. Cooper

    Public procedures:
        double random_uniform(double low, double high);
        double random_normal(double mean, double sd);
        int    random_int(int n);
        double squared(double input);
        double sigmoid_inverse(double input);
        double sigmoid(double input);
        double sigmoid_derivative(double input);
        double vector_length(int n, double *a);
        double vector_sum_square_difference(int w, double *v1, double *v2);
        double vector_rms_difference(int w, double *v1, double *v2);
        double vector_cross_entropy(int w, double *v1, double *v2);
        double vector_soft_max(int w, double *v1, double *v2);
        double vector_cosine(int n, double *a, double *b);
        double vector_correlation(int n, double *a, double *b);
        double euclidean_distance(int n, double *a, double *b);
        double jaccard_distance(int n, double *a, double *b);
        double vector_set_variability(int n, int l, double *v);

*******************************************************************************/
/******** Include files: ******************************************************/

#include <math.h>
#include <stdlib.h>  // Defines RAND_MAX
#include <float.h>
#include "lib_maths.h"

/******************************************************************************/

double random_normal(double mean, double sd)
{
    // This generates a random integer with given mean and standard deviation

    double r1 = (rand() + 1.0) / (RAND_MAX + 1.0);
    double r2 = (rand() + 1.0) / (RAND_MAX + 1.0);

    return(mean + sd * sqrt(-2 * log(r1)) * cos(6.2831853 * r2));
}

double random_uniform(double low, double high)
{
    double r = (rand() + 1.0) / (RAND_MAX + 1.0);

    return(low + (r * (high - low)));
}

int random_int(int n)
{
    // This generates a random integer in the range [0, n)

    return(n * (rand() + 1.0) / (RAND_MAX + 1.0));
}

double squared(double input)
{
    return(input * input);
}

#define REALLY_SMALL 0.00000001

static double a_log_b(double a, double b)
{
    if (a == 0) {
        return(0);
    }
    else if (b < REALLY_SMALL) {
// fprintf(stdout, "WARNING: UNDEFINED a_log_b: %f log(%f) \n", a, b);
        return(a * log(REALLY_SMALL));
    }
    else {
        return(a * log(b));
    }
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
        if (v1[w] >= 0) {
            r += squared(v1[w] - v2[w]);
        }
    }
    return(r);
}


double vector_cross_entropy(int w, double *desired, double *actual)
{
    double r = 0.0;
    while (w-- > 0) {
        if (desired[w] >= 0) {
            r -= a_log_b(desired[w], actual[w]) + a_log_b(1-desired[w], 1-actual[w]);
        }
    }
    return(r);
}

/* New??
double vector_cross_entropy(int w, double *desired, double *actual)
{
    double r = 0.0;
    while (w-- > 0) {
        r -= a_log_a_over_b(desired[w], actual[w]) + a_log_a_over_b(1-desired[w], 1-actual[w]);
    }
    return(r);
}
*/

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

double vector_correlation(int n, double *x, double *y)
{
    double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    int i;

    for (i = 0; i < n; i++) {
        sx += x[i];
        sxx += x[i]*x[i];
        sy += y[i];
        syy += y[i]*y[i];
        sxy += x[i]*y[i];
    }
    return((n*sxy-sx*sy) / sqrt((n*sxx-sx*sx)*(n*syy-sy*sy)));
}

double vector_cosine(int n, double *x, double *y)
{
    double sxx = 0, syy = 0, sxy = 0;
    int i;

    for (i = 0; i < n; i++) {
        sxx += x[i]*x[i];
        syy += y[i]*y[i];
        sxy += x[i]*y[i];
    }
    return(sxy / sqrt(sxx*syy));
}

/******************************************************************************/

double euclidean_distance(int n, double *a, double *b)
{
    double sum = 0;
    while (n-- > 0) {
        sum = sum + (a[n] - b[n]) * (a[n] - b[n]);
    }
    return(sqrt(sum));
}

double jaccard_distance(int n, double *a, double *b)
{
    int num = 0, denom = 0;

    while (n-- > 0) {
        if ((a[n] > 0.5) && (b[n] > 0.5)) {
            num++;
        }
        if ((a[n] > 0.5) || (b[n] > 0.5)) {
            denom++;
        }
    }
    if (denom == 0) {
        return(0.0);
    }
    else {
        return(1.0 - num / (double) denom);
    }
}

/******************************************************************************/
