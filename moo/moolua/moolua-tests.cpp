#include "moolua-tests.h"
#include "moolua/lua/moolua.h"
#include "mooutils/moo-test-macros.h"
#include "moolua/lua/lua.h"
#include "moolua/lua/lauxlib.h"
#include "medit-lua.h"

typedef struct {
    char **files;
} MooLuaTestData;

static void
moo_test_run_lua_script (lua_State  *L,
                         const char *script,
                         const char *filename)
{
    int ret;

    if (lua_gettop (L) != 0)
    {
        TEST_FAILED_MSG ("before running script `%s': Lua state is corrupted",
                         filename);
        return;
    }

    if (luaL_loadstring (L, script) != 0)
    {
        const char *msg = lua_tostring (L, -1);
        TEST_FAILED_MSG ("error loading script `%s': %s",
                         filename, msg);
        return;
    }

    if ((ret = lua_pcall (L, 0, 0, 0)) != 0)
    {
        const char *msg = lua_tostring (L, -1);

        switch (ret)
        {
            case LUA_ERRRUN:
                TEST_FAILED_MSG ("error running script `%s': %s",
                                 filename, msg ? msg : "<null>");
                break;
            case LUA_ERRMEM:
                TEST_FAILED_MSG ("error running script `%s', memory exhausted",
                                 filename);
                break;
            case LUA_ERRERR:
                TEST_FAILED_MSG ("error running script `%s', "
                                 "this should not have happened!",
                                 filename);
                break;
        }

        return;
    }
}

static void
moo_test_run_lua_file (const char *basename)
{
    char *contents;
    char *filename;

    filename = g_build_filename (moo_test_get_data_dir ().get(), "test-lua", basename, (char*) NULL);

    if ((contents = moo_test_load_data_file (filename)))
    {
        lua_State *L = medit_lua_new ();
        g_return_if_fail (L != NULL);

        g_assert (lua_gettop (L) == 0);

        {
            char *testdir = g_build_filename (moo_test_get_data_dir ().get(), "test-lua", "lua", (char*) NULL);
            lua_addpath (L, (char**) &testdir, 1);
            g_free (testdir);
        }

        g_assert (lua_gettop (L) == 0);

        moo_test_run_lua_script (L, contents, basename);
        lua_pop (L, lua_gettop (L));

        medit_lua_free (L);
    }

    g_free (contents);
    g_free (filename);
}

static void
test_func (MooTestEnv *env)
{
    moo_test_run_lua_file ((const char *) env->test_data);
}

static MooLuaTestData *
moo_lua_test_data_new (void)
{
    return g_slice_new0 (MooLuaTestData);
}

static void
moo_lua_test_data_free (gpointer udata)
{
    if (MooLuaTestData *data = (MooLuaTestData*) udata)
    {
        g_strfreev (data->files);
        g_slice_free (MooLuaTestData, data);
    }
}

void
moo_test_lua (MooTestOptions opts)
{
    MooLuaTestData *data;
    char **p;

    if (!(opts & MOO_TEST_INSTALLED))
        return;

    data = moo_lua_test_data_new ();
    MooTestSuite& suite = moo_test_suite_new("MooLua", "Lua scripting tests", NULL, moo_lua_test_data_free, data);

    data->files = moo_test_list_data_files ("test-lua");

    if (g_strv_length (data->files) == 0)
        g_critical ("no lua test files found");

    for (p = data->files; p && *p; ++p)
    {
        char *test_name;
        const char *basename = *p;
        if (!g_str_has_prefix (basename, "test") || !g_str_has_suffix (basename, ".lua"))
            continue;
        test_name = g_strndup (basename + strlen ("test"), strlen (basename) - strlen ("test") - strlen (".lua"));
        moo_test_suite_add_test (suite, test_name, basename, test_func, (char*) basename);
        g_free (test_name);
    }
}
