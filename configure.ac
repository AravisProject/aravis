AC_PREREQ(2.63.0)

m4_define([aravis_version_major], [0])
m4_define([aravis_version_minor], [1])
m4_define([aravis_version_micro], [0])
m4_define([aravis_version], [aravis_version_major.aravis_version_minor.aravis_version_micro])

AC_INIT([aravis],[aravis_version],[http://bugzilla.gnome.org/enter_bug.cgi?product=aravis],[aravis])

AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC

AC_HEADER_STDC

GETTEXT_PACKAGE=aravis
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

LT_INIT()

PKG_CHECK_MODULES(ARAVIS, [gobject-2.0 glib-2.0 >= 2.22 gio-2.0 libxml-2.0 gthread-2.0])
AC_SUBST(ARAVIS_CFLAGS)
AC_SUBST(ARAVIS_LIBS)

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

GOBJECT_INTROSPECTION_CHECK([0.6.7])

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
aravis.pc
])
AC_OUTPUT

echo ""
echo "Configuration:"
echo ""
echo "	Compiler:		${CC}"
echo "	Compiler flags:		${ARAVIS_CFLAGS}"
echo ""

