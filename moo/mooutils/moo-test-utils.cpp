/*
 *   moo-test-utils.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "moo-test-macros.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-messages.h"
#include "moocpp/gstr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

struct TestAssertInfo
{
    gstr text;
    gstr file;
    int line;

    TestAssertInfo(const char* text,
                   const char* file,
                   int line)
        : text(text)
        , file(file)
        , line(line)
    {
    }
};

struct MooTest
{
    gstr name;
    gstr description;
    MooTestFunc func;
    gpointer data;
    std::vector<TestAssertInfo> failed_asserts;
};

struct MooTestSuite
{
    gstr name;
    gstr description;
    std::vector<MooTest> tests;
    MooTestSuiteInit init_func;
    MooTestSuiteCleanup cleanup_func;
    gpointer data;
};

struct MooTestRegistry
{
    gstr data_dir;
    std::vector<MooTestSuite> test_suites;
    MooTestSuite *current_suite;
    MooTest *current_test;
    struct TestRun {
        guint suites;
        guint tests;
        guint asserts;
        guint suites_passed;
        guint tests_passed;
        guint asserts_passed;
    } tr;
};

static MooTestRegistry registry;

MooTestSuite&
moo_test_suite_new(const char         *name,
                   const char         *description,
                   MooTestSuiteInit    init_func,
                   MooTestSuiteCleanup cleanup_func,
                   gpointer            data)
{
    MooTestSuite ts;
    ts.name.copy(name);
    ts.description.copy(description);
    ts.init_func = init_func;
    ts.cleanup_func = cleanup_func;
    ts.data = data;

    registry.test_suites.push_back(std::move(ts));
    return registry.test_suites.back();
}

void
moo_test_suite_add_test(MooTestSuite &ts,
                        const char   *name,
                        const char   *description,
                        MooTestFunc   test_func,
                        gpointer      data)
{
    g_return_if_fail(name != NULL);
    g_return_if_fail(test_func != NULL);

    MooTest test;
    test.name.copy(name);
    test.description.copy(description);
    test.func = test_func;
    test.data = data;

    ts.tests.push_back(std::move(test));
}

static gboolean
run_test(MooTest        &test,
         MooTestSuite   &ts,
         MooTestOptions  opts)
{
    gboolean failed;

    if (opts & MOO_TEST_LIST_ONLY)
    {
        fprintf (stdout, "  Test: %s - %s\n", test.name.get(), test.description.get());
        return TRUE;
    }

    MooTestEnv env;
    env.suite_data = ts.data;
    env.test_data = test.data;

    fprintf(stdout, "  Test: %s ... ", test.name.get());
    fflush(stdout);

    registry.current_test = &test;
    test.func(&env);
    registry.current_test = NULL;

    failed = !test.failed_asserts.empty();

    if (failed)
        fprintf (stdout, "FAILED\n");
    else
        fprintf (stdout, "passed\n");

    if (!test.failed_asserts.empty())
    {
        int count = 1;
        for (const auto& ai: test.failed_asserts)
        {
            fprintf (stdout, "    %d. %s", count, !ai.file.empty() ? ai.file.get() : "<unknown>");
            if (ai.line > -1)
                fprintf (stdout, ":%d", ai.line);
            fprintf (stdout, " - %s\n", !ai.text.empty() ? ai.text.get() : "FAILED");
            ++count;
        }
    }

    registry.tr.tests += 1;
    if (!failed)
        registry.tr.tests_passed += 1;

    return !failed;
}

static void
run_suite (MooTestSuite   &ts,
           MooTest        *single_test,
           MooTestOptions  opts)
{
    gboolean run = !(opts & MOO_TEST_LIST_ONLY);
    gboolean passed = TRUE;

    if (run && ts.init_func && !ts.init_func(ts.data))
        return;

    registry.current_suite = &ts;

    g_print ("Suite: %s\n", ts.name.get());

    if (single_test)
    {
        passed = run_test(*single_test, ts, opts) && passed;
    }
    else
    {
        for (auto& t: ts.tests)
            passed = run_test(t, ts, opts) && passed;
    }

    if (run && ts.cleanup_func)
        ts.cleanup_func(ts.data);

    registry.current_suite = NULL;
    registry.tr.suites += 1;
    if (passed)
        registry.tr.suites_passed += 1;
}

static gboolean
find_test (const gstr&    name,
           MooTestSuite **ts_p,
           MooTest      **test_p)
{
    *ts_p = nullptr;
    *test_p = nullptr;

    std::vector<gstr> pieces = name.split("/", 2);
    g_return_val_if_fail (!pieces.empty(), FALSE);

    const gstr& suite_name = pieces[0];
    const gstr& test_name = pieces.size() > 1 ? pieces[1] : gstr();

    for (auto& ts: registry.test_suites)
    {
        if (ts.name == suite_name)
        {
            if (test_name.empty())
            {
                *ts_p = &ts;
                *test_p = NULL;
                return true;
            }

            for (auto& test : ts.tests)
            {
                if (test.name == test_name)
                {
                    *ts_p = &ts;
                    *test_p = &test;
                    return true;
                }
            }

            break;
        }
    }

    return false;
}

gboolean
moo_test_run_tests (const gstrvec&  tests,
                    const char     *coverage_file,
                    MooTestOptions  opts)
{
    if (coverage_file)
        moo_test_coverage_enable ();

    fprintf (stdout, "\n");

    if (!tests.empty())
    {
        for (const auto& name: tests)
        {
            MooTestSuite *single_ts = NULL;
            MooTest *single_test = NULL;

            if (!find_test (name, &single_ts, &single_test))
            {
                g_printerr ("could not find test %s", name.get());
                exit (EXIT_FAILURE);
            }

            run_suite (*single_ts, single_test, opts);
        }
    }
    else
    {
        for (auto& ts: registry.test_suites)
            run_suite(ts, nullptr, opts);
    }

    fprintf (stdout, "\n");

    if (!(opts & MOO_TEST_LIST_ONLY))
    {
        fprintf (stdout, "Run Summary: Type      Total      Ran  Passed  Failed\n");
        fprintf (stdout, "             suites    %5d     %4d  %6d  %6d\n",
                 registry.tr.suites, registry.tr.suites, registry.tr.suites_passed,
                 registry.tr.suites - registry.tr.suites_passed);
        fprintf (stdout, "             tests     %5d     %4d  %6d  %6d\n",
                 registry.tr.tests, registry.tr.tests, registry.tr.tests_passed,
                 registry.tr.tests - registry.tr.tests_passed);
        fprintf (stdout, "             asserts   %5d     %4d  %6d  %6d\n",
                 registry.tr.asserts, registry.tr.asserts, registry.tr.asserts_passed,
                 registry.tr.asserts - registry.tr.asserts_passed);
        fprintf (stdout, "\n");

        if (coverage_file)
            moo_test_coverage_write (coverage_file);
    }

    return moo_test_get_result ();
}

void
moo_test_cleanup (void)
{
    GError *error = NULL;

    registry.test_suites.clear();
    registry.data_dir.clear();

    if (g_file_test (moo_test_get_working_dir (), G_FILE_TEST_IS_DIR) &&
        !_moo_remove_dir (moo_test_get_working_dir (), TRUE, &error))
    {
        g_critical ("could not remove directory '%s': %s",
                    moo_test_get_working_dir (), error->message);
        g_error_free (error);
    }
}

gboolean
moo_test_get_result (void)
{
    return registry.tr.asserts_passed == registry.tr.asserts;
}


/**
 * moo_test_assert_impl: (moo.private 1)
 *
 * @passed:
 * @text: (type const-utf8)
 * @file: (type const-utf8) (allow-none) (default NULL)
 * @line: (default -1)
 **/
