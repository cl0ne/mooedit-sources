/*
 *   moocommand.c
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
#include <config.h>
#endif

#include "moocommand-private.h"
#include "moocommand-script.h"
#include "moocommand-exe.h"
#include "moooutputfilterregex.h"
#include "mooedit/mooeditwindow.h"
#include "mooedit/mooeditor.h"
#include "mooedit/mooedit-enums.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooutils-cpp.h"
#include <gtk/gtk.h>
#include <mooglib/moo-glib.h>
#include <string.h>
#include <stdio.h>

G_DEFINE_TYPE (MooCommand, moo_command, G_TYPE_OBJECT)
G_DEFINE_TYPE (MooCommandFactory, moo_command_factory, G_TYPE_OBJECT)
G_DEFINE_TYPE (MooCommandContext, moo_command_context, G_TYPE_OBJECT)
MOO_DEFINE_BOXED_TYPE_R (MooCommandData, moo_command_data)

enum {
    CTX_PROP_0,
    CTX_PROP_DOC,
    CTX_PROP_WINDOW
};

enum {
    CMD_PROP_0,
    CMD_PROP_OPTIONS
};

typedef struct {
    GValue value;
} Variable;

struct _MooCommandContextPrivate {
    MooEditWindow *window;
    MooEdit *doc;
};

struct _MooCommandData {
    guint ref_count;
    char **data;
    guint len;
    char *code;
};

typedef struct {
    char *id;
    char *name;
    MooCommandFilterFactory factory_func;
    gpointer data;
    GDestroyNotify data_notify;
} FilterInfo;


static GHashTable *registered_factories;
static GHashTable *registered_filters;


static void         moo_command_data_take_code          (MooCommandData     *data,
                                                         char               *code);

static MooCommand  *_moo_command_factory_create_command (MooCommandFactory  *factory,
                                                         MooCommandData     *data,
                                                         const char         *options);


MooCommand *
moo_command_create (const char     *name,
                    const char     *options,
                    MooCommandData *data)
{
    MooCommandFactory *factory;

    g_return_val_if_fail (name != NULL, NULL);

    factory = moo_command_factory_lookup (name);
    g_return_val_if_fail (factory != NULL, NULL);

    return _moo_command_factory_create_command (factory, data, options);
}


void
moo_command_factory_register (const char        *name,
                              const char        *display_name,
                              MooCommandFactory *factory,
                              char             **keys,
                              const char        *extension)
{
    MooCommandFactoryClass *klass;

    g_return_if_fail (name != NULL);
    g_return_if_fail (display_name != NULL);
    g_return_if_fail (MOO_IS_COMMAND_FACTORY (factory));

    klass = MOO_COMMAND_FACTORY_GET_CLASS (factory);
    g_return_if_fail (klass->create_command != NULL);
    g_return_if_fail (klass->create_widget != NULL);
    g_return_if_fail (klass->load_data != NULL);
    g_return_if_fail (klass->save_data != NULL);
    g_return_if_fail (klass->data_equal != NULL);

    if (registered_factories != NULL)
    {
        MooCommandFactory *old = (MooCommandFactory*) g_hash_table_lookup (registered_factories, name);

        if (old)
        {
            _moo_message ("command factory '%s' already registered", name);
            g_hash_table_remove (registered_factories, name);
            g_object_unref (old);
        }
    }

    if (!registered_factories)
        registered_factories = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    factory->name = g_strdup (name);
    factory->display_name = g_strdup (display_name);
    factory->keys = g_strdupv (keys);
    factory->n_keys = keys ? g_strv_length (keys) : 0;
    factory->extension = g_strdup (extension);

    g_hash_table_insert (registered_factories, g_strdup (name), g_object_ref (factory));
}


MooCommandFactory *
moo_command_factory_lookup (const char *name)
{
    MooCommandFactory *factory = NULL;

    g_return_val_if_fail (name != NULL, NULL);

    if (registered_factories != NULL)
        factory = (MooCommandFactory*) g_hash_table_lookup (registered_factories, name);

    return factory;
}


static void
add_factory_hash_cb (G_GNUC_UNUSED const char *name,
                     MooCommandFactory *factory,
                     GSList **list)
{
    *list = g_slist_prepend (*list, factory);
}

GSList *
moo_command_list_factories (void)
{
    GSList *list = NULL;

    if (registered_factories)
        g_hash_table_foreach (registered_factories, (GHFunc) add_factory_hash_cb, &list);

    return g_slist_reverse (list);
}


static void
moo_command_factory_init (G_GNUC_UNUSED MooCommandFactory *factory)
{
}

static void
moo_command_factory_finalize (GObject *object)
{
    MooCommandFactory *factory = MOO_COMMAND_FACTORY (object);
    g_free (factory->name);
    g_free (factory->display_name);
    g_strfreev (factory->keys);
    G_OBJECT_CLASS (moo_command_factory_parent_class)->finalize (object);
}

static GtkWidget *
dummy_create_widget (G_GNUC_UNUSED MooCommandFactory *factory)
{
    return gtk_vbox_new (FALSE, FALSE);
}

static void
dummy_load_data (G_GNUC_UNUSED MooCommandFactory *factory,
                 G_GNUC_UNUSED GtkWidget *page,
                 G_GNUC_UNUSED MooCommandData *data)
{
}

static gboolean
dummy_save_data (G_GNUC_UNUSED MooCommandFactory *factory,
                 G_GNUC_UNUSED GtkWidget *page,
                 G_GNUC_UNUSED MooCommandData *data)
{
    return FALSE;
}

static gboolean
default_data_equal (MooCommandFactory *factory,
                    MooCommandData    *data1,
                    MooCommandData    *data2)
{
    guint i;
    const char *val1, *val2;

    for (i = 0; i < factory->n_keys; ++i)
    {
        val1 = moo_command_data_get (data1, i);
        val2 = moo_command_data_get (data2, i);
        if (!moo_str_equal (val1, val2))
            return FALSE;
    }

    val1 = moo_command_data_get_code (data1);
    val2 = moo_command_data_get_code (data2);
    return moo_str_equal (val1, val2);
}

static void
moo_command_factory_class_init (MooCommandFactoryClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = moo_command_factory_finalize;
    klass->create_widget = dummy_create_widget;
    klass->load_data = dummy_load_data;
    klass->save_data = dummy_save_data;
    klass->data_equal = default_data_equal;
}


static MooCommand *
_moo_command_factory_create_command (MooCommandFactory *factory,
                                     MooCommandData    *data,
                                     const char        *options)
{
    MooCommand *cmd;

    g_return_val_if_fail (MOO_IS_COMMAND_FACTORY (factory), NULL);
    g_return_val_if_fail (MOO_COMMAND_FACTORY_GET_CLASS(factory)->create_command != NULL, NULL);

    if (data)
        moo_command_data_ref (data);
    else
        data = moo_command_data_new (factory->n_keys);

    cmd = MOO_COMMAND_FACTORY_GET_CLASS (factory)->create_command (factory, data, options);

    moo_command_data_unref (data);
    return cmd;
}

GtkWidget *
_moo_command_factory_create_widget (MooCommandFactory *factory)
{
    g_return_val_if_fail (MOO_IS_COMMAND_FACTORY (factory), NULL);
    g_return_val_if_fail (MOO_COMMAND_FACTORY_GET_CLASS(factory)->create_widget != NULL, NULL);
    return MOO_COMMAND_FACTORY_GET_CLASS (factory)->create_widget (factory);
}

void
_moo_command_factory_load_data (MooCommandFactory *factory,
                                GtkWidget         *widget,
                                MooCommandData    *data)
{
    g_return_if_fail (MOO_IS_COMMAND_FACTORY (factory));
    g_return_if_fail (MOO_COMMAND_FACTORY_GET_CLASS(factory)->load_data != NULL);
    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (data != NULL);
    MOO_COMMAND_FACTORY_GET_CLASS(factory)->load_data (factory, widget, data);
}

gboolean
_moo_command_factory_save_data (MooCommandFactory *factory,
                                GtkWidget         *widget,
                                MooCommandData    *data)
{
    g_return_val_if_fail (MOO_IS_COMMAND_FACTORY (factory), FALSE);
    g_return_val_if_fail (MOO_COMMAND_FACTORY_GET_CLASS(factory)->save_data != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
    g_return_val_if_fail (data != NULL, FALSE);
    return MOO_COMMAND_FACTORY_GET_CLASS (factory)->save_data (factory, widget, data);
}

gboolean
_moo_command_factory_data_equal (MooCommandFactory *factory,
                                 MooCommandData    *data1,
                                 MooCommandData    *data2)
{
    g_return_val_if_fail (MOO_IS_COMMAND_FACTORY (factory), FALSE);
    g_return_val_if_fail (MOO_COMMAND_FACTORY_GET_CLASS(factory)->data_equal != NULL, FALSE);
    g_return_val_if_fail (data1 != NULL, FALSE);
    g_return_val_if_fail (data2 != NULL, FALSE);
    return MOO_COMMAND_FACTORY_GET_CLASS (factory)->data_equal (factory, data1, data2);
}


static void
moo_command_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
    MooCommand *cmd = MOO_COMMAND (object);

    switch (property_id)
    {
        case CMD_PROP_OPTIONS:
            moo_command_set_options (cmd, (MooCommandOptions) g_value_get_flags (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_command_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
    MooCommand *cmd = MOO_COMMAND (object);

    switch (property_id)
    {
        case CMD_PROP_OPTIONS:
            g_value_set_flags (value, cmd->options);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static gboolean
moo_command_check_sensitive_real (MooCommand    *cmd,
                                  MooEdit       *doc)
{
    if ((cmd->options & MOO_COMMAND_NEED_DOC) && !doc)
        return FALSE;

    if ((cmd->options & MOO_COMMAND_NEED_FILE) && (!MOO_IS_EDIT (doc) || moo_edit_is_untitled (doc)))
        return FALSE;

    return TRUE;
}


static void
moo_command_class_init (MooCommandClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = moo_command_set_property;
    object_class->get_property = moo_command_get_property;

    klass->check_sensitive = moo_command_check_sensitive_real;

    g_object_class_install_property (object_class, CMD_PROP_OPTIONS,
                                     g_param_spec_flags ("options", "options", "options",
                                                         MOO_TYPE_COMMAND_OPTIONS,
                                                         0,
                                                         (GParamFlags) G_PARAM_READWRITE));
}


static void
moo_command_init (G_GNUC_UNUSED MooCommand *cmd)
{
}


static MooCommandOptions
check_options (MooCommandOptions options)
{
    MooCommandOptions checked = options;

    if (options & MOO_COMMAND_NEED_SAVE_ALL)
        checked |= MOO_COMMAND_NEED_SAVE;
    if (options & MOO_COMMAND_NEED_FILE)
        checked |= MOO_COMMAND_NEED_DOC;
    if (options & MOO_COMMAND_NEED_SAVE)
        checked |= MOO_COMMAND_NEED_DOC;

    return checked;
}


static gboolean
save_one (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);

    if (moo_edit_is_modified (doc) && !moo_edit_save (doc, NULL))
        return FALSE;

    return TRUE;
}

static gboolean
save_all (void)
{
    guint i;
    MooEditArray *docs;
    gboolean result = TRUE;

    docs = moo_editor_get_docs (moo_editor_instance ());

    for (i = 0; i < docs->n_elms; ++i)
    {
        if (!moo_edit_is_untitled(docs->elms[i]) && !save_one (docs->elms[i]))
        {
            result = FALSE;
            break;
        }
    }

    moo_edit_array_free (docs);
    return result;
}


static gboolean
check_context (MooCommandOptions  options,
               MooEdit           *doc)
{
    g_return_val_if_fail (!doc || MOO_IS_EDIT (doc), FALSE);

    if ((options & MOO_COMMAND_NEED_DOC) && !doc)
        return FALSE;

    if ((options & MOO_COMMAND_NEED_FILE) && !(doc && !moo_edit_is_untitled (doc)))
            return FALSE;

    if ((options & MOO_COMMAND_NEED_SAVE) && !doc)
        return FALSE;

    return TRUE;
}

void
moo_command_run (MooCommand         *cmd,
                 MooCommandContext  *ctx)
{
    MooEdit *doc;

    g_return_if_fail (MOO_IS_COMMAND (cmd));
    g_return_if_fail (MOO_IS_COMMAND_CONTEXT (ctx));
    g_return_if_fail (MOO_COMMAND_GET_CLASS (cmd)->run != NULL);

    doc = moo_command_context_get_doc (ctx);

    g_return_if_fail (check_context (cmd->options, doc));

    if (cmd->options & MOO_COMMAND_NEED_SAVE_ALL)
    {
        if (!save_all ())
            return;
    }
    else if (cmd->options & MOO_COMMAND_NEED_SAVE)
    {
        if (!save_one (doc) || moo_edit_is_untitled (doc))
            return;
    }

    MOO_COMMAND_GET_CLASS (cmd)->run (cmd, ctx);
}


gboolean
moo_command_check_sensitive (MooCommand    *cmd,
                             MooEdit       *doc)
{
    g_return_val_if_fail (MOO_IS_COMMAND (cmd), FALSE);
    g_return_val_if_fail (!doc || MOO_IS_EDIT (doc), FALSE);
    g_return_val_if_fail (MOO_COMMAND_GET_CLASS(cmd)->check_sensitive != NULL, FALSE);
    return MOO_COMMAND_GET_CLASS(cmd)->check_sensitive (cmd, doc);
}


void
moo_command_set_options (MooCommand       *cmd,
                         MooCommandOptions options)
{
    g_return_if_fail (MOO_IS_COMMAND (cmd));

    options = check_options (options);

    if (options != cmd->options)
    {
        cmd->options = options;
        g_object_notify (G_OBJECT (cmd), "options");
    }
}


MooCommandOptions
moo_parse_command_options (const char *string)
{
    MooCommandOptions options = MOO_COMMAND_OPTIONS_NONE;
    char **pieces, **p;

    if (!string)
        return MOO_COMMAND_OPTIONS_NONE;

    pieces = g_strsplit_set (string, " \t\r\n;,", 0);

    if (!pieces || !pieces[0])
        goto out;

    for (p = pieces; *p != NULL; ++p)
    {
        char *s = *p;

        g_strdelimit (g_strstrip (s), "_", '-');

        if (!*s)
            continue;

        if (!strcmp (s, "need-doc"))
            options |= MOO_COMMAND_NEED_DOC;
        else if (!strcmp (s, "need-file"))
            options |= MOO_COMMAND_NEED_FILE;
        else if (!strcmp (s, "need-save") || !strcmp (s, "save"))
            options |= MOO_COMMAND_NEED_SAVE;
        else if (!strcmp (s, "need-save-all") || !strcmp (s, "save-all"))
            options |= MOO_COMMAND_NEED_SAVE_ALL;
        else
            g_warning ("unknown option '%s'", s);
    }

out:
    g_strfreev (pieces);
    return options;
}


static void
moo_command_context_dispose (GObject *object)
{
    MooCommandContext *ctx = MOO_COMMAND_CONTEXT (object);

    if (ctx->priv)
    {
        if (ctx->priv->window)
            g_object_unref (ctx->priv->window);
        if (ctx->priv->doc)
            g_object_unref (ctx->priv->doc);
        ctx->priv = NULL;
    }

    G_OBJECT_CLASS (moo_command_context_parent_class)->dispose (object);
}


static void
moo_command_context_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
    MooCommandContext *ctx = MOO_COMMAND_CONTEXT (object);

    switch (property_id)
    {
        case CTX_PROP_DOC:
            moo_command_context_set_doc (ctx, static_cast<MooEdit*> (g_value_get_object (value)));
            break;
        case CTX_PROP_WINDOW:
            moo_command_context_set_window (ctx, static_cast<MooEditWindow*> (g_value_get_object (value)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_command_context_get_property (GObject *object,
                                  guint property_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
    MooCommandContext *ctx = MOO_COMMAND_CONTEXT (object);

    switch (property_id)
    {
        case CTX_PROP_DOC:
            g_value_set_object (value, ctx->priv->doc);
            break;
        case CTX_PROP_WINDOW:
            g_value_set_object (value, ctx->priv->window);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_command_context_class_init (MooCommandContextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = moo_command_context_dispose;
    object_class->set_property = moo_command_context_set_property;
    object_class->get_property = moo_command_context_get_property;

    g_type_class_add_private (klass, sizeof (MooCommandContextPrivate));

    g_object_class_install_property (object_class, CTX_PROP_DOC,
        g_param_spec_object ("doc", "doc", "doc",
                             MOO_TYPE_EDIT, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (object_class, CTX_PROP_WINDOW,
        g_param_spec_object ("window", "window", "window",
                             MOO_TYPE_EDIT_WINDOW,
                             (GParamFlags) G_PARAM_READWRITE));
}


static void
moo_command_context_init (MooCommandContext *ctx)
{
    ctx->priv = G_TYPE_INSTANCE_GET_PRIVATE (ctx, MOO_TYPE_COMMAND_CONTEXT, MooCommandContextPrivate);
}


MooCommandContext *
moo_command_context_new (MooEdit       *doc,
                         MooEditWindow *window)
{
    g_return_val_if_fail (!doc || MOO_IS_EDIT (doc), NULL);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);

    return g::object_new<MooCommandContext>("doc", doc, "window", window);
}


void
moo_command_context_set_doc (MooCommandContext *ctx,
                             MooEdit           *doc)
{
    g_return_if_fail (MOO_IS_COMMAND_CONTEXT (ctx));
    g_return_if_fail (!doc || MOO_IS_EDIT (doc));

    if (ctx->priv->doc != doc)
    {
        if (ctx->priv->doc)
            g_object_unref (ctx->priv->doc);
        if (doc)
            g_object_ref (doc);
        ctx->priv->doc = doc;
        g_object_notify (G_OBJECT (ctx), "doc");
    }
}


void
moo_command_context_set_window (MooCommandContext *ctx,
                                MooEditWindow     *window)
{
    g_return_if_fail (MOO_IS_COMMAND_CONTEXT (ctx));
    g_return_if_fail (!window || MOO_IS_EDIT_WINDOW (window));

    if (ctx->priv->window != window)
    {
        if (ctx->priv->window)
            g_object_unref (ctx->priv->window);
        if (window)
            g_object_ref (window);
        ctx->priv->window = window;
        g_object_notify (G_OBJECT (ctx), "window");
    }
}


MooEdit *
moo_command_context_get_doc (MooCommandContext *ctx)
{
    g_return_val_if_fail (MOO_IS_COMMAND_CONTEXT (ctx), NULL);
    return ctx->priv->doc;
}


MooEditWindow *
moo_command_context_get_window (MooCommandContext *ctx)
{
    g_return_val_if_fail (MOO_IS_COMMAND_CONTEXT (ctx), NULL);
    return ctx->priv->window;
}


static int
find_key (MooCommandFactory *factory,
          const char        *key)
{
    guint i;

    for (i = 0; i < factory->n_keys; ++i)
        if (!strcmp (factory->keys[i], key))
            return i;

    return -1;
}


static void
get_one_option (MooCommandFactory *factory,
                MooCommandData    *cmd_data,
                const char        *key,
                const char        *value,
                GPtrArray        **options)
{
    int index;

    index = find_key (factory, key);

    if (index >= 0)
    {
        moo_command_data_set (cmd_data, index, value);
    }
    else
    {
        if (!*options)
            *options = g_ptr_array_new ();
        g_ptr_array_add (*options, g_strdup (key));
        g_ptr_array_add (*options, g_strdup (value));
    }
}

static void
get_options_from_contents (MooCommandFactory *factory,
                           MooCommandData    *cmd_data,
                           const char        *contents,
                           GPtrArray        **options,
                           const char        *filename)
{
    guint i;
    guint line_ends;
    const char *start = NULL;
    const char *end = NULL;
    char **p, **comps;
    char *opt_string;

    for (i = 0, line_ends = 0; i < 2048 && contents[i]; i++)
    {
        if (contents[i] == '\n' || contents[i] == '\r')
        {
            if (line_ends)
                break;
            if (contents[i] == '\r' && contents[i+1] == '\n')
                i += 1;
            line_ends += 1;
        }
        else if (contents[i] == '!' && contents[i+1] == '!')
        {
            if (!start)
            {
                start = contents + i + 2;
                i += 1;
            }
            else
            {
                end = contents + i;
                break;
            }
        }
    }

    if (!start || !end)
        return;

    opt_string = g_strndup (start, end - start);
    comps = g_strsplit (opt_string, ";", 0);

    for (p = comps; p && *p; ++p)
    {
        char *eq = strchr (*p, '=');

        if (!eq)
        {
            g_warning ("%s: invalid option '%s' in file %s", G_STRFUNC, *p, filename);
        }
        else
        {
            char *key = *p;
            char *value = eq + 1;
            *eq = 0;
            g_strstrip (key);
            g_strstrip (value);
            get_one_option (factory, cmd_data, key, value, options);
        }
    }

    g_strfreev (comps);
    g_free (opt_string);
}

#ifndef __WIN32__
static void
get_options_from_file (MooCommandFactory *factory,
                       MooCommandData    *cmd_data,
                       const char        *filename,
                       GPtrArray        **options)
{
    MGW_FILE *file;
    char buf[2048];
    mgw_errno_t err;

    if (!(file = mgw_fopen (filename, "rb", &err)))
    {
        g_warning ("%s: could not open file %s: %s", G_STRFUNC, filename, mgw_strerror (err));
        return;
    }

    buf[0] = 0;
    buf[sizeof buf - 1] = '\1';

    if (mgw_fgets (buf, sizeof buf, file) && buf[sizeof buf - 1] != 0)
    {
        int len = strlen (buf);
        seriously_ignore_return_value_p (mgw_fgets (buf + len, sizeof buf - len, file));
    }

    if (mgw_ferror (file))
        g_warning ("%s: error reading file %s", G_STRFUNC, filename);
    else
        get_options_from_contents (factory, cmd_data, buf, options, filename);

    mgw_fclose (file);
}
#endif /* !__WIN32__ */

