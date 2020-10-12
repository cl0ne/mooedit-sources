/*
 *   mooutils-win32.c
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

#ifndef _WIN32
#error "This is win32-only file"
#endif
#ifndef __WIN32__
#error "__WIN32__ must be defined on win32"
#endif
#ifndef UNICODE
#error "UNICODE must be defined on win32"
#endif
#ifndef _UNICODE
#error "_UNICODE must be defined on win32"
#endif
#ifndef STRICT
#error "STRICT must be defined on win32"
#endif
#ifndef HAVE_CONFIG_H
#error "HAVE_CONFIG_H must be defined on win32"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/moowin32/mingw/fnmatch.h"
#include <mooutils/mooutils-tests.h>
#include <mooglib/moo-stat.h>

#include <gdk/gdkwin32.h>
#include <windows.h>
#include <shellapi.h>
#include <time.h>
#include <sys/time.h>
#include <io.h>
#include <stdarg.h>

#if 0
#ifdef _MSC_VER
/* This is stuff from newer Microsoft C runtime, but we want msvcrt.dll
 * which doesn't have these functions */
long _ftol( double );
long _ftol2( double dblSource ) { return _ftol( dblSource ); }
long _ftol2_sse( double dblSource ) { return _ftol( dblSource ); }
#endif
#endif

#ifdef MOO_DLL
extern HINSTANCE _moo_hinst;
#else
HINSTANCE _moo_hinst = NULL;
#endif

static const char *
get_moo_dll_name (void)
{
#ifdef MOO_DLL
    G_LOCK_DEFINE_STATIC (moo_dll_name);
    static char *moo_dll_name = NULL;

    G_LOCK (moo_dll_name);

    if (!moo_dll_name)
    {
        char *name = NULL;
        wchar_t buf[MAX_PATH+1];
        if (GetModuleFileNameW ((HMODULE) _moo_hinst, buf, G_N_ELEMENTS (buf)))
            name = g_utf16_to_utf8 (buf, -1, NULL, NULL, NULL);
        if (name)
            moo_dll_name = g_path_get_basename (name);
        if (!moo_dll_name)
            moo_dll_name = g_strdup ("libmoo.dll");
        g_free (name);
    }

    G_UNLOCK (moo_dll_name);

    return moo_dll_name;
#else
    return NULL;
#endif
}

const char *
_moo_win32_get_locale_dir (void)
{
    G_LOCK_DEFINE_STATIC (moo_locale_dir);
    static char *moo_locale_dir = NULL;

    G_LOCK (moo_locale_dir);

    if (!moo_locale_dir)
    {
        char *dir, *subdir;

        dir = g_win32_get_package_installation_directory_of_module (_moo_hinst);
        subdir = g_build_filename (dir, "share", "locale", NULL);

        moo_locale_dir = g_win32_locale_filename_from_utf8 (subdir);

        g_free (subdir);
        g_free (dir);
    }

    G_UNLOCK (moo_locale_dir);

    return moo_locale_dir;
}


static void
add_win32_data_dirs_for_dll (GPtrArray  *list,
                             const char *subdir_name,
                             const char *dllname)
{
    char *dlldir, *datadir;

    dlldir = moo_win32_get_dll_dir (dllname);

    if (g_str_has_suffix (dlldir, "\\"))
    {
        char *tmp = g_strndup (dlldir, strlen(dlldir) - 1);
        g_free (dlldir);
        dlldir = tmp;
    }

    if (g_str_has_suffix (dlldir, "bin") ||
        g_str_has_suffix (dlldir, "lib"))
    {
        char *tmp = g_path_get_dirname (dlldir);
        datadir = g_build_filename (tmp, subdir_name, NULL);
        g_free (tmp);
    }
    else
    {
        datadir = g_strdup (dlldir);
    }

    g_free (dlldir);
    g_ptr_array_add (list, datadir);
}

