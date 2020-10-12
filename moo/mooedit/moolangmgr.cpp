/*
 *   moolangmgr.c
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
#include "mooedit/moolangmgr-private.h"
#include "mooedit/moolang-private.h"
#include "mooedit/mooeditprefs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooprefs.h"
#include "marshals.h"
#include "mooutils/moo-mime.h"
#include <string.h>

#define LANGUAGE_DIR            "language-specs"
#define ELEMENT_LANG_CONFIG     MOO_EDIT_PREFS_PREFIX "/langs"
#define ELEMENT_LANG            "lang"
#define ELEMENT_EXTENSIONS      "globs"
#define ELEMENT_MIME_TYPES      "mime-types"
#define ELEMENT_CONFIG          "config"
#define PROP_LANG_ID            "id"
#define SCHEME_DEFAULT          "medit"


typedef struct {
    MooLang *lang;
    GSList *globs;
    GSList *mime_types;
    guint globs_modified : 1;
    guint mime_types_modified : 1;
} LangInfo;

static LangInfo *lang_info_new          (void);
static void     lang_info_free          (LangInfo   *info);

static void     string_list_free        (GSList     *list);
static GSList  *string_list_copy        (GSList     *list);
static void     read_langs              (MooLangMgr *mgr);
static void     read_schemes            (MooLangMgr *mgr);
static void     load_config             (MooLangMgr *mgr);
static MooLang *get_lang_for_filename   (MooLangMgr *mgr,
                                         const char *filename);
static MooLang *get_lang_for_mime_type  (MooLangMgr *mgr,
                                         const char *mime_type);


G_DEFINE_TYPE (MooLangMgr, moo_lang_mgr, G_TYPE_OBJECT)


static void
moo_lang_mgr_init (MooLangMgr *mgr)
{
    char **dirs;

    mgr->schemes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

    mgr->langs = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, (GDestroyNotify) lang_info_free);
    mgr->config = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, g_free);

    mgr->lang_mgr = gtk_source_language_manager_new ();
    dirs = moo_get_data_subdirs (LANGUAGE_DIR);
    g_object_set (mgr->lang_mgr, "search-path", dirs, nullptr);
    mgr->style_mgr = gtk_source_style_scheme_manager_new ();
    gtk_source_style_scheme_manager_set_search_path (mgr->style_mgr, dirs);

    load_config (mgr);

    g_strfreev (dirs);
}


static void
moo_lang_mgr_dispose (GObject *object)
{
    MooLangMgr *mgr = MOO_LANG_MGR (object);

    if (mgr->langs)
    {
        g_object_unref (mgr->lang_mgr);
        g_object_unref (mgr->style_mgr);
        g_hash_table_destroy (mgr->langs);
        g_hash_table_destroy (mgr->config);
        g_hash_table_destroy (mgr->schemes);
        mgr->style_mgr = NULL;
        mgr->langs = NULL;
        mgr->config = NULL;
        mgr->active_scheme = NULL;
        mgr->schemes = NULL;
    }

    G_OBJECT_CLASS (moo_lang_mgr_parent_class)->dispose (object);
}


static void
moo_lang_mgr_class_init (MooLangMgrClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = moo_lang_mgr_dispose;

    g_signal_new ("loaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _moo_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}


MooLangMgr *
moo_lang_mgr_new (void)
{
    return MOO_LANG_MGR (g_object_new (MOO_TYPE_LANG_MGR, (const char*) NULL));
}

MooLangMgr *
moo_lang_mgr_default (void)
{
    static MooLangMgr *instance;
    if (!instance)
        instance = moo_lang_mgr_new ();
    return instance;
}


static LangInfo *
lang_info_new (void)
{
    LangInfo *info = g_new0 (LangInfo, 1);
    info->lang = NULL;
    info->globs = NULL;
    info->mime_types = NULL;
    return info;
}


static void
lang_info_free (LangInfo *info)
{
    if (info)
    {
        if (info->lang)
            g_object_unref (info->lang);
        string_list_free (info->globs);
        string_list_free (info->mime_types);
        g_free (info);
    }
}


static LangInfo *
get_lang_info (MooLangMgr *mgr,
               const char *lang_id,
               gboolean    create)
{
    LangInfo *info;

    info = (LangInfo*) g_hash_table_lookup (mgr->langs, lang_id);

    if (!info && create)
    {
        info = lang_info_new ();
        g_hash_table_insert (mgr->langs, g_strdup (lang_id), info);
    }

    return info;
}


static GSList *
string_list_copy (GSList *list)
{
    GSList *copy = NULL;

    while (list)
    {
        copy = g_slist_prepend (copy, g_strdup ((const char*) list->data));
        list = list->next;
    }

    return g_slist_reverse (copy);
}

static void
string_list_free (GSList *list)
{
    g_slist_foreach (list, (GFunc) g_free, NULL);
    g_slist_free (list);
}


MooLang *
moo_lang_mgr_get_lang (MooLangMgr *mgr,
                       const char *name)
{
    MooLang *lang;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    if (!name)
        return NULL;

    lang = _moo_lang_mgr_find_lang (mgr, name);

    if (!lang)
        g_message ("could not find language '%s'", name);

    return lang;
}

MooLang *
_moo_lang_mgr_find_lang (MooLangMgr *mgr,
                         const char *name)
{
    char *id;
    LangInfo *info;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    if (!strcmp (name, MOO_LANG_NONE))
        return NULL;

    if (!g_ascii_strcasecmp (name, "c++"))
        name = "cpp";
    else if (!g_ascii_strcasecmp (name, "nxml"))
        name = "xml";
    else if (!g_ascii_strcasecmp (name, "cperl"))
        name = "perl";

    read_langs (mgr);
    id = _moo_lang_id_from_name (name);
    info = get_lang_info (mgr, id, FALSE);
    g_free (id);

    return info ? info->lang : NULL;
}


static MooLang *
get_lang_by_extension (MooLangMgr *mgr,
                       const char *filename)
{
    MooLang *lang = NULL;
    char *basename;
    GSList *langs, *l;
    gboolean found = FALSE;

    g_return_val_if_fail (filename != NULL, NULL);

    read_langs (mgr);

    basename = g_path_get_basename (filename);
    g_return_val_if_fail (basename != NULL, NULL);

    langs = moo_lang_mgr_get_available_langs (mgr);

    for (l = langs; !found && l != NULL; l = l->next)
    {
        GSList *g, *globs;

        lang = (MooLang*) l->data;
        globs = _moo_lang_mgr_get_globs (mgr, _moo_lang_id (lang));

        for (g = globs; !found && g != NULL; g = g->next)
        {
            if (_moo_glob_match_simple ((char*) g->data, basename))
            {
                found = TRUE;
                break;
            }
        }

        string_list_free (globs);
    }

    if (!found)
        lang = NULL;

    g_slist_foreach (langs, (GFunc) g_object_unref, NULL);
    g_slist_free (langs);
    g_free (basename);
    return lang;
}


static MooLang *
lang_mgr_get_lang_for_bak_filename (MooLangMgr *mgr,
                                    const char *filename)
{
    MooLang *lang = NULL;
    char *base = NULL;
    gsize len;
    guint i;

    static const char *bak_globs[] = {"*~", "*.bak", "*.in", "*.orig"};

    read_langs (mgr);

    len = strlen (filename);

    for (i = 0; i < G_N_ELEMENTS (bak_globs); ++i)
    {
        gsize ext_len = strlen (bak_globs[i]) - 1;

        if (len > ext_len && _moo_glob_match_simple (bak_globs[i], filename))
        {
            base = g_strndup (filename, len - ext_len);
            break;
        }
    }

    if (base)
        lang = get_lang_for_filename (mgr, base);

    g_free (base);
    return lang;
}


static gboolean
filename_blacklisted (MooLangMgr *mgr,
                      const char *filename)
{
    /* XXX bak files */
    char *basename;
    gboolean result = FALSE;
    LangInfo *info;
    GSList *l;

    read_langs (mgr);

    basename = g_path_get_basename (filename);
    g_return_val_if_fail (basename != NULL, FALSE);

    info = get_lang_info (mgr, MOO_LANG_NONE, FALSE);

    if (info)
        for (l = info->globs; !result && l != NULL; l = l->next)
            if (_moo_glob_match_simple ((char*) l->data, basename))
                result = TRUE;

    g_free (basename);
    return result;
}

