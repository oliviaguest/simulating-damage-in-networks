/*******************************************************************************

    File:       lib_cairox.c
    Contents:   Cairo extensions for GTK+2
    Author:     Rick Cooper
    Copyright (c) 2015 Richard P. Cooper

    Public procedures:
        double line_length(CairoxPoint *p1, CairoxPoint *p2)
        double line_calculate_slope(CairoxPoint *p1, CairoxPoint *p2)
        void line_displace(CairoxPoint *p1, CairoxPoint *p2, double offset)
        void cairox_text_parameters_set_foreground(CairoxTextParameters *p, double r, double g, double b)
        void cairox_text_parameters_set(CairoxTextParameters *p, double x, double y, PangoXAlignment x_align, PangoYAlignment y_align, double angle)
        double cairox_paint_text_in_layout(cairo_t *cr, int x, int y, PangoLayout *layout, char *text)
        void cairox_paint_pango_text(cairo_t *cr, CairoxTextParameters *p, PangoLayout *layout, const char *buffer)
        void cairox_paint_pixbuf(cairo_t *cr, GdkPixbuf *pb, int x0, int y0)
        void cairox_paint_line(cairo_t *cr, CairoxLineParameters *p, double x1, double y1, double x2, double y2)
        void cairox_paint_lines(cairo_t *cr, CairoxPoint *p, int n)
        void cairox_paint_polygon(cairo_t *cr, CairoxPoint *p, int n)
        void cairox_paint_circle(cairo_t *cr, double x1, double y1, double r)
        void cairox_arrow_parameters_set(CairoxArrowParameters *p, double width, LineStyle ls, VertexStyle vs, ArrowHeadType head, double density, Boolean bidirectional)
        void cairox_paint_arrow(cairo_t *cr, CairoxPoint *coordinates, int points, CairoxArrowParameters *params)
        void cairox_paint_grid(cairo_t *cr, int w, int h, double gs)
        void cairox_paint_marker(cairo_t *cr, double x, double y, MarkerType mark)
        void cairox_centre_text(cairo_t *cr, double x0, double y0, PangoLayout *layout, char *label)
        void cairox_vector_draw(cairo_t *cr, double x0, double y0, Boolean colour, int nf, double *vals)
        void cairox_set_colour_by_index(cairo_t *cr, int colour)
        void cairox_select_colour_scale(cairo_t *cr, double d)
        CairoxPoint *cairox_pointlist_copy(CairoxPoint *old, int length)
        void cairox_pointlist_reverse(CairoxPoint *p, int length)
        void pangox_layout_set_font_from_widget(PangoLayout *layout, GtkWidget *widg)
        void pangox_layout_select_font_from_string(PangoLayout *layout, char *string)
        void pangox_layout_set_font_face(PangoLayout *layout, const char *family)
        void pangox_layout_set_font_size(PangoLayout *layout, int size)
        void pangox_layout_set_font_weight(PangoLayout *layout, PangoWeight weight)
        void pangox_layout_set_font_style(PangoLayout *layout, PangoStyle style)
        int pangox_font_description_get_font_height(const PangoFontDescription *font)
        int pangox_layout_get_font_height(PangoLayout *layout)
        double pangox_layout_get_string_width(PangoLayout *layout, char *text)

*******************************************************************************/
/******** Include files: ******************************************************/

typedef enum boolean {FALSE, TRUE} Boolean;

#include "lib_cairox.h"
#include "lib_string.h"
#include "lib_error.h"
#include <math.h>
#include <string.h>
#include <ctype.h>

#define LURID TRUE

/******************************************************************************/
/* SECTION 1: Line utility functions: *****************************************/
/******************************************************************************/

double line_length(CairoxPoint *p1, CairoxPoint *p2)
{
    return(sqrt((p1->x-p2->x)*(p1->x-p2->x) + (p1->y-p2->y)*(p1->y-p2->y)));
}

/******************************************************************************/

double line_calculate_slope(CairoxPoint *p1, CairoxPoint *p2)
{
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;

    if (dx > 0.0) {
        return(M_PI + atan(dy/dx));
    }
    else if (dx < 0.0) {
        return(atan(dy/dx));
    }
    else if (dy > 0.0) {
        return(-M_PI / 2.0);
    }
    else if (dy < 0.0) {
        return(M_PI / 2.0);
    }
    else {
        return(0.0);
    }
}

/******************************************************************************/

void line_displace(CairoxPoint *p1, CairoxPoint *p2, double offset)
{
    if (offset != 0) {
        double theta = line_calculate_slope(p1, p2);
        double dx = (short) 5 * sin(theta);
        double dy = (short) 5 * cos(theta);

        p1->x = p1->x - offset * dx;
        p1->y = p1->y + offset * dy;
        p2->x = p2->x - offset * dx;
        p2->y = p2->y + offset * dy;
    }
}

/******************************************************************************/
/* SECTION 2: Paint cairo elements: *******************************************/
/******************************************************************************/

void cairox_text_parameters_set_foreground(CairoxTextParameters *p, double r, double g, double b)
{
    p->foreground.r = r;
    p->foreground.g = g;
    p->foreground.b = b;
}

