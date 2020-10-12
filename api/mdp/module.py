import sys
import re

import mdp.docparser as dparser

DEBUG = False

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

def get_class_method_c_name_prefix(cls):
    comps = split_camel_case_name(cls)
    return '_'.join([c.lower() for c in comps]) + '_'

def strip_class_prefix(name, cls):
    prefix = get_class_method_c_name_prefix(cls)
    if name.startswith(prefix):
        return name[len(prefix):]
    else:
        return name

def strip_module_prefix(name, mod):
    prefix = get_class_method_c_name_prefix(mod)
    if name.startswith(prefix):
        return name[len(prefix):]
    else:
        return name

def strip_module_prefix_from_class(name, mod):
    mod = mod.lower()
    mod = mod[0].upper() + mod[1:]
    if name.startswith(mod):
        return name[len(mod):]
    else:
        return name

def make_gtype_id(cls):
    comps = split_camel_case_name(cls)
    comps = [comps[0]] + ['TYPE'] + comps[1:]
    return '_'.join([c.upper() for c in comps])

class Type(object):
    def __init__(self, name):
        object.__init__(self)
        self.name = name

class BasicType(Type):
    def __init__(self, name):
        Type.__init__(self, name)

class _GTypedType(Type):
    def __init__(self, name, short_name, gtype_id, docs):
        Type.__init__(self, name)
        self.docs = docs
        self.methods = []
        self.static_methods = []
        self.gtype_id = gtype_id
        self.short_name = short_name
        self.annotations = {}

class EnumBase(_GTypedType):
    def __init__(self, name, short_name, gtype_id, docs):
        super(EnumBase, self).__init__(name, short_name, gtype_id, docs)
        self.values = []

class EnumValue(object):
    def __init__(self, name, attributes, docs):
        super(EnumValue, self).__init__()
        self.name = name
        self.attributes = attributes
        self.docs = docs

class Enum(EnumBase):
    def __init__(self, name, short_name, gtype_id, docs):
        super(Enum, self).__init__(name, short_name, gtype_id, docs)

class Flags(EnumBase):
    def __init__(self, name, short_name, gtype_id, docs):
        super(Flags, self).__init__(name, short_name, gtype_id, docs)

class _InstanceType(_GTypedType):
    def __init__(self, name, short_name, gtype_id, docs):
        _GTypedType.__init__(self, name, short_name, gtype_id, docs)
        self.constructor = None

class Class(_InstanceType):
    def __init__(self, name, short_name, parent, gtype_id, docs):
        _InstanceType.__init__(self, name, short_name, gtype_id, docs)
        self.parent = parent
        self.vmethods = []
        self.signals = []

class Boxed(_InstanceType):
    def __init__(self, name, short_name, gtype_id, docs):
        _InstanceType.__init__(self, name, short_name, gtype_id, docs)

class Pointer(_InstanceType):
    def __init__(self, name, short_name, gtype_id, docs):
        _InstanceType.__init__(self, name, short_name, gtype_id, docs)

class Symbol(object):
    def __init__(self, name, c_name, docs):
        object.__init__(self)
        self.name = name
        self.c_name = c_name
        self.docs = docs
        self.summary = None
        self.annotations = {}

class FunctionBase(Symbol):
    def __init__(self, name, c_name, params, retval, docs):
        Symbol.__init__(self, name, c_name, docs)
        self.params = params
        self.retval = retval

class Function(FunctionBase):
    def __init__(self, name, c_name, params, retval, docs):
        FunctionBase.__init__(self, name, c_name, params, retval, docs)

class Method(FunctionBase):
    def __init__(self, name, c_name, cls, params, retval, docs):
        FunctionBase.__init__(self, name, c_name, params, retval, docs)
        self.cls = cls

class StaticMethod(FunctionBase):
    def __init__(self, name, c_name, cls, params, retval, docs):
        FunctionBase.__init__(self, name, c_name, params, retval, docs)
        self.cls = cls

