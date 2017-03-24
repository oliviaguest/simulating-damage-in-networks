#ifndef _lib_dendrogram_1_0_h_

#define _lib_dendrogram_1_0_h_

typedef enum metric_type {METRIC_EUCLIDEAN, METRIC_JACCARD} MetricType;
typedef enum linkage_type {LT_MEAN, LT_MAX, LT_MIN} LinkageType;

typedef struct dendrotree_struct {
    double    x, x0, x1, y0; // Bounding box of this cluster when drawn
    char     *label;         // The label, if this is a leaf (or NULL)
    double   *vector;        // The vector, if this is a leaf (or NULL)
    int       smi;           // An index to the similarity matrix, if this is a leaf
    double    distance;      // The distance for this cluster
    struct dendrotree_struct *left;  // The left tree (if this is not a leaf) or NULL
    struct dendrotree_struct *right; // The right tree (if this is not a leaf) or NULL
} DendroTreeStruct;

typedef struct dendrogram_struct {
    DendroTreeStruct *tree;
    int               vector_width;
    int               vector_count;
    MetricType        metric;
    LinkageType       lt;
} DendrogramStruct;

typedef struct named_vector_array {
    double            *vector;
    char              *name;
} NamedVectorArray;

extern void dendrogram_print(FILE *fp, DendrogramStruct *dendrogram);
extern void dendrogram_draw(cairo_t *cr, DendrogramStruct *dendrogram, CairoxFontProperties *font, double x, double y, int height);
extern DendrogramStruct *dendrogram_create(NamedVectorArray *list, int width, int count, MetricType metric, LinkageType lt);
extern void dendrogram_free(DendrogramStruct *dendrogram);
extern NamedVectorArray *named_vector_array_free(NamedVectorArray list[], int length);

#endif


