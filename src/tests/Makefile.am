include $(top_srcdir)/Makefile.decl

INCLUDES = $(ARAVIS_CFLAGS)

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS        += formula
formula_SOURCES = formula.c
formula_LDADD = $(ARAVIS_LIBS) $(top_srcdir)/src/libaravis.la
