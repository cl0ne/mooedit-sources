#! /usr/bin/env python

import sys

from mpi.module import Module
from mpi.defswriter import Writer

for arg in sys.argv[1:]:
    mod = Module.from_xml(arg)
    Writer(sys.stdout).write(mod)
