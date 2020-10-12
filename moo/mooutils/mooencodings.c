/*
 *   mooencodings.c
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

#include "mooutils/mooencodings.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooencodings-data.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooaction.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#define MAX_RECENT_ENCODINGS 5
#define ROW_AUTO              0

#define ELM_ENCODINGS   "Editor/encodings"
#define ELM_RECENT      "recent"
#define ELM_LAST_OPEN   "last-open"
#define ELM_LAST_SAVE   "last-save"


typedef struct {
    const char *name;
    const char *short_display_name;
    char *display_name;
    char *subgroup_name;
} Encoding;

typedef struct {
    Encoding **encodings;
    guint n_encodings;
    char *name;
} EncodingGroup;

typedef struct {
    GtkWidget *menu;
    MooEncodingsMenuFunc func;
    gpointer func_data;
} MenuData;

typedef struct {
    GSList *recent;
    EncodingGroup *groups;
    guint n_groups;
    Encoding *locale_encoding;
    GSList *user_defined;
    char *last_open;
    char *last_save;
    GHashTable *encodings;

    GSList *menus;
    guint update_menu_idle;
} EncodingsManager;

enum {
    COLUMN_DISPLAY,
    COLUMN_ENCODING
};


static void     combo_changed       (GtkComboBox        *combo,
                                     gpointer            save_mode);
static void     enc_mgr_load        (EncodingsManager   *enc_mgr);
static void     enc_mgr_add_used    (EncodingsManager   *enc_mgr,
                                     Encoding           *enc);
static void     sync_recent_menu    (MenuData           *menu,
                                     gboolean            need_separator);


static Encoding *
lookup_encoding (EncodingsManager *mgr,
                 const char       *name)
{
    char *upper;
    Encoding *enc;

    g_return_val_if_fail (name != NULL, NULL);

    upper = g_ascii_strup (name, -1);
    enc = (Encoding*) g_hash_table_lookup (mgr->encodings, upper);

    g_free (upper);
    return enc;
}

static Encoding *
get_encoding (EncodingsManager *mgr,
              const char       *name)
{
    Encoding *enc;

    g_return_val_if_fail (name != NULL, NULL);

    enc = lookup_encoding (mgr, name);

    if (!enc)
    {
        enc = g_new0 (Encoding, 1);
        enc->name = g_ascii_strup (name, -1);
        enc->short_display_name = enc->name;
        enc->display_name = g_strdup (name);
        mgr->user_defined = g_slist_prepend (mgr->user_defined, enc);
        g_hash_table_insert (mgr->encodings, (char*) enc->name, enc);
    }

    return enc;
}


static gboolean
validate_encoding_name (const char *name)
{
    return name && name[0];
}


static int
compare_encodings (Encoding **enc1,
                   Encoding **enc2)
{
    int result = 0;

    if ((*enc1)->subgroup_name != NULL && (*enc2)->subgroup_name != NULL)
        result = g_utf8_collate ((*enc1)->subgroup_name, (*enc2)->subgroup_name);
    else
        result = (*enc1)->subgroup_name < (*enc2)->subgroup_name;

    if (!result)
        result = g_utf8_collate ((*enc1)->short_display_name, (*enc2)->short_display_name);

    return result;
}

static void
fill_encoding_group (int            group_no,
                     EncodingGroup *group,
                     GSList        *encodings)
{
    guint i;

    if (!encodings)
        return;

    group->n_encodings = g_slist_length (encodings);
    group->encodings = g_new0 (Encoding*, group->n_encodings);

    for (i = 0; encodings != NULL; encodings = encodings->next, ++i)
        group->encodings[i] = (Encoding*) encodings->data;

    if (group_no != ENCODING_GROUP_UNICODE)
        qsort (group->encodings, group->n_encodings, sizeof (Encoding*),
               (int(*)(const void *, const void *)) compare_encodings);
}

static void
sort_encoding_groups (EncodingsManager *mgr)
{
    /* Translators: 012345 denotes order in which character encoding
       groups should appear, e.g. 543210 means inverse order.
       0 - "West European",
       1 - "East European",
       2 - "East Asian",
       3 - "SE & SW Asian",
       4 - "Middle Eastern",
       5 - "Unicode" */
    const char *order = N_("012345");
    const char *new_order_s;
    int new_order[N_ENCODING_GROUPS] = {-1, -1, -1, -1, -1, -1};
    guint i;
    EncodingGroup *new_array;

    g_return_if_fail (strlen (order) == N_ENCODING_GROUPS);
    g_return_if_fail (mgr->n_groups == N_ENCODING_GROUPS);

    new_order_s = _(order);

    if (!strcmp (order, new_order_s))
        return;

    for (i = 0; new_order_s[i]; ++i)
    {
        int n = new_order_s[i] - '0';

        if (n < 0 || n >= (int) mgr->n_groups)
        {
            g_critical ("invalid order string %s", new_order_s);
            return;
        }

        new_order[i] = n;
    }

    for (i = 0; i < mgr->n_groups; ++i)
    {
        if (new_order[i] == -1)
        {
            g_critical ("invalid order string %s", new_order_s);
            return;
        }
    }

    new_array = g_new (EncodingGroup, mgr->n_groups);

    for (i = 0; i < mgr->n_groups; ++i)
        new_array[i] = mgr->groups[new_order[i]];

    g_free (mgr->groups);
    mgr->groups = new_array;
}

