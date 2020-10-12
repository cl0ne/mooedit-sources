/*
 *   moofileview-dialogs.c
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

#include "moofileview/moofileview-dialogs.h"
#include "moofileview/moofilesystem.h"
#include "moofileview/moofile-private.h"
#include "mooutils/mooentry.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moocompat.h"
#include "moofileview/moofileprops-gxml.h"
#include "moofileview/moocreatefolder-gxml.h"
#include "moofileview/moofileview-drop-gxml.h"
#include <time.h>
#include <string.h>
#include <gtk/gtk.h>


static void moo_file_props_dialog_destroy   (GtkObject          *object);
static void moo_file_props_dialog_show      (GtkWidget          *widget);
static void moo_file_props_dialog_response  (GtkDialog          *dialog,
                                             int                 reponse);
static void moo_file_props_dialog_ok        (MooFilePropsDialog *dialog);


G_DEFINE_TYPE(MooFilePropsDialog, _moo_file_props_dialog, GTK_TYPE_DIALOG)

static void
_moo_file_props_dialog_class_init (MooFilePropsDialogClass *klass)
{
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

    gtkobject_class->destroy = moo_file_props_dialog_destroy;
    widget_class->show = moo_file_props_dialog_show;
    dialog_class->response = moo_file_props_dialog_response;
}


static void
_moo_file_props_dialog_init (MooFilePropsDialog *dialog)
{
    dialog->xml = moo_file_props_xml_new ();

    dialog->notebook = GTK_WIDGET (dialog->xml->MooFileProps);
    dialog->icon = GTK_WIDGET (dialog->xml->icon);
    dialog->entry = GTK_WIDGET (dialog->xml->entry);
    dialog->table = GTK_WIDGET (dialog->xml->table);

    gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), dialog->notebook);
    gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK, GTK_RESPONSE_OK,
                            NULL);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}


static void
moo_file_props_dialog_response (GtkDialog  *dialog,
                                gint        response)
{
    switch (response)
    {
        case GTK_RESPONSE_OK:
            moo_file_props_dialog_ok (MOO_FILE_PROPS_DIALOG (dialog));
            break;

        case GTK_RESPONSE_DELETE_EVENT:
        case GTK_RESPONSE_CANCEL:
            break;

        default:
            g_warning ("unknown response code");
    }

    gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
moo_file_props_dialog_ok (MooFilePropsDialog *dialog)
{
    const char *old_name, *new_name;
    char *old_path, *new_path;
    GError *error = NULL;

    if (!dialog->file)
        return;

    old_name = _moo_file_display_name (dialog->file);
    new_name = gtk_entry_get_text (GTK_ENTRY (dialog->entry));

    if (!strcmp (old_name, new_name))
        return;

    old_path = _moo_file_system_make_path (_moo_folder_get_file_system (dialog->folder),
                                           _moo_folder_get_path (dialog->folder),
                                           old_name, NULL);
    new_path = _moo_file_system_make_path (_moo_folder_get_file_system (dialog->folder),
                                           _moo_folder_get_path (dialog->folder),
                                           new_name, NULL);

    if (!old_path || !new_path)
    {
        g_warning ("oops");
        goto out;
    }

    if (!_moo_file_system_move_file (_moo_folder_get_file_system (dialog->folder),
                                     old_path, new_path, &error))
    {
        g_warning ("could not rename '%s' to '%s'",
                   old_path, new_path);

        if (error)
        {
            g_warning ("%s", error->message);
            g_error_free (error);
        }

        goto out;
    }

out:
    g_free (old_path);
    g_free (new_path);
    _moo_file_props_dialog_set_file (dialog, NULL, NULL);
}


static void
set_file (MooFilePropsDialog *dialog,
          MooFile            *file,
          MooFolder          *folder)
{
    if (file == dialog->file)
        return;

    if (folder != dialog->folder)
    {
        if (dialog->folder)
            g_object_unref (dialog->folder);
        dialog->folder = folder;
        if (folder)
            g_object_ref (folder);
    }

    if (dialog->file)
        _moo_file_unref (dialog->file);
    dialog->file = file;
    if (file)
        _moo_file_ref (file);

    if (file)
    {
        char *title;
        const char *name;
        name = _moo_file_display_name (file);
        gtk_entry_set_text (GTK_ENTRY (dialog->entry), name);
        title = g_strdup_printf ("%s Properties", name);
        gtk_window_set_title (GTK_WINDOW (dialog), title);
        g_free (title);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (dialog->entry), "");
        gtk_window_set_title (GTK_WINDOW (dialog), "Properties");
    }
}


static void
container_cleanup (GtkContainer *container)
{
    GList *children = gtk_container_get_children (container);
    g_list_foreach (children, (GFunc) g_object_ref, NULL);
    while (children)
    {
        gtk_container_remove (container, children->data);
        g_object_unref (children->data);
        children = g_list_delete_link (children, children);
    }
}

void
_moo_file_props_dialog_set_file (MooFilePropsDialog *dialog,
                                 MooFile            *file,
                                 MooFolder          *folder)
{
    char *text;
    char **info, **p;
    int i;

    g_return_if_fail (MOO_IS_FILE_PROPS_DIALOG (dialog));
    g_return_if_fail ((!file && !folder) || (file && MOO_IS_FOLDER (folder)));

    if (!file)
    {
        gtk_widget_set_sensitive (dialog->notebook, FALSE);
        container_cleanup (GTK_CONTAINER (dialog->table));
        set_file (dialog, NULL, NULL);
        return;
    }

    gtk_widget_set_sensitive (dialog->notebook, TRUE);
    info = _moo_folder_get_file_info (folder, file);
    g_return_if_fail (info != NULL);

    gtk_image_set_from_pixbuf (GTK_IMAGE (dialog->icon),
                               _moo_file_get_icon (file, GTK_WIDGET (dialog), GTK_ICON_SIZE_DIALOG));
    set_file (dialog, file, folder);

    container_cleanup (GTK_CONTAINER (dialog->table));

    for (p = info, i = 0; *p != NULL; ++i)
    {
        GtkWidget *label;

        if (!p[1])
        {
            g_critical ("oops");
            break;
        }

        label = gtk_label_new (NULL);
        text = g_markup_printf_escaped ("<b>%s</b>", *(p++));
        gtk_label_set_markup (GTK_LABEL (label), text);
        g_free (text);
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_table_attach (GTK_TABLE (dialog->table), label, 0, 1, i, i+1,
                          GTK_EXPAND | GTK_FILL, 0, 0, 0);

        label = gtk_label_new (*(p++));
        gtk_label_set_selectable (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_table_attach (GTK_TABLE (dialog->table), label, 1, 2, i, i+1,
                          GTK_EXPAND | GTK_FILL, 0, 0, 0);
    }

    gtk_widget_show_all (dialog->table);

    g_strfreev (info);
}


static void
moo_file_props_dialog_destroy (GtkObject *object)
{
    MooFilePropsDialog *dialog = MOO_FILE_PROPS_DIALOG (object);

    if (dialog->xml)
    {
        if (dialog->file)
            _moo_file_unref (dialog->file);
        if (dialog->folder)
            g_object_unref (dialog->folder);
        dialog->xml = NULL;
        dialog->file = NULL;
        dialog->folder = NULL;
        dialog->notebook = NULL;
        dialog->icon = NULL;
        dialog->entry = NULL;
        dialog->table = NULL;
    }

    GTK_OBJECT_CLASS(_moo_file_props_dialog_parent_class)->destroy (object);
}


static void
moo_file_props_dialog_show (GtkWidget *widget)
{
    MooFilePropsDialog *dialog = MOO_FILE_PROPS_DIALOG (widget);
    GTK_WIDGET_CLASS(_moo_file_props_dialog_parent_class)->show (widget);
    gtk_widget_grab_focus (dialog->entry);
}


GtkWidget *
_moo_file_props_dialog_new (GtkWidget *parent)
{
    GtkWidget *dialog;

    dialog = GTK_WIDGET (g_object_new (MOO_TYPE_FILE_PROPS_DIALOG, (const char*) NULL));

    moo_window_set_parent (dialog, parent);

    return dialog;
}


char*
_moo_file_view_create_folder_dialog (GtkWidget  *parent,
                                     MooFolder  *folder)
{
    CreateFolderXml *xml;
    GtkWidget *dialog;
    char *text, *path, *new_folder_name = NULL;

    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);

    xml = create_folder_xml_new ();
    dialog = GTK_WIDGET (xml->CreateFolder);

    moo_window_set_parent (dialog, parent);

    path = g_filename_display_name (_moo_folder_get_path (folder));
    text = g_strdup_printf ("Create new folder in %s", path);
    gtk_label_set_text (xml->label, text);
    g_free (path);
    g_free (text);

    gtk_entry_set_text (GTK_ENTRY (xml->entry), "New Folder");
    moo_entry_clear_undo (xml->entry);
    gtk_widget_show_all (dialog);
    gtk_widget_grab_focus (GTK_WIDGET (xml->entry));
    gtk_editable_select_region (GTK_EDITABLE (xml->entry), 0, -1);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
        new_folder_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (xml->entry)));
    else
        new_folder_name = NULL;

    gtk_widget_destroy (dialog);
    return new_folder_name;
}


/****************************************************************************/
/* Save droppped contents
 */

