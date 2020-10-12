#include <config.h>
#include "moo-lua-api-util.h"
#include "medit-lua.h"
#include "moolua/lua/lauxlib.h"
#include "moolua/lua/ldo.h"
#include "mooutils/mooutils.h"
#include "mooutils/moo-test-utils.h"
#include <vector>
#include <string>
#include <string.h>

MOO_DEFINE_QUARK_STATIC ("moo-lua-methods", moo_lua_methods_quark)

#define GINSTANCE_META "moo-lua-ginstance-wrapper"

struct LGInstance
{
    gpointer instance;
    GType type;
};

static std::vector<std::string> function_stack;

MooLuaCurrentFunc::MooLuaCurrentFunc (const char *func)
{
    function_stack.push_back (func);
}

MooLuaCurrentFunc::~MooLuaCurrentFunc ()
{
    function_stack.pop_back ();
}

namespace {

class StringHolder
{
public:
    StringHolder (char *s)
        : m_s (s)
    {
    }

    ~StringHolder ()
    {
        g_free (m_s);
    }

private:
    char *m_s;
};

#ifdef DEBUG

class CheckStack
{
public:
    CheckStack(lua_State *L, int n_add = 0)
        : m_L(L)
        , m_top_old(lua_gettop(L))
        , m_n_add(n_add)
    {
    }

    ~CheckStack()
    {
        if (!luaD_inerror(m_L))
            g_assert(lua_gettop(m_L) == m_top_old + m_n_add);
        else
            g_assert(lua_gettop(m_L) >= m_top_old + m_n_add);
    }

private:
    lua_State *m_L;
    int m_top_old;
    int m_n_add;
};

#define CHECK_STACK(L) CheckStack check__(L)
#define CHECK_STACKN(L) CheckStack check__(L, n)

#else // !DEBUG

#define CHECK_STACK(L) G_STMT_START {} G_STMT_END
#define CHECK_STACKN(L) G_STMT_START {} G_STMT_END

#endif // !DEBUG

} // namespace

const char *
moo_lua_current_function (void)
{
    g_return_val_if_fail (function_stack.size () > 0, "`none'");
    return function_stack.back ().c_str ();
}

static int
moo_lua_error_impl (lua_State  *L,
                    const char *message)
{
    return luaL_error (L, "function %s: %s", moo_lua_current_function (), message);
}

int
moo_lua_error (lua_State  *L,
               const char *fmt,
               ...)
{
    char *message;
    va_list args;

    va_start (args, fmt);
    message = g_strdup_vprintf (fmt, args);
    va_end (args);

    StringHolder sh(message);
    return moo_lua_error_impl (L, message);
}

int
moo_lua_errorv (lua_State          *L,
                const char         *fmt,
                va_list             args)
{
    char *message = g_strdup_vprintf (fmt, args);
    StringHolder sh(message);
    return moo_lua_error_impl (L, message);
}

static int
moo_lua_arg_error_impl (lua_State  *L,
                        int         narg,
                        const char *param_name,
                        const char *message)
{
    return luaL_error (L, "function %s, argument %d (%s): %s",
                       moo_lua_current_function (),
                       narg,
                       param_name ? param_name : "`none'",
                       message);
}

int
moo_lua_arg_error (lua_State  *L,
                   int         narg,
                   const char *param_name,
                   const char *fmt,
                   ...)
{
    char *message;
    va_list args;

    va_start (args, fmt);
    message = g_strdup_vprintf (fmt, args);
    va_end (args);

    StringHolder sh(message);
    return moo_lua_arg_error_impl (L, narg, param_name, message);
}

int
moo_lua_arg_errorv (lua_State  *L,
                    int         narg,
                    const char *param_name,
                    const char *fmt,
                    va_list     args)
{
    char *message = g_strdup_vprintf (fmt, args);
    StringHolder sh(message);
    return moo_lua_arg_error_impl (L, narg, param_name, message);
}


bool
moo_lua_check_kwargs (lua_State *L,
                      int        narg)
{
    return lua_istable (L, narg);
}

int
moo_lua_get_kwarg (lua_State  *L,
                   int         narg_dict,
                   int         pos,
                   const char *kw)
{
    if (!lua_istable (L, narg_dict))
    {
        g_critical ("oops");
        luaL_error (L, "oops");
    }

    int narg_pos = MOO_NONEXISTING_INDEX;
    int narg_kw = MOO_NONEXISTING_INDEX;

    lua_pushinteger (L, pos);
    lua_gettable (L, narg_dict);
    if (!lua_isnil (L, -1))
        narg_pos = lua_gettop (L);
    else
        lua_pop (L, 1);

    lua_pushstring (L, kw);
    lua_gettable (L, narg_dict);
    if (!lua_isnil (L, -1))
        narg_kw = lua_gettop (L);
    else
        lua_pop (L, 1);

    if (narg_pos != MOO_NONEXISTING_INDEX && narg_kw != MOO_NONEXISTING_INDEX)
        luaL_error (L, "parameter %s is given twice, as positional and as keyword argument", kw);

    return narg_pos != MOO_NONEXISTING_INDEX ? narg_pos : narg_kw;
}


void
moo_lua_register_methods (GType              type,
                          MooLuaMethodEntry *entries)
{
    GHashTable *methods;

    methods = (GHashTable*) g_type_get_qdata (type, moo_lua_methods_quark ());

    if (!methods)
    {
        methods = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        g_type_set_qdata (type, moo_lua_methods_quark (), methods);
    }

    while (entries->name)
    {
        g_hash_table_insert (methods, g_strdup (entries->name), (gpointer) entries->impl);
        entries++;
    }
}

static void
moo_lua_register_method (GType         type,
                         const char   *name,
                         MooLuaMethod  meth)
{
    GHashTable *methods;

    methods = (GHashTable*) g_type_get_qdata (type, moo_lua_methods_quark ());

    if (!methods)
    {
        methods = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        g_type_set_qdata (type, moo_lua_methods_quark (), methods);
    }

    g_hash_table_insert (methods, g_strdup (name), (gpointer) meth);
}

