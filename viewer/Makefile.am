EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

SUBDIRS = data

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_VIEWER_CFLAGS) \
	-DARAVIS_DATA_DIR=\""$(aravis_datadir)"\" \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS = -Wall

bin_PROGRAMS = arv-viewer

arv_viewer_SOURCES = arvviewer.c

arv_viewer_LDADD = $(ARAVIS_VIEWER_LIBS) $(top_builddir)/src/libaravis-@ARAVIS_API_VERSION@.la
arv_viewerdir = $(aravis_datadir)
arv_viewer_DATA = arv-viewer.ui

EXTRA_DIST += arv-viewer.ui

iconthemedir = $(datadir)/icons/hicolor
icondir = ./icons/gnome

appsicon22dir = $(iconthemedir)/22x22/apps
appsicon32dir = $(iconthemedir)/32x32/apps
appsicon48dir = $(iconthemedir)/48x48/apps
appsicon256dir = $(iconthemedir)/256x256/apps

dist_appsicon22_DATA = $(icondir)/22x22/apps/aravis.png
dist_appsicon32_DATA = $(icondir)/32x32/apps/aravis.png
dist_appsicon48_DATA = $(icondir)/48x48/apps/aravis.png
dist_appsicon256_DATA = $(icondir)/256x256/apps/aravis.png

update_icon_cache = gtk-update-icon-cache --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(update_icon_cache) "$(iconthemedir)"
