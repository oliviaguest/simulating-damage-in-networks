/******************************************************************************/

#include "tyler.h"
#include "lib_gtkx.h"
#include "lib_cairoxg_2_2.h"
#include <string.h>

#define NODE_PIXEL_SIZE 6

/******************************************************************************/

#define PI 3.141592653

#define MAX_PATTERNS 48

typedef struct x_globals {
    GtkWidget        *frame;
    GtkWidget        *training_set_label;
    GtkWidget        *weight_history_label;
    GtkWidget        *weight_history_cycles;
    int               training_step;
    double            conflict[100];
    double            output[100][IO_WIDTH];
    Boolean           pause;
    Boolean           colour;
    Boolean           ddd;
    Boolean           generate;
    int               lesion_type;
    PatternList      *training_set;
    int               pattern_num;
    int               reps;
    Network          *net;
    NetworkParameters pars;
} XGlobals;

extern void test_net_create_widgets(GtkWidget *vbox, XGlobals *xg);
extern void tyler_patterns_create_widgets(GtkWidget *vbox, XGlobals *xg);
extern void test_attractor_create_widgets(GtkWidget *vbox, XGlobals *xg);
extern void weight_histogram_create_widgets(GtkWidget *vbox, XGlobals *xg);
extern void lesion_viewer_create_widgets(GtkWidget *vbox, XGlobals *xg);

extern double vector_maximum_activation(double *vector_out, int width);
extern Boolean net_viewer_repaint(GtkWidget *widget, GdkEvent *event, XGlobals *xg);
extern void net_viewer_create(GtkWidget *vbox, XGlobals *xg);
extern Boolean net_output_viewer_repaint(GtkWidget *widget, GdkEventConfigure *event, XGlobals *xg);
extern void net_output_viewer_create(GtkWidget *vbox, XGlobals *xg);

/******************************************************************************/

extern void xgraph_create(GtkWidget *vbox, XGlobals *xg);
extern void xgraph_initialise(XGlobals *xg);
extern void xgraph_set_error_scores(XGlobals *xg);

extern void action_list_clear();

extern Boolean world_state_viewer_repaint(GtkWidget *widget, GdkEventConfigure *event, void *data);

extern void create_sim1_viewer(GtkWidget *vbox);
extern void create_survival_plot_viewer(GtkWidget *vbox);
extern void create_subtask_chart_viewer(GtkWidget *vbox);
extern void create_independents_chart_viewer(GtkWidget *vbox);
extern void create_error_frequencies_viewer(GtkWidget *vbox);
extern void create_errors_per_trial_viewer(GtkWidget *vbox);

extern void action_list_clear();
extern Boolean world_state_viewer_repaint(GtkWidget *widget, GdkEventConfigure *event, void *data);

extern GtkWidget *make_widgets(XGlobals *xg);
