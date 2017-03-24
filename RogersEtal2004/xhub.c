/*******************************************************************************

    File:       xhub.c
    Contents:   X interface to hub-and-spoke reimplementation (2016 version)
    Created:    03/11/16
    Author:     Rick Cooper
    Copyright (c) 2016 Richard P. Cooper

    Functions in this file define the top-level (main) function, which creates
    the widget, initialises the model and then starts the GTK main event loop.

    Public procedures:
        int main(int argc, char **argv)

*******************************************************************************/
/******** Include files: ******************************************************/

#include "xhub.h"

/******** Declared initialised variables: *************************************/

XGlobals xg = {
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL
};

static NetworkParameters params = { // Parameter values from PDPTOOL
    0.125,      // Initial weight distribution
    0.001,      // Learning rate
    0.0,        // Momentum for learning
    0.0001,     // Weight decay (per epoch)
    UI_FIXED,   // Unit initialisation per pattern
    4,
    7,
    0.001,
    EF_CROSS_ENTROPY,
    WU_BY_ITEM,
    1000,
    0.05};

/******************************************************************************/
/* Main ***********************************************************************/

int main(int argc, char **argv)
{
    long t0 = (long) time(NULL);

    gtk_set_locale();
    gtk_init(&argc, &argv);
    srand((int) t0);

    /* Create the network ... */
    xg.net = network_create(NT_RECURRENT, NUM_IO, NUM_SEMANTIC, NUM_IO);
    network_parameters_set(xg.net, &params);

    /* Create the widgets and start the GTK+ interface */
    if (xg_make_widgets(&xg))  {
        xg_initialise(&xg);
        gtk_widget_show(xg.frame);
        gtk_main();
    }

    /* Tidy up and exit with success: */
    network_destroy(xg.net);
    xg.net = NULL;

    exit(0);
}

/******************************************************************************/