static EncodingsManager *
get_enc_mgr (void)
{
    static EncodingsManager *mgr;

    if (!mgr)
    {
        guint i;
        const char *locale_charset;
        GSList *enc_groups[N_ENCODING_GROUPS] = {NULL, NULL, NULL, NULL, NULL, NULL};

        mgr = g_new0 (EncodingsManager, 1);

        mgr->n_groups = N_ENCODING_GROUPS;
        mgr->groups = g_new0 (EncodingGroup, N_ENCODING_GROUPS);

        for (i = 0; i < mgr->n_groups; ++i)
            mgr->groups[i].name = g_strdup (_(moo_encoding_groups_names[i]));

        mgr->encodings = g_hash_table_new (g_str_hash, g_str_equal);

        for (i = 0; i < G_N_ELEMENTS (moo_encodings_data); ++i)
        {
            Encoding *enc;

            enc = g_new0 (Encoding, 1);
            enc->name = moo_encodings_data[i].name;

            if (moo_encodings_data[i].display_subgroup != NULL)
            {
                enc->short_display_name = moo_encodings_data[i].short_display_name;
                enc->subgroup_name = g_strdup (_(moo_encodings_data[i].display_subgroup));
                enc->display_name = g_strdup_printf ("%s (%s)", enc->subgroup_name,
                                                     enc->short_display_name);
            }
            else
            {
                enc->short_display_name = Q_(moo_encodings_data[i].short_display_name);
                enc->display_name = g_strdup (enc->short_display_name);
            }

            enc_groups[moo_encodings_data[i].group] =
                g_slist_prepend (enc_groups[moo_encodings_data[i].group], enc);
            g_hash_table_insert (mgr->encodings, (char*) moo_encodings_data[i].name, enc);
        }

        for (i = 0; i < G_N_ELEMENTS (moo_encoding_aliases); ++i)
        {
            Encoding *enc;

            enc = (Encoding*) g_hash_table_lookup (mgr->encodings, moo_encoding_aliases[i].name);

            if (!enc)
                g_critical ("oops %s", moo_encoding_aliases[i].name);
            else
                g_hash_table_insert (mgr->encodings, (char*)
                                     moo_encoding_aliases[i].alias,
                                     enc);
        }

        for (i = 0; i < N_ENCODING_GROUPS; ++i)
        {
            fill_encoding_group (i, &mgr->groups[i], enc_groups[i]);
            g_slist_free (enc_groups[i]);
        }

        sort_encoding_groups (mgr);

        if (!g_get_charset (&locale_charset))
            locale_charset = MOO_ENCODING_UTF8;

        mgr->locale_encoding = get_encoding (mgr, locale_charset);

        enc_mgr_load (mgr);
    }

    return mgr;
}


const char *
_moo_encoding_get_display_name (const char *enc_name)
{
    EncodingsManager *mgr;
    Encoding *enc;

    g_return_val_if_fail (enc_name != NULL, NULL);

    mgr = get_enc_mgr ();
    enc = get_encoding (mgr, enc_name);
    g_return_val_if_fail (enc != NULL, enc_name);

    return enc->display_name;
}


