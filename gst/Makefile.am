include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/ \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_GST_PLUGIN_CFLAGS) \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS = -Wall

gstplugin_LTLIBRARIES = libgstaravis-@ARAVIS_API_VERSION@.la

libgstaravis_@ARAVIS_API_VERSION@_la_SOURCES = gstaravis.c gstaravis.h
libgstaravis_@ARAVIS_API_VERSION@_la_CFLAGS = $(ARAVIS_GST_PLUGIN_CFLAGS)
libgstaravis_@ARAVIS_API_VERSION@_la_LIBADD =		\
	$(ARAVIS_GST_PLUGIN_LIBS) 			\
	$(top_builddir)/src/libaravis-@ARAVIS_API_VERSION@.la
libgstaravis_@ARAVIS_API_VERSION@_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstaravis_@ARAVIS_API_VERSION@_la_LIBTOOLFLAGS = --tag=disable-static

libgstaravis_@ARAVIS_API_VERSION@_ladir = $(includedir)/aravis-@ARAVIS_API_VERSION@

noinst_HEADERS = gstaravis.h

