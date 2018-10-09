AC_PREREQ(2.63.0)

m4_define([aravis_version_major], [0])
m4_define([aravis_version_minor], [6])
m4_define([aravis_version_micro], [1])
m4_define([aravis_version], [aravis_version_major.aravis_version_minor.aravis_version_micro])

AC_INIT([aravis],[aravis_version],[http://bugzilla.gnome.org/enter_bug.cgi?product=aravis],[aravis])

AM_INIT_AUTOMAKE([-Wno-portability dist-bzip2 dist-xz foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([src/arvconfig.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_SUBST([ARAVIS_API_VERSION], [0.6])
AC_SUBST([ARAVIS_API_VERSION_U],[AS_TR_SH([$ARAVIS_API_VERSION])])

ARAVIS_MAJOR_VERSION=aravis_version_major
ARAVIS_MINOR_VERSION=aravis_version_minor
ARAVIS_MICRO_VERSION=aravis_version_micro
ARAVIS_VERSION=aravis_version

AC_SUBST(ARAVIS_MAJOR_VERSION)
AC_SUBST(ARAVIS_MINOR_VERSION)
AC_SUBST(ARAVIS_MICRO_VERSION)
AC_SUBST(ARAVIS_VERSION)

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_PROG_CXX

AC_HEADER_STDC

AC_SUBST(GETTEXT_PACKAGE, aravis-${ARAVIS_API_VERSION})
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.31.2])

LT_INIT()

AX_VALGRIND_CHECK()
AX_CODE_COVERAGE()

AC_ARG_ENABLE(usb,
	      [AS_HELP_STRING([--enable-usb],
			      [build with USB support [default=auto]])],
			      [enable_usb=$enableval],
			      [enable_usb=auto])
AC_ARG_ENABLE(packet_socket,
	      [AS_HELP_STRING([--enable-packet-socket],
			      [build with packet socket support [default=auto]])],
			      [enable_packet_socket=$enableval],
			      [enable_packet_socket=auto])
AC_ARG_ENABLE(viewer,
	      [AS_HELP_STRING([--enable-viewer],
			      [build viewer [default=auto]])],
			      [enable_viewer=$enableval],
			      [enable_viewer=auto])
AC_ARG_ENABLE(gst_plugin,
	      [AS_HELP_STRING([--enable-gst-plugin],
			      [build gstreamer plugin [default=auto]])],
			      [enable_gst_plugin=$enableval],
			      [enable_gst_plugin=auto])
AC_ARG_ENABLE(gst_0_10_plugin,
	      [AS_HELP_STRING([--enable-gst-0.10-plugin],
			      [build gstreamer-0.10 plugin [default=auto]])],
			      [enable_gst_0_10_plugin=$enableval],
			      [enable_gst_0_10_plugin=auto])
AC_ARG_ENABLE(cpp_test,
	      [AS_HELP_STRING([--enable-cpp-test],
			      [build c++ test [default=no]])],
			      [enable_cpp_test=$enableval],
			      [enable_cpp_test=no])
AC_ARG_ENABLE(zlib_pc,
	      [AS_HELP_STRING([--enable-zlib-pc],
			      [use pkg-config for zlib dependency [default=yes]])],
			      [enable_zlib_pc=$enableval],
			      [enable_zlib_pc=yes])
AC_ARG_ENABLE(fast_heartbeat,
	      [AS_HELP_STRING([--enable-fast-heartbeat],
			      [build with extremely fast heartbeat [default=no]])],
			      [enable_fast_heartbeat=$enableval],
			      [enable_fast_heartbeat=no])
AC_ARG_ENABLE(asan,
	      [AC_HELP_STRING([--enable-asan],
			      [build with address sanitizer [default=no]])],
			      [enable_asan=$enableval],
			      [enable_asan=no])

PKG_PROG_PKG_CONFIG([0.14])

ARAVIS_REQUIREMENTS="glib-2.0 >= 2.32 gobject-2.0 gio-2.0 libxml-2.0 gthread-2.0"
ARAVIS_USB_REQUIREMENTS="libusb-1.0"
ARAVIS_PACKET_SOCKET_REQUIREMENTS="audit"
ARAVIS_GSTREAMER_REQUIREMENTS="gstreamer-base-1.0 gstreamer-app-1.0"
ARAVIS_GSTREAMER_0_10_REQUIREMENTS="gstreamer-base-0.10 gstreamer-app-0.10"
ARAVIS_VIEWER_REQUIREMENTS="$ARAVIS_GSTREAMER_REQUIREMENTS gstreamer-video-1.0 gtk+-3.0 >= 3.12 libnotify"

# Conditional use of zlib.pc, as it doesn't exist on old setup

if test "x$enable_zlib_pc" = "xyes"; then
	ARAVIS_REQUIREMENTS="${ARAVIS_REQUIREMENTS} zlib"
	ARAVIS_ADDITIONAL_LIBS="-lm"
else
	ARAVIS_ADDITIONAL_LIBS="-lm -lz"
fi

# USB support
# ===========

build_usb=no
if test "x$enable_usb" = "xyes" || test "x$enable_usb" = "xauto"; then
	PKG_CHECK_MODULES([ARAVIS_USB], [$ARAVIS_USB_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_USB], [1], [Build USB support]) build_usb=yes],
			  [AS_IF([test "x$enable_usb" = "xyes"],
				 AC_MSG_ERROR([${ARAVIS_USB_PKG_ERRORS} - Can not build USB support]))]);
fi
AM_CONDITIONAL(ARAVIS_BUILD_USB, test "x$build_usb" = "xyes")

if test "x$build_usb" = "xyes"; then
	ARAVIS_REQUIREMENTS="${ARAVIS_REQUIREMENTS} ${ARAVIS_USB_REQUIREMENTS}"
	AC_SUBST([ARAVIS_HAS_USB],[1])
else
	AC_SUBST([ARAVIS_HAS_USB],[0])
fi

# Packet socket support
# ===================

build_packet_socket=no
if test "x$enable_packet_socket" = "xyes" || test "x$enable_packet_socket" = "xauto"; then
	PKG_CHECK_MODULES([ARAVIS_PACKET_SOCKET], [$ARAVIS_PACKET_SOCKET_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_PACKET_SOCKET], [1], [Build packet socket support]) build_packet_socket=yes],
			  [AS_IF([test "x$enable_packet_socket" = "xyes"],
				 AC_MSG_ERROR([${ARAVIS_PACKET_SOCKET_PKG_ERRORS} - Can not build packet socket support]))]);