static void
enc_mgr_save (EncodingsManager *enc_mgr)
{
    MooMarkupNode *doc;
    MooMarkupNode *root;
    GString *string = NULL;

    doc = moo_prefs_get_markup (MOO_PREFS_STATE);
    g_return_if_fail (doc != NULL);

    if (!(root = moo_markup_get_element (doc, ELM_ENCODINGS)))
    {
        root = moo_markup_create_element (doc, ELM_ENCODINGS);
        moo_markup_create_element (root, ELM_RECENT);
        moo_markup_create_element (root, ELM_LAST_OPEN);
        moo_markup_create_element (root, ELM_LAST_SAVE);
    }

    if (enc_mgr->recent)
    {
        GSList *l;

        string = g_string_new (NULL);

        for (l = enc_mgr->recent; l != NULL; l = l->next)
        {
            if (l != enc_mgr->recent)
                g_string_append (string, ",");
            g_string_append (string, ((Encoding*)l->data)->name);
        }
    }

    if (string)
        moo_markup_set_content (moo_markup_get_element (root, ELM_RECENT), string->str);
    else
        moo_markup_set_content (moo_markup_get_element (root, ELM_RECENT), NULL);

    moo_markup_set_content (moo_markup_get_element (root, ELM_LAST_OPEN), enc_mgr->last_open);
    moo_markup_set_content (moo_markup_get_element (root, ELM_LAST_SAVE), enc_mgr->last_save);

    if (string)
        g_string_free (string, TRUE);
}

static const char *
get_rid_of_locale (const char *enc_name)
{
    if (enc_name && !strcmp (enc_name, "locale"))
        return _moo_encoding_locale ();
    else
        return enc_name;
}

static void
enc_mgr_load (EncodingsManager *enc_mgr)
{
    MooMarkupNode *doc;
    MooMarkupNode *root;
    MooMarkupNode *node;

    g_return_if_fail (enc_mgr->recent == NULL);

    if (!(doc = moo_prefs_get_markup (MOO_PREFS_STATE)) ||
        !(root = moo_markup_get_element (doc, ELM_ENCODINGS)))
            return;

    if ((node = moo_markup_get_element (root, ELM_RECENT)))
    {
        const char *string = moo_markup_get_content (node);

        if (string && *string)
        {
            char **encs, **p;
            encs = g_strsplit (string, ",", 0);
            for (p = encs; p && *p; ++p)
            {
                Encoding *e;

                e = get_encoding (enc_mgr, get_rid_of_locale (*p));

                if (e)
                    enc_mgr->recent = g_slist_prepend (enc_mgr->recent, e);
            }
            enc_mgr->recent = g_slist_reverse (enc_mgr->recent);
            g_strfreev (encs);
        }
    }

    if ((node = moo_markup_get_element (root, ELM_LAST_OPEN)))
    {
        g_free (enc_mgr->last_open);
        enc_mgr->last_open = g_strdup (get_rid_of_locale (moo_markup_get_content (node)));
    }

    if ((node = moo_markup_get_element (root, ELM_LAST_SAVE)))
    {
        g_free (enc_mgr->last_save);
        enc_mgr->last_save = g_strdup (get_rid_of_locale (moo_markup_get_content (node)));
    }
}


typedef struct {
    const char *name;
    GtkTreeIter iter;
    gboolean found;
} FindEncodingData;

static gboolean
find_encoding_func (GtkTreeModel     *model,
                    G_GNUC_UNUSED GtkTreePath *path,
                    GtkTreeIter      *iter,
                    FindEncodingData *data)
{
    char *name;

    gtk_tree_model_get (model, iter, COLUMN_ENCODING, &name, -1);

    if (name && !strcmp (name, data->name))
    {
        data->iter = *iter;
        data->found = TRUE;
    }

    g_free (name);
    return data->found;
}

static gboolean
find_encoding_iter (GtkTreeModel *model,
                    GtkTreeIter  *iter,
                    const char   *name)
{
    FindEncodingData data;

    data.found = FALSE;
    data.name = name;

    gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) find_encoding_func, &data);

    if (data.found && iter)
        *iter = data.iter;

    return data.found;
}


static int
get_row_recent (GtkTreeStore *store)
{
    return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (store), "moo-encodings-model-row-recent"));
}

static void
set_row_recent (GtkTreeStore *store,
		int           row)
{
    g_object_set_data (G_OBJECT (store),
                       "moo-encodings-model-row-recent",
                       GINT_TO_POINTER (row));
}


static void
sync_recent_list (GtkTreeStore *store,
                  guint         n_old_items,
                  GSList       *list,
                  gboolean      add_separator)
{
    GtkTreeIter iter;
    guint i;

    for (i = 0; i < n_old_items; ++i)
    {
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &iter,
                                       NULL, get_row_recent (store));
        gtk_tree_store_remove (store, &iter);
    }

    if (add_separator)
        gtk_tree_store_insert (store, &iter, NULL, get_row_recent (store));

    while (list)
    {
        Encoding *enc = (Encoding*) list->data;
        gtk_tree_store_insert (store, &iter, NULL, get_row_recent (store));
        gtk_tree_store_set (store, &iter,
                            COLUMN_DISPLAY, enc->display_name,
                            COLUMN_ENCODING, enc->name, -1);
        list = list->next;
    }
}

