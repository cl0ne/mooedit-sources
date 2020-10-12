/*
 *   moofileview-aux.h
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

#include "moofileview/moofile-private.h"
#include <string.h>
#include <mooglib/moo-glib.h>

#ifndef MOO_FILE_VIEW_AUX_H
#define MOO_FILE_VIEW_AUX_H


#define COLUMN_FILE MOO_FOLDER_MODEL_COLUMN_FILE


/* TODO: strncmp should accept char len, not byte len? */
typedef struct TextFuncs {
    gboolean    (*file_equals)          (MooFile      *file,
                                         const char   *str);
    gboolean    (*file_has_prefix)      (MooFile      *file,
                                         const char   *str,
                                         gsize         len);
    char*       (*normalize)            (const char   *str,
                                         gssize        len);
} TextFuncs;


static gboolean
file_equals (MooFile    *file,
             const char *str)
{
    return !strcmp (str, _moo_file_display_name (file));
}

static gboolean
file_has_prefix (MooFile    *file,
                 const char *str,
                 gsize       len)
{
    return !strncmp (str, _moo_file_display_name (file), len);
}

static char *
normalize (const char *str,
           gssize      len)
{
    return g_utf8_normalize (str, len, G_NORMALIZE_ALL);
}


static char *
case_normalize (const char *str,
                gssize      len)
{
    char *norm = g_utf8_normalize (str, len, G_NORMALIZE_ALL);
    char *res = g_utf8_casefold (norm, -1);
    g_free (norm);
    return res;
}

static gboolean
case_file_equals (MooFile    *file,
                  const char *str)
{
    char *temp = case_normalize (str, -1);
    gboolean ret = !strcmp (temp, _moo_file_case_display_name (file));
    g_free (temp);
    return ret;
}

static gboolean
case_file_has_prefix (MooFile    *file,
                      const char *str,
                      gsize       len)
{
    char *temp = case_normalize (str, len);
    gboolean ret = g_str_has_prefix (_moo_file_case_display_name (file), temp);
    g_free (temp);
    return ret;
}


static void
set_text_funcs (TextFuncs *funcs,
                gboolean   case_sensitive)
{
    if (case_sensitive)
    {
        funcs->file_equals = file_equals;
        funcs->file_has_prefix = file_has_prefix;
        funcs->normalize = normalize;
    }
    else
    {
        funcs->file_equals = case_file_equals;
        funcs->file_has_prefix = case_file_has_prefix;
        funcs->normalize = case_normalize;
    }
}


static gboolean
model_find_next_match (GtkTreeModel   *model,
                       GtkTreeIter    *iter,
                       const char     *text,
                       gssize          len,
                       TextFuncs      *funcs,
                       gboolean        exact_match)
{
    char *normalized_text;
    gsize normalized_text_len;

    g_return_val_if_fail (text != NULL, FALSE);

    normalized_text = funcs->normalize (text, len);
    normalized_text_len = strlen (normalized_text);

    while (TRUE)
    {
        MooFile *file = NULL;
        gboolean match;

        gtk_tree_model_get (model, iter, COLUMN_FILE, &file, -1);

        if (file)
        {
            if (exact_match)
                match = funcs->file_equals (file, normalized_text);
            else
                match = funcs->file_has_prefix (file,
                                                normalized_text,
                                                normalized_text_len);
            _moo_file_unref (file);

            if (match)
            {
                g_free (normalized_text);
                return TRUE;
            }
        }

        if (!gtk_tree_model_iter_next (model, iter))
        {
            g_free (normalized_text);
            return FALSE;
        }
    }
}


static GString *
model_find_max_prefix (GtkTreeModel   *model,
                       const char     *text,
                       TextFuncs      *funcs,
                       gboolean       *unique_p,
                       GtkTreeIter    *unique_iter_p)
{
    GtkTreeIter iter;
    gsize text_len;
    GString *prefix = NULL;
    gboolean unique = FALSE;

    g_assert (text && text[0]);

    text_len = strlen (text);

    if (!gtk_tree_model_get_iter_first (model, &iter))
        goto out;

    while (TRUE)
    {
        MooFile *file = NULL;
        const char *name;
        gsize i;

        if (!model_find_next_match (model, &iter, text, text_len, funcs, FALSE))
            goto out;

        gtk_tree_model_get (model, &iter,
                            COLUMN_FILE, &file, -1);
        g_assert (file != NULL);

        name = _moo_file_display_name (file);

        if (!prefix)
        {
            prefix = g_string_new (_moo_file_display_name (file));
            unique = TRUE;

            if (unique_iter_p)
                *unique_iter_p = iter;

            /* nothing to look for, just check if it's really unique */
            if (prefix->len == text_len)
            {
                if (gtk_tree_model_iter_next (model, &iter) &&
                    model_find_next_match (model, &iter, text, text_len, funcs, FALSE))
                    unique = FALSE;

                goto out;
            }
        }
        else
        {
            for (i = text_len; i < prefix->len && name[i] == prefix->str[i]; ++i) ;

            prefix->str[i] = 0;
            prefix->len = i;
            unique = FALSE;

            if (prefix->len == text_len)
                goto out;
        }

        if (!gtk_tree_model_iter_next (model, &iter))
            goto out;
    }

out:
    if (unique_p)
        *unique_p = unique;
    return prefix;
}


#endif /* MOO_FILE_VIEW_AUX_H */