fi
AM_CONDITIONAL(ARAVIS_BUILD_PACKET_SOCKET, test "x$build_packet_socket" = "xyes")

if test "x$build_packet_socket" = "xyes"; then
	ARAVIS_REQUIREMENTS="${ARAVIS_REQUIREMENTS} ${ARAVIS_PACKET_SOCKET_REQUIREMENTS}"
	AC_SUBST([ARAVIS_HAS_PACKET_SOCKET],[1])
else
	AC_SUBST([ARAVIS_HAS_PACKET_SOCKET],[0])
fi

PKG_CHECK_MODULES(ARAVIS, [$ARAVIS_REQUIREMENTS])

# Standard math library doesn't come with a pkgconfig file.

ARAVIS_LIBS="${ARAVIS_ADDITIONAL_LIBS} ${ARAVIS_LIBS}"

AC_SUBST(ARAVIS_CFLAGS)
AC_SUBST(ARAVIS_LIBS)

AC_SUBST(aravis_datadir, '$(datadir)/aravis-$(ARAVIS_API_VERSION)')

# Viewer
# ======

build_viewer=no
if test "x$enable_viewer" = "xyes" || test "x$enable_viewer" = "xauto"; then
	PKG_CHECK_MODULES([ARAVIS_VIEWER], [$ARAVIS_VIEWER_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_VIEWER], [1], [Build viewer]) build_viewer=yes],
			  [AS_IF([test "x$enable_viewer" = "xyes"],
				 AC_MSG_ERROR([${ARAVIS_VIEWER_PKG_ERRORS} - Can not build viewer]))]);
fi
AM_CONDITIONAL(ARAVIS_BUILD_VIEWER, test "x$build_viewer" = "xyes")

# Standard math library doesn't come with a pkgconfig file.
ARAVIS_VIEWER_LIBS="${ARAVIS_ADDITIONAL_LIBS} ${ARAVIS_VIEWER_LIBS}"

AC_SUBST(ARAVIS_VIEWER_CFLAGS)
AC_SUBST(ARAVIS_VIEWER_LIBS)

# GStreamer plugin
# ================

build_gst_plugin=no
if test "x$enable_gst_plugin" = "xyes" || test "x$enable_gst_plugin" = "xauto"; then
	PKG_CHECK_MODULES([ARAVIS_GST_PLUGIN], [$ARAVIS_GSTREAMER_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_GST_PLUGIN], [1], [Build gstreamer plugin]) build_gst_plugin=yes],
			  [AS_IF([test "x$enable_gst_plugin" = "xyes"],
				 AC_MSG_ERROR([${ARAVIS_GST_PLUGIN_PKG_ERRORS} - Can not build gstreamer plugin]))]);
fi
AM_CONDITIONAL(ARAVIS_BUILD_GST_PLUGIN, test "x$build_gst_plugin" = "xyes")

# Standard math library doesn't come with a pkgconfig file.
ARAVIS_GST_PLUGIN_LIBS="${ARAVIS_ADDITIONAL_LIBS} ${ARAVIS_GST_PLUGIN_LIBS}"

