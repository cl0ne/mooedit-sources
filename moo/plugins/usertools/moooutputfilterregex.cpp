/*
 *   moooutputfilterregex.c
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

#include "moooutputfilterregex.h"
#include "moocommand.h"
#include "../support/moocmdview.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/moolangmgr.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moomarkup.h"
#include "mooutils/mooi18n.h"
#include <mooglib/moo-glib.h>
#include <string.h>

#define FILTERS_VERSION     "1.0"

#define FILTERS_FILE        "filters.xml"
#define ELEMENT_ROOT        "medit-filters"
#define ELEMENT_FILTER      "filter"
#define ELEMENT_MATCH       "match"
#define ELEMENT_ACTION      "action"
#define PROP_VERSION        "version"
#define PROP_FILTER_ID      "id"
#define PROP_FILTER_NAME    "name"
#define PROP_FILTER__NAME   "_name"
#define PROP_OUTPUT_TYPE    "what"
#define PROP_SPAN           "span"
#define PROP_PATTERN        "pattern"
#define PROP_STYLE          "style"
#define PROP_ACTION_TYPE    "type"
#define PROP_ACTION_TARGET  "name"
#define PROP_SUBSTRING      "substring"


typedef enum {
    OUTPUT_ALL,
    OUTPUT_STDOUT,
    OUTPUT_STDERR
} OutputType;

typedef enum {
    ACTION_POP,
    ACTION_PUSH
} ActionType;

typedef enum {
    ACTION_FILE,
    ACTION_DIR
} ActionTarget;

typedef struct FilterStore FilterStore;
typedef struct PatternInfo PatternInfo;
typedef struct ActionInfo ActionInfo;
typedef struct FilterState FilterState;
typedef struct FilterInfo FilterInfo;

struct FilterState {
    guint ref_count;
    PatternInfo **patterns;
    guint n_patterns;
    GRegex *re_out;
    GRegex *re_err;
};

struct PatternInfo {
    OutputType type;
    GRegex *re;
    GMatchInfo *mi;
    char *style;
    GSList *actions;
    guint span;
};

struct ActionInfo {
    ActionType type;
    ActionTarget target;
    char *data;
};

struct FilterInfo {
    guint ref_count;
    char *id;
    char *name;
    FilterState *state;
};

struct _MooOutputFilterRegexPrivate {
    FilterInfo *filter;
    FilterState *state;
    GSList *file_stack;
    GSList *dir_stack;

    guint span;
    char *style;
    MooFileLineData *line;
};


static FilterInfo  *filter_info_new     (const char     *id,
                                         const char     *name,
                                         GSList         *patterns);
static FilterInfo  *filter_info_ref     (FilterInfo     *filter);
static void         filter_info_unref   (FilterInfo     *filter);

static PatternInfo *pattern_info_new    (OutputType      type,
                                         const char     *pattern,
                                         const char     *style,
                                         GSList         *actions,
                                         guint           span);
static void         pattern_info_free   (PatternInfo    *pattern);

static ActionInfo  *action_info_new     (ActionType      type,
                                         ActionTarget    target,
                                         const char     *substring);
static void         action_info_free    (ActionInfo     *action);


G_DEFINE_TYPE (MooOutputFilterRegex, _moo_output_filter_regex, MOO_TYPE_OUTPUT_FILTER)


static void
moo_output_filter_regex_dispose (GObject *object)
{
    MooOutputFilterRegex *filter = MOO_OUTPUT_FILTER_REGEX (object);

    if (filter->priv->filter)
    {
        filter_info_unref (filter->priv->filter);
        filter->priv->filter = NULL;
        filter->priv->state = NULL;
    }

    g_free (filter->priv->style);
    filter->priv->style = NULL;
    moo_file_line_data_free (filter->priv->line);
    filter->priv->line = NULL;

    g_slist_foreach (filter->priv->file_stack, (GFunc) g_free, NULL);
    g_slist_free (filter->priv->file_stack);
    filter->priv->file_stack = NULL;
    g_slist_foreach (filter->priv->dir_stack, (GFunc) g_free, NULL);
    g_slist_free (filter->priv->dir_stack);
    filter->priv->dir_stack = NULL;

    G_OBJECT_CLASS (_moo_output_filter_regex_parent_class)->dispose (object);
}


static void
moo_output_filter_regex_attach (MooOutputFilter *base)
{
    MooOutputFilterRegex *filter = MOO_OUTPUT_FILTER_REGEX (base);
    g_return_if_fail (filter->priv->state != NULL);
}


static void
moo_output_filter_regex_detach (MooOutputFilter *base)
{
    MooOutputFilterRegex *filter = MOO_OUTPUT_FILTER_REGEX (base);
    g_return_if_fail (filter->priv->state != NULL);
}


static char *
find_file_in_dir (const char *file,
                  const char *dir)
{
    char *path;

    if (!file || !dir)
        return NULL;

    path = g_build_filename (dir, file, nullptr);

    if (g_file_test (path, G_FILE_TEST_EXISTS))
        return path;

    g_free (path);
    return NULL;
}

static char *
find_file_in_dirs (const char         *file,
                   const char * const *dirs)
{
    for ( ; file && dirs && *dirs; ++dirs)
    {
        char *path = find_file_in_dir (file, *dirs);
        if (path)
            return path;
    }

    return NULL;
}

static char *
find_file (const char           *file,
           MooOutputFilterRegex *filter)
{
    char *real_file = NULL;

    if (!file || g_path_is_absolute (file))
        real_file = g_strdup (file);

    if (!real_file && filter->priv->dir_stack)
        real_file = find_file_in_dir (file, (const char*) filter->priv->dir_stack->data);

    if (!real_file)
        real_file = find_file_in_dirs (file, moo_output_filter_get_active_dirs (MOO_OUTPUT_FILTER (filter)));

    if (!real_file)
        real_file = g_strdup (file);

    return real_file;
}


static MooFileLineData *
parse_file_line (MooOutputFilterRegex *filter,
                 const char           *file,
                 const char           *line,
                 const char           *character)
{
    MooFileLineData *data;
    char *freeme = NULL;

    file = file && *file ? file : NULL;
    line = line && *line ? line : NULL;

    if (!file && !line)
        return NULL;

    if (!file && filter->priv->file_stack)
        file = (const char*) filter->priv->file_stack->data;

    if (!file)
        file = moo_output_filter_get_active_file (MOO_OUTPUT_FILTER (filter));

    freeme = find_file (file, filter);
    file = freeme;

    if (!file)
        return NULL;

    data = moo_file_line_data_new (file, -1, -1);
    data->line = _moo_convert_string_to_int (line, 0) - 1;
    data->character = _moo_convert_string_to_int (character, 0) - 1;

    g_free (freeme);
    return data;
}


static MooFileLineData *
process_location (MooOutputFilterRegex *filter,
                  PatternInfo          *pattern,
                  const char           *text,
                  MooLineView          *view,
                  int                   line_no)
{
    char *file, *line, *character;
    MooFileLineData *data = NULL;

    file = g_match_info_fetch_named (pattern->mi, "file");
    line = g_match_info_fetch_named (pattern->mi, "line");
    character = g_match_info_fetch_named (pattern->mi, "character");

    if (file || line)
    {
        data = parse_file_line (filter, file, line, character);

        if (data)
            moo_line_view_set_boxed (view, line_no, MOO_TYPE_FILE_LINE_DATA, data);
        else
            g_message ("could not parse '%s', '%s' in '%s'",
                       file ? file : "<null>",
                       line ? line : "<null>",
                       text);
    }

    g_free (file);
    g_free (line);
    g_free (character);

    return data;
}


static gboolean
find_match (const char   *text,
            int           pos,
            FilterState  *state,
            OutputType    type,
            PatternInfo **pattern_p,
            int          *start,
            int          *end)
{
    guint i;
    GRegex *re_all;
    GMatchInfo *match_info = NULL;

    if (type == OUTPUT_STDOUT)
        re_all = state->re_out;
    else
        re_all = state->re_err;

    if (!re_all)
        return FALSE;

    if (!g_regex_match_full (re_all, text, -1, pos, (GRegexMatchFlags) 0, &match_info, NULL))
    {
        g_match_info_free (match_info);
        return FALSE;
    }

    g_match_info_fetch_pos (match_info, 0, start, end);
    g_match_info_free (match_info);

    for (i = 0; i < state->n_patterns; ++i)
    {
        int p_start, p_end;
        PatternInfo *pattern = state->patterns[i];

        if (pattern->type != type && pattern->type != OUTPUT_ALL)
            continue;

        if (pattern->mi)
        {
            g_match_info_free (pattern->mi);
            pattern->mi = NULL;
        }

        if (!g_regex_match_full (pattern->re, text, -1, *start,
                                 G_REGEX_MATCH_ANCHORED,
                                 &pattern->mi, NULL))
            continue;

        g_match_info_fetch_pos (pattern->mi, 0, &p_start, &p_end);

        if (p_start == pos && p_end == pos)
        {
            g_warning ("empty match");
            continue;
        }

        *pattern_p = pattern;
        *start = p_start;
        *end = p_end;
        return TRUE;
    }

    return FALSE;
}


static GtkTextTag *
get_tag (MooLineView *view,
         OutputType   type,
         const char  *name)
{
    GtkTextTag *tag;

    if (!name)
    {
        if (type == OUTPUT_STDERR)
            return get_tag (view, type, "output-stderr");
        else
            return get_tag (view, type, "output-stdout");
    }

    tag = moo_line_view_lookup_tag (view, name);

    if (!tag)
    {
        MooLangMgr *lang_mgr;
        MooTextStyleScheme *scheme;
        MooTextStyle *style = NULL;

        tag = moo_line_view_create_tag (view, name, NULL);
        lang_mgr = moo_lang_mgr_default ();
        scheme = moo_lang_mgr_get_active_scheme (lang_mgr);

        if (scheme)
            style = _moo_text_style_scheme_lookup_style (scheme, name);

        if (style)
            _moo_text_style_apply_to_tag (style, tag);
        else if (type == OUTPUT_STDERR || !strcmp (name, "output-error") ||
                 !strcmp (name, "output-stderr"))
            g_object_set (tag, "foreground", "red", nullptr);
    }

    return tag;
}


static void
process_action (MooOutputFilterRegex *filter,
                PatternInfo          *pattern,
                ActionInfo           *action,
                const char           *text)
{
    GSList **list = NULL;
    char *data;

    switch (action->target)
    {
        case ACTION_FILE:
            list = &filter->priv->file_stack;
            break;
        case ACTION_DIR:
            list = &filter->priv->dir_stack;
            break;
    }

    g_return_if_fail (list != NULL);

    switch (action->type)
    {
        case ACTION_PUSH:
            data = NULL;
            if (action->data)
                data = g_match_info_fetch_named (pattern->mi, action->data);
            if (!data)
            {
                if (*list)
                    data = g_strdup ((const char*) (*list)->data);
                else
                    data = g_strdup (moo_output_filter_get_active_file (MOO_OUTPUT_FILTER (filter)));
            }
            *list = g_slist_prepend (*list, data);
            break;

        case ACTION_POP:
            if (!*list)
            {
                g_critical ("error in %s", text);
            }
            else
            {
                g_free ((*list)->data);
                *list = g_slist_delete_link (*list, *list);
            }
            break;
    }
}


static gboolean
process_line (MooOutputFilterRegex  *filter,
              const char            *text,
              FilterState           *state,
              OutputType             type)
{
    gssize start_pos;
    int match_start, match_end;
    int line_no;
    PatternInfo *pattern;
    gboolean found;
    MooLineView *view;

    view = moo_output_filter_get_view (MOO_OUTPUT_FILTER (filter));

    if (filter->priv->span)
    {
        line_no = moo_line_view_write_line (view, text, -1, get_tag (view, type, filter->priv->style));

        if (filter->priv->line)
            moo_line_view_set_boxed (view, line_no, MOO_TYPE_FILE_LINE_DATA, filter->priv->line);

        if (!--filter->priv->span)
        {
            g_free (filter->priv->style);
            filter->priv->style = NULL;
            moo_file_line_data_free (filter->priv->line);
            filter->priv->line = NULL;
        }

        return TRUE;
    }

    start_pos = 0;
    found = FALSE;
    line_no = 0;

    while (text[start_pos] && find_match (text, start_pos, state, type, &pattern, &match_start, &match_end))
    {
        GSList *l;
        MooFileLineData *line_data;

        if (!found)
            line_no = moo_line_view_start_line (view);

        found = TRUE;

        if (match_start > start_pos)
            moo_line_view_write (view, text + start_pos,
                                 match_start - start_pos,
                                 get_tag (view, type, NULL));

        if (match_end > match_start)
            moo_line_view_write (view, text + match_start,
                                 match_end - match_start,
                                 get_tag (view, type, pattern->style));

        line_data = process_location (filter, pattern, text, view, line_no);

        for (l = pattern->actions; l != NULL; l = l->next)
            process_action (filter, pattern, (ActionInfo*) l->data, text);

        if (pattern->span)
        {
            if (pattern->span > 1)
            {
                filter->priv->span = pattern->span - 1;
                filter->priv->style = g_strdup (pattern->style);
                filter->priv->line = line_data;
                line_data = NULL;
            }

            moo_line_view_write (view, text + match_end, -1,
                                 get_tag (view, type, pattern->style));
            start_pos = strlen (text);

        }
        else
        {
            start_pos = match_end;
        }

        moo_file_line_data_free (line_data);
    }

    if (found)
    {
        if (text[start_pos])
            moo_line_view_write (view, text + start_pos, -1,
                                 get_tag (view, type, NULL));
        moo_line_view_end_line (view);
    }

    return found;
}


static gboolean
moo_output_filter_regex_stdout_line (MooOutputFilter *base,
                                      const char      *line)
{
    MooOutputFilterRegex *filter = MOO_OUTPUT_FILTER_REGEX (base);
    g_return_val_if_fail (filter->priv->state != NULL, FALSE);
    return process_line (filter, line, filter->priv->state, OUTPUT_STDOUT);
}


static gboolean
moo_output_filter_regex_stderr_line (MooOutputFilter *base,
                                      const char      *line)
{
    MooOutputFilterRegex *filter = MOO_OUTPUT_FILTER_REGEX (base);
    g_return_val_if_fail (filter->priv->state != NULL, FALSE);
    return process_line (filter, line, filter->priv->state, OUTPUT_STDERR);
}


static void
_moo_output_filter_regex_class_init (MooOutputFilterRegexClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    MooOutputFilterClass *filter_class = MOO_OUTPUT_FILTER_CLASS (klass);

    object_class->dispose = moo_output_filter_regex_dispose;

    filter_class->attach = moo_output_filter_regex_attach;
    filter_class->detach = moo_output_filter_regex_detach;
    filter_class->stdout_line = moo_output_filter_regex_stdout_line;
    filter_class->stderr_line = moo_output_filter_regex_stderr_line;

    g_type_class_add_private (klass, sizeof (MooOutputFilterRegexPrivate));
}


static void
_moo_output_filter_regex_init (MooOutputFilterRegex *filter)
{
    filter->priv = G_TYPE_INSTANCE_GET_PRIVATE (filter, MOO_TYPE_OUTPUT_FILTER_REGEX, MooOutputFilterRegexPrivate);
}


static MooOutputFilter *
filter_factory_func (const char *id,
                     gpointer    data)
{
    FilterInfo *info;
    MooOutputFilterRegex *filter;

    info = (FilterInfo*) data;

    g_return_val_if_fail (id != NULL, NULL);
    g_return_val_if_fail (!strcmp (info->id, id), NULL);

    filter = (MooOutputFilterRegex*) g_object_new (MOO_TYPE_OUTPUT_FILTER_REGEX, (const char*) NULL);
    filter->priv->filter = filter_info_ref (info);
    filter->priv->state = info->state;

    return MOO_OUTPUT_FILTER (filter);
}


static ActionInfo *
action_info_new (ActionType      type,
                 ActionTarget    target,
                 const char     *substring)
{
    ActionInfo *info = g_new0 (ActionInfo, 1);
    info->type = type;
    info->target = target;
    info->data = g_strdup (substring);
    return info;
}

static void
action_info_free (ActionInfo *action)
{
    if (action)
    {
        g_free (action->data);
        g_free (action);
    }
}


static PatternInfo *
pattern_info_new (OutputType  type,
                  const char *pattern,
                  const char *style,
                  GSList     *actions,
                  guint       span)
{
    GRegex *re;
    PatternInfo *info;
    GError *error = NULL;

    re = g_regex_new (pattern, (GRegexCompileFlags) 0, (GRegexMatchFlags) 0, &error);

    if (!re)
    {
        g_warning ("%s", error->message);
        g_error_free (error);
        return NULL;
    }

    info = g_new0 (PatternInfo, 1);
    info->type = type;
    info->re = re;
    info->mi = NULL;
    info->style = g_strdup (style);
    info->actions = actions;
    info->span = span;

    return info;
}

static void
pattern_info_free (PatternInfo *pattern)
{
    if (pattern)
    {
        if (pattern->re)
            g_regex_unref (pattern->re);
        g_match_info_free (pattern->mi);
        g_slist_foreach (pattern->actions, (GFunc) action_info_free, NULL);
        g_slist_free (pattern->actions);
        g_free (pattern->style);
        g_free (pattern);
    }
}


static GRegex *
get_re_all (GSList    *patterns,
            OutputType type)
{
    GString *str = NULL;
    GRegex *regex;
    GError *error = NULL;

    while (patterns)
    {
        PatternInfo *pat;

        pat = (PatternInfo*) patterns->data;
        patterns = patterns->next;

        if (pat->type != type && pat->type != OUTPUT_ALL)
            continue;

        if (!str)
            str = g_string_new (NULL);
        else
            g_string_append_c (str, '|');

        g_string_append (str, g_regex_get_pattern (pat->re));
    }

    if (!str)
        return NULL;

    regex = g_regex_new (str->str, G_REGEX_DUPNAMES, (GRegexMatchFlags) 0, &error);

    if (!regex)
    {
        g_warning ("%s", error->message);
        g_error_free (error);
    }

    g_string_free (str, TRUE);
    return regex;
}

static FilterState *
filter_state_new (GSList *patterns)
{
    FilterState *state;
    guint i;

    state = g_new0 (FilterState, 1);
    state->ref_count = 1;
    state->re_out = get_re_all (patterns, OUTPUT_STDOUT);
    state->re_err = get_re_all (patterns, OUTPUT_STDERR);
    state->n_patterns = g_slist_length (patterns);
    state->patterns = g_new0 (PatternInfo*, state->n_patterns);

    for (i = 0; patterns != NULL; ++i, patterns = patterns->next)
        state->patterns[i] = (PatternInfo*) patterns->data;

    return state;
}

static void
filter_state_unref (FilterState *state)
{
    if (state && !--state->ref_count)
    {
        guint i;

        for (i = 0; i < state->n_patterns; ++i)
            pattern_info_free (state->patterns[i]);

        if (state->re_out)
            g_regex_unref (state->re_out);
        if (state->re_err)
            g_regex_unref (state->re_err);

        g_free (state->patterns);
        g_free (state);
    }
}


static FilterInfo *
filter_info_new (const char *id,
                 const char *name,
                 GSList     *patterns)
{
    FilterInfo *info;

    info = g_new0 (FilterInfo, 1);
    info->ref_count = 1;
    info->id = g_strdup (id);
    info->name = g_strdup (name);
    info->state = filter_state_new (patterns);

    return info;
}

static FilterInfo *
filter_info_ref (FilterInfo *filter)
{
    g_return_val_if_fail (filter != NULL, NULL);
    ++filter->ref_count;
    return filter;
}

static void
filter_info_unref (FilterInfo *filter)
{
    if (filter && !--filter->ref_count)
    {
        filter_state_unref (filter->state);
        g_free (filter->name);
        g_free (filter->id);
        g_free (filter);
    }
}


/****************************************************************************/
/* Loading and saving
 */

