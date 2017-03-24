CFLAGS = `pkg-config --cflags gtk+-2.0` -Wall -O2 -g
LIBS =  `pkg-config --libs gtk+-2.0` -lm

CC = gcc
RM = /bin/rm -f

OBJECTS = utils_hub.o lib_string.o lib_maths.o

XOBJECTS = xhub.o xhub_frame.o xhub_explore.o xhub_train.o xhub_lesion.o \
	xhub_patterns.o \
	lib_gtkx.o lib_error.o \
	lib_cairox.o lib_cairoxg_2_2.o lib_cairoxt_2_0.o lib_dendrogram_1_0.o

all:
#	make hub
	make xhub

hub:	$(OBJECTS) hub.o Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) hub.o $(OBJECTS) $(LIBS)

xhub:	$(OBJECTS) $(XOBJECTS) Makefile
	$(RM) $@
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(XOBJECTS) $(LIBS)

clean:
	$(RM) *.o *~ core tmp.* */*~
	$(RM) *.tgz hub xhub

tar:
	make clean
	tar cvfz ~/Dropbox/Source/hub_20160519.tgz *