/*----------------------------------------------------------------------------*/

void cairox_text_parameters_set(CairoxTextParameters *p, double x, double y, PangoXAlignment x_align, PangoYAlignment y_align, double angle)
{
    cairox_text_parameters_set_foreground(p, 0.0, 0.0, 0.0);
    p->x = x;
    p->y = y;
    p->x_align = x_align;
    p->y_align = y_align;
    p->angle = angle;
}

/*----------------------------------------------------------------------------*/

double cairox_paint_text_in_layout(cairo_t *cr, int x, int y, PangoLayout *layout, char *text)
{
    cairo_move_to(cr, x, y);
    pango_layout_set_text(layout, text, strlen(text));
    pango_cairo_show_layout(cr, layout);
    return(pangox_layout_get_string_width(layout, text));
}

/*----------------------------------------------------------------------------*/

void cairox_paint_pango_text(cairo_t *cr, CairoxTextParameters *p, PangoLayout *layout, const char *buffer)
{
    int x, y, w, h;

    if ((buffer == NULL) || (strlen(buffer) == 0)) {
        return;
    }

    cairo_save(cr);
    cairo_translate(cr, p->x, p->y); 
    cairo_rotate(cr, -p->angle * M_PI / 180.0);

    pango_layout_set_text(layout, buffer, strlen(buffer));

    if (p->x_align == PANGOX_XALIGN_CENTER) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        if ((w = pango_layout_get_width(layout)) <= 0) {
            pango_layout_get_size(layout, &w, &h);
        }
        x = - w/((double) PANGO_SCALE*2.0);
    }
    else if (p->x_align == PANGOX_XALIGN_RIGHT) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
        if ((w = pango_layout_get_width(layout)) <= 0) {
            pango_layout_get_size(layout, &w, &h);
        }
        x = - w/((double) PANGO_SCALE);
    }
    else {
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
        x = 0;
    }

    if (p->y_align == PANGOX_YALIGN_BOTTOM) {
        pango_layout_get_size(layout, &w, &h);
        y = - h/((double) PANGO_SCALE);
    }
    else if (p->y_align == PANGOX_YALIGN_CENTER) {
        pango_layout_get_size(layout, &w, &h);
        y = - h/((double) PANGO_SCALE*2.0);
    }
    else {
        y = 0;
    }

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, x-1, y-1);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x, y-1);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x+1, y-1);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x-1, y);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x+1, y);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x-1, y+1);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x, y+1);
    pango_cairo_show_layout(cr, layout);
    cairo_move_to(cr, x+1, y+1);
    pango_cairo_show_layout(cr, layout);

    cairo_set_source_rgba(cr, p->foreground.r, p->foreground.g, p->foreground.b, 1.0);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);
    cairo_stroke(cr);
    cairo_restore(cr);
}

/*----------------------------------------------------------------------------*/

void cairox_paint_pixbuf(cairo_t *cr, GdkPixbuf *pb, int x0, int y0)
{
    if (pb != NULL) {
        gdk_cairo_set_source_pixbuf(cr, pb, x0, y0);
        cairo_paint(cr);
    }
}

/*----------------------------------------------------------------------------*/

void cairox_line_parameters_set(CairoxLineParameters *p, double width, LineStyle ls, Boolean antialias)
{
    if (p != NULL) {
        p->width = width;
        p->line_style = ls;
        p->antialias = antialias;
    }
}

void cairox_paint_line(cairo_t *cr, CairoxLineParameters *params, double x1, double y1, double x2, double y2)
{
    if (params == NULL) {
        cairo_set_dash(cr, NULL, 0, 0);
        cairo_set_line_width(cr, 1.0);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    }
    else {
        cairo_set_line_width(cr, params->width);
        if (params->line_style == LS_DOTTED) {
            double dots[2] = {2, 2};
            cairo_set_dash(cr, dots, 2, 0);
        }
        else if (params->line_style == LS_DASHED) {
            double dashes[2] = {6, 6};
            cairo_set_dash(cr, dashes, 2, 0);
        }
        else {
            cairo_set_dash(cr, NULL, 0, 0);
        }
        if (!params->antialias) {
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        }
    }

    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);

    if (params != NULL) {
        cairo_set_dash(cr, NULL, 0, 0);
        cairo_set_line_width(cr, 1.0);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    }
}

/*----------------------------------------------------------------------------*/

void cairox_paint_lines(cairo_t *cr, CairoxPoint *p, int n)
{
    int i;

    cairo_new_path(cr);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, p[0].x, p[0].y);
    for (i = 1; i < n; i++) {
        cairo_line_to(cr, p[i].x, p[i].y);
    }
    cairo_stroke(cr);
}

/*----------------------------------------------------------------------------*/

void cairox_paint_polygon(cairo_t *cr, CairoxPoint *p, int n)
{
    int i;

    cairo_new_path(cr);
    cairo_move_to(cr, p[0].x, p[0].y);
    for (i = 1; i < n; i++) {
        cairo_line_to(cr, p[i].x, p[i].y);
    }
    cairo_close_path(cr);
    cairo_fill(cr);
}

