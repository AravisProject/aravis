EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

-include $(INTROSPECTION_MAKEFILE)
  INTROSPECTION_GIRS =
  INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
  INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(ARAVIS_CFLAGS) \
	-DARAVIS_DATA_DIR=\""$(aravis_datadir)"\" \
	-DG_LOG_DOMAIN=\"Aravis\"

AM_CFLAGS = -Wall

lib_LTLIBRARIES = libaravis-@ARAVIS_API_VERSION@.la

BUILT_SOURCES = arvenumtypes.h arvenumtypes.c

EXTRA_DIST += arvenumtypes.h.template \
	      arvenumtypes.c.template

libaravis_@ARAVIS_API_VERSION@_la_LIBADD = 				\
	$(ARAVIS_LIBS)

ARAVIS_SRCS =					\
	arvenums.c				\
	arvenumtypes.c				\
	arvdebug.c				\
	arvsystem.c				\
	arvevaluator.c				\
	arvcamera.c				\
	arvgc.c					\
	arvgcnode.c				\
	arvgccategory.c				\
	arvgcboolean.c				\
	arvgcenumeration.c			\
	arvgcenumentry.c			\
	arvgcintegernode.c			\
	arvgcfloatnode.c			\
	arvgcregister.c				\
	arvgccommand.c				\
	arvgcswissknife.c			\
	arvgcconverter.c			\
	arvgcport.c				\
	arvgcinteger.c				\
	arvgcstring.c				\
	arvgcfloat.c				\
	arvinterface.c				\
	arvdevice.c				\
	arvstream.c				\
	arvbuffer.c				\
	arvgvinterface.c			\
	arvgvdevice.c				\
	arvgvstream.c				\
	arvfakeinterface.c			\
	arvfakedevice.c				\
	arvfakestream.c				\
	arvfakecamera.c

ARAVIS_SRCS_NO_INTRO =				\
	arvtools.c				\
	arvzip.c				\
	arvgvcp.c				\
	arvgvsp.c

ARAVIS_HDRS = 					\
	arv.h					\
	arvtypes.h				\
	arvenums.h				\
	arvdebug.h				\
	arvsystem.h				\
	arvevaluator.h				\
	arvcamera.h				\
	arvgc.h					\
	arvgcnode.h				\
	arvgccategory.h				\
	arvgcboolean.h				\
	arvgcenumeration.h			\
	arvgcenumentry.h			\
	arvgcintegernode.h			\
	arvgcfloatnode.h			\
	arvgcregister.h				\
	arvgccommand.h				\
	arvgcswissknife.h			\
	arvgcconverter.h			\
	arvgcport.h				\
	arvgcinteger.h				\
	arvgcstring.h				\
	arvgcfloat.h				\
	arvinterface.h				\
	arvdevice.h				\
	arvstream.h				\
	arvbuffer.h				\
	arvgvinterface.h			\
	arvgvdevice.h				\
	arvgvstream.h				\
	arvfakeinterface.h			\
	arvfakedevice.h				\
	arvfakestream.h				\
	arvfakecamera.h

ARAVIS_HDRS_NO_INTRO =				\
	arvtools.h				\
	arvzip.h				\
	arvgvcp.h				\
	arvgvsp.h

arv_fake_cameradir = $(aravis_datadir)
arv_fake_camera_DATA = arv-fake-camera.xml

EXTRA_DIST += arv-fake-camera.xml

libaravis_@ARAVIS_API_VERSION@_la_SOURCES = $(ARAVIS_SRCS) $(ARAVIS_SRCS_NO_INTRO)

libaravis_@ARAVIS_API_VERSION@_ladir = $(includedir)/aravis-@ARAVIS_API_VERSION@
libaravis_@ARAVIS_API_VERSION@_la_HEADERS = $(ARAVIS_HDRS) $(ARAVIS_HDRS_NO_INTRO)
libaravis_@ARAVIS_API_VERSION@_la_HEADERS += arvenumtypes.h arvconfig.h

arvenumtypes.h: arvenumtypes.h.template $(ARAVIS_HDRS) $(ARAVIS_HDRS_NO_INTRO) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template arvenumtypes.h.template $(libaravis_@ARAVIS_API_VERSION@_la_HEADERS)) > $@

arvenumtypes.c: arvenumtypes.c.template $(ARAVIS_HDRS) $(ARAVIS_HDRS_NO_INTRO) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template arvenumtypes.c.template $(libaravis_@ARAVIS_API_VERSION@_la_HEADERS)) > $@

bin_PROGRAMS = arv-fake-gv-camera-@ARAVIS_API_VERSION@ arv-show-devices-@ARAVIS_API_VERSION@

arv_fake_gv_camera_@ARAVIS_API_VERSION@_SOURCES = arvfakegvcamera.c
arv_fake_gv_camera_@ARAVIS_API_VERSION@_LDADD = $(ARAVIS_LIBS) libaravis-@ARAVIS_API_VERSION@.la

arv_show_devices_@ARAVIS_API_VERSION@_SOURCES = arvshowdevices.c
arv_show_devices_@ARAVIS_API_VERSION@_LDADD = $(ARAVIS_LIBS) libaravis-@ARAVIS_API_VERSION@.la

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION

introspection_files = $(ARAVIS_SRCS) \
		      $(ARAVIS_HDRS)

Aravis-@ARAVIS_API_VERSION@.gir: $(INTROSPECTION_SCANNER)
Aravis_0_2_gir_INCLUDES = GObject-2.0 Gio-2.0
Aravis_0_2_gir_SCANNERFLAGS = --identifier-prefix=Arv --warn-all
Aravis_0_2_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)/src
Aravis_0_2_gir_LIBS = libaravis-@ARAVIS_API_VERSION@.la
Aravis_0_2_gir_FILES = $(addprefix $(srcdir)/,$(introspection_files))
INTROSPECTION_GIRS += Aravis-@ARAVIS_API_VERSION@.gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelib_DATA)

endif

