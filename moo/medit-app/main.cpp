/*
 *   medit-app.c
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

#include <config.h>
#include "mooapp/mooapp.h"
#include "mooedit/mooplugin.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#include "moocpp/regex.h"
#include "plugins/mooplugin-builtin.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "mem-debug.h"
#include "run-tests.h"
#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#include <windows.h>
#include <windowsx.h>
#endif

struct MeditOpts 
{
    int use_session = -1;
	int pid = -1;
    gboolean new_app = false;
    const char *instance_name = nullptr;
    gboolean new_window = false;
    gboolean new_tab = false;
    gboolean reload = false;
    const char *project = nullptr;
    gboolean project_mode = false;
    int line = 0;
    const char *encoding = nullptr;
	const char *log_file = nullptr;
	gboolean log_window = false;
	const char *exec_string = nullptr;
	const char *exec_file = nullptr;
	gstrvec files;
	char **filesp = nullptr;
	const char *geometry = nullptr;
	gboolean show_version = false;
	const char *debug = nullptr;
	gboolean ut = false;
	gboolean ut_uninstalled = false;
	gboolean ut_list = false;
	char *ut_dir = nullptr;
	char *ut_coverage_file = nullptr;
    gstrvec ut_tests;
	char **run_script = nullptr;
	char **send_script = nullptr;
    gboolean portable = false;
};

static MeditOpts medit_opts;

#include "parse.h"

typedef MooApp MeditApp;
typedef MooAppClass MeditAppClass;
MOO_DEFINE_TYPE_STATIC (MeditApp, medit_app, MOO_TYPE_APP)

static void
medit_app_init_plugins (G_GNUC_UNUSED MooApp *app)
{
    moo_plugin_init ();
}

static void
medit_app_class_init (MooAppClass *klass)
{
    klass->init_plugins = medit_app_init_plugins;
}

static void
medit_app_init (G_GNUC_UNUSED MooApp *app)
{
}

static gboolean
parse_use_session (const char *option_name,
                   const char *value,
                   G_GNUC_UNUSED gpointer data,
                   GError **error)
{
    if (!value || strcmp (value, "yes") == 0)
    {
        medit_opts.use_session = TRUE;
        return TRUE;
    }
    else if (strcmp (value, "no") == 0)
    {
        medit_opts.use_session = FALSE;
        return TRUE;
    }
    else
    {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     /* error message for wrong command line */
                     _("Invalid value '%s' for option %s"), value, option_name);
        return FALSE;
    }
}

/* Check if there is an argument of the form +<number>, and treat it as --line <number> */
static void
check_plus_line_arg (void)
{
    g::Regex re = g::Regex::compile("^\\+(?P<line>\\d+)", g::Regex::OPTIMIZE | g::Regex::DUPNAMES);
	g_return_if_fail(re.is_valid());

    for (size_t i = 0; i < medit_opts.files.size(); ++i)
    {
        const gstr& file = medit_opts.files[i];
        if (g::MatchInfo match_info = re.match(file))
        {
            int line = 0;
            gstr line_string = match_info.fetch_named("line");

            errno = 0;
            line = strtol(line_string.get(), NULL, 10);
            if (errno != 0)
                line = 0;

            // if a file "+10" exists, open it
            if (line > 0 && g_file_test (file.get(), G_FILE_TEST_EXISTS))
                line = 0;

            if (line > 0)
            {
                medit_opts.line = line;
                medit_opts.files.erase(medit_opts.files.begin() + i);
                return;
            }
        }
    }
}

static gboolean
post_parse_func (void)
{
	medit_opts.files = gstr::take(medit_opts.filesp);
	medit_opts.filesp = nullptr;

    if (medit_opts.show_version)
    {
        g_print ("medit " MOO_DISPLAY_VERSION "\n");
        exit (0);
    }

    if (medit_opts.ut_list)
    {
        list_unit_tests (medit_opts.ut_dir);
        exit (0);
    }

    if (medit_opts.ut)
        std::swap(medit_opts.ut_tests, medit_opts.files);

    if (medit_opts.pid > 0 && medit_opts.instance_name)
    {
        /* error message for wrong commmand line */
        g_printerr (_("%s and %s options may not be used simultaneously\n"),
                    "--app-name", "--pid");
        exit (EXIT_FAILURE);
    }

    if (medit_opts.debug)
        g_setenv ("MOO_DEBUG", medit_opts.debug, FALSE);

    if (medit_opts.project)
        medit_opts.project_mode = TRUE;

    check_plus_line_arg ();

    return TRUE;
}

