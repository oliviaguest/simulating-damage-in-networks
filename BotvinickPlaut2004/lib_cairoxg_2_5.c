/*******************************************************************************

    File:       lib_cairoxg_2_5.c
    Contents:   Cairo-based library for drawing line/bar graphs
    Author:     Rick Cooper
    Copyright (c) 2016/2018 Richard P. Cooper

    Public procedures:
        void graph_destroy(GraphStruct *gd)
        GraphStruct *graph_create(int num_ds)
        void graph_set_extent(GraphStruct *gd, int x, int y, int w, int h)
        void graph_set_margins(GraphStruct *gd, int left, int right, int upper, int lower)
        void graph_set_title(GraphStruct *gd, char *title)
        void graph_set_axis_tick_marks(GraphStruct *gd, GtkOrientation orientation, int n, char **labels)
        void graph_set_axis_properties(GraphStruct *gd, GtkOrientation orientation, double min, double max, int ticks, char *format, char *label)
        void graph_set_legend_properties(GraphStruct *gd, Boolean show, double x, double y, char *label)
        void graph_set_dataset_properties(GraphStruct *gd, int l, char *label, double r, double g, double b, int bar_width, LineStyle style, MarkerType mark)
        void graph_set_dataset_barwidth(GraphStruct *gd, int i, int bar_width);
        void graph_set_axis_font_properties(GraphStruct *gd, GtkOrientation orientation, CairoxFontProperties *fp)
        void graph_set_title_font_properties(GraphStruct *gd, CairoxFontProperties *fp)
        void graph_set_legend_font_properties(GraphStruct *gd, CairoxFontProperties *fp)
        void graph_set_stack_bars(GraphStruct *gd, int stack)
        void graph_set_show_values(GraphStruct *gd, Boolean show)
        void cairox_draw_graph(cairo_t *cr, GraphStruct *gd, Boolean colour)

    TO DO: 
      * Do away with the restriction on number of points per datasets and
        the symbolic constant CXG_POINT_MAX 
      * Improve interface for setting axis tick labels

*******************************************************************************/

typedef enum {FALSE, TRUE} Boolean;

/******** Include files: ******************************************************/

#include <math.h>
#include "lib_cairoxg_2_5.h"
#include "lib_string.h"

/* History: ********************************************************************

Version 2.0:
22/07/2016: Added x/y  coordinates for graphs, in addition to width and height
22/07/2016: Changed line related stuff to "dataset"
22/07/2016: Added bar_width property to datasets. If greater than zero then
            show the data as a bar of the given width. Otherwise present it
            as a line

Version 2.1:
10/11/2016: Merged improved legend drawing from a different branch of the
            source tree
10/11/2016: Fixed initialisation bug in setting initial tickmark labels
10/11/2016: Fixed bug in locating lower-limit of bar chart data

Version 2.2:
23/11/2016: Do away with the restriction on number of datasets per graph and
            the symbolic constant CXG_LINE_MAX 
24/11/2016: Allow setting of title / axis /legend font size, colour, style, etc.
23/02/2017: Improve legend positioning and don't draw legend entries if no data

Version 2.3:
26/02/2017: Add graph properties to allow showing of values
27/02/2017: Allow stacked bar charts
23/08/2017: Added default extent / margins to ensure a poorly initialised
            graph appears when drawn
23/08/2017: Fixed default behaviour when axes are not labelled
23/08/2017: Fixed colour handling to use GdkRGBA instead of GdkColor, for
            compatibility with latest lib_cairox.c
20/09/2017: Fixed sizing of legend frame - the legend title will now fit
            within the frame
20/09/2017: Fixed positioning of legend w.r.t. graph frame when the frame
            is offset from the page coordinates

Version 2.4:
13/09/2018: Added new line style - LS_NONE - for scatter plots
04/10/2018: Tweak to positioning of axis labels

Version 2.5:
13/03/2019: Allow asymmetric error bars
14/03/2020: Adjust legend drawing so that unlabelled datasets are not included
14/03/2020: Allow multiple stacks in stacked bar charts

NOTE: This library also includes 
   void graph_set_dataset_barwidth(GraphStruct *gd, int i, int bar_width)
This function is defined in some copies of version 2.4, but not all!

*******************************************************************************/