static gboolean
file_blacklisted (MooLangMgr *mgr,
                  const char *filename)
{
    /* XXX mime type */
    return filename_blacklisted (mgr, filename);
}


MooLang *
moo_lang_mgr_get_lang_for_file (MooLangMgr *mgr,
                                GFile      *file)
{
    MooLang *lang = NULL;
    const char *mime_type;
    char *filename;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    read_langs (mgr);

    filename = g_file_get_parse_name (file);

    if (file_blacklisted (mgr, filename))
        goto out;

    lang = get_lang_by_extension (mgr, filename);

    if (lang)
        goto out;

    lang = lang_mgr_get_lang_for_bak_filename (mgr, filename);

    if (lang)
        goto out;

    mime_type = moo_get_mime_type_for_file (filename, NULL);

    if (mime_type != MOO_MIME_TYPE_UNKNOWN)
        lang = get_lang_for_mime_type (mgr, mime_type);

    if (lang)
        goto out;

out:
    g_free (filename);
    return lang;
}


static MooLang *
get_lang_for_filename (MooLangMgr *mgr,
                       const char *filename)
{
    MooLang *lang = NULL;
    const char *mime_type;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);
    g_return_val_if_fail (filename != NULL, NULL);

    read_langs (mgr);

    if (filename_blacklisted (mgr, filename))
        return NULL;

    lang = get_lang_by_extension (mgr, filename);

    if (lang)
        return lang;

    mime_type = moo_get_mime_type_for_filename (filename);

    if (mime_type != MOO_MIME_TYPE_UNKNOWN)
        lang = get_lang_for_mime_type (mgr, mime_type);

    if (lang)
        return lang;

    lang = lang_mgr_get_lang_for_bak_filename (mgr, filename);

    if (lang)
        return lang;

    return NULL;
}


