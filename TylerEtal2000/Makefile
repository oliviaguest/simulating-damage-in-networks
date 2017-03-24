
CFLAGS = `pkg-config --cflags gtk+-2.0` -Wall -O2 -g
LIBS =  `pkg-config --libs gtk+-2.0` -lm

CC = gcc
RM = /bin/rm -f

OBJECTS = world.o lib_maths.o lib_network_io.o utils_network.o lib_string.o

XOBJECTS = xtyler.o xframe.o lib_gtkx.o x_graph.o x_lesion_viewer.o \
	xnet_test.o xnet_test_viewer.o xnet_test_attractor.o \
	x_weight_histogram.o x_patterns.o \
	lib_cairox.o lib_cairoxg_2_2.o lib_error.o lib_dendrogram_1_0.o

all:
	make tyler
	make xtyler
#	make tyler_client

tyler:	$(OBJECTS) tyler.o Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) tyler.o $(OBJECTS) $(LIBS)

xtyler:	$(OBJECTS) $(XOBJECTS) Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(XOBJECTS) $(LIBS)

tyler_client:	$(OBJECTS) tyler_client.o /usr/local/lib/libexpress.a Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) tyler_client.o $(OBJECTS) $(LIBS) -lexpress

clean:
	$(RM) *.o *~ core tmp.* */*~
	$(RM) *.tgz tyler xtyler tyler_client

tar:
	make clean
	tar cvfz ~/Dropbox/Source/tyler_20160519.tgz *
