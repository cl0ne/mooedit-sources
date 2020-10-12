import StringIO

from mpi.util import *
from mpi.module import *

lua_constants = {
    'NULL': '<constant>nil</constant>',
    'TRUE': '<constant>true</constant>',
    'FALSE': '<constant>false</constant>',
    'INDEXBASE': '<constant>1</constant>',
}

python_constants = {
    'NULL': '<constant>None</constant>',
    'TRUE': '<constant>True</constant>',
    'FALSE': '<constant>False</constant>',
    'GTK_RESPONSE_OK': '<constant><ulink url="http://library.gnome.org/devel/pygtk/stable/' +
                       'gtk-constants.html#gtk-response-type-constants">gtk.RESPONSE_OK</ulink></constant>',
    'INDEXBASE': '<constant>0</constant>',
}

common_types = {
    'gboolean': '<constant>bool</constant>',
    'strv': 'list of strings',
}

lua_types = dict(common_types)
lua_types.update({
    'index': '<constant>integer index (1-based)</constant>',
    'value': '<constant>value</constant>',
    'gunichar': '<constant>string</constant>',
    'double': '<constant>number</constant>',
    'int': '<constant>integer</constant>',
    'gint': '<constant>integer</constant>',
    'guint': '<constant>integer</constant>',
    'gulong': '<constant>integer</constant>',
    'const-utf8': '<constant>string</constant>',
    'utf8': '<constant>string</constant>',
    'const-filename': '<constant>string</constant>',
    'filename': '<constant>string</constant>',
    'const-cstring': '<constant>string</constant>',
    'cstring': '<constant>string</constant>',
})

python_types = dict(common_types)
python_types.update({
    'index': '<constant>integer index (0-based)</constant>',
    'gunichar': '<constant>str</constant>',
    'double': '<constant>float</constant>',
    'int': '<constant>int</constant>',
    'gint': '<constant>int</constant>',
    'guint': '<constant>int</constant>',
    'gulong': '<constant>int</constant>',
    'const-utf8': '<constant>str</constant>',
    'utf8': '<constant>str</constant>',
    'const-filename': '<constant>str</constant>',
    'filename': '<constant>str</constant>',
    'const-cstring': '<constant>str</constant>',
    'cstring': '<constant>str</constant>',
})

def format_python_symbol_ref(symbol):
    constants = {
    }
    classes = {
        'GFile': '<ulink url="http://library.gnome.org/devel/pygobject/stable/class-giofile.html">gio.File</ulink>',
        'GObject': '<ulink url="http://library.gnome.org/devel/pygobject/stable/class-gobject.html">gobject.Object</ulink>',
        'GType': 'type',
    }
    if symbol in constants:
        return '<constant>%s</constant>' % constants[symbol]
    if symbol in classes:
        return '<constant>%s</constant>' % classes[symbol]
    if symbol.startswith('Gtk'):
        return ('<constant><ulink url="http://library.gnome.org/devel/pygtk/stable/class-' + \
                'gtk%s.html">gtk.%s</ulink></constant>') % (symbol[3:].lower(), symbol[3:])
    if symbol.startswith('Gdk'):
        return '<constant><ulink url="http://library.gnome.org/devel/pygtk/stable/class-' + \
                'gdk%s.html">gtk.gdk.%s</ulink></constant>' % (symbol[3:].lower(), symbol[3:])
    raise NotImplementedError(symbol)

def split_camel_case_name(name):
    comps = []
    cur = ''
    for c in name:
        if c.islower() or not cur:
            cur += c
        else:
            comps.append(cur)
            cur = c
    if cur:
        comps.append(cur)
    return comps

def name_all_caps(cls):
    return '_'.join([s.upper() for s in split_camel_case_name(cls.name)])