AC_SUBST(ARAVIS_GST_PLUGIN_CFLAGS)
AC_SUBST(ARAVIS_GST_PLUGIN_LIBS)

if test "x${prefix}" = "x$HOME"; then
  gstplugindir="$HOME/.gstreamer-1.0/plugins"
else
  gstplugindir="\$(libdir)/gstreamer-1.0"
fi
AC_SUBST(gstplugindir)

GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

# GStreamer-0.10 plugin
# =====================

build_gst_0_10_plugin=no
if test "x$enable_gst_0_10_plugin" = "xyes" || test "x$enable_gst_0_10_plugin" = "xauto"; then
	PKG_CHECK_MODULES([ARAVIS_GST_0_10_PLUGIN], [$ARAVIS_GSTREAMER_0_10_REQUIREMENTS],
			  [AC_DEFINE([ARAVIS_BUILD_GST_0_10_PLUGIN], [1], [Build gstreamer-0.10 plugin]) build_gst_0_10_plugin=yes],
			  [AS_IF([test "x$enable_gst_0_10_plugin" = "xyes"],
				 AC_MSG_ERROR([${ARAVIS_GST_0_10_PLUGIN_PKG_ERRORS} - Can not build gstreamer plugin]))]);
fi
AM_CONDITIONAL(ARAVIS_BUILD_GST_0_10_PLUGIN, test "x$build_gst_0_10_plugin" = "xyes")

# Standard math library doesn't come with a pkgconfig file.
ARAVIS_GST_0_10_PLUGIN_LIBS="${ARAVIS_ADDITIONAL_LIBS} ${ARAVIS_GST_0_10_PLUGIN_LIBS}"

AC_SUBST(ARAVIS_GST_0_10_PLUGIN_CFLAGS)
AC_SUBST(ARAVIS_GST_0_10_PLUGIN_LIBS)

if test "x${prefix}" = "x$HOME"; then
  gst010plugindir="$HOME/.gstreamer-0.10/plugins"
else
  gst010plugindir="\$(libdir)/gstreamer-0.10"
fi
AC_SUBST(gst010plugindir)

GST_0_10_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_0_10_PLUGIN_LDFLAGS)

# Fast heartbeat
# =============

build_fast_heartbeat=no
if test "x$enable_fast_heartbeat" = "xyes" ; then
    build_fast_heartbeat=yes
    AC_DEFINE([ARAVIS_FAST_HEARTBEAT], [], [Build with a much fast heartbeat rate])
fi
AM_CONDITIONAL(ARAVIS_FAST_HEARTBEAT, test "x$build_fast_heartbeat" = "xyes")

# C++ test
# ========

AM_CONDITIONAL(ARAVIS_BUILD_CPP_TEST, test "x$enable_cpp_test" = "xyes")

# Address sanatizer
# =================

AC_MSG_CHECKING([for asan flags])
if test "x$enable_asan" = "xyes"; then
    CFLAGS="$CFLAGS -fsanitize=address -O1 -fno-omit-frame-pointer -g"
    build_with_asan="yes"
else
    build_with_asan="no"
fi
AM_CONDITIONAL(ARAVIS_WITH_ASAN, test "x$enable_asan" = "xyes")
AC_MSG_RESULT($build_with_asan)


AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

m4_ifdef([GOBJECT_INTROSPECTION_CHECK], [
GOBJECT_INTROSPECTION_CHECK([0.10.0])
], [
   AM_CONDITIONAL([HAVE_INTROSPECTION], [false])
   enable_introspection=no])

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])
AM_CONDITIONAL([BUILD_GTK_DOC], [test "x$enable_gtk_doc" = "xyes" || test ! -f "autogen.sh"])

APPSTREAM_XML

AC_CONFIG_FILES([
Makefile
src/Makefile
src/arvfeatures.h
src/arvversion.h
viewer/Makefile
viewer/data/Makefile
viewer/data/arv-viewer.desktop.in
viewer/data/arv-viewer.appdata.xml
gst/Makefile
gst-0.10/Makefile
tests/Makefile
po/Makefile.in
man/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/aravis/Makefile
aravis.pc
])
AC_OUTPUT

echo ""
echo "Configuration:"
echo ""
echo "  Compiler:                     ${CC}"
echo "  Compiler flags:               ${ARAVIS_CFLAGS}"
echo "  Linker flags:                 ${ARAVIS_LIBS}"
echo ""
echo "  Build USB support:            $build_usb"
echo "  Build packet socket support:  $build_packet_socket"
echo "  Build viewer:                 $build_viewer"
echo "  Build gstreamer plugin:       $build_gst_plugin"
echo "  Build gstreamer-0.10 plugin:  $build_gst_0_10_plugin"
echo "  Build with fast heartbeat:    $build_fast_heartbeat"
echo "  Build with address sanatizer: $build_with_asan"
echo ""
