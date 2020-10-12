/*
 *   mooeditprefspage.c
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

#include "mooedit/mooedit-impl.h"
#include "mooedit/mooeditor-impl.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/moolangmgr.h"
#include "mooedit/mooeditfiltersettings.h"
#include "mooutils/mooprefsdialog.h"
#include "mooutils/moostock.h"
#include "mooutils/mooglade.h"
#include "mooutils/moofontsel.h"
#include "mooutils/mooutils-treeview.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooencodings.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moohelp.h"
#include "mooedit/mooeditprefs-file-gxml.h"
#include "mooedit/mooeditprefs-filters-gxml.h"
#include "mooedit/mooeditprefs-general-gxml.h"
#include "mooedit/mooeditprefs-langs-gxml.h"
#include "mooedit/mooeditprefs-view-gxml.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <string.h>


typedef struct PrefsPage PrefsPage;


static void     prefs_page_apply_lang_prefs (MooPrefsPage       *page);
static void     apply_filter_settings       (PrefsFiltersXml    *gxml);

static void     scheme_combo_init           (GtkComboBox        *combo);
static void     scheme_combo_data_func      (GtkCellLayout      *layout,
                                             GtkCellRenderer    *cell,
                                             GtkTreeModel       *model,
                                             GtkTreeIter        *iter);
static void     scheme_combo_set_scheme     (GtkComboBox        *combo,
                                             MooTextStyleScheme *scheme);

static void     lang_combo_init             (GtkComboBox        *combo,
                                             MooPrefsPage       *page,
                                             PrefsLangsXml      *gxml);

static void     filter_treeview_init        (PrefsFiltersXml    *gxml);

static GtkTreeModel *create_lang_model      (void);

static void     save_encoding_combo_init    (PrefsFileXml       *gxml);
static void     save_encoding_combo_apply   (PrefsFileXml       *gxml);

static GtkTreeModel *page_get_lang_model    (MooPrefsPage       *page);
static MooTextStyleScheme *page_get_scheme  (PrefsGeneralXml    *gxml);


static GtkWidget *
prefs_page_new (MooEditor          *editor,
                const char         *label,
                const char         *stock_id,
                MooPrefsPageInitUi  init_ui,
                MooPrefsPageInit    init,
                MooPrefsPageApply   apply)
{
    GtkWidget *prefs_page;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    _moo_edit_init_config ();

    prefs_page = moo_prefs_page_new (label, stock_id);
    moo_prefs_page_set_callbacks (MOO_PREFS_PAGE (prefs_page),
                                  init_ui, init, apply);

    g_signal_connect_swapped (prefs_page, "apply",
                              G_CALLBACK (_moo_editor_apply_prefs),
                              editor);

    return prefs_page;
}

#define BIND_SETTING(wid,setting) \
    moo_prefs_page_bind_setting (page, GTK_WIDGET (gxml->wid), \
                                 moo_edit_setting (setting))
#define BIND_SENSITIVE(btn, wid) \
    moo_bind_sensitive (GTK_WIDGET (gxml->btn), GTK_WIDGET (gxml->wid), FALSE)

static void
page_general_init_ui (MooPrefsPage *page)
{
    PrefsGeneralXml *gxml;

    gxml = prefs_general_xml_new_with_root (GTK_WIDGET (page));
    g_object_set_data (G_OBJECT (page), "moo-edit-prefs-page-xml", gxml);

    BIND_SETTING (smarthome, MOO_EDIT_PREFS_SMART_HOME_END);
    BIND_SETTING (enable_auto_indent, MOO_EDIT_PREFS_AUTO_INDENT);
    BIND_SETTING (usespaces, MOO_EDIT_PREFS_SPACES_NO_TABS);
    BIND_SETTING (backspace_indents, MOO_EDIT_PREFS_BACKSPACE_INDENTS);
    BIND_SETTING (tab_indents, MOO_EDIT_PREFS_TAB_INDENTS);
    BIND_SETTING (tab_width, MOO_EDIT_PREFS_TAB_WIDTH);
    BIND_SETTING (indent_width, MOO_EDIT_PREFS_INDENT_WIDTH);
    BIND_SETTING (fontbutton, MOO_EDIT_PREFS_FONT);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (page), HELP_SECTION_PREFS_GENERAL);
#endif
}

static void
tab_width_changed (GtkSpinButton *tab_width,
                   GtkSpinButton *indent_width)
{
    gtk_spin_button_set_value (indent_width,
                               gtk_spin_button_get_value_as_int (tab_width));
}

static void
page_general_init (MooPrefsPage *page)
{
    PrefsGeneralXml *gxml = (PrefsGeneralXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");

    g_signal_connect (gxml->tab_width, "value-changed",
                      G_CALLBACK (tab_width_changed),
                      gxml->indent_width);

    {
        MooTextStyleScheme *scheme;

        g_object_set (gxml->fontbutton, "monospace", TRUE, nullptr);

        scheme = moo_lang_mgr_get_active_scheme (moo_lang_mgr_default ());
        g_return_if_fail (scheme != NULL);

        scheme_combo_init (gxml->color_scheme_combo);
        scheme_combo_set_scheme (gxml->color_scheme_combo, scheme);
    }
}

static void
page_general_apply (MooPrefsPage *page)
{
    PrefsGeneralXml *gxml = (PrefsGeneralXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");
    MooTextStyleScheme *scheme = page_get_scheme (gxml);

    if (scheme)
    {
        moo_prefs_set_string (moo_edit_setting (MOO_EDIT_PREFS_COLOR_SCHEME),
                              moo_text_style_scheme_get_id (scheme));
        g_object_unref (scheme);
    }
}

GtkWidget *
moo_edit_prefs_page_new_1 (MooEditor *editor)
{
    return prefs_page_new (editor,
                           /* Label of a Preferences dialog page, remove the part before and including | */
                           Q_("PreferencesPage|General"),
                           GTK_STOCK_EDIT,
                           page_general_init_ui,
                           page_general_init,
                           page_general_apply);
}