static int
check_mime_subclass (const char *base,
                     const char *mime)
{
    return !moo_mime_type_is_subclass (mime, base);
}


static MooLang *
get_lang_for_mime_type (MooLangMgr *mgr,
                        const char *mime)
{
    GSList *l, *langs;
    MooLang *lang = NULL;
    gboolean found = FALSE;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);
    g_return_val_if_fail (mime != NULL, NULL);

    langs = moo_lang_mgr_get_available_langs (mgr);

    for (l = langs; !found && l != NULL; l = l->next)
    {
        GSList *mimetypes;

        lang = (MooLang*) l->data;
        mimetypes = _moo_lang_mgr_get_mime_types (mgr, _moo_lang_id (lang));

        if (g_slist_find_custom (mimetypes, mime, (GCompareFunc) strcmp))
            found = TRUE;

        string_list_free (mimetypes);
    }

    for (l = langs; !found && l != NULL; l = l->next)
    {
        GSList *mimetypes;

        lang = (MooLang*) l->data;
        mimetypes = _moo_lang_mgr_get_mime_types (mgr, _moo_lang_id (lang));

        if (g_slist_find_custom (mimetypes, mime, (GCompareFunc) check_mime_subclass))
            found = TRUE;

        string_list_free (mimetypes);
    }

    g_slist_foreach (langs, (GFunc) g_object_unref, NULL);
    g_slist_free (langs);
    return found ? lang : NULL;
}


static void
read_langs (MooLangMgr *mgr)
{
    const char * const *ids = NULL;

    if (mgr->got_langs)
        return;

    read_schemes (mgr);

    mgr->got_langs = TRUE;

    ids = gtk_source_language_manager_get_language_ids (mgr->lang_mgr);

    get_lang_info (mgr, MOO_LANG_NONE, TRUE);

    while (ids && *ids)
    {
        LangInfo *info;
        const char *id;
        GtkSourceLanguage *lang;

        id = *ids;
        lang = gtk_source_language_manager_get_language (mgr->lang_mgr, id);

        info = get_lang_info (mgr, id, TRUE);
        info->lang = (MooLang*) g_object_ref (lang);

        if (!info->globs_modified)
            info->globs = _moo_lang_get_globs (info->lang);
        if (!info->mime_types_modified)
            info->mime_types = _moo_lang_get_mime_types (info->lang);

        ids++;
    }

    g_signal_emit_by_name (mgr, "loaded");
}


static void
set_list (MooLangMgr *mgr,
          const char *lang_id,
          const char *string,
          gboolean    globs)
{
    GSList **ptr;
    LangInfo *info;

    info = get_lang_info (mgr, lang_id, TRUE);

    ptr = globs ? &info->globs : &info->mime_types;

    if (*ptr)
        string_list_free (*ptr);

    *ptr = _moo_lang_parse_string_list (string);

    if (globs)
        info->globs_modified = TRUE;
    else
        info->mime_types_modified = TRUE;
}

