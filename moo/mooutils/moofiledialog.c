/*
 *   moofiledialog.c
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

/**
 * class:MooFileDialog: (parent GObject) (constructable) (moo.private 1)
 **/

#include "config.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooencodings.h"
#include "mooutils/moohelp.h"
#include "mooutils/moofiltermgr.h"
#include "mooutils/mooutils-enums.h"
#include "mooutils/mooi18n.h"
#include "marshals.h"
#include <gtk/gtk.h>
#include <string.h>


struct _MooFileDialogPrivate {
    gboolean multiple;
    char *title;
    char *current_dir;
    char *name;
    MooFileDialogType type;
    GtkWidget *parent;
    char *filter_mgr_id;
    gboolean enable_encodings;

    char *size_prefs_key;
    char *help_id;

    GSList *uris;
    char *uri;
    char *encoding;

    GtkWidget *extra_widget;
    MooFileDialogCheckNameFunc check_name_func;
    gpointer check_name_func_data;
    GDestroyNotify check_name_func_data_notify;
};


G_DEFINE_TYPE (MooFileDialog, moo_file_dialog, G_TYPE_OBJECT)


enum {
    PROP_0,
    PROP_MULTIPLE,
    PROP_TITLE,
    PROP_CURRENT_FOLDER_URI,
    PROP_NAME,
    PROP_TYPE,
    PROP_PARENT,
    PROP_FILTER_MGR_ID,
    PROP_ENABLE_ENCODINGS
};

static char *
get_string_maybe_stock (const char *string)
{
    GtkStockItem item;
    char *underscore;
    char *copy;

    if (!gtk_stock_lookup (string, &item))
        return g_strdup (string);

    if (!(underscore = strchr (item.label, '_')))
        return g_strdup (item.label);

    copy = g_strdup (item.label);
    memmove (copy + (underscore - item.label),
             copy + (underscore - item.label) + 1,
             strlen (copy + (underscore - item.label) + 1) + 1);

    return copy;
}