static GOptionContext *
parse_args (int argc, char *argv[])
{
    GOptionContext *ctx;
    GOptionGroup *grp;
	GError *error = nullptr;

	GOptionEntry medit_options[] = {
		{ "new-app", 'n', 0, G_OPTION_ARG_NONE, &medit_opts.new_app,
				/* help message for command line option --new-app */ N_("Run new instance of application"), NULL },
		{ "use-session", 's', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*) parse_use_session,
				/* help message for command line option --use-session */ N_("Load and save session"), "yes|no" },
		{ "pid", 0, 0, G_OPTION_ARG_INT, &medit_opts.pid,
				/* help message for command line option --pid=PID */ N_("Use existing instance with process id PID"),
				/* "PID" part in "--pid=PID" */ N_("PID") },
		{ "app-name", 0, 0, G_OPTION_ARG_STRING, (gpointer) &medit_opts.instance_name,
				/* help message for command line option --app-name=NAME */ N_("Set instance name to NAME if it's not already running"),
				/* "NAME" part in "--app-name=NAME" */ N_("NAME") },
		{ "new-window", 'w', 0, G_OPTION_ARG_NONE, &medit_opts.new_window,
				/* help message for command line option --new-window */ N_("Open file(s) in a new window"), NULL },
		{ "new-tab", 't', 0, G_OPTION_ARG_NONE, &medit_opts.new_tab,
				/* help message for command line option --new-tab */ N_("Open file(s) in a new tab"), NULL },
		{ "project", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_FILENAME, (gpointer) &medit_opts.project,
				"Open project file FILE", "FILE" },
		{ "project-mode", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &medit_opts.project_mode,
				"IDE mode", NULL },
		{ "line", 'l', 0, G_OPTION_ARG_INT, &medit_opts.line,
				/* help message for command line option --line=LINE */ N_("Open file and position cursor on line LINE"),
				/* "LINE" part in --line=LINE */ N_("LINE") },
		{ "encoding", 'e', 0, G_OPTION_ARG_STRING, (gpointer) &medit_opts.encoding,
				/* help message for command line option --encoding=ENCODING */ N_("Use character encoding ENCODING"),
				/* "ENCODING" part in --encoding=ENCODING */ N_("ENCODING") },
		{ "reload", 'r', 0, G_OPTION_ARG_NONE, &medit_opts.reload,
				/* help message for command line option --reload */ N_("Automatically reload file if it was modified on disk"), NULL },
		{ "run-script", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING_ARRAY, (gpointer) &medit_opts.run_script,
				"Run SCRIPT", "SCRIPT" },
		{ "send-script", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING_ARRAY, (gpointer) &medit_opts.send_script,
				"Send SCRIPT to existing instance", "SCRIPT" },
		{ "log-window", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &medit_opts.log_window,
				"Show debug output", NULL },
		{ "log-file", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_FILENAME, (gpointer) &medit_opts.log_file,
				"Write debug output to FILE", "FILE" },
		{ "debug", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, (gpointer) &medit_opts.debug,
				"Run in debug mode", NULL },
		{ "geometry", 0, 0, G_OPTION_ARG_STRING, (gpointer) &medit_opts.geometry,
				/* help message for command line option --geometry=WIDTHxHEIGHT[+X+Y] */ N_("Default window size and position"),
				/* "WIDTHxHEIGHT[+X+Y]" part in --geometry=WIDTHxHEIGHT[+X+Y] */ N_("WIDTHxHEIGHT[+X+Y]") },
		{ "version", 0, 0, G_OPTION_ARG_NONE, &medit_opts.show_version,
				/* help message for command line option --version */ N_("Show version information and exit"), NULL },
		{ "ut", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &medit_opts.ut,
				"Run unit tests", NULL },
		{ "ut-uninstalled", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &medit_opts.ut_uninstalled,
				"Run unit tests in uninstalled medit", NULL },
		{ "ut-dir", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &medit_opts.ut_dir,
				"Data dir for unit tests", NULL },
		{ "ut-coverage", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_FILENAME, &medit_opts.ut_coverage_file,
				"File to write coverage data to", NULL },
		{ "ut-list", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &medit_opts.ut_list,
				"List unit tests", NULL },
	#ifdef __WIN32__
		{ "portable", 0, G_OPTION_ARG_NONE, G_OPTION_ARG_NONE, &medit_opts.portable,
				"Run medit in portable mode", NULL },
	#endif
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &medit_opts.filesp,
				NULL, /* "FILES" part in "medit [OPTION...] [FILES]" */ N_("FILES") },
		{ NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
	};

    grp = g_option_group_new ("medit", "medit", "medit", NULL, NULL);
    g_option_group_add_entries (grp, medit_options);
    g_option_group_set_parse_hooks (grp, NULL, (GOptionParseFunc) post_parse_func);
    g_option_group_set_translation_domain (grp, GETTEXT_PACKAGE);

    ctx = g_option_context_new (NULL);
    g_option_context_set_main_group (ctx, grp);
    g_option_context_add_group (ctx, gtk_get_option_group (FALSE));

    if (!g_option_context_parse (ctx, &argc, &argv, &error))
    {
        g_printerr ("%s\n", error->message);
        exit (EXIT_FAILURE);
    }

    return ctx;
}

