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

gstplugin_LTLIBRARIES = libgstaravis-@ARAVIS_API_VERSION@.la

libgstaravis_@ARAVIS_API_VERSION@_la_SOURCES = gstaravis.c gstaravis.h
libgstaravis_@ARAVIS_API_VERSION@_la_CFLAGS = $(ARAVIS_EXTRA_CFLAGS)
libgstaravis_@ARAVIS_API_VERSION@_la_LIBADD = $(ARAVIS_EXTRA_LIBS)
libgstaravis_@ARAVIS_API_VERSION@_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstaravis_@ARAVIS_API_VERSION@_la_LIBTOOLFLAGS = --tag=disable-static

libgstaravis_@ARAVIS_API_VERSION@_ladir = $(includedir)/aravis-@ARAVIS_API_VERSION@

noinst_HEADERS = gstaravis.h

endif