static ActionInfo *
parse_action_node (MooMarkupNode *node,
                   const char    *file)
{
    const char *type_prop;
    const char *name;
    const char *substring;
    ActionType type;
    ActionTarget target;

    type_prop = moo_markup_get_prop (node, PROP_ACTION_TYPE);

    if (!type_prop)
    {
        g_warning ("in file %s: action type missing", file);
        return NULL;
    }

    if (!strcmp (type_prop, "push"))
    {
        type = ACTION_PUSH;
    }
    else if (!strcmp (type_prop, "pop"))
    {
        type = ACTION_POP;
    }
    else
    {
        g_warning ("in file %s: unknown action type '%s'", file, type_prop);
        return NULL;
    }

    name = moo_markup_get_prop (node, PROP_ACTION_TARGET);

    if (!strcmp (name, "file"))
        target = ACTION_FILE;
    else if (!strcmp (name, "directory"))
        target = ACTION_DIR;
    else
    {
        g_warning ("in file %s: unknown action target type '%s'", file, name);
        return NULL;
    }

    substring = moo_markup_get_prop (node, PROP_SUBSTRING);

    return action_info_new (type, target, substring);
}


static PatternInfo *
parse_match_node (MooMarkupNode *node,
                  const char    *file)
{
    const char *type_prop;
    const char *span_prop;
    const char *pattern;
    const char *style;
    GSList *actions = NULL;
    OutputType type = OUTPUT_ALL;
    PatternInfo *pattern_info;
    MooMarkupNode *child;
    int span = 0;

    type_prop = moo_markup_get_prop (node, PROP_OUTPUT_TYPE);
    pattern = moo_markup_get_prop (node, PROP_PATTERN);
    style = moo_markup_get_prop (node, PROP_STYLE);
    span_prop = moo_markup_get_prop (node, PROP_SPAN);

    if (span_prop)
    {
        span = _moo_convert_string_to_int (span_prop, -1);
        if (span < 0)
        {
            g_warning ("in file %s: invalid span value '%s'", file, span_prop);
            goto error;
        }
    }

    if (type_prop)
    {
        if (!strcmp (type_prop, "all"))
            type = OUTPUT_ALL;
        else if (!strcmp (type_prop, "stdout"))
            type = OUTPUT_STDOUT;
        else if (!strcmp (type_prop, "stderr"))
            type = OUTPUT_STDERR;
        else
        {
            g_warning ("in file %s: invalid output type '%s'", file, type_prop);
            goto error;
        }
    }

    for (child = node->children; child != NULL; child = child->next)
    {
        ActionInfo *action;

        if (!MOO_MARKUP_IS_ELEMENT (child))
            continue;

        if (strcmp (child->name, ELEMENT_ACTION) != 0)
        {
            g_warning ("in file %s: invalid element %s", file, child->name);
            goto error;
        }

        action = parse_action_node (child, file);

        if (!action)
            goto error;

        actions = g_slist_prepend (actions, action);
    }

    actions = g_slist_reverse (actions);
    pattern_info = pattern_info_new (type, pattern, style, actions, span);

    if (!pattern_info)
        goto error;

    return pattern_info;

error:
    g_slist_foreach (actions, (GFunc) action_info_free, NULL);
    g_slist_free (actions);
    return NULL;
}