static void
page_filters_init_ui (MooPrefsPage *page)
{
    PrefsFiltersXml *gxml;
    gxml = prefs_filters_xml_new_with_root (GTK_WIDGET (page));
    g_object_set_data (G_OBJECT (page), "moo-edit-prefs-page-xml", gxml);
#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (page), HELP_SECTION_PREFS_FILTERS);
#endif
}

static void
page_filters_init (MooPrefsPage *page)
{
    PrefsFiltersXml *gxml = (PrefsFiltersXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");
    filter_treeview_init (gxml);
}

static void
page_filters_apply (MooPrefsPage *page)
{
    PrefsFiltersXml *gxml = (PrefsFiltersXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");
    apply_filter_settings (gxml);
}

GtkWidget *
moo_edit_prefs_page_new_5 (MooEditor *editor)
{
    return prefs_page_new (editor,
                           /* Label of a Preferences dialog page, remove the part before and including | */
                           Q_("PreferencesPage|File Filters"),
                           GTK_STOCK_EDIT,
                           page_filters_init_ui,
                           page_filters_init,
                           page_filters_apply);
}


static void
page_view_init_ui (MooPrefsPage *page)
{
    PrefsViewXml *gxml;

    gxml = prefs_view_xml_new_with_root (GTK_WIDGET (page));
    g_object_set_data (G_OBJECT (page), "moo-edit-prefs-page-xml", gxml);

    BIND_SENSITIVE (enable_wrapping, dont_split_words);
    BIND_SENSITIVE (draw_rigth_margin, right_margin_box);
    BIND_SETTING (enable_wrapping, MOO_EDIT_PREFS_WRAP_ENABLE);
    BIND_SETTING (dont_split_words, MOO_EDIT_PREFS_WRAP_WORDS);
    BIND_SETTING (check_enable_highlighting, MOO_EDIT_PREFS_ENABLE_HIGHLIGHTING);
    BIND_SETTING (check_highlight_matching_brackets, MOO_EDIT_PREFS_HIGHLIGHT_MATCHING);
    BIND_SETTING (check_highlight_mismatching_brackets, MOO_EDIT_PREFS_HIGHLIGHT_MISMATCHING);
    BIND_SETTING (check_highlight_current_line, MOO_EDIT_PREFS_HIGHLIGHT_CURRENT_LINE);
    BIND_SETTING (show_line_numbers, MOO_EDIT_PREFS_SHOW_LINE_NUMBERS);
    BIND_SETTING (check_show_tabs, MOO_EDIT_PREFS_SHOW_TABS);
    BIND_SETTING (check_show_spaces, MOO_EDIT_PREFS_SHOW_SPACES);
    BIND_SETTING (check_show_trailing_spaces, MOO_EDIT_PREFS_SHOW_TRAILING_SPACES);
    BIND_SETTING (draw_rigth_margin, MOO_EDIT_PREFS_DRAW_RIGHT_MARGIN);
    BIND_SETTING (spin_right_margin_offset, MOO_EDIT_PREFS_RIGHT_MARGIN_OFFSET);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (page), HELP_SECTION_PREFS_VIEW);
#endif
}

static void
page_view_init (MooPrefsPage *page)
{
    PrefsViewXml *gxml = (PrefsViewXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");
    MOO_UNUSED (gxml);
}

static void
page_view_apply (MooPrefsPage *page)
{
    PrefsViewXml *gxml = (PrefsViewXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");
    MOO_UNUSED (gxml);
}

GtkWidget *
moo_edit_prefs_page_new_2 (MooEditor *editor)
{
    return prefs_page_new (editor,
                           /* Label of a Preferences dialog page, remove the part before and including | */
                           Q_("PreferencesPage|View"),
                           GTK_STOCK_EDIT,
                           page_view_init_ui,
                           page_view_init,
                           page_view_apply);
}


static void
page_file_init_ui (MooPrefsPage *page)
{
    PrefsFileXml *gxml;

    gxml = prefs_file_xml_new_with_root (GTK_WIDGET (page));
    g_object_set_data (G_OBJECT (page), "moo-edit-prefs-page-xml", gxml);

    BIND_SETTING (entry_encodings, MOO_EDIT_PREFS_ENCODINGS);
    BIND_SETTING (check_strip, MOO_EDIT_PREFS_STRIP);
    BIND_SETTING (check_add_newline, MOO_EDIT_PREFS_ADD_NEWLINE);
    BIND_SETTING (check_make_backups, MOO_EDIT_PREFS_MAKE_BACKUPS);
    BIND_SETTING (check_save_session, MOO_EDIT_PREFS_SAVE_SESSION);
    BIND_SETTING (check_open_dialog_follows_doc, MOO_EDIT_PREFS_DIALOGS_OPEN_FOLLOWS_DOC);
    BIND_SETTING (check_auto_sync, MOO_EDIT_PREFS_AUTO_SYNC);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (page), HELP_SECTION_PREFS_FILE);
#endif
}

static void
page_file_init (MooPrefsPage *page)
{
    PrefsFileXml *gxml = (PrefsFileXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");

    save_encoding_combo_init (gxml);
}

static void
page_file_apply (MooPrefsPage *page)
{
    PrefsFileXml *gxml = (PrefsFileXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");

    save_encoding_combo_apply (gxml);
}

GtkWidget *
moo_edit_prefs_page_new_3 (MooEditor *editor)
{
    return prefs_page_new (editor,
                           /* Label of a Preferences dialog page, remove the part before and including | */
                           Q_("PreferencesPage|File"),
                           GTK_STOCK_EDIT,
                           page_file_init_ui,
                           page_file_init,
                           page_file_apply);
}


static void
page_langs_init_ui (MooPrefsPage *page)
{
    PrefsLangsXml *gxml;
    gxml = prefs_langs_xml_new_with_root (GTK_WIDGET (page));
    g_object_set_data (G_OBJECT (page), "moo-edit-prefs-page-xml", gxml);
#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (page), HELP_SECTION_PREFS_LANGS);
#endif
}

static void
page_langs_init (MooPrefsPage *page)
{
    PrefsLangsXml *gxml = (PrefsLangsXml*) g_object_get_data (G_OBJECT (page), "moo-edit-prefs-page-xml");
    MooTreeHelper *helper;

    lang_combo_init (gxml->lang_combo, page, gxml);

    helper = (MooTreeHelper*) g_object_get_data (G_OBJECT (page), "moo-tree-helper");
    _moo_tree_helper_update_widgets (helper);
}

static void
page_langs_apply (MooPrefsPage *page)
{
    prefs_page_apply_lang_prefs (page);
}

GtkWidget *
moo_edit_prefs_page_new_4 (MooEditor *editor)
{
    return prefs_page_new (editor,
                           /* Label of a Preferences dialog page, remove the part before and including | */
                           Q_("PreferencesPage|Languages"),
                           GTK_STOCK_EDIT,
                           page_langs_init_ui,
                           page_langs_init,
                           page_langs_apply);
}


static void
scheme_combo_init (GtkComboBox *combo)
{
    GtkListStore *store;
    MooLangMgr *mgr;
    GSList *list, *l;
    GtkCellRenderer *cell;

    mgr = moo_lang_mgr_default ();
    list = moo_lang_mgr_list_schemes (mgr);
    g_return_if_fail (list != NULL);

    store = gtk_list_store_new (1, MOO_TYPE_TEXT_STYLE_SCHEME);

    for (l = list; l != NULL; l = l->next)
    {
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, l->data, -1);
    }

    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), cell,
                                        (GtkCellLayoutDataFunc) scheme_combo_data_func,
                                        NULL, NULL);

    g_object_unref (store);
    g_slist_foreach (list, (GFunc) g_object_unref, NULL);
    g_slist_free (list);
}


static void
scheme_combo_data_func (G_GNUC_UNUSED GtkCellLayout *layout,
                        GtkCellRenderer    *cell,
                        GtkTreeModel       *model,
                        GtkTreeIter        *iter)
{
    MooTextStyleScheme *scheme = NULL;
    gtk_tree_model_get (model, iter, 0, &scheme, -1);
    g_return_if_fail (scheme != NULL);
    g_object_set (cell, "text", moo_text_style_scheme_get_name (scheme), nullptr);
    g_object_unref (scheme);
}


static GtkTreeModel *
page_get_lang_model (MooPrefsPage *page)
{
    GtkTreeModel *model;

    model = (GtkTreeModel*) g_object_get_data (G_OBJECT (page), "moo-lang-model");

    if (!model)
    {
        model = create_lang_model ();
        g_object_set_data_full (G_OBJECT (page), "moo-lang-model",
                                model, g_object_unref);
    }

    return model;
}


static void
scheme_combo_set_scheme (GtkComboBox        *combo,
                         MooTextStyleScheme *scheme)
{
    GtkTreeModel *model = gtk_combo_box_get_model (combo);
    gboolean found = FALSE;
    GtkTreeIter iter;

    gtk_tree_model_get_iter_first (model, &iter);
    do {
        MooTextStyleScheme *s;
        gtk_tree_model_get (model, &iter, 0, &s, -1);
        g_object_unref (s);
        if (scheme == s)
        {
            found = TRUE;
            break;
        }
    }
    while (gtk_tree_model_iter_next (model, &iter));

    g_return_if_fail (found);

    gtk_combo_box_set_active_iter (combo, &iter);
}


static MooTextStyleScheme *
page_get_scheme (PrefsGeneralXml *gxml)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    MooTextStyleScheme *scheme = NULL;

    if (!gtk_combo_box_get_active_iter (gxml->color_scheme_combo, &iter))
        return NULL;

    model = gtk_combo_box_get_model (gxml->color_scheme_combo);
    gtk_tree_model_get (model, &iter, 0, &scheme, -1);

    return scheme;
}


