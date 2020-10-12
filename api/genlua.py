#! /usr/bin/env python

import os
import sys
import optparse

from mpi.module import Module
from mpi.luawriter import Writer

op = optparse.OptionParser()
op.add_option("-i", "--import", action="append", dest="import_modules")
op.add_option("--include-header", action="append", dest="include_headers")
op.add_option("--output", dest="output")
(opts, args) = op.parse_args()

import_modules = []
if opts.import_modules:
    for filename in opts.import_modules:
        import_modules.append(Module.from_xml(filename))

if opts.output:
    out_file = open(opts.output + '.tmp', 'w')
else:
    out_file = sys.stdout

assert len(args) == 1
mod = Module.from_xml(args[0])
for im in import_modules:
    mod.import_module(im)
Writer(out_file).write(mod, opts.include_headers)

if opts.output:
    out_file.close()
    if os.access(opts.output, os.F_OK):
        os.remove(opts.output)
    os.rename(opts.output + '.tmp', opts.output)
