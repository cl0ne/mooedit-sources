/*
 *   moo-test-utils.h
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

#pragma once

#include <mooglib/moo-glib.h>
#include <string.h>
#include <stdarg.h>
#include <mooutils/mooutils-macros.h>
#include <mooutils/mooutils-misc.h>
#include <moocpp/gstr.h>

struct MooTestEnv
{
    gpointer suite_data;
    gpointer test_data;
};

typedef enum {
    MOO_TEST_LIST_ONLY   = 1 << 0,
    MOO_TEST_FATAL_ERROR = 1 << 1,
    MOO_TEST_INSTALLED   = 1 << 2
} MooTestOptions;

typedef struct MooTestSuite MooTestSuite;
typedef gboolean (*MooTestSuiteInit)    (gpointer    data);
typedef void     (*MooTestSuiteCleanup) (gpointer    data);
typedef void     (*MooTestFunc)         (MooTestEnv *env);

MooTestSuite    &moo_test_suite_new         (const char         *name,
                                             const char         *description,
                                             MooTestSuiteInit    init_func,
                                             MooTestSuiteCleanup cleanup_func,
                                             gpointer            data);
void             moo_test_suite_add_test    (MooTestSuite       &ts,
                                             const char         *name,
                                             const char         *description,
                                             MooTestFunc         test_func,
                                             gpointer            data);

gboolean         moo_test_run_tests         (const gstrvec&      tests,
                                             const char         *coverage_file,
                                             MooTestOptions      opts);
void             moo_test_cleanup           (void);
gboolean         moo_test_get_result        (void);

void             moo_test_assert_impl       (gboolean            passed,
                                             const char         *text,
                                             const char         *filename,
                                             int                 line);
void             moo_test_assert_msgv       (gboolean            passed,
                                             const char         *file,
                                             int                 line,
                                             const char         *format,
                                             va_list             args) G_GNUC_PRINTF(4, 0);
void             moo_test_assert_msg        (gboolean            passed,
                                             const char         *file,
                                             int                 line,
                                             const char         *format,
                                             ...) G_GNUC_PRINTF(4, 5);

char            *moo_test_load_data_file    (const char         *basename);
gstr             moo_test_find_data_file    (const char         *basename);
void             moo_test_set_data_dir      (const char         *dir);
const gstr&      moo_test_get_data_dir      (void);
const char      *moo_test_get_working_dir   (void);
char           **moo_test_list_data_files   (const char         *subdir);

void             moo_test_coverage_enable   (void);
void             moo_test_coverage_write    (const char         *filename);
void             moo_test_coverage_record   (const char         *lang,
                                             const char         *function);

gboolean         moo_test_set_silent_messages   (gboolean        silent);