static void
set_last (EncodingsManager *mgr,
          const char       *name,
          gboolean          save_mode)
{
    char **ptr = save_mode ? &mgr->last_save : &mgr->last_open;
    char *tmp = *ptr;
    *ptr = g_strdup (name);
    g_free (tmp);
}

static gboolean
update_menu_idle (EncodingsManager *mgr)
{
    GSList *l;

    mgr->update_menu_idle = 0;

    for (l = mgr->menus; l != NULL; l = l->next)
    {
        MenuData *menu = (MenuData*) l->data;
        sync_recent_menu (menu, FALSE);
    }

    enc_mgr_save (mgr);

    return FALSE;
}

static void
enc_mgr_add_used (EncodingsManager *mgr,
                  Encoding         *new_enc)
{
    GSList *l;
    gboolean found_recent;

    for (l = mgr->recent, found_recent = FALSE; l != NULL; l = l->next)
    {
        Encoding *enc = (Encoding*) l->data;

        if (!strcmp (new_enc->name, enc->name))
        {
            if (l != mgr->recent)
            {
                mgr->recent = g_slist_remove_link (mgr->recent, l);
                l->next = mgr->recent;
                mgr->recent = l;
            }

            found_recent = TRUE;
            break;
        }
    }

    if (!found_recent)
    {
        mgr->recent = g_slist_prepend (mgr->recent, new_enc);

        if (g_slist_length (mgr->recent) > MAX_RECENT_ENCODINGS)
            mgr->recent = g_slist_delete_link (mgr->recent, g_slist_last (mgr->recent));
    }

    if (!mgr->update_menu_idle)
        mgr->update_menu_idle = g_idle_add ((GSourceFunc) update_menu_idle, mgr);
}

static void
encoding_combo_set_active (GtkComboBox *combo,
                           const char  *enc_name,
                           gboolean     save_mode)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GSList *recent_copy;
    EncodingsManager *mgr;
    guint n_old_recent;
    Encoding *new_enc;
    gboolean need_separator = FALSE;

    mgr = get_enc_mgr ();

    if (!validate_encoding_name (enc_name))
        enc_name = save_mode ? MOO_ENCODING_UTF8 : MOO_ENCODING_AUTO;

    set_last (mgr, enc_name, save_mode);

    model = gtk_combo_box_get_model (combo);
    g_signal_handlers_block_by_func (combo, (gpointer) combo_changed,
                                     GINT_TO_POINTER (save_mode));

    if (!strcmp (enc_name, MOO_ENCODING_AUTO))
    {
        gtk_tree_model_iter_nth_child (model, &iter, NULL, ROW_AUTO);
        gtk_combo_box_set_active_iter (combo, &iter);
        goto out;
    }

    new_enc = get_encoding (mgr, enc_name);
    g_return_if_fail (new_enc != NULL);

    set_last (mgr, new_enc->name, save_mode);

    if (!mgr->recent)
        need_separator = TRUE;
    n_old_recent = g_slist_length (mgr->recent);

    enc_mgr_add_used (mgr, new_enc);

    recent_copy = g_slist_reverse (g_slist_copy (mgr->recent));
    sync_recent_list (GTK_TREE_STORE (model), n_old_recent, recent_copy, need_separator);
    g_slist_free (recent_copy);

    if (find_encoding_iter (model, &iter, new_enc->name))
        gtk_combo_box_set_active_iter (combo, &iter);

out:
    g_signal_handlers_unblock_by_func (combo, (gpointer) combo_changed,
                                       GINT_TO_POINTER (save_mode));
}

static void
combo_changed (GtkComboBox *combo,
               gpointer     save_mode)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *enc_name;

    if (!gtk_combo_box_get_active_iter (combo, &iter))
        return;

    model = gtk_combo_box_get_model (combo);

    if (gtk_tree_model_iter_has_child (model, &iter))
        return;

    gtk_tree_model_get (model, &iter, COLUMN_ENCODING, &enc_name, -1);

    if (!enc_name)
        return;

    encoding_combo_set_active (combo, enc_name, GPOINTER_TO_INT (save_mode));
    g_free (enc_name);
}


static gboolean
row_separator_func (GtkTreeModel *model,
                    GtkTreeIter  *iter)
{
    char *text;
    gboolean separator;

    gtk_tree_model_get (model, iter, COLUMN_DISPLAY, &text, -1);
    separator = text == NULL;

    g_free (text);
    return separator;
}

