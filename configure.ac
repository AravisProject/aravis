AC_PREREQ(2.63.0)

m4_define([aravis_version_major], [0])
m4_define([aravis_version_minor], [1])
m4_define([aravis_version_micro], [1])
m4_define([aravis_version], [aravis_version_major.aravis_version_minor.aravis_version_micro])

AC_INIT([aravis],[aravis_version],[http://bugzilla.gnome.org/enter_bug.cgi?product=aravis],[aravis])

AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([src/arvconfig.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC

AC_HEADER_STDC

GETTEXT_PACKAGE=aravis
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.31.2])

LT_INIT()

AC_ARG_ENABLE(cairo,
	      [  --enable-cairo          Enable cairo support],
	      [enable_cairo=$enableval],
	      [enable_cairo=no])
AC_ARG_ENABLE(gstreamer,
	      [  --enable-gstreamer      Enable gstreamer support],
	      [enable_gstreamer=$enableval],
	      [enable_gstreamer=no])

PKG_PROG_PKG_CONFIG([0.14])

ARAVIS_REQUIRES="[gobject-2.0 glib-2.0 >= 2.22 gio-2.0 libxml-2.0 gthread-2.0]"

with_cairo=no
if test "x$enable_cairo" = "xyes"; then
	PKG_CHECK_MODULES([CAIRO],   [cairo >= 1.2],
			  [AC_DEFINE([ARAVIS_WITH_CAIRO], [1], [Cairo support])
			  ARAVIS_REQUIRES="${ARAVIS_REQUIRES} [cairo >= 1.2]"
			  with_cairo=yes],
			  [echo -n])
fi
AM_CONDITIONAL(ARAVIS_WITH_CAIRO, test "x$with_cairo" = "xyes")

with_gstreamer=no
if test "x$enable_gstreamer" = "xyes"; then
	PKG_CHECK_MODULES([GSTREAMER],   [gstreamer-base-0.10 >= 0.10],
			  [AC_DEFINE([ARAVIS_WITH_GSTREAMER], [1], [Cairo support])
			  ARAVIS_REQUIRES="${ARAVIS_REQUIRES} [gstreamer-base-0.10 >= 0.10]"
			  with_gstreamer=yes],
			  [echo -n])
fi
AM_CONDITIONAL(ARAVIS_WITH_GSTREAMER, test "x$with_gstreamer" = "xyes")

if test "x${prefix}" = "x$HOME"; then
  gstplugindir="$HOME/.gstreamer-0.10/plugins"
else
  gstplugindir="\$(libdir)/gstreamer-0.10"
fi
AC_SUBST(gstplugindir)

AC_SUBST(aravis_datadir, '$(datadir)/aravis')

GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

PKG_CHECK_MODULES(ARAVIS, $ARAVIS_REQUIRES)

AC_SUBST(ARAVIS_CFLAGS)
AC_SUBST(ARAVIS_LIBS)

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

GOBJECT_INTROSPECTION_CHECK([0.6.7])
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])
AM_CONDITIONAL([BUILD_GTK_DOC], [test "x$enable_gtk_doc" = "xyes" || test ! -f "autogen.sh"])

AC_CONFIG_FILES([
Makefile
src/Makefile
gst/Makefile
tests/Makefile
po/Makefile.in
docs/Makefile
docs/reference/Makefile
docs/reference/aravis/Makefile
aravis.pc
])
AC_CONFIG_FILES([gst/gst-aravis-launch], [chmod +x gst/gst-aravis-launch])
AC_OUTPUT

echo ""
echo "Configuration:"
echo ""
echo "	Cairo support:		$with_cairo"
echo "	Gstreamer support:	$with_gstreamer"
echo "	Compiler:		${CC}"
echo "	Compiler flags:		${ARAVIS_CFLAGS}"
echo ""
