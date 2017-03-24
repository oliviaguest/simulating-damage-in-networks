#ifndef _xhub_h_

#define _xhub_h_

#include "hub.h"
#include <gtk/gtk.h>
#include "lib_gtkx.h"

typedef struct xglobals {
    GtkWidget        *frame;
    GtkWidget        *label_pattern_set;
    GtkWidget        *label_weight_history;
    GtkWidget        *label_training_cycles;
    int               training_step;
    char             *pattern_set_name;
    PatternList      *pattern_set;
    PatternList      *current_pattern;
    Network          *net;
} XGlobals;

extern Boolean weight_file_read(XGlobals *xg, char *filename);
extern Boolean xpattern_set_read(XGlobals *xg, char *file);
extern Boolean xg_make_widgets(XGlobals *xg);
extern void    xg_initialise(XGlobals *xg);

extern Boolean pattern_file_load_from_folder(XGlobals *xg, char *subfolder);

extern void populate_weight_history_page(GtkWidget *page, XGlobals *xg);
extern void populate_parameters_page(GtkWidget *page, XGlobals *xg);
extern void hub_pattern_set_widgets(GtkWidget *page, XGlobals *xg);
extern void hub_train_create_widgets(GtkWidget *page, XGlobals *xg);
extern void hub_explore_create_widgets(GtkWidget *page, XGlobals *xg);
extern void hub_lesion_create_widgets(GtkWidget *page, XGlobals *xg);

extern void hub_explore_initialise_network(XGlobals *xg);

#endif