/*********************************************************************/
/* Language combo
 */

static void
fix_style (G_GNUC_UNUSED GtkWidget *combo)
{
#ifdef __WIN32__
    static gboolean been_here = FALSE;

    gtk_widget_set_name (combo, "moo-lang-combo");

    if (!been_here)
    {
        been_here = TRUE;
        gtk_rc_parse_string ("style \"moo-lang-combo\"\n"
                             "{\n"
                             "   GtkComboBox::appears-as-list = 0\n"
                             "}\n"
                             "widget \"*.moo-lang-combo\" style \"moo-lang-combo\"\n");
    }
#endif
}


enum {
    COLUMN_ID,
    COLUMN_NAME,
    COLUMN_LANG,
    COLUMN_EXTENSIONS,
    COLUMN_MIMETYPES,
    COLUMN_CONFIG
};


static char *
list_to_string (GSList  *list,
                gboolean free_list)
{
    GSList *l;
    GString *string = g_string_new (NULL);

    for (l = list; l != NULL; l = l->next)
    {
        g_string_append (string, (const char*) l->data);

        if (l->next)
            g_string_append_c (string, ';');
    }

    if (free_list)
    {
        g_slist_foreach (list, (GFunc) g_free, NULL);
        g_slist_free (list);
    }

    return g_string_free (string, FALSE);
}


