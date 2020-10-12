m4_define([_moo_major_version_],[1])
m4_define([_moo_minor_version_],[2])
m4_define([_moo_micro_version_],[92])
m4_define([_moo_version_suffix_],[devel])
m4_define([_moo_module_major_version_],[2])
m4_define([_moo_module_minor_version_],[0])
m4_define([_moo_version_],[_moo_major_version_._moo_minor_version_._moo_micro_version_])

m4_if(_moo_version_suffix_,[],
  [m4_define([_moo_display_version_],[_moo_version_])],
  [m4_define([_moo_display_version_],[_moo_version_-_moo_version_suffix_])])

m4_define([_moo_website_],[http://mooedit.sourceforge.net/])
m4_define([_moo_web_contact_],[http://mooedit.sourceforge.net/contact.html])
m4_define([_moo_email_],[emuntyan@users.sourceforge.net])
m4_define([_moo_copyright_],[2004-2014 Yevgen Muntyan <_moo_email_>])

AC_PREREQ([2.68])
AC_INIT([medit],[_moo_display_version_],[_moo_email_])
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST(ACLOCAL_FLAGS)

AM_INIT_AUTOMAKE([1.11 foreign dist-bzip2 no-dist-gzip subdir-objects])
AM_SILENT_RULES([yes])
LT_INIT([disable-shared])
# AC_PROG_RANLIB

AC_SUBST(MOO_MAJOR_VERSION,_moo_major_version_)
AC_SUBST(MOO_MINOR_VERSION,_moo_minor_version_)
AC_SUBST(MOO_MICRO_VERSION,_moo_micro_version_)
AC_SUBST(MOO_MODULE_MAJOR_VERSION,_moo_module_major_version_)
AC_SUBST(MOO_MODULE_MINOR_VERSION,_moo_module_minor_version_)
AC_SUBST(MOO_VERSION,_moo_version_)
AC_SUBST(MOO_DISPLAY_VERSION,"_moo_display_version_")
AC_SUBST(MOO_EMAIL,"_moo_email_")
AC_SUBST(MOO_WEBSITE,"_moo_website_")
AC_SUBST(MOO_WEB_CONTACT,"_moo_web_contact_")
AC_SUBST(MOO_COPYRIGHT,"_moo_copyright_")

# keep in sync with po/maintain
AC_SUBST(MOO_PACKAGE_NAME,"medit-1")

MOO_PREFS_XML_FILE_NAME="prefs.xml"
MOO_STATE_XML_FILE_NAME="state.xml"
MOO_NAMED_SESSION_XML_FILE_NAME="session-%s.xml"
MOO_SESSION_XML_FILE_NAME="session.xml"
MEDIT_PORTABLE_MAGIC_FILE_NAME="medit-portable"
MEDIT_PORTABLE_DATA_DIR="medit-portable-data"
MEDIT_PORTABLE_CACHE_DIR="medit-portable-cache"
AC_SUBST(MEDIT_PORTABLE_MAGIC_FILE_NAME,$MEDIT_PORTABLE_MAGIC_FILE_NAME)

AC_SUBST(GETTEXT_PACKAGE,$MOO_PACKAGE_NAME)
AC_SUBST(GETTEXT_PACKAGE_GSV,"$MOO_PACKAGE_NAME-gsv")
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[GETTEXT_PACKAGE])
MOO_INTL

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AM_PROG_CC_C_O

AC_ARG_VAR([WINDRES], [windres])
AC_CHECK_TOOL(WINDRES, windres, :)

AC_ARG_VAR([MOO_PYTHON], [Python executable, required to run build scripts])
AC_CHECK_PROGS(MOO_PYTHON, [python2 python2.7 python2.6 python2.5 python2.4 python2.3 python2.2 python2.1 python2.0 python], [none])
if test "x$MOO_PYTHON" = "x"; then
  AC_MSG_ERROR([Python not found, it is required for the build])
fi

MOO_AC_FLAGS

AM_MAINTAINER_MODE([enable])

AC_CHECK_PROG([TXT2TAGS], txt2tags, txt2tags)
if test x$MOO_DEV_MODE = "xyes" -a "x$TXT2TAGS" = "x"; then
  AC_MSG_ERROR([txt2tags not found])
fi

AC_CHECK_PROG([XSLTPROC], xsltproc, xsltproc)
if test x$MOO_DEV_MODE = "xyes" -a "x$XSLTPROC" = "x"; then
  AC_MSG_ERROR([xsltproc not found])
fi

AC_DEFINE_UNQUOTED(MOO_CONFIG_H_INCLUDED, 1, MOO_CONFIG_H_INCLUDED)
AC_CONFIG_HEADERS([config.h])
AH_TOP([#ifndef __CONFIG_H__
#define __CONFIG_H__])
AH_BOTTOM([#include <moo-config.h>
#endif // __CONFIG_H__])

MOO_LT_LIB_M

AC_DEFINE_UNQUOTED(MOO_VERSION,["$MOO_VERSION"],[MOO_VERSION])
AC_DEFINE_UNQUOTED(MOO_DISPLAY_VERSION,["$MOO_DISPLAY_VERSION"],[MOO_DISPLAY_VERSION])
AC_DEFINE_UNQUOTED(MOO_MAJOR_VERSION,[$MOO_MAJOR_VERSION],[MOO_MAJOR_VERSION])
AC_DEFINE_UNQUOTED(MOO_MICRO_VERSION,[$MOO_MICRO_VERSION],[MOO_MICRO_VERSION])
AC_DEFINE_UNQUOTED(MOO_MINOR_VERSION,[$MOO_MINOR_VERSION],[MOO_MINOR_VERSION])
AC_DEFINE_UNQUOTED(MOO_MODULE_MAJOR_VERSION,[$MOO_MODULE_MAJOR_VERSION],[MOO_MODULE_MAJOR_VERSION])
AC_DEFINE_UNQUOTED(MOO_MODULE_MINOR_VERSION,[$MOO_MODULE_MINOR_VERSION],[MOO_MODULE_MINOR_VERSION])

AC_DEFINE_UNQUOTED(MOO_EMAIL,["$MOO_EMAIL"],MOO_EMAIL)
AC_DEFINE_UNQUOTED(PACKAGE_BUGREPORT,["$PACKAGE_BUGREPORT"],PACKAGE_BUGREPORT)
AC_DEFINE_UNQUOTED(MOO_COPYRIGHT,["$MOO_COPYRIGHT"],MOO_COPYRIGHT)
AC_DEFINE_UNQUOTED(MOO_WEBSITE,["$MOO_WEBSITE"],MOO_WEBSITE)
AC_DEFINE_UNQUOTED(MOO_WEB_CONTACT,["$MOO_WEB_CONTACT"],MOO_WEB_CONTACT)

MOO_AC_CHECK_OS
MOO_AC_PYTHON

MOO_BUILD_APP=true
MOO_BUILD_MODULE=false
AC_ARG_ENABLE(moo-module,
  AS_HELP_STRING([--enable-moo-module],[build standalone python module instead of medit, default NO (you must also use --enable-shared with this option)]),[
  if test "$enable_moo_module" = "yes"; then
    MOO_BUILD_APP=false
    MOO_BUILD_MODULE=true
  fi
])
if $MOO_BUILD_MODULE; then
  if ! $MOO_ENABLE_PYTHON; then
    AC_MSG_ERROR([Python bindings are not enabled, can't build python module])
  elif test "$enable_shared" != "yes"; then
    AC_MSG_ERROR([Python module can't be built without --enable-shared option])
  fi
fi

AM_CONDITIONAL(MOO_BUILD_APP, [$MOO_BUILD_APP])
AM_CONDITIONAL(MOO_BUILD_MODULE, [$MOO_BUILD_MODULE])

AC_SUBST(MOO_TOP_SRCDIR,`cd $srcdir && pwd`)
if test x"$MOO_TOP_SRCDIR" = x"`pwd`"; then
  AC_SUBST(MOO_CP_TO_SRCDIR,true)
  AC_SUBST(MOO_MV_TO_SRCDIR,true)
else
  AC_SUBST(MOO_CP_TO_SRCDIR,cp)
  AC_SUBST(MOO_MV_TO_SRCDIR,mv)
fi

AM_CONDITIONAL(MOO_BUILD_CTAGS, [$MOO_OS_UNIX])
if $MOO_OS_UNIX; then
  AC_DEFINE(MOO_BUILD_CTAGS, 1, [build ctags plugin])
fi

AC_ARG_ENABLE(coverage,
  AS_HELP_STRING([--enable-coverage],[check test coverage]),[
  MOO_ENABLE_COVERAGE="$enable_coverage"
  ],[
  MOO_ENABLE_COVERAGE="no"
])
AM_CONDITIONAL(MOO_ENABLE_COVERAGE, test "x$MOO_ENABLE_COVERAGE" = "xyes")
if test "x$MOO_ENABLE_COVERAGE" = "xyes"; then
  AC_DEFINE(MOO_ENABLE_COVERAGE, 1, [enable code coverage testing])
fi

if $MOO_OS_WIN32; then
  _moo_cygwin=false
  if test "$OSTYPE" = "cygwin"; then
    _moo_cygwin=true
  fi

  if $_moo_cygwin; then
    MEDIT_INNO_COMPILER='"/cygdrive/c/Program Files (x86)/Inno Setup 5/ISCC.exe"'
  elif test -d $HOME/.wine/drive_c/windows/syswow64 -o -d "/cygdrive/c/Program Files (x86)"; then
    MEDIT_INNO_COMPILER='wine "c:\\program files (x86)\\inno setup 5\\ISCC.exe"'
  else
    MEDIT_INNO_COMPILER='wine "c:\\program files\\inno setup 5\\ISCC.exe"'
  fi
  AC_MSG_CHECKING([Inno compiler])
  AC_MSG_RESULT([$MEDIT_INNO_COMPILER])

  _moo_win32_config=release
  if test "$MOO_DEBUG_ENABLED" = "yes"; then
    _moo_win32_config="debug"
  fi
  if test -n "$mgwconfig"; then
    _moo_win32_config="$mgwconfig"
  fi
  AC_ARG_VAR([MOO_WIN32_CONFIG], [$_moo_win32_config])
  AC_MSG_CHECKING([Win32 build configuration])
  AC_MSG_RESULT([$MOO_WIN32_CONFIG])

  AC_SUBST(MEDIT_WIN32_APP_UID,"7F9F899F-EE8A-47F0-9981-8C525AF78E4D")

  if $_moo_cygwin; then
    AC_SUBST(MEDIT_INNO_TOP_SRCDIR,"`cygpath -m $MOO_TOP_SRCDIR`")
    _moo_this_dir=`pwd`
    AC_SUBST(MEDIT_INNO_TOP_BUILDDIR,"`cygpath -m $_moo_this_dir`")
    AC_SUBST(MEDIT_INNO_INSTDIR,"`cygpath -m $prefix`")
  else
    AC_SUBST(MEDIT_INNO_TOP_SRCDIR,"Z:$MOO_TOP_SRCDIR")
    AC_SUBST(MEDIT_INNO_TOP_BUILDDIR,"Z:`pwd`")
    AC_SUBST(MEDIT_INNO_INSTDIR,"Z:`cd $prefix && pwd`")
  fi

  AC_MSG_CHECKING([MEDIT_INNO_TOP_SRCDIR])
  AC_MSG_RESULT([$MEDIT_INNO_TOP_SRCDIR])
  AC_MSG_CHECKING([MEDIT_INNO_TOP_BUILDDIR])
  AC_MSG_RESULT([$MEDIT_INNO_TOP_BUILDDIR])
  AC_MSG_CHECKING([MEDIT_INNO_INSTDIR])
  AC_MSG_RESULT([$MEDIT_INNO_INSTDIR])

  AC_SUBST(MEDIT_INNO_COMPILER)
  MEDIT_INNO_INSTALLER_SUFFIX=""
  if test -n "$MOO_WIN32_CONFIG" -a "$MOO_WIN32_CONFIG" != "release"; then
    MEDIT_INNO_INSTALLER_SUFFIX="-$MOO_WIN32_CONFIG"
  fi
  AC_SUBST(MEDIT_SETUP_NAME, "medit-$MOO_DISPLAY_VERSION$MEDIT_INNO_INSTALLER_SUFFIX")
  AC_MSG_CHECKING([Win32 setup name])
  AC_MSG_RESULT([$MEDIT_SETUP_NAME])
fi

AC_ARG_ENABLE(install-hooks,
  AS_HELP_STRING([--enable-install-hooks],[run gtk-update-icon-cache during 'make install']),[
  MOO_ENABLE_INSTALL_HOOKS="$enable_install_hooks"
  ],[
  if $MOO_BUILD_APP; then
    MOO_ENABLE_INSTALL_HOOKS="yes"
  else
    MOO_ENABLE_INSTALL_HOOKS="no"
  fi
])
AM_CONDITIONAL(MOO_ENABLE_INSTALL_HOOKS, test "x$MOO_ENABLE_INSTALL_HOOKS" = "xyes")

AC_CONFIG_FILES([
  Makefile
  api/Makefile
  doc/Makefile
  moo/Makefile
  po/Makefile
  po-gsv/Makefile
])
AC_OUTPUT