/*----------------------------------------------------------------------------*/

void cairox_paint_circle(cairo_t *cr, double x1, double y1, double r)
{
    cairo_new_path(cr);
    cairo_arc(cr, x1, y1, r, 0.0, 2.0 * M_PI);
    cairo_close_path(cr);
    cairo_fill(cr);
}

/*----------------------------------------------------------------------------*/



static void cairox_paint_arrow_head(cairo_t *cr, ArrowHeadType head, CairoxPoint p[2], double width)
{
    CairoxPoint q[4];
    double theta = line_calculate_slope(&p[0], &p[1]);
    double cos_theta = cos(theta);
    double sin_theta = sin(theta);

    if (head == AH_SHARP) {
        q[0].x = p[1].x;
        q[0].y = p[1].y;
        q[1].x = (p[1].x + (width * 14.0 * cos(theta-0.3)));
        q[1].y = (p[1].y + (width * 14.0 * sin(theta-0.3)));
        q[2].x = (p[1].x + (width * 10.0 * cos_theta));
        q[2].y = (p[1].y + (width * 10.0 * sin_theta));
        q[3].x = (p[1].x + (width * 14.0 * cos(theta+0.3)));
        q[3].y = (p[1].y + (width * 14.0 * sin(theta+0.3)));
        cairox_paint_polygon(cr, q, 4);
    }
    else if (head == AH_WIDE) {
        q[0].x = p[1].x;
        q[0].y = p[1].y;
        q[1].x = (p[1].x + (width * 12.0 * cos(theta-0.5)));
        q[1].y = (p[1].y + (width * 12.0 * sin(theta-0.5)));
        q[2].x = (p[1].x + (width *  9.0 * cos_theta));
        q[2].y = (p[1].y + (width *  9.0 * sin_theta));
        q[3].x = (p[1].x + (width * 12.0 * cos(theta+0.5)));
        q[3].y = (p[1].y + (width * 12.0 * sin(theta+0.5)));
        cairox_paint_polygon(cr, q, 4);
    }
    else if (head == AH_BLUNT) {
        q[0].x = (p[1].x + (width * 10.0 * cos_theta));
        q[0].y = (p[1].y + (width * 10.0 * sin_theta));
        q[1].x = (p[1].x + (width * 5.0 * sin_theta));
        q[1].y = (p[1].y - (width * 5.0 * cos_theta));
        q[2].x = (p[1].x - (width * 5.0 * sin_theta));
        q[2].y = (p[1].y + (width * 5.0 * cos_theta));
        cairox_paint_polygon(cr, q, 3);
    }
    else if (head == AH_CIRCLE) {
        q[0].x = p[1].x;
        q[0].y = p[1].y;
        cairox_paint_circle(cr, q[0].x, q[0].y, width * 4);
    }
}

/*----------------------------------------------------------------------------*/

static void cairox_arrow_paint_heads(cairo_t *cr, CairoxPoint *p, int points, CairoxArrowParameters *params)
{
    CairoxPoint q[2];
    double head_width = (params->width + 5.0) / 6.0;

    if (params->head1 != AH_NONE) {
        q[0].x = p[1].x;
        q[0].y = p[1].y;
        q[1].x = p[0].x;
        q[1].y = p[0].y;
        cairox_paint_arrow_head(cr, params->head1, q, head_width);
    }

    if (params->head2  != AH_NONE) {
        q[0].x = p[points-2].x;
        q[0].y = p[points-2].y;
        q[1].x = p[points-1].x;
        q[1].y = p[points-1].y;
        cairox_paint_arrow_head(cr, params->head2, q, head_width);
    }
}

/*----------------------------------------------------------------------------*/

static void cairox_arrow_move_to_start_point(cairo_t *cr, CairoxPoint *p, int points, CairoxArrowParameters *params)
{
    double head_width = (params->width + 5.0) / 6.0;
    double theta = line_calculate_slope(&p[0], &p[1]);
    double cos_theta = cos(theta);
    double sin_theta = sin(theta);
    CairoxPoint q;

    switch (params->head1) {
        case AH_SHARP: {
            q.x = p[0].x - (head_width * 10.0 * cos_theta);
            q.y = p[0].y - (head_width * 10.0 * sin_theta);
            break;
        }
        case AH_WIDE: {
            q.x = p[0].x - (head_width * 9.0 * cos_theta);
            q.y = p[0].y - (head_width * 9.0 * sin_theta);
            break;
        }
        case AH_CIRCLE: {
            q.x = p[0].x - (head_width * 4.0 * cos_theta);
            q.y = p[0].y - (head_width * 4.0 * sin_theta);
            break;
        }
        default: {
            q.x = p[0].x;
            q.y = p[0].y;
            break;
        }
    }

    cairo_move_to(cr, q.x, q.y);
}

/*----------------------------------------------------------------------------*/

