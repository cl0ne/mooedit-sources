import re
import sys
import xml.etree.ElementTree as _etree

class Doc(object):
    def __init__(self, text):
        object.__init__(self)
        self.text = text

    @staticmethod
    def from_xml(elm):
        return Doc(elm.text)

def _set_unique_attribute(obj, attr, value):
    if getattr(obj, attr) is not None:
        raise RuntimeError("duplicated attribute '%s'" % (attr,))
    setattr(obj, attr, value)

def _set_unique_attribute_bool(obj, attr, value):
    if value.lower() in ('0', 'false', 'no'):
        value = False
    elif value.lower() in ('1', 'true', 'yes'):
        value = True
    else:
        raise RuntimeError("bad value '%s' for boolean attribute '%s'" % (value, attr))
    _set_unique_attribute(obj, attr, value)

class _XmlObject(object):
    def __init__(self):
        object.__init__(self)
        self.doc = None
        self.summary = None
        self.annotations = {}
        self.module = None

    @classmethod
    def from_xml(cls, module, elm, *args):
        obj = cls()
        obj.module = module
        obj._parse_xml(module, elm, *args)
        return obj

    def _parse_xml_element(self, module, elm):
        if elm.tag in ('doc', 'summary'):
            _set_unique_attribute(self, elm.tag, Doc.from_xml(elm))
        else:
            raise RuntimeError('unknown element %s' % (elm.tag,))

    def _parse_attribute(self, attr, value):
        if attr.find('.') >= 0:
            self.annotations[attr] = value
            return True
        else:
            return False

    def _parse_xml(self, module, elm, *args):
        for attr, value in elm.items():
            if not self._parse_attribute(attr, value):
                raise RuntimeError('unknown attribute %s' % (attr,))
        for child in elm.getchildren():
            self._parse_xml_element(module, child)

class _ParamBase(_XmlObject):
    def __init__(self):
        _XmlObject.__init__(self)
        self.type = None
        self.transfer_mode = None

    def _parse_attribute(self, attr, value):
        if attr in ('type', 'transfer_mode'):
            _set_unique_attribute(self, attr, value)
        else:
            return _XmlObject._parse_attribute(self, attr, value)
        return True

class Param(_ParamBase):
    def __init__(self):
        _ParamBase.__init__(self)
        self.name = None
        self.default_value = None
        self.allow_none = None

    def _parse_attribute(self, attr, value):
        if attr in ('default_value', 'name'):
            _set_unique_attribute(self, attr, value)
        elif attr in ('allow_none',):
            _set_unique_attribute_bool(self, attr, value)
        else:
            return _ParamBase._parse_attribute(self, attr, value)
        return True

class Retval(_ParamBase):
    def __init__(self):
        _ParamBase.__init__(self)

class FunctionBase(_XmlObject):
    def __init__(self):
        _XmlObject.__init__(self)
        self.name = None
        self.c_name = None
        self.retval = None
        self.params = []
        self.has_gerror_return = False
        self.kwargs = None

    def _parse_attribute(self, attr, value):
        if attr in ('c_name', 'name'):
            _set_unique_attribute(self, attr, value)
        elif attr in ('kwargs',):
            _set_unique_attribute_bool(self, attr, value)
        else:
            return _XmlObject._parse_attribute(self, attr, value)
        return True

    def _parse_xml_element(self, module, elm):
        if elm.tag == 'retval':
            _set_unique_attribute(self, 'retval', Retval.from_xml(module, elm))
        elif elm.tag == 'param':
            self.params.append(Param.from_xml(module, elm))
        else:
            _XmlObject._parse_xml_element(self, module, elm)

    def _parse_xml(self, module, elm, *args):
        _XmlObject._parse_xml(self, module, elm, *args)
        if not self.name:
            raise RuntimeError('function name missing')
        if not self.c_name:
            raise RuntimeError('function c_name missing')

class Function(FunctionBase):
    def __init__(self):
        FunctionBase.__init__(self)

    def symbol_id(self):
        return self.c_name

class MethodBase(FunctionBase):
    def __init__(self):
        super(MethodBase, self).__init__()

    def _parse_xml(self, module, elm, cls):
        super(MethodBase, self)._parse_xml(module, elm, cls)
        self.cls = cls

