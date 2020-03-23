#include "tyler.h"
#include "lib_maths.h"

void world_set_network_input_vector(double *vector, PatternList *training_set, int p)
{
    PatternList *this = training_set;
    int i;

    while ((this != NULL) && (--p > 0)) {
        this = this->next;
    }
    if (this != NULL) {
        for (i = 0; i < IO_WIDTH; i++) {
            vector[i] = this->vector_in[i];
        }
    }
}

PatternList *world_decode_vector(PatternList *training_set, double *vector)
{
    PatternList *this = training_set;

    if ((this == NULL) || (this->next == NULL)) {
        return(this);
    }
    else {
        PatternList *result = this, *tmp;
        double d = vector_sum_square_difference(IO_WIDTH, this->vector_in, vector);

        for (tmp = this->next; tmp != NULL; tmp = tmp->next) {
            double d1 = vector_sum_square_difference(IO_WIDTH, tmp->vector_in, vector);
            if (d1 < d) {
                result = tmp;
                d = d1;
            }
        }
        return(result);
    }
}

Boolean pattern_is_animal1(PatternList *p)
{
    return(p->vector_in[12] > 0.5);
}

Boolean pattern_is_animal2(PatternList *p)
{
    return(p->vector_in[13] > 0.5);
}

Boolean pattern_is_animal(PatternList *p)
{
    return(pattern_is_animal1(p) || pattern_is_animal2(p));
}

Boolean pattern_is_artifact1(PatternList *p)
{
    return(p->vector_in[8] > 0.5);
}

Boolean pattern_is_artifact2(PatternList *p)
{
    return(p->vector_in[9] > 0.5);
}

Boolean pattern_is_artifact(PatternList *p)
{
    return(pattern_is_artifact1(p) || pattern_is_artifact2(p));
}

Boolean response_is_correct(PatternList *training_set, PatternList *input, double *r)
{
    PatternList *result;

    result = world_decode_vector(training_set, r);
    return(result == input);
}

Boolean response_is_animal1(PatternList *training_set, double *r)
{
    return(pattern_is_animal1(world_decode_vector(training_set, r)));
}

Boolean response_is_animal2(PatternList *training_set, double *r)
{
    return(pattern_is_animal2(world_decode_vector(training_set, r)));
}

Boolean response_is_artifact1(PatternList *training_set, double *r)
{
    return(pattern_is_artifact1(world_decode_vector(training_set, r)));
}

Boolean response_is_artifact2(PatternList *training_set, double *r)
{
    return(pattern_is_artifact2(world_decode_vector(training_set, r)));
}

double response_error(PatternList *input, double *result, FeatureType ft)
{
    int offset = 0, i;
    double e;

    switch (ft) {
        case F_DISTINCTIVE: {
            offset = 0;
            break;
        }
        case F_SHARED: {
            offset = 8;
            break;
        }
        case F_FUNCTIONAL: {
            offset = 16;
            break;
        }
    }

    e = 0.0;
    for (i = 0; i < 8; i++) {
        e += fabs(input->vector_out[offset+i]-result[offset+i]);
    }
    return(e / 8.0);
}

