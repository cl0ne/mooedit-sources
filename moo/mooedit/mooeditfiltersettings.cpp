/*
 *   mooeditfiltersettings.c
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

#include "mooedit/mooeditfiltersettings.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mooeditaction.h"
#include "mooedit/moolang.h"
#include "mooedit/mooeditconfig.h"
#include "mooedit/mooedit.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-debug.h"
#include <mooglib/moo-glib.h>
#include <string.h>

MOO_DEBUG_INIT(filters, FALSE)

#define ELEMENT_FILTER_SETTINGS MOO_EDIT_PREFS_PREFIX "/filter-settings"
#define ELEMENT_SETTING         "setting"
#define PROP_FILTER             "filter"
#define PROP_CONFIG             "config"

typedef enum {
    MOO_EDIT_FILTER_LANGS,
    MOO_EDIT_FILTER_GLOBS,
    MOO_EDIT_FILTER_REGEX,
    MOO_EDIT_FILTER_INVALID
} MooEditFilterType;

struct MooEditFilter {
    MooEditFilterType type;
    union {
        GSList *langs;
        GSList *globs;
        GRegex *regex;
        char *string;
    } u;
};

typedef struct {
    MooEditFilter *filter;
    char *config;
} FilterSetting;

typedef struct {
    GSList *settings;
} FilterSettingsStore;

static FilterSettingsStore *settings_store;


static char             *filter_settings_store_get_setting  (FilterSettingsStore    *store,
                                                             MooEdit                *doc);

static MooEditFilter    *moo_edit_filter_new_langs          (const char             *string,
                                                             GError                **error);
static MooEditFilter    *moo_edit_filter_new_regex          (const char             *string,
                                                             GError                **error);
static MooEditFilter    *moo_edit_filter_new_globs          (const char             *string,
                                                             GError                **error);

static char *
moo_edit_filter_get_string (MooEditFilter     *filter,
                            MooEditFilterType  default_type)
{
    GString *str;
    GSList *l;

    g_return_val_if_fail (filter != NULL, NULL);

    str = g_string_new (NULL);

    if (filter->type != default_type)
    {
        switch (filter->type)
        {
            case MOO_EDIT_FILTER_LANGS:
                g_string_append (str, "langs:");
                break;
            case MOO_EDIT_FILTER_GLOBS:
                g_string_append (str, "globs:");
                break;
            case MOO_EDIT_FILTER_REGEX:
                g_string_append (str, "regex:");
                break;
            case MOO_EDIT_FILTER_INVALID:
                break;
        }
    }

    switch (filter->type)
    {
        case MOO_EDIT_FILTER_LANGS:
        case MOO_EDIT_FILTER_GLOBS:
            for (l = filter->u.langs; l != NULL; l = l->next)
            {
                if (l != filter->u.langs)
                    g_string_append (str, ";");
                g_string_append (str, (const char*) l->data);
            }
            break;
        case MOO_EDIT_FILTER_REGEX:
            g_string_append (str, g_regex_get_pattern (filter->u.regex));
            break;
        case MOO_EDIT_FILTER_INVALID:
            g_string_append (str, filter->u.string);
            break;
    }

    return g_string_free (str, FALSE);
}

MooEditFilter *
_moo_edit_filter_new_full (const char         *string,
                           MooEditFilterKind   kind,
                           GError            **error)
{
    g_return_val_if_fail (string && string[0], NULL);

    if (!strncmp (string, "langs:", strlen ("langs:")))
        return moo_edit_filter_new_langs (string + strlen ("langs:"), error);
    if (!strncmp (string, "globs:", strlen ("globs:")))
        return moo_edit_filter_new_globs (string + strlen ("globs:"), error);
    if (!strncmp (string, "regex:", strlen ("regex:")))
        return moo_edit_filter_new_regex (string + strlen ("regex:"), error);

    switch (kind)
    {
        case MOO_EDIT_FILTER_CONFIG:
            return moo_edit_filter_new_regex (string, error);
        case MOO_EDIT_FILTER_ACTION:
            return moo_edit_filter_new_globs (string, error);
    }

    g_return_val_if_reached (NULL);
}

MooEditFilter *
_moo_edit_filter_new (const char        *string,
                      MooEditFilterKind  kind)
{
    GError *error = NULL;
    MooEditFilter *filter;

    g_return_val_if_fail (string != NULL, NULL);

    filter = _moo_edit_filter_new_full (string, kind, &error);

    if (error)
    {
        g_warning ("could not parse filter '%s': %s", string, error->message);
        g_error_free (error);
    }

    return filter;
}

static GSList *
_moo_edit_parse_langs (const char *string)
{
    char **pieces, **p;
    GSList *list = NULL;

    if (!string)
        return NULL;

    pieces = g_strsplit_set (string, " \t\r\n;,", 0);

    if (!pieces)
        return NULL;

    for (p = pieces; *p != NULL; ++p)
    {
        g_strstrip (*p);

        if (**p)
            list = g_slist_prepend (list, _moo_lang_id_from_name (*p));
    }

    g_strfreev (pieces);
    return g_slist_reverse (list);
}

static MooEditFilter *
moo_edit_filter_new_langs (const char  *string,
                           GError     **error)
{
    MooEditFilter *filt;

    moo_return_error_if_fail_p (string != NULL);

    filt = g_new0 (MooEditFilter, 1);
    filt->type = MOO_EDIT_FILTER_LANGS;
    filt->u.langs = _moo_edit_parse_langs (string);

    return filt;
}

static MooEditFilter *
moo_edit_regex_new_invalid (const char  *string,
                            GError     **error)
{
    MooEditFilter *filt;

    moo_return_error_if_fail_p (string != NULL);

    filt = g_new0 (MooEditFilter, 1);
    filt->type = MOO_EDIT_FILTER_INVALID;
    filt->u.string = g_strdup (string);

    return filt;
}

static MooEditFilter *
moo_edit_filter_new_regex (const char  *string,
                           GError     **error)
{
    MooEditFilter *filt;
    GRegex *regex;

    moo_return_error_if_fail_p (string != NULL);

    regex = g_regex_new (string, G_REGEX_OPTIMIZE
#ifdef __WIN32__
                                | G_REGEX_CASELESS
#endif
                         , GRegexMatchFlags (0), error);

    if (!regex)
        return moo_edit_regex_new_invalid (string, NULL);

    filt = g_new0 (MooEditFilter, 1);
    filt->type = MOO_EDIT_FILTER_REGEX;
    filt->u.regex = regex;

    return filt;
}

static GSList *
parse_globs (const char *string)
{
    char **pieces, **p;
    GSList *list = NULL;

    if (!string)
        return NULL;

    pieces = g_strsplit_set (string, " \t\r\n;,", 0);

    if (!pieces)
        return NULL;

    for (p = pieces; *p != NULL; ++p)
    {
        g_strstrip (*p);

        if (**p)
            list = g_slist_prepend (list, g_strdup (*p));
    }

    g_strfreev (pieces);
    return g_slist_reverse (list);
}

static MooEditFilter *
moo_edit_filter_new_globs (const char  *string,
                           GError     **error)
{
    MooEditFilter *filt;

    moo_return_error_if_fail_p (string != NULL);

    filt = g_new0 (MooEditFilter, 1);
    filt->type = MOO_EDIT_FILTER_GLOBS;
    filt->u.globs = parse_globs (string);

    return filt;
}

gboolean
_moo_edit_filter_valid (MooEditFilter *filter)
{
    g_return_val_if_fail (filter != NULL, FALSE);
    return filter->type != MOO_EDIT_FILTER_INVALID;
}

void
_moo_edit_filter_free (MooEditFilter *filter)
{
    if (filter)
    {
        switch (filter->type)
        {
            case MOO_EDIT_FILTER_GLOBS:
            case MOO_EDIT_FILTER_LANGS:
                g_slist_foreach (filter->u.langs, (GFunc) g_free, NULL);
                g_slist_free (filter->u.langs);
                break;
            case MOO_EDIT_FILTER_REGEX:
                if (filter->u.regex)
                    g_regex_unref (filter->u.regex);
                break;
            case MOO_EDIT_FILTER_INVALID:
                g_free (filter->u.string);
                break;
        }

        g_free (filter);
    }
}


static gboolean
moo_edit_filter_check_globs (GSList  *globs,
                             MooEdit *doc)
{
    GFile *file = moo_edit_get_file (doc);

    while (globs)
    {
        if (!strcmp ((const char*) globs->data, "*") ||
            (file && moo_file_fnmatch (file, (const char*) globs->data)))
        {
            moo_file_free (file);
            return TRUE;
        }

        globs = globs->next;
    }

    moo_file_free (file);
    return FALSE;
}

static gboolean
moo_edit_filter_check_langs (GSList  *langs,
                             MooEdit *doc)
{
    char *lang_id = moo_edit_get_lang_id (doc);

    while (lang_id && langs)
    {
        if (strcmp ((const char*) langs->data, lang_id) == 0)
        {
            g_free (lang_id);
            return TRUE;
        }

        langs = langs->next;
    }

    g_free (lang_id);
    return FALSE;
}

static gboolean
moo_edit_filter_check_regex (GRegex  *regex,
                             MooEdit *doc)
{
    const char *name = moo_edit_get_display_name (doc);
    g_return_val_if_fail (name != NULL, FALSE);
    return g_regex_match (regex, name, GRegexMatchFlags (0), NULL);
}

gboolean
_moo_edit_filter_match (MooEditFilter *filter,
                        MooEdit       *doc)
{
    g_return_val_if_fail (filter != NULL, FALSE);
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);

    switch (filter->type)
    {
        case MOO_EDIT_FILTER_GLOBS:
            return moo_edit_filter_check_globs (filter->u.globs, doc);
        case MOO_EDIT_FILTER_LANGS:
            return moo_edit_filter_check_langs (filter->u.langs, doc);
        case MOO_EDIT_FILTER_REGEX:
            return moo_edit_filter_check_regex (filter->u.regex, doc);
        case MOO_EDIT_FILTER_INVALID:
            return FALSE;
    }

    g_return_val_if_reached (FALSE);
}


static void
filter_setting_free (FilterSetting *setting)
{
    if (setting)
    {
        _moo_edit_filter_free (setting->filter);
        g_free (setting->config);
        g_free (setting);
    }
}


static FilterSetting *
filter_setting_new (const char *filter,
                    const char *config)
{
    FilterSetting *setting;

    g_return_val_if_fail (filter != NULL, NULL);
    g_return_val_if_fail (config != NULL, NULL);

    setting = g_new0 (FilterSetting, 1);
    setting->filter = _moo_edit_filter_new (filter, MOO_EDIT_FILTER_CONFIG);
    setting->config = g_strdup (config);

    if (!setting->filter)
    {
        g_warning ("could not parse filter '%s'", filter);
        filter_setting_free (setting);
        setting = NULL;
    }

    return setting;
}


static FilterSettingsStore *
filter_settings_store_new (void)
{
    FilterSettingsStore *store;

    store = g_new0 (FilterSettingsStore, 1);

    return store;
}


static void
filter_settings_store_free (FilterSettingsStore *store)
{
    g_slist_foreach (store->settings, (GFunc) filter_setting_free, NULL);
    g_slist_free (store->settings);
    g_free (store);
}


static void
load_node (FilterSettingsStore *store,
           MooMarkupNode       *node)
{
    const char *filter, *config;
    FilterSetting *setting;

    filter = moo_markup_get_prop (node, PROP_FILTER);
    config = moo_markup_get_prop (node, PROP_CONFIG);
    g_return_if_fail (filter != NULL && config != NULL);

    setting = filter_setting_new (filter, config);
    g_return_if_fail (setting != NULL);

    store->settings = g_slist_prepend (store->settings, setting);
}


static void
filter_settings_store_load (FilterSettingsStore *store)
{
    MooMarkupNode *xml;
    MooMarkupNode *root, *node;

    g_return_if_fail (!store->settings);

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    root = moo_markup_get_element (xml, ELEMENT_FILTER_SETTINGS);

    if (!root)
        return;

    for (node = root->children; node != NULL; node = node->next)
    {
        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        if (strcmp (node->name, ELEMENT_SETTING))
        {
            g_warning ("invalid '%s' element", node->name);
            continue;
        }

        load_node (store, node);
    }

    store->settings = g_slist_reverse (store->settings);
}


void
_moo_edit_filter_settings_load (void)
{
    if (settings_store)
        return;

    settings_store = filter_settings_store_new ();
    filter_settings_store_load (settings_store);
}


static void
_moo_edit_filter_settings_reload (void)
{
    if (settings_store)
        filter_settings_store_free (settings_store);
    settings_store = NULL;
    _moo_edit_filter_settings_load ();
}


char *
_moo_edit_filter_settings_get_for_doc (MooEdit *doc)
{
    g_return_val_if_fail (settings_store != NULL, NULL);
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return filter_settings_store_get_setting (settings_store, doc);
}


static char *
filter_settings_store_get_setting (FilterSettingsStore *store,
                                   MooEdit             *doc)
{
    GSList *l;
    GString *result = NULL;

    for (l = store->settings; l != NULL; l = l->next)
    {
        FilterSetting *setting = (FilterSetting*) l->data;
        if (_moo_edit_filter_match (setting->filter, doc))
        {
            if (!result)
                result = g_string_new (setting->config);
            else
                g_string_append_printf (result, ";%s", setting->config);
        }
    }

    return result ? g_string_free (result, FALSE) : NULL;
}


void
_moo_edit_filter_settings_set_strings (GSList *list)
{
    MooMarkupNode *xml;
    MooMarkupNode *root;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    root = moo_markup_get_element (xml, ELEMENT_FILTER_SETTINGS);

    if (root)
        moo_markup_delete_node (root);

    if (!list)
    {
        _moo_edit_filter_settings_reload ();
        return;
    }

    root = moo_markup_create_element (xml, ELEMENT_FILTER_SETTINGS);

    while (list)
    {
        MooMarkupNode *node;
        const char *filter, *config;

        g_return_if_fail (list->data && list->next && list->next->data);

        filter = (const char*) list->data;
        config = (const char*) list->next->data;

        node = moo_markup_create_element (root, ELEMENT_SETTING);
        moo_markup_set_prop (node, PROP_FILTER, filter);
        moo_markup_set_prop (node, PROP_CONFIG, config);

        list = list->next->next;
    }

    _moo_edit_filter_settings_reload ();
}


GSList *
_moo_edit_filter_settings_get_strings (void)
{
    GSList *strings = NULL, *l;

    g_return_val_if_fail (settings_store != NULL, NULL);

    for (l = settings_store->settings; l != NULL; l = l->next)
    {
        FilterSetting *setting = (FilterSetting*) l->data;
        strings = g_slist_prepend (strings, moo_edit_filter_get_string (setting->filter, MOO_EDIT_FILTER_REGEX));
        strings = g_slist_prepend (strings, g_strdup (setting->config));
    }

    return g_slist_reverse (strings);
}