void
_moo_win32_add_data_dirs (GPtrArray  *list,
                          const char *prefix)
{
    char *subdir;
    const char *dll_name;

    subdir = g_strdup_printf ("%s\\" MOO_PACKAGE_NAME, prefix);
    add_win32_data_dirs_for_dll (list, subdir, NULL);

    if ((dll_name = get_moo_dll_name ()))
        add_win32_data_dirs_for_dll (list, subdir, dll_name);

    g_free (subdir);
}


char *
moo_win32_get_app_dir (void)
{
    static char *moo_app_dir;
    G_LOCK_DEFINE_STATIC(moo_app_dir);

    G_LOCK (moo_app_dir);

    if (!moo_app_dir)
        moo_app_dir = moo_win32_get_dll_dir (NULL);

    G_UNLOCK (moo_app_dir);

    return g_strdup (moo_app_dir);
}

char *
moo_win32_get_dll_dir (const char *dll)
{
    wchar_t *dll_utf16 = NULL;
    char *dir;
    char *dllname = NULL;
    HMODULE handle;
    wchar_t buf[MAX_PATH+1];

    if (dll)
    {
    	GError *error = NULL;

    	dll_utf16 = (wchar_t*) g_utf8_to_utf16 (dll, -1, NULL, NULL, &error);

    	if (!dll_utf16)
    	{
            g_critical ("could not convert name '%s' to UTF16: %s",
                        dll, moo_error_message (error));
            g_error_free (error);
            return g_strdup (".");
    	}
    }

    handle = GetModuleHandleW (dll_utf16);
    g_return_val_if_fail (handle != NULL, g_strdup ("."));

    if (GetModuleFileNameW (handle, buf, G_N_ELEMENTS (buf)) > 0)
        dllname = g_utf16_to_utf8 ((const gunichar2*) buf, -1, NULL, NULL, NULL);

    if (dllname)
        dir = g_path_get_dirname (dllname);
    else
        dir = g_strdup (".");

    g_free (dllname);
    g_free (dll_utf16);
    return dir;
}


gboolean
_moo_win32_open_uri (const char *uri)
{
    HINSTANCE h;

    g_return_val_if_fail (uri != NULL, FALSE);

    h = ShellExecuteA (NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);

    if ((int)h <= 32)
    {
        char *msg = g_win32_error_message (GetLastError());
        g_warning ("could not open uri '%s': %s", uri, msg);
        g_free (msg);
        return FALSE;
    }

    return TRUE;
}


void
_moo_win32_show_fatal_error (const char *domain,
                             const char *logmsg)
{
    char *msg = NULL;

#define PLEASE_REPORT \
    "Please report it to " PACKAGE_BUGREPORT " and provide "\
    "steps needed to reproduce this error."
    if (domain)
        _moo_win32_message_box(NULL, MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND,
                               "Error", "Fatal error:\n---\n%s: %s\n---\n"
                               PLEASE_REPORT, domain, logmsg);
    else
        _moo_win32_message_box(NULL, MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND,
                               "Error", "Fatal error:\n---\n%s\n---\n"
                               PLEASE_REPORT, logmsg);
#undef PLEASE_REPORT

    g_free (msg);
}


int
_moo_win32_message_box(GtkWidget      *parent,
                       guint           type,
                       const char     *title,
                       const char     *format,
                       ...)
{
    int ret;
    char *text = NULL;
    HWND parenthwnd = NULL;
    wchar_t *wtitle = NULL;
    wchar_t *wtext = NULL;

    if (format)
    {
        va_list args;
        va_start (args, format);
        text = g_strdup_vprintf (format, args);
        va_end (args);
    }

    if (parent)
        parent = gtk_widget_get_toplevel (parent);
    if (parent)
        parenthwnd = (HWND) GDK_WINDOW_HWND (parent->window);

    if (title)
        wtitle = (wchar_t*) g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);
    if (text)
        wtext = (wchar_t*) g_utf8_to_utf16 (text, -1, NULL, NULL, NULL);

    ret = MessageBox(parenthwnd, wtext, wtitle, type);

    g_free (wtext);
    g_free (wtitle);
    g_free (text);

    return ret;
}


