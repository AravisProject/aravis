EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-introspection --enable-gtk-doc --enable-gst-plugin --enable-gst-0.10-plugin --enable-viewer --enable-cpp-test --enable-appstream-util

SUBDIRS = src

if ARAVIS_BUILD_VIEWER
SUBDIRS += viewer
endif

if ARAVIS_BUILD_GST_PLUGIN
SUBDIRS += gst
endif

if ARAVIS_BUILD_GST_0_10_PLUGIN
SUBDIRS += gst-0.10
endif

SUBDIRS += tests po docs

aravisdocdir = ${prefix}/doc/aravis-@ARAVIS_API_VERSION@
aravisdoc_DATA = 	\
	README		\
	COPYING		\
	ChangeLog 	\
	AUTHORS		\
	NEWS

EXTRA_DIST += m4/introspection.m4

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = aravis-@ARAVIS_API_VERSION@.pc

CLEANFILES = $(pkgconfig_DATA)

%-@ARAVIS_API_VERSION@.pc: %.pc
	cp $< $@