MooCommandData *
_moo_command_parse_file (const char         *filename,
                         MooCommandFactory **factory_p,
                         char             ***params)
{
    GSList *factories;
    MooCommandFactory *factory = NULL;
    MooCommandData *cmd_data = NULL;
    GPtrArray *array = NULL;

    g_return_val_if_fail (filename != NULL, NULL);
    g_return_val_if_fail (factory_p != NULL, NULL);
    g_return_val_if_fail (params != NULL, NULL);

    *factory_p = NULL;
    *params = NULL;

    factories  = moo_command_list_factories ();
    while (factories)
    {
        factory = (MooCommandFactory*) factories->data;

        if (factory->extension && g_str_has_suffix (filename, factory->extension))
            break;

        factory = NULL;
        factories = g_slist_delete_link (factories, factories);
    }
    g_slist_free (factories);

    if (factory)
    {
        char *contents;
        GError *error = NULL;

        if (!g_file_get_contents (filename, &contents, NULL, &error))
        {
            g_warning ("could not read file %s: %s", filename, error->message);
            g_error_free (error);
            return NULL;
        }

        cmd_data = moo_command_data_new (factory->n_keys);
        moo_command_data_take_code (cmd_data, contents);
        get_options_from_contents (factory, cmd_data, contents, &array, filename);
    }

#ifndef __WIN32__
    if (!factory)
    {
        if (g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE))
            factory = moo_command_factory_lookup ("exe");

        if (factory)
        {
            cmd_data = moo_command_data_new (factory->n_keys);
            moo_command_data_set_code (cmd_data, filename);
            get_options_from_file (factory, cmd_data, filename, &array);
        }
    }
