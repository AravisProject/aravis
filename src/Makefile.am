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

lib_LTLIBRARIES = libaravis.la

BUILT_SOURCES = arvenumtypes.h arvenumtypes.c

EXTRA_DIST += arvenumtypes.h.template \
	      arvenumtypes.c.template

libaravis_la_LIBADD = 				\
	$(ARAVIS_LIBS)

libaravis_la_SOURCES =				\
	arvenums.c				\
	arvenumtypes.c				\
	arvdebug.c				\
	arvtools.c				\
	arvzip.c				\
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
	arvgvcp.c				\
	arvgvsp.c				\
	arvgvinterface.c			\
	arvgvdevice.c				\
	arvgvstream.c				\
	arvfakeinterface.c			\
	arvfakedevice.c				\
	arvfakestream.c				\
	arvfakecamera.c

ARAVIS_HDRS = 					\
	arv.h					\
	arvtypes.h				\
	arvenums.h				\
	arvdebug.h				\
	arvtools.h				\
	arvzip.h				\
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
	arvgvcp.h				\
	arvgvsp.h				\
	arvgvinterface.h			\
	arvgvdevice.h				\
	arvgvstream.h				\
	arvfakeinterface.h			\
	arvfakedevice.h				\
	arvfakestream.h				\
	arvfakecamera.h

arv_fake_cameradir = $(aravis_datadir)
arv_fake_camera_DATA = arv-fake-camera.xml

EXTRA_DIST += arv-fake-camera.xml

libaravis_ladir = $(includedir)/aravis
libaravis_la_HEADERS = $(ARAVIS_HDRS)
libaravis_la_HEADERS += arvenumtypes.h arvconfig.h

arvenumtypes.h: arvenumtypes.h.template $(ARAVIS_HDRS) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template arvenumtypes.h.template $(libaravis_la_HEADERS)) > $@

arvenumtypes.c: arvenumtypes.c.template $(ARAVIS_HDRS) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template arvenumtypes.c.template $(libaravis_la_HEADERS)) > $@

bin_PROGRAMS = arv-fake-gv-camera arv-show-devices

arv_fake_gv_camera_SOURCES = arvfakegvcamera.c
arv_fake_gv_camera_LDADD = $(ARAVIS_LIBS) libaravis.la

arv_show_devices_SOURCES = arvshowdevices.c
arv_show_devices_LDADD = $(ARAVIS_LIBS) libaravis.la

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION

introspection_files = $(libaravis_la_SOURCES) \
		      $(ARAVIS_HDRS)

Aravis-1.0.gir: $(INTROSPECTION_SCANNER) libaravis.la Makefile
Aravis_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
Aravis_1_0_gir_SCANNERFLAGS = --identifier-prefix=Arv
Aravis_1_0_gir_CFLAGS = $(INCLUDES) -I$(top_srcdir)/src
Aravis_1_0_gir_LIBS = libaravis.la
Aravis_1_0_gir_FILES = $(addprefix $(srcdir)/,$(introspection_files))
INTROSPECTION_GIRS += Aravis-1.0.gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelib_DATA)

endif