static GType *
list_parent_types (GType type)
{
    GType *ifaces, *p;
    GArray *types;

    if (G_TYPE_FUNDAMENTAL (type) != G_TYPE_OBJECT)
        return NULL;

    types = g_array_new (TRUE, TRUE, sizeof (GType));

    ifaces = g_type_interfaces (type, NULL);
    for (p = ifaces; p && *p; ++p)
        g_array_append_val (types, *p);

    while (type != G_TYPE_OBJECT)
    {
        type = g_type_parent (type);
        g_array_append_val (types, type);
    }

    g_free (ifaces);
    return (GType*) g_array_free (types, FALSE);
}

static MooLuaMethod
lookup_method_simple (GType       type,
                      const char *name)
{
    GHashTable *methods = (GHashTable*) g_type_get_qdata (type, moo_lua_methods_quark ());
    return methods ? (MooLuaMethod) g_hash_table_lookup (methods, name) : NULL;
}

MooLuaMethod
moo_lua_lookup_method (lua_State  *L,
                       GType       type,
                       const char *name)
{
    GType *parent_types = NULL;
    GType *p;
    MooLuaMethod meth = NULL;

    g_return_val_if_fail (name != NULL, NULL);

    meth = lookup_method_simple (type, name);

    if (!meth)
    {
        parent_types = list_parent_types (type);

        for (p = parent_types; p && *p; ++p)
        {
            meth = lookup_method_simple (*p, name);

            if (meth != NULL)
                break;
        }

        if (meth != NULL)
            moo_lua_register_method (type, name, meth);
    }

    g_free (parent_types);

    if (!meth)
        luaL_error (L, "no method %s in type %s", name, g_type_name (type));

    return meth;
}


void
moo_lua_register_static_methods (lua_State      *L,
                                 const char     *package_name,
                                 const char     *class_name,
                                 const luaL_Reg *methods)
{
    std::string name(package_name);
    name.push_back('.');
    name.append(class_name);

    if (luaL_findtable(L, LUA_GLOBALSINDEX, name.c_str(), 1))
        luaL_error(L, "oops");

    while (methods->func)
    {
        lua_pushcclosure(L, methods->func, 0);
        lua_setfield(L, -2, methods->name);
        methods++;
    }

    lua_pop (L, 1);
}


static void
moo_lua_add_enum_value (lua_State  *L,
                        const char *name,
                        int         value,
                        const char *prefix)
{
    if (g_str_has_prefix (name, prefix))
        name += strlen (prefix);
    lua_pushinteger (L, value);
    lua_setfield (L, -2, name);
}

void
moo_lua_register_enum (lua_State  *L,
                       const char *package_name,
                       GType       type,
                       const char *prefix)
{
    lua_getfield (L, LUA_GLOBALSINDEX, package_name);

    g_return_if_fail (!lua_isnil (L, -1));

    if (g_type_is_a (type, G_TYPE_ENUM))
    {
        GEnumClass *klass = (GEnumClass*) g_type_class_ref (type);
        g_return_if_fail (klass != NULL);
        for (guint i = 0; i < klass->n_values; ++i)
            moo_lua_add_enum_value (L,
                                    klass->values[i].value_name,
                                    klass->values[i].value,
                                    prefix);
        g_type_class_unref (klass);
    }
    else if (g_type_is_a (type, G_TYPE_FLAGS))
    {
        GFlagsClass *klass = (GFlagsClass*) g_type_class_ref (type);
        g_return_if_fail (klass != NULL);
        for (guint i = 0; i < klass->n_values; ++i)
            moo_lua_add_enum_value (L,
                                    klass->values[i].value_name,
                                    klass->values[i].value,
                                    prefix);
        g_type_class_unref (klass);
    }
    else
    {
        g_return_if_reached ();
    }

    lua_pop (L, 1);
}


static LGInstance *
get_arg_instance (lua_State *L, int arg, const char *param_name)
{
    void *p = lua_touserdata (L, arg);

    if (!p || !lua_getmetatable (L, arg))
        moo_lua_arg_error (L, arg, param_name,
                           "instance expected, got %s",
                           luaL_typename (L, arg));

    lua_getfield (L, LUA_REGISTRYINDEX, GINSTANCE_META);
    if (!lua_rawequal (L, -1, -2))
        moo_lua_arg_error (L, arg, param_name,
                           "instance expected, got %s",
                           luaL_typename (L, arg));

    lua_pop (L, 2);  /* remove both metatables */
    return (LGInstance*) p;
}

static LGInstance *
get_arg_if_instance (lua_State *L, int arg)
{
    void *p = lua_touserdata (L, arg);

    if (!p || !lua_getmetatable (L, arg))
        return NULL;

    lua_getfield (L, LUA_REGISTRYINDEX, GINSTANCE_META);

    if (!lua_rawequal(L, -1, -2))
        p = NULL;

    lua_pop (L, 2);
    return (LGInstance*) p;
}

static const char *
get_arg_string (lua_State *L, int narg, const char *param_name)
{
    const char *arg = luaL_checkstring (L, narg);
    if (arg == 0)
        moo_lua_arg_error (L, narg, param_name, "nil string");
    return arg;
}


static gboolean
ginstance_eq (LGInstance *lg1, LGInstance *lg2)
{
    if (!lg1->instance || !lg2->instance)
        return lg1->instance == lg2->instance;

    if (lg1->type == lg2->type && lg1->type == GTK_TYPE_TEXT_ITER && lg2->type == GTK_TYPE_TEXT_ITER)
        return gtk_text_iter_equal ((GtkTextIter*) lg1->instance, (GtkTextIter*) lg2->instance);

    return lg1->instance == lg2->instance;
}

static int
cfunc_ginstance__eq (lua_State *L)
{
    LGInstance *lg1 = get_arg_instance (L, 1, "arg1");
    LGInstance *lg2 = get_arg_instance (L, 2, "arg2");
    lua_pushboolean (L, ginstance_eq (lg1, lg2));
    return 1;
}

static int
cfunc_ginstance__lt (lua_State *L)
{
    LGInstance *lg1 = get_arg_instance (L, 1, "arg1");
    LGInstance *lg2 = get_arg_instance (L, 2, "arg2");

    if (lg1->instance && lg2->instance && lg1->type == lg2->type &&
        lg1->type == GTK_TYPE_TEXT_ITER && lg2->type == GTK_TYPE_TEXT_ITER)
    {
        lua_pushboolean (L, gtk_text_iter_compare ((GtkTextIter*) lg1->instance, (GtkTextIter*) lg2->instance) < 0);
        return 1;
    }

    return luaL_error (L, "unsupported operation");
}