static void cairox_arrow_paint_last_segment(cairo_t *cr, CairoxPoint *p, int points, CairoxArrowParameters *params)
{
    double head_width = (params->width + 5.0) / 6.0;
    double theta = line_calculate_slope(&p[points-2], &p[points-1]);
    double cos_theta = cos(theta);
    double sin_theta = sin(theta);
    CairoxPoint q;

    switch (params->head2) {
        case AH_SHARP: {
            q.x = p[points-1].x + (head_width * 10.0 * cos_theta);
            q.y = p[points-1].y + (head_width * 10.0 * sin_theta);
            break;
        }
        case AH_WIDE: {
            q.x = p[points-1].x + (head_width * 9.0 * cos_theta);
            q.y = p[points-1].y + (head_width * 9.0 * sin_theta);
            break;
        }
        case AH_CIRCLE: {
            q.x = p[points-1].x + (head_width * 4.0 * cos_theta);
            q.y = p[points-1].y + (head_width * 4.0 * sin_theta);
            break;
        }
        default: {
            q.x = p[points-1].x;
            q.y = p[points-1].y;
            break;
        }
    }

    cairo_line_to(cr, q.x, q.y);
    cairo_stroke(cr);
}

/*----------------------------------------------------------------------------*/

static void cairox_draw_wide_arrow(cairo_t *cr, CairoxPoint *coordinates, int points, CairoxArrowParameters *params)
{
    cairox_arrow_move_to_start_point(cr, coordinates, points, params);
    double dots[2] = {2, 2};
    double dashes[2] = {6, 6};

    if (params->line_style == LS_DOTTED) {
        cairo_set_dash(cr, dots, 2, 0);
    }
    else if (params->line_style == LS_DASHED) {
        cairo_set_dash(cr, dashes, 2, 0);
    }
    else {
        cairo_set_dash(cr, NULL, 0, 0);
    }

    if (points <= 2) {
        cairox_arrow_paint_last_segment(cr, coordinates, points, params);
    }
    else if (params->vertex_style == VS_STRAIGHT) {
        int i = 0;

        while (++i < points-1) {
            /* Make line segments */
            cairo_line_to(cr, coordinates[i].x, coordinates[i].y);
        }
        cairox_arrow_paint_last_segment(cr, coordinates, points, params);
    }
    else if (params->vertex_style == VS_CURVED) {
        int i = 0;

        while (++i < points-1) {
            double w1 = line_length(&coordinates[i-1], &coordinates[i]);
            double w2 = line_length(&coordinates[i+1], &coordinates[i]);
            double dw = MIN(w1, w2) / 2.0;
            CairoxPoint point_a, point_b, centre;
            double slope_a, slope_b, radius;
            double angle1, angle2;

            /* Where does the arc start: */
            point_a.x = coordinates[i-1].x + (coordinates[i].x-coordinates[i-1].x)*(w1-dw)/w1;
            point_a.y = coordinates[i-1].y + (coordinates[i].y-coordinates[i-1].y)*(w1-dw)/w1;

            /* Where does the arc end: */
            point_b.x = coordinates[i+1].x + (coordinates[i].x-coordinates[i+1].x)*(w2-dw)/w2;
            point_b.y = coordinates[i+1].y + (coordinates[i].y-coordinates[i+1].y)*(w2-dw)/w2;

            slope_a = line_calculate_slope(&coordinates[i-1], &coordinates[i]);
            slope_b = line_calculate_slope(&coordinates[i+1], &coordinates[i]);

            if (fabs(slope_b - slope_a) < 0.0001) {
                /* Just draw a straight line to where the arc finishes! */
                cairo_line_to(cr, point_b.x, point_b.y);
            }
            else {
                radius = dw * tan((slope_b-slope_a)/2.0);

                /* arc is of size radius and goes from point_a to point_b */
                centre.x = point_a.x - radius * sin(slope_a);
                centre.y = point_a.y + radius * cos(slope_a);

                /* Draw a line segment in the appropriate direction: */
                cairo_line_to(cr, point_a.x, point_a.y);
                /* Now draw the arc: */
                if (radius < 0) {
                    angle1 = slope_a + M_PI / 2.0;
                    angle2 = slope_b - M_PI / 2.0;
                    cairo_arc(cr, centre.x, centre.y, -radius, angle1, angle2);
                }
                else {
                    angle1 = slope_a - M_PI / 2.0;
                    angle2 = slope_b + M_PI / 2.0;
                    cairo_arc_negative(cr, centre.x, centre.y, radius, angle1, angle2);
                }
            }
        }
        cairox_arrow_paint_last_segment(cr, coordinates, points, params);
    }
    cairox_arrow_paint_heads(cr, coordinates, points, params);
    cairo_set_dash(cr, NULL, 0, 0);
}

/*----------------------------------------------------------------------------*/

void cairox_arrow_parameters_set(CairoxArrowParameters *p, double width, LineStyle ls, VertexStyle vs, ArrowHeadType head, double density, Boolean bidirectional)
{
    if (p != NULL) {
        p->width = width;
        p->line_style = ls;
        p->vertex_style = vs;
        p->head2 = head;
        p->density = density;
        if (bidirectional) {
            p->head1 = head;
        }
        else {
            p->head1 = AH_NONE;
        }
    }
}

/*----------------------------------------------------------------------------*/