static FilterInfo *
parse_filter_node (MooMarkupNode *elm,
                   const char    *file)
{
    const char *id;
    const char *name;
    const char *_name;
    FilterInfo *info;
    GSList *patterns = NULL;
    MooMarkupNode *child;

    id = moo_markup_get_prop (elm, PROP_FILTER_ID);
    name = moo_markup_get_prop (elm, PROP_FILTER_NAME);
    _name = moo_markup_get_prop (elm, PROP_FILTER__NAME);

    if (!id || !id[0])
    {
        g_warning ("in file %s: filter id missing", file);
        return NULL;
    }

    if (_name && _name[0])
    {
        name = Q_(_name);
    }
    else if (!name || !name[0])
    {
        g_warning ("in file %s: filter name missing", file);
        return NULL;
    }

    for (child = elm->children; child != NULL; child = child->next)
    {
        PatternInfo *pat;

        if (!MOO_MARKUP_IS_ELEMENT (child))
            continue;

        if (strcmp (child->name, ELEMENT_MATCH) != 0)
        {
            g_warning ("in file %s: invalid element %s", file, child->name);
            goto error;
        }

        pat = parse_match_node (child, file);

        if (!pat)
            goto error;

        patterns = g_slist_prepend (patterns, pat);
    }

    patterns = g_slist_reverse (patterns);
    info = filter_info_new (id, name, patterns);
    g_slist_free (patterns);
    return info;

error:
    g_slist_foreach (patterns, (GFunc) pattern_info_free, NULL);
    g_slist_free (patterns);
    return NULL;
}