static int
cfunc_ginstance__le (lua_State *L)
{
    LGInstance *lg1 = get_arg_instance (L, 1, "arg1");
    LGInstance *lg2 = get_arg_instance (L, 2, "arg2");

    if (lg1->instance && lg2->instance && lg1->type == lg2->type &&
        lg1->type == GTK_TYPE_TEXT_ITER && lg2->type == GTK_TYPE_TEXT_ITER)
    {
        lua_pushboolean (L, gtk_text_iter_compare ((GtkTextIter*) lg1->instance, (GtkTextIter*) lg2->instance) <= 0);
        return 1;
    }

    return luaL_error (L, "unsupported operation");
}

static int
cfunc_ginstance__tostring (lua_State *L)
{
    LGInstance *lg = get_arg_instance (L, 1, "arg");

    if (!lg->instance)
    {
        lua_pushstring (L, "<null>");
        return 1;
    }

    GType type = g_type_is_a (lg->type, G_TYPE_OBJECT) ? G_OBJECT_TYPE (lg->instance) : lg->type;
    char *s = g_strdup_printf ("<%s %p>", g_type_name (type), lg->instance);
    StringHolder sh(s);

    lua_pushstring (L, s);
    return 1;
}

int
cfunc_call_named_method (lua_State *L)
{
    // Allow both obj.method(arg) and obj:method(arg) syntaxes.
    // We store the object as upvalue so it's always available and
    // we only need to check whether the first function argument
    // is the same object or not. (This does mean a method can't
    // take a first argument equal to the target object)

    LGInstance *self = get_arg_instance (L, lua_upvalueindex (1), "self");
    LGInstance *arg = get_arg_if_instance (L, 1);

    int first_arg = 2;
    if (!arg || self->instance != arg->instance)
        first_arg = 1;

    const char *meth = get_arg_string (L, lua_upvalueindex (2), NULL);

    MooLuaMethod func = moo_lua_lookup_method (L, self->type, meth);
    g_return_val_if_fail (func != NULL, 0);

    return func (self->instance, L, first_arg);
}

static int
cfunc_ginstance__index (lua_State *L)
{
    lua_pushvalue (L, 1);
    lua_pushvalue (L, 2);
    lua_pushcclosure (L, cfunc_call_named_method, 2);
    return 1;
}

int
cfunc_ginstance__gc (lua_State *L)
{
    LGInstance *self = get_arg_instance (L, 1, "self");

    if (self->instance)
    {
        switch (G_TYPE_FUNDAMENTAL (self->type))
        {
            case G_TYPE_OBJECT:
                g_object_unref (self->instance);
                break;
            case G_TYPE_BOXED:
                g_boxed_free (self->type, self->instance);
                break;
            case G_TYPE_POINTER:
                break;
            default:
                g_critical ("%s", g_type_name (self->type));
                moo_assert_not_reached ();
                break;
        }
    }

    self->instance = NULL;
    return 0;
}

static int
moo_lua_push_ginstance (lua_State *L,
                        gpointer   instance,
                        GType      type,
                        GType      type_fundamental,
                        gboolean   make_copy)
{
    g_return_val_if_fail (L != NULL, 0);

    if (!instance)
    {
        lua_pushnil (L);
        return 1;
    }

    LGInstance *lg = (LGInstance*) lua_newuserdata (L, sizeof (LGInstance));
    lg->instance = NULL;
    lg->type = type;

    // create metatable M
    if (luaL_newmetatable (L, GINSTANCE_META))
    {
        lua_pushcfunction (L, cfunc_ginstance__eq);
        lua_setfield (L, -2, "__eq");
        lua_pushcfunction (L, cfunc_ginstance__index);
        lua_setfield (L, -2, "__index");
        lua_pushcfunction (L, cfunc_ginstance__gc);
        lua_setfield (L, -2, "__gc");
        lua_pushcfunction (L, cfunc_ginstance__lt);
        lua_setfield (L, -2, "__lt");
        lua_pushcfunction (L, cfunc_ginstance__le);
        lua_setfield (L, -2, "__le");
        lua_pushcfunction (L, cfunc_ginstance__tostring);
        lua_setfield (L, -2, "__tostring");
    }

    lua_setmetatable (L, -2);

    if (!make_copy)
        lg->instance = instance;
    else if (type_fundamental == G_TYPE_POINTER)
        lg->instance = instance;
    else if (type_fundamental == G_TYPE_BOXED)
        lg->instance = g_boxed_copy (type, instance);
    else if (type_fundamental == G_TYPE_OBJECT)
        lg->instance = g_object_ref (instance);
    else
        g_return_val_if_reached (0);

    return 1;
}

int
moo_lua_push_object (lua_State *L,
                     GObject   *obj,
                     gboolean   make_copy)
{
    if (obj)
    {
        return moo_lua_push_ginstance (L, obj, G_OBJECT_TYPE (obj), G_TYPE_OBJECT, make_copy);
    }
    else
    {
        lua_pushnil (L);
        return 1;
    }
}

int
moo_lua_push_boxed (lua_State *L,
                    gpointer   boxed,
                    GType      type,
                    gboolean   make_copy)
{
    return moo_lua_push_ginstance (L, boxed, type, G_TYPE_BOXED, make_copy);
}

int
moo_lua_push_pointer (lua_State *L,
                      gpointer   ptr,
                      GType      type,
                      gboolean   make_copy)
{
    return moo_lua_push_ginstance (L, ptr, type, G_TYPE_POINTER, make_copy);
}


struct CallbacksData
{
    GSList *closures; // MooLuaSignalClosure*
};

static CallbacksData *get_callbacks_data (lua_State *L)
{
    CallbacksData *data = (CallbacksData*) medit_lua_get (L, "moo-lua-callbacks");

    if (!data)
    {
        data = g_new0 (CallbacksData, 1);
        medit_lua_set (L, "moo-lua-callbacks", data, g_free);
    }

    return data;
}

