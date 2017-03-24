#ifndef _lib_cairoxt_2_0_h_

#define _lib_cairoxt_2_0_h_

#include "lib_cairox.h"

// Maximum of 50 rows and 10 columns
#define CXT_ROW_MAX 50
#define CXT_COL_MAX 10

typedef struct table_cell {
    char *val;
    PangoStyle style;
    GdkColor colour;
} TableCell;

typedef struct table_column {
    char *title;
    TableCell cell[CXT_ROW_MAX];
    PangoAlignment align;
    int width;
} TableColumn;

typedef struct table_data {
    int x;
    int y;
    int width;
    int height;
    int margin_left;
    int margin_right;
    int margin_upper;
    int margin_lower;
    char *title;
    int rows;
    int columns;
    TableColumn column[CXT_COL_MAX];
} TableStruct;

extern void table_destroy(TableStruct *gd);
extern TableStruct *table_create();

extern void table_set_extent(TableStruct *gd, int x, int y, int  w, int h);
extern void table_set_margins(TableStruct *gd, int left, int right, int upper, int lower);
extern void table_set_title(TableStruct *gd, char *title);
extern void table_set_dimensions(TableStruct *td, int rows, int columns);
extern void table_set_column_properties(TableStruct *td, int column, char *heading, PangoAlignment align, int width);
extern void table_cell_set_properties(TableStruct *td, int row, int column, GdkColor *colour, PangoStyle font_style);
extern void table_cell_set_value_from_string(TableStruct *td, int row, int column, char *value);
extern void table_cell_set_value_from_int(TableStruct *td, int row, int column, int value);
extern void table_cell_set_value_from_double(TableStruct *td, int row, int column, double value);

extern void cairox_draw_table(cairo_t *cr, TableStruct *gd);

#endif