static void
load_lang_node (MooLangMgr    *mgr,
                MooMarkupNode *lang_node)
{
    const char *lang_id;
    MooMarkupNode *node;

    lang_id = moo_markup_get_prop (lang_node, PROP_LANG_ID);
    g_return_if_fail (lang_id != NULL);

    for (node = lang_node->children; node != NULL; node = node->next)
    {
        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        if (!strcmp (node->name, ELEMENT_EXTENSIONS))
        {
            set_list (mgr, lang_id, moo_markup_get_content (node), TRUE);
        }
        else if (!strcmp (node->name, ELEMENT_MIME_TYPES))
        {
            set_list (mgr, lang_id, moo_markup_get_content (node), FALSE);
        }
        else if (!strcmp (node->name, ELEMENT_CONFIG))
        {
            const char *string = moo_markup_get_content (node);
            g_hash_table_insert (mgr->config, g_strdup (lang_id), g_strdup (string));
        }
        else
        {
            /* 'extensions' is leftover from previous versions */
            if (strcmp (node->name, "extensions") != 0)
                g_warning ("ignoring node '%s'", node->name);
        }
    }
}


static void
set_default_config (MooLangMgr *mgr)
{
    _moo_lang_mgr_set_config (mgr, "makefile", "use-tabs: true");
    _moo_lang_mgr_set_config (mgr, "diff", "strip: false");
}

static void
load_config (MooLangMgr *mgr)
{
    MooMarkupNode *xml;
    MooMarkupNode *root, *node;

    set_default_config (mgr);
    mgr->modified = FALSE;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    root = moo_markup_get_element (xml, ELEMENT_LANG_CONFIG);

    if (!root)
        return;

    for (node = root->children; node != NULL; node = node->next)
    {
        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        if (strcmp (node->name, ELEMENT_LANG))
        {
            g_warning ("invalid '%s' element", node->name);
            continue;
        }

        load_lang_node (mgr, node);
    }

    mgr->modified = FALSE;
}


MooTextStyleScheme *
moo_lang_mgr_get_active_scheme (MooLangMgr *mgr)
{
    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_schemes (mgr);

    return mgr->active_scheme;
}


void
_moo_lang_mgr_set_active_scheme (MooLangMgr *mgr,
                                 const char *name)
{
    MooTextStyleScheme *scheme = NULL;

    g_return_if_fail (MOO_IS_LANG_MGR (mgr));

    read_schemes (mgr);

    if (name)
        scheme = (MooTextStyleScheme*) g_hash_table_lookup (mgr->schemes, name);
    if (!scheme)
        scheme = (MooTextStyleScheme*) g_hash_table_lookup (mgr->schemes, SCHEME_DEFAULT);

    if (!scheme)
        g_message ("could not find style scheme '%s'", name ? name : "<NULL>");
    else
        mgr->active_scheme = scheme;
}


static void
prepend_scheme (G_GNUC_UNUSED const char *name,
                MooTextStyleScheme *scheme,
                GSList            **list)
{
    *list = g_slist_prepend (*list, g_object_ref (scheme));
}

GSList *
moo_lang_mgr_list_schemes (MooLangMgr *mgr)
{
    GSList *list = NULL;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_schemes (mgr);
    g_hash_table_foreach (mgr->schemes, (GHFunc) prepend_scheme, &list);

    return list;
}


static void
read_schemes (MooLangMgr *mgr)
{
    const char *const *ids;

    if (mgr->got_schemes)
        return;

    mgr->got_schemes = TRUE;

    ids = gtk_source_style_scheme_manager_get_scheme_ids (mgr->style_mgr);

    while (ids && *ids)
    {
        MooTextStyleScheme *scheme;

        scheme = MOO_TEXT_STYLE_SCHEME (gtk_source_style_scheme_manager_get_scheme (mgr->style_mgr, *ids));

        if (!scheme)
        {
            g_critical ("oops");
        }
        else
        {
            if (!mgr->active_scheme || !strcmp (*ids, SCHEME_DEFAULT))
                mgr->active_scheme = scheme;
            g_hash_table_insert (mgr->schemes, g_strdup (*ids), g_object_ref (scheme));
        }

        ids++;
    }
}


