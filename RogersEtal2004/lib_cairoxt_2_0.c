/*******************************************************************************

    File:       lib_cairoxt_2_0.c
    Contents:   Cairo-based library for drawing tables
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:

*******************************************************************************/

typedef enum {FALSE, TRUE} Boolean;

/******** Include files: ******************************************************/

#include "lib_cairoxt_2_0.h"
#include "lib_string.h"

/*----------------------------------------------------------------------------*/

static void gfree(void *p)
{
    if (p != NULL) { free(p); }
}

/*----------------------------------------------------------------------------*/

static void table_column_destroy(TableColumn *column)
{
    if (column != NULL) {
        int j;
        gfree(column->title);
        column->title = NULL;
        for (j = 0; j < CXT_ROW_MAX; j++) {
            gfree(column->cell[j].val);
            column->cell[j].val = NULL;
        }
    }
}

void table_destroy(TableStruct *td)
{
    int i;

    if (td != NULL) {
        gfree(td->title);
        for (i = 0; i < CXT_COL_MAX; i++) {
            table_column_destroy(&(td->column[i]));
        }
        free(td);
    }
}

static void table_column_create(TableColumn *column)
{
    if (column != NULL) {
        int j;
        column->title = NULL;
        for (j = 0; j < CXT_ROW_MAX; j++) {
            column->cell[j].val = NULL;
            column->cell[j].style = PANGO_STYLE_NORMAL;
            column->cell[j].colour.red = 0.0;
            column->cell[j].colour.green = 0.0;
            column->cell[j].colour.blue = 0.0;
        }
    }
}

TableStruct *table_create()
{
    TableStruct *td = (TableStruct *)malloc(sizeof(TableStruct));
    int i;

    if (td != NULL) {
        td->title = NULL;
        for (i = 0; i < CXT_COL_MAX; i++) {
            table_column_create(&(td->column[i]));
        }
    }
    return(td);
}

void table_set_extent(TableStruct *td, int x, int y, int w, int h)
{
    if (td != NULL) {
        td->x = x;
        td->y = y;
        td->width = w;
        td->height = h;
    }
}

void table_set_margins(TableStruct *td, int left, int right, int upper, int lower)
{
    if (td != NULL) {
        td->margin_left = left;
        td->margin_right = right;
        td->margin_upper = upper;
        td->margin_lower = lower;
    }
}

void table_set_title(TableStruct *td, char *title)
{
    if (td != NULL) {
        gfree(td->title);
        td->title = string_copy(title);
    }
}

void table_set_dimensions(TableStruct *td, int rows, int columns)
{
    if (td != NULL) {
        if (rows > CXT_ROW_MAX) {
            fprintf(stderr, "WARNING: Table dimensions are too big (not enough rows)\n");
            td->rows = CXT_ROW_MAX;
        }
        else {
            td->rows = rows;
        }
        if (columns > CXT_COL_MAX) {
            fprintf(stderr, "WARNING: Table dimensions are too big (not enough columns)\n");
            td->columns = CXT_COL_MAX;
        }
        else {
            td->columns = columns;
        }
    }
}

void table_set_column_properties(TableStruct *td, int column, char *heading, PangoAlignment align, int width)
{
    if (td == NULL) { 
        fprintf(stderr, "WARNING: NULL table in %s\n", __FUNCTION__);
    }
    else if (column >= td->columns) {
        fprintf(stderr, "WARNING: Attempting to set properties of non-exitent column in %s\n", __FUNCTION__);
    }
    else {
        gfree(td->column[column].title);
        td->column[column].title = string_copy(heading);
        td->column[column].align = align;
        td->column[column].width = width;
    }
}

void table_cell_set_properties(TableStruct *td, int r, int c, GdkColor *colour, PangoStyle fs)
{
    td->column[c].cell[r].style = fs;
    td->column[c].cell[r].colour.red = colour->red;
    td->column[c].cell[r].colour.green = colour->green;
    td->column[c].cell[r].colour.blue = colour->blue;
}

void table_cell_set_value_from_string(TableStruct *td, int row, int column, char *value)
{
    if ((td != NULL) && (column <= td->columns) && (row <= td->rows)) {
        gfree(td->column[column].cell[row].val);
        td->column[column].cell[row].val = string_copy(value);
    }
}

