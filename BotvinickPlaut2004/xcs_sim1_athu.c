
#include "xframe.h"
#include "lib_cairox_2_0.h"
#include "xcs_sequences.h"
#include "utils_maths.h"

extern int categorise_action_sequence(ActionType *sequence);
extern void draw_3d_vector(cairo_t *cr, double *vector, int width, int x, int y);

/******************************************************************************/

char *task_name[3] = {"No instruction", "Prepare Coffee", "Prepare Tea"};

/******************************************************************************/

static GtkWidget *viewer_widget = NULL;      // The canvas
static cairo_surface_t *viewer_surface = NULL; // A surface for dobule-buffering
static TaskType athu_task = {TASK_COFFEE, DAMAGE_NONE, {TRUE, FALSE, FALSE, FALSE, FALSE}}; // Current task
static int athu_variant = SEQ_COFFEE1;   // Current variant of the task
static int athu_step = 0;                // Step of interest in current variant of task
static int athu_vc = 0;                  // Vector count
static int athu_st = 37;                 // Vector width -- number of steps in task
static double athu_hu[38*50*100];        // Array of hidden units
static double athu_variability[38];      // Variability of hidden units at each step

/******************************************************************************/

static Boolean athu_viewer_expose(GtkWidget *widt, GdkEventConfigure *event, void *data)
{
    if (!GTK_WIDGET_MAPPED(viewer_widget) || (viewer_surface == NULL)) {
        return(TRUE);
    }
    else {
        PangoLayout *layout;
        cairo_t *cr;
        CairoxTextParameters p;
        int height = viewer_widget->allocation.height;
        int width  = viewer_widget->allocation.width;
        char buffer[64];
        int i, x, y;
        int x_pos = width - 260;

        cr = cairo_create(viewer_surface);
        layout = pango_cairo_create_layout(cr);
        pangox_layout_set_font_size(layout, 12);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);

        // x_pos is the x coordinate of the text giving the stats. It would
        // be nice if you could set this through some menu option

        for (i = athu_vc; i > 0; i--) {
            x = (int) width/2.0 - 13.0 * (i-athu_vc/2) * cos(M_PI/6.0);
            y = (int) height/2.0 + 25.0 - 13.0 * (i-athu_vc/2) * sin(M_PI/6.0);
            draw_3d_vector(cr, &athu_hu[((i-1) * (athu_st+1) + athu_step) * HIDDEN_WIDTH], HIDDEN_WIDTH, x, y);
        }

        g_snprintf(buffer, 64, "%s", variant_name[athu_variant]);
        cairox_text_parameters_set(&p,  x_pos, height-35, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, buffer);

        g_snprintf(buffer, 64, "Step %d of %d; %d vectors", athu_step, athu_st, athu_vc);
        cairox_text_parameters_set(&p,  x_pos, height-20, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, buffer);

        g_snprintf(buffer, 64, "Variability: %f", athu_variability[athu_step]);
        cairox_text_parameters_set(&p,  x_pos, height-5, PANGOX_XALIGN_LEFT, PANGOX_YALIGN_BOTTOM, 0.0);
        cairox_paint_pango_text(cr, &p, layout, buffer);

        g_object_unref(layout);
        cairo_destroy(cr);

        /* Now copy the surface to the window: */
        if ((viewer_widget->window != NULL) && ((cr = gdk_cairo_create(viewer_widget->window)) != NULL)) {
            cairo_set_source_surface(cr, viewer_surface, 0, 0);
            cairo_paint(cr);
            cairo_destroy(cr);
        }
        return(FALSE);
    }
}

/******************************************************************************/

static void reset_athu_data()
{
    athu_step = 0;
    athu_vc = 0;
    athu_st = (athu_task.base == TASK_COFFEE) ? 37 : 20;
}

static void athu_set_task_callback(GtkWidget *mi, void *dummy)
{
    athu_variant = (long) dummy;
    athu_task.base = (athu_variant < SEQ_TEA1) ? TASK_COFFEE : TASK_TEA;
    athu_task.initial_state.bowl_closed = pars.sugar_closed;
    athu_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    athu_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    athu_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    athu_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    reset_athu_data();
    athu_viewer_expose(NULL, NULL, NULL);
}

static void athu_collect_data_callback(GtkWidget *mi, void *dummy)
{
    double *vector_in = (double *)malloc(IN_WIDTH * sizeof(double));
    double *vector_out = (double *)malloc(OUT_WIDTH * sizeof(double));
    int step, count, j, attempts;
    double *tmp;

    athu_task.initial_state.bowl_closed = pars.sugar_closed;
    athu_task.initial_state.mug_contains_coffee = pars.mug_with_coffee; 
    athu_task.initial_state.mug_contains_tea = pars.mug_with_tea;
    athu_task.initial_state.mug_contains_cream = pars.mug_with_cream;
    athu_task.initial_state.mug_contains_sugar = pars.mug_with_sugar; 
    reset_athu_data();
    for (attempts = 0; attempts < 100; attempts++) {
        ActionType this[100];
        step = 0;
        count = 0;
        world_initialise(&athu_task);
        network_tell_randomise_hidden_units(xg.net);
        network_ask_hidden(xg.net, &athu_hu[(athu_vc*(athu_st+1)+step++)*HIDDEN_WIDTH]);
        do {
            world_set_network_input_vector(vector_in);
            network_tell_input(xg.net, vector_in);
            network_tell_propagate2(xg.net);
            network_ask_hidden(xg.net, &athu_hu[(athu_vc*(athu_st+1)+step)*HIDDEN_WIDTH]);
            network_ask_output(xg.net, vector_out);
            this[count] = world_get_network_output_action(NULL, vector_out);
            world_perform_action(this[count]);
            step++;
        } while ((this[count] != ACTION_SAY_DONE) && (++count < 100));

        if (categorise_action_sequence(this) == athu_variant) {
            athu_vc++;
        }
    }

    free(vector_in);
    free(vector_out);

    // Now work out the variability: TO DO!

    tmp = (double *)malloc(athu_vc * HIDDEN_WIDTH * sizeof(double));
    for (step = 0; step <= athu_st; step++) {
        for (count = 0; count < athu_vc; count++) {
            for (j = 0; j < HIDDEN_WIDTH; j++) {
                tmp[j + count * HIDDEN_WIDTH] = athu_hu[j+(count*(athu_st+1)+step)*HIDDEN_WIDTH];
            }
        }
        athu_variability[step] = vector_set_variability(athu_vc, HIDDEN_WIDTH, tmp);
    }
    free(tmp);

    athu_viewer_expose(NULL, NULL, NULL);
}