void cairox_paint_arrow(cairo_t *cr, CairoxPoint *coordinates, int points, CairoxArrowParameters *params)
{
    cairo_set_line_width(cr, params->width);
    cairox_draw_wide_arrow(cr, coordinates, points, params);
    if ((params->width > 2) && (params->density < 1.0)) {
        double d = 1.0 - params->density;

        cairo_set_line_width(cr, params->width-2);
        cairo_set_source_rgb(cr, d, d, d);
        cairox_draw_wide_arrow(cr, coordinates, points, params);
    }
}

/*----------------------------------------------------------------------------*/

void cairox_paint_grid(cairo_t *cr, int w, int h, double gs)
{
    double x, y;

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_set_line_width(cr, 1);

    for (x = 0; x < w; x = x + gs) {
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, h);
    }

    for (y = 0; y < h; y = y + gs) {
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, w, y);
    }
    cairo_stroke(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
}

/*----------------------------------------------------------------------------*/

void cairox_paint_marker(cairo_t *cr, double x, double y, MarkerType mark)
{
    double d = 6.0;

    switch (mark) {
        case MARK_NONE: {
            break;
        }
        case MARK_CIRCLE_OPEN: {
            cairo_arc(cr, x, y, d, 0.0, 2 * M_PI);
            cairo_stroke(cr);
            break;
        }
        case MARK_CIRCLE_FILLED: {
            cairo_arc(cr, x, y, d, 0.0, 2 * M_PI);
            cairo_fill(cr);
            break;
        }
        case MARK_DIAMOND_OPEN: {
            cairo_move_to(cr, x-d, y);
            cairo_line_to(cr, x, y-d);
            cairo_line_to(cr, x+d, y);
            cairo_line_to(cr, x, y+d);
            cairo_close_path(cr);
            cairo_stroke(cr);
            break;
        }
        case MARK_DIAMOND_FILLED: {
            cairo_move_to(cr, x-d, y);
            cairo_line_to(cr, x, y-d);
            cairo_line_to(cr, x+d, y);
            cairo_line_to(cr, x, y+d);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
        }
        case MARK_TRIANGLE_OPEN: {
            cairo_move_to(cr, x-(d+1), y+(d+1)*(sqrt(3)-1));
            cairo_line_to(cr, x+(d+1), y+(d+1)*(sqrt(3)-1));
            cairo_line_to(cr, x, y-(d+1));
            cairo_close_path(cr);
            cairo_stroke(cr);
            break;
        }
        case MARK_TRIANGLE_FILLED: {
            cairo_move_to(cr, x-(d+1), y+(d+1)*(sqrt(3)-1));
            cairo_line_to(cr, x+(d+1), y+(d+1)*(sqrt(3)-1));
            cairo_line_to(cr, x, y-(d+1));
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
        }
        case MARK_SQUARE_OPEN: {
            cairo_move_to(cr, x-(d-1), y-(d-1));
            cairo_line_to(cr, x+(d-1), y-(d-1));
            cairo_line_to(cr, x+(d-1), y+(d-1));
            cairo_line_to(cr, x-(d-1), y+(d-1));
            cairo_close_path(cr);
            cairo_stroke(cr);
            break;
        }
        case MARK_SQUARE_FILLED: {
            cairo_move_to(cr, x-(d-1), y-(d-1));
            cairo_line_to(cr, x+(d-1), y-(d-1));
            cairo_line_to(cr, x+(d-1), y+(d-1));
            cairo_line_to(cr, x-(d-1), y+(d-1));
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
        }
    }
}

/*----------------------------------------------------------------------------*/

void cairox_centre_text(cairo_t *cr, double x0, double y0, PangoLayout *layout, char *label)
{
    CairoxTextParameters tp;

    cairox_text_parameters_set(&tp, x0, y0, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_CENTER, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, label);
}

/*----------------------------------------------------------------------------*/

void cairox_vector_draw(cairo_t *cr, double x0, double y0, Boolean colour, int nf, double *vals)
{
    /* Draw a vector of neuron-like units, centred on (x0,y0) */

    double x, y, w, h;
    int i;

    // Dimensions of units:
    w = 4;
    h = 8;

    // Start location of vector (top left):
    x = x0 - (nf * w * 0.5);
    y = y0 - h/2;

    // Cairo drawing parameters:
    cairo_set_line_width(cr, 1.0);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    // Loop through printing each unit: 
    for (i = 0; i < nf; i++) {
        cairo_rectangle(cr, x, y, w, h);
        // set the colour/shade and draw the unit:
        if (colour) {        
            cairox_select_colour_scale(cr, vals[i]);
        }
        else {
            cairo_set_source_rgb(cr, 1.0-vals[i], 1.0-vals[i], 1.0-vals[i]);
        }
        cairo_fill_preserve(cr);

        // Black surround:
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);
        // X location of next unit:
        x = x + w;
    }
}

/******************************************************************************/
/* SECTION 3: Select a colour for value between zero and one: *****************/
/******************************************************************************/

#if LURID