static int
lang_cmp (MooLang *lang1,
          MooLang *lang2)
{
    const char *name1, *name2;

    name1 = _moo_lang_display_name (lang1);
    name2 = _moo_lang_display_name (lang2);

    return g_utf8_collate (name1, name2);
}

static GtkTreeModel *
create_lang_model (void)
{
    GtkTreeStore *store;
    MooLangMgr *mgr;
    GSList *langs, *sections, *l;
    GtkTreeIter iter;
    const char *config;
    char *ext, *mime;

    mgr = moo_lang_mgr_default ();
    langs = g_slist_sort (moo_lang_mgr_get_available_langs (mgr), (GCompareFunc) lang_cmp);
    sections = moo_lang_mgr_get_sections (mgr);

    store = gtk_tree_store_new (6, G_TYPE_STRING, G_TYPE_STRING, MOO_TYPE_LANG,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    gtk_tree_store_append (store, &iter, NULL);
    config = _moo_lang_mgr_get_config (mgr, MOO_LANG_NONE);
    ext = list_to_string (_moo_lang_mgr_get_globs (mgr, NULL), TRUE);
    mime = list_to_string (_moo_lang_mgr_get_mime_types (mgr, NULL), TRUE);
    gtk_tree_store_set (store, &iter,
                        COLUMN_ID, MOO_LANG_NONE,
                        /* Label in the Language combo box on Languages preferences page,
                           remove the part before and including | */
                        COLUMN_NAME, Q_("Language|None"),
                        COLUMN_CONFIG, config,
                        COLUMN_MIMETYPES, mime,
                        COLUMN_EXTENSIONS, ext,
                        -1);
    g_free (ext);
    g_free (mime);

    while (sections)
    {
        char *section = (char*) sections->data;

        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter, COLUMN_NAME, section, -1);

        for (l = langs; l != NULL; l = l->next)
        {
            MooLang *lang = (MooLang*) l->data;

            if (!_moo_lang_get_hidden (lang) &&
                !strcmp (_moo_lang_get_section (lang), section))
            {
                GtkTreeIter child;

                ext = list_to_string (_moo_lang_mgr_get_globs (mgr, _moo_lang_id (lang)), TRUE);
                mime = list_to_string (_moo_lang_mgr_get_mime_types (mgr, _moo_lang_id (lang)), TRUE);

                config = _moo_lang_mgr_get_config (mgr, _moo_lang_id (lang));
                gtk_tree_store_append (store, &child, &iter);
                gtk_tree_store_set (store, &child,
                                    COLUMN_ID, _moo_lang_id (lang),
                                    COLUMN_NAME, _moo_lang_display_name (lang),
                                    COLUMN_LANG, lang,
                                    COLUMN_CONFIG, config,
                                    COLUMN_MIMETYPES, mime,
                                    COLUMN_EXTENSIONS, ext,
                                    -1);

                g_free (ext);
                g_free (mime);
            }
        }

        g_free (section);
        sections = g_slist_delete_link (sections, sections);
    }

    g_slist_foreach (langs, (GFunc) g_object_unref, NULL);
    g_slist_free (langs);
    return GTK_TREE_MODEL (store);
}


