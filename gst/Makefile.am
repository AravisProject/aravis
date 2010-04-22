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

lib_LTLIBRARIES = libaravis-gst.la

libaravis_gst_la_LIBADD =			\
	$(ARAVIS_LIBS)

libaravis_gst_la_SOURCES =			\
	arvgst.c

libaravis_gst_la_HEADERS =			\
	arvgst.h

libaravis_gst_ladir = $(includedir)/aravis

endif
