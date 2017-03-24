#ifndef _lib_cairox_h_
#define _lib_cairox_h_

#include <gtk/gtk.h>
#include <cairo-pdf.h>

typedef struct cairox_point {
    double x, y;
} CairoxPoint;

typedef struct cairox_colour {
    double r, g, b;
} CairoxColour;

typedef enum pango_x_alignment {
    PANGOX_XALIGN_LEFT, PANGOX_XALIGN_CENTER, PANGOX_XALIGN_RIGHT
} PangoXAlignment;

typedef enum pango_y_alignment {
    PANGOX_YALIGN_TOP, PANGOX_YALIGN_CENTER, PANGOX_YALIGN_BOTTOM
} PangoYAlignment;

typedef enum arrow_head_type {
    AH_NONE, AH_SHARP, AH_BLUNT, AH_WIDE, AH_CIRCLE
} ArrowHeadType;

typedef enum vertex_style {
    VS_STRAIGHT, VS_CURVED
} VertexStyle;

typedef enum line_style {
    LS_SOLID, LS_DASHED, LS_DOTTED
} LineStyle;

typedef struct cairox_text_parameters {
    double x;
    double y;
    PangoXAlignment x_align;
    PangoYAlignment y_align;
    double angle;
    CairoxColour foreground;
} CairoxTextParameters;

typedef struct cairox_line_parameters {
    double     width;
    LineStyle  line_style;
    Boolean    antialias;
} CairoxLineParameters;

typedef struct cairox_arrow_parameters {
    double width;
    LineStyle line_style;
    VertexStyle vertex_style;
    ArrowHeadType head1;
    ArrowHeadType head2;
    double density;
} CairoxArrowParameters;

extern double line_calculate_slope(CairoxPoint *p1, CairoxPoint *p2);
extern void line_displace(CairoxPoint *p1, CairoxPoint *p2, double offset);

extern void cairox_set_colour_by_index(cairo_t *cr, int colour);
extern void cairox_select_colour_scale(cairo_t *cr, double value);

extern void cairox_text_parameters_set(CairoxTextParameters *p, double x, double y, PangoXAlignment x_align, PangoYAlignment y_align, double angle);
extern void cairox_text_parameters_set_foreground(CairoxTextParameters *p, double r, double g, double b);
extern void cairox_arrow_parameters_set(CairoxArrowParameters *p, double width, LineStyle ls, VertexStyle vs, ArrowHeadType head, double density, Boolean bidirectional);
extern void cairox_line_parameters_set(CairoxLineParameters *p, double width, LineStyle ls, Boolean antialias);
extern double cairox_paint_text_in_layout(cairo_t *cr, int x, int y, PangoLayout *layout, char *text);
extern void cairox_paint_pango_text(cairo_t *cr, CairoxTextParameters *p, PangoLayout *layout, const char *buffer);
extern void cairox_paint_pixbuf(cairo_t *cr, GdkPixbuf *pb, int x0, int y0);
extern void cairox_paint_line(cairo_t *cr, CairoxLineParameters *p, double x1, double y1, double x2, double y2);
extern void cairox_paint_lines(cairo_t *cr, CairoxPoint *p, int n);
extern void cairox_paint_polygon(cairo_t *cr, CairoxPoint *p, int n);
extern void cairox_paint_circle(cairo_t *cr, double x1, double y1, double r);
extern void cairox_paint_arrow(cairo_t *cr, CairoxPoint *points, int n, CairoxArrowParameters *params);
extern void cairox_paint_grid(cairo_t *cr, int w, int h, double gs);

extern CairoxPoint *cairox_pointlist_copy(CairoxPoint *p, int length);
extern void cairox_pointlist_reverse(CairoxPoint *p, int length);

extern void pangox_layout_select_font_from_string(PangoLayout *layout, char *string);
extern void pangox_layout_set_font_face(PangoLayout *layout, const char *family);
extern void pangox_layout_set_font_weight(PangoLayout *layout, PangoWeight weight);
extern void pangox_layout_set_font_style(PangoLayout *layout, PangoStyle style);
extern void pangox_layout_set_font_size(PangoLayout *layout, int size);

extern int pangox_font_description_get_font_height(const PangoFontDescription *font);

extern int  pangox_layout_get_font_height(PangoLayout *layout);
extern double pangox_layout_get_string_width(PangoLayout *layout, char *text);

#endif
