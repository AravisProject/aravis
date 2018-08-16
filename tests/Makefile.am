include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = -g -I$(top_srcdir)/src/ $(ARAVIS_CFLAGS)

AM_CFLAGS = -Wall

test_progs_ldadd = 					\
	$(top_builddir)/src/libaravis-@ARAVIS_API_VERSION@.la		\
	$(ARAVIS_LIBS)

noinst_PROGRAMS = 			\
	arv-device-test			\
	arv-genicam-test		\
       	arv-evaluator-test		\
       	arv-zip-test			\
       	arv-camera-test			\
	arv-chunk-parser-test		\
       	arv-heartbeat-test		\
	arv-acquisition-test		\
       	arv-example			\
	arv-auto-packet-size-test	\
	arv-device-scan-test		\
	time-test			\
	realtime-test			\
	load-http-test

arv_device_test_SOURCES = arvdevicetest.c
arv_device_test_LDADD = $(test_progs_ldadd)

arv_genicam_test_SOURCES = arvgenicamtest.c
arv_genicam_test_LDADD = $(test_progs_ldadd)

arv_evaluator_test_SOURCES = arvevaluatortest.c
arv_evaluator_test_LDADD = $(test_progs_ldadd)

arv_zip_test_SOURCES = arvziptest.c
arv_zip_test_LDADD = $(test_progs_ldadd)

arv_camera_test_SOURCES = arvcameratest.c
arv_camera_test_LDADD = $(test_progs_ldadd)

arv_chunk_parser_test_SOURCES = arvchunkparsertest.c
arv_chunk_parser_test_LDADD = $(test_progs_ldadd)

arv_heartbeat_test_SOURCES = arvheartbeattest.c
arv_heartbeat_test_LDADD = $(test_progs_ldadd)

arv_acquisition_test_SOURCES = arvacquisitiontest.c
arv_acquisition_test_LDADD = $(test_progs_ldadd)

arv_auto_packet_size_test_SOURCES = arvautopacketsizetest.c
arv_auto_packet_size_test_LDADD = $(test_progs_ldadd)

arv_device_scan_test_SOURCES = arvdevicescantest.c
arv_device_scan_test_LDADD = $(test_progs_ldadd)

arv_example_SOURCES = arvexample.c
arv_example_LDADD = $(test_progs_ldadd)

time_test_SOURCES = timetest.c
time_test_LDADD = $(test_progs_ldadd)

realtime_test_SOURCES = realtimetest.c
realtime_test_LDADD = $(test_progs_ldadd)

load_http_test_SOURCES = loadhttptest.c
load_http_test_LDADD = $(test_progs_ldadd)

TESTS = evaluator buffer misc genicam fake fakegv
if ARAVIS_BUILD_CPP_TEST
TESTS += cpp
endif

@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = aravis.supp
EXTRA_DIST = aravis.supp

check_PROGRAMS = $(TESTS)

@CODE_COVERAGE_RULES@

evaluator_SOURCES = evaluator.c
evaluator_CFLAGS = $(CODE_COVERAGE_CFLAGS)
evaluator_LDADD = $(test_progs_ldadd) $(CODE_COVERAGE_LIBS)

buffer_SOURCES = buffer.c
buffer_CFLAGS = $(CODE_COVERAGE_CFLAGS)
buffer_LDADD = $(test_progs_ldadd) $(CODE_COVERAGE_LIBS)

misc_SOURCES = misc.c
misc_CFLAGS = $(CODE_COVERAGE_CFLAGS)
misc_LDADD = $(test_progs_ldadd) $(CODE_COVERAGE_LIBS)

fake_SOURCES = fake.c
fake_CFLAGS = -DGENICAM_FILENAME="\"$(top_srcdir)/src/arv-fake-camera.xml\"" $(CODE_COVERAGE_CFLAGS)
fake_LDADD = $(test_progs_ldadd) $(CODE_COVERAGE_LIBS)

fakegv_SOURCES = fakegv.c
fakegv_CFLAGS = -DGENICAM_FILENAME="\"$(top_srcdir)/src/arv-fake-camera.xml\"" $(CODE_COVERAGE_CFLAGS)
fakegv_LDADD = $(test_progs_ldadd) $(CODE_COVERAGE_LIBS)

genicam_SOURCES = genicam.c
genicam_CFLAGS = -DGENICAM_FILENAME="\"$(top_srcdir)/tests/data/genicam.xml\"" $(CODE_COVERAGE_CFLAGS)
genicam_LDADD = $(test_progs_ldadd) $(CODE_COVERAGE_LIBS)

cpp_SOURCES = cpp.cc
cpp_LDADD = $(test_progs_ldadd)

EXTRA_DIST += data/genicam.xml
