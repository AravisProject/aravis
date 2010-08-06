include $(top_srcdir)/Makefile.decl

INCLUDES = -g -I$(top_srcdir)/src/ $(ARAVIS_CFLAGS)

noinst_PROGRAMS = $(TEST_PROGS)

test_progs_ldadd = 					\
	$(top_builddir)/src/libaravis.la		\
	$(ARAVIS_LIBS)

TEST_PROGS += evaluator fake

evaluator_SOURCES = evaluator.c
evaluator_LDADD = $(test_progs_ldadd)

fake_SOURCES = fake.c
fake_CFLAGS = -DGENICAM_FILENAME="\"../src/arv-fake-camera.xml\""
fake_LDADD = $(test_progs_ldadd)

