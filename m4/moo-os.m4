AC_DEFUN([MOO_AC_CHECK_OS],[
AC_REQUIRE([AC_CANONICAL_HOST])

  m4_define([_moo_oses_],[CYGWIN WIN32 MINGW DARWIN UNIX FREEBSD BSD LINUX FDO])

  m4_foreach_w([_moo_os_],_moo_oses_,[dnl
MOO_OS_[]_moo_os_=false
])

  case $host in
    *-*-mingw32*)
      MOO_OS_WIN32=true
      MOO_OS_NAME="Win32"
      ;;
    *-*-cygwin*)
      MOO_OS_CYGWIN=true
      MOO_OS_NAME="CygWin"
      ;;
    *-*-darwin*)
      MOO_OS_DARWIN=true
      MOO_OS_NAME="Darwin"
      ;;
    *-*-freebsd*)
      MOO_OS_FREEBSD=true
      MOO_OS_NAME="FreeBSD"
      ;;
    *-*-linux*)
      MOO_OS_LINUX=true
      MOO_OS_NAME="Linux"
      ;;
    *)
      MOO_OS_UNIX=true
      MOO_OS_NAME="Unix"
      ;;
  esac

  if $MOO_OS_WIN32; then : ; else MOO_OS_UNIX=true; fi
  if $MOO_OS_DARWIN; then MOO_OS_BSD=true; fi
  if $MOO_OS_FREEBSD; then MOO_OS_BSD=true; fi

  m4_foreach_w([_moo_os_],_moo_oses_,[dnl
AM_CONDITIONAL(MOO_OS_[]_moo_os_,[$MOO_OS_[]_moo_os_])
])

])

# LT_LIB_M macro from libtool.m4
AC_DEFUN([MOO_LT_LIB_M],
[AC_REQUIRE([AC_CANONICAL_HOST])dnl
LIBM=
case $host in
*-*-beos* | *-*-cegcc* | *-*-cygwin* | *-*-haiku* | *-*-pw32* | *-*-darwin*)
  # These system don't have libm, or don't need it
  ;;
*-ncr-sysv4.3*)
  AC_CHECK_LIB(mw, _mwvalidcheckl, LIBM="-lmw")
  AC_CHECK_LIB(m, cos, LIBM="$LIBM -lm")
  ;;
*)
  AC_CHECK_LIB(m, cos, LIBM="-lm")
  ;;
esac
AC_SUBST([LIBM])
])# LT_LIB_M