static void
moo_file_dialog_set_property (GObject        *object,
                              guint           prop_id,
                              const GValue   *value,
                              GParamSpec     *pspec)
{
    char *tmp;
    MooFileDialog *dialog = MOO_FILE_DIALOG (object);

    switch (prop_id)
    {
        case PROP_MULTIPLE:
            dialog->priv->multiple = g_value_get_boolean (value) != 0;
            g_object_notify (object, "multiple");
            break;

        case PROP_TITLE:
            tmp = dialog->priv->title;
            dialog->priv->title = get_string_maybe_stock (g_value_get_string (value));
            g_free (tmp);
            g_object_notify (object, "title");
            break;

        case PROP_CURRENT_FOLDER_URI:
            MOO_ASSIGN_STRING (dialog->priv->current_dir, g_value_get_string (value));
            g_object_notify (object, "current-folder-uri");
            break;

        case PROP_NAME:
            MOO_ASSIGN_STRING (dialog->priv->name, g_value_get_string (value));
            g_object_notify (object, "name");
            break;

        case PROP_TYPE:
            dialog->priv->type = (MooFileDialogType) g_value_get_enum (value);
            g_object_notify (object, "type");
            break;

        case PROP_PARENT:
            dialog->priv->parent = GTK_WIDGET (g_value_get_object (value));
            g_object_notify (object, "parent");
            break;

        case PROP_FILTER_MGR_ID:
            g_free (dialog->priv->filter_mgr_id);
            dialog->priv->filter_mgr_id = g_value_dup_string (value);
            g_object_notify (object, "filter-mgr-id");
            break;

        case PROP_ENABLE_ENCODINGS:
            dialog->priv->enable_encodings = g_value_get_boolean (value);
            g_object_notify (object, "enable-encodings");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_file_dialog_get_property (GObject        *object,
                              guint           prop_id,
                              GValue         *value,
                              GParamSpec     *pspec)
{
    MooFileDialog *dialog = MOO_FILE_DIALOG (object);

    switch (prop_id)
    {
        case PROP_MULTIPLE:
            g_value_set_boolean (value, dialog->priv->multiple != 0);
            break;

        case PROP_TITLE:
            g_value_set_string (value, dialog->priv->title);
            break;

        case PROP_CURRENT_FOLDER_URI:
            g_value_set_string (value, dialog->priv->current_dir);
            break;

        case PROP_NAME:
            g_value_set_string (value, dialog->priv->name);
            break;

        case PROP_TYPE:
            g_value_set_enum (value, dialog->priv->type);
            break;

        case PROP_PARENT:
            g_value_set_object (value, dialog->priv->parent);
            break;

        case PROP_FILTER_MGR_ID:
            g_value_set_string (value, dialog->priv->filter_mgr_id);
            break;

        case PROP_ENABLE_ENCODINGS:
            g_value_set_boolean (value, dialog->priv->enable_encodings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
string_slist_free (GSList *list)
{
    g_slist_foreach (list, (GFunc) g_free, NULL);
    g_slist_free (list);
}


static void
moo_file_dialog_finalize (GObject *object)
{
    MooFileDialog *dialog = MOO_FILE_DIALOG (object);

    g_free (dialog->priv->current_dir);
    g_free (dialog->priv->title);
    g_free (dialog->priv->name);
    g_free (dialog->priv->filter_mgr_id);
    g_free (dialog->priv->uri);
    g_free (dialog->priv->encoding);
    g_free (dialog->priv->help_id);
    g_free (dialog->priv->size_prefs_key);
    string_slist_free (dialog->priv->uris);

    if (dialog->priv->check_name_func_data_notify)
        dialog->priv->check_name_func_data_notify (dialog->priv->check_name_func_data);

    if (dialog->priv->extra_widget)
        g_object_unref (dialog->priv->extra_widget);

    g_free (dialog->priv);

    G_OBJECT_CLASS(moo_file_dialog_parent_class)->finalize (object);
}


static void
moo_file_dialog_init (MooFileDialog *dialog)
{
    dialog->priv = g_new0 (MooFileDialogPrivate, 1);
    dialog->priv->type = MOO_FILE_DIALOG_OPEN;
}


static void
moo_file_dialog_class_init (MooFileDialogClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_file_dialog_finalize;
    gobject_class->set_property = moo_file_dialog_set_property;
    gobject_class->get_property = moo_file_dialog_get_property;

    g_object_class_install_property (gobject_class, PROP_MULTIPLE,
        g_param_spec_boolean ("multiple", "multiple", "multiple",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_ENABLE_ENCODINGS,
        g_param_spec_boolean ("enable-encodings", "enable-encodings", "enable-encodings",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_TITLE,
        g_param_spec_string ("title", "title", "title",
                             NULL, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_CURRENT_FOLDER_URI,
        g_param_spec_string ("current-folder-uri", "current-folder-uri",
                             "current-folder-uri", NULL,
                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_NAME,
        g_param_spec_string ("name", "name", "name", NULL,
                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_PARENT,
        g_param_spec_object ("parent", "parent", "parent",
                             GTK_TYPE_WIDGET,
                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_TYPE,
        g_param_spec_enum ("type", "type", "type",
                           MOO_TYPE_FILE_DIALOG_TYPE, MOO_FILE_DIALOG_OPEN,
                           (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_FILTER_MGR_ID,
        g_param_spec_string ("filter-mgr-id", "filter-mgr-id", "filter-mgr-id",
                             NULL, (GParamFlags) G_PARAM_READWRITE));
}


inline static
GtkWidget *file_chooser_dialog_new (const char *title,
                                    GtkFileChooserAction action,
                                    const char *okbtn,
                                    const char *start_dir_uri,
                                    const char *help_id)
{
    GtkWidget *dialog =
            gtk_file_chooser_dialog_new (title, NULL, action,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         okbtn, GTK_RESPONSE_OK,
                                         NULL);

    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    if (start_dir_uri)
        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER(dialog),
                                                 start_dir_uri);


    if (help_id)
    {
        moo_help_set_id (dialog, help_id);
        moo_help_connect_keys (dialog);
        gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
    }

    return dialog;
}

#define file_chooser_set_select_multiple(dialog,multiple) \
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), multiple)
#define file_chooser_set_name(dialog, name) \
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), name)

const char *
moo_file_dialog (GtkWidget  *parent,
                 MooFileDialogType type,
                 const char *start_name,
                 const char *title,
                 const char *start_dir)
{
    static char *filename;
    MooFileDialog *dialog;
    GFile *start;
    GFile *file;

    start = start_dir ? g_file_new_for_path (start_dir) : NULL;

    dialog = moo_file_dialog_new (type, parent, FALSE, title, start, start_name);
    g_return_val_if_fail (dialog != NULL, NULL);

    moo_file_dialog_run (dialog);

    file = moo_file_dialog_get_file (dialog);

    g_free (filename);
    filename = file ? g_file_get_path (file) : NULL;

    g_object_unref (file);
    g_object_unref (start);
    g_object_unref (dialog);
    return filename;
}


static GtkWidget *
moo_file_dialog_create_widget (MooFileDialog *dialog)
{
    GtkFileChooserAction chooser_action;
    GtkWidget *widget = NULL;
    GtkWidget *extra_box = NULL;

    switch (dialog->priv->type)
    {
        case MOO_FILE_DIALOG_OPEN:
        case MOO_FILE_DIALOG_OPEN_ANY:
        case MOO_FILE_DIALOG_OPEN_DIR:
            if (dialog->priv->type == MOO_FILE_DIALOG_OPEN_DIR)
                chooser_action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
            else
                chooser_action = GTK_FILE_CHOOSER_ACTION_OPEN;

            widget = file_chooser_dialog_new (dialog->priv->title,
                                              chooser_action,
                                              GTK_STOCK_OPEN,
                                              dialog->priv->current_dir,
                                              dialog->priv->help_id);
            file_chooser_set_select_multiple (widget, dialog->priv->multiple);
            break;

        case MOO_FILE_DIALOG_SAVE:
            chooser_action = GTK_FILE_CHOOSER_ACTION_SAVE;

            widget = file_chooser_dialog_new (dialog->priv->title,
                                              chooser_action,
                                              GTK_STOCK_SAVE,
                                              dialog->priv->current_dir,
                                              dialog->priv->help_id);

            if (dialog->priv->name)
                file_chooser_set_name (widget, dialog->priv->name);
            break;

        default:
            g_critical ("incorrect dialog type specified");
            return NULL;
    }

    gtk_dialog_set_default_response (GTK_DIALOG (widget), GTK_RESPONSE_OK);

    if (dialog->priv->filter_mgr_id || dialog->priv->enable_encodings)
    {
        extra_box = gtk_hbox_new (FALSE, 0);
        gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (widget), extra_box);
        gtk_widget_show (extra_box);
    }

    if (dialog->priv->filter_mgr_id)
        moo_filter_mgr_attach (moo_filter_mgr_default (),
                               GTK_FILE_CHOOSER (widget), extra_box,
                               dialog->priv->filter_mgr_id);

    if (dialog->priv->enable_encodings)
        _moo_encodings_attach_combo (widget, extra_box,
                                     dialog->priv->type == MOO_FILE_DIALOG_SAVE,
                                     dialog->priv->encoding);

    if (dialog->priv->extra_widget)
        gtk_box_pack_start (GTK_BOX (extra_box), dialog->priv->extra_widget,
                            FALSE, FALSE, 0);

    if (dialog->priv->size_prefs_key)
        _moo_window_set_remember_size (GTK_WINDOW (widget),
                                       dialog->priv->size_prefs_key, -1, -1, TRUE);
    if (dialog->priv->parent)
        moo_window_set_parent (widget, dialog->priv->parent);

    return widget;
}


static char **
string_slist_to_strv (GSList *list)
{
    guint len, i;
    char **uris;

    if (!list)
        return NULL;

    len = g_slist_length (list);
    uris = g_new (char*, len + 1);

    for (i = 0; i < len; i++, list = list->next)
        uris[i] = g_strdup ((const char*) list->data);

    uris[i] = NULL;
    return uris;
}

static MooFileArray *
uri_list_to_files (GSList *list)
{
    MooFileArray *flocs;

    if (!list)
        return NULL;

    flocs = moo_file_array_new ();

    while (list)
    {
        moo_file_array_take (flocs, g_file_new_for_uri (list->data));
        list = list->next;
    }

    return flocs;
}


static void
set_uri (MooFileDialog *dialog,
         char          *uri)
{
    g_free (dialog->priv->uri);
    dialog->priv->uri = uri;
}

static void
set_uris (MooFileDialog *dialog,
          GSList        *uris)
{
    string_slist_free (dialog->priv->uris);
    dialog->priv->uris = uris;
}


static void
set_encoding (MooFileDialog *dialog,
              const char    *encoding)
{
    MOO_ASSIGN_STRING (dialog->priv->encoding, encoding);
}

void
moo_file_dialog_set_encoding (MooFileDialog *dialog,
                              const char    *encoding)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));
    set_encoding (dialog, encoding);
}


static gboolean
uri_is_valid (G_GNUC_UNUSED const char *uri,
              G_GNUC_UNUSED char      **msg)
{
#ifndef __WIN32__
    return TRUE;
#else
    struct name {
        const char *name;
        guint len;
    } names[] = {
        { "con", 3 }, { "aux", 3 }, { "prn", 3 }, { "nul", 3 },
        { "com1", 4 }, { "com2", 4 }, { "com3", 4 }, { "com4", 4 },
        { "lpt1", 4 }, { "lpt2", 4 }, { "lpt3", 4 }
    };
    guint i;
    char *filename;
    const char *basename;
    gboolean invalid = FALSE;

    if (!(filename = g_filename_from_uri (uri, NULL, NULL)))
        return TRUE;

    basename = strrchr (filename, '\\');

    if (!basename)
        basename = strrchr (filename, '/');

    if (!basename)
        basename = filename;
    else
        basename += 1;

    for (i = 0; i < G_N_ELEMENTS (names); ++i)
    {
        if (!g_ascii_strncasecmp (basename, names[i].name, names[i].len))
        {
            if (!basename[names[i].len] || basename[names[i].len] == '.')
                invalid = TRUE;
            break;
        }
    }

    if (invalid)
    {
        char *base = g_path_get_basename (filename);
        *msg = g_strdup_printf ("Filename '%s' is a reserved device name.\n"
                                "Please choose another name", base);
        g_free (base);
    }

    g_free (filename);
    return !invalid;
#endif
}


static char *
get_uri_for_saving (MooFileDialog *dialog,
                    GtkWidget     *filechooser)
{
    char *uri;
    char *real;

    if (!(uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filechooser))) ||
        !dialog->priv->check_name_func)
            return uri;

    real = dialog->priv->check_name_func (dialog, uri,
                                          dialog->priv->check_name_func_data);

    g_free (uri);
    return real;
}

gboolean
moo_file_dialog_run (MooFileDialog *dialog)
{
    GtkWidget *filechooser;
    gboolean result = FALSE;
    int response;

    g_return_val_if_fail (MOO_IS_FILE_DIALOG (dialog), FALSE);

    set_uri (dialog, NULL);
    set_uris (dialog, NULL);

    filechooser = moo_file_dialog_create_widget (dialog);

    switch (dialog->priv->type)
    {
        case MOO_FILE_DIALOG_OPEN:
        case MOO_FILE_DIALOG_OPEN_ANY:
        case MOO_FILE_DIALOG_OPEN_DIR:
            while ((response = gtk_dialog_run (GTK_DIALOG (filechooser))) == GTK_RESPONSE_HELP)
                moo_help_open (filechooser);
            if (response == GTK_RESPONSE_OK)
            {
                set_uri (dialog, gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filechooser)));

                if (dialog->priv->multiple)
                    set_uris (dialog, gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (filechooser)));

                result = TRUE;
            }
            goto out;

        case MOO_FILE_DIALOG_SAVE:
            while (TRUE)
            {
                char *msg;
                char *filename;
                char *uri;

                response = gtk_dialog_run (GTK_DIALOG (filechooser));

                if (response == GTK_RESPONSE_HELP)
                {
                    moo_help_open (filechooser);
                    continue;
                }

                if (response != GTK_RESPONSE_OK)
                    goto out;

                uri = get_uri_for_saving (dialog, filechooser);

                if (!uri_is_valid (uri, &msg))
                {
                    moo_error_dialog (msg, NULL, filechooser);
                    g_free (uri);
                    g_free (msg);
                    continue;
                }

                if ((filename = g_filename_from_uri (uri, NULL, NULL)))
                {
                    if (g_file_test (filename, G_FILE_TEST_EXISTS) &&
                        !g_file_test (filename, G_FILE_TEST_IS_REGULAR))
                    {
                        moo_error_dialog (_("Selected file is not a regular file"),
                                          NULL,
                                          filechooser);
                        g_free (filename);
                        g_free (uri);
                        continue;
                    }

                    if (g_file_test (filename, G_FILE_TEST_EXISTS) &&
                        g_file_test (filename, G_FILE_TEST_IS_REGULAR))
                    {
                        char *basename = g_path_get_basename (filename);
                        char *dirname = g_path_get_dirname (filename);
                        char *display_name = g_filename_display_name (basename);
                        char *display_dirname = g_filename_display_name (dirname);
                        gboolean overwrite;

                        overwrite = moo_overwrite_file_dialog (display_name, display_dirname, filechooser);

                        g_free (basename);
                        g_free (dirname);
                        g_free (display_name);
                        g_free (display_dirname);

                        if (!overwrite)
                        {
                            g_free (filename);
                            g_free (uri);
                            continue;
                        }
                    }
                }

                set_uri (dialog, uri);
                g_free (filename);
                result = TRUE;
                goto out;
            }

        default:
            g_critical ("incorrect dialog type specified");
    }