class Constructor(MethodBase):
    def __init__(self):
        super(Constructor, self).__init__()

    def symbol_id(self):
        return self.c_name

class StaticMethod(MethodBase):
    def __init__(self):
        super(StaticMethod, self).__init__()

    def symbol_id(self):
        return self.c_name

class Method(MethodBase):
    def __init__(self):
        super(Method, self).__init__()

    def symbol_id(self):
        return self.c_name

class VMethod(MethodBase):
    def __init__(self):
        super(VMethod, self).__init__()
        self.c_name = "fake"

    def symbol_id(self):
        return '%s::%s' % (self.cls.name, self.name)

class Signal(MethodBase):
    def __init__(self):
        super(Signal, self).__init__()
        self.c_name = "fake"

    def symbol_id(self):
        return 'signal:%s:%s' % (self.cls.name, self.name)

class Type(_XmlObject):
    def __init__(self):
        _XmlObject.__init__(self)
        self.name = None
        self.c_name = None
        self.gtype_id = None

    def symbol_id(self):
        return self.name

class BasicType(Type):
    def __init__(self, name):
        Type.__init__(self)
        self.name = name

class ArrayType(Type):
    def __init__(self, elm_type):
        Type.__init__(self)
        self.elm_type = elm_type
        self.name = '%sArray*' % elm_type.name
        self.c_name = '%sArray*' % elm_type.name

class GErrorReturnType(Type):
    def __init__(self):
        Type.__init__(self)
        self.name = 'GError**'

class GTypedType(Type):
    def __init__(self):
        Type.__init__(self)
        self.short_name = None

    def _parse_attribute(self, attr, value):
        if attr in ('short_name', 'name', 'gtype_id'):
            _set_unique_attribute(self, attr, value)
        else:
            return Type._parse_attribute(self, attr, value)
        return True

    def _parse_xml(self, module, elm, *args):
        Type._parse_xml(self, module, elm, *args)
        if self.name is None:
            raise RuntimeError('class name missing')
        if self.short_name is None:
            raise RuntimeError('class short name missing')
        if self.gtype_id is None:
            raise RuntimeError('class gtype missing')

class EnumValue(_XmlObject):
    def __init__(self):
        super(EnumValue, self).__init__()
        self.name = None
        self.short_name = None
        self.enum = None

    def symbol_id(self):
        return self.name

    def _parse_attribute(self, attr, value):
        if attr in ('name'):
            _set_unique_attribute(self, attr, value)
        else:
            return super(EnumValue, self)._parse_attribute(attr, value)
        return True

    def _parse_xml(self, module, elm, *args):
        super(EnumValue, self)._parse_xml(module, elm, *args)
        if self.name is None:
            raise RuntimeError('enum value name missing')
        if not self.short_name:
            self.short_name = self.name
            prefix = module.name.upper() + '_'
            if self.short_name.startswith(prefix):
                self.short_name = self.short_name[len(prefix):]

class EnumBase(GTypedType):
    def __init__(self):
        super(EnumBase, self).__init__()
        self.values = []
        self.__value_hash = {}

    def _parse_xml_element(self, module, elm):
        if elm.tag == 'value':
            value = EnumValue.from_xml(module, elm)
            assert not value.name in self.__value_hash
            self.__value_hash[value.name] = value
            value.enum = self
            self.values.append(value)
        else:
            super(EnumBase, self)._parse_xml_element(module, elm)

class Enum(EnumBase):
    def __init__(self):
        super(Enum, self).__init__()

class Flags(EnumBase):
    def __init__(self):
        super(Flags, self).__init__()

class InstanceType(GTypedType):
    def __init__(self):
        GTypedType.__init__(self)
        self.constructor = None
        self.methods = []
        self.static_methods = []
        self.__method_hash = {}

    def _parse_xml_element(self, module, elm):
        if elm.tag == 'method':
            meth = Method.from_xml(module, elm, self)
            assert not meth.name in self.__method_hash
            self.__method_hash[meth.name] = meth
            self.methods.append(meth)
        elif elm.tag == 'static-method':
            meth = StaticMethod.from_xml(module, elm, self)
            assert not meth.name in self.__method_hash
            self.__method_hash[meth.name] = meth
            self.static_methods.append(meth)
        elif elm.tag == 'constructor':
            assert not self.constructor
            self.constructor = Constructor.from_xml(module, elm, self)
        else:
            GTypedType._parse_xml_element(self, module, elm)

