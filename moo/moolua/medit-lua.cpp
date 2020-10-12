#include "moolua/medit-lua.h"
#include "moolua/moo-lua-api-util.h"
#include "mooutils/mooutils.h"
#include "moolua/lua-default-init.h"
#include "mooapp/mooapp.h"

extern "C" {

/**
 * class:MooLuaState: (parent GObject) (moo.private 1) (moo.lua 0) (constructable)
 **/

typedef struct MooLuaStateClass MooLuaStateClass;

struct MooLuaState
{
    GObject base;
    lua_State *L;
};

struct MooLuaStateClass
{
    GObjectClass base_class;
};

G_DEFINE_TYPE (MooLuaState, moo_lua_state, G_TYPE_OBJECT)

static void
moo_lua_state_init (MooLuaState *lua)
{
    try
    {
        lua->L = medit_lua_new ();
    }
    catch (...)
    {
        g_critical ("oops");
    }
}

static void
moo_lua_state_finalize (GObject *object)
{
    MooLuaState *lua = (MooLuaState*) object;

    try
    {
        medit_lua_free (lua->L);
    }
    catch (...)
    {
        g_critical ("oops");
    }

    G_OBJECT_CLASS (moo_lua_state_parent_class)->finalize (object);
}

static void
moo_lua_state_class_init (MooLuaStateClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = moo_lua_state_finalize;
}

/**
 * moo_lua_state_run_string:
 *
 * @lua:
 * @string: (type const-utf8)
 *
 * Returns: (type utf8)
 **/
char *
moo_lua_state_run_string (MooLuaState *lua,
                          const char  *string)
{
    g_return_val_if_fail (lua && lua->L, g_strdup ("error:unexpected error"));
    g_return_val_if_fail (string != NULL, g_strdup ("error:unexpected error"));

    try
    {
        int old_top = lua_gettop (lua->L);

        if (luaL_dostring (lua->L, string) == 0)
        {
            GString *sret = g_string_new ("ok:");
            int nret = lua_gettop (lua->L) - old_top;
            if (nret > 1 || (nret == 1 && !lua_isnil (lua->L, -1)))
                for (int i = 1; i <= nret; ++i)
                {
                    if (i > 1)
                        g_string_append (sret, ", ");
                    if (lua_isnil (lua->L, -i))
                        g_string_append (sret, "nil");
                    else
                        g_string_append (sret, lua_tostring (lua->L, -i));
                }
            lua_pop (lua->L, nret);
            return g_string_free (sret, FALSE);
        }

        char *ret;
        const char *msg = lua_tostring (lua->L, -1);
        if (strstr (msg, "<eof>"))
            ret = g_strdup ("error:incomplete");
        else
            ret = g_strdup_printf ("error:%s", msg);
        lua_pop (lua->L, lua_gettop (lua->L) - old_top);
        return ret;
    }
    catch (...)
    {
        g_critical ("oops");
        return g_strdup ("error:unexpected error");
    }
}

} // extern "C"

void gtk_lua_api_add_to_lua (lua_State *L);
void moo_lua_api_add_to_lua (lua_State *L);

static bool
add_raw_api (lua_State *L)
{
    g_assert (lua_gettop (L) == 0);

    gtk_lua_api_add_to_lua (L);
    lua_pop(L, 1);

    g_assert (lua_gettop (L) == 0);

    moo_lua_api_add_to_lua (L);
    lua_pop(L, 1);

    g_assert (lua_gettop (L) == 0);

    return true;
}

static bool
medit_lua_setup (lua_State *L)
{
    try
    {
        g_assert (lua_gettop (L) == 0);

        if (!add_raw_api (L))
            return false;

        g_assert (lua_gettop (L) == 0);

        if (!medit_lua_do_string (L, LUA_DEFAULT_INIT))
        {
            g_critical ("oops");
            return false;
        }

        return true;
    }
    catch (...)
    {
        moo_assert_not_reached();
        return false;
    }
}

struct MeditLuaData {
    int ref_count;
};

static MeditLuaData *medit_lua_data_new (void)
{
    MeditLuaData *data = g_slice_new0 (MeditLuaData);
    data->ref_count = 1;
    return data;
}

