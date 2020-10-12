##############################################################################
# MOO_AC_PYTHON()
#
AC_DEFUN_ONCE([MOO_AC_PYTHON],[
  AC_REQUIRE([MOO_AC_CHECK_OS])

  MOO_ENABLE_PYTHON=true
  _moo_want_python="auto"
  _moo_python_version=2.2

  AC_ARG_WITH([python],AS_HELP_STRING([--with-python],[whether to compile python support (default = YES)]),[
    if test "x$with_python" = "xno"; then
      MOO_ENABLE_PYTHON=false
    elif test "x$with_python" = "xyes"; then
      _moo_want_python="yes"
      _moo_python_version="2.2"
    else
      _moo_want_python="yes"
      _moo_python_version="$with_python"
    fi
  ])

  if $MOO_ENABLE_PYTHON; then
    MOO_ENABLE_PYTHON=false
    MOO_AC_CHECK_PYTHON($_moo_python_version,[
      PKG_CHECK_MODULES(PYGTK,pygtk-2.0 >= 2.6.0,[
        MOO_ENABLE_PYTHON=true
      ],[:])
    ])

    if $MOO_ENABLE_PYTHON; then
      AC_SUBST([PYGTK_DEFS_DIR],[`$PKG_CONFIG --variable=defsdir pygtk-2.0`])
      AC_SUBST([PYGOBJECT_DEFS_DIR],[`$PKG_CONFIG --variable=defsdir pygobject-2.0`])
    fi

    if $MOO_ENABLE_PYTHON; then
      AC_MSG_NOTICE([compiling python support])
    elif test "x$_moo_want_python" = "xyes"; then
      AC_MSG_ERROR([python support requested but python cannot be used])
    elif test "x$_moo_want_python" = "xauto"; then
      AC_MSG_WARN([disabled python support])
    else
      AC_MSG_NOTICE([disabled python support])
    fi
  fi

  AM_CONDITIONAL(MOO_ENABLE_PYTHON, $MOO_ENABLE_PYTHON)
  if $MOO_ENABLE_PYTHON; then
    AC_DEFINE(MOO_ENABLE_PYTHON, 1, [build python bindings and plugin])
  fi

  if $MOO_ENABLE_PYTHON; then
    MOO_CFLAGS="$MOO_CFLAGS $PYGTK_CFLAGS $PYTHON_INCLUDES"
    MOO_CXXFLAGS="$MOO_CXXFLAGS $PYGTK_CFLAGS $PYTHON_INCLUDES"
    MOO_LIBS="$MOO_LIBS $PYGTK_LIBS $PYTHON_LIBS"
  fi
])
