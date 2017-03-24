# Upgraded for GTK+2.0

CFLAGS = `pkg-config --cflags gtk+-2.0` -Wall -O2
LIBS =  `pkg-config --libs gtk+-2.0` -lm
CC = gcc
RM = /bin/rm -f

OBJECTS = utils_maths.o lib_network.o utils_time.o world.o utils_ps.o

XOBJECTS = xbp.o xframe.o gtkx.o xgraph.o error_analysis.o \
	xnet_diagram.o xnet_test.o xnet_test_2d_viewer.o xnet_test_3d_viewer.o xnet_test_output.o xnet_test_state.o xnet_test_actions.o \
	lib_string.o lib_cairox.o lib_cairoxg_2_3.o lib_error.o \
	xcs_sim1.o xcs_sim1_athu.o xcs_sim1_table4.o \
	xcs_sim2.o xcs_sim2_survival_viewer.o xcs_sim2_subtask_chart.o \
	xcs_sim3_independents_chart.o xcs_sim3_error_frequencies.o xcs_sim3_errors_per_trial.o \
	xcs_sim3.o xcs_sim4.o xcs_sim5_table.o xcs_sim5_graph.o \
	xcs_sim6.o xcs_jb_analysis.o 


all:
	make bp
	make xbp
#	make bp_client

bp:	$(OBJECTS) bp.o Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) bp.o $(OBJECTS) $(LIBS)

xbp:	$(OBJECTS) $(XOBJECTS) Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(XOBJECTS) $(LIBS)

bp_client:	$(OBJECTS) bp_client.o /usr/local/lib/libexpress.a Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) bp_client.o $(OBJECTS) $(LIBS) -lexpress

clean:
	$(RM) *.o *~ core tmp.* */*~
	$(RM) *.tgz bp xbp bp_client

tar:
	make clean
	tar cvfz bp_20130726.tgz *