class Constructor(FunctionBase):
    def __init__(self, name, c_name, cls, params, retval, docs):
        FunctionBase.__init__(self, name, c_name, params, retval, docs)
        self.cls = cls

class VMethod(FunctionBase):
    def __init__(self, name, cls, params, retval, docs):
        FunctionBase.__init__(self, name, name, params, retval, docs)
        self.cls = cls

class Signal(FunctionBase):
    def __init__(self, name, cls, params, retval, docs):
        FunctionBase.__init__(self, name, name, params, retval, docs)
        self.cls = cls

class ParamBase(object):
    def __init__(self, typ, docs):
        object.__init__(self)
        self.type = typ
        self.docs = docs
        self.attributes = {}
        self.transfer_mode = None
        self.element_type = None
        self.array = False
        self.array_fixed_len = None
        self.array_len_param = None
        self.array_zero_terminated = None

class Param(ParamBase):
    def __init__(self, name, typ, docs):
        ParamBase.__init__(self, typ, docs)
        self.name = name
        self.out = False
        self.caller_allocates = False
        self.callee_allocates = False
        self.in_ = False
        self.inout = False
        self.allow_none = False
        self.default_value = None
        self.scope = 'call'

class Retval(ParamBase):
    def __init__(self, typ, docs):
        ParamBase.__init__(self, typ, docs)

