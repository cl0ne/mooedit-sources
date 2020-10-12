/*
 *   mooeditaction.c
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
 * class:MooEditAction: (parent MooAction) (moo.private 1)
 **/

#include "mooedit/mooeditaction.h"
#include "mooedit/mooeditaction-factory.h"
#include "mooedit/mooeditfiltersettings.h"
#include "mooedit/mooedit-impl.h"
#include "mooutils/mooutils.h"
#include <mooglib/moo-glib.h>
#include <string.h>


typedef enum {
    FILTER_SENSITIVE,
    FILTER_VISIBLE
} FilterType;

#define N_FILTERS 2

struct MooEditActionPrivate {
    MooEdit *doc;
    MooEditFilter *file_filter;
    GRegex *filters[N_FILTERS];
};

typedef struct {
    GRegex *regex;
    guint use_count;
} RegexRef;

typedef struct {
    GHashTable *hash;
} FilterStore;

static FilterStore filter_store;

G_DEFINE_TYPE (MooEditAction, moo_edit_action, MOO_TYPE_ACTION)

enum {
    PROP_0,
    PROP_DOC,
    PROP_FILE_FILTER,
    PROP_FILTER_SENSITIVE,
    PROP_FILTER_VISIBLE
};


static GRegex   *get_filter_regex   (const char *pattern);
static void      unuse_filter_regex (GRegex     *regex);

static gboolean moo_edit_action_check_visible   (MooEditAction  *action);
static gboolean moo_edit_action_check_sensitive (MooEditAction  *action);
static void     moo_edit_action_check_state     (MooEditAction  *action);


static void
moo_edit_action_finalize (GObject *object)
{
    guint i;
    MooEditAction *action = MOO_EDIT_ACTION (object);

    for (i = 0; i < N_FILTERS; ++i)
        if (action->priv->filters[i])
            unuse_filter_regex (action->priv->filters[i]);

    _moo_edit_filter_free (action->priv->file_filter);

    G_OBJECT_CLASS(moo_edit_action_parent_class)->finalize (object);
}


static const char *
moo_edit_action_get_filter (MooEditAction *action,
                            FilterType     type)
{
    g_assert (type < N_FILTERS);
    return action->priv->filters[type] ?
            g_regex_get_pattern (action->priv->filters[type]) : NULL;
}


