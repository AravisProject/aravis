EXTRA_DIST =

include $(top_srcdir)/Makefile.decl

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = --enable-introspection --enable-gtk-doc

SUBDIRS = src gst po docs

aravisdocdir = ${prefix}/doc/aravis
aravisdoc_DATA = 	\
	README		\
	COPYING		\
	ChangeLog 	\
	AUTHORS		\
	INSTALL		\
	NEWS

EXTRA_DIST += $(aravisdoc_DATA) m4/introspection.m4

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = aravis.pc