GSList *
moo_lang_mgr_get_sections (MooLangMgr *mgr)
{
    GSList *sections = NULL;
    const char * const *ids = NULL;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_langs (mgr);

    ids = gtk_source_language_manager_get_language_ids (mgr->lang_mgr);

    while (ids && *ids)
    {
        GtkSourceLanguage *lang;
        const char *section;
        const char *id;

        id = *ids;
        lang = gtk_source_language_manager_get_language (mgr->lang_mgr, id);
        section = _moo_lang_get_section (MOO_LANG (lang));

        if (section && !g_slist_find_custom (sections, section, (GCompareFunc) strcmp))
            sections = g_slist_prepend (sections, g_strdup (section));

        ids++;
    }

    return sections;
}


GSList *
moo_lang_mgr_get_available_langs (MooLangMgr *mgr)
{
    GSList *list = NULL;
    const char *const *ids = NULL;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_langs (mgr);

    ids = gtk_source_language_manager_get_language_ids (mgr->lang_mgr);

    while (ids && *ids)
    {
        GtkSourceLanguage *lang;
        lang = gtk_source_language_manager_get_language (mgr->lang_mgr, *ids);
        if (lang)
            list = g_slist_prepend (list, g_object_ref (lang));
        ids++;
    }

    return g_slist_reverse (list);
}


GSList *
_moo_lang_mgr_get_globs (MooLangMgr *mgr,
                         const char *lang_id)
{
    char *id;
    LangInfo *info;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_langs (mgr);

    id = _moo_lang_id_from_name (lang_id);
    info = get_lang_info (mgr, id, FALSE);
    g_return_val_if_fail (info != NULL, NULL);

    g_free (id);
    return string_list_copy (info->globs);
}


GSList *
_moo_lang_mgr_get_mime_types (MooLangMgr *mgr,
                              const char *lang_id)
{
    char *id;
    LangInfo *info;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_langs (mgr);

    id = _moo_lang_id_from_name (lang_id);
    info = get_lang_info (mgr, id, FALSE);
    g_return_val_if_fail (info != NULL, NULL);

    g_free (id);
    return string_list_copy (info->mime_types);
}


static gboolean
string_list_equal (GSList *list1,
                   GSList *list2)
{
    while (list1 && list2)
    {
        if (strcmp ((const char*) list1->data, (const char*) list2->data) != 0)
            return FALSE;

        list1 = list1->next;
        list2 = list2->next;
    }

    return list1 == list2;
}


static void
set_globs_or_mime_types (MooLangMgr *mgr,
                         const char *lang_id,
                         const char *string,
                         gboolean    globs)
{
    LangInfo *info;
    GSList *new_;
    GSList *old = NULL;
    GSList **ptr;
    char *id = NULL;
    gboolean modified = TRUE;

    read_langs (mgr);

    id = _moo_lang_id_from_name (lang_id);
    info = get_lang_info (mgr, id, FALSE);

    g_return_if_fail (info != NULL);

    new_ = _moo_lang_parse_string_list (string);
    old = globs ? _moo_lang_mgr_get_globs (mgr, id) :
                  _moo_lang_mgr_get_mime_types (mgr, id);

    if (string_list_equal (new_, old))
    {
        g_free (id);
        string_list_free (old);
        string_list_free (new_);
        return;
    }

    string_list_free (old);
    mgr->modified = TRUE;

    if (info->lang)
    {
        GSList *builtin = globs ? _moo_lang_get_globs (info->lang) :
                                  _moo_lang_get_mime_types (info->lang);
        modified = !string_list_equal (builtin, new_);

        if (modified)
        {
            string_list_free (builtin);
        }
        else
        {
            string_list_free (new_);
            new_ = builtin;
        }
    }

    ptr = globs ? &info->globs : &info->mime_types;
    string_list_free (*ptr);
    *ptr = new_;

    if (globs)
        info->globs_modified = modified;
    else
        info->mime_types_modified = modified;
}


void
_moo_lang_mgr_set_globs (MooLangMgr *mgr,
                         const char *lang_id,
                         const char *globs)
{
    g_return_if_fail (MOO_IS_LANG_MGR (mgr));
    set_globs_or_mime_types (mgr, lang_id, globs, TRUE);
}


void
_moo_lang_mgr_set_mime_types (MooLangMgr *mgr,
                              const char *lang_id,
                              const char *mime)
{
    g_return_if_fail (MOO_IS_LANG_MGR (mgr));
    set_globs_or_mime_types (mgr, lang_id, mime, FALSE);
}