// Default Font size for title / legend / axis labels

#define DEFAULT_TITLE_FONT_SIZE   16
#define DEFAULT_LEGEND_FONT_SIZE  14
#define DEFAULT_AXIS_FONT_SIZE    14

/*----------------------------------------------------------------------------*/

static void gfree(void *p)
{
    if (p != NULL) { free(p); }
}

/*----------------------------------------------------------------------------*/

static void free_font_props(CairoxFontProperties *font)
{
    if (font->face != NULL) {
        free(font->face);
    }
    font->face = NULL;
    if (font->colour != NULL) {
#ifdef GDK2
        gdk_color_free(font->colour);
#else
        gdk_rgba_free(font->colour);
#endif
    }
    font->colour = NULL;
}

/*----------------------------------------------------------------------------*/

static void font_props_set_defaults(CairoxFontProperties *fp, int size)
{
    fp->size = size;
    fp->face = NULL;
    fp->colour = NULL;
    fp->style = PANGO_STYLE_NORMAL;
    fp->weight = PANGO_WEIGHT_NORMAL;
}

/*----------------------------------------------------------------------------*/

void graph_destroy(GraphStruct *gd)
{
    if (gd != NULL) {
        gfree(gd->title);
        free_font_props(&(gd->title_font));
        gfree(gd->axis[0].label);
        gfree(gd->axis[0].format);
        gfree(gd->axis[0].tick_labels);
        free_font_props(&(gd->axis[0].font));
        gfree(gd->axis[1].label);
        gfree(gd->axis[1].format);
        gfree(gd->axis[1].tick_labels);
        free_font_props(&(gd->axis[1].font));
        gfree(gd->legend_label);
        free_font_props(&(gd->legend_font));
        gfree(gd->dataset);
        free(gd);
    }
}

/******************************************************************************/

GraphStruct *graph_create(int num_ds)
{
    GraphStruct *gd;
    GraphDataSet *ds;
    int i;

    if ((ds = malloc(num_ds * sizeof(GraphDataSet))) == NULL) {
        return(NULL);
    }
    else if ((gd = (GraphStruct *)malloc(sizeof(GraphStruct))) == NULL) {
        free(ds);
        return(NULL);
    }
    else {
        gd->title = NULL;
        font_props_set_defaults(&(gd->title_font), DEFAULT_TITLE_FONT_SIZE);
        gd->axis[0].label = NULL;
        gd->axis[0].format = NULL;
        gd->axis[0].ticks = 0;
        gd->axis[0].tick_labels = NULL;
        font_props_set_defaults(&(gd->axis[0].font), DEFAULT_AXIS_FONT_SIZE);
        gd->axis[1].label = NULL;
        gd->axis[1].format = NULL;
        gd->axis[1].ticks = 0;
        gd->axis[1].tick_labels = NULL;
        font_props_set_defaults(&(gd->axis[1].font), DEFAULT_AXIS_FONT_SIZE);
        gd->legend_show = FALSE;
        gd->legend_label = NULL;
        gd->show_values = FALSE;
        gd->stack_bars = 0;
        font_props_set_defaults(&(gd->legend_font), DEFAULT_LEGEND_FONT_SIZE);
        gd->dataset = ds;
        gd->datasets = num_ds;
        for (i = 0; i < num_ds; i++) {
            gd->dataset[i].points = 0;
            gd->dataset[i].label = NULL;
        }
        /* Defaults: */
        graph_set_extent(gd, 0, 0, 100, 100);
        graph_set_margins(gd, 10, 10, 10, 10);

        return(gd);
    }
}

/******************************************************************************/

