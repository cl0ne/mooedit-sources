import sys
import xml.etree.ElementTree as etree
import optparse

op = optparse.OptionParser()
op.add_option("--output-python", action="store")
op.add_option("--output-lua", action="store")
(opts, args) = op.parse_args()

lua_functions = []
python_functions = []

def walk(elm, lang, out, cls=None):
    if elm.get('moo.private') == '1' or elm.get('moo.' + lang) == '0':
        return
    if elm.tag in ('module', 'class', 'boxed', 'pointer', 'enum', 'flags'):
        for child in elm.getchildren():
            walk(child, lang, out, elm.get('name'))
    elif elm.tag in ('static-method', 'method', 'function', 'constructor'):
        print >>out, elm.get('c_name')
    elif elm.tag in ('signal'):
        print >>out, '%s::%s' % (cls, elm.get('name'))

lua_out = open(opts.output_lua, 'w')
python_out = open(opts.output_python, 'w')

for f in args:
    xml = etree.parse(f)
    walk(xml.getroot(), 'lua', lua_out)
    walk(xml.getroot(), 'python', python_out)

lua_out.close()
python_out.close()
