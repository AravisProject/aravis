EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

INCLUDES = \
	-I$(top_srcdir)/src \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_VIEWER_CFLAGS) \
	-DARAVIS_DATA_DIR=\""$(aravis_datadir)"\" \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS = -Wall -lm

bin_PROGRAMS = arv-viewer-@ARAVIS_API_VERSION@

arv_viewer_@ARAVIS_API_VERSION@_SOURCES = arvviewer.c	\
		     arvfeaturewidget.c \
		     arvfeaturewidget.h

arv_viewer_@ARAVIS_API_VERSION@_LDADD = $(ARAVIS_VIEWER_LIBS) $(top_builddir)/src/libaravis-@ARAVIS_API_VERSION@.la
arv_viewer_@ARAVIS_API_VERSION@dir = $(aravis_datadir)
arv_viewer_@ARAVIS_API_VERSION@_DATA = arv-viewer.ui

EXTRA_DIST += $(arv_viewer_@ARAVIS_API_VERSION@_DATA)