void
moo_test_assert_impl (gboolean    passed,
                      const char *text,
                      const char *file,
                      int         line)
{
    g_return_if_fail (registry.current_test != NULL);

    registry.tr.asserts += 1;
    if (passed)
        registry.tr.asserts_passed += 1;
    else
        registry.current_test->failed_asserts.push_back(TestAssertInfo(text, file, line));
}

void
moo_test_assert_msgv (gboolean    passed,
                      const char *file,
                      int         line,
                      const char *format,
                      va_list     args)
{
    char *text = NULL;

    if (format)
        text = g_strdup_vprintf (format, args);

    moo_test_assert_impl (passed, text, file, line);

    g_free (text);
}

void
moo_test_assert_msg (gboolean    passed,
                     const char *file,
                     int         line,
                     const char *format,
                     ...)
{
    va_list args;
    va_start (args, format);
    moo_test_assert_msgv (passed, file, line, format, args);
    va_end (args);
}


const gstr&
moo_test_get_data_dir (void)
{
    return registry.data_dir;
}

void
moo_test_set_data_dir (const char *dir)
{
    g_return_if_fail (dir != NULL);

    if (!g_file_test (dir, G_FILE_TEST_IS_DIR))
    {
        g_critical ("not a directory: %s", dir);
        return;
    }

    registry.data_dir.steal(_moo_normalize_file_path(dir));
}