char **
_moo_win32_lame_parse_cmd_line (const char  *cmd_line,
                                GError     **error)
{
    char **argv;
    char *filename;

    if (!g_shell_parse_argv (cmd_line, NULL, &argv, error))
        return NULL;

    if (!(filename = g_find_program_in_path (argv[0])))
    {
        guint len = g_strv_length (argv);
        argv = g_renew (char*, argv, len + 3);
        memmove (argv + 2, argv, (len + 1) * sizeof (*argv));
        argv[0] = g_strdup ("cmd.exe");
        argv[1] = g_strdup ("/c");
    }

    g_free (filename);
    return argv;
}


#ifdef MOO_NEED_GETTIMEOFDAY
#include <mooutils/moowin32/ms/sys/time.h>
int
_moo_win32_gettimeofday (struct timeval *tp,
                         G_GNUC_UNUSED gpointer tzp)
{
    time_t sec;

    if (tp == NULL || tzp != NULL)
        return -1;

    sec = time (NULL);

    if (sec == (time_t) -1)
        return -1;

    tp->tv_sec = sec;
    tp->tv_usec = 0;

     return 0;
}
#endif /* MOO_NEED_GETTIMEOFDAY */


extern "C" int
_moo_win32_fnmatch (const char *pattern,
                    const char *string,
                    int         flags)
{
    g_return_val_if_fail (flags == 0, -1);
    return _moo_glob_match_simple (pattern, string) ? 0 : 1;
}


static void
test_parse_cmd_line_one (const char  *cmd_line,
                         const char **expected)
{
    char **argv;
    GError *error = NULL;

    /* Anything but NULL must be treated properly, without warnings */
    TEST_EXPECT_WARNING (!cmd_line, "_moo_win32_lame_parse_cmd_line(%s)",
                         TEST_FMT_STR (cmd_line));

    argv = _moo_win32_lame_parse_cmd_line (cmd_line, &error);

    TEST_CHECK_WARNING ();

    TEST_ASSERT_STRV_EQ_MSG (argv, (char**) expected,
                             "_moo_win32_lame_parse_cmd_line(%s)",
                             TEST_FMT_STR (cmd_line));

    if (cmd_line && !expected && !argv)
        TEST_ASSERT_MSG (error != NULL,
                         "_moo_win32_lame_parse_cmd_line(%s): error not set",
                         TEST_FMT_STR (cmd_line));

    if (error)
        g_error_free (error);

    g_strfreev (argv);
}

static void
test_moo_win32_lame_parse_cmd_line (void)
{
    guint i;

    struct {
        const char *cmd_line;
        const char *argv[20];
    } cases[] = {
        { "dir foobar", { "cmd.exe", "/c", "dir", "foobar" } },
        { "dir \"c:\\program files\"", { "cmd.exe", "/c", "dir", "c:\\program files" } },
        /* wine doesn't have cmd.exe even though it has msiexec.exe! */
        { "msiexec", { "msiexec" } },
        { "msiexec.exe", { "msiexec.exe" } },
    };

    const char *fail_cases[] = {
        "quote '", "quote \"", NULL, "", " "
    };

    for (i = 0; i < G_N_ELEMENTS (cases); ++i)
        test_parse_cmd_line_one (cases[i].cmd_line, cases[i].argv);

    for (i = 0; i < G_N_ELEMENTS (fail_cases); ++i)
        test_parse_cmd_line_one (fail_cases[i], NULL);
}

void
moo_test_mooutils_win32 (void)
{
    MooTestSuite& suite = moo_test_suite_new ("mooutils-win32", "win32 utility functions", NULL, NULL, NULL);
    moo_test_suite_add_test (suite, "_moo_win32_lame_parse_cmd_line", "test of _moo_win32_lame_parse_cmd_line()",
                             (MooTestFunc) test_moo_win32_lame_parse_cmd_line,
                             NULL);
}
