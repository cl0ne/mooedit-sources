AC_DEFUN([_MOO_INTLTOOL],[
  AC_PATH_PROG(INTLTOOL_UPDATE, [intltool-update])
  AC_PATH_PROG(INTLTOOL_MERGE, [intltool-merge])
  AC_PATH_PROG(INTLTOOL_EXTRACT, [intltool-extract])
  if test -z "$INTLTOOL_UPDATE" -o -z "$INTLTOOL_MERGE" -o -z "$INTLTOOL_EXTRACT"; then
    AC_MSG_ERROR([The intltool scripts were not found. Please install intltool or use --disable-nls to ignore.])
  fi
  AC_SUBST(MOO_INTLTOOL_INI_DEPS,'$(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po)')
  AC_SUBST(MOO_INTLTOOL_INI_CMD,'$(AM''_V_GEN)LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< [$]@')
])

AC_DEFUN([_MOO_INTLTOOL_NO_NLS],[
  AC_SUBST(MOO_INTLTOOL_INI_DEPS,'')
  AC_SUBST(MOO_INTLTOOL_INI_CMD,'$(AM''_V_GEN)sed -e "s/^_//g" $< > [$]@.tmp && mv [$]@.tmp [$]@')
])

AC_DEFUN([MOO_INTL],[
  AM_GLIB_GNU_GETTEXT
  AC_ARG_ENABLE([nls],AS_HELP_STRING([--disable-nls],[do not use Native Language Support]),[
    ENABLE_NLS=$enableval
  ],[
    ENABLE_NLS=yes
  ])
  AC_SUBST([ENABLE_NLS])
  if test "$ENABLE_NLS" = "yes"; then
    _MOO_INTLTOOL
    AC_DEFINE(ENABLE_NLS, 1)
  else
    _MOO_INTLTOOL_NO_NLS
  fi
  AC_SUBST(MOO_PO_SUBDIRS_RULE,'$(top_srcdir)/po-gsv/Makefile.am: $(top_srcdir)/po/Makefile.am ; sed -e "s/GETTEXT_PACKAGE/GETTEXT_PACKAGE_GSV/g" $(top_srcdir)/po/Makefile.am > $(top_srcdir)/po-gsv/Makefile.am.tmp && mv $(top_srcdir)/po-gsv/Makefile.am.tmp $(top_srcdir)/po-gsv/Makefile.am')
])
