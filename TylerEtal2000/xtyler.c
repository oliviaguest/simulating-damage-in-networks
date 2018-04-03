
#include "xframe.h"


XGlobals xg =  {
//    GtkWidget        *frame;
                NULL,
//    GtkWidget        *training_set_label;
                NULL,
//    GtkWidget        *weight_history_label;
                NULL,
//    GtkWidget        *weight_history_cycles;
                NULL,
//    int               training_step;
                0,
//    double            conflict[100];
                {},
//    double            output[100][IO_WIDTH];
                {},
//    Boolean           pause;
                FALSE,
//    Boolean           colour;
                FALSE,
//    Boolean           3D view;
                TRUE,
//    Boolean           generate;
                TRUE,
//    int               lesion_type;
                0,
//    PatternList      *training_set;
                NULL,
//    int               pattern_num;
                0,
//    int               number_of_replications;
		0,
//    Network          *net;
		NULL,
//    NetworkParameters pars: NT, WN, LR, Momentum, Decay, Error, Update, Epochs, Criterion
		{NT_FEEDFORWARD, 0.010, 0.250, 0.100, 0.000, SUM_SQUARE_ERROR, UPDATE_BY_EPOCH, 1000, 0.01}
#ifdef CLAMPED
                //	{NT_RECURRENT, 0.010, 0.025, 0.100, 0.000, SUM_SQUARE_ERROR, UPDATE_BY_EPOCH, 1000, 0.01}
#else
                //	{NT_RECURRENT, 0.010, 0.025, 0.100, 0.000, SUM_SQUARE_ERROR, UPDATE_BY_EPOCH, 5000, 0.01}
#endif
};

/******************************************************************************/
/* Main ***********************************************************************/

int main(int argc, char **argv)
{
    long t0 = (long) time(NULL);
    GtkWidget *frame;

    gtk_set_locale();
    gtk_init(&argc, &argv);
    srand((int) t0);

    if ((frame = make_widgets(&xg)) != NULL)  {
        xgraph_initialise(&xg);
        gtk_widget_show(frame);
        gtk_main();
    }
    exit(0);
}

/******************************************************************************/