class Writer(object):
    def __init__(self, mode, template, out):
        super(Writer, self).__init__()
        self.file = out
        self.out = StringIO.StringIO()
        self.mode = mode
        self.template = template
        if mode == 'python':
            self.constants = python_constants
            self.builtin_types = python_types
        elif mode == 'lua':
            self.constants = lua_constants
            self.builtin_types = lua_types
        else:
            oops('unknown mode %s' % mode)

        self.section_suffix = ' (%s)' % self.mode.capitalize()

    def __format_symbol_ref(self, name):
        sym = self.symbols.get(name)
        if sym:
            if isinstance(sym, EnumValue):
                return '<constant><link linkend="%(mode)s.%(parent)s" endterm="%(mode)s.%(symbol)s.title"></link></constant>' % \
                    dict(symbol=name, mode=self.mode, parent=sym.enum.symbol_id())
            elif isinstance(sym, Type):
                return '<constant><link linkend="%(mode)s.%(symbol)s">%(name)s</link></constant>' % \
                    dict(symbol=name, mode=self.mode, name=self.__make_class_name(sym))
            elif isinstance(sym, Method):
                return '<constant><link linkend="%(mode)s.%(symbol)s">%(Class)s.%(method)s()</link></constant>' % \
                    dict(symbol=name, mode=self.mode, Class=self.__make_class_name(sym.cls), method=sym.name)
            else:
                oops(name)
        if self.mode == 'python':
            return format_python_symbol_ref(name)
        else:
            raise NotImplementedError(name)

    def __string_to_bool(self, s):
        if s == '0':
            return False
        elif s == '1':
            return True
        else:
            oops()

    def __check_bind_ann(self, obj):
        bind = self.__string_to_bool(obj.annotations.get('moo.' + self.mode, '1'))
        if bind:
            bind = not self.__string_to_bool(obj.annotations.get('moo.private', '0'))
        return bind

    def __format_constant(self, value):
        if value in self.constants:
            return self.constants[value]
        try:
            i = int(value)
            return value
        except ValueError:
            pass
        formatted = self.__format_symbol_ref(value)
        if formatted:
            return formatted
        error("unknown constant '%s'" % value)
        return value

    def __format_default_value(self, p):
        if p.type.name == 'index' and self.mode == 'lua':
            return str(int(p.default_value) + 1)
        else:
            return self.__format_constant(p.default_value)

    def __format_doc(self, doc):
        text = doc.text
        text = re.sub(r'@([\w\d_]+)(?!\{)', r'<parameter>\1</parameter>', text)
        text = re.sub(r'%NULL\b', '<constant>%s</constant>' % self.constants['NULL'], text)
        text = re.sub(r'%TRUE\b', '<constant>%s</constant>' % self.constants['TRUE'], text)
        text = re.sub(r'%FALSE\b', '<constant>%s</constant>' % self.constants['FALSE'], text)
        text = re.sub(r'%INDEXBASE\b', '<constant>%s</constant>' % self.constants['INDEXBASE'], text)
        text = text.replace(r'<n/>', '')
        text = text.replace(r'<nl/>', '\n')

        def repl_func(m):
            func_id = m.group(1)
            symbol = self.symbols.get(func_id)
            if not isinstance(symbol, MethodBase) or symbol.cls == self.current_class:
                return '<function><link linkend="%(mode)s.%(func_id)s" endterm="%(mode)s.%(func_id)s.title"/></function>' % \
                    dict(func_id=func_id, mode=self.mode)
            else:
                return '<function><link linkend="%(mode)s.%(func_id)s">%(Class)s.%(method)s()</link></function>' % \
                    dict(func_id=func_id, mode=self.mode, Class=self.__make_class_name(symbol.cls), method=symbol.name)
        text = re.sub(r'([\w\d_.]+)\(\)', repl_func, text)

        def repl_signal(m):
            cls = m.group(1)
            signal = m.group(2).replace('_', '-')
            symbol = 'signal:%s:%s' % (cls, signal)
            if self.symbols[cls] != self.current_class:
                return '<function><link linkend="%(mode)s.%(symbol)s">%(Class)s.%(signal)s</link></function>' % \
                    dict(Class=cls, symbol=symbol, mode=self.mode, signal=signal)
            else:
                return '<function><link linkend="%(mode)s.%(symbol)s">%(signal)s</link></function>' % \
                    dict(symbol=symbol, mode=self.mode, signal=signal)
        text = re.sub(r'([\w\d_-]+)::([\w\d_-]+)', repl_signal, text)

        def repl_symbol(m):
            symbol = m.group(1)
            formatted = self.__format_symbol_ref(symbol)
            if not formatted:
                raise RuntimeError('unknown symbol %s' % symbol)
            return formatted
        text = re.sub(r'#([\w\d_]+)', repl_symbol, text)

        assert not re.search(r'NULL|TRUE|FALSE', text)
        return text

    def __make_class_name(self, cls):
        return '%s.%s' % (cls.module.name.lower(), cls.short_name)

    def __get_obj_name(self, cls):
        name = cls.annotations.get('moo.doc-object-name')
        if not name:
            name = '_'.join([c.lower() for c in split_camel_case_name(cls.name)[1:]])
        return name

    def __write_function(self, func, cls):
        if not self.__check_bind_ann(func):
            return

        func_params = list(func.params)
        if func.has_gerror_return:
            func_params = func_params[:-1]
        params = []
        for p in func_params:
            if not self.__check_bind_ann(p.type):
                return
            if p.default_value is not None:
                params.append('%s=%s' % (p.name, self.__format_default_value(p)))
            else:
                params.append(p.name)

        if isinstance(func, Constructor):
            if self.mode == 'python':
                func_title = cls.short_name + '()'
                func_name = cls.short_name
            elif self.mode == 'lua':
                func_title = 'new' + '()'
                func_name = '%s.new' % cls.short_name
            else:
                oops()
        elif isinstance(func, Signal):
            if self.mode == 'python':
                func_title = 'signal ' + func.name
                func_name = func.name
            elif self.mode == 'lua':
                func_title = 'signal ' + func.name
                func_name = func.name
            else:
                oops()
        elif cls is not None:
            if self.mode in ('python', 'lua'):
                func_title = func.name + '()'
                func_name = '%s.%s' % (self.__get_obj_name(cls), func.name) \
                                if not isinstance(func, StaticMethod) \
                                else '%s.%s' % (cls.short_name, func.name)
            else:
                oops()
        else:
            func_title = func.name + '()'
            func_name = '%s.%s' % (self.module.name.lower(), func.name)

        params_string = ', '.join(params)

        if func.summary:
            oops(func_id)

        func_id = func.symbol_id()
        mode = self.mode

        if mode == 'lua' and func.kwargs:
            left_paren, right_paren = '{}'
        else:
            left_paren, right_paren = '()'

        self.out.write("""\
<!-- %(func_id)s -->
<sect2 id="%(mode)s.%(func_id)s">
<title id="%(mode)s.%(func_id)s.title">%(func_title)s</title>
<programlisting>%(func_name)s%(left_paren)s%(params_string)s%(right_paren)s</programlisting>
""" % locals())

        if func.doc:
            self.out.write('<para>%s</para>\n' % self.__format_doc(func.doc))

        if func_params:
            self.out.write("""\
<variablelist>
<?dbhtml list-presentation="table"?>
<?dbhtml term-separator=" : "?>
""")

            for p in func_params:
                if p.doc:
                    docs = doc=self.__format_doc(p.doc)
                else:
                    ptype = p.type.symbol_id()
                    if ptype.endswith('*'):
                        ptype = ptype[:-1]
                    if ptype.startswith('const-'):
                        ptype = ptype[len('const-'):]
                    if ptype in self.builtin_types:
                        docs = self.builtin_types[ptype]
                        if p.allow_none:
                            docs = docs + ' or ' + self.__format_constant('NULL')
                    elif ptype.endswith('Array'):
                        elmtype = ptype[:-len('Array')]
                        if elmtype in self.symbols and isinstance(self.symbols[elmtype], InstanceType):
                            docs = 'list of %s objects' % self.__format_symbol_ref(self.symbols[elmtype].symbol_id())
                        else:
                            docs = self.__format_symbol_ref(ptype)
                    else:
                        docs = self.__format_symbol_ref(ptype)
                        if p.allow_none:
                            docs = docs + ' or ' + self.__format_constant('NULL')

                param_dic = dict(param=p.name, doc=docs)
                self.out.write("""\
<varlistentry>
<term><parameter>%(param)s</parameter></term>
<listitem><para>%(doc)s</para></listitem>
</varlistentry>
""" % param_dic)

            self.out.write('</variablelist>\n')

        if func.retval:
            retdoc = None
            if func.retval.doc:
                retdoc = self.__format_doc(func.retval.doc)
            else:
                rettype = func.retval.type.symbol_id()
                if rettype.endswith('*'):
                    rettype = rettype[:-1]
                if rettype in self.builtin_types:
                    retdoc = self.builtin_types[rettype]
                elif rettype.endswith('Array'):
                    elmtype = rettype[:-len('Array')]
                    if elmtype in self.symbols and isinstance(self.symbols[elmtype], InstanceType):
                        retdoc = 'list of %s objects' % self.__format_symbol_ref(self.symbols[elmtype].symbol_id())
                    else:
                        retdoc = self.__format_symbol_ref(rettype)
                else:
                    retdoc = self.__format_symbol_ref(rettype)
            if retdoc:
                self.out.write('<para><parameter>Returns:</parameter> %s</para>\n' % retdoc)

        self.out.write('</sect2>\n')

    def __write_gobject_constructor(self, cls):
        func = Constructor()
        self.__write_function(func, cls)

    def __write_class(self, cls):
        if not self.__check_bind_ann(cls):
            return

        self.current_class = cls

        title = self.__make_class_name(cls)
        if cls.summary:
            title += ' - ' + cls.summary.text
        dic = dict(class_id=cls.symbol_id(), title=title, mode=self.mode)
        self.out.write("""\
<!-- %(class_id)s -->
<sect1 id="%(mode)s.%(class_id)s">
<title id="%(mode)s.%(class_id)s.title">%(title)s</title>
""" % dic)

        if cls.doc:
            self.out.write('<para>%s</para>\n' % self.__format_doc(cls.doc))

        if getattr(cls, 'parent', 'none') != 'none':
            self.out.write("""\
<programlisting>
%(ParentClass)s
  |
  +-- %(Class)s
</programlisting>
""" % dict(ParentClass=self.__format_symbol_ref(cls.parent), Class=self.__make_class_name(cls)))

        if hasattr(cls, 'signals') and cls.signals:
            for signal in cls.signals:
                self.__write_function(signal, cls)
        if cls.constructor is not None:
            self.__write_function(cls.constructor, cls)
        if hasattr(cls, 'constructable') and cls.constructable:
            self.__write_gobject_constructor(cls)
        for meth in cls.static_methods:
            self.__write_function(meth, cls)
        if isinstance(cls, Class):
            if cls.vmethods:
                implement_me('virtual methods of %s' % cls.name)
        for meth in cls.methods:
            self.__write_function(meth, cls)

        self.out.write("""\
</sect1>
""" % dic)

        self.current_class = None

    def __write_enum(self, enum):
        if not self.__check_bind_ann(enum):
            return

        do_write = False
        for v in enum.values:
            if self.__check_bind_ann(v):
                do_write = True
                break

        if not do_write:
            return

        title = self.__make_class_name(enum)
        if enum.summary:
            title += ' - ' + enum.summary.text
        dic = dict(title=title,
                   mode=self.mode,
                   enum_id=enum.symbol_id())

        self.out.write("""\
<!-- %(enum_id)s -->
<sect2 id="%(mode)s.%(enum_id)s">
<title id="%(mode)s.%(enum_id)s.title">%(title)s</title>
""" % dic)

        if enum.doc:
            self.out.write('<para>%s</para>\n' % self.__format_doc(enum.doc))

        self.out.write("""\
<variablelist>
<?dbhtml list-presentation="table"?>
""")

        for v in enum.values:
            if not self.__check_bind_ann(v):
                continue
            value_dic = dict(mode=self.mode, enum_id=v.name, value=v.short_name,
                             doc=self.__format_doc(v.doc) if v.doc else '')
            self.out.write("""\
<varlistentry>
 <term><constant id="%(mode)s.%(enum_id)s.title">%(value)s</constant></term>
 <listitem><para>%(doc)s</para></listitem>
</varlistentry>
""" % value_dic)

        self.out.write('</variablelist>\n')

        self.out.write('</sect2>\n')

    def write(self, module):
        self.module = module
        self.symbols = module.get_symbols()
        self.current_class = None

        classes = module.get_classes() + module.get_boxed() + module.get_pointers()
        for cls in sorted(classes, lambda cls1, cls2: cmp(cls1.short_name, cls2.short_name)):
            self.__write_class(cls)

        funcs = []
        for f in module.get_functions():
            if self.__check_bind_ann(f):
                funcs.append(f)

        dic = dict(mode=self.mode)

        if funcs:
            self.out.write("""\
<!-- functions -->
<sect1 id="%(mode)s.functions">
<title>Functions</title>
""" % dic)

            for func in funcs:
                self.__write_function(func, None)

            self.out.write('</sect1>\n')

        self.out.write("""\
<!-- enums -->
<sect1 id="%(mode)s.enums">
<title>Enumerations</title>
""" % dic)

        for func in module.get_enums():
            self.__write_enum(func)

        self.out.write('</sect1>\n')

        content = self.out.getvalue()
        template = open(self.template).read()
        self.file.write(template.replace('###GENERATED###', content))

        del self.out
        del self.module