static void
notify_startup_complete (void)
{
#ifdef GDK_WINDOWING_X11
    const char *v = g_getenv ("DESKTOP_STARTUP_ID");

    if (v && *v)
    {
        gtk_init (NULL, NULL);
        gdk_notify_startup_complete ();
    }
#endif
}

static guint32
get_time_stamp (void)
{
#ifdef GDK_WINDOWING_X11
    const char *startup_id;
    char *time_str, *end;
    gulong stamp;

    startup_id = g_getenv ("DESKTOP_STARTUP_ID");
    if (!startup_id || !startup_id[0])
        return 0;

    if (!(time_str = g_strrstr (startup_id, "_TIME")))
        return 0;

    errno = 0;
    time_str += 5;
    stamp = strtoul (time_str, &end, 0);

    return !*end && !errno ? stamp : 0;
#else
    return 0;
#endif
}

static void
push_appdir_to_path (void)
{
#ifdef __WIN32__
    char *appdir;
    const char *path;
    char *new_path;

    appdir = moo_win32_get_app_dir ();
    g_return_if_fail (appdir != NULL);

    path = g_getenv ("Path");

    if (path)
        new_path = g_strdup_printf ("%s;%s", appdir, path);
    else
        new_path = g_strdup (appdir);

    g_setenv ("Path", new_path, TRUE);

    g_free (new_path);
    g_free (appdir);
#endif
}

#ifdef MOO_ENABLE_PROJECT
static void
project_mode (const char *file)
{
    MooPlugin *plugin;

    plugin = (MooPlugin*) moo_plugin_lookup ("ProjectManager");

    if (!plugin)
    {
        fputs ("Could not initialize project manager plugin\n", stderr);
        exit (EXIT_FAILURE);
    }

    if (file)
    {
        char *project = moo_filename_from_locale (file);
        g_object_set (plugin, "project", project, NULL);
        g_free (project);
    }

    moo_plugin_set_enabled (plugin, TRUE);
}
#endif

#undef WANT_SYNAPTICS_FIX
#if defined(GDK_WINDOWING_WIN32) && !GTK_CHECK_VERSION(2,24,8)
#define WANT_SYNAPTICS_FIX 1
#endif

#ifdef WANT_SYNAPTICS_FIX

static GdkWindow *
_moo_get_toplevel_window_at_pointer (void)
{
    GList *list, *l;
    GSList *windows = NULL;
    GtkWidget *top;
    POINT point;

    GetCursorPos (&point);

    list = gtk_window_list_toplevels ();

    for (l = list; l != NULL; l = l->next)
    {
        HWND hwnd;
        RECT rect = { 0 };
        GdkWindow *window = GTK_WIDGET (l->data)->window;

        if (!window)
            continue;

        hwnd = GDK_WINDOW_HWND (window);
        GetWindowRect(hwnd, &rect);
        if (IsWindowVisible(hwnd) && PtInRect(&rect, point))
            windows = g_slist_prepend (windows, l->data);
    }

    top = windows ? GTK_WIDGET (_moo_get_top_window (windows)) : NULL;

    g_list_free (list);
    g_slist_free (windows);
    return top ? top->window : NULL;
}