static void
cell_data_func (G_GNUC_UNUSED GtkCellLayout *layout,
		GtkCellRenderer *cell,
                GtkTreeModel    *model,
                GtkTreeIter     *iter)
{
    gboolean sensitive;
    sensitive = !gtk_tree_model_iter_has_child (model, iter);
    g_object_set (cell, "sensitive", sensitive, NULL);
}

static void
setup_combo (GtkComboBox      *combo,
             EncodingsManager *enc_mgr,
             gboolean          save_mode,
	     gboolean	       use_separators)
{
    GtkCellRenderer *cell;
    GtkTreeStore *store;
    GtkTreeIter iter;
    GSList *l;
    guint i;
    char *start;

    store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

    if (!save_mode)
    {
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
                            COLUMN_DISPLAY, _("Auto Detected"),
                            COLUMN_ENCODING, MOO_ENCODING_AUTO,
                            -1);

	if (use_separators)
	    gtk_tree_store_append (store, &iter, NULL);

        set_row_recent (store, use_separators ? 2 : 1);
    }
    else
    {
        set_row_recent (store, 0);
    }

    for (l = enc_mgr->recent; l != NULL; l = l->next)
    {
        Encoding *enc = (Encoding*) l->data;
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
                            COLUMN_DISPLAY, enc->display_name,
                            COLUMN_ENCODING, enc->name,
                            -1);
    }

    if (enc_mgr->recent && use_separators)
        gtk_tree_store_append (store, &iter, NULL);

    gtk_tree_store_append (store, &iter, NULL);
    /* Translators: remove the part before and including | */
    gtk_tree_store_set (store, &iter, COLUMN_DISPLAY, Q_("Encodings combo submenu|Other"), -1);

    for (i = 0; i < enc_mgr->n_groups; ++i)
    {
        guint j;
        GtkTreeIter child, group_iter;
        EncodingGroup *group;

        group = &enc_mgr->groups[i];
        gtk_tree_store_append (store, &group_iter, &iter);
        gtk_tree_store_set (store, &group_iter, COLUMN_DISPLAY, group->name, -1);

        for (j = 0; j < group->n_encodings; ++j)
        {
            gtk_tree_store_append (store, &child, &group_iter);
            gtk_tree_store_set (store, &child,
                                COLUMN_DISPLAY, group->encodings[j]->display_name,
                                COLUMN_ENCODING, group->encodings[j]->name,
                                -1);
        }
    }

    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));
    gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (combo), COLUMN_DISPLAY);

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                    "text", COLUMN_DISPLAY, NULL);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), cell,
                                        (GtkCellLayoutDataFunc) cell_data_func,
					NULL, NULL);

    if (use_separators)
	gtk_combo_box_set_row_separator_func (combo, (GtkTreeViewRowSeparatorFunc) row_separator_func,
					      NULL, NULL);

    if (save_mode)
    {
        if (enc_mgr->last_save)
            start = g_strdup (enc_mgr->last_save);
        else
            start = g_strdup (MOO_ENCODING_UTF8);
    }
    else
    {
        if (enc_mgr->last_open)
            start = g_strdup (enc_mgr->last_open);
        else
            start = g_strdup (MOO_ENCODING_AUTO);
    }

    g_signal_connect (combo, "changed", G_CALLBACK (combo_changed),
                      GINT_TO_POINTER (save_mode));
    encoding_combo_set_active (GTK_COMBO_BOX (combo), start, save_mode);

    g_free (start);
    g_object_unref (store);
}


void
_moo_encodings_combo_init (GtkComboBox          *combo,
                           MooEncodingComboType  type,
			   gboolean		 use_separators)
{
    g_return_if_fail (GTK_IS_COMBO_BOX_ENTRY (combo));
    setup_combo (combo, get_enc_mgr (),
		 type == MOO_ENCODING_COMBO_SAVE,
		 use_separators);
}

void
_moo_encodings_combo_set_enc (GtkComboBox         *combo,
                              const char          *enc,
                              MooEncodingComboType type)
{
    g_return_if_fail (GTK_IS_COMBO_BOX_ENTRY (combo));
    encoding_combo_set_active (combo, enc, type == MOO_ENCODING_COMBO_SAVE);
}


void
_moo_encodings_attach_combo (GtkWidget  *dialog,
                             GtkWidget  *parent,
                             gboolean    save_mode,
                             const char *encoding)
{
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;

    g_return_if_fail (GTK_IS_FILE_CHOOSER (dialog));

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Charact_er encoding:"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    combo = gtk_combo_box_entry_new ();
    gtk_widget_show (combo);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

    setup_combo (GTK_COMBO_BOX (combo), get_enc_mgr (), save_mode, TRUE);

    if (save_mode && encoding)
        encoding_combo_set_active (GTK_COMBO_BOX (combo), encoding, save_mode);

    g_object_set_data (G_OBJECT (dialog), "moo-encodings-combo", combo);
}


