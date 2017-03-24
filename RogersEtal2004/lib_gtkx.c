/*******************************************************************************

    File:       lib_gtkx.c
    Contents:   Miscellaneous general purpose GTK+ routines
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Public procedures:

*******************************************************************************/

/******** Include files: ******************************************************/

#include "lib_gtkx.h"
#include <stdio.h>
 
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
    gtkx_position_popup(frame, dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/******************************************************************************/

int gtkx_viewer_expose(GtkWidget *widget, GdkEventExpose *event, void *data)
{
    GdkPixmap *canvas = g_object_get_data(G_OBJECT(widget), "canvas");

    if (canvas == NULL) {
        canvas = gtkx_canvas_get_blank(widget);
    }

    if (event == NULL) {
        gdk_draw_pixmap(widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    canvas,
                    0, 0, 0, 0,
                    widget->allocation.width, widget->allocation.height);
    }
    else {
        gdk_draw_pixmap(widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    canvas,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);

    }
    return(FALSE);
}

/******************************************************************************/

GdkPixmap *gtkx_canvas_get_blank(GtkWidget *widget)
{
    GdkPixmap *canvas = g_object_get_data(G_OBJECT(widget), "canvas");

    if (canvas != NULL) {
        gdk_pixmap_unref(canvas);
    }
    if (widget->window != NULL) {
        canvas = gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
    }
    else {
        canvas = NULL;
    }
    if (canvas != NULL) {
        gdk_draw_rectangle(canvas, widget->style->white_gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    }
    g_object_set_data(G_OBJECT(widget), "canvas", canvas);
    return(canvas);
}

/******************************************************************************/

GtkWidget *gtkx_stock_image_button_create(GtkWidget *box, const char *stock_id, void callback(), void *arg)
{
    GtkWidget *image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_BUTTON);
    GtkWidget *button = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(callback), arg);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    return(button);
}

/*----------------------------------------------------------------------------*/

GtkWidget *gtkx_button_create(GtkWidget *parent, const char *label, void callback(), void *arg)
{
    GtkWidget *button = gtk_button_new_with_label(label);

    if (callback != NULL) {
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(callback), arg);
    }
    gtk_box_pack_start(GTK_BOX(parent), button, TRUE, TRUE, 0);
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
/* Toolbar extensions: -------------------------------------------------------*/

GtkWidget *gtkx_toolbar_insert_space(GtkToolbar *toolbar, int expand, int position)
{
    GtkToolItem *item;

    item = gtk_separator_tool_item_new();
    gtk_tool_item_set_expand(GTK_TOOL_ITEM(item), expand);
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(item), !expand);
    gtk_toolbar_insert(toolbar, GTK_TOOL_ITEM(item), position);
    gtk_widget_show(GTK_WIDGET(item));
    return(GTK_WIDGET(item));
}

/*----------------------------------------------------------------------------*/

GtkWidget *gtkx_toolbar_insert_stock(GtkToolbar *toolbar, const char *stock_id, GtkTooltips *tips, const char *text, GCallback callback, void *data, int position)
{
    GtkToolItem *item;

    item = gtk_tool_button_new_from_stock(stock_id);
    if ((tips != NULL) && (text != NULL)) {
        gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(item), tips, text, NULL);
    }
    gtk_toolbar_insert(toolbar, GTK_TOOL_ITEM(item), position);
    gtk_widget_show(GTK_WIDGET(item));

    if (callback != NULL) {
        g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(callback), data);
    }

    return(GTK_WIDGET(item));
}

/******************************************************************************/