class Module(object):
    def __init__(self, name):
        object.__init__(self)
        self.name = name
        self.classes = []
        self.boxed = []
        self.__class_dict = {}
        self.functions = []
        self.__methods = {}
        self.__constructors = {}
        self.__vmethods = {}
        self.__signals = {}
        self.types = {}
        self.enums = []

    def __add_class(self, pcls):
        name = pcls.name
        short_name = getattr(pcls, 'short_name', strip_module_prefix_from_class(pcls.name, self.name))
        gtype_id = getattr(pcls, 'gtype_id', make_gtype_id(pcls.name))
        docs = pcls.docs
        parent = None
        constructable = False
        annotations = {}
        for a in pcls.annotations:
            pieces = a.split()
            prefix = pieces[0]
            if prefix == 'parent':
                assert len(pieces) == 2
                parent = pieces[1]
            elif prefix == 'constructable':
                assert len(pieces) == 1
                constructable = True
            elif prefix.find('.') >= 0:
                annotations[prefix] = ' '.join(pieces[1:])
            else:
                raise RuntimeError("unknown annotation '%s' in class %s" % (a, name))
        cls = Class(name, short_name, parent, gtype_id, docs)
        cls.summary = pcls.summary
        cls.annotations = annotations
        cls.constructable = constructable
        self.classes.append(cls)
        self.__class_dict[name] = cls

    def __add_boxed_or_pointer(self, pcls, What):
        name = pcls.name
        short_name = getattr(pcls, 'short_name', strip_module_prefix_from_class(pcls.name, self.name))
        gtype_id = getattr(pcls, 'gtype_id', make_gtype_id(pcls.name))
        docs = pcls.docs
        annotations = {}
        for a in pcls.annotations:
            pieces = a.split()
            prefix = pieces[0]
            if prefix.find('.') >= 0:
                annotations[prefix] = ' '.join(pieces[1:])
            else:
                raise RuntimeError("unknown annotation '%s' in class %s" % (a, name))
        cls = What(name, short_name, gtype_id, docs)
        cls.summary = pcls.summary
        cls.annotations = annotations
        self.boxed.append(cls)
        self.__class_dict[name] = cls

    def __add_boxed(self, pcls):
        self.__add_boxed_or_pointer(pcls, Boxed)

    def __add_pointer(self, pcls):
        self.__add_boxed_or_pointer(pcls, Pointer)

    def __add_enum(self, ptyp):
        if DEBUG:
            print 'enum', ptyp.name
        name = ptyp.name
        short_name = getattr(ptyp, 'short_name', strip_module_prefix_from_class(ptyp.name, self.name))
        gtype_id = getattr(ptyp, 'gtype_id', make_gtype_id(ptyp.name))
        docs = ptyp.docs
        annotations = {}
        for a in ptyp.annotations:
            pieces = a.split()
            prefix = pieces[0]
            if prefix.find('.') >= 0:
                annotations[prefix] = ' '.join(pieces[1:])
            else:
                raise RuntimeError("unknown annotation '%s' in class %s" % (a, name))
        if isinstance(ptyp, dparser.Enum):
            enum = Enum(name, short_name, gtype_id, docs)
        else:
            enum = Flags(name, short_name, gtype_id, docs)
        for value in ptyp.values:
            attributes = self.__parse_enum_value_annotations(value.annotations)
            enum.values.append(EnumValue(value.name, attributes, value.docs))
        enum.summary = ptyp.summary
        enum.annotations = annotations
        self.enums.append(enum)

    def __parse_enum_value_annotations(self, annotations):
        attributes = {}
        for a in annotations:
            pieces = a.split()
            prefix = pieces[0]
            if '.' in prefix[1:-1] and len(pieces) == 2:
                attributes[prefix] = pieces[1]
        if attributes:
            return attributes
        else:
            return None

    def __parse_param_or_retval_annotation(self, annotation, param):
        pieces = annotation.split()
        prefix = pieces[0]
        if prefix == 'transfer':
            if len(pieces) > 2:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            if not pieces[1] in ('none', 'container', 'full'):
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.transfer_mode = pieces[1]
            return True
        if prefix == 'element-type':
            if len(pieces) > 3:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            if len(pieces) == 2:
                param.element_type = pieces[1]
            else:
                param.element_type = pieces[1:]
            return True
        if prefix == 'array':
            if len(pieces) == 1:
                param.array = True
                return True
            if len(pieces) > 2:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            m = re.match(r'fixed-size\s*=\s*(\d+)$', pieces[1])
            if m:
                param.array_fixed_size = int(m.group(1))
                return True
            m = re.match(r'length\s*=\s*(\S+)$', pieces[1])
            if m:
                param.array_len_param = m.group(1)
                return True
            m = re.match(r'zero-terminated\s*=\s*(\d+)$', pieces[1])
            if m:
                param.array_zero_terminated = bool(int(m.group(1)))
                return True
            raise RuntimeError("invalid annotation '%s'" % (a,))
        if prefix == 'type':
            if len(pieces) > 2:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.type = pieces[1]
            return True
        if '.' in prefix[1:-1] and len(pieces) == 2:
            param.attributes[prefix] = pieces[1]
        return False

    def __parse_param_annotation(self, annotation, param):
        pieces = annotation.split()
        prefix = pieces[0]
        if prefix == 'out':
            if len(pieces) > 2:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            if len(pieces) == 1:
                param.out = True
                return True
            if pieces[1] == 'caller-allocates':
                param.out = True
                param.caller_allocates = True
                return True
            if pieces[1] == 'callee-allocates':
                param.out = True
                param.callee_allocates = True
                return True
            raise RuntimeError("invalid annotation '%s'" % (a,))
        if prefix == 'in':
            if len(pieces) > 1:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.in_ = True
            return True
        if prefix == 'inout':
            if len(pieces) > 1:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.inout = True
            return True
        if prefix == 'allow-none':
            if len(pieces) > 1:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.allow_none = True
            return True
        if prefix == 'default':
            if len(pieces) != 2:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.default_value = pieces[1]
            return True
        if prefix == 'scope':
            if len(pieces) != 2:
                raise RuntimeError("invalid annotation '%s'" % (a,))
            if not pieces[1] in ('call', 'async', 'notified'):
                raise RuntimeError("invalid annotation '%s'" % (a,))
            param.scope = pieces[1]
            return True
        if '.' in prefix[1:-1] and len(pieces) == 2:
            param.attributes[prefix] = pieces[1]
        return False

    def __parse_retval(self, pretval):
        retval = Retval(pretval.type, pretval.docs)
        if pretval.annotations:
            for a in pretval.annotations:
                if not self.__parse_param_or_retval_annotation(a, retval):
                    raise RuntimeError("invalid annotation '%s'" % (a,))
        if not retval.type:
            raise RuntimeError('return type missing')
        return retval

    def __parse_param(self, pp, pfunc):
        if DEBUG:
            print pp.name, pp.type, pp.docs
        param = Param(pp.name, pp.type, pp.docs)
        if pp.annotations:
            for a in pp.annotations:
                if self.__parse_param_or_retval_annotation(a, param):
                    pass
                elif self.__parse_param_annotation(a, param):
                    pass
                else:
                    raise RuntimeError("in %s: invalid annotation '%s'" % (pfunc.name, a,))
        if param.type is None:
            raise RuntimeError('in %s: type of param "%s" is missing' % (pfunc.name, param.name))
        return param

    def __add_vmethod(self, pfunc):
        _, cls, name = pfunc.name.split(':')
        assert _ == 'vfunc'
        params = []
        retval = None
        docs = pfunc.docs
        if pfunc.annotations:
            for a in pfunc.annotations:
                raise RuntimeError("unknown annotation '%s' in function %s" % (a, name))

        if pfunc.params:
            for p in pfunc.params:
                params.append(self.__parse_param(p, pfunc))

        if pfunc.retval:
            retval = self.__parse_retval(pfunc.retval)

        meth = VMethod(name, cls, params[1:], retval, docs)
        this_class_methods = self.__vmethods.get(cls)
        if not this_class_methods:
            this_class_methods = []
            self.__vmethods[cls] = this_class_methods
        this_class_methods.append(meth)

    def __add_signal(self, pfunc):
        _, cls, name = pfunc.name.split(':')
        assert _ == 'signal'
        params = []
        retval = None
        docs = pfunc.docs
        if pfunc.annotations:
            for a in pfunc.annotations:
                raise RuntimeError("unknown annotation '%s' in function %s" % (a, name))

        if pfunc.params:
            for p in pfunc.params:
                params.append(self.__parse_param(p, pfunc))

        if pfunc.retval:
            retval = self.__parse_retval(pfunc.retval)

        meth = Signal(name, cls, params[1:], retval, docs)
        this_class_signals = self.__signals.get(cls)
        if not this_class_signals:
            this_class_signals = []
            self.__signals[cls] = this_class_signals
        this_class_signals.append(meth)

    def __add_function(self, pfunc):
        name = pfunc.name
        c_name = pfunc.name
        params = []
        retval = None
        docs = pfunc.docs
        cls = None
        constructor_of = None
        static_method_of = None
        kwargs = False
        annotations = {}

        if pfunc.annotations:
            for a in pfunc.annotations:
                pieces = a.split()
                prefix = pieces[0]
                if prefix == 'constructor-of':
                    assert len(pieces) == 2
                    constructor_of = pieces[1]
                elif prefix == 'static-method-of':
                    assert len(pieces) == 2
                    static_method_of = pieces[1]
                elif prefix == 'moo-kwargs':
                    assert len(pieces) == 1
                    kwargs = True
                elif prefix.find('.') >= 0:
                    annotations[prefix] = ' '.join(pieces[1:])
                else:
                    raise RuntimeError("unknown annotation '%s' in function %s" % (a, name))

        if pfunc.params:
            for p in pfunc.params:
                params.append(self.__parse_param(p, pfunc))

        if pfunc.retval:
            retval = self.__parse_retval(pfunc.retval)

        if hasattr(pfunc, 'method_of'):
            cls = pfunc.method_of
        elif static_method_of:
            cls = static_method_of
        elif constructor_of:
            cls = constructor_of
        elif params:
            m = re.match(r'(const-)?([\w\d_]+)\*', params[0].type)
            if m:
                cls = m.group(2)
                if not self.__class_dict.has_key(cls):
                    cls = None

        if cls:
            name = strip_class_prefix(name, cls)
        else:
            name = strip_module_prefix(name, self.name)

        if constructor_of:
            func = Constructor(name, c_name, cls, params, retval, docs)
            if constructor_of in self.__constructors:
                raise RuntimeError('duplicated constructor of class %s' % constructor_of)
            self.__constructors[constructor_of] = func
        elif cls:
            if static_method_of:
                func = StaticMethod(name, c_name, cls, params, retval, docs)
            else:
                func = Method(name, c_name, cls, params[1:], retval, docs)
            this_class_methods = self.__methods.get(cls)
            if not this_class_methods:
                this_class_methods = []
                self.__methods[cls] = this_class_methods
            this_class_methods.append(func)
        else:
            func = Function(name, c_name, params, retval, docs)
            self.functions.append(func)

        func.summary = pfunc.summary
        func.annotations = annotations
        func.kwargs = kwargs

    def init_from_dox(self, blocks):
        for b in blocks:
            if isinstance(b, dparser.Class):
                self.__add_class(b)
            elif isinstance(b, dparser.Boxed):
                self.__add_boxed(b)
            elif isinstance(b, dparser.Pointer):
                self.__add_pointer(b)
            elif isinstance(b, dparser.Enum) or isinstance(b, dparser.Flags):
                self.__add_enum(b)
            elif isinstance(b, dparser.Flags):
                self.__add_flags(b)
            elif isinstance(b, dparser.VMethod):
                self.__add_vmethod(b)
            elif isinstance(b, dparser.Signal):
                self.__add_signal(b)
            elif isinstance(b, dparser.Function):
                self.__add_function(b)
            else:
                raise RuntimeError('oops')

        instance_types = {}
        for cls in self.classes + self.boxed:
            if cls.name in instance_types:
                raise RuntimeError('duplicated class %s' % (cls.name,))
            instance_types[cls.name] = cls

        for cls in self.__constructors:
            func = self.__constructors[cls]
            if not cls in instance_types:
                raise RuntimeError('Constructor of unknown class %s' % cls)
            else:
                cls = instance_types[cls]
                if cls.constructor is not None:
                    raise RuntimeError('duplicated constructor in class %s' % cls)
                cls.constructor = func

        for cls in self.__methods:
            methods = self.__methods[cls]
            if not cls in instance_types:
                raise RuntimeError('Methods of unknown class %s' % cls)
            else:
                cls = instance_types[cls]
                for m in methods:
                    m.cls = cls
                    if isinstance(m, Method):
                        cls.methods.append(m)
                    elif isinstance(m, StaticMethod):
                        cls.static_methods.append(m)
                    else:
                        oops()

        for cls in self.__vmethods:
            methods = self.__vmethods[cls]
            if not cls in instance_types:
                raise RuntimeError('Virtual methods of unknown class %s' % cls)
            else:
                cls = instance_types[cls]
                for m in methods:
                    m.cls = cls
                cls.vmethods += methods

        for cls in self.__signals:
            signals = self.__signals[cls]
            if not cls in instance_types:
                raise RuntimeError('Signals of unknown class %s' % cls)
            else:
                cls = instance_types[cls]
                for s in signals:
                    s.cls = cls
                cls.signals += signals

        def format_func(func):
            if func.retval and func.retval.type:
                s = func.retval.type + ' '
            else:
                s = 'void '
            s += func.name
            s += ' ('
            for i in range(len(func.params)):
                if i != 0:
                    s += ', '
                p = func.params[i]
                s += '%s %s' % (p.type, p.name)
            s += ')'
            return s

        if DEBUG:
            for cls in self.classes:
                print 'class %s' % (cls.name,)
                for meth in cls.methods:
                    print '  %s' % (format_func(meth),)
            for func in self.functions:
                print format_func(func)
