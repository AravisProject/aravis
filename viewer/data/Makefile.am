include $(top_srcdir)/Makefile.decl

@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@

appstream_in_files = arv-viewer.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)

EXTRA_DIST = $(appstream_in_files)

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = arv-viewer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

dist_noinst_DATA = \
	$(desktop_in_files)

CLEANFILES = 					\
	$(appstream_XML)				\
	$(desktop_DATA)
