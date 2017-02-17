# appstream-xml.m4
#
# serial 6

dnl APPSTREAM_XML
dnl Installs and validates AppData XML files.
dnl
dnl Call APPSTREAM_XML in configure.ac to check for the appstream-util tool.
dnl Add @APPSTREAM_XML_RULES@ to a Makefile.am to substitute the make rules. Add
dnl .appdata.xml files to appstream_XML in Makefile.am and they will be validated
dnl at make check time, if appstream-util is installed, as well as installed
dnl to the correct location automatically. Add --enable-appstream-util to
dnl AM_DISTCHECK_CONFIGURE_FLAGS in Makefile.am to require valid AppData XML when
dnl doing a distcheck.
dnl
dnl Adding files to appstream_XML does not distribute them automatically.

AC_DEFUN([APPSTREAM_XML],
[
  m4_pattern_allow([AM_V_GEN])
  AC_ARG_ENABLE([appstream-util],
                [AS_HELP_STRING([--disable-appstream-util],
                                [Disable validating AppData XML files during check phase])])

  AS_IF([test "x$enable_appstream_validate" != "xno"],
        [AC_PATH_PROG([APPSTREAM_UTIL], [appstream-util])
         AS_IF([test "x$APPSTREAM_UTIL" = "x"],
               [have_appstream_validate=no],
               [have_appstream_validate=yes
                AC_SUBST([APPSTREAM_UTIL])])],
        [have_appstream_validate=no])

  AS_IF([test "x$have_appstream_validate" != "xno"],
        [appstream_validate=yes],
        [appstream_validate=no
         AS_IF([test "x$enable_appstream_validate" = "xyes"],
               [AC_MSG_ERROR([AppData validation was requested but appstream-util was not found])])])

  AC_SUBST([appstreamxmldir], [${datadir}/appdata])

  APPSTREAM_XML_RULES='
.PHONY : uninstall-appstream-xml install-appstream-xml clean-appstream-xml

mostlyclean-am: clean-appstream-xml

%.appdata.valid: %.appdata.xml
	$(AM_V_GEN) if test -f "$<"; then d=; else d="$(srcdir)/"; fi; \
		if test -n "$(APPSTREAM_UTIL)"; \
			then $(APPSTREAM_UTIL) --nonet validate $${d}$<; fi \
		&& touch [$]@

check-am: $(appstream_XML:.appdata.xml=.appdata.valid)
uninstall-am: uninstall-appstream-xml
install-data-am: install-appstream-xml

.SECONDARY: $(appstream_XML)

install-appstream-xml: $(appstream_XML)
	@$(NORMAL_INSTALL)
	if test -n "$^"; then \
		test -z "$(appstreamxmldir)" || $(MKDIR_P) "$(DESTDIR)$(appstreamxmldir)"; \
		$(INSTALL_DATA) $^ "$(DESTDIR)$(appstreamxmldir)"; \
	fi

uninstall-appstream-xml:
	@$(NORMAL_UNINSTALL)
	@list='\''$(appstream_XML)'\''; test -n "$(appstreamxmldir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e '\''s|^.*/||'\''`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '\''$(DESTDIR)$(appstreamxmldir)'\'' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(appstreamxmldir)" && rm -f $$files

clean-appstream-xml:
	rm -f $(appstream_XML:.appdata.xml=.appdata.valid)
'
  _APPSTREAM_XML_SUBST(APPSTREAM_XML_RULES)
])

dnl _APPSTREAM_XML_SUBST(VARIABLE)
dnl Abstract macro to do either _AM_SUBST_NOTMAKE or AC_SUBST
AC_DEFUN([_APPSTREAM_XML_SUBST],
[
AC_SUBST([$1])
m4_ifdef([_AM_SUBST_NOTMAKE], [_AM_SUBST_NOTMAKE([$1])])
]
)