void table_cell_set_value_from_double(TableStruct *td, int row, int column, double value)
{
    char buffer[32];

    if ((td != NULL) && (column <= td->columns) && (row <= td->rows)) {
        g_snprintf(buffer, 32, "%6.3f", value);
        gfree(td->column[column].cell[row].val);
        td->column[column].cell[row].val = string_copy(buffer);
    }
}

void table_cell_set_value_from_int(TableStruct *td, int row, int column, int value)
{
    char buffer[32];

    if ((td != NULL) && (column <= td->columns) && (row <= td->rows)) {
        g_snprintf(buffer, 32, "%d", value);
        gfree(td->column[column].cell[row].val);
        td->column[column].cell[row].val = string_copy(buffer);
    }
}

/******************************************************************************/

static void cairoxt_draw_title(cairo_t *cr, PangoLayout *layout, TableStruct *td)
{
    if ((td != NULL) && (td->title != NULL)) {
        CairoxTextParameters tp;

        pangox_layout_set_font_size(layout, 14);
        cairox_text_parameters_set(&tp, td->width*0.5, td->margin_upper-2, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, td->title);
    }
}

static void cairoxt_draw_column(cairo_t *cr, PangoLayout *layout, TableColumn *column, int x, int y, int rows)
{
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    int y0, y1;
    int j;
    int pad = 5;

    y0 = y; y1 = y;
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    pangox_layout_set_font_size(layout, 12);
    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);
    cairox_paint_line(cr, &lp, x, y0, x+column->width, y0);

    // The column's heading:
    y1 = y1 + 20;
    if (column->title != NULL) {
        pangox_layout_set_font_style(layout, PANGO_STYLE_ITALIC);
        cairox_text_parameters_set(&tp, x+column->width/2, y1-3, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &tp, layout, column->title);
        pangox_layout_set_font_style(layout, PANGO_STYLE_NORMAL);
    }
    cairox_paint_line(cr, &lp, x, y1, x+column->width, y1);

    // The column's rows:
    for (j = 0; j < rows; j++) {
        y1 = y1 + 20;
        if (column->cell[j].val != NULL) {
            if (column->align == PANGO_ALIGN_LEFT) {
                cairox_text_parameters_set(&tp, x+pad, y1-3, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
            }
            else if (column->align == PANGO_ALIGN_RIGHT) {
                cairox_text_parameters_set(&tp, x+column->width-pad, y1-3, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_BOTTOM, 0.0);
            }
            else {
                cairox_text_parameters_set(&tp, x+column->width/2, y1-3, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            }
            cairox_text_parameters_set_foreground(&tp, column->cell[j].colour.red, column->cell[j].colour.green, column->cell[j].colour.blue);
            pangox_layout_set_font_style(layout, column->cell[j].style);
            cairox_paint_pango_text(cr, &tp, layout, column->cell[j].val);
            pangox_layout_set_font_style(layout, PANGO_STYLE_NORMAL);
        }
        cairox_paint_line(cr, &lp, x, y1, x+column->width, y1);
    }

    // Vertical lines around the column:
    cairox_paint_line(cr, &lp, x, y0, x, y1);
    cairox_paint_line(cr, &lp, x+column->width, y0, x+column->width, y1);

}

void cairox_draw_table(cairo_t *cr, TableStruct *td)
{
    if (cr == NULL) {
        fprintf(stderr, "NULL cr in %s\n", __FUNCTION__);
    }
    else if (td == NULL) {
        fprintf(stderr, "NULL td in %s\n", __FUNCTION__);
    }
    else {
        PangoLayout *layout = pango_cairo_create_layout(cr);
        int i, w, x, y;

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_rectangle(cr, td->x, td->y, td->width, td->height);
        cairo_fill(cr);

        w = 0;
        for (i = 0; i < td->columns; i++) {
            w += td->column[i].width;
        }

        x = (td->width - w) / 2;
        y = td->margin_upper;
        cairoxt_draw_title(cr, layout, td);
        for (i = 0; i < td->columns; i++) {
            cairoxt_draw_column(cr, layout, &td->column[i], x, y, td->rows);
            x = x + td->column[i].width;
        }
        g_object_unref(layout);
    }
}

/******************************************************************************/