void cairox_select_colour_scale(cairo_t *cr, double d)
{
    if (d < 0.5) {
        cairo_set_source_rgb(cr, (255-(255-241)*2*(0.5-d))/255.0, (255-(255-163)*2*(0.5-d))/255.0, (255-(255-64)*2*(0.5-d))/255.0);
    }
    else if (d > 0.5) {
        cairo_set_source_rgb(cr, (255-(255-153)*2*(d-0.5))/255.0, (255-(255-142)*2*(d-0.5))/255.0, (255-(255-195)*2*(d-0.5))/255.0);
    }
    else {
        /* 0.5 maps to white: */
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    }
}

#else


static double hsl_index_to_rgb_index(double tc, double p, double q)
{
    if (tc < 0.1666667) {
        return(p + ((q - p) * 6.0 * tc));
    }
    else if (tc < 0.5) {
        return(q);
    }
    else if (tc < 0.6666667) {
        return(p + ((q - p) * 6.0 * (0.6666667 - tc)));
    }
    else {
        return(p);
    }
}

/*----------------------------------------------------------------------------*/

static void hsl_to_rgb(double h, double s, double l, double *r, double *g, double *b)
{
    double q, p, tr, tg, tb;

    q = (l < 0.5) ? l * (s + 1.0) : (l + s - (l * s));
    p = 2.0 * l - q;
    tr = (h > 0.6666667) ? (h - 0.6666667) : h + 0.3333333;
    tg = h;
    tb = (h < 0.3333333) ? (h + 0.6666667) : h - 0.3333333;
    *r = hsl_index_to_rgb_index(tr, p, q);
    *g = hsl_index_to_rgb_index(tg, p, q);
    *b = hsl_index_to_rgb_index(tb, p, q);
}

/*----------------------------------------------------------------------------*/

void cairox_select_colour_scale(cairo_t *cr, double d)
{
    double r, g, b;

    if (d < 0.0) {
        hsl_to_rgb(0.0 * 0.666667, 1.0, 0.5, &r, &g, &b);
    }
    else if (d > 1.0) {
        hsl_to_rgb(1.0 * 0.666667, 1.0, 0.5, &r, &g, &b);
    }
    else {
        hsl_to_rgb(d * 0.666667, 1.0, 0.5, &r, &g, &b);
    }
    cairo_set_source_rgb(cr, r, g, b);
}
#endif

/******************************************************************************/

void cairox_set_colour_by_index(cairo_t *cr, int colour)
{
    switch (colour) {
        case 0: { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
        case 1: { cairo_set_source_rgb(cr, 1.0, 1.0, 0.0); break; }
        case 2: { cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); break; }
        case 3: { cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); break; }
        case 4: { cairo_set_source_rgb(cr, 0.0, 0.0, 1.0); break; }
        case 5: { cairo_set_source_rgb(cr, 1.0, 0.0, 1.0); break; }
        case 6: { cairo_set_source_rgb(cr, 0.0, 1.0, 0.0); break; }
        case 7: { cairo_set_source_rgb(cr, 0.5, 1.0, 0.5); break; }
        case 8: { cairo_set_source_rgb(cr, 0.5, 0.5, 0.5); break; }
        default:  { cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); break; }
    }
}

/******************************************************************************/
/* SECTION 4: Miscellaneous cairo extensions: *********************************/
/******************************************************************************/

CairoxPoint *cairox_pointlist_copy(CairoxPoint *old, int length)
{
    CairoxPoint *new;
    int i;

    if ((new = (CairoxPoint *)malloc(length * sizeof(CairoxPoint))) == NULL) {
        lib_error_report(ERROR_MALLOC_FAILED, "while copying a list of points");
    }
    else {
        for (i = 0; i < length; i++) {
            new[i].x = old[i].x;
            new[i].y = old[i].y;
        }
    }
    return(new);
}

/*----------------------------------------------------------------------------*/

void cairox_pointlist_reverse(CairoxPoint *p, int length)
{
    double tmp;
    int i;

    for (i = 0; i < (length/2); i++) {
        tmp = p[i].x;
        p[i].x = p[length-i-1].x;
        p[length-i-1].x = tmp;

        tmp = p[i].y;
        p[i].y = p[length-i-1].y;
        p[length-i-1].y = tmp;
    }
}

/******************************************************************************/
/* SECTION 5: Miscellaneous pango extensions: *********************************/
/******************************************************************************/

void pangox_layout_select_font_from_string(PangoLayout *layout, char *string)
{
    if ((layout != NULL) && (string != NULL)) {
        PangoFontDescription *font;

        font = pango_font_description_from_string(string);
        pango_layout_set_font_description(layout, font);
        pango_font_description_free(font);
    }
}

/*----------------------------------------------------------------------------*/

void pangox_layout_set_font_face(PangoLayout *layout, const char *family)
{
    // family might be "Sans", "Serif" or "Fixed", etc.

    if ((layout != NULL) && (family != NULL)) {
        const PangoFontDescription *old_font;
        PangoFontDescription *new_font;

        if ((old_font = pango_layout_get_font_description(layout)) == NULL) {
            PangoContext *context = pango_layout_get_context(layout);
            new_font = pango_font_description_copy(pango_context_get_font_description(context));
        }
        else {
            new_font = pango_font_description_copy(old_font);
        }
        pango_font_description_set_family(new_font, family);
        pango_layout_set_font_description(layout, new_font);
        pango_font_description_free(new_font);
    }
}

