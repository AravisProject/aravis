include $(top_srcdir)/Makefile.decl

@APPDATA_XML_RULES@
@INTLTOOL_XML_RULE@

appdata_in_files = arv-viewer.appdata.xml.in
appdata_XML = $(appdata_in_files:.xml.in=.xml)

EXTRA_DIST = $(appdata_in_files)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = arv-viewer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

dist_noinst_DATA = \
	$(desktop_in_files)

CLEANFILES = 					\
	$(appdata_XML)				\
	$(desktop_DATA)