// stolen from gdkwindow-win32.c
static GdkModifierType
get_current_mask (void)
{
    GdkModifierType mask = 0;
    BYTE kbd[256];

    GetKeyboardState (kbd);

    if (kbd[VK_SHIFT] & 0x80)
        mask |= GDK_SHIFT_MASK;
    if (kbd[VK_CAPITAL] & 0x80)
        mask |= GDK_LOCK_MASK;
    if (kbd[VK_CONTROL] & 0x80)
        mask |= GDK_CONTROL_MASK;
    if (kbd[VK_MENU] & 0x80)
        mask |= GDK_MOD1_MASK;
    if (kbd[VK_LBUTTON] & 0x80)
        mask |= GDK_BUTTON1_MASK;
    if (kbd[VK_MBUTTON] & 0x80)
        mask |= GDK_BUTTON2_MASK;
    if (kbd[VK_RBUTTON] & 0x80)
        mask |= GDK_BUTTON3_MASK;

    return mask;
}

// stolen from gdkwindow-win32.c
static GdkWindow *
get_pointer (GdkWindow *window,
             gint      *x,
             gint      *y)
{
    POINT point;
    HWND hwnd, hwndc;
    GdkWindow *retval = window;
    GdkWindow *child = NULL;

    hwnd = GDK_WINDOW_HWND (window);
    GetCursorPos (&point);
    ScreenToClient (hwnd, &point);

    *x = point.x;
    *y = point.y;

    hwndc = ChildWindowFromPointEx (hwnd, point, CWP_SKIPINVISIBLE);
    if (hwndc != NULL && hwndc != hwnd)
        child = gdk_win32_handle_table_lookup ((GdkNativeWindow) hwndc);
    if (child != NULL)
        retval = child;

    return retval;
}

static GdkWindow *
get_youngest_child_at_pointer (GdkWindow *window,
                               int       *x,
                               int       *y)
{
    gboolean first_time = TRUE;

    while (TRUE)
    {
        int tmp_x, tmp_y;
        GdkWindow *child_window = get_pointer (window, &tmp_x, &tmp_y);

        if (first_time)
        {
            *x = tmp_x;
            *y = tmp_y;
        }

        if (!child_window || child_window == window)
            return window;
        else
            window = child_window;
    }
}

static gboolean
fill_scroll_event (GdkEvent  *event,
                   const MSG *msg)
{
    GdkWindow *window, *child_window;
    int x, y;

    if (!(window = _moo_get_toplevel_window_at_pointer ()))
        return FALSE;

    child_window = get_youngest_child_at_pointer (window, &x, &y);

    if (child_window && child_window != window)
    {
        int origin_x, origin_y;
        int origin_child_x, origin_child_y;
        gdk_window_get_origin (window, &origin_x, &origin_y);
        gdk_window_get_origin (child_window, &origin_child_x, &origin_child_y);
        x -= (origin_child_x - origin_x);
        y -= (origin_child_y - origin_y);
        window = child_window;
    }

    event->type = GDK_SCROLL;
    event->scroll.window = window;
    event->scroll.direction = (((short) HIWORD (msg->wParam)) > 0) ? GDK_SCROLL_UP : GDK_SCROLL_DOWN;
    event->scroll.time = 0;
    event->scroll.x = x;
    event->scroll.y = y;
    event->scroll.x_root = GET_X_LPARAM (msg->lParam);
    event->scroll.y_root = GET_Y_LPARAM (msg->lParam);
    event->scroll.state = get_current_mask ();
    event->scroll.device = gdk_device_get_core_pointer ();

    return TRUE;
}

/* Synaptics touchpad is smart: it doesn't generate normal scroll events
 * for some miraculous reason if the window doesn't have fancy scrolling
 * window styles; instead it creates its own window to draw neat image
 * of a scrollbar under the mouse; and sends a scroll event to the gdk
 * window underneath. Now, gdk is smart too, it checks what window is
 * under the mouse and does nothing if that window isn't gdk's. That window
 * isn't gdk's because it's that neat image of a scrollbar. Thank you very
 * much Synaptics. */
