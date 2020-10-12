#include "moopython-utils.h"
#include "moopython-tests.h"
#include "medit-python.h"

static void
moo_test_run_python_file (const char *basename)
{
    gstr filename = moo_test_find_data_file (basename);

    if (filename.empty())
        TEST_FAILED_MSG ("could not find file `%s'", basename);
    else if (!medit_python_run_file (filename.get(), TRUE))
        TEST_FAILED_MSG ("error running file `%s'", basename);
}

static void
test_func (MooTestEnv *env)
{
    static gboolean been_here = FALSE;

    if (!been_here)
    {
        char *dir;
        been_here = TRUE;
        dir = g_build_filename (moo_test_get_data_dir ().get(), "test-python", nullptr);
        moo_python_add_path (dir);
        g_free (dir);
    }

    moo_test_run_python_file ((const char *) env->test_data);
}

static void
add_test (MooTestSuite &suite, const char *name, const char *description, const char *python_file)
{
    moo_test_suite_add_test (suite, name, description, test_func, (void*) python_file);
}

void
moo_test_python (void)
{
    if (moo_python_enabled ())
    {
        MooTestSuite& suite = moo_test_suite_new ("MooPython", "Python scripting tests", NULL, NULL, NULL);

        add_test (suite, "moo", "test of moo module", "test-python/testmoo.py");
    }
}