#endif

    if (!factory)
        g_warning ("%s: unknown file type: %s", G_STRFUNC, filename);

    if (array)
    {
        g_ptr_array_add (array, NULL);
        *params = (char**) g_ptr_array_free (array, FALSE);
    }

    *factory_p = factory;
    return cmd_data;
}


MooCommandData *
_moo_command_parse_item (MooMarkupNode      *elm,
                         const char         *name,
                         const char         *filename,
                         MooCommandFactory **factory_p,
                         char              **options_p)
{
    MooMarkupNode *child;
    MooCommandData *data = NULL;
    MooCommandFactory *factory;
    const char *factory_name;
    const char *options = NULL;
    char *prefix = NULL;

    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (elm), NULL);

    if (!(child = moo_markup_get_element (elm, KEY_TYPE)) ||
        !(factory_name = MOO_MARKUP_ELEMENT (child)->content) ||
        !*factory_name)
    {
        g_warning ("missing or empty '%s' element in tool '%s' in file '%s'", KEY_TYPE, name, filename);
        goto error;
    }

    if ((child = moo_markup_get_element (elm, KEY_OPTIONS)))
        options = MOO_MARKUP_ELEMENT (child)->content;

    factory = moo_command_factory_lookup (factory_name);

    if (!factory)
    {
        g_warning ("unknown command type '%s' in tool '%s' in file '%s'",
                   factory_name, name, filename);
        goto error;
    }

    data = moo_command_data_new (factory->n_keys);
    prefix = g_strdup_printf ("%s:", factory_name);

    for (child = elm->children; child != NULL; child = child->next)
    {
        const char *key;

        if (!MOO_MARKUP_IS_ELEMENT (child))
            continue;

        if (!g_str_has_prefix (child->name, prefix))
            continue;

        key = child->name + strlen (prefix);

        if (strcmp (key, KEY_CODE) == 0)
        {
            /* strip leading newline */
            const char *code = MOO_MARKUP_ELEMENT (child)->content;
            if (code)
            {
                if (code[0] == '\n')
                    code += 1;
                else if (code[0] == '\r' && code[1] == '\n')
                    code += 2;
                else if (code[0] == '\r')
                    code += 1;
                moo_command_data_set_code (data, code);
            }
        }
        else
        {
            int index = find_key (factory, key);

            if (index < 0)
            {
                g_warning ("invalid element '%s' in item '%s' in file '%s'",
                           child->name, name, filename);
                goto error;
            }

            moo_command_data_set (data, index, MOO_MARKUP_ELEMENT (child)->content);
        }
    }

    if (factory_p)
        *factory_p = factory;

    if (options_p)
        *options_p = g_strdup (options);

    g_free (prefix);

    return data;

