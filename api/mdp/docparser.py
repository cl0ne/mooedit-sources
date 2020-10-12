import re
import string
import sys
import os

DEBUG = False

class ParseError(RuntimeError):
    def __init__(self, message, block=None):
        if block:
            RuntimeError.__init__(self, '%s in file %s, around line %d' % \
                                    (message, block.filename, block.first_line))
        else:
            RuntimeError.__init__(self, message)

class DoxBlock(object):
    def __init__(self, block):
        object.__init__(self)

        self.symbol = None
        self.annotations = []
        self.params = []
        self.attributes = []
        self.docs = []
        self.summary = None

        self.__parse(block)

    def __parse(self, block):
        first_line = True
        chunks = []
        cur = [None, None, []]
        for line in block.lines:
            if first_line:
                m = re.match(r'([\w\d_.-]+(:+[\w\d_.-]+)*)(:(\s.*)?)?$', line)
                if m is None:
                    raise ParseError('bad id line', block)
                cur[0] = m.group(1)
                annotations, docs = self.__parse_annotations(m.group(4) or '')
                cur[1] = annotations
                cur[2] = [docs] if docs else []
            elif not line:
                if cur[0] is not None:
                    chunks.append(cur)
                    cur = [None, None, []]
                elif cur[2]:
                    cur[2].append(line)
            else:
                m = re.match(r'(@[\w\d_.-]+|Returns|Return value|Since):(.*)$', line)
                if m:
                    if cur[0] or cur[2]:
                        chunks.append(cur)
                        cur = [None, None, []]
                    cur[0] = m.group(1)
                    annotations, docs = self.__parse_annotations(m.group(2) or '')
                    cur[1] = annotations
                    cur[2] = [docs] if docs else []
                else:
                    cur[2].append(line)
            first_line = False
        if cur[0] or cur[2]:
            chunks.append(cur)

        self.symbol = chunks[0][0]
        self.annotations = chunks[0][1]
        self.summary = chunks[0][2]

        for chunk in chunks[1:]:
            if chunk[0]:
                if chunk[0].startswith('@'):
                    self.params.append(chunk)
                else:
                    self.attributes.append(chunk)
            else:
                self.docs = chunk[2]

        if not self.symbol:
            raise ParseError('bad id line', block)

    def __parse_annotations(self, text):
        annotations = []
        ann_start = -1
        for i in xrange(len(text)):
            c = text[i]
            if c in ' \t':
                pass
            elif c == ':':
                if ann_start < 0:
                    if annotations:
                        return annotations, text[i+1:].strip()
                    else:
                        return None, text
                else:
                    pass
            elif c != '(' and ann_start < 0:
                if annotations:
                    raise ParseError('bad annotations')
                return None, text
            elif c == '(':
                if ann_start >= 0:
                    raise ParseError('( inside ()')
                ann_start = i
            elif c == ')':
                assert ann_start >= 0
                if ann_start + 1 < i:
                    annotations.append(text[ann_start+1:i])
                ann_start = -1
        if ann_start >= 0:
            raise ParseError('unterminated annotation')
        return annotations, None

class Block(object):
    def __init__(self, lines, filename, first_line, last_line):
        object.__init__(self)
        self.lines = list(lines)
        self.filename = filename
        self.first_line = first_line
        self.last_line = last_line

class Symbol(object):
    def __init__(self, name, annotations, docs, block):
        object.__init__(self)
        self.name = name
        self.annotations = annotations or []
        self.docs = docs
        self.block = block

class Class(Symbol):
    def __init__(self, name, annotations, docs, block):
        Symbol.__init__(self, name, annotations, docs, block)

class Boxed(Symbol):
    def __init__(self, name, annotations, docs, block):
        Symbol.__init__(self, name, annotations, docs, block)

class Pointer(Symbol):
    def __init__(self, name, annotations, docs, block):
        Symbol.__init__(self, name, annotations, docs, block)

class EnumBase(Symbol):
    def __init__(self, name, annotations, docs, values, block):
        super(EnumBase, self).__init__(name, annotations, docs, block)
        self.values = values

class Enum(EnumBase):
    def __init__(self, name, annotations, docs, values, block):
        super(Enum, self).__init__(name, annotations, docs, values, block)

class Flags(EnumBase):
    def __init__(self, name, annotations, docs, values, block):
        super(Flags, self).__init__(name, annotations, docs, values, block)