void graph_set_extent(GraphStruct *gd, int x, int y, int w, int h)
{
    if (gd != NULL) {
        gd->x = x;
        gd->y = y;
        gd->w = w;
        gd->h = h;
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_margins(GraphStruct *gd, int left, int right, int upper, int lower)
{
    if (gd != NULL) {
        gd->margin_left  = left;
        gd->margin_right = right;
        gd->margin_upper = upper;
        gd->margin_lower = lower;
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_title(GraphStruct *gd, char *title)
{
    if (gd != NULL) {
        gfree(gd->title);
        gd->title = string_copy(title);
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_axis_tick_marks(GraphStruct *gd, GtkOrientation orientation, int n, char **labels)
{
    if (gd != NULL) {
        int i = (orientation == GTK_ORIENTATION_VERTICAL) ? 1 : 0;
        gfree(gd->axis[i].tick_labels);
        if (n <= 0) {
            gd->axis[i].tick_labels = NULL;
        }
        else if ((gd->axis[i].tick_labels = (char **)malloc(n * sizeof(char *))) != NULL) {
            int j;

            for (j = 0; j < n; j++) {
                gd->axis[i].tick_labels[j] = string_copy(labels[j]);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_axis_properties(GraphStruct *gd, GtkOrientation orientation, double min, double max, int ticks, char *format, char *label)
{
    if (gd != NULL) {
        int i = (orientation == GTK_ORIENTATION_VERTICAL) ? 1 : 0;

        gd->axis[i].min = min;
        gd->axis[i].max = max;
        gd->axis[i].ticks = ticks;
        gfree(gd->axis[i].format);
        gd->axis[i].format = string_copy(format);
        gfree(gd->axis[i].label);
        gd->axis[i].label = string_copy(label);
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_legend_properties(GraphStruct *gd, Boolean show, double x, double y, char *label)
{
    if (gd != NULL) {
        gd->legend_show = show;
        if (gd->legend_label != NULL) {
            free(gd->legend_label);
        }
        if (label == NULL) {
            gd->legend_label = NULL;
        }
        else {
            gd->legend_label = string_copy(label);
        }
        gd->legend_x = x;
        gd->legend_y = y;
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_dataset_properties(GraphStruct *gd, int l, char *label, double r, double g, double b, int bar_width, LineStyle style, MarkerType mark)
{
    if (gd != NULL) {
        gd->dataset[l].label = string_copy(label);
        gd->dataset[l].r = r;
        gd->dataset[l].g = g;
        gd->dataset[l].b = b;
        gd->dataset[l].bar_width = bar_width;
        gd->dataset[l].style = style;
        gd->dataset[l].mark = mark;
    }
}

/*----------------------------------------------------------------------------*/

void graph_set_dataset_barwidth(GraphStruct *gd, int i, int bar_width)
{
    if ((gd != NULL) && (gd->datasets > i)) {
        gd->dataset[i].bar_width = bar_width;
    }
}

/*============================================================================*/

static void font_properties_set(CairoxFontProperties *target, CairoxFontProperties *source)
{
    target->size = source->size;
    target->style = source->style;
    target->weight = source->weight;
    if (target->face != NULL) {
       string_free(target->face);
    }
    target->face = string_copy(source->face);
#ifdef GDK2
    if (target->colour != NULL) {
        gdk_color_free(target->colour);
    }
    target->colour = gdk_color_copy(source->colour);
#else
    if (target->colour != NULL) {
        gdk_rgba_free(target->colour);
    }
    target->colour = gdk_rgba_copy(source->colour);
#endif
}

/*----------------------------------------------------------------------------*/

void graph_set_axis_font_properties(GraphStruct *gd, GtkOrientation orientation, CairoxFontProperties *fp)
{
    int i = (orientation == GTK_ORIENTATION_VERTICAL) ? 1 : 0;

    font_properties_set(&(gd->axis[i].font), fp);
}

/*----------------------------------------------------------------------------*/

void graph_set_title_font_properties(GraphStruct *gd, CairoxFontProperties *fp)
{
    font_properties_set(&(gd->title_font), fp);
}

/*----------------------------------------------------------------------------*/

void graph_set_legend_font_properties(GraphStruct *gd, CairoxFontProperties *fp)
{
    font_properties_set(&(gd->legend_font), fp);
}

/******************************************************************************/

#ifdef GDK2
static void cairox_text_parameters_set_colour(CairoxTextParameters *tp, GdkColor *colour)
#else
static void cairox_text_parameters_set_colour(CairoxTextParameters *tp, GdkRGBA *colour)
#endif
{
    if (colour != NULL) {
        cairox_text_parameters_set_foreground(tp, colour->red, colour->green, colour->blue);
    }
}

static void cairoxg_draw_grid(cairo_t *cr, PangoLayout *layout, GraphStruct *gd)
{
    if ((gd != NULL) && (cr != NULL)) {
        double x0 = gd->x+gd->margin_left;
        double x1 = gd->x+gd->w-gd->margin_right;
        double y0 = gd->y+gd->margin_upper;
        double y1 = gd->y+gd->h-gd->margin_lower;

        cairo_set_line_width(cr, 1.0);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_rectangle(cr, x0, y0, x1-x0, y1-y0);
        cairo_stroke(cr);
    }
}

static void cairoxg_draw_title(cairo_t *cr, PangoLayout *layout, GraphStruct *gd)
{
    if ((gd != NULL) && (gd->title != NULL)) {
        CairoxTextParameters p;

        pangox_layout_set_font_properties(layout, &(gd->title_font));
        cairox_text_parameters_set(&p, gd->x+(gd->w+gd->margin_left-gd->margin_right)*0.5, gd->y+gd->margin_upper-4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_text_parameters_set_colour(&p, gd->title_font.colour);
        cairox_paint_pango_text(cr, &p, layout, gd->title);
    }
}

static void cairoxg_draw_axis_labels(cairo_t *cr, PangoLayout *layout, GraphStruct *gd, GtkOrientation orientation)
{
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    int x0 = gd->x, x1 = gd->x + gd->w;
    int y0 = gd->y, y1 = gd->y + gd->h;
    double x, y;
    char buffer[32];
    int w, h, i, max_w = 0;

    cairox_line_parameters_set(&lp, 1.0, LS_SOLID, FALSE);

    if (orientation == GTK_ORIENTATION_VERTICAL) {
        /* Tick marks: */
        pangox_layout_set_font_properties(layout, &(gd->axis[1].font));
        for (i = 0; i < gd->axis[1].ticks; i++) {
            y = y1-gd->margin_lower - (y1-y0-gd->margin_upper-gd->margin_lower) * i / (double) (gd->axis[1].ticks-1);
            if (gd->axis[1].tick_labels != NULL) {
                g_snprintf(buffer, 32, "%s", gd->axis[1].tick_labels[i]);
            }
            else if (gd->axis[1].format != NULL) {
                g_snprintf(buffer, 32, gd->axis[1].format, gd->axis[1].min + (gd->axis[1].max-gd->axis[1].min) * i / (double) (gd->axis[1].ticks-1));
	    }
            else {
                buffer[0] = '\0';
            }
            if (buffer[0] != '\0') {
                /* Only show the tick and label if the label is non-null */
                cairox_paint_line(cr, &lp, x0+gd->margin_left-3, y, x0+gd->margin_left+2, y);
                cairox_text_parameters_set(&tp, x0+gd->margin_left-4, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 0.0);
                cairox_text_parameters_set_colour(&tp, (gd->axis[1].font.colour));
                cairox_paint_pango_text(cr, &tp, layout, buffer);
                pango_layout_get_size(layout, &w, &h);
                max_w = MAX(w/PANGO_SCALE, max_w);
            }
        }
        /* The axis label: */
        if (gd->axis[1].label != NULL) {
            x = x0+gd->margin_left-max_w-gd->axis[1].font.size;
            y = y1-gd->margin_lower - (y1-y0-2-gd->margin_upper-gd->margin_lower) * 0.5;
            //            cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 90.0); // HERE
            cairox_text_parameters_set(&tp, x0, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 90.0);
            cairox_text_parameters_set_colour(&tp, (gd->axis[1].font.colour));
            cairox_paint_pango_text(cr, &tp, layout, gd->axis[1].label);
        }
    }
    else if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        pangox_layout_set_font_properties(layout, &(gd->axis[0].font));
        for (i = 0; i < gd->axis[0].ticks; i++) {
            x = x0+gd->margin_left + (x1-x0-gd->margin_left-gd->margin_right) * i / (double) (gd->axis[0].ticks-1);
            if (gd->axis[0].tick_labels != NULL) {
                g_snprintf(buffer, 32, "%s", gd->axis[0].tick_labels[i]);
            }
            else if (gd->axis[0].format != NULL) {
                g_snprintf(buffer, 32, gd->axis[0].format, gd->axis[0].min+(gd->axis[0].max-gd->axis[0].min) * i / (double) (gd->axis[0].ticks-1));
	    }
            else {
                buffer[0] = '\0';
            }
            if (buffer[0] != '\0') {
                /* Only show the tick and label if the label is non-null */
                cairox_paint_line(cr, &lp, x, y1-gd->margin_lower-2, x,  y1-gd->margin_lower+3);
                cairox_text_parameters_set(&tp, x, y1-gd->margin_lower+4, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
                cairox_text_parameters_set_colour(&tp, (gd->axis[0].font.colour));
                cairox_paint_pango_text(cr, &tp, layout, buffer);
            }
        }
        /* The axis label: */
        if (gd->axis[0].label != NULL) {
            x = x0+gd->margin_left + (x1-x0-gd->margin_left-gd->margin_right) * 0.5;
            cairox_text_parameters_set(&tp, x, y1, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_text_parameters_set_colour(&tp, (gd->axis[0].font.colour));
            cairox_paint_pango_text(cr, &tp, layout, gd->axis[0].label);
        }
    }
}

static void cairoxg_dataset_set_colour(cairo_t *cr, GraphStruct *gd, int l, Boolean colour)
{
    if (colour) {
        cairo_set_source_rgb(cr, gd->dataset[l].r, gd->dataset[l].g, gd->dataset[l].b);
    }
    else if (gd->dataset[l].bar_width > 0) { // Black/white bar data - use a shade or grey:
        double grey = (gd->dataset[l].r + gd->dataset[l].g + gd->dataset[l].b) / 3.0;
        cairo_set_source_rgb(cr, grey, grey, grey);
    }
    else { // Black/white line data - use black:
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    }
}

static void cairoxg_dataset_set_style(cairo_t *cr, GraphStruct *gd, int l)
{
    double dots[2] = {2.0, 2.0};
    double dashes[2] = {6.0, 6.0};

    if (gd->dataset[l].style == LS_DOTTED) {
        cairo_set_dash(cr, dots, 2, 0);
    }
    else if (gd->dataset[l].style == LS_DASHED) {
        cairo_set_dash(cr, dashes, 2, 0);
    }
    else {
        cairo_set_dash(cr, NULL, 0, 0);
    }
}

static Boolean out_of_bounds(GraphStruct *gd, double y)
{
    // Allow up to 10% extra
    return(FALSE);    //     return(y > (gd->axis[1].max * 1.1));
}

static int max_width_of_bar(GraphStruct *gd, int i, int datasets_per_bar)
{
    int j, mw;

    mw = gd->dataset[i].bar_width;
    for (j = 1; j < datasets_per_bar; j++) {
        if (gd->dataset[i+j].bar_width > mw) {
            mw = gd->dataset[i+j].bar_width;
        }
    }
    return(mw);
}

static double calculate_bar_offset(GraphStruct *gd, int l)
{
    if (gd->stack_bars == 0) {
        double total_bar_width = 0;
        double bar_width_so_far = 0;
        int i;

        for (i = 0; i < gd->datasets; i++) {
            if (gd->dataset[i].bar_width > 0) {
                if (i < l) {
                    bar_width_so_far += gd->dataset[i].bar_width;
                }
                total_bar_width += gd->dataset[i].bar_width;
            }
        }
        return(bar_width_so_far - (total_bar_width / 2.0));
    }
    else if (gd->stack_bars == 1) {
        int bar_width = max_width_of_bar(gd, 0, gd->datasets);
        return(-bar_width / 2.0);
    }
    else {
        // Multiple stacks - offset will depend on the number of datasets
        // per bar:

        double total_bar_width = 0;
        double bar_width_so_far = 0;
        int datasets_per_bar = (int) ceil(gd->datasets / (double) gd->stack_bars);
        int i, bar_width;

        i = 0;
        while (i < gd->datasets) {
            bar_width = max_width_of_bar(gd, i, datasets_per_bar);
            if (i + datasets_per_bar <= l) {
                bar_width_so_far += bar_width;
            }
            total_bar_width += bar_width;
            i = i + datasets_per_bar;
        }
        return(bar_width_so_far - (total_bar_width / 2.0));
    }
}

static void cairoxg_draw_dataset(cairo_t *cr, PangoLayout *layout, GraphStruct *gd, int l, Boolean colour)
{
    CairoxTextParameters tp;
    char buffer[64];
    double x_offset, y0, y_offset;
    double x, y;
    int i = 0;

    cairoxg_dataset_set_colour(cr, gd, l, colour);
    cairoxg_dataset_set_style(cr, gd, l);

    if (gd->dataset[l].bar_width > 0) { /* Bar data */
        y0 = gd->y + gd->h -gd->margin_lower;

        x_offset = calculate_bar_offset(gd, l);

        for (i = 0; i < gd->dataset[l].points; i++) {
            x = gd->x+gd->margin_left + (gd->dataset[l].x[i] - gd->axis[0].min) * (gd->w-gd->margin_left-gd->margin_right) / (gd->axis[0].max - gd->axis[0].min);
            y = gd->y+gd->margin_upper + (gd->axis[1].max - gd->dataset[l].y[i]) * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);

            if (gd->stack_bars == 0) {
                y_offset = 0;
            }
            else if (gd->stack_bars == 1) {
                int j;
                double sum = 0;
                for (j = 0; j < l; j++) {
                    sum += gd->dataset[j].y[i];
                }
                y_offset = sum * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
            }
            else {
                // Multiple stacks: stack_base is the index of the first
                // dataset in the current stack
                int j, stack_base;
                double sum = 0;
                int datasets_per_bar = (int) ceil(gd->datasets / (double) gd->stack_bars);

                stack_base = datasets_per_bar * (l / datasets_per_bar);
                for (j = stack_base; j < l; j++) {
                    sum += gd->dataset[j].y[i];
                }
                y_offset = sum * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
            }

            if (out_of_bounds(gd, gd->dataset[l].y[i])) {
                // Cap y, or show nothing???
                fprintf(stdout, "FIXME: %s: %s at %d [Value is out of bounds!]\n", __FILE__, __FUNCTION__, __LINE__);
            }
            cairoxg_dataset_set_colour(cr, gd, l, colour);
            cairo_rectangle(cr, x+x_offset, y-y_offset, gd->dataset[l].bar_width, y0-y);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_stroke(cr);

            if ((gd->show_values) && (gd->dataset[l].y[i] != 0.0)) {
                // This shows the actual value within the bar
                g_snprintf(buffer, 64, "%6.3f", gd->dataset[l].y[i]);
                cairox_text_parameters_set(&tp, x+x_offset+gd->dataset[l].bar_width*0.5, y0-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 90.0);
                cairox_paint_pango_text(cr, &tp, layout, buffer);
            }
        }
    }
    else { /* Show the data as a line: */
        if ((gd->dataset[l].style != LS_NONE) && (gd->dataset[l].points > 1)) {
            i = 0;
            // First draw a continuous line from left to right:
            x = gd->x+gd->margin_left + (gd->dataset[l].x[i] - gd->axis[0].min) * (gd->w-gd->margin_left-gd->margin_right) / (gd->axis[0].max - gd->axis[0].min);
            y = gd->y+gd->margin_upper + (gd->axis[1].max - gd->dataset[l].y[i]) * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
            cairo_move_to(cr, x, y);
            while (++i < gd->dataset[l].points) {
                // If the point is out of bounds ...
                if (out_of_bounds(gd, gd->dataset[l].y[i])) {
                    fprintf(stdout, "FIXME: %s: %s at %d [Value is out of bounds!]\n", __FILE__, __FUNCTION__, __LINE__);
                    // stroke the line so far,
                    cairo_stroke(cr);
                    // and cairo_move_to the next point that is within bounds, if there is one
                    if (++i < gd->dataset[l].points) {
                        if (!out_of_bounds(gd, gd->dataset[l].y[i])) {
                            x = gd->x+gd->margin_left + (gd->dataset[l].x[i] - gd->axis[0].min) * (gd->w-gd->margin_left-gd->margin_right) / (gd->axis[0].max - gd->axis[0].min);
                            y = gd->y+gd->margin_upper + (gd->axis[1].max - gd->dataset[l].y[i]) * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
                            cairo_move_to(cr, x, y);
                        }
                    }
                }
                else {
                    x = gd->x+gd->margin_left + (gd->dataset[l].x[i] - gd->axis[0].min) * (gd->w-gd->margin_left-gd->margin_right) / (gd->axis[0].max - gd->axis[0].min);
                    y = gd->y+gd->margin_upper + (gd->axis[1].max - gd->dataset[l].y[i]) * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
                    cairo_line_to(cr, x, y);
                }
            }
            cairo_stroke(cr);
        }
    }
}

static void cairoxg_draw_dataset_errorbars(cairo_t *cr, PangoLayout *layout, GraphStruct *gd, int l, Boolean colour)
{
    double x_offset, y_offset;
    double x, y, dy;
    int i = 0;

    // Ensure the line style is SOLID
    cairo_set_dash(cr, NULL, 0, 0);
    // Now paint markers and error bars for each point, if appropriate:
    for (i = 0; i < gd->dataset[l].points; i++) {
        x = gd->x+gd->margin_left + (gd->dataset[l].x[i] - gd->axis[0].min) * (gd->w-gd->margin_left-gd->margin_right) / (gd->axis[0].max - gd->axis[0].min);

        // Adjust y value in case we have stacked bars
        if (gd->stack_bars > 1) {
            int j, stack_base;
            double sum = 0;
            int datasets_per_bar = (int) ceil(gd->datasets / (double) gd->stack_bars);

            stack_base = datasets_per_bar * (l / datasets_per_bar);
            for (j = stack_base; j < l; j++) {
                sum += gd->dataset[j].y[i];
            }
            y_offset = sum + gd->dataset[l].y[i];
        }
        else {
            y_offset = gd->dataset[l].y[i];
        }

        y = gd->y+gd->margin_upper + (gd->axis[1].max - y_offset) * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);

        /* If it's bar data we might need to add a horizontal offset: */
        if (gd->dataset[l].bar_width > 0) { /* Bar data */
            x_offset = calculate_bar_offset(gd, l);
            x = x + x_offset + gd->dataset[l].bar_width/2.0;
        }

        if (gd->dataset[l].mark != MARK_NONE) {
            cairoxg_dataset_set_colour(cr, gd, l, colour);
            cairox_paint_marker(cr, x, y, gd->dataset[l].mark);
        }

        // Error bars are in black
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

        /* Lower error bar: */
        if (gd->dataset[l].dy1[i] > 0) {
            dy = gd->dataset[l].dy1[i] * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
            cairo_save(cr);
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x, y+dy);
            cairo_stroke(cr);
            cairo_move_to(cr, x-4, y+dy);
            cairo_line_to(cr, x+4, y+dy);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
        /* Upper error bar: */
        if (gd->dataset[l].dy2[i] > 0) {
            dy = gd->dataset[l].dy2[i] * (gd->h-gd->margin_upper-gd->margin_lower) / (gd->axis[1].max - gd->axis[1].min);
            cairo_save(cr);
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x, y-dy);
            cairo_stroke(cr);
            cairo_move_to(cr, x-4, y-dy);
            cairo_line_to(cr, x+4, y-dy);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
    }
}

static void cairoxg_draw_legend(cairo_t *cr, PangoLayout *layout, GraphStruct *gd, Boolean colour)
{
    CairoxTextParameters tp;
    CairoxLineParameters lp;
    double x, y, y0;
    int line_sep = gd->legend_font.size + 2;
    int legend_width = 40; // Minimum legend width
    int entry_width;
    int i;

    x = gd->x + gd->margin_left + gd->legend_x * (gd->w-gd->margin_left-gd->margin_right);
    y0 = gd->y + gd->margin_upper + gd->legend_y * (gd->h-gd->margin_upper-gd->margin_lower);

    pangox_layout_set_font_properties(layout, &(gd->legend_font));

    y = y0+5;
    if (gd->legend_label != NULL) {
        y = y + line_sep;
        cairox_text_parameters_set(&tp, x+29, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_text_parameters_set_colour(&tp, gd->legend_font.colour);
        cairox_paint_pango_text(cr, &tp, layout, gd->legend_label);
        legend_width = 29 + pangox_layout_get_string_width(layout, gd->legend_label) + 29;
    }

    for (i = 0; i < gd->datasets; i++) {
        if (gd->dataset[i].label != NULL) {
            if (gd->dataset[i].points > 0) {
                y = y + line_sep;
                // set the colour/shade and draw
                cairoxg_dataset_set_colour(cr, gd, i, colour);
                if (gd->dataset[i].bar_width == 0) {
                    cairox_line_parameters_set(&lp, 1.0, gd->dataset[i].style, TRUE);
                    cairox_paint_line(cr, &lp, x+4, y-8, x+20, y-8);
                    cairox_paint_marker(cr, x+14, y-8, gd->dataset[i].mark);
                }
                else {
                    cairo_set_line_width(cr, 1.0);
                    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
                    cairo_rectangle(cr, x+4, y-gd->legend_font.size, 20, gd->legend_font.size);
                    cairo_fill_preserve(cr);
                    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
                    cairo_stroke(cr);
                }
                cairox_text_parameters_set(&tp, x+29, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
                cairox_text_parameters_set_colour(&tp, gd->legend_font.colour);
                cairox_paint_pango_text(cr, &tp, layout, gd->dataset[i].label);
                entry_width = pangox_layout_get_string_width(layout, gd->dataset[i].label);
                legend_width = MAX(legend_width, entry_width + 4 + 29);
            }
        }
    }

    /* The box around the legend: */
    if (y > y0+5) {
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_rectangle(cr, x, y0, legend_width, y-y0+5);
        cairo_stroke(cr);
    }
}

void graph_set_stack_bars(GraphStruct *gd, int stack)
{
    if (gd != NULL) {
        gd->stack_bars = stack;
    }
}

void graph_set_show_values(GraphStruct *gd, Boolean show)
{
    if (gd != NULL) {
        gd->show_values = show;
    }
}

/*----------------------------------------------------------------------------*/

void cairox_draw_graph(cairo_t *cr, GraphStruct *gd, Boolean colour)
{
    if (cr == NULL) {
        fprintf(stderr, "NULL cr in %s\n", __FUNCTION__);
    }
    else if (gd == NULL) {
        fprintf(stderr, "NULL gd in %s\n", __FUNCTION__);
    }
    else {
        PangoLayout *layout = pango_cairo_create_layout(cr);
        int i;

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_rectangle(cr, gd->x, gd->y, gd->w, gd->h);
        cairo_fill(cr);

        cairoxg_draw_grid(cr, layout, gd);
        cairoxg_draw_title(cr, layout, gd);
        cairoxg_draw_axis_labels(cr, layout, gd, GTK_ORIENTATION_HORIZONTAL);
        cairoxg_draw_axis_labels(cr, layout, gd, GTK_ORIENTATION_VERTICAL);
        for (i = 0; i < gd->datasets; i++) {
            cairoxg_draw_dataset(cr, layout, gd, i, colour);
        }
        for (i = 0; i < gd->datasets; i++) {
            cairoxg_draw_dataset_errorbars(cr, layout, gd, i, colour);
        }
        if (gd->legend_show) {
            cairoxg_draw_legend(cr, layout, gd, colour);
        }

        g_object_unref(layout);
    }
}

/******************************************************************************/