error:
    if (data)
        moo_command_data_unref (data);
    g_free (prefix);
    return NULL;
}


static void
write_code (MooFileWriter *writer,
            const char    *code)
{
    char **pieces, **p;

    if (!strstr (code, "]]>"))
    {
        moo_file_writer_write (writer, code, -1);
        return;
    }

    pieces = g_strsplit (code, "]]>", 0);

    for (p = pieces; p && *p; ++p)
    {
        if (p != pieces)
            moo_file_writer_write (writer, "]]>]]&gt;<![CDATA[", -1);
        moo_file_writer_write (writer, *p, -1);
    }

    g_strfreev (pieces);
}

void
_moo_command_write_item (MooFileWriter     *writer,
                         MooCommandData    *data,
                         MooCommandFactory *factory,
                         char              *options)
{
    g_return_if_fail (writer != NULL);
    g_return_if_fail (MOO_IS_COMMAND_FACTORY (factory));

    if (options && options[0])
        moo_file_writer_printf_markup (writer, "    <" KEY_OPTIONS ">%s</" KEY_OPTIONS ">\n", options);

    moo_file_writer_printf (writer, "    <" KEY_TYPE ">%s</" KEY_TYPE ">\n", factory->name);

    if (data)
    {
        guint i;
        const char *code;

        for (i = 0; i < factory->n_keys; ++i)
        {
            const char *key = factory->keys[i];
            const char *value = moo_command_data_get (data, i);
            if (value)
                moo_file_writer_printf_markup (writer, "    <%s:%s>%s</%s:%s>\n",
                                               factory->name, key, value,
                                               factory->name, key);
        }

        if ((code = moo_command_data_get_code (data)))
        {
            moo_file_writer_printf (writer, "    <%s:%s><![CDATA[\n",
                                    factory->name, KEY_CODE);
            write_code (writer, code);
            moo_file_writer_printf (writer, "]]></%s:%s>\n",
                                    factory->name, KEY_CODE);
        }
    }
}


