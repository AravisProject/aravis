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

arv_example_SOURCES = arvexample.c
arv_example_LDADD = $(test_progs_ldadd)

time_test_SOURCES = timetest.c
time_test_LDADD = $(test_progs_ldadd)

realtime_test_SOURCES = realtimetest.c
realtime_test_LDADD = $(test_progs_ldadd)

load_http_test_SOURCES = loadhttptest.c
load_http_test_LDADD = $(test_progs_ldadd)

TESTS = evaluator buffer misc fake genicam
if ARAVIS_BUILD_CPP_TEST
TESTS += cpp
endif

check_PROGRAMS = $(TESTS)

evaluator_SOURCES = evaluator.c
evaluator_LDADD = $(test_progs_ldadd)

buffer_SOURCES = buffer.c
buffer_LDADD = $(test_progs_ldadd)

misc_SOURCES = misc.c
misc_LDADD = $(test_progs_ldadd)

fake_SOURCES = fake.c
fake_CFLAGS = -DGENICAM_FILENAME="\"$(top_srcdir)/src/arv-fake-camera.xml\""
fake_LDADD = $(test_progs_ldadd)

genicam_SOURCES = genicam.c
genicam_CFLAGS = -DGENICAM_FILENAME="\"$(top_srcdir)/tests/data/genicam.xml\""
genicam_LDADD = $(test_progs_ldadd)

cpp_SOURCES = cpp.cc
cpp_LDADD = $(test_progs_ldadd)

EXTRA_DIST = data/genicam.xml

@VALGRIND_CHECK_RULES@