static GdkFilterReturn
touchpad_filter_func (GdkXEvent *xevent,
                      GdkEvent  *event,
                      G_GNUC_UNUSED gpointer data)
{
    const MSG *msg = (MSG*) xevent;
    wchar_t class_name[256];
    HWND mouse_hwnd;
    POINT point;

    if (msg->message != WM_MOUSEWHEEL)
        return GDK_FILTER_CONTINUE;

    point.x = GET_X_LPARAM (msg->lParam);
    point.y = GET_Y_LPARAM (msg->lParam);

    if ((mouse_hwnd = WindowFromPoint (point)) == NULL)
        return GDK_FILTER_CONTINUE;

    if (GetClassName (mouse_hwnd, class_name, G_N_ELEMENTS (class_name)) == 0 ||
        wcscmp (class_name, L"SynTrackCursorWindowClass") != 0)
            return GDK_FILTER_CONTINUE;

    return fill_scroll_event (event, msg) ? GDK_FILTER_TRANSLATE : GDK_FILTER_CONTINUE;
}

static void
hookup_synaptics_touchpad (void)
{
    gdk_window_add_filter (NULL, touchpad_filter_func, NULL);
}

#endif // WANT_SYNAPTICS_FIX

static void
unit_test_func (void)
{
    MooTestOptions opts = MooTestOptions (0);
    int status;

    if (!medit_opts.ut_uninstalled)
        opts = MooTestOptions (opts | MOO_TEST_INSTALLED);

    status = unit_tests_main (opts, medit_opts.ut_tests, medit_opts.ut_dir, medit_opts.ut_coverage_file);
    moo_app_set_exit_status (moo_app_instance (), status);
    moo_app_quit (moo_app_instance ());
}

static void
run_script_func (void)
{
    char **p;
    for (p = medit_opts.run_script; p && *p; ++p)
        moo_app_run_script (moo_app_instance(), *p);
}

static void
install_log_handlers (void)
{
    if (medit_opts.log_file)
        moo_set_log_func_file (medit_opts.log_file);
    else if (medit_opts.log_window)
        moo_set_log_func_window (TRUE);
#ifdef __WIN32__
    // this will install do-nothing log and print handlers plus
    // a fatal error win32 message handler (it will also turn off
    // console output, but that is not visible anyway)
    else
        moo_set_log_func_silent ();
#endif
}

#ifdef __WIN32__
static void
setup_portable_mode (void)
{
    char *appdir = NULL;
    char *share = NULL;
    char *datadir = NULL;
    char *cachedir = NULL;
    char *tmp = NULL;

    appdir = moo_win32_get_app_dir ();
    g_return_if_fail (appdir != NULL);

    share = g_build_filename (appdir, "..", "share", NULL);
    g_return_if_fail (share != NULL);

    if (g_file_test (share, G_FILE_TEST_IS_DIR))
    {
        datadir = g_build_filename (share, MEDIT_PORTABLE_DATA_DIR, NULL);
        cachedir = g_build_filename (share, MEDIT_PORTABLE_CACHE_DIR, NULL);
    }
    else
    {
        datadir = g_build_filename (appdir, MEDIT_PORTABLE_DATA_DIR, NULL);
        cachedir = g_build_filename (appdir, MEDIT_PORTABLE_CACHE_DIR, NULL);
    }

    g_return_if_fail (datadir != NULL && cachedir != NULL);
    
    tmp = _moo_normalize_file_path (datadir);
    moo_set_user_data_dir (tmp);
    g_free (tmp);
    tmp = NULL;

    tmp = _moo_normalize_file_path (cachedir);
    moo_set_user_cache_dir (tmp);
    g_free (tmp);
    tmp = NULL;

    g_free (cachedir);
    g_free (datadir);
    g_free (share);
    g_free (appdir);
}

static void
check_portable_mode (void)
{
    gboolean portable = FALSE;
    char *appdir = NULL;
    char *magic_file = NULL;

    if (medit_opts.portable)
        portable = TRUE;

    if (!portable)
    {
        appdir = moo_win32_get_app_dir ();
        g_return_if_fail (appdir != NULL);
        magic_file = g_build_filename (appdir, MEDIT_PORTABLE_MAGIC_FILE_NAME, NULL);
        g_return_if_fail (magic_file != NULL);
        if (g_file_test (magic_file, G_FILE_TEST_EXISTS))
            portable = TRUE;
    }

    if (portable)
        setup_portable_mode ();

    g_free (magic_file);
    g_free (appdir);
}
#endif // __WIN32__

