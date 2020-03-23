/*******************************************************************************

    File:       lib_dendrogram_1_0.c
    Contents:   Build and draw dendograms
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:
        void dendrogram_draw(cairo_t *cr, DendrogramStruct *tree, CairoxFontProperties *fp, double x, double y, int height)
        void dendrogram_print(FILE *fp, DendrogramStruct *tree)
        void dendrogram_free(DendrogramStruct *tree)
        DendrogramStruct *dendrogram_create(NamedVectorArray *list, int vector_width, int vector_count, MetricType metric, LinkageType  lt)
        NamedVectorArray *named_vector_array_free(NamedVectorArray list[], int length)

*******************************************************************************/

/******** Include files: ******************************************************/

typedef enum {FALSE, TRUE} Boolean;

#include "lib_string.h"
#include "lib_cairox.h"
#include <math.h> 
#include "lib_maths.h"
#include "lib_dendrogram_1_0.h"

/******************************************************************************/

void dendrogram_print_tree(FILE *fp, DendroTreeStruct *tree, int level)
{
    if (tree->vector == NULL) {
        fprintf(fp, "%*c%f\n", 2*level, ' ', tree->distance);
        dendrogram_print_tree(fp, tree->left, level+1);
        dendrogram_print_tree(fp, tree->right, level+1);
    }
    else {
        fprintf(fp, "%*c%s\n", 2*level, ' ', tree->label);
    }
}

/*----------------------------------------------------------------------------*/

void dendrogram_print(FILE *fp, DendrogramStruct *dendrogram)
{
    dendrogram_print_tree(fp, dendrogram->tree, 0);
}

/******************************************************************************/

static int dendrogram_max(DendroTreeStruct *tree)
{
    if (tree == NULL) {
        return(1);
    }
    else {
        return((int) ceil(tree->distance));
    }
}

/*----------------------------------------------------------------------------*/

static double y_pos_calculate(int range, double d, int canvas_height)
{
    return(canvas_height - 10 - d * (canvas_height - 20) / range);
}

/*----------------------------------------------------------------------------*/

static void dendrogram_draw_tree(cairo_t *cr, PangoLayout *layout, DendroTreeStruct *tree, double x, double y, double d_max, int canvas_height)
{
    CairoxLineParameters lp;

    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
    if (tree->label != NULL) {
        CairoxTextParameters tp;

        tree->x0 = x;
        tree->x1 = x + 14;
        tree->x = (tree->x0 + tree->x1) / 2.0;
        tree->y0 = y;

        cairox_paint_line(cr, &lp, tree->x, y, tree->x, y+10);
        cairox_text_parameters_set(&tp, x+7, y+12, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 90.0);
        cairox_paint_pango_text(cr, &tp, layout, tree->label);
    }
    else {
        double x0, x1, y0;

        tree->x0 = x;
        tree->y0 = y;

        y0 = y_pos_calculate(d_max, tree->distance, canvas_height);

        dendrogram_draw_tree(cr, layout, tree->left, x, y0, d_max, canvas_height);
        dendrogram_draw_tree(cr, layout, tree->right, tree->left->x1 + 1, y0, d_max, canvas_height);

        tree->x1 = tree->right->x1;

        x0 = tree->left->x;
        x1 = tree->right->x;
        tree->x = (x0 + x1) / 2.0;
        cairox_paint_line(cr, &lp, x0, y0, x1, y0);
        cairox_paint_line(cr, &lp, tree->x, tree->y0, tree->x, y0);
    }
}

/*----------------------------------------------------------------------------*/

static void dendrogram_draw_axis(cairo_t *cr, PangoLayout *layout, DendrogramStruct *dendrogram, double d_max, int canvas_height)
{
    char buffer[16];
    double y_min, y_max, y;
    double d_min = 0.0;
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    int i;

    y_min = y_pos_calculate(d_max, d_min, canvas_height);
    y_max = y_pos_calculate(d_max, d_max, canvas_height);

    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
    cairox_paint_line(cr, &lp, 40, y_min, 40, y_max);
    for (i = 0; i <= 5; i++) {
        y = y_min + ((y_max - y_min) * i / 5.0);
        g_snprintf(buffer, 16, "%3.1f", d_min + (d_max - d_min) * i / 5.0);
        cairox_paint_line(cr, &lp, 38, y, 42, y);
        cairox_text_parameters_set(&tp, 36, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, buffer);
    }

    cairox_text_parameters_set(&tp, 16, (y_max + y_min) * 0.5, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 90.0);

    if (dendrogram->metric == METRIC_EUCLIDEAN) {
        cairox_paint_pango_text(cr, &tp, layout, "Euclidean Distance");
    }
    else if (dendrogram->metric == METRIC_JACCARD) {
        cairox_paint_pango_text(cr, &tp, layout, "Jaccard Distance");
    }
}