extern "C" {
struct MooLuaSignalClosure
{
    GClosure base;
    lua_State *L;
    int cb_ref;
#ifdef MOO_ENABLE_COVERAGE
    char *signal_full_name;
#endif
};
} // extern "C"

static void signal_closure_finalize (G_GNUC_UNUSED gpointer dummy, GClosure *gclosure)
{
    MooLuaSignalClosure *closure = (MooLuaSignalClosure*) gclosure;

    if (closure->L)
    {
        CallbacksData *data = get_callbacks_data (closure->L);
        data->closures = g_slist_remove (data->closures, closure);
        luaL_unref (closure->L, LUA_REGISTRYINDEX, closure->cb_ref);
        medit_lua_unref (closure->L);
    }

#ifdef MOO_ENABLE_COVERAGE
    g_free (closure->signal_full_name);
#endif
}

static int
push_gvalue (lua_State *L, const GValue *value)
{
    GType type = G_VALUE_TYPE (value);
    GType type_fundamental = G_TYPE_FUNDAMENTAL (type);

    // XXX overflows

    switch (type_fundamental)
    {
        case G_TYPE_BOOLEAN:
            lua_pushboolean (L, g_value_get_boolean (value));
            return 1;

        case G_TYPE_INT:
            lua_pushnumber (L, g_value_get_int (value));
            return 1;
        case G_TYPE_UINT:
            lua_pushnumber (L, g_value_get_uint (value));
            return 1;
        case G_TYPE_LONG:
            lua_pushnumber (L, g_value_get_long (value));
            return 1;
        case G_TYPE_ULONG:
            lua_pushnumber (L, g_value_get_ulong (value));
            return 1;
        case G_TYPE_INT64:
            lua_pushnumber (L, g_value_get_int64 (value));
            return 1;
        case G_TYPE_UINT64:
            lua_pushnumber (L, g_value_get_uint64 (value));
            return 1;
        case G_TYPE_FLOAT:
            lua_pushnumber (L, g_value_get_float (value));
            return 1;
        case G_TYPE_DOUBLE:
            lua_pushnumber (L, g_value_get_double (value));
            return 1;

        case G_TYPE_ENUM:
            lua_pushinteger (L, g_value_get_enum (value));
            return 1;
        case G_TYPE_FLAGS:
            lua_pushinteger (L, g_value_get_flags (value));
            return 1;

        case G_TYPE_STRING:
            return moo_lua_push_string_copy (L, g_value_get_string (value));

        case G_TYPE_POINTER:
            return moo_lua_push_pointer (L, g_value_get_pointer (value), type, TRUE);
        case G_TYPE_BOXED:
            return moo_lua_push_boxed (L, g_value_get_boxed (value), type, TRUE);
        case G_TYPE_OBJECT:
            return moo_lua_push_object (L, (GObject*) g_value_get_object (value), TRUE);

        case G_TYPE_INTERFACE:
            {
                if (GObject *obj = (GObject*) g_value_get_object (value))
                    return moo_lua_push_object (L, obj, TRUE);
                lua_pushnil (L);
                return 1;
            }

        default:
            g_critical ("don't know how to handle value of type %s", g_type_name (type));
            return 0;
    }
}

static void
get_arg_gvalue (lua_State  *L,
                int         narg,
                const char *argname,
                GValue     *value)
{
    GType type = G_VALUE_TYPE (value);
    GType type_fundamental = G_TYPE_FUNDAMENTAL (type);

    switch (type_fundamental)
    {
        case G_TYPE_BOOLEAN:
            g_value_set_boolean (value, moo_lua_get_arg_bool (L, narg, argname));
            break;
        case G_TYPE_INT:
            g_value_set_int (value, moo_lua_get_arg_int (L, narg, argname));
            break;
        case G_TYPE_UINT:
            g_value_set_uint (value, moo_lua_get_arg_uint (L, narg, argname));
            break;
        case G_TYPE_LONG:
            g_value_set_long (value, moo_lua_get_arg_long (L, narg, argname));
            break;
        case G_TYPE_ULONG:
            g_value_set_ulong (value, moo_lua_get_arg_ulong (L, narg, argname));
            break;
        case G_TYPE_INT64:
            g_value_set_int64 (value, moo_lua_get_arg_int64 (L, narg, argname));
            break;
        case G_TYPE_UINT64:
            g_value_set_uint64 (value, moo_lua_get_arg_uint64 (L, narg, argname));
            break;
        case G_TYPE_FLOAT:
            g_value_set_float (value, moo_lua_get_arg_float (L, narg, argname));
            break;
        case G_TYPE_DOUBLE:
            g_value_set_double (value, moo_lua_get_arg_double (L, narg, argname));
            break;

        case G_TYPE_ENUM:
            g_value_set_enum (value, moo_lua_get_arg_enum (L, narg, argname, type));
            break;
        case G_TYPE_FLAGS:
            g_value_set_flags (value, moo_lua_get_arg_enum (L, narg, argname, type));
            break;

        case G_TYPE_STRING:
            g_value_set_string (value, moo_lua_get_arg_string (L, narg, argname, TRUE));
            break;

        case G_TYPE_POINTER:
            g_value_set_pointer (value, moo_lua_get_arg_instance (L, narg, argname, type, TRUE));
            break;
        case G_TYPE_BOXED:
            g_value_set_boxed (value, moo_lua_get_arg_instance (L, narg, argname, type, TRUE));
            break;
        case G_TYPE_OBJECT:
            g_value_set_object (value, moo_lua_get_arg_instance (L, narg, argname, type, TRUE));
            break;

        default:
            luaL_error (L, "don't know how to handle value of type %s", g_type_name (type));
            break;
    }
}

static int
cfunc_get_ret_gvalue (lua_State *L)
{
    GValue *value = (GValue *) lua_touserdata (L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, "moo-signal-ret-value");
    get_arg_gvalue (L, -1, "<retval>", value);
    return 0;
}

static void
get_ret_gvalue (lua_State *L, GValue *value)
{
    lua_setfield(L, LUA_REGISTRYINDEX, "moo-signal-ret-value");

    if (lua_cpcall (L, cfunc_get_ret_gvalue, value) != 0)
    {
        const char *msg = lua_tostring (L, -1);
        g_critical ("%s", msg ? msg : "ERROR");
        lua_pop (L, 1);
    }

    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "moo-signal-ret-value");
}

