EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

INCLUDES = \
	-I$(top_srcdir)/src \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_EXTRA_CFLAGS) \
	-DARAVIS_DATA_DIR=\""$(aravis_datadir)"\" \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS = -Wall -lm

bin_PROGRAMS = arv-viewer

arv_viewer_SOURCES = arvviewer.c	\
		     arvfeaturewidget.c

arv_viewer_LDADD = $(ARAVIS_EXTRA_LIBS) $(top_builddir)/src/libaravis.la
arv_viewerdir = $(aravis_datadir)
arv_viewer_DATA = arv-viewer.ui

EXTRA_DIST += $(arv_viewer_DATA)