static void athu_clear_callback(GtkWidget *mi, void *dummy)
{
    reset_athu_data();
    athu_viewer_expose(NULL, NULL, NULL);
}

static void athu_step_back_callback(GtkWidget *mi, void *dummy)
{
    if (athu_step > 0) {
        athu_step--;
        athu_viewer_expose(NULL, NULL, NULL);
    }
    else {
        gtkx_warn(xg.frame, "Cannot step back: Already at bginning of task");
    }
}

static void athu_step_fwrd_callback(GtkWidget *mi, void *dummy)
{
    if (athu_step > (athu_st-1)) {
        gtkx_warn(xg.frame, "Cannot step forward: Already at end of task");
    }
    else {
        athu_step++;
        athu_viewer_expose(NULL, NULL, NULL);
    }
}

static void athu_step_fback_callback(GtkWidget *mi, void *dummy)
{
    athu_step = 0;
    athu_viewer_expose(NULL, NULL, NULL);
}

static void athu_step_ffwrd_callback(GtkWidget *mi, void *dummy)
{
    athu_step = athu_st;
    athu_viewer_expose(NULL, NULL, NULL);
}

static void callback_toggle_colour(GtkWidget *button, void *data)
{
    xg.colour = (Boolean) (GTK_TOGGLE_BUTTON(button)->active);
    athu_viewer_expose(NULL, NULL, NULL);
}

void create_attractor_viewer(GtkWidget *vbox)
{
    GtkWidget *page = gtk_vbox_new(FALSE, 0);
    GtkWidget *menubar, *mi, *menu, *tmp, *toolbar, *hbox;

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(hbox), menubar, FALSE, FALSE, 0);
    gtk_widget_show(menubar);

    menu = gtk_menu_new();
    tmp = gtk_menu_item_new_with_label("Grounds -> Sugar (pack) -> Cream -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_COFFEE1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Grounds -> Sugar (bowl) -> Cream -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_COFFEE2);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Grounds -> Cream -> Sugar (pack) -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_COFFEE3);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Grounds -> Cream -> Sugar (bowl) -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_COFFEE4);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Grounds -> Sugar (bowl open) -> Cream -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_COFFEE5);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Grounds -> Cream -> Sugar (bowl open) -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_COFFEE6);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Teabag -> Sugar (pack) -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_TEA1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Teabag -> Sugar (bowl) -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_TEA2);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);
    tmp = gtk_menu_item_new_with_label("Teabag -> Sugar (bowl open) -> Drink");
    g_signal_connect(G_OBJECT(tmp), "activate", G_CALLBACK(athu_set_task_callback), (void *)SEQ_TEA3);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), tmp);
    gtk_widget_show(tmp);

    mi = gtk_menu_item_new_with_label("Task");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), mi);
    gtk_widget_show(mi);

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
    gtk_widget_show(toolbar);

    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Collect Data", NULL, NULL, NULL, G_CALLBACK(athu_collect_data_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Clear", NULL, NULL, NULL, G_CALLBACK(athu_clear_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "| <", NULL, NULL, NULL, G_CALLBACK(athu_step_fback_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), " < ", NULL, NULL, NULL, G_CALLBACK(athu_step_back_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), " > ", NULL, NULL, NULL, G_CALLBACK(athu_step_fwrd_callback), NULL);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "> |", NULL, NULL, NULL, G_CALLBACK(athu_step_ffwrd_callback), NULL);

    /* Widgets for setting colour/greyscale: */
    /*--- The checkbox: ---*/
    tmp = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp), xg.colour);
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    g_signal_connect(G_OBJECT(tmp), "toggled", G_CALLBACK(callback_toggle_colour), tmp);
    gtk_widget_show(tmp);
    /*--- The label: ---*/
    tmp = gtk_label_new("Colour:");
    gtk_box_pack_end(GTK_BOX(hbox), tmp, FALSE, FALSE, 5);
    gtk_widget_show(tmp);

    viewer_widget = gtk_drawing_area_new();
    g_signal_connect(G_OBJECT(viewer_widget), "expose_event", G_CALLBACK(athu_viewer_expose), NULL);
    g_signal_connect(G_OBJECT(viewer_widget), "configure_event", G_CALLBACK(gtkx_configure_surface_callback), &viewer_surface);
    g_signal_connect(G_OBJECT(viewer_widget), "destroy", G_CALLBACK(gtkx_destroy_surface_callback), &viewer_surface);
    gtk_widget_set_events(viewer_widget, GDK_EXPOSURE_MASK);
    gtk_box_pack_start(GTK_BOX(page), viewer_widget, TRUE, TRUE, 0);
    gtk_widget_show(viewer_widget);

    gtk_container_add(GTK_CONTAINER(vbox), page);
    gtk_widget_show(page);
}

/******************************************************************************/