static void
signal_closure_marshal (MooLuaSignalClosure *closure,
                        GValue              *return_value,
                        guint                n_param_values,
                        const GValue        *param_values)
{
    lua_State *L = closure->L;

    g_return_if_fail (L != NULL);

    int old_top = lua_gettop (L);

    try
    {
        lua_rawgeti (L, LUA_REGISTRYINDEX, closure->cb_ref);

        int n_args = 0;

        for (guint i = 0; i < n_param_values; ++i)
        {
            int pushed_here = push_gvalue (L, &param_values[i]);
            if (!pushed_here)
                goto out;
            n_args += pushed_here;
        }

        // ok, pushed the arguments successfully

        if (lua_pcall (L, n_args, 1, 0) == 0)
        {
            if (return_value != NULL)
                get_ret_gvalue (L, return_value);

#ifdef MOO_ENABLE_COVERAGE
            moo_test_coverage_record ("lua", closure->signal_full_name);
#endif
        }
        else
        {
            const char *msg = lua_tostring (L, -1);
            g_critical ("%s", msg ? msg : "ERROR");
        }
    }
    catch (...)
    {
        g_critical ("oops");
    }


out:
    lua_settop (L, old_top);
}

static GClosure *signal_closure_new (lua_State *L, int cb_ref)
{
    GClosure *gclosure = g_closure_new_simple (sizeof (MooLuaSignalClosure), NULL);
    MooLuaSignalClosure *closure = (MooLuaSignalClosure*) gclosure;

    closure->L = L;
    closure->cb_ref = cb_ref;

    CallbacksData *data = get_callbacks_data (L);
    data->closures = g_slist_prepend (data->closures, closure);
    medit_lua_ref (L);

    g_closure_set_marshal (gclosure, (GClosureMarshal) signal_closure_marshal);
    g_closure_add_finalize_notifier (gclosure, NULL, signal_closure_finalize);

    return gclosure;
}

MooLuaSignalClosure *
moo_lua_get_arg_signal_closure (lua_State  *L,
                                int         narg,
                                const char *param_name)
{
    // first check it's something we can call

    switch (lua_type (L, narg))
    {
        case LUA_TLIGHTUSERDATA:
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
            break;
        default:
            moo_lua_arg_error (L, narg, param_name,
                               "expected a callable object, got %s",
                               lua_typename (L, narg));
    }

    // ref the callback
    lua_pushvalue (L, narg);
    int cb_ref = luaL_ref (L, LUA_REGISTRYINDEX);

    GClosure *closure = signal_closure_new (L, cb_ref);
    g_closure_ref (closure);
    g_closure_sink (closure);

    return (MooLuaSignalClosure*) closure;
}

gulong
moo_signal_connect_closure (gpointer             instance,
                            const char          *signal,
                            MooLuaSignalClosure *closure,
                            gboolean             after)
{
    gulong handler_id = g_signal_connect_closure (instance, signal, (GClosure*) closure, after);

    if (handler_id == 0)
        return 0;

#ifdef MOO_ENABLE_COVERAGE
    GSignalQuery query;
    g_signal_query (g_signal_lookup (signal, G_OBJECT_TYPE (instance)), &query);
    g_assert (query.signal_id != 0);
    ((MooLuaSignalClosure*) closure)->signal_full_name =
        g_strdup_printf ("%s::%s",
                         g_type_name (query.itype),
                         query.signal_name);
#endif

    return handler_id;
}


int
moo_lua_cfunc_set_property (gpointer   pself,
                            lua_State *L,
                            int        first_arg)
{
    g_return_val_if_fail (G_IS_OBJECT (pself), 0);

    const char *property = moo_lua_get_arg_string (L, first_arg, "property_name", FALSE);
    GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (pself), property);

    if (!pspec)
        luaL_error (L, "no property '%s' in object class %s",
                    property, G_OBJECT_TYPE_NAME (pself));

    if (!(pspec->flags & G_PARAM_WRITABLE))
        luaL_error (L, "property '%s' of object class '%s' is not writable",
                    property, G_OBJECT_TYPE_NAME (pself));

    GValue value = { 0 };
    g_value_init (&value, pspec->value_type);
    get_arg_gvalue (L, first_arg + 1, "value", &value);
    g_object_set_property (G_OBJECT (pself), property, &value);
    g_value_unset (&value);

    return 0;
}

namespace {

class ValueHolder
{
public:
    ValueHolder(GValue &gval)
        : m_gval(gval)
    {
    }

    ~ValueHolder()
    {
        g_value_unset (&m_gval);
    }

    ValueHolder(const ValueHolder&) = delete;
    ValueHolder& operator=(const ValueHolder&) = delete;

private:
    GValue &m_gval;
};

}

int
moo_lua_cfunc_get_property (gpointer   pself,
                            lua_State *L,
                            int        first_arg)
{
    g_return_val_if_fail (G_IS_OBJECT (pself), 0);

    const char *property = moo_lua_get_arg_string (L, first_arg, "property_name", FALSE);
    GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (pself), property);

    if (!pspec)
        luaL_error (L, "no property '%s' in object class %s",
                    property, G_OBJECT_TYPE_NAME (pself));

    if (!(pspec->flags & G_PARAM_READABLE))
        luaL_error (L, "property '%s' of object class '%s' is not readable",
                    property, G_OBJECT_TYPE_NAME (pself));

    GValue value = { 0 };
    g_value_init (&value, pspec->value_type);
    ValueHolder vh(value);
    g_object_get_property (G_OBJECT (pself), property, &value);
    return push_gvalue (L, &value);
}


int
moo_lua_push_bool (lua_State *L,
                   gboolean   value)
{
    lua_pushboolean (L, value);
    return 1;
}

int
moo_lua_push_int (lua_State *L,
                  int        value)
{
    return moo_lua_push_int64 (L, value);
}

int
moo_lua_push_int64 (lua_State *L,
                    gint64     value)
{
    lua_pushinteger (L, value);
    return 1;
}

int
moo_lua_push_uint64 (lua_State *L,
                     guint64    value)
{
    lua_pushinteger (L, value);
    return 1;
}

