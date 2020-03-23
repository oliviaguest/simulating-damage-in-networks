#include "bp.h"
#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>

/******************************************************************************/
/* Main ***********************************************************************/

int main(int argc, char **argv)
{
    extern GtkWidget *make_widgets();
    extern void xgraph_initialise();

    TaskType task = {TASK_COFFEE, DAMAGE_NONE, {TRUE, FALSE, FALSE, FALSE, FALSE}};
    long t0 = (long) time(NULL);
    GtkWidget *frame;

    gtk_init(&argc, &argv);
    srand((int) t0);

    world_initialise(&task);
    if ((frame = make_widgets()) != NULL)  {
        xgraph_initialise();
        gtk_widget_show(frame);
        gtk_main();
    }

    exit(0);
}

/******************************************************************************/
