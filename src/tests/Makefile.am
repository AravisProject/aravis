include $(top_srcdir)/Makefile.decl

INCLUDES = -g -I$(top_srcdir)/src/ $(ARAVIS_CFLAGS)

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS += evaluator fake

evaluator_SOURCES = evaluator.c
evaluator_LDADD = $(ARAVIS_LIBS) $(top_builddir)/src/libaravis.la

fake_SOURCES = fake.c
fake_CFLAGS = -DGENICAM_FILENAME="\"$(srcdir)/../arv-fake-camera.xml\""
fake_LDADD = $(ARAVIS_LIBS) $(top_builddir)/src/libaravis.la

