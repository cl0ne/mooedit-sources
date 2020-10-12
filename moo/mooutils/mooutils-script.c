/*
 *   mooutils-script.c
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

#include "config.h"
#include "mooutils-script.h"
#include "mooutils-fs.h"
#include "mooutils-debug.h"
#include "mooutils.h"
#include <stdlib.h>

static char *moo_temp_dir;

static void     moo_install_atexit      (void);
static void     moo_remove_tempdir      (void);

static gboolean
quit_main_loop (GMainLoop *main_loop)
{
    g_main_loop_quit (main_loop);
    return FALSE;
}

/**
 * moo_spin_main_loop:
 **/
void
moo_spin_main_loop (double sec)
{
    GMainLoop *main_loop;

    g_return_if_fail (sec > 0);

    main_loop = g_main_loop_new (NULL, FALSE);
    g_timeout_add (sec * 1000, (GSourceFunc) quit_main_loop, main_loop);

    g_main_loop_run (main_loop);

    g_main_loop_unref (main_loop);
}


/**
 * moo_tempdir:
 *
 * Return the path of &medit; temporary file directory. This directory
 * is created automatically and is removed on exit. It is unique among
 * all &medit; instances.
 *
 * Returns: (type filename)
 **/
char *
moo_tempdir (void)
{
    MOO_DO_ONCE_BEGIN
    {
        int i;
        char *dirname = NULL;
        const char *short_name;

        moo_assert (!moo_temp_dir);

        short_name = MOO_PACKAGE_NAME;

        for (i = 0; i < 1000; ++i)
        {
            char *basename;
            mgw_errno_t err;

            basename = g_strdup_printf ("%s-tmpdir-%08x", short_name, g_random_int ());
            dirname = g_build_filename (g_get_tmp_dir (), basename, NULL);
            g_free (basename);

            if (_moo_mkdir (dirname, &err) == 0)
                break;

            g_free (dirname);
            dirname = NULL;
        }

        moo_temp_dir = dirname;
        moo_install_atexit ();
    }
    MOO_DO_ONCE_END

    g_return_val_if_fail (moo_temp_dir != NULL, NULL);
    return g_strdup (moo_temp_dir);
}

/**
 * moo_tempnam:
 *
 * Generate a unique filename for a temporary file. Generated filename
 * is located inside directory returned by moo_tempdir(), and it
 * will be automatically removed on exit.
 *
 * @extension: (type const-filename) (allow-none) (default NULL)
 *
 * Returns: (type filename)
 **/
char *
moo_tempnam (const char *extension)
{
    int i;
    char *tmpdir;
    char *filename = NULL;
    static int counter;
    G_LOCK_DEFINE_STATIC (counter);

    tmpdir = moo_tempdir ();
    g_return_val_if_fail (tmpdir != NULL, NULL);

    G_LOCK (counter);

    for (i = counter + 1; i < counter + 1000; ++i)
    {
        char *basename;

        basename = g_strdup_printf ("tmpfile-%03d%s", i, MOO_NZS (extension));
        filename = g_build_filename (tmpdir, basename, NULL);
        g_free (basename);

        if (!g_file_test (filename, G_FILE_TEST_EXISTS))
            break;

        g_free (filename);
        filename = NULL;
    }

    counter = i;

    G_UNLOCK (counter);

    g_free (tmpdir);

    g_return_val_if_fail (filename != NULL, NULL);
    return filename;
}

static void
moo_remove_tempdir (void)
{
    if (moo_temp_dir)
    {
        GError *error = NULL;
        _moo_remove_dir (moo_temp_dir, TRUE, &error);

        if (error)
        {
            _moo_message ("%s", error->message);
            g_error_free (error);
        }

        g_free (moo_temp_dir);
        moo_temp_dir = NULL;
    }
}

static void
moo_atexit_handler (void)
{
    moo_remove_tempdir ();
}

static void
moo_install_atexit (void)
{
    static gboolean installed;

    if (!installed)
    {
        atexit (moo_atexit_handler);
        installed = TRUE;
    }
}

void
moo_cleanup (void)
{
    moo_remove_tempdir ();
}
