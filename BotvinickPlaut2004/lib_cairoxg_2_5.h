#ifndef _lib_cairoxg_2_5_h_

#define _lib_cairoxg_2_5_h_

#include "lib_cairox_2_0.h"

// Maximum number of points in a dataset:
#define CXG_POINT_MAX 2501

typedef struct graph_data_set {
    double x[CXG_POINT_MAX];
    double y[CXG_POINT_MAX];
    double dy1[CXG_POINT_MAX];
    double dy2[CXG_POINT_MAX];
    char *label;
    int points;
    int bar_width;
    double r;
    double g;
    double b;
    LineStyle style;
    MarkerType mark;
} GraphDataSet;

typedef struct graph_axis {
    char *label;
    double min;
    double max;
    char *format;
    char **tick_labels;
    int ticks;
    CairoxFontProperties font;
} GraphAxis;

typedef struct graph_struct {
    int x, y, w, h;
    int margin_left;
    int margin_right;
    int margin_upper;
    int margin_lower;
    int stack_bars;
    Boolean show_values;
    Boolean legend_show;
    char *legend_label;
    double legend_x;
    double legend_y;
    int datasets;
    GraphDataSet *dataset;
    char *title;
    CairoxFontProperties title_font;
    CairoxFontProperties legend_font;
    GraphAxis axis[2];
} GraphStruct;

extern void graph_destroy(GraphStruct *gd);
extern GraphStruct *graph_create(int datasets);

extern void graph_set_extent(GraphStruct *gd, int x, int y, int w, int h);
extern void graph_set_margins(GraphStruct *gd, int left, int right, int upper, int lower);
extern void graph_set_title(GraphStruct *gd, char *title);
extern void graph_set_title_font_properties(GraphStruct *gd, CairoxFontProperties *fp);
extern void graph_set_axis_tick_marks(GraphStruct *gd, GtkOrientation orientation, int i, char **labels);
extern void graph_set_axis_properties(GraphStruct *gd, GtkOrientation orientation, double min, double max, int ticks, char *format, char *label);
extern void graph_set_axis_font_properties(GraphStruct *gd, GtkOrientation orientation, CairoxFontProperties *fp);
extern void graph_set_dataset_properties(GraphStruct *gd, int l, char *label, double r, double g, double b, int bar_width, LineStyle style, MarkerType mark);
extern void graph_set_dataset_barwidth(GraphStruct *gd, int i, int bar_width);
extern void graph_set_legend_properties(GraphStruct *gd, Boolean show, double x, double y, char *label);
extern void graph_set_legend_font_properties(GraphStruct *gd, CairoxFontProperties *fp);
extern void graph_set_stack_bars(GraphStruct *gd, int stack);
extern void graph_set_show_values(GraphStruct *gd, Boolean show);

extern void cairox_draw_graph(cairo_t *cr, GraphStruct *gd, Boolean colour);

#endif