static void
parse_filter_file (const char *file)
{
    MooMarkupDoc *doc;
    MooMarkupNode *root, *node;
    GError *error = NULL;

    if (!g_file_test (file, G_FILE_TEST_EXISTS))
        return;

    doc = moo_markup_parse_file (file, &error);

    if (!doc)
    {
        g_warning ("could not parse file %s: %s", file, error->message);
        g_error_free (error);
        return;
    }

    root = moo_markup_get_root_element (doc, ELEMENT_ROOT);

    if (root)
    {
        const char *version = moo_markup_get_prop (root, PROP_VERSION);

        if (!version || strcmp (version, FILTERS_VERSION) != 0)
        {
            g_warning ("in file %s: invalid version '%s'",
                       file, version ? version : "(null)");
            moo_markup_doc_unref (doc);
            return;
        }
    }
    else
    {
        root = MOO_MARKUP_NODE (doc);
    }

    for (node = root->children; node != NULL; node = node->next)
    {
        FilterInfo *info;

        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        if (strcmp (node->name, ELEMENT_FILTER) != 0)
        {
            g_warning ("in file %s: invalid element %s", file, node->name);
            continue;
        }

        info = parse_filter_node (node, file);

        if (info)
            moo_command_filter_register (info->id, info->name,
                                         filter_factory_func, info,
                                         (GDestroyNotify) filter_info_unref);
    }

    moo_markup_doc_unref (doc);
}


void
_moo_command_filter_regex_load (void)
{
    char **files, **p;

    files = _moo_strv_reverse (moo_get_data_files (FILTERS_FILE));

    for (p = files; p && *p; ++p)
        parse_filter_file (*p);

    g_strfreev (files);
}
