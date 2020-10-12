#include <mooedit/mooeditor-tests.h>
#include <moolua/moolua-tests.h>
#include <moopython/moopython-tests.h>
#include <mooutils/mooutils-tests.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "mem-debug.h"

static void
add_tests (MooTestOptions opts)
{
    moo_test_gobject ();
    moo_test_mooaccel ();
    moo_test_mooutils_fs ();
    moo_test_moo_file_writer ();
    moo_test_mooutils_misc ();
    moo_test_i18n (opts);

#ifdef __WIN32__
    moo_test_mooutils_win32 ();
#endif

    moo_test_lua (opts);

#ifdef MOO_ENABLE_PYTHON
    moo_test_python ();
#endif

    moo_test_editor ();
}

static int
unit_tests_main(MooTestOptions opts, const gstrvec& tests, const char *data_dir_arg, const char *coverage_file)
{
    const char *data_dir = NULL;
    gboolean passed;

#ifdef MOO_UNIT_TEST_DATA_DIR
    data_dir = MOO_UNIT_TEST_DATA_DIR;
#endif

    if (data_dir_arg)
        data_dir = data_dir_arg;

    moo_test_set_data_dir (data_dir);

    add_tests (opts);

    passed = moo_test_run_tests (tests, coverage_file, opts);

    moo_test_cleanup ();

    return passed ? 0 : 1;
}

static void
list_unit_tests (const char *data_dir)
{
    unit_tests_main(MOO_TEST_LIST_ONLY, gstrvec(), data_dir, NULL);
}