#if 0
static gboolean
separator_func (GtkTreeModel *model,
                GtkTreeIter  *iter,
                G_GNUC_UNUSED gpointer data)
{
    char *name = NULL;

    gtk_tree_model_get (model, iter, COLUMN_NAME, &name, -1);

    if (!name)
        return TRUE;

    g_free (name);
    return FALSE;
}
#endif


static void
set_sensitive (G_GNUC_UNUSED GtkCellLayout *cell_layout,
               GtkCellRenderer *cell,
               GtkTreeModel    *model,
               GtkTreeIter     *iter,
               G_GNUC_UNUSED gpointer data)
{
    g_object_set (cell, "sensitive",
                  !gtk_tree_model_iter_has_child (model, iter),
                  nullptr);
}


/*********************************************************************/
/* Per-lang settings
 */

static void
helper_update_widgets (PrefsLangsXml      *gxml,
                       GtkTreeModel       *model,
                       G_GNUC_UNUSED GtkTreePath *path,
                       GtkTreeIter        *iter)
{
    MooLang *lang = NULL;
    char *ext = NULL, *mime = NULL, *id = NULL, *conf = NULL;

    g_return_if_fail (iter != NULL);

    gtk_tree_model_get (model, iter,
                        COLUMN_LANG, &lang,
                        COLUMN_MIMETYPES, &mime,
                        COLUMN_EXTENSIONS, &ext,
                        COLUMN_CONFIG, &conf,
                        COLUMN_ID, &id,
                        -1);
    g_return_if_fail (id != NULL);

    gtk_entry_set_text (gxml->extensions, MOO_NZS (ext));
    gtk_entry_set_text (gxml->mimetypes, MOO_NZS (mime));
    gtk_entry_set_text (gxml->config, MOO_NZS (conf));
    gtk_widget_set_sensitive (GTK_WIDGET (gxml->mimetypes), lang != NULL);
    gtk_widget_set_sensitive (GTK_WIDGET (gxml->label_mimetypes), lang != NULL);

    if (lang)
        g_object_unref (lang);

    g_free (conf);
    g_free (ext);
    g_free (mime);
    g_free (id);
}