class EnumValue(object):
    def __init__(self, name, annotations=None, docs=None):
        object.__init__(self)
        self.docs = docs
        self.annotations = annotations or []
        self.name = name

class FunctionBase(Symbol):
    def __init__(self, name, annotations, params, retval, docs, block):
        Symbol.__init__(self, name, annotations, docs, block)
        self.params = params
        self.retval = retval

class Signal(FunctionBase):
    def __init__(self, name, annotations, params, retval, docs, block):
        FunctionBase.__init__(self, name, annotations, params, retval, docs, block)

class Function(FunctionBase):
    def __init__(self, name, annotations, params, retval, docs, block):
        FunctionBase.__init__(self, name, annotations, params, retval, docs, block)

class VMethod(Function):
    def __init__(self, name, annotations, params, retval, docs, block):
        Function.__init__(self, name, annotations, params, retval, docs, block)

class ParamBase(object):
    def __init__(self, annotations=[], docs=None):
        object.__init__(self)
        self.docs = docs
        self.type = None
        self.annotations = annotations or []

class Param(ParamBase):
    def __init__(self, name=None, annotations=None, docs=None):
        ParamBase.__init__(self, annotations, docs)
        self.name = name

class Retval(ParamBase):
    def __init__(self, annotations=None, docs=None):
        ParamBase.__init__(self, annotations, docs)