static void medit_lua_data_free (MeditLuaData *data)
{
    if (data)
    {
        g_dataset_destroy (data);
        g_slice_free (MeditLuaData, data);
    }
}

static int
lua_panic (lua_State *L)
{
    g_error ("PANIC: unprotected error in call to Lua API (%s)\n",
             lua_tostring(L, -1));
    // g_error does not return
}

lua_State *
medit_lua_new ()
{
    lua_State *L = luaL_newstate ();
    g_return_val_if_fail (L != NULL, NULL);

    lua_atpanic (L, lua_panic);

    luaL_openlibs (L);
    moo_lua_add_user_path (L);

    MeditLuaData *data = medit_lua_data_new ();
    lua_pushlightuserdata (L, data);
    lua_setfield (L, LUA_REGISTRYINDEX, "_MEDIT_LUA_DATA");

    moo_assert (lua_gettop (L) == 0);

    if (!medit_lua_setup (L))
    {
        medit_lua_data_free (data);
        lua_close (L);
        return NULL;
    }

    moo_assert (lua_gettop (L) == 0);

    return L;
}

static MeditLuaData *
medit_lua_get_data (lua_State *L)
{
    g_return_val_if_fail (L != NULL, NULL);

    try
    {
        lua_getfield (L, LUA_REGISTRYINDEX, "_MEDIT_LUA_DATA");
        MeditLuaData *data = (MeditLuaData*) lua_touserdata (L, -1);
        lua_pop (L, 1);
        return data;
    }
    catch (...)
    {
        g_critical ("oops");
        return NULL;
    }
}

void
medit_lua_unref (lua_State *L)
{
    MeditLuaData *data = medit_lua_get_data (L);
    g_return_if_fail (data != NULL);

    if (!--data->ref_count)
    {
        lua_close (L);
        medit_lua_data_free (data);
    }
}

void
medit_lua_ref (lua_State *L)
{
    MeditLuaData *data = medit_lua_get_data (L);
    g_return_if_fail (data != NULL);
    ++data->ref_count;
}

void
medit_lua_free (lua_State *L)
{
    if (L)
        medit_lua_unref (L);
}

void
medit_lua_set (lua_State      *L,
               const char     *key,
               gpointer        data,
               GDestroyNotify  notify)
{
    g_return_if_fail (L != NULL);
    g_return_if_fail (key != NULL);

    MeditLuaData *mdata = medit_lua_get_data (L);
    g_return_if_fail (mdata != NULL);

    g_dataset_set_data_full (mdata, key, data, notify);
}

gpointer
medit_lua_get (lua_State  *L,
               const char *key)
{
    g_return_val_if_fail (L != NULL, NULL);
    g_return_val_if_fail (key != NULL, NULL);

    MeditLuaData *mdata = medit_lua_get_data (L);
    g_return_val_if_fail (mdata != NULL, NULL);

    return g_dataset_get_data (mdata, key);
}

bool
medit_lua_do_string (lua_State *L, const char *string)
{
    g_return_val_if_fail (L != NULL, FALSE);
    g_return_val_if_fail (string != NULL, FALSE);

    bool retval = true;

    if (luaL_dostring (L, string) != 0)
    {
        const char *msg = lua_tostring (L, -1);
        g_critical ("%s", msg ? msg : "ERROR");
        retval = false;
    }

    return retval;
}

bool
medit_lua_do_file (lua_State *L, const char *filename)
{
    g_return_val_if_fail (L != NULL, FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    char *content = NULL;
    GError *error = NULL;
    if (!g_file_get_contents (filename, &content, NULL, &error))
    {
        g_warning ("could not read file '%s': %s", filename, error->message);
        g_error_free (error);
        return false;
    }

    gboolean ret = medit_lua_do_string (L, content);
    g_free (content);
    return ret;
}


extern "C" void
medit_lua_run_string (const char *string)
{
    g_return_if_fail (string != NULL);
    lua_State *L = medit_lua_new ();
    if (L)
        medit_lua_do_string (L, string);
    medit_lua_free (L);
}

extern "C" void
medit_lua_run_file (const char *filename)
{
    g_return_if_fail (filename != NULL);
    lua_State *L = medit_lua_new ();
    if (L)
        medit_lua_do_file (L, filename);
    medit_lua_free (L);
}
