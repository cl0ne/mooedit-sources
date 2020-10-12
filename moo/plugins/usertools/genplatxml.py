import os
import sys
import re
import optparse

op = optparse.OptionParser()
op.add_option("--enable", action="append")
(opts, args) = op.parse_args()

assert opts.enable
assert len(args) == 1

in_command = False
skip = False
for line in open(args[0]):
    m = re.search(r'<(/?)command\b.*>', line)
    if not m:
        if not skip:
            sys.stdout.write(line)
    elif m.group(1):
        if not skip:
            sys.stdout.write(line)
        skip = False
        in_command = False
    else:
        assert not skip
        assert not in_command
        in_command = True
        pat = r'<!--\s*###([^#]+)###\s*-->'
        m = re.search(pat, line)
        if m:
            skip = not m.group(1) in opts.enable
        if not skip:
            sys.stdout.write(re.sub(pat, '', line))
