import sys
import xml.etree.ElementTree as etree

import mdp.module as module

class Writer(object):
    def __init__(self, out):
        object.__init__(self)
        self.out = out
        self.xml = etree.TreeBuilder()
        self.__tag_opened = False
        self.__depth = 0

    def __start_tag(self, tag, attrs={}):
        if self.__tag_opened:
            self.xml.data('\n')
        if self.__depth > 0:
            self.xml.data('  ' * self.__depth)
        elm = self.xml.start(tag, attrs)
        self.__tag_opened = True
        self.__depth += 1
        return elm

    def __end_tag(self, tag):
        if not self.__tag_opened and self.__depth > 1:
            self.xml.data('  ' * (self.__depth - 1))
        elm = self.xml.end(tag)
        self.xml.data('\n')
        self.__tag_opened = False
        self.__depth -= 1
        return elm

    def __write_docs(self, docs):
        if not docs:
            return
        self.__start_tag('doc')
        docs = ' '.join(docs)
        self.xml.data(docs)
        self.__end_tag('doc')

    def __write_summary(self, summary):
        if not summary:
            return
        self.__start_tag('summary')
        if isinstance(summary, list):
            assert len(summary) == 1
            summary = summary[0]
        self.xml.data(summary)
        self.__end_tag('summary')

    def __write_param_or_retval_annotations(self, param, elm):
        for k in param.attributes:
            elm.set(k, self.attributes[v])
        if param.transfer_mode is not None:
            elm.set('transfer_mode', param.transfer_mode)
        if param.element_type is not None:
            elm.set('element_type', param.element_type)
        if param.array:
            elm.set('array', '1')
        if param.array_fixed_len is not None:
            elm.set('array_fixed_len', str(param.array_fixed_len))
        if param.array_len_param is not None:
            elm.set('array_len_param', param.array_len_param)
        if param.array_zero_terminated:
            elm.set('array_zero_terminated', '1')

    def __write_param_annotations(self, param, elm):
        self.__write_param_or_retval_annotations(param, elm)
        if param.inout:
            elm.set('inout', '1')
        elif param.out:
            elm.set('out', '1')
        if param.caller_allocates:
            elm.set('caller_allocates', '1')
        elif param.callee_allocates:
            elm.set('callee_allocates', '1')
        if param.allow_none:
            elm.set('allow_none', '1')
        if param.default_value is not None:
            elm.set('default_value', param.default_value)
        if param.scope != 'call':
            elm.set('scope', param.scope)

    def __write_retval_annotations(self, retval, elm):
        self.__write_param_or_retval_annotations(retval, elm)

    def __check_type(self, param, func):
        if param.type in ('char*', 'const-char*'):
            print >>sys.stderr, '*** WARNING: raw type %s used in function %s' % (param.type, func)

    def __write_param(self, param, func):
        self.__check_type(param, func)
        dic = dict(name=param.name, type=param.type)
        elm = self.__start_tag('param', dic)
        self.__write_param_annotations(param, elm)
        self.__write_docs(param.docs)
        self.__end_tag('param')

    def __write_retval(self, retval, func):
        self.__check_type(retval, func)
        dic = dict(type=retval.type)
        elm = self.__start_tag('retval', dic)
        self.__write_retval_annotations(retval, elm)
        self.__write_docs(retval.docs)
        self.__end_tag('retval')

    def __write_class(self, cls):
        if not cls.parent and cls.name != 'GObject':
            raise RuntimeError('parent missing in class %s' % (cls.name,))
        dic = dict(name=cls.name, short_name=cls.short_name, parent=cls.parent or 'none', gtype_id=cls.gtype_id)
        for k in cls.annotations:
            dic[k] = cls.annotations[k]
        if cls.constructable:
            dic['constructable'] = '1'
        self.__start_tag('class', dic)
        self.__write_summary(cls.summary)
        self.__write_docs(cls.docs)
        if cls.constructor is not None:
            self.__write_function(cls.constructor, 'constructor')
        for meth in sorted(cls.static_methods, lambda x, y: cmp(x.name, y.name)):
            self.__write_function(meth, 'static-method')
        for meth in sorted(cls.signals, lambda x, y: cmp(x.name, y.name)):
            self.__write_function(meth, 'signal')
        for meth in sorted(cls.vmethods, lambda x, y: cmp(x.name, y.name)):
            self.__write_function(meth, 'virtual')
        for meth in sorted(cls.methods, lambda x, y: cmp(x.name, y.name)):
            self.__write_function(meth, 'method')
        self.__end_tag('class')

    def __write_boxed(self, cls):
        dic = dict(name=cls.name, short_name=cls.short_name, gtype_id=cls.gtype_id)
        for k in cls.annotations:
            dic[k] = cls.annotations[k]
        tag = 'boxed' if isinstance(cls, module.Boxed) else 'pointer'
        self.__start_tag(tag, dic)
        self.__write_summary(cls.summary)
        self.__write_docs(cls.docs)
        if cls.constructor is not None:
            self.__write_function(cls.constructor, 'constructor')
        for meth in cls.methods:
            self.__write_function(meth, 'method')
        self.__end_tag(tag)

    def __write_enum(self, enum):
        if isinstance(enum, module.Enum):
            tag = 'enum'
        else:
            tag = 'flags'
        dic = dict(name=enum.name, short_name=enum.short_name, gtype_id=enum.gtype_id)
        for k in enum.annotations:
            dic[k] = enum.annotations[k]
        self.__start_tag(tag, dic)

        for v in enum.values:
            dic = dict(name=v.name)
            if v.attributes:
                dic.update(v.attributes)
            elm = self.__start_tag('value', dic)
            self.__write_docs(v.docs)
            self.__end_tag('value')

        self.__write_summary(enum.summary)
        self.__write_docs(enum.docs)
        self.__end_tag(tag)

    def __write_function(self, func, tag):
        name=func.name
        if tag == 'signal':
            name = name.replace('_', '-')
        dic = dict(name=name)
        if tag != 'virtual' and tag != 'signal':
            dic['c_name'] = func.c_name
        if getattr(func, 'kwargs', False):
            dic['kwargs'] = '1'
        for k in func.annotations:
            dic[k] = func.annotations[k]
        self.__start_tag(tag, dic)
        for p in func.params:
            self.__write_param(p, func.c_name)
        if func.retval:
            self.__write_retval(func.retval, func.c_name)
        self.__write_summary(func.summary)
        self.__write_docs(func.docs)
        self.__end_tag(tag)

    def write(self, module):
        self.__start_tag('module', dict(name=module.name))
        for cls in sorted(module.classes, lambda x, y: cmp(x.name, y.name)):
            self.__write_class(cls)
        for cls in sorted(module.boxed, lambda x, y: cmp(x.name, y.name)):
            self.__write_boxed(cls)
        for enum in sorted(module.enums, lambda x, y: cmp(x.name, y.name)):
            self.__write_enum(enum)
        for func in sorted(module.functions, lambda x, y: cmp(x.name, y.name)):
            self.__write_function(func, 'function')
        self.__end_tag('module')
        elm = self.xml.close()
        etree.ElementTree(elm).write(self.out)

def write_xml(module, out):
    Writer(out).write(module)