const char *
_moo_lang_mgr_get_config (MooLangMgr *mgr,
                          const char *lang_id)
{
    char *id;
    const char *config;

    g_return_val_if_fail (MOO_IS_LANG_MGR (mgr), NULL);

    read_langs (mgr);

    id = _moo_lang_id_from_name (lang_id);
    config = (const char*) g_hash_table_lookup (mgr->config, id);

    g_free (id);
    return config;
}


void
_moo_lang_mgr_set_config (MooLangMgr *mgr,
                          const char *lang_id,
                          const char *config)
{
    char *id;
    char *norm = NULL;
    const char *old;

    g_return_if_fail (MOO_IS_LANG_MGR (mgr));

    read_langs (mgr);

    if (config && config[0])
    {
        norm = g_strstrip (g_strdup (config));

        if (!norm[0])
        {
            g_free (norm);
            norm = NULL;
        }
    }

    id = _moo_lang_id_from_name (lang_id);
    old = (const char*) g_hash_table_lookup (mgr->config, id);

    if (!moo_str_equal (old, norm))
        mgr->modified = TRUE;

    g_hash_table_insert (mgr->config, id, norm);
}


void
_moo_lang_mgr_update_config (MooLangMgr     *mgr,
                             MooEditConfig  *config,
                             const char     *lang_id)
{
    const char *lang_config;

    g_return_if_fail (MOO_IS_LANG_MGR (mgr));
    g_return_if_fail (MOO_IS_EDIT_CONFIG (config));

    read_langs (mgr);

    g_object_freeze_notify (G_OBJECT (config));

    moo_edit_config_unset_by_source (config, MOO_EDIT_CONFIG_SOURCE_LANG);

    lang_config = _moo_lang_mgr_get_config (mgr, lang_id);

    if (lang_config)
        moo_edit_config_parse (config, lang_config,
                               MOO_EDIT_CONFIG_SOURCE_LANG);

    g_object_thaw_notify (G_OBJECT (config));
}


static char *
list_to_string (GSList *list)
{
    GString *string = g_string_new (NULL);

    while (list)
    {
        g_string_append (string, (const char*) list->data);

        if ((list = list->next))
            g_string_append_c (string, ';');
    }

    return g_string_free (string, FALSE);
}

namespace {

struct SaveOneData 
{
    MooLangMgr *mgr;
    MooMarkupNode *xml;
    MooMarkupNode *root;
};

} // anonymous namespace

static void
save_one (const char *lang_id,
          LangInfo   *info,
          gpointer    user_data)
{
    const char *config;
    MooMarkupNode *lang_node;
    SaveOneData *data;

    data = (SaveOneData*) user_data;
    config = (const char*) g_hash_table_lookup (data->mgr->config, lang_id);

    if (!config && !info->globs_modified && !info->mime_types_modified)
        return;

    if (!data->root)
        data->root = moo_markup_create_element (data->xml, ELEMENT_LANG_CONFIG);

    lang_node = moo_markup_create_element (data->root, ELEMENT_LANG);
    moo_markup_set_prop (lang_node, PROP_LANG_ID, lang_id);

    if (info->globs_modified)
    {
        char *string = list_to_string (info->globs);
        moo_markup_create_text_element (lang_node, ELEMENT_EXTENSIONS, string);
        g_free (string);
    }

    if (info->mime_types_modified)
    {
        char *string = list_to_string (info->mime_types);
        moo_markup_create_text_element (lang_node, ELEMENT_EXTENSIONS, string);
        g_free (string);
    }

    if (config)
        moo_markup_create_text_element (lang_node, ELEMENT_CONFIG, config);
}

void
_moo_lang_mgr_save_config (MooLangMgr *mgr)
{
    MooMarkupNode *xml;
    MooMarkupNode *root;
    SaveOneData data;

    g_return_if_fail (MOO_IS_LANG_MGR (mgr));

    if (!mgr->modified)
        return;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    mgr->modified = FALSE;
    root = moo_markup_get_element (xml, ELEMENT_LANG_CONFIG);

    if (root)
        moo_markup_delete_node (root);

    root = NULL;

    data.mgr = mgr;
    data.xml = xml;
    data.root = NULL;
    g_hash_table_foreach (mgr->langs, (GHFunc) save_one, &data);
}