#define CLIP_NAME_BASE "clip"
#define CLIP_NAME_SUFFIX ".txt"

static char *
find_available_clip_name (const char *dirname)
{
    int i;

    g_assert (dirname != NULL);

    for (i = 0; i < 1000; ++i)
    {
        char *fullname;
        char *basename;

        if (i)
            basename = g_strdup_printf (CLIP_NAME_BASE "%d" CLIP_NAME_SUFFIX, i);
        else
            basename = g_strdup (CLIP_NAME_BASE CLIP_NAME_SUFFIX);

        fullname = g_build_filename (dirname, basename, NULL);

        if (!g_file_test (fullname, G_FILE_TEST_EXISTS))
        {
            g_free (fullname);
            return basename;
        }

        g_free (fullname);
        g_free (basename);
    }

    for (i = 0; i < 1000; ++i)
    {
        char *fullname;
        char *basename;

        basename = g_strdup_printf (CLIP_NAME_BASE "%08x" CLIP_NAME_SUFFIX,
                                    g_random_int ());
        fullname = g_build_filename (dirname, basename, NULL);

        if (!g_file_test (fullname, G_FILE_TEST_EXISTS))
        {
            g_free (fullname);
            return basename;
        }

        g_free (fullname);
        g_free (basename);
    }

    return g_strdup (CLIP_NAME_BASE CLIP_NAME_SUFFIX);
}


