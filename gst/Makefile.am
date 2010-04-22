INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_CFLAGS) \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS =\
	 -Wall\
	 -Werror \
	 -g \
	 -O0

if ARAVIS_WITH_GSTREAMER

lib_LTLIBRARIES = libgstaravis.la

libgstaravis_la_LIBADD =			\
	$(ARAVIS_LIBS)

libgstaravis_la_SOURCES =			\
	gstaravis.c

libgstaravis_la_HEADERS =			\
	gstaravis.h

libgstaravis_ladir = $(includedir)/aravis

endif