out:
    if (result && dialog->priv->enable_encodings)
        set_encoding (dialog, _moo_encodings_combo_get (filechooser,
                                    dialog->priv->type == MOO_FILE_DIALOG_SAVE));

    if (result)
    {
        g_free (dialog->priv->current_dir);
        dialog->priv->current_dir =
            gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (filechooser));
    }

    gtk_widget_destroy (filechooser);
    return result;
}


void
moo_file_dialog_set_check_name_func (MooFileDialog  *dialog,
                                     MooFileDialogCheckNameFunc func,
                                     gpointer        data,
                                     GDestroyNotify  notify)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));

    if (dialog->priv->check_name_func_data_notify)
        dialog->priv->check_name_func_data_notify (dialog->priv->check_name_func_data);

    dialog->priv->check_name_func = func;
    dialog->priv->check_name_func_data = data;
    dialog->priv->check_name_func_data_notify = notify;
}

void
moo_file_dialog_set_extra_widget (MooFileDialog *dialog,
                                  GtkWidget     *widget)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));
    g_return_if_fail (!widget || GTK_IS_WIDGET (widget));

    if (widget)
        g_object_ref_sink (widget);
    if (dialog->priv->extra_widget)
        g_object_unref (dialog->priv->extra_widget);
    dialog->priv->extra_widget = widget;
}