/*----------------------------------------------------------------------------*/

void dendrogram_draw(cairo_t *cr, DendrogramStruct *dendrogram, CairoxFontProperties *fp, double x, double y, int height)
{
    double d_max = dendrogram_max(dendrogram->tree);
    PangoLayout *layout;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    layout = pango_cairo_create_layout(cr);    
    pangox_layout_set_font_properties(layout, fp);
    dendrogram_draw_axis(cr, layout, dendrogram, d_max, height);
    if (dendrogram->tree != NULL) {
        dendrogram_draw_tree(cr, layout, dendrogram->tree, x, y, d_max, height);
    }
    g_object_unref(layout);
}

/******************************************************************************/

static int sm_index(int vector_count, int i, int j)
{
    // INDEX(i, j) where j > i
    // if i is 0 then (j - 1) [so (0,47) gives 46]
    // if i is 1 then (j - 1) + (vector_count - 2) [so (1,2) gives 47, and (1, 47) gives 92]
    // if i is 2 then (j - 1) + (vector_count - 2) + (vector_count - 3) [so (2, 3) gives 93 and (2, 47) gives 137]
    // if i is 3 then (j - 1) + (vector_count - 2) + (vector_count - 3) + (vector_count - 4) [so(3, 4) gives 138]
    // equivalently:
    // if i is 0 then j - vector_count + (vector_count - 1)
    // if i is 1 then j - vector_count + (vector_count - 1) + (vector_count - 2)
    // if i is 2 then j - vector_count + (vector_count - 1) + (vector_count - 2) + (vector_count - 3)
    // if i is 3 then j - vector_count + (vector_count - 1) + (vector_count - 2) + (vector_count - 3) + (vector_count - 4)
    // More generally:
    // index = j - vector_count + (i + 1) * vector_count - [(i + 1) * (i + 2) / 2]
    //       = j + (i * vector_count) - [(i + 1) * (i + 2) / 2]

    // examples:
    //     (0, 1) => 1 - 1 = 0
    //     (0, 47) => 47 - 1 = 46
    //     (1, 2) => 2 + 48 - 3 = 47
    //     (3, 4) => 4 + 144 - 10 = 138
    //     (46, 47) => 47 + 46*48-[47*48/2] = 47 + 2208 - 1128 = 1127

    if (j > i) {
        return(j + i * vector_count - (i+1)*(i+2)/2);
    }
    else {
        return(-1);
    }
}

static void sm_set(double *sm, int vector_count, int i, int j, double d)
{
    int index;

    if ((index = sm_index(vector_count, i, j)) < 0) {
        fprintf(stdout, "WARNING: Ignoring invalid index into similarity matrix\n");
    }
    else {
        sm[index] = d;
    }
}

static double sm_get(double *sm, int vector_count, int i, int j)
{
    int index;

    if (j > i) {
        index = sm_index(vector_count, i, j);
    }
    else {
        index = sm_index(vector_count, j, i);
    }

    if (index < 0) {
        fprintf(stdout, "WARNING: Invalid index into similarity matrix\n");
        return(0.0);
    }
    else {
        return(sm[index]);
    }
}

/*----------------------------------------------------------------------------*/

static double *similarity_matrix_calculate(NamedVectorArray *list, int vector_width, int vector_count, MetricType metric)
{
    int i, j, n;
    double *sm;

    n = (vector_count * (vector_count - 1)) / 2;

    if ((sm = (double *)malloc(n * sizeof(double))) != NULL) {
        for (i = 0; i < vector_count; i++) {
            for (j = i + 1; j < vector_count; j++) {
                double d = 0.0;
                if (metric == METRIC_EUCLIDEAN) {
                    d = euclidean_distance(vector_width, list[i].vector, list[j].vector);
                }
                else if (metric == METRIC_JACCARD) {
                    d = jaccard_distance(vector_width, list[i].vector, list[j].vector);
                }
                sm_set(sm, vector_count, i, j, d);
            }
	}
    }
    return(sm);
}