/*----------------------------------------------------------------------------*/

void pangox_layout_set_font_size(PangoLayout *layout, int size)
{
    if (layout != NULL) {
        const PangoFontDescription *old_font;
        PangoFontDescription *new_font;

        if ((old_font = pango_layout_get_font_description(layout)) == NULL) {
            PangoContext *context = pango_layout_get_context(layout);
            new_font = pango_font_description_copy(pango_context_get_font_description(context));
        }
        else {
            new_font = pango_font_description_copy(old_font);
        }
        pango_font_description_set_size(new_font, size * PANGO_SCALE * 0.71);
        pango_layout_set_font_description(layout, new_font);
        pango_font_description_free(new_font);
    }
}

/*----------------------------------------------------------------------------*/

void pangox_layout_set_font_weight(PangoLayout *layout, PangoWeight weight)
{
    if (layout != NULL) {
        const PangoFontDescription *old_font;
        PangoFontDescription *new_font;

        if ((old_font = pango_layout_get_font_description(layout)) == NULL) {
            PangoContext *context = pango_layout_get_context(layout);
            new_font = pango_font_description_copy(pango_context_get_font_description(context));
        }
        else {
            new_font = pango_font_description_copy(old_font);
        }
        pango_font_description_set_weight(new_font, weight);
        pango_layout_set_font_description(layout, new_font);
        pango_font_description_free(new_font);
    }
}

/*----------------------------------------------------------------------------*/

void pangox_layout_set_font_style(PangoLayout *layout, PangoStyle style)
{
    if (layout != NULL) {
        const PangoFontDescription *old_font;
        PangoFontDescription *new_font;

        if ((old_font = pango_layout_get_font_description(layout)) == NULL) {
            PangoContext *context = pango_layout_get_context(layout);
            new_font = pango_font_description_copy(pango_context_get_font_description(context));
        }
        else {
            new_font = pango_font_description_copy(old_font);
        }
        pango_font_description_set_style(new_font, style);
        pango_layout_set_font_description(layout, new_font);
        pango_font_description_free(new_font);
    }
}

/*----------------------------------------------------------------------------*/

int pangox_font_description_get_font_height(const PangoFontDescription *font)
{
    if (font != NULL) {
        return(1.33 * pango_font_description_get_size(font) / PANGO_SCALE);
    }
    else {
        return(0);
    }
}

/*----------------------------------------------------------------------------*/

int pangox_layout_get_font_height(PangoLayout *layout)
{
    if (layout != NULL) {
        const PangoFontDescription *font = pango_layout_get_font_description(layout);

        return(pangox_font_description_get_font_height(font));
    }
    else {
        return(0);
    }
}

/*----------------------------------------------------------------------------*/

double pangox_layout_get_string_width(PangoLayout *layout, char *text)
{
    if (text == NULL) {
        return(0.0);
    }
    else {
        int w, h;

        pango_layout_set_text(layout, text, strlen(text));
        pango_layout_get_size(layout, &w, &h);
        return(w / (double) PANGO_SCALE);
    }
}

/******************************************************************************/

void cairox_scale_parameters_set(CairoxScaleParameters *ss, GtkPositionType pt, double w, double h, double min, double max, int ticks, char *fmt)
{
    ss->pt = pt;
    ss->w = w;
    ss->h = h;
    ss->min = min;
    ss->max = max;
    ss->ticks = ticks;
    if (ss->fmt != NULL) {
        free(ss->fmt);
    }
    ss->fmt = string_copy(fmt);
}