GFile *
moo_file_dialog_get_file (MooFileDialog *dialog)
{
    g_return_val_if_fail (MOO_IS_FILE_DIALOG (dialog), NULL);
    return dialog->priv->uri ? g_file_new_for_uri (dialog->priv->uri) : NULL;
}

char *
moo_file_dialog_get_uri (MooFileDialog *dialog)
{
    g_return_val_if_fail (MOO_IS_FILE_DIALOG (dialog), NULL);
    return g_strdup (dialog->priv->uri);
}

MooFileArray *
moo_file_dialog_get_files (MooFileDialog *dialog)
{
    g_return_val_if_fail (MOO_IS_FILE_DIALOG (dialog), NULL);
    return uri_list_to_files (dialog->priv->uris);
}

char **
moo_file_dialog_get_uris (MooFileDialog *dialog)
{
    return string_slist_to_strv (dialog->priv->uris);
}


const char *
moo_file_dialog_get_encoding (MooFileDialog *dialog)
{
    g_return_val_if_fail (MOO_IS_FILE_DIALOG (dialog), NULL);
    return dialog->priv->encoding ? dialog->priv->encoding : MOO_ENCODING_UTF8;
}


const char *
moo_file_dialogp (GtkWidget          *parent,
                  MooFileDialogType   type,
                  const char         *start_name,
                  const char         *title,
                  const char         *prefs_key,
                  const char         *alternate_prefs_key)
{
    const char *start = NULL;
    const char *filename = NULL;

    if (!title)
        title = "Choose File";

    if (prefs_key)
    {
        moo_prefs_create_key (prefs_key, MOO_PREFS_STATE, G_TYPE_STRING, NULL);
        start = moo_prefs_get_string (prefs_key);
    }

    if (!start && alternate_prefs_key)
    {
        moo_prefs_create_key (alternate_prefs_key, MOO_PREFS_STATE, G_TYPE_STRING, NULL);
        start = moo_prefs_get_string (alternate_prefs_key);
    }

    filename = moo_file_dialog (parent, type, start_name, title, start);

    if (filename && prefs_key)
    {
        char *new_start = g_path_get_dirname (filename);
        moo_prefs_create_key (prefs_key, MOO_PREFS_STATE, G_TYPE_STRING, NULL);
        moo_prefs_set_filename (prefs_key, new_start);
        g_free (new_start);
    }

    return filename;
}


