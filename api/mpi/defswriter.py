from mpi.module import *

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

class_template = """\
(define-object %(short_name)s
  (in-module "%(module)s")
  (parent "%(parent)s")
  (c-name "%(name)s")
  (gtype-id "%(gtype_id)s")
)
"""

method_start_template = """\
(define-method %(name)s
  (of-object "%(class)s")
  (c-name "%(c_name)s")
  (return-type "%(return_type)s")
"""

vmethod_start_template = """\
(define-virtual %(name)s
  (of-object "%(class)s")
  (return-type "%(return_type)s")
"""

function_start_template = """\
(define-function %(name)s
  (c-name "%(c_name)s")
  (return-type "%(return_type)s")
"""

type_template = """\
(define-%(what)s %(short_name)s
  (in-module "%(module)s")
  (c-name "%(name)s")
  (gtype-id "%(gtype_id)s")
)
"""

class Writer(object):
    def __init__(self, out):
        object.__init__(self)
        self.out = out
        self.module = None

    def __write_class_decl(self, cls):
        dic = dict(name=cls.name,
                   short_name=cls.short_name,
                   module=self.module.name,
                   parent=cls.parent,
                   gtype_id=cls.gtype_id)
        self.out.write(class_template % dic)
        self.out.write('\n')

    def __write_boxed_decl(self, cls):
        dic = dict(name=cls.name,
                   short_name=cls.short_name,
                   module=self.module.name,
                   gtype_id=cls.gtype_id,
                   what='boxed')
        self.out.write(type_template % dic)
        self.out.write('\n')

    def __write_pointer_decl(self, cls):
        dic = dict(name=cls.name,
                   short_name=cls.short_name,
                   module=self.module.name,
                   gtype_id=cls.gtype_id,
                   what='pointer')
        self.out.write(type_template % dic)
        self.out.write('\n')

    def __write_enum_decl(self, cls):
        dic = dict(name=cls.name,
                   short_name=cls.short_name,
                   module=self.module.name,
                   gtype_id=cls.gtype_id,
                   what='enum' if isinstance(cls, Enum) else 'flags')
        self.out.write(type_template % dic)
        self.out.write('\n')

    def __get_pygtk_type_name(self, typ):
        if isinstance(typ, InstanceType):
            return typ.name + '*'
        elif typ.name in ('utf8', 'filename', 'cstring'):
            return 'char*'
        elif typ.name in ('const-utf8', 'const-filename', 'const-cstring'):
            return 'const-char*'
        else:
            return typ.name

    def __write_function_or_method(self, meth, cls):
        if meth.retval is None:
            return_type = 'none'
        else:
            return_type = self.__get_pygtk_type_name(meth.retval.type)
        dic = dict(name=meth.name, c_name=meth.c_name, return_type=return_type)
        if not cls:
            self.out.write(function_start_template % dic)
        elif isinstance(meth, Constructor):
            dic['class'] = cls.name
            self.out.write(function_start_template % dic)
            self.out.write('  (is-constructor-of %s)\n' % cls.name)
        elif isinstance(meth, StaticMethod):
            dic['class'] = cls.name
            self.out.write(method_start_template % dic)
            self.out.write('  (is-static-method #t)\n')
        elif isinstance(meth, VMethod):
            dic['class'] = cls.name
            self.out.write(vmethod_start_template % dic)
        else:
            dic['class'] = cls.name
            self.out.write(method_start_template % dic)
        if meth.retval:
            if meth.retval.transfer_mode == 'full':
                self.out.write('  (caller-owns-return #t)\n')
            elif meth.retval.transfer_mode is not None:
                raise RuntimeError('do not know how to handle transfer mode %s' % (meth.retval.transfer_mode,))
        if meth.params:
            self.out.write('  (parameters\n')
            for p in meth.params:
                self.out.write('    \'("%s" "%s"' % (self.__get_pygtk_type_name(p.type), p.name))
                if p.allow_none:
                    self.out.write(' (null-ok)')
                if p.default_value is not None:
                    self.out.write(' (default "%s")' % (p.default_value,))
                self.out.write(')\n')
            self.out.write('  )\n')
        self.out.write(')\n\n')

    def __write_class_method(self, meth, cls):
        self.__write_function_or_method(meth, cls)

    def __write_static_class_method(self, meth, cls):
        self.__write_function_or_method(meth, cls)

    def __write_class_methods(self, cls):
        self.out.write('; methods of %s\n\n' % cls.name)
        if cls.constructor is not None:
            self.__write_function_or_method(cls.constructor, cls)
        if hasattr(cls, 'constructable') and cls.constructable:
            cons = Constructor()
            cons.name = '%s__new__' % cls.name
            cons.c_name = cons.name
            self.__write_function_or_method(cons, cls)
        if isinstance(cls, Class):
            for meth in cls.vmethods:
                self.__write_class_method(meth, cls)
        for meth in cls.methods:
            self.__write_class_method(meth, cls)
        for meth in cls.static_methods:
            self.__write_static_class_method(meth, cls)

    def __write_function(self, func):
        self.__write_function_or_method(func, None)

    def write(self, module):
        self.module = module
        self.out.write('; -*- scheme -*-\n\n')
        self.out.write('; classes\n\n')
        for cls in module.get_classes():
            self.__write_class_decl(cls)
        self.out.write('; boxed types\n\n')
        for cls in module.get_boxed():
            self.__write_boxed_decl(cls)
        self.out.write('; pointer types\n\n')
        for cls in module.get_pointers():
            self.__write_pointer_decl(cls)
        self.out.write('; enums and flags\n\n')
        for enum in module.get_enums():
            self.__write_enum_decl(enum)
        for cls in module.get_classes():
            self.__write_class_methods(cls)
        for cls in module.get_boxed():
            self.__write_class_methods(cls)
        for cls in module.get_pointers():
            self.__write_class_methods(cls)
        self.out.write('; functions\n\n')
        for func in module.get_functions():
            self.__write_function(func)
        self.module = None