void cairox_scale_draw(cairo_t *cr, PangoLayout *layout, CairoxScaleParameters *ss, double x0, double y0, Boolean colour)
{
    CairoxTextParameters tp;
    char buffer[16];
    double dx = 0.0, dy = 0.0;
    int i;

    // Bleed box:
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 2.0);
    cairo_rectangle(cr, x0, y0, ss->w, ss->h);
    cairo_stroke(cr);

    switch (ss->pt) {
    case GTK_POS_LEFT: case GTK_POS_RIGHT:
        dy = ss->h / 100.0;
        for (i = 0; i < 100; i++) {
            double val = (i / 99.0);
            cairo_rectangle(cr, x0, y0+i*dy, ss->w, dy);
            if (colour) {
                cairox_select_colour_scale(cr, val);
            }
            else {
                cairo_set_source_rgb(cr, 1.0-val, 1.0-val, 1.0-val);
            }
            cairo_fill(cr);
        }
        break;
    case GTK_POS_TOP: case GTK_POS_BOTTOM: {
        dx = ss->w / 100.0;
        for (i = 0; i < 100; i++) {
            double val = (i / 99.0);
            cairo_rectangle(cr, x0+i*dx, y0, dx, ss->h);
            if (colour) {
                cairox_select_colour_scale(cr, val);
            }
            else {
                cairo_set_source_rgb(cr, 1.0-val, 1.0-val, 1.0-val);
            }
            cairo_fill(cr);
        }
        break;
    }
    }

    pangox_layout_set_font_size(layout, 12);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);

    switch (ss->pt) {
    case GTK_POS_LEFT:
        for (i = 0; i < (ss->ticks+1); i++) {
            // Subtract 0.5 to perfect alignment
            double y = y0-0.5+((ss->h+1)/ss->h) * i*100*dy/(double) (ss->ticks);
            g_snprintf(buffer, 16, ss->fmt, ss->min + (ss->max-ss->min) * (ss->ticks - i) / (double) ss->ticks);
            cairox_text_parameters_set(&tp, x0-5, y, PANGOX_XALIGN_RIGHT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairo_move_to(cr, x0-4, y);
            cairo_line_to(cr, x0+2, y);
            cairo_stroke(cr);
        }
        break;
    case GTK_POS_RIGHT:
        for (i = 0; i < (ss->ticks+1); i++) {
            // Subtract 0.5 to perfect alignment
            double y = y0-0.5+((ss->h+1)/ss->h) * i*100*dy/(double) (ss->ticks);
            g_snprintf(buffer, 16, ss->fmt, ss->min + (ss->max-ss->min) * (ss->ticks - i) / (double) ss->ticks);
            cairox_text_parameters_set(&tp, x0+ss->w+5, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_CENTER, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairo_move_to(cr, x0+ss->w-2, y);
            cairo_line_to(cr, x0+ss->w+4, y);
            cairo_stroke(cr);
        }
        break;
    case GTK_POS_TOP: 
        for (i = 0; i < (ss->ticks+1); i++) {
            double x = x0+((ss->w+1)/ss->w) * i*100*dx/(double) (ss->ticks);
            g_snprintf(buffer, 16, ss->fmt, ss->min + (ss->max-ss->min) * (ss->ticks - i) / (double) ss->ticks);
            cairox_text_parameters_set(&tp, x, y0-5, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairo_move_to(cr, x, y0-4);
            cairo_line_to(cr, x, y0+2);
            cairo_stroke(cr);
        }
        break;
    case GTK_POS_BOTTOM:
        for (i = 0; i < (ss->ticks+1); i++) {
            double x = x0+((ss->w+1)/ss->w) * i*100*dx/(double) (ss->ticks);
            g_snprintf(buffer, 16, ss->fmt, ss->min + (ss->max-ss->min) * (ss->ticks - i) / (double) ss->ticks);
            cairox_text_parameters_set(&tp, x, y0+ss->h+5, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_TOP, 0.0);
            cairox_paint_pango_text(cr, &tp, layout, buffer);

            cairo_move_to(cr, x, y0+ss->h-2);
            cairo_line_to(cr, x, y0+ss->h+4);
            cairo_stroke(cr);
        }
        break;
    }
}

/******************************************************************************/

void cairox_pixel_matrix_draw(cairo_t *cr, double x, double y, double dx, double dy, double pixels[], int n, Boolean colour)
{
    int i, j;

    // Bleed box:
    cairo_set_line_width(cr, 2.0);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_rectangle(cr, x, y, n*dx, n*dy);
    cairo_stroke(cr);

    // The pixels:
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            double val = pixels[j*n+i];
            if ((val >= 0) && (val <= 1)) {
                if (colour) {
                    cairox_select_colour_scale(cr, val);
                }
                else {
                    cairo_set_source_rgb(cr, 1.0-val, 1.0-val, 1.0-val);
                }
                cairo_rectangle(cr, x+j*dx, y+i*dy, dx, dy);
                cairo_fill(cr);
            }
        }
    }
}

/******************************************************************************/

CairoxFontProperties *font_properties_create(char *face, int size, PangoStyle style, PangoWeight weight, char *colour)
{
    CairoxFontProperties *fp;

    if ((fp = (CairoxFontProperties *)malloc(sizeof(CairoxFontProperties))) != NULL) {
        GdkColor text_colour;

        fp->face = string_copy(face);
        fp->size = size;
        fp->style = style;
        fp->weight = weight;
        gdk_color_parse(colour, &text_colour);
        fp->colour = gdk_color_copy(&text_colour);
    }
    return(fp);
}

void font_properties_set_size(CairoxFontProperties *fp, int size)
{
    fp->size = size;
}

void font_properties_destroy(CairoxFontProperties *fp)
{
    if (fp != NULL) {
        if (fp->face != NULL) {
            string_free(fp->face);
        }
        if (fp->colour != NULL) {
            gdk_color_free(fp->colour);
        }
        free(fp);
    }
}

void pangox_layout_set_font_properties(PangoLayout *layout, CairoxFontProperties *fp)
{
    if (fp->face != NULL) {
        pangox_layout_set_font_face(layout, fp->face);
    }
    pangox_layout_set_font_size(layout, fp->size);
    pangox_layout_set_font_style(layout, fp->style);
    pangox_layout_set_font_weight(layout, fp->weight);
}

/******************************************************************************/
/******************************************************************************/

