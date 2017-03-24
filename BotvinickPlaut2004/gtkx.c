/*******************************************************************************

    FILE: gtkx.c
    CONTENTS: Miscellaneous general purpose GTK+ routines

*******************************************************************************/

#include "gtkx.h"
#include <stdio.h>
 
/* X_DRIFT and Y_DRIFT are the window border widths added by the window       */
/* manager. These may be different for different window managers...           */

#define X_DRIFT 0
#define Y_DRIFT 1

/******************************************************************************/

void gtkx_flush_events()
{
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
} 

/******************************************************************************/

void gtkx_position_popup(GtkWidget *caller, GtkWidget *child)
{
    gtk_window_set_position(GTK_WINDOW(child), GTK_WIN_POS_CENTER);
}

/******************************************************************************/

void gtkx_warn(GtkWidget *frame, char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(frame),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_OK,
                                    "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/******************************************************************************/

GtkWidget *gtkx_button_create(GtkWidget *parent, const char *label, void callback(), void *arg)
{
    GtkWidget *button = gtk_button_new_with_label(label);

    if (callback != NULL) {
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(callback), arg);
    }
    gtk_container_add(GTK_CONTAINER(parent), button);
    gtk_widget_show(button);
    return(button);
}

/******************************************************************************/

void gtkx_configure_surface_callback(GtkWidget *widget, GdkEvent *event, cairo_surface_t **surface)
{
    int width = widget->allocation.width;
    int height = widget->allocation.height;

    if (*surface != NULL) {
        cairo_surface_destroy(*surface);
    }
    *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
}

/*----------------------------------------------------------------------------*/

void gtkx_destroy_surface_callback(GtkWidget *widget, cairo_surface_t **surface)
{
    if (*surface != NULL) {
        cairo_surface_destroy(*surface);
    }
}

/******************************************************************************/