static void
helper_update_model (PrefsLangsXml      *gxml,
                     GtkTreeModel       *model,
                     G_GNUC_UNUSED GtkTreePath *path,
                     GtkTreeIter        *iter)
{
    const char *ext, *mime, *conf;

    ext = gtk_entry_get_text (gxml->extensions);
    mime = gtk_entry_get_text (gxml->mimetypes);
    conf = gtk_entry_get_text (gxml->config);

    gtk_tree_store_set (GTK_TREE_STORE (model), iter,
                        COLUMN_MIMETYPES, mime,
                        COLUMN_EXTENSIONS, ext,
                        COLUMN_CONFIG, conf, -1);
}


static void
lang_combo_init (GtkComboBox   *combo,
                 MooPrefsPage  *page,
                 PrefsLangsXml *gxml)
{
    GtkTreeModel *model;
    GtkCellRenderer *cell;
    MooTreeHelper *helper;

    fix_style (GTK_WIDGET (combo));

    model = page_get_lang_model (page);
    g_return_if_fail (model != NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                    "text", COLUMN_NAME, nullptr);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), cell,
                                        set_sensitive, NULL, NULL);

    gtk_combo_box_set_model (combo, model);

    _moo_combo_box_select_first (combo);
    helper = _moo_tree_helper_new (GTK_WIDGET (combo), NULL, NULL, NULL, NULL);
    g_return_if_fail (helper != NULL);

    g_object_set_data_full (G_OBJECT (page), "moo-tree-helper",
                            helper, g_object_unref);
    g_signal_connect_swapped (helper, "update-widgets",
                              G_CALLBACK (helper_update_widgets), gxml);
    g_signal_connect_swapped (helper, "update-model",
                              G_CALLBACK (helper_update_model), gxml);
    _moo_tree_helper_update_widgets (helper);
}


static gboolean
apply_one_lang (GtkTreeModel *model,
                G_GNUC_UNUSED GtkTreePath *path,
                GtkTreeIter  *iter,
                MooLangMgr   *mgr)
{
    MooLang *lang = NULL;
    char *config = NULL, *id = NULL;
    char *mime, *ext;

    gtk_tree_model_get (model, iter,
                        COLUMN_LANG, &lang,
                        COLUMN_ID, &id,
                        COLUMN_CONFIG, &config, -1);

    if (!id)
        return FALSE;

    gtk_tree_model_get (model, iter,
                        COLUMN_MIMETYPES, &mime,
                        COLUMN_EXTENSIONS, &ext, -1);

    _moo_lang_mgr_set_mime_types (mgr, id, mime);
    _moo_lang_mgr_set_globs (mgr, id, ext);
    _moo_lang_mgr_set_config (mgr, id, config);

    if (lang)
        g_object_unref (lang);

    g_free (mime);
    g_free (ext);
    g_free (config);
    g_free (id);
    return FALSE;
}


static void
prefs_page_apply_lang_prefs (MooPrefsPage *page)
{
    GtkTreeModel *model;
    MooTreeHelper *helper;
    MooLangMgr *mgr;

    helper = (MooTreeHelper*) g_object_get_data (G_OBJECT (page), "moo-tree-helper");
    _moo_tree_helper_update_model (helper, NULL, NULL);

    model = page_get_lang_model (page);
    g_return_if_fail (model != NULL);

    mgr = moo_lang_mgr_default ();
    gtk_tree_model_foreach (model, (GtkTreeModelForeachFunc) apply_one_lang, mgr);
    _moo_lang_mgr_save_config (mgr);
    _moo_edit_queue_recheck_config_all ();
}


/*********************************************************************/
/* Filters
 */

enum {
    FILTER_COLUMN_INVALID,
    FILTER_COLUMN_INVALID_TOOLTIP,
    FILTER_COLUMN_FILTER,
    FILTER_COLUMN_CONFIG,
    FILTER_NUM_COLUMNS
};