const char *
moo_test_get_working_dir (void)
{
    return "test-working-dir";
}

gstr
moo_test_find_data_file (const char *basename)
{
    g_return_val_if_fail(!registry.data_dir.empty(), gstr());

    if (!_moo_path_is_absolute(basename))
        return gstr::take(g_build_filename(registry.data_dir.get(), basename, nullptr));
    else
        return gstr(basename);
}

char **
moo_test_list_data_files (const char *dir)
{
    GDir *gdir;
    gstr tmp;
    GError *error = NULL;
    const char *name;
    GPtrArray *names = NULL;

    if (!_moo_path_is_absolute (dir))
    {
        g_return_val_if_fail (registry.data_dir != nullptr, NULL);
        tmp.steal(g_build_filename(registry.data_dir.get(), dir, nullptr));
        dir = tmp.get();
    }

    if (!(gdir = g_dir_open (dir, 0, &error)))
    {
        g_warning ("could not open directory '%s': %s",
                   dir, moo_error_message (error));
        g_error_free (error);
        error = NULL;
    }

    names = g_ptr_array_new ();
    while (gdir && (name = g_dir_read_name (gdir)))
        g_ptr_array_add (names, g_strdup (name));
    g_ptr_array_add (names, NULL);

    if (gdir)
        g_dir_close (gdir);

    return (char**) g_ptr_array_free (names, FALSE);
}

char *
moo_test_load_data_file (const char *basename)
{
    char *fullname;
    char *contents = NULL;
    GError *error = NULL;

    g_return_val_if_fail (registry.data_dir != nullptr, NULL);

    if (!_moo_path_is_absolute (basename))
        fullname = g_build_filename (registry.data_dir.get(), basename, nullptr);
    else
        fullname = g_strdup (basename);

    if (!g_file_get_contents (fullname, &contents, NULL, &error))
    {
        TEST_FAILED_MSG ("could not open file `%s': %s",
                         fullname, error->message);
        g_error_free (error);
    }

    g_free (fullname);
    return contents;
}


/************************************************************************************
 * coverage
 */

static GHashTable *called_functions = NULL;

void
moo_test_coverage_enable (void)
{
    g_return_if_fail (called_functions == NULL);
    called_functions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
add_func (gpointer key,
          G_GNUC_UNUSED gpointer value,
          GString *string)
{
    g_string_append (string, (const char*) key);
    g_string_append (string, "\n");
}

void
moo_test_coverage_write (const char *filename)
{
    GString *content;
    GError *error = NULL;

    g_return_if_fail (called_functions != NULL);
    g_return_if_fail (filename != NULL);

    content = g_string_new (NULL);
    g_hash_table_foreach (called_functions, (GHFunc) add_func, content);

    if (!g_file_set_contents (filename, content->str, -1, &error))
    {
        g_critical ("could not save file %s: %s", filename, moo_error_message (error));
        g_error_free (error);
    }

    g_string_free (content, TRUE);
    g_hash_table_destroy (called_functions);
    called_functions = NULL;
}

void
moo_test_coverage_record (const char *lang,
                          const char *function)
{
    if (G_UNLIKELY (called_functions))
        g_hash_table_insert (called_functions,
                             g_strdup_printf ("%s.%s", lang, function),
                             GINT_TO_POINTER (TRUE));
}


/************************************************************************************
 * log handlers
 */

static gboolean silent_messages;

static void
silent_log_handler (const gchar    *log_domain,
                    GLogLevelFlags  log_level,
                    const gchar    *message,
                    gpointer        data)
{
    if (log_level & G_LOG_LEVEL_ERROR)
        g_log_default_handler (log_domain, log_level, message, data);
}

/**
 * moo_test_set_silent_messages: (moo.private 1)
 **/
gboolean
moo_test_set_silent_messages (gboolean silent)
{
    if (silent == silent_messages)
        return silent_messages;

    if (silent)
        g_log_set_default_handler (silent_log_handler, NULL);
    else
        g_log_set_default_handler (g_log_default_handler, NULL);

    silent_messages = silent;
    return !silent;
}
