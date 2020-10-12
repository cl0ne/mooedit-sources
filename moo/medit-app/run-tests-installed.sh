#! /bin/sh

if [ -z $IGNORE_COVERAGE ]; then
  coverage_arg=--coverage
else
  coverage_arg=
fi

exec $srcdir/medit-app/run-tests.sh --installed $coverage_arg
