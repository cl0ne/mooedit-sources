/*
 *   moofileview-tools.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "moofileview/moofileview-tools.h"
#include "moofileview/moofileview-private.h"
#include "moofileview/moofile-private.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooaction.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils.h"
#include "mooutils/moospawn.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moo-mime.h"
#include <string.h>


typedef struct {
    guint merge_id;
    GSList *actions;
} ToolsInfo;

typedef struct {
    MooAction parent;
    MooFileView *fileview;
    GSList *extensions;
    GSList *mimetypes;
    char *command;
} ToolAction;

typedef MooActionClass ToolActionClass;

MOO_DEFINE_TYPE_STATIC (ToolAction, _moo_file_view_tool_action, MOO_TYPE_ACTION)


static void
_moo_file_view_tool_action_init (G_GNUC_UNUSED ToolAction *action)
{
}


static void
moo_file_view_tool_action_finalize (GObject *object)
{
    ToolAction *action = (ToolAction*) object;

    g_slist_foreach (action->extensions, (GFunc) g_free, NULL);
    g_slist_free (action->extensions);
    g_slist_foreach (action->mimetypes, (GFunc) g_free, NULL);
    g_slist_free (action->mimetypes);
    g_free (action->command);

    G_OBJECT_CLASS (_moo_file_view_tool_action_parent_class)->finalize (object);
}


static void
run_command (const char *command_template,
             const char *files)
{
    char *command;
    GError *error = NULL;
    GRegex *regex;

    regex = g_regex_new ("%[fF]", 0, 0, NULL);
    g_return_if_fail (regex != NULL);

    command = g_regex_replace_literal (regex, command_template, -1, 0, files, 0, &error);

    if (!command)
    {
        g_critical ("%s", moo_error_message (error));
        g_error_free (error);
        error = NULL;
    }

    if (command && !moo_spawn_command_line_async_with_flags (command, MOO_SPAWN_WIN32_HIDDEN_CONSOLE, &error))
    {
        g_warning ("%s", moo_error_message (error));
        g_error_free (error);
    }

    g_free (command);
    g_regex_unref (regex);
}

static void
moo_file_view_tool_action_activate (GtkAction *_action)
{
    ToolAction *action = (ToolAction*) _action;
    GList *files;
    gboolean has_small_f;
    gboolean has_cap_f;

    g_return_if_fail (MOO_IS_FILE_VIEW (action->fileview));
    g_return_if_fail (action->command && action->command[0]);

    has_small_f = strstr (action->command, "%f") != NULL;
    has_cap_f = strstr (action->command, "%F") != NULL;
    g_return_if_fail (has_small_f + has_cap_f == 1);

    files = _moo_file_view_get_filenames (action->fileview);
    g_return_if_fail (files != NULL);

    if (has_small_f)
    {
        while (files)
        {
            char *repl = g_strdup_printf ("\"%s\"", (char*) files->data);
            run_command (action->command, repl);
            g_free (repl);
            g_free (files->data);
            files = g_list_delete_link (files, files);
        }
    }
    else
    {
        GString *repl = g_string_new (NULL);
        gboolean first = TRUE;
        while (files)
        {
            if (!first)
                g_string_append_c (repl, ' ');
            first = FALSE;
            g_string_append_c (repl, '"');
            g_string_append (repl, (char*) files->data);
            g_string_append_c (repl, '"');
            g_free (files->data);
            files = g_list_delete_link (files, files);
        }
        run_command (action->command, repl->str);
        g_string_free (repl, TRUE);
    }
}


static void
_moo_file_view_tool_action_class_init (ToolActionClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = moo_file_view_tool_action_finalize;
    GTK_ACTION_CLASS (klass)->activate = moo_file_view_tool_action_activate;
}


static void
tools_info_free (ToolsInfo *info)
{
    if (info)
    {
        g_slist_foreach (info->actions, (GFunc) g_object_unref, NULL);
        g_slist_free (info->actions);
        g_free (info);
    }
}


static void
remove_old_tools (MooFileView    *fileview,
                  MooUiXml       *xml,
                  GtkActionGroup *group)
{
    ToolsInfo *info;

    info = g_object_get_data (G_OBJECT (fileview), "moo-file-view-tools-info");

    if (info)
    {
        moo_ui_xml_remove_ui (xml, info->merge_id);

        while (info->actions)
        {
            GtkAction *action = info->actions->data;
            gtk_action_group_remove_action (group, action);
            g_object_unref (action);
            info->actions = g_slist_delete_link (info->actions, info->actions);
        }
    }

    /* this will call tools_info_free */
    g_object_set_data (G_OBJECT (fileview), "moo-file-view-tools-info", NULL);
}


static GtkAction *
tool_action_new (MooFileView *fileview,
                 const char  *label,
                 const char  *extensions,
                 const char  *mimetypes,
                 const char  *command)
{
    ToolAction *action;
    char *name;
    char **set, **p;

    g_return_val_if_fail (label != NULL, NULL);
    g_return_val_if_fail (command && (strstr (command, "%f") || strstr (command, "%F")), NULL);

    name = g_strdup_printf ("moofileview-action-%08x", g_random_int ());
    action = (ToolAction*)
        g_object_new (_moo_file_view_tool_action_get_type (),
                      "label", label, "name", name,
                      (const char*) NULL);
    action->fileview = fileview;
    action->command = g_strdup (command);

    if (extensions && extensions[0])
    {
        set = g_strsplit_set (extensions, ";,", 0);

        for (p = set; p && *p; ++p)
        {
            g_strstrip (*p);

            if (**p)
                action->extensions = g_slist_prepend (action->extensions,
                                                      g_strdup (*p));
        }

        action->extensions = g_slist_reverse (action->extensions);
        g_strfreev (set);
    }

    if (mimetypes && mimetypes[0])
    {
        set = g_strsplit_set (mimetypes, ";,", 0);

        for (p = set; p && *p; ++p)
        {
            g_strstrip (*p);

            if (**p)
                action->mimetypes = g_slist_prepend (action->mimetypes,
                                                     g_strdup (*p));
        }

        action->mimetypes = g_slist_reverse (action->mimetypes);
        g_strfreev (set);
    }

    g_free (name);
    return GTK_ACTION (action);
}