int
moo_lua_push_index (lua_State *L,
                    int        value)
{
    lua_pushinteger (L, value + 1);
    return 1;
}

int
moo_lua_push_string (lua_State *L,
                     char      *value)
{
    moo_lua_push_string_copy (L, value);
    g_free (value);
    return 1;
}

int
moo_lua_push_string_copy (lua_State  *L,
                          const char *value)
{
    if (!value)
        lua_pushnil (L);
    else
        lua_pushstring (L, value);
    return 1;
}

int
moo_lua_push_utf8 (lua_State *L,
                   char      *value)
{
    return moo_lua_push_string (L, value);
}

int
moo_lua_push_utf8_copy (lua_State  *L,
                        const char *value)
{
    return moo_lua_push_string_copy (L, value);
}

int
moo_lua_push_gstr (lua_State  *L,
                   const gstr &value)
{
    return moo_lua_push_string_copy (L, value.get());
}

int
moo_lua_push_filename (lua_State *L,
                       char      *value)
{
    // XXX
    return moo_lua_push_string (L, value);
}

int
moo_lua_push_filename_copy (lua_State  *L,
                            const char *value)
{
    // XXX
    return moo_lua_push_string_copy (L, value);
}

int
moo_lua_push_gunichar (lua_State *L,
                       gunichar   value)
{
    char buf[12];

    if (!value)
    {
        lua_pushnil (L);
        return 1;
    }

    buf[g_unichar_to_utf8 (value, buf)] = 0;
    lua_pushstring (L, buf);
    return 1;
}

int
moo_lua_push_object_array (lua_State      *L,
                           MooObjectArray *value,
                           gboolean        make_copy)
{
    if (!value)
    {
        lua_pushnil (L);
        return 1;
    }

    lua_createtable (L, value->n_elms, 0);

    for (guint i = 0; i < value->n_elms; ++i)
    {
        // table[i+1] = ar[i]
        moo_lua_push_object (L, value->elms[i], TRUE);
        lua_rawseti(L, -2, i + 1);
    }

    if (!make_copy)
        moo_object_array_free (value);

    return 1;
}

int
moo_lua_push_strv (lua_State  *L,
                   char      **value)
{
    if (!value)
    {
        lua_pushnil (L);
        return 1;
    }

    guint n_elms = g_strv_length (value);
    lua_createtable (L, n_elms, 0);

    for (guint i = 0; i < n_elms; ++i)
    {
        // table[i+1] = ar[i]
        moo_lua_push_string_copy (L, value[i]);
        lua_rawseti(L, -2, i + 1);
    }

    g_strfreev (value);
    return 1;
}

int
moo_lua_push_error (lua_State *L,
                    GError    *error)
{
    if (error)
    {
        lua_pushstring (L, error->message);
        lua_pushinteger (L, error->domain);
        lua_pushinteger (L, error->code);
        g_error_free (error);
        return 3;
    }
    else
    {
        return 0;
    }
}


gpointer
moo_lua_get_arg_instance_opt (lua_State  *L,
                              int         narg,
                              const char *param_name,
                              GType       type,
                              gboolean    null_ok)
{
    CHECK_STACK (L);

    if (lua_isnone (L, narg))
        return NULL;

    if (lua_isnil (L, narg))
    {
        if (null_ok)
            return NULL;
        else
            moo_lua_arg_error (L, narg, param_name,
                               "expected %s, got nil",
                               g_type_name (type));
    }

    LGInstance *lg = get_arg_instance (L, narg, param_name);

    if (!lg->instance)
        moo_lua_arg_error (L, narg, param_name, "oops");

    if ((G_TYPE_FUNDAMENTAL (lg->type) != G_TYPE_OBJECT && lg->type != type) ||
        (G_TYPE_FUNDAMENTAL (lg->type) == G_TYPE_OBJECT && !g_type_is_a (lg->type, type)))
            moo_lua_arg_error (L, narg, param_name,
                               "expected %s%s, got %s",
                               g_type_name (type),
                               null_ok ? " or nil" : "",
                               g_type_name (lg->type));

    return lg->instance;
}

gpointer
moo_lua_get_arg_instance (lua_State  *L,
                          int         narg,
                          const char *param_name,
                          GType       type,
                          gboolean    null_ok)
{
    CHECK_STACK (L);
    luaL_checkany (L, narg);
    return moo_lua_get_arg_instance_opt (L, narg, param_name, type, null_ok);
}

