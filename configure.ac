AC_PREREQ(2.63.0)

m4_define([aravis_version_major], [0])
m4_define([aravis_version_minor], [1])
m4_define([aravis_version_micro], [14])
m4_define([aravis_version], [aravis_version_major.aravis_version_minor.aravis_version_micro])

AC_INIT([aravis],[aravis_version],[http://bugzilla.gnome.org/enter_bug.cgi?product=aravis],[aravis])

AM_INIT_AUTOMAKE([-Wno-portability dist-bzip2])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([src/arvconfig.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_SUBST([ARAVIS_API_VERSION], [0.2])
AC_SUBST([ARAVIS_API_VERSION_U],[AS_TR_SH([$ARAVIS_API_VERSION])])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_PROG_CXX

AC_HEADER_STDC

AC_SUBST(GETTEXT_PACKAGE, aravis-${ARAVIS_API_VERSION})
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.31.2])

LT_INIT()

AC_ARG_ENABLE(gst_plugin,
	      [  --enable-gst-plugin     build gstreamer plugin],
	      [enable_gst_plugin=$enableval],
	      [enable_gst_plugin=no])
AC_ARG_ENABLE(viewer,
	      [  --enable-viewer         build viewer],
	      [enable_viewer=$enableval],
	      [enable_viewer=no])
AC_ARG_ENABLE(gtk3,
	      [  --enable-gtk3           build viewer with gtk+-3.0],
	      [enable_gtk3=$enableval],
	      [enable_gtk3=no])
AC_ARG_ENABLE(cpp_test,
	      [  --enable-cpp-test       build c++ test],
	      [enable_cpp_test=$enableval],
	      [enable_cpp_test=no])

PKG_PROG_PKG_CONFIG([0.14])

ARAVIS_REQUIREMENTS="glib-2.0 >= 2.26 gobject-2.0 gio-2.0 libxml-2.0 gthread-2.0"
ARAVIS_GSTREAMER_REQUIREMENTS="gstreamer-base-0.10 >= 0.10.31 gstreamer-app-0.10 gstreamer-interfaces-0.10"


PKG_CHECK_MODULES(ARAVIS, [$ARAVIS_REQUIREMENTS])

AC_SUBST(ARAVIS_CFLAGS)
AC_SUBST(ARAVIS_LIBS)

AC_SUBST(aravis_datadir, '$(datadir)/aravis-$(ARAVIS_API_VERSION)')

ARAVIS_VIEWER_REQUIREMENTS=$ARAVIS_GSTREAMER_REQUIREMENTS
if test "x$enable_gtk3" = "xyes"; then
	ARAVIS_VIEWER_REQUIREMENTS+=" gtk+-3.0"
else
	ARAVIS_VIEWER_REQUIREMENTS+=" gtk+-2.0 >= 2.14"
fi
AM_CONDITIONAL(ARAVIS_WITH_GTK3, test "x$enable_gtk3" = "xyes")

build_viewer=no
if test "x$enable_viewer" = "xyes"; then
	PKG_CHECK_MODULES([ARAVIS_VIEWER], [$ARAVIS_VIEWER_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_VIEWER], [1], [Build viewer])
			  build_viewer=yes],
			  [echo -n])
fi
AM_CONDITIONAL(ARAVIS_BUILD_VIEWER, test "x$build_viewer" = "xyes")

AC_SUBST(ARAVIS_VIEWER_CFLAGS)
AC_SUBST(ARAVIS_VIEWER_LIBS)

build_gst_plugin=no
if test "x$enable_gst_plugin" = "xyes"; then
	PKG_CHECK_MODULES([ARAVIS_GST_PLUGIN], [$ARAVIS_GSTREAMER_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_GST_PLUGIN], [1], [Build gstreamer plugin])
			  build_gst_plugin=yes],
			  [echo -n])
fi
AM_CONDITIONAL(ARAVIS_BUILD_GST_PLUGIN, test "x$build_gst_plugin" = "xyes")

AM_CONDITIONAL(ARAVIS_BUILD_CPP_TEST, test "x$enable_cpp_test" = "xyes")

AC_SUBST(ARAVIS_GST_PLUGIN_CFLAGS)
AC_SUBST(ARAVIS_GST_PLUGIN_LIBS)

if test "x${prefix}" = "x$HOME"; then
  gstplugindir="$HOME/.gstreamer-0.10/plugins"
else
  gstplugindir="\$(libdir)/gstreamer-0.10"
fi
AC_SUBST(gstplugindir)

GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

GOBJECT_INTROSPECTION_CHECK([0.10.0])
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])
AM_CONDITIONAL([BUILD_GTK_DOC], [test "x$enable_gtk_doc" = "xyes" || test ! -f "autogen.sh"])

AC_CONFIG_FILES([
Makefile
src/Makefile
viewer/Makefile
gst/Makefile
tests/Makefile
po/Makefile.in
docs/Makefile
docs/reference/Makefile
docs/reference/aravis/Makefile
aravis.pc
])
AC_OUTPUT

echo ""
echo "Configuration:"
echo ""
echo "	Build viewer:		$build_viewer"
echo "	Build gstreamer plugin:	$build_gst_plugin"
echo "	Compiler:		${CC}"
echo "	Compiler flags:		${ARAVIS_CFLAGS}"
echo "	Linker flags:		${ARAVIS_LIBS}"
echo ""