class Parser(object):
    def __init__(self):
        object.__init__(self)
        self.classes = []
        self.enums = []
        self.functions = []
        self.signals = []
        self.vmethods = []
        self.__symbol_dict = {}

    def __split_block(self, block):
        chunks = []
        current_prefix = None
        current_annotations = None
        current_text = None
        first_line = True

        re_id = re.compile(r'([\w\d._-]+:(((\s*\([^()]\)\s*)+):)?')
        re_special = re.compile(r'(@[\w\d._-]+|(SECTION|PROPERTY|SIGNAL)-[\w\d._-]+||Since|Returns|Return value):(((\s*\([^()]\)\s*)+):)?')

        for line in block.lines:
            if first_line:
                line = re.sub(r'^SECTION:([\w\d_-]+):?', r'SECTION-\1:', line)
                line = re.sub(r'^([\w\d_-]+):([\w\d_-]+):?', r'PROPERTY-\1-\2:', line)
                line = re.sub(r'^([\w\d_-]+)::([\w\d_-]+):?', r'SIGNAL-\1-\2:', line)
            first_line = False
            if not line:
                if current_prefix is not None:
                    chunks.append([current_prefix, current_annotations, current_text])
                    current_prefix = None
                    current_annotations = None
                    current_text = None
                elif current_text is not None:
                    current_text.append(line)
            else:
                m = re_special.match(line)
                if m:
                    if current_prefix is not None or current_text is not None:
                        chunks.append([current_prefix, current_annotations, current_text])
                        current_prefix = None
                        current_annotations = None
                        current_text = None
                    current_prefix = m.group(1)
                    suffix = m.group(4) or ''
                    annotations, text = self.__parse_annotations(suffix)
                    current_annotations = annotations
                    current_text = [text] if text else []
                else:
                    if current_text is not None:
                        current_text.append(line)
                    else:
                        current_text = [line]
        if current_text is not None:
            chunks.append([current_prefix, current_annotations, current_text])

        return chunks

    def __parse_annotations(self, text):
        annotations = []
        ann_start = -1
        for i in xrange(len(text)):
            c = text[i]
            if c in ' \t':
                pass
            elif c == ':':
                if ann_start < 0:
                    if annotations:
                        return annotations, text[i+1:].strip()
                    else:
                        return None, text
                else:
                    pass
            elif c != '(' and ann_start < 0:
                if annotations:
                    raise ParseError('bad annotations')
                return None, text
            elif c == '(':
                if ann_start >= 0:
                    raise ParseError('( inside ()')
                ann_start = i
            elif c == ')':
                assert ann_start >= 0
                if ann_start + 1 < i:
                    annotations.append(text[ann_start+1:i])
                ann_start = -1
        if ann_start >= 0:
            raise ParseError('unterminated annotation')
        if annotations:
            return annotations, None
        else:
            return None, None

    def __parse_function(self, block):
        db = DoxBlock(block)

        params = []
        retval = None

        for p in db.params:
            params.append(Param(p[0][1:], p[1], p[2]))
        for attr in db.attributes:
            if attr[0] in ('Returns', 'Return value'):
                retval = Retval(attr[1], attr[2])
            elif attr[0] in ('Since'):
                pass
            else:
                raise ParseError('unknown attribute %s' % (attr[0],), block)

        if '::' in db.symbol:
            What = Signal
            db.symbol = 'signal:' + db.symbol.replace('_', '-').replace('::', ':')
            symbol_list = self.signals
        elif ':' in db.symbol:
            What = Property
            db.symbol = 'property:' + db.symbol.replace('_', '-')
            symbol_list = self.properties
        else:
            What = Function
            symbol_list = self.functions

        func = What(db.symbol, db.annotations, params, retval, db.docs, block)
        func.summary = db.summary
        if DEBUG:
            print 'func.name:', func.name
        if func.name in self.__symbol_dict:
            raise ParseError('duplicated symbol %s' % (func.name,), block)
        self.__symbol_dict[func.name] = func
        symbol_list.append(func)

    def __parse_class(self, block):
        db = DoxBlock(block)

        name = db.symbol
        if name.startswith('class:'):
            name = name[len('class:'):]

        if db.params:
            raise ParseError('class params', block)
        if db.attributes:
            raise ParseError('class attributes', block)

        cls = Class(name, db.annotations, db.docs, block)
        cls.summary = db.summary
        self.classes.append(cls)

    def __parse_boxed(self, block):
        What = None
        db = DoxBlock(block)

        name = db.symbol
        if name.startswith('boxed:'):
            What = Boxed
            name = name[len('boxed:'):]
        elif name.startswith('pointer:'):
            What = Pointer
            name = name[len('pointer:'):]
        else:
            raise ParseError('bad id', block)

        if db.params:
            raise ParseError('boxed params', block)
        if db.attributes:
            raise ParseError('boxed attributes', block)

        cls = What(name, db.annotations, db.docs, block)
        cls.summary = db.summary
        self.classes.append(cls)

    def __parse_enum_or_flags(self, block, What):
        db = DoxBlock(block)

        name = db.symbol
        prefix = 'enum:' if What is Enum else 'flags:'
        if name.startswith(prefix):
            name = name[len(prefix):]

        if db.attributes:
            raise ParseError('enum attributes', block)

        values = []
        if db.params:
            for p in db.params:
                values.append(EnumValue(p[0][1:], p[1], p[2]))

        enum = What(name, db.annotations, db.docs, values, block)
        enum.summary = db.summary
        self.enums.append(enum)

    def __parse_enum(self, block):
        return self.__parse_enum_or_flags(block, Enum)

    def __parse_flags(self, block):
        return self.__parse_enum_or_flags(block, Flags)

    def __parse_block(self, block):
        line = block.lines[0]
        if line.startswith('class:'):
            self.__parse_class(block)
        elif line.startswith('boxed:'):
            self.__parse_boxed(block)
        elif line.startswith('pointer:'):
            self.__parse_boxed(block)
        elif line.startswith('enum:'):
            self.__parse_enum(block)
        elif line.startswith('flags:'):
            self.__parse_flags(block)
        elif line.startswith('SECTION:'):
            pass
        else:
            self.__parse_function(block)

    def __add_block(self, block, filename, first_line, last_line):
        lines = []
        for line in block:
            if line.startswith('*'):
                line = line[1:].strip()
            if line or lines:
                lines.append(line)
            else:
                first_line += 1
        i = len(lines) - 1
        while i >= 0:
            if not lines[i]:
                del lines[i]
                i -= 1
                last_line -= 1
            else:
                break
        if lines:
            assert last_line >= first_line
            self.__parse_block(Block(lines, filename, first_line, last_line))

    def __read_comments(self, filename):
        block = None
        first_line = 0
        line_no = 0
        for line in open(filename):
            line = line.strip()
            if not block:
                if line.startswith('/**'):
                    line = line[3:]
                    if not line.startswith('*') and not '*/' in line:
                        block = [line]
                        first_line = line_no
            else:
                end = line.find('*/')
                if end >= 0:
                    line = line[:end]
                    block.append(line)
                    self.__add_block(block, filename, first_line, line_no)
                    block = None
                else:
                    block.append(line)
            line_no += 1
        if block:
            raise ParseError('unterminated block in file %s' % (filename,))

    def read_files(self, filenames):
        sys.stderr.write('parsing gtk-doc comments... ')
        sys.stderr.flush()
        for f in filenames:
            self.__read_comments(f)
        sys.stderr.write('done\n')
        sys.stderr.write('parsing declarations... ')
        sys.stderr.flush()
        for f in filenames:
            if f.endswith('.h'):
                self.__read_declarations(f)
        sys.stderr.write('done\n')

    # Code copied from h2def.py by Toby D. Reeves <toby@max.rl.plh.af.mil>

    def __strip_comments(self, buf):
        parts = []
        lastpos = 0
        while 1:
            pos = string.find(buf, '/*', lastpos)
            if pos >= 0:
                if buf[pos:pos+len('/**vtable:')] == '/**vtable:':
                    parts.append(buf[lastpos:pos+len('/**vtable:')])
                    lastpos = pos + len('/**vtable:')
                elif buf[pos:pos+len('/**signal:')] == '/**signal:':
                    parts.append(buf[lastpos:pos+len('/**signal:')])
                    lastpos = pos + len('/**signal:')
                else:
                    parts.append(buf[lastpos:pos])
                    pos = string.find(buf, '*/', pos)
                    if pos >= 0:
                        lastpos = pos + 2
                    else:
                        break
            else:
                parts.append(buf[lastpos:])
                break
        return string.join(parts, '')

    # Strips the dll API from buffer, for example WEBKIT_API
    def __strip_dll_api(self, buf):
        pat = re.compile("[A-Z]*_API ")
        buf = pat.sub("", buf)
        return buf

    def __clean_func(self, buf):
        """
        Ideally would make buf have a single prototype on each line.
        Actually just cuts out a good deal of junk, but leaves lines
        where a regex can figure prototypes out.
        """
        # bulk comments
        buf = self.__strip_comments(buf)

        # dll api
        buf = self.__strip_dll_api(buf)

        # compact continued lines
        pat = re.compile(r"""\\\n""", re.MULTILINE)
        buf = pat.sub('', buf)

        # Preprocess directives
        pat = re.compile(r"""^[#].*?$""", re.MULTILINE)
        buf = pat.sub('', buf)

        #typedefs, stucts, and enums
        pat = re.compile(r"""^(typedef|struct|enum)(\s|.|\n)*?;\s*""",
                         re.MULTILINE)
        buf = pat.sub('', buf)

        #strip DECLS macros
        pat = re.compile(r"""G_(BEGIN|END)_DECLS|(BEGIN|END)_LIBGTOP_DECLS""", re.MULTILINE)
        buf = pat.sub('', buf)

        #extern "C"
        pat = re.compile(r"""^\s*(extern)\s+\"C\"\s+{""", re.MULTILINE)
        buf = pat.sub('', buf)

        #multiple whitespace
        pat = re.compile(r"""\s+""", re.MULTILINE)
        buf = pat.sub(' ', buf)

        #clean up line ends
        pat = re.compile(r""";\s*""", re.MULTILINE)
        buf = pat.sub('\n', buf)
        buf = buf.lstrip()

        #associate *, &, and [] with type instead of variable
        #pat = re.compile(r'\s+([*|&]+)\s*(\w+)')
        pat = re.compile(r' \s* ([*|&]+) \s* (\w+)', re.VERBOSE)
        buf = pat.sub(r'\1 \2', buf)
        pat = re.compile(r'\s+ (\w+) \[ \s* \]', re.VERBOSE)
        buf = pat.sub(r'[] \1', buf)

        buf = string.replace(buf, '/** vtable:', '/**vtable:')
        pat = re.compile(r'(\w+) \s* \* \s* \(', re.VERBOSE)
        buf = pat.sub(r'\1* (', buf)

        # make return types that are const work.
        buf = re.sub(r'\s*\*\s*G_CONST_RETURN\s*\*\s*', '** ', buf)
        buf = string.replace(buf, 'G_CONST_RETURN ', 'const-')
        buf = string.replace(buf, 'const ', 'const-')

        #strip GSEAL macros from the middle of function declarations:
        pat = re.compile(r"""GSEAL""", re.VERBOSE)
        buf = pat.sub('', buf)

        return buf

    def __read_declarations_in_buf(self, buf, filename):
        vproto_pat=re.compile(r"""
        /\*\*\s*(?P<what>(vtable|signal)):(?P<vtable>[\w\d_]+)\s*\*\*/\s*
        (?P<ret>(-|\w|\&|\*)+\s*)  # return type
        \s+                   # skip whitespace
        \(\s*\*\s*(?P<vfunc>\w+)\s*\)
        \s*[(]   # match the function name until the opening (
        \s*(?P<args>.*?)\s*[)]     # group the function arguments
        """, re.IGNORECASE|re.VERBOSE)
        proto_pat=re.compile(r"""
        (?P<ret>(-|\w|\&|\*)+\s*)  # return type
        \s+                   # skip whitespace
        (?P<func>\w+)\s*[(]   # match the function name until the opening (
        \s*(?P<args>.*?)\s*[)]     # group the function arguments
        """, re.IGNORECASE|re.VERBOSE)
        arg_split_pat = re.compile("\s*,\s*")

        buf = self.__clean_func(buf)
        buf = string.split(buf,'\n')

        for p in buf:
            if not p:
                continue

            if DEBUG:
                print 'matching line', repr(p)

            fname = None
            vfname = None
            is_signal = False
            m = proto_pat.match(p)
            if m is None:
                if DEBUG:
                    print 'proto_pat not matched'
                m = vproto_pat.match(p)
                if m is None:
                    if DEBUG:
                        print 'vproto_pat not matched'
                        if p.find('vtable:') >= 0:
                            print "oops", repr(p)
                        if p.find('moo_file_enc_new') >= 0:
                            print '***', repr(p)
                    continue
                else:
                    vfname = m.group('vfunc')
                    if m.group('what') == 'signal':
                        is_signal = True
                    if DEBUG:
                        print 'proto_pat matched', repr(m.group(0))
                        print '+++ ', ('vfunc', 'signal')[is_signal], vfname
            else:
                if DEBUG:
                    print 'proto_pat matched', repr(m.group(0))
                fname = m.group('func')
            ret = m.group('ret')
            if ret in ('return', 'else', 'if', 'switch'):
                continue
            if fname:
                func = self.__symbol_dict.get(fname)
                if func is None:
                    continue
                if DEBUG:
                    print 'match:|%s|' % fname
            elif not is_signal:
                symbol_name = 'vfunc:%s:%s' % (m.group('vtable'), m.group('vfunc'))
                func = self.__symbol_dict.get(symbol_name)
                if func is None:
                    func = VMethod(symbol_name, None, None, None, None, None)
                    self.__symbol_dict[symbol_name] = func
                    self.vmethods.append(func)
                if DEBUG:
                    print 'match:|%s|' % func.name
            else:
                symbol_name = ('signal:%s:%s' % (m.group('vtable'), m.group('vfunc'))).replace('_', '-')
                func = self.__symbol_dict.get(symbol_name)
                if func is None:
                    func = Signal(symbol_name, None, None, None, None, None)
                    self.__symbol_dict[symbol_name] = func
                    self.signals.append(func)
                if DEBUG:
                    print 'match:|%s|' % func.name

            args = m.group('args')
            args = arg_split_pat.split(args)
            for i in range(len(args)):
                spaces = string.count(args[i], ' ')
                if spaces > 1:
                    args[i] = string.replace(args[i], ' ', '-', spaces - 1).replace('gchar', 'char')

            if ret != 'void':
                ret = ret.replace('gchar', 'char')
                if func.retval is None:
                    func.retval = Retval()
                if func.retval.type is None:
                    func.retval.type = ret
                if ret in ('char*', 'strv', 'char**'):
                    func.retval.annotations.insert(0, 'transfer full')

            is_varargs = 0
            has_args = len(args) > 0
            for arg in args:
                if arg == '...':
                    is_varargs = 1
                elif arg in ('void', 'void '):
                    has_args = 0
            if DEBUG:
                print 'func ', ', '.join([p.name for p in func.params] if func.params else '')
            if has_args and not is_varargs:
                if func.params is None:
                    func.params = []
                elif func.params:
                    assert len(func.params) == len(args)
                for i in range(len(args)):
                    if DEBUG:
                        print 'arg:', args[i]
                    argtype, argname = string.split(args[i])
                    if DEBUG:
                        print argtype, argname
                    if len(func.params) <= i:
                        func.params.append(Param())
                    if func.params[i].name is None:
                        func.params[i].name = argname
                    if func.params[i].type is None:
                        func.params[i].type = argtype
            if DEBUG:
                print 'func ', ', '.join([p.name for p in func.params])

    def __read_declarations(self, filename):
        if DEBUG:
            print filename
        buf = open(filename).read()
        self.__read_declarations_in_buf(buf, filename)