static void
filter_store_set_modified (GObject* store,
                           gboolean modified)
{
    g_return_if_fail (GTK_IS_LIST_STORE (store));
    g_object_set_data (store, "filter-store-modified",
                       GINT_TO_POINTER (modified));
    _moo_tree_helper_set_modified ((MooTreeHelper*) g_object_get_data (store, "tree-helper"), FALSE);
}

static gboolean
filter_store_get_modified (GObject* store)
{
    g_return_val_if_fail (GTK_IS_LIST_STORE (store), FALSE);
    return g_object_get_data (store, "filter-store-modified") != NULL ||
           _moo_tree_helper_get_modified ((MooTreeHelper*) g_object_get_data (store, "tree-helper"));
}

static gboolean
check_filter (const char  *string,
              char       **message)
{
    gboolean result;
    MooEditFilter *filter;
    GError *error = NULL;

    if (message)
        *message = NULL;

    if (!string || !string[0])
        return TRUE;

    filter = _moo_edit_filter_new_full (string, MOO_EDIT_FILTER_CONFIG, &error);
    g_return_val_if_fail (filter != NULL, FALSE);

    result = _moo_edit_filter_valid (filter);

    if (error && message)
        *message = g_strdup (error->message);

    if (error)
        g_error_free (error);

    _moo_edit_filter_free (filter);
    return result;
}

static void
check_filter_row (GtkListStore *store,
                  GtkTreeIter  *iter)
{
    gboolean invalid;
    char *filter = NULL;
    char *message = NULL;

    gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
                        FILTER_COLUMN_FILTER, &filter,
                        -1);
    g_return_if_fail (filter != NULL);

    invalid = !check_filter (filter, &message);

    if (invalid)
        gtk_list_store_set (store, iter,
                            FILTER_COLUMN_INVALID, TRUE,
                            FILTER_COLUMN_INVALID_TOOLTIP, message,
                            -1);
    else
        gtk_list_store_set (store, iter,
                            FILTER_COLUMN_INVALID, FALSE,
                            FILTER_COLUMN_INVALID_TOOLTIP, NULL,
                            -1);

    g_free (message);
    g_free (filter);
}

static void
populate_filter_settings_store (GtkListStore *store)
{
    GSList *strings, *l;

    _moo_edit_filter_settings_load ();

    l = strings = _moo_edit_filter_settings_get_strings ();

    while (l)
    {
        GtkTreeIter iter;
        const char *filter;
        const char *config;
        gboolean valid;
        char *message = NULL;

        g_return_if_fail (l->next != NULL);

        filter = (const char*) l->data;
        config = (const char*) l->next->data;
        valid = check_filter (filter, &message);

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            FILTER_COLUMN_INVALID, !valid,
                            FILTER_COLUMN_INVALID_TOOLTIP, message,
                            FILTER_COLUMN_FILTER, filter,
                            FILTER_COLUMN_CONFIG, config,
                            -1);

        g_free (message);

        l = l->next->next;
    }

    g_slist_foreach (strings, (GFunc) g_free, NULL);
    g_slist_free (strings);
}


static void
filter_cell_edited (GtkCellRendererText *cell,
                    const char          *path_string,
                    const char          *text,
                    GtkListStore        *store)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    int column;
    char *old_text;

    g_return_if_fail (GTK_IS_LIST_STORE (store));

    column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "filter-store-column-id"));
    g_return_if_fail (column >= 0 && column < FILTER_NUM_COLUMNS);

    path = gtk_tree_path_new_from_string (path_string);

    if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
    {
        gtk_tree_path_free (path);
        return;
    }

    gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, column, &old_text, -1);

    if (!moo_str_equal (old_text, text))
    {
        gtk_list_store_set (store, &iter, column, text, -1);
        filter_store_set_modified (G_OBJECT (store), TRUE);
        check_filter_row (store, &iter);
    }

    g_free (old_text);
    gtk_tree_path_free (path);
}


static void
filter_icon_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                       GtkCellRenderer *cell,
                       GtkTreeModel    *model,
                       GtkTreeIter     *iter)
{
    gboolean invalid;
    gtk_tree_model_get (model, iter, FILTER_COLUMN_INVALID, &invalid, -1);
    g_object_set (cell, "visible", invalid, nullptr);
}