static void
sync_combo (GtkComboBox *combo,
            gboolean     save_mode)
{
    const char *enc_name;
    GtkTreeIter dummy;

    if (gtk_combo_box_get_active_iter (combo, &dummy))
        return;

    enc_name = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (combo)->child));

    if (!validate_encoding_name (enc_name))
        enc_name = save_mode ? MOO_ENCODING_UTF8 : MOO_ENCODING_AUTO;

    encoding_combo_set_active (combo, enc_name, save_mode);
}


static const char  *
combo_get (GtkComboBox *combo,
           gboolean     save_mode)
{
    const char *enc_name;
    EncodingsManager *mgr;

    sync_combo (combo, save_mode);

    mgr = get_enc_mgr ();
    enc_mgr_save (mgr);

    if (save_mode)
    {
        if (mgr->last_save)
            enc_name = mgr->last_save;
        else
            enc_name = MOO_ENCODING_UTF8;
    }
    else
    {
        if (mgr->last_open)
            enc_name = mgr->last_open;
        else
            enc_name = MOO_ENCODING_AUTO;
    }

    return enc_name;
}

const char *
_moo_encodings_combo_get (GtkWidget *dialog,
                          gboolean   save_mode)
{
    GtkComboBox *combo;

    combo = GTK_COMBO_BOX (g_object_get_data (G_OBJECT (dialog), "moo-encodings-combo"));
    g_return_val_if_fail (GTK_IS_COMBO_BOX_ENTRY (combo), MOO_ENCODING_UTF8);

    return combo_get (combo, save_mode);
}

const char *
_moo_encodings_combo_get_enc (GtkComboBox          *combo,
                              MooEncodingComboType  type)
{
    g_return_val_if_fail (GTK_IS_COMBO_BOX (combo), MOO_ENCODING_UTF8);
    return combo_get (combo, type == MOO_ENCODING_COMBO_SAVE);
}


const char *
_moo_encoding_locale (void)
{
    EncodingsManager *mgr;

    mgr = get_enc_mgr ();

    return mgr->locale_encoding->name;
}


gboolean
_moo_encodings_equal (const char *enc1_name,
                      const char *enc2_name)
{
    Encoding *enc1, *enc2;
    EncodingsManager *mgr;

    enc1_name = enc1_name && enc1_name[0] ? enc1_name : MOO_ENCODING_UTF8;
    enc2_name = enc2_name && enc2_name[0] ? enc2_name : MOO_ENCODING_UTF8;

    mgr = get_enc_mgr ();

    enc1 = lookup_encoding (mgr, enc1_name);
    enc2 = lookup_encoding (mgr, enc2_name);

    if (!enc1 || !enc2)
        return !strcmp (enc1_name, enc2_name);

    return enc1 == enc2;
}


static void
menu_data_free (MenuData *menu_data)
{
    g_free (menu_data);
}

static void
menu_destroyed (GtkWidget *widget,
                MenuData  *menu)
{
    EncodingsManager *mgr = get_enc_mgr ();

    g_return_if_fail (g_slist_find (mgr->menus, menu) != NULL);

    g_signal_handlers_disconnect_by_func (widget, (gpointer) menu_destroyed, menu);
    mgr->menus = g_slist_remove (mgr->menus, menu);

    menu_data_free (menu);
}

static void
menu_item_activated (GtkWidget *item,
                     MenuData  *menu)
{
    EncodingsManager *mgr = get_enc_mgr ();
    Encoding *enc;

    enc = (Encoding*) g_object_get_data (G_OBJECT (item), "moo-encoding");
    g_return_if_fail (enc != NULL);

    menu->func (enc->name, menu->func_data);

    if (g_slist_find (mgr->menus, menu) != NULL)
        enc_mgr_add_used (mgr, enc);
}

static GtkWidget *
create_menu_item (Encoding *enc,
                  MenuData *menu,
                  gboolean  recent)
{
    GtkWidget *item;

    item = gtk_menu_item_new_with_label (enc->display_name);
    gtk_widget_show (item);

    g_signal_connect (item, "activate", G_CALLBACK (menu_item_activated), menu);
    g_object_set_data (G_OBJECT (item), "moo-encoding", enc);

    if (recent)
        g_object_set_data (G_OBJECT (item), "moo-recent-encoding",
                           GINT_TO_POINTER (TRUE));

    return item;
}

