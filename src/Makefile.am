EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_CFLAGS) \
	-DARAVIS_DATA_DIR=\""$(aravis_datadir)"\" \
	-DARAVIS_API_VERSION=\""$(ARAVIS_API_VERSION)"\" \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS = -Wall

lib_LTLIBRARIES = libaravis-@ARAVIS_API_VERSION@.la

BUILT_SOURCES = arvenumtypes.h arvenumtypes.c

EXTRA_DIST += arvenumtypes.h.template \
	      arvenumtypes.c.template

libaravis_@ARAVIS_API_VERSION@_la_LIBADD = 	\
	$(ARAVIS_LIBS)

ARAVIS_SRCS =					\
	arvenums.c				\
	arvenumtypes.c				\
	arvdebug.c				\
	arvsystem.c				\
	arvevaluator.c				\
	arvdomnode.c				\
	arvdomnodelist.c			\
	arvdomnamednodemap.c			\
	arvdomdocument.c			\
	arvdomdocumentfragment.c		\
	arvdomelement.c				\
	arvdomcharacterdata.c			\
	arvdomtext.c				\
	arvdomparser.c				\
	arvdomimplementation.c			\
	arvcamera.c				\
	arvgc.c					\
	arvgcnode.c				\
	arvgcpropertynode.c			\
	arvgcindexnode.c			\
	arvgcvariablenode.c			\
	arvgcinvalidatornode.c			\
	arvgcfeaturenode.c			\
	arvgcregisterdescriptionnode.c		\
	arvgcgroupnode.c			\
	arvgccategory.c				\
	arvgcboolean.c				\
	arvgcenumeration.c			\
	arvgcenumentry.c			\
	arvgcintegernode.c			\
	arvgcfloatnode.c			\
	arvgcregisternode.c			\
	arvgcstructentrynode.c			\
	arvgccommand.c				\
	arvgcswissknife.c			\
	arvgcconverter.c			\
	arvgcport.c				\
	arvgcregister.c				\
	arvgcinteger.c				\
	arvgcstring.c				\
	arvgcfloat.c				\
	arvinterface.c				\
	arvdevice.c				\
	arvstream.c				\
	arvbuffer.c				\
	arvchunkparser.c			\
	arvgvinterface.c			\
	arvgvdevice.c				\
	arvgvstream.c				\
	arvfakeinterface.c			\
	arvfakedevice.c				\
	arvfakestream.c				\
	arvfakecamera.c				\
	arvrealtime.c

ARAVIS_SRCS_NO_INTRO =				\
	arvmisc.c				\
	arvzip.c				\
	arvstr.c				\
	arvgvcp.c				\
	arvgvsp.c

ARAVIS_HDRS = 					\
	arv.h					\
	arvtypes.h				\
	arvenums.h				\
	arvdebug.h				\
	arvsystem.h				\
	arvevaluator.h				\
	arvdomnode.h				\
	arvdomnodelist.h			\
	arvdomnamednodemap.h			\
	arvdomdocument.h			\
	arvdomdocumentfragment.h		\
	arvdomelement.h				\
	arvdomcharacterdata.h			\
	arvdomtext.h				\
	arvdomparser.h				\
	arvdomimplementation.h			\
	arvcamera.h				\
	arvgc.h					\
	arvgcnode.h				\
	arvgcpropertynode.h			\
	arvgcindexnode.h			\
	arvgcvariablenode.h			\
	arvgcinvalidatornode.h			\
	arvgcfeaturenode.h			\
	arvgcregisterdescriptionnode.h		\
	arvgcgroupnode.h			\
	arvgccategory.h				\
	arvgcboolean.h				\
	arvgcenumeration.h			\
	arvgcenumentry.h			\
	arvgcintegernode.h			\
	arvgcfloatnode.h			\
	arvgcregisternode.h			\
	arvgcstructentrynode.h			\
	arvgccommand.h				\
	arvgcswissknife.h			\
	arvgcconverter.h			\
	arvgcport.h				\
	arvgcregister.h				\
	arvgcinteger.h				\
	arvgcstring.h				\
	arvgcfloat.h				\
	arvinterface.h				\
	arvdevice.h				\
	arvstream.h				\
	arvbuffer.h				\
	arvchunkparser.h			\
	arvgvinterface.h			\
	arvgvdevice.h				\
	arvgvstream.h				\
	arvfakeinterface.h			\
	arvfakedevice.h				\
	arvfakestream.h				\
	arvfakecamera.h				\
	arvrealtime.h