static void
create_filter_column (GtkTreeView  *treeview,
                      GtkListStore *store,
                      const char   *title,
                      int           column_id)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, title);
    gtk_tree_view_append_column (treeview, column);

    if (column_id == FILTER_COLUMN_FILTER)
    {
        cell = gtk_cell_renderer_pixbuf_new ();
        g_object_set (cell, "stock-id", GTK_STOCK_DIALOG_ERROR, nullptr);
        gtk_tree_view_column_pack_start (column, cell, FALSE);
        gtk_tree_view_column_set_cell_data_func (column, cell,
                                                 (GtkTreeCellDataFunc) filter_icon_data_func,
                                                 NULL, NULL);
    }

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_attributes (column, cell, "text", column_id, nullptr);

    g_object_set (cell, "editable", TRUE, nullptr);
    g_object_set_data (G_OBJECT (cell), "filter-store-column-id", GINT_TO_POINTER (column_id));
    g_signal_connect (cell, "edited", G_CALLBACK (filter_cell_edited), store);
}


static void
filter_treeview_init (PrefsFiltersXml *gxml)
{
    GtkListStore *store;
    MooTreeHelper *helper;

    store = gtk_list_store_new (FILTER_NUM_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    populate_filter_settings_store (store);
    gtk_tree_view_set_model (gxml->filter_treeview, GTK_TREE_MODEL (store));

    /* Column label on File Filters prefs page */
    create_filter_column (gxml->filter_treeview, store, C_("filter-prefs-column", "Filter"), FILTER_COLUMN_FILTER);
    /* Column label on File Filters prefs page */
    create_filter_column (gxml->filter_treeview, store, C_("filter-prefs-column", "Options"), FILTER_COLUMN_CONFIG);

    gtk_tree_view_set_tooltip_column (gxml->filter_treeview, FILTER_COLUMN_INVALID_TOOLTIP);

    helper = _moo_tree_helper_new (GTK_WIDGET (gxml->filter_treeview),
                                   GTK_WIDGET (gxml->new_filter_setting),
                                   GTK_WIDGET (gxml->delete_filter_setting),
                                   GTK_WIDGET (gxml->filter_setting_up),
                                   GTK_WIDGET (gxml->filter_setting_down));
    g_object_set_data_full (G_OBJECT (gxml->filter_treeview), "tree-helper", helper, g_object_unref);
    g_object_set_data (G_OBJECT (store), "tree-helper", helper);

    _moo_tree_helper_update_widgets (helper);

    g_object_unref (store);
}


static gboolean
prepend_filter_and_config (GtkTreeModel *model,
                           G_GNUC_UNUSED GtkTreePath *path,
                           GtkTreeIter  *iter,
                           GSList      **list)
{
    char *filter = NULL, *config = NULL;

    gtk_tree_model_get (model, iter,
                        FILTER_COLUMN_FILTER, &filter,
                        FILTER_COLUMN_CONFIG, &config,
                        -1);

    *list = g_slist_prepend (*list, filter ? filter : g_strdup (""));
    *list = g_slist_prepend (*list, config ? config : g_strdup (""));

    return FALSE;
}

static void
apply_filter_settings (PrefsFiltersXml *gxml)
{
    GSList *strings = NULL;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model (gxml->filter_treeview);

    if (!filter_store_get_modified (G_OBJECT (model)))
        return;

    gtk_tree_model_foreach (model,
                            (GtkTreeModelForeachFunc) prepend_filter_and_config,
                            &strings);
    strings = g_slist_reverse (strings);

    _moo_edit_filter_settings_set_strings (strings);
    filter_store_set_modified (G_OBJECT (model), FALSE);

    g_slist_foreach (strings, (GFunc) g_free, NULL);
    g_slist_free (strings);
}


/**************************************************************************/
/* Encoding combos
 */

static void
save_encoding_combo_init (PrefsFileXml *gxml)
{
    const char *enc;
    _moo_encodings_combo_init (GTK_COMBO_BOX (gxml->encoding_save),
                               MOO_ENCODING_COMBO_SAVE, TRUE);
    enc = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_ENCODING_SAVE));
    _moo_encodings_combo_set_enc (GTK_COMBO_BOX (gxml->encoding_save),
                                  enc, MOO_ENCODING_COMBO_SAVE);
}


static void
save_encoding_combo_apply (PrefsFileXml *gxml)
{
    const char *enc;
    enc = _moo_encodings_combo_get_enc (GTK_COMBO_BOX (gxml->encoding_save),
                                        MOO_ENCODING_COMBO_SAVE);
    moo_prefs_set_string (moo_edit_setting (MOO_EDIT_PREFS_ENCODING_SAVE), enc);
}