class Pointer(InstanceType):
    def __init__(self):
        InstanceType.__init__(self)

class Boxed(InstanceType):
    def __init__(self):
        InstanceType.__init__(self)

class Class(InstanceType):
    def __init__(self):
        InstanceType.__init__(self)
        self.parent = None
        self.vmethods = []
        self.signals = []
        self.constructable = None
        self.__vmethod_hash = {}
        self.__signal_hash = {}

    def _parse_attribute(self, attr, value):
        if attr in ('parent'):
            _set_unique_attribute(self, attr, value)
        elif attr in ('constructable'):
            _set_unique_attribute_bool(self, attr, value)
        else:
            return InstanceType._parse_attribute(self, attr, value)
        return True

    def _parse_xml_element(self, module, elm):
        if elm.tag == 'virtual':
            meth = VMethod.from_xml(module, elm, self)
            assert not meth.name in self.__vmethod_hash
            self.__vmethod_hash[meth.name] = meth
            self.vmethods.append(meth)
        elif elm.tag == 'signal':
            meth = Signal.from_xml(module, elm, self)
            assert not meth.name in self.__signal_hash
            self.__signal_hash[meth.name] = meth
            self.signals.append(meth)
        else:
            InstanceType._parse_xml_element(self, module, elm)

    def _parse_xml(self, module, elm, *args):
        InstanceType._parse_xml(self, module, elm, *args)
        if self.parent is None:
            raise RuntimeError('class parent name missing')
        if self.constructable and self.constructor:
            raise RuntimeError('both constructor and constructable attributes present')