ARAVIS_HDRS_NO_INTRO =				\
	arvmisc.h				\
	arvzip.h				\
	arvstr.h				\
	arvgvcp.h				\
	arvgvsp.h

noinst_HEADERS =				\
	arvbufferprivate.h			\
	arvchunkparserprivate.h			\
	arvdeviceprivate.h			\
	arvstreamprivate.h			\
	arvrealtimeprivate.h

arv_fake_cameradir = $(aravis_datadir)
arv_fake_camera_DATA = arv-fake-camera.xml

EXTRA_DIST += arv-fake-camera.xml

libaravis_@ARAVIS_API_VERSION@_la_SOURCES = $(ARAVIS_SRCS) $(ARAVIS_SRCS_NO_INTRO)

libaravis_@ARAVIS_API_VERSION@_ladir = $(includedir)/aravis-@ARAVIS_API_VERSION@
libaravis_@ARAVIS_API_VERSION@_la_HEADERS = $(ARAVIS_HDRS) $(ARAVIS_HDRS_NO_INTRO)
libaravis_@ARAVIS_API_VERSION@_la_HEADERS += arvenumtypes.h

arvenumtypes.h: arvenumtypes.h.template $(ARAVIS_HDRS) $(ARAVIS_HDRS_NO_INTRO) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template arvenumtypes.h.template $(libaravis_@ARAVIS_API_VERSION@_la_HEADERS)) > $@

arvenumtypes.c: arvenumtypes.c.template $(ARAVIS_HDRS) $(ARAVIS_HDRS_NO_INTRO) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template arvenumtypes.c.template $(libaravis_@ARAVIS_API_VERSION@_la_HEADERS)) > $@

bin_PROGRAMS =						\
	arv-fake-gv-camera-@ARAVIS_API_VERSION@		\
	arv-tool-@ARAVIS_API_VERSION@

arv_fake_gv_camera_@ARAVIS_API_VERSION@_SOURCES = arvfakegvcamera.c
arv_fake_gv_camera_@ARAVIS_API_VERSION@_LDADD = $(ARAVIS_LIBS) libaravis-@ARAVIS_API_VERSION@.la

arv_tool_@ARAVIS_API_VERSION@_SOURCES = arvtool.c
arv_tool_@ARAVIS_API_VERSION@_LDADD = $(ARAVIS_LIBS) libaravis-@ARAVIS_API_VERSION@.la

CLEANFILES = $(BUILT_SOURCES)

-include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

introspection_files = $(ARAVIS_SRCS) \
		      $(ARAVIS_HDRS) \
		      arvenumtypes.c \
		      arvenumtypes.h

Aravis-@ARAVIS_API_VERSION@.gir: $(INTROSPECTION_SCANNER) libaravis-@ARAVIS_API_VERSION@.la Makefile

Aravis_@ARAVIS_API_VERSION_U@_gir_INCLUDES = GObject-2.0 Gio-2.0
Aravis_@ARAVIS_API_VERSION_U@_gir_SCANNERFLAGS = --identifier-prefix=Arv --warn-all
Aravis_@ARAVIS_API_VERSION_U@_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)/src
Aravis_@ARAVIS_API_VERSION_U@_gir_LIBS = libaravis-@ARAVIS_API_VERSION@.la
Aravis_@ARAVIS_API_VERSION_U@_gir_FILES = $(addprefix $(srcdir)/,$(introspection_files))

INTROSPECTION_GIRS += Aravis-@ARAVIS_API_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

endif

