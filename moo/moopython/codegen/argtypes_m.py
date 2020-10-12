from argtypes import ArgType, IntArg, matcher

matcher.register('index', IntArg())

class StrvArg(ArgType):
    def write_param(self, ptype, pname, pdflt, pnull, info):
        if pdflt:
            if pdflt != 'NULL': raise TypeError("Only NULL is supported as a default char** value")
            info.varlist.add('char', '**' + pname + ' = ' + pdflt)
        else:
            info.varlist.add('char', '**' + pname)
        info.arglist.append(pname)
        if pnull:
            info.add_parselist('O&', ['_moo_pyobject_to_strv', '&' + pname], [pname])
        else:
            info.add_parselist('O&', ['_moo_pyobject_to_strv_no_null', '&' + pname], [pname])
    def write_return(self, ptype, ownsreturn, info):
        if ownsreturn:
            # have to free result ...
            info.varlist.add('char', '**ret')
            info.varlist.add('PyObject', '*py_ret')
            info.codeafter.append('    py_ret = _moo_strv_to_pyobject (ret);\n' +
                                  '    g_strfreev (ret);\n' +
                                  '    return py_ret;')
        else:
            info.varlist.add('char', '**ret')
            info.codeafter.append('    return _moo_strv_to_pyobject (ret);')

class StringSListArg(ArgType):
    def write_return(self, ptype, ownsreturn, info):
        if ownsreturn:
            # have to free result ...
            info.varlist.add('GSList', '*ret')
            info.varlist.add('PyObject', '*py_ret')
            info.codeafter.append('    py_ret = _moo_string_slist_to_pyobject (ret);\n' +
                                  '    g_slist_foreach (ret, (GFunc) g_free, NULL);\n' +
                                  '    g_slist_free (ret);\n' +
                                  '    return py_ret;')
        else:
            info.varlist.add('GSList', '*ret')
            info.codeafter.append('    return _moo_string_slist_to_pyobject (ret);')

class ObjectSListArg(ArgType):
    def write_return(self, ptype, ownsreturn, info):
        if ownsreturn:
            # have to free result ...
            info.varlist.add('GSList', '*ret')
            info.varlist.add('PyObject', '*py_ret')
            info.codeafter.append('    py_ret = _moo_object_slist_to_pyobject (ret);\n' +
                                  '    g_slist_foreach (ret, (GFunc) g_object_unref, NULL);\n' +
                                  '    g_slist_free (ret);\n' +
                                  '    return py_ret;')
        else:
            info.varlist.add('GSList', '*ret')
            info.codeafter.append('    return _moo_object_slist_to_pyobject (ret);')

class ObjectArrayArg(ArgType):
    def __init__(self, c_type, c_prefix):
        self.c_type = c_type
        self.c_prefix = c_prefix

    def write_param(self, ptype, pname, pdflt, pnull, info):
        if pdflt:
            if pdflt != 'NULL': raise TypeError("Only NULL is supported as a default %s* value" % self.c_type)
            info.varlist.add(self.c_type, '*' + pname + ' = ' + pdflt)
        else:
            info.varlist.add(self.c_type, '*' + pname)
        info.arglist.append(pname)
        if pnull:
            info.add_parselist('O&', ['_moo_pyobject_to_object_array', '(MooObjectArray**) &' + pname], [pname])
        else:
            info.add_parselist('O&', ['_moo_pyobject_to_object_array_no_null', '(MooObjectArray**) &' + pname], [pname])
        info.codeafter.append('    %s_free (%s);' % (self.c_prefix, pname))

    def write_return(self, ptype, ownsreturn, info):
        if ownsreturn:
            # have to free result ...
            info.varlist.add(self.c_type, '*ret')
            info.varlist.add('PyObject', '*py_ret')
            info.codeafter.append(('    py_ret = _moo_object_array_to_pyobject ((MooObjectArray*) ret);\n' +
                                   '    %s_free (ret);\n' +
                                   '    return py_ret;') % (self.c_prefix,))
        else:
            info.varlist.add(self.c_type, '*ret')
            info.codeafter.append('    return _moo_object_array_to_pyobject ((MooObjectArray*) ret);')

class BoxedArrayArg(ArgType):
    def __init__(self, elm_c_type, elm_c_prefix, elm_g_type):
        self.elm_c_type = elm_c_type
        self.elm_c_prefix = elm_c_prefix
        self.elm_g_type = elm_g_type
        self.c_type = elm_c_type + 'Array'

    def write_param(self, ptype, pname, pdflt, pnull, info):
        if pdflt:
            if pdflt != 'NULL': raise TypeError("Only NULL is supported as a default %s* value" % self.c_type)
            info.varlist.add(self.c_type, '*' + pname + ' = ' + pdflt)
        else:
            info.varlist.add(self.c_type, '*' + pname)
        info.arglist.append(pname)
        if pnull:
            info.add_parselist('O&', ['_moo_pyobject_to_boxed_array', self.elm_g_type + ', (MooPtrArray**) &' + pname], [pname])
        else:
            info.add_parselist('O&', ['_moo_pyobject_to_boxed_array_no_null', self.elm_g_type + ', (MooPtrArray**) &' + pname], [pname])
        info.codeafter.append('    _moo_boxed_array_free (%s, (MooPtrArray*) %s);' % (self.elm_g_type, pname))

    def write_return(self, ptype, ownsreturn, info):
        if ownsreturn:
            # have to free result ...
            info.varlist.add(self.c_type, '*ret')
            info.varlist.add('PyObject', '*py_ret')
            info.codeafter.append(('    py_ret = _moo_boxed_array_to_pyobject (%s, (MooPtrArray*) ret);\n' +
                                   '    _moo_boxed_array_free (%s, ret);\n' +
                                   '    return py_ret;') % (self.elm_g_type, self.elm_g_type))
        else:
            info.varlist.add(self.c_type, '*ret')
            info.codeafter.append('    return _moo_boxed_array_to_pyobject (%s, (MooPtrArray*) ret);' % (self.elm_g_type,))

class NoRefObjectSListArg(ArgType):
    def write_return(self, ptype, ownsreturn, info):
        if ownsreturn:
            # have to free result ...
            info.varlist.add('GSList', '*ret')
            info.varlist.add('PyObject', '*py_ret')
            info.codeafter.append('    py_ret = _moo_object_slist_to_pyobject (ret);\n' +
                                  '    g_slist_free (ret);\n' +
                                  '    return py_ret;')
        else:
            info.varlist.add('GSList', '*ret')
            info.codeafter.append('    return _moo_object_slist_to_pyobject (ret);')

arg = StrvArg()
matcher.register('strv', arg)

arg = StringSListArg()
matcher.register('string-slist', arg)
arg = ObjectSListArg()
matcher.register('object-slist', arg)
arg = NoRefObjectSListArg()
matcher.register('no-ref-object-slist', arg)

for typ, prefix in (('MooFileArray', 'moo_file_array'),
                    ('MooEditArray', 'moo_edit_array'),
                    ('MooEditViewArray', 'moo_edit_view_array'),
                    ('MooEditTabArray', 'moo_edit_tab_array'),
                    ('MooEditWindowArray', 'moo_edit_window_array'),
                    ('MooOpenInfoArray', 'moo_open_info_array'),
                    ):
    arg = ObjectArrayArg(typ, prefix)
    matcher.register(typ + '*', arg)
