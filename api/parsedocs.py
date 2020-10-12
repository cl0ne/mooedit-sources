#! /usr/bin/env python

import os
import re
import sys
import optparse
import fnmatch
import filecmp

from mdp.docparser import Parser
from mdp.module import Module
import mdp.xmlwriter

def read_files(files, opts):
    p = Parser()
    p.read_files(files)
    mod = Module('Moo' if not opts.module else opts.module)
    mod.init_from_dox(p.classes + p.enums + p.functions + p.vmethods + p.signals)
    return mod

def parse_args():
    op = optparse.OptionParser()
    op.add_option("--source-dir", dest="source_dirs", action="append",
                  help="parse source files from DIR", metavar="DIR")
    op.add_option("--source-file", dest="source_files", action="append",
                  help="parse source file FILE", metavar="FILE")
    op.add_option("--skip", dest="skip_globs", action="append",
                  help="skip files which match pattern PAT", metavar="PAT")
    op.add_option("--output", dest="output", action="store",
                  help="write result to FILE", metavar="FILE")
    op.add_option("--module", dest="module", action="store",
                  help="generate module MOD", metavar="MOD")
    (opts, args) = op.parse_args()
    if args:
        op.error("too many arguments")
    source_files = []
    if opts.source_files:
        source_files += [os.path.abspath(f) for f in opts.source_files]
    skip_pat = None
    if opts.skip_globs:
        skip_pat = re.compile('|'.join([fnmatch.translate(g) for g in opts.skip_globs]))
    if opts.source_dirs:
        for source_dir in opts.source_dirs:
            for root, dirs, files in os.walk(source_dir):
                for f in files:
                    if f.endswith('.c') or f.endswith('.cpp') or f.endswith('.h'):
                        if skip_pat is None or not skip_pat.match(f):
                            source_files.append(os.path.join(root, f))
    if not source_files:
        op.error("no input files")
    return opts, source_files

opts, files = parse_args()
mod = read_files(files, opts)
tmp_file = opts.output + '.tmp' 
with open(tmp_file, 'w') as out:
    mdp.xmlwriter.write_xml(mod, out)

if not os.path.exists(opts.output):
    os.rename(tmp_file, opts.output)
elif filecmp.cmp(tmp_file, opts.output):
    os.remove(tmp_file)
else:
    os.remove(opts.output)
    os.rename(tmp_file, opts.output)