char *
_moo_file_view_save_drop_dialog (GtkWidget  *parent,
                                 const char *dirname)
{
    DropXml *xml;
    GtkWidget *dialog;
    GtkEntry *entry;
    char *start_name, *fullname = NULL;

    g_return_val_if_fail (dirname != NULL, NULL);

    xml = drop_xml_new ();
    dialog = GTK_WIDGET (xml->Drop);

    moo_position_window_at_pointer (dialog, parent);

    entry = GTK_ENTRY (xml->entry);

    start_name = find_available_clip_name (dirname);
    gtk_entry_set_text (entry, start_name);
    moo_entry_clear_undo (MOO_ENTRY (entry));

    gtk_widget_show_all (dialog);
    gtk_widget_grab_focus (GTK_WIDGET (entry));

    moo_bind_bool_property (xml->ok_button, "sensitive", entry, "empty", TRUE);

    while (TRUE)
    {
        const char *text;
        char *name, *err_text, *sec_text;

        if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK)
            goto out;

        text = gtk_entry_get_text (entry);

        if (!text[0])
        {
            g_critical ("oops");
            goto out;
        }

        /* XXX error checking, you know */
        name = g_filename_from_utf8 (text, -1, NULL, NULL, NULL);

        if (!name)
        {
            err_text = g_strdup_printf ("Can not save file as '%s'", text);
            sec_text = g_strdup_printf ("Could not convert '%s' to filename encoding.\n"
                                        "Please consider simpler name, such as foo.blah "
                                        "or blah.foo", text);
            moo_error_dialog (err_text, sec_text, dialog);
            g_free (err_text);
            g_free (sec_text);
            continue;
        }

        fullname = g_build_filename (dirname, name, NULL);
        g_free (name);

        if (!g_file_test (fullname, G_FILE_TEST_EXISTS))
            goto out;

        err_text = g_strdup_printf ("File '%s' already exists", text);
        moo_error_dialog (err_text, NULL, dialog);

        g_free (err_text);
        g_free (fullname);
        fullname = NULL;
    }

out:
    gtk_widget_destroy (dialog);
    g_free (start_name);
    return fullname;
}