static void
moo_edit_action_get_property (GObject        *object,
                              guint           prop_id,
                              GValue         *value,
                              GParamSpec     *pspec)
{
    MooEditAction *action = MOO_EDIT_ACTION (object);

    switch (prop_id)
    {
        case PROP_DOC:
            g_value_set_object (value, action->priv->doc);
            break;

        case PROP_FILTER_VISIBLE:
            g_value_set_string (value, moo_edit_action_get_filter (action, FILTER_VISIBLE));
            break;

        case PROP_FILTER_SENSITIVE:
            g_value_set_string (value, moo_edit_action_get_filter (action, FILTER_SENSITIVE));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_edit_action_set_file_filter (MooEditAction *action,
                                 const char    *string)
{
    _moo_edit_filter_free (action->priv->file_filter);

    if (string && string[0])
        action->priv->file_filter = _moo_edit_filter_new (string, MOO_EDIT_FILTER_ACTION);
    else
        action->priv->file_filter = NULL;

    g_object_notify (G_OBJECT (action), "file-filter");
}


MooEdit *
moo_edit_action_get_doc (MooEditAction *action)
{
    g_return_val_if_fail (MOO_IS_EDIT_ACTION (action), NULL);
    return action->priv->doc;
}


static void
moo_edit_action_set_filter (MooEditAction *action,
                            const char    *filter,
                            FilterType     type)
{
    GRegex *tmp;

    g_assert (type < N_FILTERS);

    tmp = action->priv->filters[type];
    action->priv->filters[type] = filter ? get_filter_regex (filter) : NULL;
    unuse_filter_regex (tmp);
}


static void
moo_edit_action_set_property (GObject        *object,
                              guint           prop_id,
                              const GValue   *value,
                              GParamSpec     *pspec)
{
    MooEditAction *action = MOO_EDIT_ACTION (object);

    switch (prop_id)
    {
        case PROP_DOC:
            action->priv->doc = (MooEdit*) g_value_get_object (value);
            g_object_notify (object, "doc");
            break;

        case PROP_FILE_FILTER:
            moo_edit_action_set_file_filter (action, g_value_get_string (value));
            break;

        case PROP_FILTER_SENSITIVE:
            moo_edit_action_set_filter (action,
                                        g_value_get_string (value),
                                        FILTER_SENSITIVE);
            break;

        case PROP_FILTER_VISIBLE:
            moo_edit_action_set_filter (action,
                                        g_value_get_string (value),
                                        FILTER_VISIBLE);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_edit_action_init (MooEditAction *action)
{
    action->priv = G_TYPE_INSTANCE_GET_PRIVATE (action, MOO_TYPE_EDIT_ACTION, MooEditActionPrivate);
}


static const char *
get_current_line (MooEdit *doc)
{
    char *line;

    line = (char*) g_object_get_data (G_OBJECT (doc), "moo-edit-current-line");

    if (!line)
    {
        GtkTextIter line_start, line_end;
        GtkTextBuffer *buffer;

        buffer = moo_edit_get_buffer (doc);
        gtk_text_buffer_get_iter_at_mark (buffer, &line_start,
                                          gtk_text_buffer_get_insert (buffer));
        line_end = line_start;
        gtk_text_iter_set_line_offset (&line_start, 0);
        if (!gtk_text_iter_ends_line (&line_end))
            gtk_text_iter_forward_to_line_end (&line_end);

        line = gtk_text_buffer_get_slice (buffer, &line_start, &line_end, TRUE);

        g_object_set_data_full (G_OBJECT (doc), "moo-edit-current-line", line, g_free);
    }

    return line;
}


static gboolean
moo_edit_action_check_visible_real (MooEditAction *action)
{
    gboolean visible = TRUE;
    GRegex *filter = action->priv->filters[FILTER_VISIBLE];

    if (!action->priv->doc)
        return gtk_action_get_visible (GTK_ACTION (action));

    if (!action->priv->file_filter && !filter)
        return gtk_action_get_visible (GTK_ACTION (action));

    if (visible && action->priv->file_filter)
        if (!_moo_edit_filter_match (action->priv->file_filter, action->priv->doc))
            visible = FALSE;

    if (visible && filter)
    {
        const char *line = get_current_line (action->priv->doc);
        if (!g_regex_match (filter, line, (GRegexMatchFlags) 0, NULL))
            visible = FALSE;
    }

    return visible;
}


static gboolean
moo_edit_action_check_sensitive_real (MooEditAction *action)
{
    const char *line;
    GRegex *filter = action->priv->filters[FILTER_SENSITIVE];

    if (!action->priv->doc || !filter)
        return gtk_action_get_sensitive (GTK_ACTION (action));

    line = get_current_line (action->priv->doc);
    return g_regex_match (filter, line, (GRegexMatchFlags) 0, NULL);
}


static void
moo_edit_action_check_state_real (MooEditAction *action)
{
    gboolean visible, sensitive;

    visible = moo_edit_action_check_visible (action);
    sensitive = moo_edit_action_check_sensitive (action);

    g_object_set (action, "sensitive", sensitive, "visible", visible, nullptr);
}


static void
moo_edit_action_class_init (MooEditActionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_edit_action_finalize;
    gobject_class->set_property = moo_edit_action_set_property;
    gobject_class->get_property = moo_edit_action_get_property;

    klass->check_state = moo_edit_action_check_state_real;
    klass->check_visible = moo_edit_action_check_visible_real;
    klass->check_sensitive = moo_edit_action_check_sensitive_real;

    g_type_class_add_private (klass, sizeof (MooEditActionPrivate));

    g_object_class_install_property (gobject_class,
                                     PROP_DOC,
                                     g_param_spec_object ("doc",
                                             "doc",
                                             "doc",
                                             MOO_TYPE_EDIT,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_FILE_FILTER,
                                     g_param_spec_string ("file-filter",
                                             "file-filter",
                                             "file-filter",
                                             NULL,
                                             G_PARAM_WRITABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_FILTER_SENSITIVE,
                                     g_param_spec_string ("filter-sensitive",
                                             "filter-sensitive",
                                             "filter-sensitive",
                                             NULL,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_FILTER_VISIBLE,
                                     g_param_spec_string ("filter-visible",
                                             "filter-visible",
                                             "filter-visible",
                                             NULL,
                                             (GParamFlags) G_PARAM_READWRITE));
}


static gboolean
moo_edit_action_check_visible (MooEditAction *action)
{
    g_return_val_if_fail (MOO_IS_EDIT_ACTION (action), TRUE);

    if (MOO_EDIT_ACTION_GET_CLASS (action)->check_visible)
        return MOO_EDIT_ACTION_GET_CLASS (action)->check_visible (action);
    else
        return gtk_action_get_visible (GTK_ACTION (action));
}

static gboolean
moo_edit_action_check_sensitive (MooEditAction *action)
{
    g_return_val_if_fail (MOO_IS_EDIT_ACTION (action), TRUE);

    if (MOO_EDIT_ACTION_GET_CLASS (action)->check_sensitive)
        return MOO_EDIT_ACTION_GET_CLASS (action)->check_sensitive (action);
    else
        return gtk_action_get_sensitive (GTK_ACTION (action));
}

static void
moo_edit_action_check_state (MooEditAction *action)
{
    g_return_if_fail (MOO_IS_EDIT_ACTION (action));
    g_return_if_fail (MOO_EDIT_ACTION_GET_CLASS (action)->check_state != NULL);
    MOO_EDIT_ACTION_GET_CLASS (action)->check_state (action);
}


void
_moo_edit_check_actions (MooEdit     *edit,
                         MooEditView *view)
{
    GtkActionGroup *group = moo_edit_get_actions (edit);
    GList *actions = gtk_action_group_list_actions (group);

    while (actions)
    {
        GtkAction *action = (GtkAction*) actions->data;
        g_object_set_data (G_OBJECT (action), "moo-edit", edit);
        g_object_set_data (G_OBJECT (action), "moo-edit-view", view);
        if (MOO_IS_EDIT_ACTION (action))
            moo_edit_action_check_state (MOO_EDIT_ACTION (action));
        actions = g_list_delete_link (actions, actions);
    }
}


static void
regex_ref_free (RegexRef *ref)
{
    if (ref)
    {
        g_regex_unref (ref->regex);
        g_free (ref);
    }
}

static void
init_filter_store (void)
{
    if (!filter_store.hash)
        filter_store.hash = g_hash_table_new_full (g_str_hash, g_str_equal,  g_free,
                                                   (GDestroyNotify) regex_ref_free);
}


static GRegex *
get_filter_regex (const char *pattern)
{
    RegexRef *ref;

    g_return_val_if_fail (pattern != NULL, NULL);

    init_filter_store ();

    ref = (RegexRef*) g_hash_table_lookup (filter_store.hash, pattern);

    if (!ref)
    {
        GRegex *regex;
        GError *error = NULL;

        regex = g_regex_new (pattern, G_REGEX_OPTIMIZE, (GRegexMatchFlags) 0, &error);

        if (!regex)
        {
            g_warning ("%s", moo_error_message (error));
            g_error_free (error);
            return NULL;
        }

        ref = g_new0 (RegexRef, 1);
        ref->regex = regex;

        g_hash_table_insert (filter_store.hash, g_strdup (pattern), ref);
    }

    ref->use_count++;
    return ref->regex;
}


static void
unuse_filter_regex (GRegex *regex)
{
    RegexRef *ref;
    const char *pattern;

    g_return_if_fail (regex != NULL);

    init_filter_store ();

    pattern = g_regex_get_pattern (regex);
    ref = (RegexRef*) g_hash_table_lookup (filter_store.hash, pattern);
    g_return_if_fail (ref != NULL);

    if (!--ref->use_count)
        g_hash_table_remove (filter_store.hash, pattern);
}