static int
medit_main (int argc, char *argv[])
{
    MooApp *app = NULL;
    MooEditor *editor;
    int retval;
    gboolean new_instance = FALSE;
    gboolean run_input = TRUE;
    guint32 stamp;
    const char *name = NULL;
    char pid_buf[32];
    GOptionContext *ctx;
    MooOpenInfoArray *files;

#ifdef __WIN32__
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif // __WIN32__

    init_mem_stuff ();
    moo_thread_init ();
    g_set_prgname ("medit");

    ctx = parse_args (argc, argv);

    stamp = get_time_stamp ();

#ifdef WANT_SYNAPTICS_FIX
    hookup_synaptics_touchpad ();
#endif

#if 0
    gdk_window_set_debug_updates (TRUE);
#endif

#if 0
    g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc) exit, NULL, NULL);
#endif

#ifdef __WIN32__
    check_portable_mode ();
#endif

    if (medit_opts.new_app || medit_opts.project_mode)
        new_instance = TRUE;

    run_input = !medit_opts.new_app || medit_opts.instance_name ||
                 medit_opts.use_session == 1 || medit_opts.project_mode;

    if (medit_opts.ut)
    {
        new_instance = TRUE;
        run_input = FALSE;
        medit_opts.use_session = FALSE;
    }

    if (medit_opts.pid > 0)
    {
        sprintf (pid_buf, "%d", medit_opts.pid);
        name = pid_buf;
    }
    else if (medit_opts.instance_name)
        name = medit_opts.instance_name;
    else if (!medit_opts.new_app)
        name = g_getenv ("MEDIT_PID");

    if (name && !name[0])
        name = NULL;

    if (medit_opts.send_script)
    {
        char **p;
        for (p = medit_opts.send_script; *p; ++p)
        {
            GString *msg = g_string_new ("e");
            g_string_append (msg, *p);
            moo_app_send_msg (name, msg->str, msg->len + 1);
        }
        notify_startup_complete ();
        exit (0);
    }

    files = parse_files ();

    if (name)
    {
        if (moo_app_send_files (files, stamp, name))
            exit (0);

        if (!medit_opts.instance_name)
        {
            g_printerr ("Could not send files to instance '%s'\n", name);
            exit (EXIT_FAILURE);
        }
    }

    if (!new_instance && !medit_opts.instance_name &&
         moo_app_send_files (files, stamp, NULL))
    {
        notify_startup_complete ();
        exit (0);
    }

    push_appdir_to_path ();

    gtk_init (NULL, NULL);

    install_log_handlers ();

    app = MOO_APP (g_object_new (medit_app_get_type (),
                                 "run-input", run_input,
                                 "use-session", medit_opts.use_session,
                                 "instance-name", medit_opts.instance_name,
                                 (const char*) NULL));

    if (!moo_app_init (app))
    {
        gdk_notify_startup_complete ();
        g_object_unref (app);
        exit (EXIT_FAILURE);
    }

    if (medit_opts.geometry && *medit_opts.geometry)
        moo_window_set_default_geometry (medit_opts.geometry);

    if (medit_opts.project_mode)
#ifdef MOO_ENABLE_PROJECT
        project_mode (medit_opts.project);
#else
    {
        fputs ("medit was built without project support\n", stderr);
    exit (EXIT_FAILURE);
    }
#endif
    else
        moo_app_load_session (app);

    editor = moo_app_get_editor (app);
    if (!moo_editor_get_active_window (editor))
        moo_editor_new_window (editor);

    if (files)
        moo_app_open_files (app, files, stamp);

    moo_open_info_array_free (files);
    g_option_context_free (ctx);

    if (medit_opts.ut)
        g_signal_connect (app, "started", G_CALLBACK (unit_test_func), NULL);
    if (medit_opts.run_script)
        g_signal_connect (app, "started", G_CALLBACK (run_script_func), NULL);

    retval = moo_app_run (app);

    g_object_unref (app);

#ifdef __WIN32__
    CoUninitialize();
#endif // __WIN32__

    return retval;
}

int
main (int argc, char *argv[])
{
    return medit_main (argc, argv);
}

#if defined(__WIN32__) && !defined(__GNUC__)

#include <windows.h>

int WINAPI
WinMain (HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         char     *lpszCmdLine,
         int       nCmdShow)
{
    return main (__argc, __argv);
}

#endif