MooCommandData *
moo_command_data_new (guint len)
{
    MooCommandData *data = g_new0 (MooCommandData, 1);
    data->ref_count = 1;
    data->data = len ? g_new0 (char*, len) : NULL;
    data->len = len;
    return data;
}


#if 0
void
moo_command_data_clear (MooCommandData *data)
{
    guint i;

    g_return_if_fail (data != NULL);

    for (i = 0; i < data->len; ++i)
    {
        g_free (data->data[i]);
        data->data[i] = NULL;
    }
}
#endif


MooCommandData *
moo_command_data_ref (MooCommandData *data)
{
    g_return_val_if_fail (data != NULL, NULL);
    data->ref_count++;
    return data;
}


void
moo_command_data_unref (MooCommandData *data)
{
    g_return_if_fail (data != NULL);

    if (!--data->ref_count)
    {
        guint i;
        for (i = 0; i < data->len; ++i)
            g_free (data->data[i]);
        g_free (data->data);
        g_free (data->code);
        g_free (data);
    }
}


void
moo_command_data_set (MooCommandData *data,
                      guint           index,
                      const char     *value)
{
    g_return_if_fail (data != NULL);
    g_return_if_fail (index < data->len);
    MOO_ASSIGN_STRING (data->data[index], value);
}


const char *
moo_command_data_get (MooCommandData *data,
                      guint           index)
{
    g_return_val_if_fail (data != NULL, NULL);
    g_return_val_if_fail (index < data->len, NULL);
    return data->data[index];
}