static void
sync_recent_menu (MenuData *menu,
                  gboolean  first_time)
{
    EncodingsManager *mgr = get_enc_mgr ();
    GList *children, *l;
    GSList *recent_items;
    gboolean have_separator = FALSE;
    int pos = first_time ? 0 : 2;

    children = gtk_container_get_children (GTK_CONTAINER (menu->menu));
    for (l = children; l != NULL; l = l->next)
        if (g_object_get_data (G_OBJECT (l->data), "moo-recent-encoding"))
            gtk_container_remove (GTK_CONTAINER (menu->menu), GTK_WIDGET (l->data));
    g_list_free (children);

    recent_items = g_slist_reverse (g_slist_copy (mgr->recent));

    while (recent_items)
    {
        Encoding *enc = (Encoding*) recent_items->data;
        GtkWidget *item;

        if (first_time && !have_separator)
        {
            item = gtk_separator_menu_item_new ();
            gtk_menu_shell_insert (GTK_MENU_SHELL (menu->menu), item, pos);
            have_separator = TRUE;
        }

        item = create_menu_item (enc, menu, TRUE);
        gtk_menu_shell_insert (GTK_MENU_SHELL (menu->menu), item, pos);

        recent_items = g_slist_delete_link (recent_items, recent_items);
    }
}

static void
exclude_item (MenuData *menu_data,
              Encoding *exclude_enc)
{
    GList *children, *l;

    children = gtk_container_get_children (GTK_CONTAINER (menu_data->menu));

    for (l = children; l != NULL; l = l->next)
    {
        GtkWidget *item = GTK_WIDGET (l->data);

        if (g_object_get_data (G_OBJECT (item), "moo-recent-encoding"))
        {
            gboolean visible = TRUE;

            if (exclude_enc)
            {
                Encoding *enc = (Encoding*) g_object_get_data (G_OBJECT (item), "moo-encoding");
                visible = exclude_enc != enc;
            }

            g_object_set (item, "visible", visible, NULL);
        }
    }

    g_list_free (children);
}

static MenuData *
_moo_encodings_menu_new (MooEncodingsMenuFunc   func,
                         gpointer               data)
{
    MenuData *menu_data = NULL;
    GtkWidget *widget;
    EncodingsManager *mgr;
    guint cgr;

    g_return_val_if_fail (func != NULL, NULL);

    mgr = get_enc_mgr ();

    menu_data = g_new0 (MenuData, 1);
    menu_data->func = func;
    menu_data->func_data = data;

    widget = gtk_menu_new ();
    menu_data->menu = widget;

    mgr->menus = g_slist_prepend (mgr->menus, menu_data);
    g_signal_connect (widget, "destroy", G_CALLBACK (menu_destroyed), menu_data);

    for (cgr = 0; cgr < mgr->n_groups; ++cgr)
    {
        EncodingGroup *grp = &mgr->groups[cgr];
        GtkWidget *item, *submenu;
        guint i;

        item = gtk_menu_item_new_with_label (grp->name);
        gtk_menu_shell_append (GTK_MENU_SHELL (widget), item);
        submenu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

        for (i = 0; i < grp->n_encodings; ++i)
        {
            Encoding *enc = grp->encodings[i];
            item = create_menu_item (enc, menu_data, FALSE);
            gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
        }
    }

    sync_recent_menu (menu_data, TRUE);

    gtk_widget_show_all (widget);
    return menu_data;
}

typedef struct {
    MooAction base;
    MooEncodingsMenuFunc func;
    gpointer func_data;
    Encoding *cur_enc;
    GtkWidget *cur_item;
    GtkWidget *cur_separator;
    MenuData *menu_data;
    guint update_idle;
} MooEncodingsMenuAction;
typedef MooActionClass MooEncodingsMenuActionClass;
MOO_DEFINE_TYPE_STATIC (MooEncodingsMenuAction, moo_encodings_menu_action, MOO_TYPE_ACTION)
#define MOO_TYPE_ENCODINGS_MENU_ACTION              (moo_encodings_menu_action_get_type ())
#define MOO_ENCODINGS_MENU_ACTION(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_ENCODINGS_MENU_ACTION, MooEncodingsMenuAction))
#define MOO_ENCODINGS_MENU_ACTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_ENCODINGS_MENU_ACTION, MooEncodingsMenuActionClass))
#define MOO_IS_ENCODINGS_MENU_ACTION(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_ENCODINGS_MENU_ACTION))
#define MOO_IS_ENCODINGS_MENU_ACTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_ENCODINGS_MENU_ACTION))
#define ENCODINGS_MENU_ACTION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_ENCODINGS_MENU_ACTION, MooEncodingsMenuActionClass))

