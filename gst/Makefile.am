include $(top_srcdir)/Makefile.decl

INCLUDES = \
	-I$(top_srcdir)/src/ \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_EXTRA_CFLAGS) \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS =\
	 -Wall\
	 -Werror \
	 -g \
	 -O0

if ARAVIS_WITH_GSTREAMER

gstplugin_LTLIBRARIES = libgstaravis.la

libgstaravis_la_SOURCES = gstaravis.c gstaravis.h
libgstaravis_la_CFLAGS = $(ARAVIS_EXTRA_CFLAGS)
libgstaravis_la_LIBADD = $(ARAVIS_EXTRA_LIBS)
libgstaravis_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstaravis_la_LIBTOOLFLAGS = --tag=disable-static

libgstaravis_ladir = $(includedir)/aravis

noinst_HEADERS = gstaravis.h

endif