MooObjectArray *
moo_lua_get_arg_object_array (lua_State  *L,
                              int         narg,
                              const char *param_name,
                              GType       type)
{
    CHECK_STACK (L);

    if (!lua_istable (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "table expected, got %s",
                           luaL_typename (L, narg));

    std::vector<gpointer> vec;
    size_t len = lua_objlen(L, narg);

    lua_pushnil(L);
    while (lua_next(L, narg) != 0)
    {
        if (!lua_isnumber (L, -2))
            moo_lua_arg_error (L, narg, param_name,
                               "list expected, got dict");

        gpointer instance = moo_lua_get_arg_instance (L, -1, NULL, type, FALSE);

        int idx = luaL_checkint (L, -2);
        if (idx <= 0 || idx > (int) len)
            moo_lua_arg_error (L, narg, param_name,
                               "list expected, got dict");

        if ((int) vec.size () < idx)
            vec.resize (idx);

        vec[idx - 1] = instance;

        lua_pop (L, 1);
    }

    MooObjectArray *array = moo_object_array_new ();
    for (int i = 0, c = (int) vec.size(); i < c; ++i)
        moo_object_array_append (array, G_OBJECT (vec[i]));

    return array;
}

char **
moo_lua_get_arg_strv_opt (lua_State  *L,
                          int         narg,
                          const char *param_name,
                          gboolean    null_ok)
{
    CHECK_STACK (L);

    if (lua_isnone (L, narg))
        return NULL;
    else
        return moo_lua_get_arg_strv (L, narg, param_name, null_ok);
}

char **
moo_lua_get_arg_strv (lua_State  *L,
                      int         narg,
                      const char *param_name,
                      gboolean    null_ok)
{
    CHECK_STACK (L);

    if (lua_isnil (L, narg) && null_ok)
        return NULL;

    if (!lua_istable (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "table expected, got %s",
                           luaL_typename (L, narg));

    std::vector<std::string> vec;
    size_t len = lua_objlen(L, narg);

    lua_pushnil(L);
    while (lua_next(L, narg) != 0)
    {
        if (!lua_isnumber (L, -2))
            moo_lua_arg_error (L, narg, param_name,
                               "list expected, got dict");

        const char *s = moo_lua_get_arg_string (L, -1, NULL, FALSE);

        int idx = luaL_checkint (L, -2);
        if (idx <= 0 || idx > (int) len)
            moo_lua_arg_error (L, narg, param_name,
                               "list expected, got dict");

        if ((int) vec.size () < idx)
            vec.resize (idx);

        vec[idx - 1] = s;

        lua_pop (L, 1);
    }

    char **strv = g_new (char*, vec.size() + 1);
    for (int i = 0, c = (int) vec.size(); i < c; ++i)
        strv[i] = g_strdup (vec[i].c_str());
    strv[vec.size()] = NULL;

    return strv;
}

gboolean
moo_lua_get_arg_bool_opt (lua_State  *L,
                          int         narg,
                          const char *param_name,
                          gboolean    default_value)
{
    CHECK_STACK (L);

    if (lua_isnoneornil (L, narg))
        return default_value;

    if (!lua_isboolean (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "boolean or nil expected, got %s",
                           luaL_typename (L, narg));

    return lua_toboolean (L, narg);
}

gboolean
moo_lua_get_arg_bool (lua_State   *L,
                      int          narg,
                      const char  *param_name)
{
    CHECK_STACK (L);
    luaL_checkany (L, narg);
    return moo_lua_get_arg_bool_opt (L, narg, param_name, FALSE);
}

gint64
moo_lua_get_arg_int64 (lua_State  *L,
                       int         narg,
                       const char *param_name)
{
    CHECK_STACK (L);

    if (!lua_isnumber (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "number expected, got %s",
                           luaL_typename (L, narg));

    // XXX overflow
    return lua_tointeger (L, narg);
}

guint64
moo_lua_get_arg_uint64 (lua_State  *L,
                        int         narg,
                        const char *param_name)
{
    CHECK_STACK (L);

    if (!lua_isnumber (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "number expected, got %s",
                           luaL_typename (L, narg));

    // XXX overflow
    return lua_tointeger (L, narg);
}

int
moo_lua_get_arg_int_opt (lua_State  *L,
                         int         narg,
                         G_GNUC_UNUSED const char *param_name,
                         int         default_value)
{
    CHECK_STACK (L);

    if (lua_isnoneornil (L, narg))
        return default_value;

    // XXX  overflow
    return moo_lua_get_arg_int64 (L, narg, param_name);
}

static void
check_not_none_or_nil (lua_State *L,
                       int        narg)
{
    if (lua_type (L, narg) == LUA_TNONE)
        luaL_argerror (L, narg, "value expected");
    if (lua_type (L, narg) == LUA_TNIL)
        luaL_argerror (L, narg, "non-nil value expected");
}

int
moo_lua_get_arg_int (lua_State  *L,
                     int         narg,
                     const char *param_name)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    // XXX  overflow
    return moo_lua_get_arg_int64 (L, narg, param_name);
}

guint
moo_lua_get_arg_uint (lua_State  *L,
                      int         narg,
                      const char *param_name)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    // XXX  overflow
    return moo_lua_get_arg_uint64 (L, narg, param_name);
}

long
moo_lua_get_arg_long (lua_State  *L,
                      int         narg,
                      const char *param_name)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    // XXX  overflow
    return moo_lua_get_arg_int64 (L, narg, param_name);
}

gulong
moo_lua_get_arg_ulong (lua_State  *L,
                       int         narg,
                       const char *param_name)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    // XXX  overflow
    return moo_lua_get_arg_uint64 (L, narg, param_name);
}

double
moo_lua_get_arg_double (lua_State  *L,
                        int         narg,
                        const char *param_name)
{
    CHECK_STACK (L);

    check_not_none_or_nil (L, narg);

    if (!lua_isnumber (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "number expected, got %s",
                           luaL_typename (L, narg));

    return lua_tonumber (L, narg);
}

double
moo_lua_get_arg_double_opt (lua_State  *L,
                            int         narg,
                            const char *param_name,
                            double      default_value)
{
    CHECK_STACK (L);

    if (lua_isnoneornil (L, narg))
        return default_value;

    return moo_lua_get_arg_double (L, narg, param_name);
}

float
moo_lua_get_arg_float (lua_State  *L,
                       int         narg,
                       const char *param_name)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    // XXX overflow
    return moo_lua_get_arg_double (L, narg, param_name);
}

int
moo_lua_get_arg_index (lua_State  *L,
                       int         narg,
                       const char *param_name)
{
    CHECK_STACK (L);
    // XXX overflow
    return moo_lua_get_arg_int (L, narg, param_name) - 1;
}

int
moo_lua_get_arg_index_opt (lua_State  *L,
                           int         narg,
                           const char *param_name,
                           int         default_value)
{
    CHECK_STACK (L);

    if (lua_isnoneornil (L, narg))
        return default_value;
    else
        return moo_lua_get_arg_index (L, narg, param_name);
}

void
moo_lua_get_arg_iter (lua_State     *L,
                      int            narg,
                      const char    *param_name,
                      GtkTextBuffer *buffer,
                      GtkTextIter   *iter)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    moo_lua_get_arg_iter_opt (L, narg, param_name, buffer, iter);
}

gboolean
moo_lua_get_arg_iter_opt (lua_State     *L,
                          int            narg,
                          const char    *param_name,
                          GtkTextBuffer *buffer,
                          GtkTextIter   *iter)
{
    CHECK_STACK (L);

    GtkTextIter *iter_here;

    if (lua_isnoneornil (L, narg))
        return FALSE;

    if (lua_isnumber (L, narg))
    {
        if (!buffer)
            moo_lua_arg_error (L, narg, param_name,
                               "could not convert integer to iterator without a buffer instance");

        int pos = lua_tointeger (L, narg);
        if (pos <= 0 || pos > gtk_text_buffer_get_char_count (buffer) + 1)
            moo_lua_arg_error (L, narg, param_name,
                               "invalid position %d", pos);

        gtk_text_buffer_get_iter_at_offset (buffer, iter, pos - 1);
        return TRUE;
    }

    iter_here = (GtkTextIter*) moo_lua_get_arg_instance (L, narg, param_name, GTK_TYPE_TEXT_ITER, FALSE);

    if (!iter_here)
        moo_lua_arg_error (L, narg, param_name, "oops");

    *iter = *iter_here;
    return TRUE;
}

