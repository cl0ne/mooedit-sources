/*
 *   moolang.c
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
#include "mooedit/moolang-private.h"
#include "mooutils/mooi18n.h"
#include <mooglib/moo-glib.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


GType
moo_lang_get_type (void)
{
    static GType type;

    if (G_UNLIKELY (!type))
        type = GTK_TYPE_SOURCE_LANGUAGE;

    return type;
}


GtkSourceEngine *
_moo_lang_get_engine (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    return _gtk_source_language_create_engine (GTK_SOURCE_LANGUAGE (lang));
}


const char *
_moo_lang_id (MooLang *lang)
{
    g_return_val_if_fail (!lang || MOO_IS_LANG (lang), NULL);

    if (lang)
        return GTK_SOURCE_LANGUAGE(lang)->priv->id;
    else
        return MOO_LANG_NONE;
}


const char *
_moo_lang_display_name (MooLang *lang)
{
    g_return_val_if_fail (!lang || MOO_IS_LANG (lang), NULL);

    if (lang)
        return GTK_SOURCE_LANGUAGE(lang)->priv->name;
    else
        return _(MOO_LANG_NONE_NAME);
}


const char *
_moo_lang_get_line_comment (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    return (const char*) g_hash_table_lookup (GTK_SOURCE_LANGUAGE(lang)->priv->properties, "line-comment-start");
}


const char *
_moo_lang_get_block_comment_start (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    return (const char*) g_hash_table_lookup (GTK_SOURCE_LANGUAGE(lang)->priv->properties, "block-comment-start");
}


const char *
_moo_lang_get_block_comment_end (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    return (const char*) g_hash_table_lookup (GTK_SOURCE_LANGUAGE(lang)->priv->properties, "block-comment-end");
}


const char *
_moo_lang_get_brackets (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    return NULL;
}


const char *
_moo_lang_get_section (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    return GTK_SOURCE_LANGUAGE(lang)->priv->section;
}


gboolean
_moo_lang_get_hidden (MooLang *lang)
{
    g_return_val_if_fail (MOO_IS_LANG (lang), TRUE);
    return GTK_SOURCE_LANGUAGE(lang)->priv->hidden != 0;
}


char *
_moo_lang_id_from_name (const char *whatever)
{
    if (!whatever || !g_ascii_strcasecmp (whatever, MOO_LANG_NONE))
        return g_strdup (MOO_LANG_NONE);
    else
        return g_strstrip (g_ascii_strdown (whatever, -1));
}


GSList *
_moo_lang_get_globs (MooLang *lang)
{
    const char *prop;
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    prop = gtk_source_language_get_metadata (GTK_SOURCE_LANGUAGE (lang), "globs");
    return _moo_lang_parse_string_list (prop);
}


GSList *
_moo_lang_get_mime_types (MooLang *lang)
{
    const char *prop;
    g_return_val_if_fail (MOO_IS_LANG (lang), NULL);
    prop = gtk_source_language_get_metadata (GTK_SOURCE_LANGUAGE (lang), "mimetypes");
    return _moo_lang_parse_string_list (prop);
}


GSList *
_moo_lang_parse_string_list (const char *string)
{
    char *copy;
    GSList *list = NULL;
    char **pieces, **p;

    if (!string || !string[0])
        return NULL;

    copy = g_strstrip (g_strdup (string));

    pieces = g_strsplit_set (copy, ",;", 0);
    g_return_val_if_fail (pieces != NULL, NULL);

    for (p = pieces; *p; p++)
        if (**p)
            list = g_slist_prepend (list, g_strdup (*p));

    g_strfreev (pieces);
    g_free (copy);

    return g_slist_reverse (list);
}
