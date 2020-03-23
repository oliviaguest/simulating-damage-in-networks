
#include "xframe.h"
#include "lib_cairox_2_0.h"

static GtkWidget *viewer_widget = NULL;
static cairo_surface_t *viewer_surface = NULL;

#define DX  8.0
#define DY 12.0

#define FONT_SIZE 20

/******************************************************************************/
/* The 2D network state view drawing functions ********************************/

static void draw_2d_vector(cairo_t *cr, double *vector, int width, double x, double y)
{
    // The size of the rectangles for each unit: */
    int i;

    /* x and y are the centre of the vector ... apply offsets: */

    x = x - (width * 0.5 * DX);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1.0);

    for (i = 0; i < width; i++) {
        double x0 = x + (i - 0.5) * DX;
        double y0 = y - 0.5 * DY;

        if (xg.colour) {
            /* The bar's colour: */
            cairox_select_colour_scale(cr, 1.0 - vector[i]);
        }
        else {
            /* The bar's greyscale: */
            cairo_set_source_rgb(cr, (1.0 - vector[i]), (1.0 - vector[i]), (1.0 - vector[i]));
        }

        /* The unit's rectangle: */
        cairo_rectangle(cr, x0, y0, DX, DY);
        cairo_fill_preserve(cr);
        /* The outline: */
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_stroke(cr);
    }
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
}

/******************************************************************************/

static void nullify_vector(double *vector, int width)
{
    int i;

    for (i = 0; i < width; i++) {
        vector[i] = 0.0;
    }
}

/******************************************************************************/