void
_moo_file_view_tools_load (MooFileView *fileview)
{
    ToolsInfo *info;
    MooMarkupNode *doc;
    MooMarkupNode *root, *child;
    MooUiXml *xml;
    MooActionCollection *actions;
    GtkActionGroup *group;
    MooUiNode *ph;
    GSList *l;

    g_return_if_fail (MOO_IS_FILE_VIEW (fileview));

    xml = moo_file_view_get_ui_xml (fileview);
    actions = moo_file_view_get_actions (fileview);
    group = moo_action_collection_get_group (actions, NULL);
    remove_old_tools (fileview, xml, group);

    doc = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (doc != NULL);

    info = g_new0 (ToolsInfo, 1);
    info->merge_id = moo_ui_xml_new_merge_id (xml);
    g_object_set_data_full (G_OBJECT (fileview), "moo-file-view-tools-info", info,
                            (GDestroyNotify) tools_info_free);

    ph = moo_ui_xml_find_placeholder (xml, "OpenWith");
    g_return_if_fail (ph != NULL);

    root = moo_markup_get_element (doc, "MooFileView/Tools");

    for (child = root ? root->children : NULL; child != NULL; child = child->next)
    {
        GtkAction *action;
        const char *label, *extensions, *mimetypes;
        const char *command;

        if (!MOO_MARKUP_IS_ELEMENT (child))
            continue;

        if (strcmp (child->name, "tool"))
        {
            g_warning ("unknown node '%s'", child->name);
            continue;
        }

        label = moo_markup_get_prop (child, "label");
        extensions = moo_markup_get_prop (child, "extensions");
        mimetypes = moo_markup_get_prop (child, "mimetypes");
        command = moo_markup_get_content (child);

        if (!label)
        {
            g_warning ("label missing");
            continue;
        }

        if (!command)
        {
            g_warning ("command missing");
            continue;
        }

        action = tool_action_new (fileview, label, extensions, mimetypes, command);

        if (!action)
            continue;

        info->actions = g_slist_prepend (info->actions, action);
    }

    {
        /* Translators: this is a context menu item label in the file selector, remove the part before and including | */
        GtkAction *action = tool_action_new (fileview, Q_("Open with|Default Application"), "*", NULL,
#ifndef __WIN32__
                                             "xdg-open %f"
#else
                                             "cmd /c start %f"
#endif
                                            );
        info->actions = g_slist_prepend (info->actions, action);
    }

    info->actions = g_slist_reverse (info->actions);

    for (l = info->actions; l != NULL; l = l->next)
    {
        GtkAction *action = l->data;
        char *markup;

        gtk_action_group_add_action (group, action);

        markup = g_markup_printf_escaped ("<item action=\"%s\"/>",
                                          gtk_action_get_name (action));
        moo_ui_xml_insert (xml, info->merge_id, ph, -1, markup);
        g_free (markup);
    }
}


static gboolean
action_check_one (ToolAction *action,
                  MooFile    *file)
{
    GSList *l;
    G_GNUC_UNUSED const char *mime;

    g_return_val_if_fail (file != NULL, FALSE);

    if (!action->extensions && !action->mimetypes)
        return TRUE;

    for (l = action->extensions; l != NULL; l = l->next)
        if (_moo_glob_match_simple (l->data, _moo_file_display_name (file)))
            return TRUE;

    mime = _moo_file_get_mime_type (file);
    g_return_val_if_fail (mime != NULL, FALSE);

    if (mime == MOO_MIME_TYPE_UNKNOWN)
        return FALSE;

    for (l = action->mimetypes; l != NULL; l = l->next)
        if (moo_mime_type_is_subclass (mime, l->data))
            return TRUE;

    return FALSE;
}


static void
action_check (ToolAction *action,
              GList      *files)
{
    gboolean visible = TRUE;

    while (files)
    {
        MooFile *f = files->data;

        if (!MOO_FILE_EXISTS (f) || MOO_FILE_IS_DIR (f) || !action_check_one (action, f))
        {
            visible = FALSE;
            break;
        }

        files = files->next;
    }

    g_object_set (action, "visible", visible, NULL);
}


void
_moo_file_view_tools_check (MooFileView *fileview)
{
    GList *files;
    ToolsInfo *info;
    GSList *l;

    info = g_object_get_data (G_OBJECT (fileview), "moo-file-view-tools-info");

    if (!info)
        return;

    files = _moo_file_view_get_files (fileview);

    if (!files)
    {
        for (l = info->actions; l != NULL; l = l->next)
            g_object_set (l->data, "visible", FALSE, NULL);
        return;
    }

    for (l = info->actions; l != NULL; l = l->next)
        action_check (l->data, files);

    g_list_foreach (files, (GFunc) _moo_file_unref, NULL);
    g_list_free (files);
}