static void
moo_encodings_menu_action_init (MooEncodingsMenuAction *action)
{
    action->func = NULL;
    action->func_data = NULL;
    action->cur_enc = NULL;
    action->cur_item = NULL;
    action->cur_separator = NULL;
    action->menu_data = NULL;
    g_object_set (action, "no-accel", TRUE, NULL);
}

static void
action_item_activated (const char *encoding,
                       gpointer    data)
{
    MooEncodingsMenuAction *action = MOO_ENCODINGS_MENU_ACTION (data);

    g_return_if_fail (action->func != NULL);

    action->func (encoding, action->func_data);
}

static GtkWidget *
moo_encodings_menu_action_create_menu_item (GtkAction *gtkaction)
{
    MooEncodingsMenuAction *action = MOO_ENCODINGS_MENU_ACTION (gtkaction);
    GtkWidget *menu_item;

    menu_item = GTK_ACTION_CLASS (moo_encodings_menu_action_parent_class)->create_menu_item (gtkaction);
    action->menu_data = _moo_encodings_menu_new (action_item_activated, action);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), action->menu_data->menu);

    action->cur_separator = gtk_separator_menu_item_new ();
    gtk_menu_shell_prepend (GTK_MENU_SHELL (action->menu_data->menu), action->cur_separator);
    action->cur_item = gtk_radio_menu_item_new_with_label (NULL, "");
    gtk_menu_shell_prepend (GTK_MENU_SHELL (action->menu_data->menu), action->cur_item);

    if (action->cur_enc)
    {
        Encoding *enc = action->cur_enc;
        action->cur_enc = NULL;
        _moo_encodings_menu_action_set_current (gtkaction, enc->name);
    }

    return menu_item;
}

static void
moo_encodings_menu_action_finalize (GObject *object)
{
    MooEncodingsMenuAction *action = MOO_ENCODINGS_MENU_ACTION (object);
    if (action->update_idle)
        g_source_remove (action->update_idle);
    G_OBJECT_CLASS (moo_encodings_menu_action_parent_class)->finalize (object);
}

static void
moo_encodings_menu_action_class_init (MooEncodingsMenuActionClass *klass)
{
    G_OBJECT_CLASS(klass)->finalize = moo_encodings_menu_action_finalize;
    GTK_ACTION_CLASS (klass)->create_menu_item = moo_encodings_menu_action_create_menu_item;
}

GtkAction *
_moo_encodings_menu_action_new (const char            *id,
                                const char            *label,
                                MooEncodingsMenuFunc   func,
                                gpointer               data)
{
    MooEncodingsMenuAction *action;

    g_return_val_if_fail (id != NULL, NULL);
    g_return_val_if_fail (label != NULL, NULL);
    g_return_val_if_fail (func != NULL, NULL);

    action = MOO_ENCODINGS_MENU_ACTION (g_object_new (MOO_TYPE_ENCODINGS_MENU_ACTION,
                                                      "name", id, "label", label,
                                                      (const char*) NULL));
    action->func = func;
    action->func_data = data;

    return GTK_ACTION (action);
}

static gboolean
update_recent_list_visibility (MooEncodingsMenuAction *action)
{
    action->update_idle = 0;

    if (action->cur_enc)
    {
        GtkWidget *child;

        gtk_widget_show (action->cur_item);
        gtk_widget_show (action->cur_separator);

        child = GTK_BIN (action->cur_item)->child;
        gtk_label_set_text (GTK_LABEL (child), action->cur_enc->display_name);

        exclude_item (action->menu_data, action->cur_enc);
    }
    else
    {
        gtk_widget_hide (action->cur_item);
        gtk_widget_hide (action->cur_separator);
    }

    return FALSE;
}

void
_moo_encodings_menu_action_set_current (GtkAction  *gtkaction,
                                        const char *enc_name)
{
    MooEncodingsMenuAction *action;
    Encoding *enc;
    EncodingsManager *mgr;

    g_return_if_fail (MOO_IS_ENCODINGS_MENU_ACTION (gtkaction));

    action = MOO_ENCODINGS_MENU_ACTION (gtkaction);

    mgr = get_enc_mgr ();
    enc = enc_name ? get_encoding (mgr, enc_name) : NULL;

    if (enc != action->cur_enc)
    {
        action->cur_enc = enc;
        if (action->cur_item && !action->update_idle)
            action->update_idle = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE + 1,
                                                   (GSourceFunc) update_recent_list_visibility,
                                                   action, NULL);
    }
}