/*----------------------------------------------------------------------------*/

static double subtree_distance(DendroTreeStruct *s1, DendroTreeStruct *s2, LinkageType lt, double *sm, int vector_count)
{
    // Calculate the distance between two dendrogram trees
    double d = 0.0;

    if ((s1->vector != NULL) && (s2->vector != NULL)) {
        // We have two leaves - just look up the similarity table
        d = sm_get(sm, vector_count, s1->smi, s2->smi);
    }
    else if ((s1->vector != NULL) && (s2->vector == NULL)) {
        // s1 is a leaf but s2 isn't - apply the linkage to the two subtrees
        if (lt == LT_MAX) {
            d = subtree_distance(s1, s2->left, lt, sm, vector_count);
            d = MAX(d, subtree_distance(s1, s2->right, lt, sm, vector_count));
        }
        else if (lt == LT_MIN) {
            d = subtree_distance(s1, s2->left, lt, sm, vector_count);
            d = MIN(d, subtree_distance(s1, s2->right, lt, sm, vector_count));
        }
        else if (lt == LT_MEAN) {
            d = subtree_distance(s1, s2->left, lt, sm, vector_count);
            d += subtree_distance(s1, s2->right, lt, sm, vector_count);
            d = d / 2.0;
        }
    }
    else if ((s1->vector == NULL) && (s2->vector != NULL)) {
        // s2 is a leaf but s1 isn't - apply the linkage to the two subtrees
        if (lt == LT_MAX) {
            d = subtree_distance(s1->left, s2, lt, sm, vector_count);
            d = MAX(d, subtree_distance(s1->right, s2, lt, sm, vector_count));
        }
        else if (lt == LT_MIN) {
            d = subtree_distance(s1->left, s2, lt, sm, vector_count);
            d = MIN(d, subtree_distance(s1->right, s2, lt, sm, vector_count));
        }
        else if (lt == LT_MEAN) {
            d = subtree_distance(s1->left, s2, lt, sm, vector_count);
            d += subtree_distance(s1->right, s2, lt, sm, vector_count);
            d = d / 2.0;
        }
    }
    else if ((s1->vector == NULL) && (s2->vector == NULL)) {
        // Both s1 and s2 are proper subtrees
        if (lt == LT_MAX) {
            d = subtree_distance(s1->left, s2->left, lt, sm, vector_count);
            d = MAX(d, subtree_distance(s1->left, s2->right, lt, sm, vector_count));
            d = MAX(d, subtree_distance(s1->right, s2->left, lt, sm, vector_count));
            d = MAX(d, subtree_distance(s1->right, s2->right, lt, sm, vector_count));
        }
        else if (lt == LT_MIN) {
            d = subtree_distance(s1->left, s2->left, lt, sm, vector_count);
            d = MIN(d, subtree_distance(s1->left, s2->right, lt, sm, vector_count));
            d = MIN(d, subtree_distance(s1->right, s2->left, lt, sm, vector_count));
            d = MIN(d, subtree_distance(s1->right, s2->right, lt, sm, vector_count));
        }
        else if (lt == LT_MEAN) {
            d = subtree_distance(s1->left, s2->left, lt, sm, vector_count);
            d += subtree_distance(s1->left, s2->right, lt, sm, vector_count);
            d += subtree_distance(s1->right, s2->left, lt, sm, vector_count);
            d += subtree_distance(s1->right, s2->right, lt, sm, vector_count);
            d = d / 4.0;
        }
    }
    else {
        d = 1.3023932;
    }
    return(d);
}

/*----------------------------------------------------------------------------*/

static double get_nearest_neighbours(GList *subtrees, LinkageType lt, double *sm, int vector_count, DendroTreeStruct **s1, DendroTreeStruct **s2)
{
    double d_min = DBL_MAX, d;
    GList *l1, *l2;

    *s1 = NULL;
    *s2 = NULL;

    for (l1 = subtrees; l1 != NULL; l1 = g_list_next(l1)) {
        for (l2 = g_list_next(l1); l2 != NULL; l2 = g_list_next(l2)) {
            d = subtree_distance(l1->data, l2->data, lt, sm, vector_count);
            if (d < d_min) {
                d_min = d;
                *s1 = l1->data;
                *s2 = l2->data;
            }
        }
    }

    return(d_min);
}

