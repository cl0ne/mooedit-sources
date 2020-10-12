#! /bin/sh

coverage=false
uninstalled=false

for arg; do
  case $arg in
    --installed)
      uninstalled=false
      ;;
    --uninstalled)
      uninstalled=true
      ;;
    --coverage)
      coverage=true
      ;;
    *)
      echo "Unknown option $arg"
      exit 1
      ;;
  esac
done

relpath() {
  from=`cd $1 && pwd`
  to=`cd $2 && pwd`
  $PYTHON -c 'import sys; import os; print os.path.relpath(sys.argv[2], sys.argv[1])' $from $to
}

if $uninstalled; then
  if ! [ -e ./medit ]; then
    echo "file ./medit doesn't exist"
    exit 1
  fi
  medit_cmd_line="./medit --ut --ut-uninstalled"
else
  if [ -z $bindir ]; then
    medit=`which medit`
  else
    medit=$bindir/medit$EXEEXT
  fi
  if ! [ -e $medit ]; then
    echo "file $medit doesn't exist"
    exit 1
  fi
  if [ ./medit -nt $medit ]; then
    echo "file ./medit is newer than '$medit', did you forget run make install?"
    exit 1
  fi
  medit_cmd_line="$medit --ut"
fi

if $coverage; then
  medit_cmd_line="$medit_cmd_line --ut-coverage called-functions"
fi

medit_cmd_line="$medit_cmd_line --ut-dir `relpath . $srcdir/medit-app/data`"

echo "$medit_cmd_line"
$medit_cmd_line || exit $?

if $coverage; then
  sort called-functions > called-functions.tmp || exit 1
  mv called-functions.tmp called-functions || exit 1

  moo_xml=$top_srcdir/api/moo.xml
  gtk_xml=$top_srcdir/api/gtk.xml
  [ -f $moo_xml ] || { echo "file $moo_xml doesn't exist"; exit 1; }
  [ -f $gtk_xml ] || { echo "file $gtk_xml doesn't exist"; exit 1; }

  $PYTHON $print_functions $moo_xml $gtk_xml --output-lua=lua-functions.tmp --output-python=python-functions.tmp || exit 1
  sort lua-functions.tmp > lua-functions.tmp2 && mv lua-functions.tmp2 lua-functions || exit 1
  sort python-functions.tmp > python-functions.tmp2 && mv python-functions.tmp2 python-functions || exit 1
  cat lua-functions python-functions | sort | uniq > all-functions
  rm -f *-functions.tmp*

  grep --extended-regexp 'lua\.' called-functions | sed --regexp-extended 's/(lua|python)\.//g' | sort | uniq > lua-called-functions
  grep --extended-regexp 'python\.' called-functions | sed --regexp-extended 's/(lua|python)\.//g' | sort | uniq > python-called-functions
  sed --regexp-extended 's/(lua|python)\.//g' called-functions | sort | uniq > all-called-functions

  comm -3 -2 lua-functions lua-called-functions > not-covered-lua-functions
  comm -3 -2 python-functions python-called-functions > not-covered-python-functions
  comm -3 -2 all-functions all-called-functions > not-covered-functions

  if [ -z "$IGNORE_COVERAGE" ] && [ -s not-covered-functions ]; then
    echo "*** Not all functions are covered, see file not-covered-functions"
    exit 1
  else
    rm -f *-functions *-called-functions called-functions not-covered-*-functions not-covered-functions
    exit 0
  fi
fi