static void cr_diagram_write(cairo_t *cr, int width, int height)
{
    double in_vector[IN_WIDTH];
    double hidden_vector[HIDDEN_WIDTH];
    double out_vector[OUT_WIDTH];
    PangoLayout *layout;
    CairoxTextParameters tp;
    CairoxArrowParameters ap;
    CairoxPoint coordinates[8]; 
    double x, y, dl, xc;
    char buffer[128];

    layout = pango_cairo_create_layout(cr);
    pangox_layout_set_font_size(layout, FONT_SIZE);
    pangox_layout_set_font_face(layout, "Sans");

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    /* Work out the inter-layer spacing: */

    dl = height/3.0;
    xc = width/2.0 + 100; // Offset the entire diagram to the right

    /* The various vectors: */

    nullify_vector(in_vector, IN_WIDTH);
    x = xc;
    y = height/2.0 - dl;
    draw_2d_vector(cr, in_vector, IN_WIDTH, x, y);

    nullify_vector(hidden_vector, HIDDEN_WIDTH);
    x = xc;
    y = height/2.0;
    draw_2d_vector(cr, hidden_vector, HIDDEN_WIDTH, x, y);

    nullify_vector(out_vector, OUT_WIDTH);
    x = xc;
    y = height/2.0 + dl;
    draw_2d_vector(cr, out_vector, OUT_WIDTH, x, y);

    /* Arrows to indicate network connectivity - Input to hidden: */

    coordinates[0].x = xc;
    coordinates[0].y = height/2.0 - dl + DY;
    coordinates[1].x = xc;
    coordinates[1].y = height/2.0 - DY;

    cairox_arrow_parameters_set(&ap, 3.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &ap);

    /* Arrows to indicate network connectivity - Hidden to output: */

    coordinates[0].x = xc;
    coordinates[0].y = height/2.0 + DY;
    coordinates[1].x = xc;
    coordinates[1].y = height/2.0 + dl - DY;

    cairox_arrow_parameters_set(&ap, 3.0, LS_SOLID, VS_STRAIGHT, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 2, &ap);

    /* Arrows to indicate network connectivity - Hidden to Hiden: */

    coordinates[0].x = xc;
    coordinates[0].y = height/2.0 + DY;
    coordinates[1].x = xc;
    coordinates[1].y = height/2.0 + DY + 60;
    coordinates[2].x = xc + 220;
    coordinates[2].y = height/2.0 + DY + 60;
    coordinates[3].x = xc + 220;
    coordinates[3].y = height/2.0 + DY;
    coordinates[4].x = xc + 220;
    coordinates[4].y = height/2.0 - DY;
    coordinates[5].x = xc + 220;
    coordinates[5].y = height/2.0 - DY - 60;
    coordinates[6].x = xc;
    coordinates[6].y = height/2.0 - DY - 60;
    coordinates[7].x = xc;
    coordinates[7].y = height/2.0 - DY;

    cairox_arrow_parameters_set(&ap, 3.0, LS_SOLID, VS_CURVED, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 8, &ap);

    /* Write the fixated object at the top left: */

    x = xc - DX * 0.5 + 10;
    y = height/2.0 - dl - 0.5 * DY - 1;

    cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    g_snprintf(buffer, 128, "%d Input Units", IN_WIDTH);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    /* Write the hidden unit label: */

    x = xc - DX * 0.5 + 10;
    y = height/2.0 - 0.5 * DY - 1;

    cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    g_snprintf(buffer, 128, "%d Hidden Units", HIDDEN_WIDTH);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    /* Write the action beneath the network: */

    x = xc - DX * 0.5 + 10;
    y = height/2.0 + dl - 0.5 * DY - 1;

    cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
    g_snprintf(buffer, 128, "%d Output / Action Units", OUT_WIDTH);
    cairox_paint_pango_text(cr, &tp, layout, buffer);

    /* Draw the environment loop: */

    coordinates[0].x = xc;
    coordinates[0].y = height/2.0 + dl + DY/2.0;
    coordinates[1].x = xc;
    coordinates[1].y = height/2.0 + dl + DY/2.0 + 50;
    coordinates[2].x = xc - 380;
    coordinates[2].y = height/2.0 + dl + DY/2.0 + 50;
    coordinates[3].x = xc - 380;
    coordinates[3].y = height/2.0 + dl + DY/2.0;
    coordinates[4].x = xc - 380;
    coordinates[4].y = height/2.0 + DY/2.0;

    cairox_arrow_parameters_set(&ap, 1.0, LS_DASHED, VS_CURVED, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 5, &ap);

//    x = xc - 300;
//    y = height/2.0;
    cairox_text_parameters_set(&tp, xc - 380, height/2.0 + 6, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "The\nEnvironment");

    coordinates[0].x = xc - 380;
    coordinates[0].y = height/2.0 - DY/2.0 - 2*FONT_SIZE;
    coordinates[1].x = xc - 380;
    coordinates[1].y = height/2.0 - dl - DY/2.0;
    coordinates[2].x = xc - 380;
    coordinates[2].y = height/2.0 - dl - DY/2.0 - 50;
    coordinates[3].x = xc;
    coordinates[3].y = height/2.0 - dl - DY/2.0 - 50;
    coordinates[4].x = xc;
    coordinates[4].y = height/2.0 - dl - DY/2.0;

    cairox_arrow_parameters_set(&ap, 1.0, LS_DASHED, VS_CURVED, AH_SHARP, 1.0, FALSE);
    cairox_paint_arrow(cr, coordinates, 5, &ap);

    pangox_layout_set_font_size(layout, FONT_SIZE*0.9);
    x = xc - 190;
    y = height/2.0-dl-52-DY/2.0;
    cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Sensory Input");
    x = xc - 190;
    y = height/2.0+dl+48+DY/2.0;
    cairox_text_parameters_set(&tp, x, y, PANGOX_XALIGN_CENTER, PANGOX_YALIGN_BOTTOM, 0.0);
    cairox_paint_pango_text(cr, &tp, layout, "Action Output");

    g_object_unref(layout);
}

/*----------------------------------------------------------------------------*/

Boolean diagram_viewer_expose(GtkWidget *widget, GdkEvent *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        int width  = viewer_widget->allocation.width;
        int height = viewer_widget->allocation.height;
        char filename[64];
        cairo_surface_t *pdf_surface;
        cairo_t *cr;

        cr = cairo_create(viewer_surface);
        cr_diagram_write(cr, width, height);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((viewer_widget->window != NULL) && ((cr = gdk_cairo_create(viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }

// HERE!
        width  = 700;
        height = 450;

        /* Write PNG file: */
        g_snprintf(filename, 64, "%snetwork_diagram.png", OUTPUT_FOLDER);
        cairo_surface_write_to_png(viewer_surface, filename);

        /* Write PDF file: */
        g_snprintf(filename, 64, "%snetwork_diagram.pdf", OUTPUT_FOLDER);
        pdf_surface = cairo_pdf_surface_create(filename, width, height);
        cr = cairo_create(pdf_surface);
        cr_diagram_write(cr, width, height);
        cairo_destroy(cr);
        cairo_surface_destroy(pdf_surface);
    }

    return(FALSE);
}

/******************************************************************************/

GtkWidget *xnet_diagram_create()
{
    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(diagram_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_show(viewer_widget);
    return(viewer_widget);
}

/******************************************************************************/