/*============================================================================*/

static DendroTreeStruct *tree_node_create(DendroTreeStruct *left, DendroTreeStruct *right, double d)
{
    DendroTreeStruct *new = (DendroTreeStruct *)malloc(sizeof(DendroTreeStruct));

    if (new != NULL) {
        new->left = left;
        new->right = right;
        new->label = NULL;
        new->distance = d;
        new->vector = NULL;
    }
    return(new);
}

/*----------------------------------------------------------------------------*/

static DendroTreeStruct *dendrogram_leaf_create(char *label, double *vector, int length, int smi)
{
    DendroTreeStruct *new;
    double *nv;
    int i;

    if ((nv = (double *)malloc(length*sizeof(double))) == NULL) {
        return(NULL);
    }
    else if ((new = (DendroTreeStruct *)malloc(sizeof(DendroTreeStruct))) == NULL) {
        free(vector);
        return(NULL);
    }
    else {
        new->left = NULL;
        new->right = NULL;
        new->label = string_copy(label);
        new->distance = 0.0;
        for (i = 0; i < length; i++) {
            nv[i] = vector[i];
        }
        new->vector = nv;
        new->smi = smi;
        return(new);
    }
}

/*----------------------------------------------------------------------------*/

static GList *dendrogram_build_leaves(NamedVectorArray *list, int vector_width, int vector_count)
{
    DendroTreeStruct *leaf;
    GList *leaves = NULL;
    int smi = 0;
    int i;

    for (i = 0; i < vector_count; i++) {
        leaf = dendrogram_leaf_create(list[i].name, list[i].vector, vector_width, smi++);
        leaves = g_list_append(leaves, leaf);
    }
    return(leaves);
}

/*============================================================================*/  

DendrogramStruct *dendrogram_create(NamedVectorArray *list, int vector_width, int vector_count, MetricType metric, LinkageType  lt)
{
    DendrogramStruct *new;
    GList *subtrees = NULL;
    DendroTreeStruct *s1, *s2, *tree = NULL;
    double *sm, d;

    if ((new = (DendrogramStruct *)malloc(sizeof(DendrogramStruct))) == NULL) {
        return(NULL);
    }
    else {
        if (list != NULL) {
            sm = similarity_matrix_calculate(list, vector_width, vector_count, metric);
            // Make a list of dendrogram leaves, maintainint SM indices
            subtrees = dendrogram_build_leaves(list, vector_width, vector_count);
            // Now repeat merging the subtrees until we have just one tree left:
            while (g_list_length(subtrees) > 1) {
                // Get the two nearest subtrees:
                d = get_nearest_neighbours(subtrees, lt, sm, vector_count, &s1, &s2);
                // Remove those subtrees from the subtree list
                subtrees = g_list_remove(subtrees, s1);
                subtrees = g_list_remove(subtrees, s2);
                // Create a new node that dominates s1 and s2
                tree = tree_node_create(s1, s2, d);
                // Append the new node to the leaf list:
                subtrees = g_list_append(subtrees, tree);
            }
            tree = subtrees->data;
            g_list_free(subtrees);
            free(sm);
        }
        new->tree = tree;
        new->vector_width = vector_width;
        new->vector_count = vector_count;
        new->metric = metric;
        new->lt = lt;
    }
    return(new);
}

/******************************************************************************/

void dendrotree_free(DendroTreeStruct *tree)
{
    if (tree != NULL) {
        dendrotree_free(tree->left);
        dendrotree_free(tree->right);
        string_free(tree->label);
        if (tree->vector != NULL) {
            free(tree->vector);
        }
        free(tree);
    }
}

/*----------------------------------------------------------------------------*/

void dendrogram_free(DendrogramStruct *dendrogram)
{
    if (dendrogram != NULL) {
        free(dendrogram->tree);
        free(dendrogram);
    }
}

/******************************************************************************/

NamedVectorArray *named_vector_array_free(NamedVectorArray list[], int length)
{
    int i;

    if (list != NULL) {
        for (i = 0; i < length; i++) {
            if (list[i].vector != NULL) {
                string_free(list[i].name);
                free(list[i].vector);
            }
        }
        free(list);
    }
    return(NULL);
}

/******************************************************************************/