MooFileDialog *
moo_file_dialog_new (MooFileDialogType type,
                     GtkWidget      *parent,
                     gboolean        multiple,
                     const char     *title,
                     GFile          *start_loc,
                     const char     *start_name)
{
    char *start_dir_uri = NULL;
    MooFileDialog *dialog;

    if (start_loc)
        start_dir_uri = g_file_get_uri (start_loc);

    dialog = MOO_FILE_DIALOG (
             g_object_new (MOO_TYPE_FILE_DIALOG,
                           "parent", parent,
                           "type", type,
                           "multiple", multiple,
                           "title", title,
                           "current-folder-uri", start_dir_uri,
                           "name", start_name,
                           (const char*) NULL));

    g_free (start_dir_uri);
    return dialog;
}


void
moo_file_dialog_set_filter_mgr_id (MooFileDialog *dialog,
                                   const char    *id)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));
    g_object_set (dialog, "filter-mgr-id", id, NULL);
}


void
moo_file_dialog_set_help_id (MooFileDialog *dialog,
                             const char    *id)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));
    MOO_ASSIGN_STRING (dialog->priv->help_id, id);
}


void
moo_file_dialog_set_remember_size (MooFileDialog *dialog,
                                   const char    *prefs_key)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));
    g_return_if_fail (prefs_key != NULL);
    MOO_ASSIGN_STRING (dialog->priv->size_prefs_key, prefs_key);
}


void
moo_file_dialog_set_current_folder_uri (MooFileDialog *dialog,
                                        const char    *uri)
{
    g_return_if_fail (MOO_IS_FILE_DIALOG (dialog));
    MOO_ASSIGN_STRING (dialog->priv->current_dir, uri);
}

const char *
moo_file_dialog_get_current_folder_uri (MooFileDialog *dialog)
{
    g_return_val_if_fail (MOO_IS_FILE_DIALOG (dialog), NULL);
    return dialog->priv->current_dir;
}