void
moo_lua_get_arg_rect (lua_State    *L,
                      int           narg,
                      const char   *param_name,
                      GdkRectangle *rect)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    moo_lua_get_arg_rect_opt (L, narg, param_name, rect);
}

gboolean
moo_lua_get_arg_rect_opt (lua_State    *L,
                          int           narg,
                          const char   *param_name,
                          GdkRectangle *prect)
{
    CHECK_STACK (L);

    if (lua_isnoneornil (L, narg))
        return FALSE;

    // XXX

    GdkRectangle *rect = (GdkRectangle*) moo_lua_get_arg_instance (L, narg, param_name, GDK_TYPE_RECTANGLE, FALSE);

    if (!rect)
        moo_lua_arg_error (L, narg, param_name, "oops");

    *prect = *rect;
    return TRUE;
}

static int
parse_enum (const char *string,
            GType       type,
            lua_State  *L,
            int         narg,
            const char *param_name)
{
    GEnumValue *value;
    GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_peek (type));

    value = g_enum_get_value_by_nick (enum_class, string);
    if (!value)
        value = g_enum_get_value_by_name (enum_class, string);

    if (!value)
        moo_lua_arg_error (L, narg, param_name,
                           "could not convert string '%s' to a value of type %s",
                           string, g_type_name (type));

    return value->value;
}

static int
parse_flags (const char *string,
             GType       type,
             lua_State  *L,
             int         narg,
             const char *param_name)
{
    char **pieces, **p;
    GEnumValue *value;
    GEnumClass *enum_class = G_ENUM_CLASS (g_type_class_peek (type));
    int ret = 0;

    pieces = g_strsplit_set (string, ",;|", 0);

    for (p = pieces; p && *p; ++p)
    {
        value = g_enum_get_value_by_nick (enum_class, string);
        if (!value)
            value = g_enum_get_value_by_name (enum_class, string);

        if (!value)
        {
            g_strfreev (pieces);
            moo_lua_arg_error (L, narg, param_name,
                               "could not convert string '%s' to a value of type %s",
                               string, g_type_name (type));
        }

        ret |= value->value;
    }

    g_strfreev (pieces);
    return ret;
}

static int
moo_lua_get_arg_enum_from_string (lua_State  *L,
                                  int         narg,
                                  const char *param_name,
                                  GType       type)
{
    CHECK_STACK (L);

    const char *s = lua_tostring (L, narg);

    if (!s)
        moo_lua_arg_error (L, narg, param_name, "null string unexpected");

    if (g_type_is_a (type, G_TYPE_ENUM))
        return parse_enum (s, type, L, narg, param_name);
    else
        return parse_flags (s, type, L, narg, param_name);
}

int
moo_lua_get_arg_enum_opt (lua_State  *L,
                          int         narg,
                          const char *param_name,
                          GType       type,
                          int         default_value)
{
    CHECK_STACK (L);

    if (lua_isnoneornil (L, narg))
        return default_value;

    if (lua_isnumber (L, narg))
        return lua_tointeger (L, narg);

    if (lua_isstring (L, narg))
        return moo_lua_get_arg_enum_from_string (L, narg, param_name, type);

    moo_lua_arg_error (L, narg, param_name,
                       "expected string or integer, got %s",
                       luaL_typename (L, narg));
    return 0;
}

int
moo_lua_get_arg_enum (lua_State  *L,
                      int         narg,
                      const char *param_name,
                      GType       type)
{
    CHECK_STACK (L);
    check_not_none_or_nil (L, narg);
    return moo_lua_get_arg_enum_opt (L, narg, param_name, type, 0);
}

const char *
moo_lua_get_arg_string (lua_State  *L,
                        int         narg,
                        const char *param_name,
                        gboolean    null_ok)
{
    CHECK_STACK (L);

    luaL_checkany (L, narg);

    if (lua_isnil (L, narg))
    {
        if (null_ok)
            return NULL;
        else
            moo_lua_arg_error (L, narg, param_name,
                               "expected string, got nil");
    }

    if (!lua_isstring (L, narg))
        moo_lua_arg_error (L, narg, param_name,
                           "expected string%s, got %s",
                           null_ok ? " or nil" : "",
                           luaL_typename (L, narg));

    return lua_tostring (L, narg);
}

const char *
moo_lua_get_arg_string_opt (lua_State  *L,
                            int         narg,
                            const char *param_name,
                            const char *default_value,
                            gboolean    null_ok)
{
    CHECK_STACK (L);

    if (lua_isnone (L, narg))
        return default_value;
    else
        return moo_lua_get_arg_string (L, narg, param_name, null_ok);
}

const char *
moo_lua_get_arg_utf8_opt (lua_State  *L,
                          int         narg,
                          const char *param_name,
                          const char *default_value,
                          gboolean    null_ok)
{
    CHECK_STACK (L);
    return moo_lua_get_arg_string_opt (L, narg, param_name, default_value, null_ok);
}

const char *
moo_lua_get_arg_utf8 (lua_State  *L,
                      int         narg,
                      const char *param_name,
                      gboolean    null_ok)
{
    CHECK_STACK (L);
    return moo_lua_get_arg_string (L, narg, param_name, null_ok);
}

const char *
moo_lua_get_arg_filename_opt (lua_State  *L,
                              int         narg,
                              const char *param_name,
                              const char *default_value,
                              gboolean    null_ok)
{
    CHECK_STACK (L);
    return moo_lua_get_arg_string_opt (L, narg, param_name, default_value, null_ok);
}

const char *
moo_lua_get_arg_filename (lua_State  *L,
                          int         narg,
                          const char *param_name,
                          gboolean    null_ok)
{
    CHECK_STACK (L);
    return moo_lua_get_arg_string (L, narg, param_name, null_ok);
}