class Module(object):
    def __init__(self):
        object.__init__(self)
        self.name = None
        self.__classes = []
        self.__boxed = []
        self.__pointers = []
        self.__enums = []
        self.__class_hash = {}
        self.__functions = []
        self.__function_hash = {}
        self.__import_modules = []
        self.__parsing_done = False
        self.__types = {}
        self.__symbols = {}

    def __add_type(self, typ):
        if typ.name in self.__class_hash:
            raise RuntimeError('duplicated type %s' % typ.name)
        self.__class_hash[typ.name] = typ

    def __finish_type(self, typ):
        if isinstance(typ, Type):
            return typ
        if typ in self.__types:
            return self.__types[typ]
        if typ == 'GError**':
            return GErrorReturnType()
        m = re.match(r'(const-)?([\w\d_]+)\*', typ)
        if m:
            name = m.group(2)
            if name in self.__types:
                obj_type = self.__types[name]
                if isinstance(obj_type, InstanceType):
                    return obj_type
        m = re.match(r'Array<([\w\d_]+)>\*', typ)
        if m:
            elm_type = self.__finish_type(m.group(1))
            return ArrayType(elm_type)
        m = re.match(r'([\w\d_]+)Array\*', typ)
        if m:
            elm_type = self.__finish_type(m.group(1))
            return ArrayType(elm_type)
        return BasicType(typ)

    def __finish_parsing_method(self, meth, typ):
        sym_id = meth.symbol_id()
        if self.__symbols.get(sym_id):
            raise RuntimeError('duplicate symbol %s' % sym_id)
        self.__symbols[sym_id] = meth

        for p in meth.params:
            p.type = self.__finish_type(p.type)
        if meth.retval:
            meth.retval.type = self.__finish_type(meth.retval.type)

        meth.has_gerror_return = False
        if meth.params and isinstance(meth.params[-1].type, GErrorReturnType):
            meth.has_gerror_return = True

        if meth.kwargs:
            params = list(meth.params)
            pos_args = []
            kw_args = []
            if meth.has_gerror_return:
                params = params[:-1]
            seen_kwarg = False
            for p in params:
                if p.default_value is not None:
                    seen_kwarg = True
                elif seen_kwarg:
                    raise RuntimeError('in %s: parameter without a default value follows a kwarg one' % sym_id)
                if p.default_value is not None:
                    kw_args.append(p)
                else:
                    pos_args.append(p)

    def __finish_parsing_type(self, typ):
        if hasattr(typ, 'constructor') and typ.constructor is not None:
            self.__finish_parsing_method(typ.constructor, typ)
        for meth in typ.static_methods:
            self.__finish_parsing_method(meth, typ)
        for meth in typ.methods:
            self.__finish_parsing_method(meth, typ)
        if hasattr(typ, 'vmethods'):
            for meth in typ.vmethods:
                self.__finish_parsing_method(meth, typ)
        if hasattr(typ, 'signals'):
            for meth in typ.signals:
                self.__finish_parsing_method(meth, typ)

    def __finish_parsing_enum(self, typ):
        for v in typ.values:
            sym_id = v.symbol_id()
            if self.__symbols.get(sym_id):
                raise RuntimeError('duplicate symbol %s' % sym_id)
            self.__symbols[sym_id] = v

    def __add_type_symbol(self, typ):
        sym_id = typ.symbol_id()
        if self.__symbols.get(sym_id):
            raise RuntimeError('duplicate symbol %s' % sym_id)
        self.__symbols[sym_id] = typ

    def __finish_parsing(self):
        if self.__parsing_done:
            return

        for typ in self.__classes + self.__boxed + self.__pointers + self.__enums:
            self.__add_type_symbol(typ)
            self.__types[typ.name] = typ

        for module in self.__import_modules:
            for typ in module.get_classes() + module.get_boxed() + \
                       module.get_pointers() + module.get_enums():
                self.__types[typ.name] = typ
            for sym in module.__symbols:
                if self.__symbols.get(sym):
                    raise RuntimeError('duplicate symbol %s' % sym)
                self.__symbols[sym] = module.__symbols[sym]

        for typ in self.__classes + self.__boxed + self.__pointers:
            self.__finish_parsing_type(typ)

        for typ in self.__enums:
            self.__finish_parsing_enum(typ)

        for func in self.__functions:
            self.__finish_parsing_method(func, None)

        self.__parsing_done = True

    def get_classes(self):
        self.__finish_parsing()
        return list(self.__classes)

    def get_boxed(self):
        self.__finish_parsing()
        return list(self.__boxed)

    def get_pointers(self):
        self.__finish_parsing()
        return list(self.__pointers)

    def get_enums(self):
        self.__finish_parsing()
        return list(self.__enums)

    def get_functions(self):
        self.__finish_parsing()
        return list(self.__functions)

    def get_symbols(self):
        self.__finish_parsing()
        return dict(self.__symbols)

    def __parse_module_entry(self, elm):
        if elm.tag == 'class':
            cls = Class.from_xml(self, elm)
            self.__add_type(cls)
            self.__classes.append(cls)
        elif elm.tag == 'boxed':
            cls = Boxed.from_xml(self, elm)
            self.__add_type(cls)
            self.__boxed.append(cls)
        elif elm.tag == 'pointer':
            cls = Pointer.from_xml(self, elm)
            self.__add_type(cls)
            self.__pointers.append(cls)
        elif elm.tag == 'enum':
            enum = Enum.from_xml(self, elm)
            self.__add_type(enum)
            self.__enums.append(enum)
        elif elm.tag == 'flags':
            enum = Flags.from_xml(self, elm)
            self.__add_type(enum)
            self.__enums.append(enum)
        elif elm.tag == 'function':
            func = Function.from_xml(self, elm)
            assert not func.name in self.__function_hash
            self.__function_hash[func.name] = func
            self.__functions.append(func)
        else:
            raise RuntimeError('unknown element %s' % (elm.tag,))

    def import_module(self, mod):
        self.__import_modules.append(mod)

    @staticmethod
    def from_xml(filename):
        mod = Module()
        xml = _etree.ElementTree()
        xml.parse(filename)
        root = xml.getroot()
        assert root.tag == 'module'
        mod.name = root.get('name')
        assert mod.name is not None
        for elm in root.getchildren():
            mod.__parse_module_entry(elm)
        return mod
