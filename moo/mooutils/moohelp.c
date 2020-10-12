/*
 *   moohelp.c
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

#include "moohelp.h"
#include "mooaccel.h"
#include "mooutils-misc.h"
#include "mooi18n.h"
#include "moodialogs.h"
#include <gdk/gdkkeysyms.h>

typedef struct {
    MooHelpFunc func;
    gpointer func_data;
    GDestroyNotify notify;
} MooHelpData;

static MooHelpData *moo_help_get_data       (GtkWidget  *widget);


static MooHelpData *
moo_help_get_data (GtkWidget *widget)
{
    return (MooHelpData*) g_object_get_data (G_OBJECT (widget), "moo-help-data");
}


static gboolean
moo_help_callback_id (GtkWidget *widget,
                      gpointer   data)
{
    moo_help_open_id ((const char*) data, widget);
    return TRUE;
}

void
moo_help_set_id (GtkWidget  *widget,
                 const char *id)
{
    g_return_if_fail (GTK_IS_WIDGET (widget));

    if (id)
        moo_help_set_func_full (widget, moo_help_callback_id, g_strdup (id), g_free);
    else
        moo_help_set_func (widget, NULL);
}


static void
moo_help_data_free (MooHelpData *data)
{
    if (data)
    {
        if (data->notify)
            data->notify (data->func_data);
        g_free (data);
    }
}

void
moo_help_set_func_full (GtkWidget     *widget,
                        MooHelpFunc    func,
                        gpointer       func_data,
                        GDestroyNotify notify)
{
    MooHelpData *data = NULL;

    g_return_if_fail (GTK_IS_WIDGET (widget));

    if (func)
    {
        data = g_new0 (MooHelpData, 1);
        data->func = func;
        data->func_data = func_data;
        data->notify = notify;
    }

    g_object_set_data_full (G_OBJECT (widget), "moo-help-data", data,
                            (GDestroyNotify) moo_help_data_free);
}

void
moo_help_set_func (GtkWidget   *widget,
                   gboolean (*func) (GtkWidget*))
{
    g_return_if_fail (GTK_IS_WIDGET (widget));
    moo_help_set_func_full (widget, (MooHelpFunc) func, NULL, NULL);
}


gboolean
moo_help_open (GtkWidget *widget)
{
    MooHelpData *data;

    g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

    if (!(data = moo_help_get_data (widget)))
        return FALSE;

    return data->func (widget, data->func_data);
}

void
moo_help_open_any (GtkWidget *widget)
{
    while (widget)
    {
        if (moo_help_open (widget))
            return;
        widget = widget->parent;
    }

    moo_help_open_id (MOO_HELP_ID_CONTENTS, widget);
}


static gboolean
moo_help_key_press (GtkWidget   *widget,
                    GdkEventKey *event)
{
    if (moo_accel_check_event (widget, event,
                               MOO_ACCEL_HELP_KEY,
                               MOO_ACCEL_HELP_MODS))
        return moo_help_open (widget);
    else
        return FALSE;
}

void
moo_help_connect_keys (GtkWidget *widget)
{
    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_signal_handlers_disconnect_by_func (widget, (gpointer) moo_help_key_press, NULL);
    g_signal_connect (widget, "key-press-event", G_CALLBACK (moo_help_key_press), NULL);
}


static gboolean
try_help_dir (const char *dir)
{
    char *filename = g_build_filename (dir, "index.html", NULL);
    gboolean found_html = g_file_test (filename, G_FILE_TEST_IS_REGULAR);
    g_free (filename);
    return found_html;
}

static const char *
find_help_dir (void)
{
    static gboolean been_here = FALSE;
    static char *help_dir;
    const char *const *dirs;
    const char *const *p;

    if (been_here)
        return help_dir;
    been_here = TRUE;

#ifdef MOO_HELP_DIR
    if (try_help_dir (MOO_HELP_DIR))
    {
        help_dir = (char*) MOO_HELP_DIR;
        return help_dir;
    }
#endif

    dirs = g_get_system_data_dirs ();

    for (p = dirs; p && *p; ++p)
    {
        char *d = g_build_filename (*p, "doc", MOO_PACKAGE_NAME, "help", NULL);

        if (try_help_dir (d))
        {
            help_dir = d;
            return help_dir;
        }

        g_free (d);
    }

    return NULL;
}

static void
warn_no_help (GtkWidget *parent)
{
    static gboolean been_here;

    if (!been_here)
    {
        been_here = TRUE;
        moo_error_dialog (_("Help files not found"), NULL, parent);
    }
}

static void
warn_no_help_file (const char *basename,
                   GtkWidget  *parent)
{
    char *dir_utf8;
    char *msg;

    dir_utf8 = g_filename_display_name (find_help_dir ());
    msg = g_strdup_printf (_("File '%s' is missing in the directory '%s'"),
                           basename, dir_utf8);

    moo_error_dialog (_("Could not find help file"), msg, parent);

    g_free (msg);
    g_free (dir_utf8);
}

static void
open_file_by_id (const char *id,
                 GtkWidget  *parent)
{
    const char *dir;
    const char *basename;
    char *filename;

    g_return_if_fail (id != NULL);

    if (!(dir = find_help_dir ()))
    {
        warn_no_help (parent);
        return;
    }

    if (strcmp (id, MOO_HELP_ID_CONTENTS) == 0)
        id = MOO_HELP_ID_INDEX;

    if (strcmp (id, MOO_HELP_ID_INDEX) == 0 || id[0] == 0)
        basename = "index.html";
    else
        basename = id;

    filename = g_build_filename (dir, basename, NULL);

    if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        moo_open_file (filename);
    else if (strcmp (id, MOO_HELP_ID_INDEX) != 0)
        moo_open_file (MOO_HELP_ID_INDEX);
    else
        warn_no_help_file (basename, parent);

    g_free (filename);
}

void
moo_help_open_id (const char *id,
                  GtkWidget  *parent)
{
    g_return_if_fail (id != NULL);
    g_return_if_fail (!parent || GTK_IS_WIDGET (parent));
    open_file_by_id (id, parent);
}