static void
moo_command_data_take_code (MooCommandData *data,
                            char           *code)
{
    g_return_if_fail (data != NULL);
    g_free (data->code);
    data->code = code;
}


void
moo_command_data_set_code (MooCommandData *data,
                           const char     *code)
{
    moo_command_data_take_code (data, g_strdup (code));
}


const char *
moo_command_data_get_code (MooCommandData *data)
{
    g_return_val_if_fail (data != NULL, NULL);
    return data->code;
}


void
_moo_command_init (void)
{
    static gboolean been_here = FALSE;

    if (!been_here)
    {
        g_type_class_unref (g_type_class_ref (MOO_TYPE_COMMAND_SCRIPT));
        g_type_class_unref (g_type_class_ref (MOO_TYPE_COMMAND_EXE));
        _moo_command_filter_regex_load ();
        been_here = TRUE;
    }
}


static void
filters_init (void)
{
    if (!registered_filters)
        registered_filters = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


static FilterInfo *
filter_lookup (const char *id)
{
    g_return_val_if_fail (id != NULL, NULL);

    if (registered_filters)
        return (FilterInfo*) g_hash_table_lookup (registered_filters, id);
    else
        return NULL;
}


void
moo_command_filter_register (const char             *id,
                             const char             *name,
                             MooCommandFilterFactory factory_func,
                             gpointer                data,
                             GDestroyNotify          data_notify)
{
    FilterInfo *info;

    g_return_if_fail (id != NULL);
    g_return_if_fail (name != NULL);
    g_return_if_fail (factory_func != NULL);

    filters_init ();

    if (filter_lookup (id))
    {
        _moo_message ("reregistering filter '%s'", id);
        moo_command_filter_unregister (id);
    }

    info = g_new0 (FilterInfo, 1);
    info->id = g_strdup (id);
    info->name = g_strdup (name);
    info->factory_func = factory_func;
    info->data = data;
    info->data_notify = data_notify;

    g_hash_table_insert (registered_filters, g_strdup (id), info);
}


void
moo_command_filter_unregister (const char *id)
{
    FilterInfo *info;

    g_return_if_fail (id != NULL);

    info = filter_lookup (id);

    if (!info)
    {
        g_warning ("filter '%s' not registered", id);
        return;
    }

    g_hash_table_remove (registered_filters, id);

    if (info->data_notify)
        info->data_notify (info->data);

    g_free (info->name);
    g_free (info->id);
    g_free (info);
}


const char *
moo_command_filter_lookup (const char *id)
{
    FilterInfo *info;

    g_return_val_if_fail (id != NULL, 0);

    info = filter_lookup (id);

    return info ? info->name : NULL;
}


static void
prepend_filter_info (G_GNUC_UNUSED const char *id,
                     FilterInfo *info,
                     GSList    **list)
{
    *list = g_slist_prepend (*list, info);
}

static int
compare_filter_names (FilterInfo *fi1,
                      FilterInfo *fi2)
{
    if (strcmp (fi1->id, "default") == 0)
        return -1;
    else if (strcmp (fi2->id, "default") == 0)
        return 1;
    else if (strcmp (fi1->id, "none") == 0)
        return -1;
    else if (strcmp (fi2->id, "none") == 0)
        return 1;
    else
        return g_utf8_collate (fi1->name, fi2->name);
}

GSList *
moo_command_filter_list (void)
{
    GSList *list = NULL;
    GSList *ids = NULL;

    if (registered_filters)
        g_hash_table_foreach (registered_filters, (GHFunc) prepend_filter_info, &list);

    list = g_slist_sort (list, (GCompareFunc) compare_filter_names);

    while (list)
    {
        FilterInfo *fi = (FilterInfo*) list->data;
        ids = g_slist_prepend (ids, g_strdup (fi->id));
        list = g_slist_delete_link (list, list);
    }

    return g_slist_reverse (ids);
}


MooOutputFilter *
moo_command_filter_create (const char *id)
{
    FilterInfo *info;

    g_return_val_if_fail (id != NULL, NULL);

    info = filter_lookup (id);
    g_return_val_if_fail (info != NULL, NULL);

    return info->factory_func (id, info->data);
}
