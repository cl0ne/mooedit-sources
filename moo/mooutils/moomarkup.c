/*
 *   moomarkup.c
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

#include "mooutils/moomarkup.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#include <string.h>
#include <mooglib/moo-glib.h>


typedef struct {
    MooMarkupDoc   *doc;
    MooMarkupNode  *current;
} ParserState;

MOO_DEFINE_BOXED_TYPE_R (MooMarkupDoc, moo_markup_doc)

static MooMarkupDoc     *moo_markup_doc_new_priv    (const char         *name);
static void              moo_markup_doc_set_name    (MooMarkupDoc       *doc,
                                                     const char         *name);
static MooMarkupNode    *moo_markup_element_new     (MooMarkupDoc       *doc,
                                                     MooMarkupNode      *parent,
                                                     const char         *name,
                                                     const char        **attribute_names,
                                                     const char        **attribute_values);
static MooMarkupNode    *moo_markup_text_node_new   (MooMarkupNodeType   type,
                                                     MooMarkupDoc       *doc,
                                                     MooMarkupNode      *parent,
                                                     const char         *text,
                                                     gssize              text_len);
static MooMarkupNode    *moo_markup_text_node_new_take_string (MooMarkupNodeType   type,
                                                     MooMarkupDoc       *doc,
                                                     MooMarkupNode      *parent,
                                                     char               *text,
                                                     gssize              text_len);
static void              add_node                   (MooMarkupDoc       *doc,
                                                     MooMarkupNode      *parent,
                                                     MooMarkupNode      *node);


static void moo_markup_doc_unref_private        (MooMarkupDoc       *doc);
static void moo_markup_element_free             (MooMarkupElement   *node);
static void moo_markup_text_node_free           (MooMarkupNode      *node);
static void moo_markup_node_free                (MooMarkupNode      *node);

static void collect_text_content                (MooMarkupElement   *node);
static void moo_markup_text_node_add_text       (MooMarkupText      *node,
                                                 const char         *text,
                                                 gssize              text_len);

static void moo_markup_element_print            (MooMarkupElement   *node,
                                                 GString            *dest);
static void moo_markup_text_node_print          (MooMarkupNode      *node,
                                                 GString            *dest);
static void moo_markup_comment_node_print       (MooMarkupNode      *node,
                                                 GString            *dest);


static void start_element   (GMarkupParseContext    *context,
                             const gchar            *element_name,
                             const gchar           **attribute_names,
                             const gchar           **attribute_values,
                             ParserState            *state,
                             GError **error);
static void end_element     (GMarkupParseContext    *context,
                             const gchar            *element_name,
                             ParserState            *state,
                             GError                **error);
static void text            (GMarkupParseContext    *context,
                             const gchar            *text,
                             gsize                   text_len,
                             ParserState            *state,
                             GError                **error);
static void passthrough     (GMarkupParseContext    *context,
                             const gchar            *passthrough_text,
                             gsize                   text_len,
                             ParserState            *state,
                             GError                **error);


static void
set_modified (MooMarkupNode *node,
              gboolean       modified)
{
    if (node->doc)
    {
#ifdef MOO_DEBUG
        if (modified && !node->doc->modified && node->doc->track_modified)
        {
            g_message ("markup doc %s modified",
                       node->doc->name ? node->doc->name : "<NULL>");
        }
#endif

        node->doc->modified = modified != 0;
    }
}


void
_moo_markup_set_modified (MooMarkupDoc *doc,
                          gboolean      modified)
{
    g_return_if_fail (MOO_MARKUP_IS_DOC (doc));
    set_modified (MOO_MARKUP_NODE (doc), modified);
}


void
_moo_markup_set_track_modified (MooMarkupDoc *doc,
                                gboolean      track)
{
    g_return_if_fail (MOO_MARKUP_IS_DOC (doc));
    doc->track_modified = track != 0;
}


gboolean
_moo_markup_get_modified (MooMarkupDoc *doc)
{
    g_return_val_if_fail (MOO_MARKUP_IS_DOC (doc), FALSE);
    return doc->modified;
}


static MooMarkupDoc *
markup_parse_memory (const char     *buffer,
                     gssize          len,
                     G_GNUC_UNUSED const char *filename,
                     GError        **error)
{
    GMarkupParser parser = { (MooMarkupStartElementFunc) start_element,
                             (MooMarkupEndElementFunc) end_element,
                             (MooMarkupTextFunc) text,
                             (MooMarkupPassthroughFunc) passthrough,
                             NULL};

    ParserState state;
    GMarkupParseContext *context;

    if (len < 0)
        len = strlen (buffer);

    state.doc = moo_markup_doc_new_priv (NULL);
    _moo_markup_set_modified (state.doc, TRUE);

    state.current = MOO_MARKUP_NODE (state.doc);
    context = g_markup_parse_context_new (&parser, G_MARKUP_TREAT_CDATA_AS_TEXT, &state, NULL);

    if (!g_markup_parse_context_parse (context, buffer, len, error) ||
         !g_markup_parse_context_end_parse (context, error))
    {
        g_markup_parse_context_free (context);
        moo_markup_doc_unref (state.doc);
        return NULL;
    }

    g_markup_parse_context_free (context);
    return state.doc;
}

MooMarkupDoc *
moo_markup_parse_memory (const char     *buffer,
                         gssize          size,
                         GError        **error)
{
    g_return_val_if_fail (buffer != NULL, NULL);
    return markup_parse_memory (buffer, size, "memory", error);
}


MooMarkupDoc *
moo_markup_parse_file (const char     *filename,
                       GError        **error)
{
    char *content;
    MooMarkupDoc *doc;
    gsize len;

    g_return_val_if_fail (filename != NULL, NULL);

    /* XXX use moo_parse_markup_file here */

    if (!g_file_get_contents (filename, &content, &len, error))
        return NULL;

    doc = markup_parse_memory (content, len, filename, error);

    if (doc)
        moo_markup_doc_set_name (doc, filename);

    g_free (content);
    return doc;
}


static void
start_element (G_GNUC_UNUSED GMarkupParseContext    *ctx,
               const gchar            *element_name,
               const gchar           **attribute_names,
               const gchar           **attribute_values,
               ParserState            *state,
               G_GNUC_UNUSED GError                **error)
{
    MooMarkupNode *elm = moo_markup_element_new (state->doc,
                                                 state->current,
                                                 element_name,
                                                 attribute_names,
                                                 attribute_values);
    g_assert (elm->parent == state->current);
    state->current = elm;
}


static void
end_element (G_GNUC_UNUSED GMarkupParseContext    *ctx,
             G_GNUC_UNUSED const gchar            *elm,
             ParserState            *state,
             G_GNUC_UNUSED GError                **error)
{
    g_assert (state->current->type == MOO_MARKUP_ELEMENT_NODE);
    collect_text_content (MOO_MARKUP_ELEMENT (state->current));
    g_assert (state->current->parent != NULL);
    state->current = state->current->parent;
}


static void
text (G_GNUC_UNUSED GMarkupParseContext    *ctx,
      const gchar            *text,
      gsize                   text_len,
      ParserState            *state,
      G_GNUC_UNUSED GError                **error)
{
    if (MOO_MARKUP_IS_TEXT (state->current->last))
        moo_markup_text_node_add_text (MOO_MARKUP_TEXT (state->current->last),
                                       text, text_len);
    else
        moo_markup_text_node_new (MOO_MARKUP_TEXT_NODE,
                                  state->doc, state->current,
                                  text, text_len);
}


static void
passthrough (G_GNUC_UNUSED GMarkupParseContext    *ctx,
             const gchar            *passthrough_text,
             gsize                   text_len,
             ParserState            *state,
             G_GNUC_UNUSED GError                **error)
{
    if (MOO_MARKUP_IS_COMMENT (state->current->last))
        moo_markup_text_node_add_text (MOO_MARKUP_COMMENT (state->current->last),
                                       passthrough_text, text_len);
    else
        moo_markup_text_node_new (MOO_MARKUP_COMMENT_NODE,
                                  state->doc, state->current,
                                  passthrough_text, text_len);
}


static MooMarkupDoc *
moo_markup_doc_new_priv (const char *name)
{
    MooMarkupDoc *doc = g_slice_new0 (MooMarkupDoc);

    doc->type = MOO_MARKUP_DOC_NODE;
    doc->name = g_strdup (MOO_NZS (name));
    doc->doc = doc;
    doc->ref_count = 1;

    return doc;
}


static void
moo_markup_doc_set_name (MooMarkupDoc *doc,
                         const char   *name)
{
    g_return_if_fail (MOO_MARKUP_IS_DOC (doc));
    MOO_ASSIGN_STRING (doc->name, MOO_NZS (name));
}


MooMarkupDoc *
moo_markup_doc_new (const char *name)
{
    return moo_markup_doc_new_priv (name);
}


static MooMarkupNode *
moo_markup_element_new (MooMarkupDoc   *doc,
                        MooMarkupNode  *parent,
                        const char     *name,
                        const char    **attribute_names,
                        const char    **attribute_values)
{
    MooMarkupElement *elm;

    elm = g_slice_new0 (MooMarkupElement);
    add_node (doc, parent, MOO_MARKUP_NODE (elm));

    elm->type = MOO_MARKUP_ELEMENT_NODE;
    elm->name = g_strdup (name);

    elm->attr_names = g_strdupv ((char**) attribute_names);
    elm->attr_vals = g_strdupv ((char**) attribute_values);
    if (elm->attr_names)
        for (elm->n_attrs = 0; attribute_names[elm->n_attrs]; ++elm->n_attrs) ;
    else
        elm->n_attrs = 0;

    return MOO_MARKUP_NODE (elm);
}


static MooMarkupNode *
moo_markup_text_node_new_take_string (MooMarkupNodeType   type,
                                      MooMarkupDoc       *doc,
                                      MooMarkupNode      *parent,
                                      char               *text,
                                      gssize              text_len)
{
    MooMarkupText *node;

    g_assert (type == MOO_MARKUP_TEXT_NODE || type == MOO_MARKUP_COMMENT_NODE);

    node = g_slice_new0 (MooMarkupText);
    add_node (doc, parent, MOO_MARKUP_NODE (node));

    node->type = type;

    if (type == MOO_MARKUP_TEXT_NODE)
        node->name = g_strdup ("TEXT");
    else
        node->name = g_strdup ("COMMENT");

    if (text_len < 0)
        text_len = strlen (text);

    node->text = text;
    node->size = text_len;

    return MOO_MARKUP_NODE (node);
}

static MooMarkupNode *
moo_markup_text_node_new (MooMarkupNodeType   type,
                          MooMarkupDoc       *doc,
                          MooMarkupNode      *parent,
                          const char         *text,
                          gssize              text_len)
{
    if (text_len < 0)
        text_len = strlen (text);

    return moo_markup_text_node_new_take_string (type, doc, parent,
                                                 g_strndup (text, text_len),
                                                 text_len);
}


static void
add_node (MooMarkupDoc     *doc,
          MooMarkupNode    *parent,
          MooMarkupNode    *node)
{
    g_assert (MOO_MARKUP_IS_DOC (doc));
    g_assert (parent != NULL);
    g_assert (node != NULL);

    node->doc = doc;
    node->parent = parent;

    if (parent->last)
    {
        parent->last->next = node;
        node->prev = parent->last;
        parent->last = node;
    }
    else
    {
        g_assert (parent->children == NULL);
        g_assert (parent->last == NULL);
        parent->children = node;
        parent->last = node;
    }

    _moo_markup_set_modified (doc, TRUE);
}


static void
moo_markup_text_node_add_text (MooMarkupText  *node,
                               const char     *text,
                               gssize          text_len)
{
    if (text_len < 0)
        text_len = strlen (text);

    if (text_len == 0)
        return;

    node->text = g_renew (char, node->text, node->size + text_len + 1);
    memcpy (node->text + node->size, text, text_len);
    node->size += text_len;
    node->text[node->size] = 0;

    set_modified (MOO_MARKUP_NODE (node), TRUE);
}


static void
collect_text_content (MooMarkupElement *node)
{
    MooMarkupNode *child;
    GString *text = NULL;

    for (child = node->children; child != NULL; child = child->next)
    {
        if (MOO_MARKUP_IS_TEXT (child))
        {
            if (text)
            {
                g_string_append_len (text,
                                     MOO_MARKUP_TEXT(child)->text,
                                     MOO_MARKUP_TEXT(child)->size);
            }
            else
            {
                text = g_string_new_len (MOO_MARKUP_TEXT(child)->text,
                                         MOO_MARKUP_TEXT(child)->size);
            }
        }
    }

    g_free (node->content);

    if (text)
        node->content = g_string_free (text, FALSE);
    else
        node->content = NULL;
}


static void
moo_markup_node_free (MooMarkupNode *node)
{
    MooMarkupNode *child;

    g_return_if_fail (node != NULL);

    child = node->children;

    while (child)
    {
        MooMarkupNode *next = child->next;
        moo_markup_node_free (child);
        child = next;
    }

    g_free (node->name);

    switch (node->type)
    {
        case MOO_MARKUP_DOC_NODE:
            moo_markup_doc_unref_private (MOO_MARKUP_DOC (node));
            g_slice_free (MooMarkupDoc, (MooMarkupDoc*) node);
            break;
        case MOO_MARKUP_ELEMENT_NODE:
            moo_markup_element_free (MOO_MARKUP_ELEMENT (node));
            g_slice_free (MooMarkupElement, (MooMarkupElement*) node);
            break;
        case MOO_MARKUP_TEXT_NODE:
            moo_markup_text_node_free (node);
            g_slice_free (MooMarkupText, (MooMarkupText*) node);
            break;
        case MOO_MARKUP_COMMENT_NODE:
            moo_markup_text_node_free (node);
            g_slice_free (MooMarkupComment, (MooMarkupComment*) node);
            break;
        default:
            g_assert_not_reached ();
    }
}


static void
moo_markup_element_free (MooMarkupElement *node)
{
    g_free (node->content);
    g_strfreev (node->attr_names);
    g_strfreev (node->attr_vals);
}


static void
moo_markup_text_node_free (MooMarkupNode *node)
{
    g_assert (node->type == MOO_MARKUP_TEXT_NODE || node->type == MOO_MARKUP_COMMENT_NODE);
    g_free (((MooMarkupText*)node)->text);
}


void
moo_markup_doc_unref (MooMarkupDoc *doc)
{
    if (doc)
    {
        g_return_if_fail (MOO_MARKUP_IS_DOC (doc));
        if (--(doc->ref_count)) return;
        moo_markup_node_free (MOO_MARKUP_NODE (doc));
    }
}


MooMarkupDoc*
moo_markup_doc_ref (MooMarkupDoc *doc)
{
    g_return_val_if_fail (MOO_MARKUP_IS_DOC (doc), NULL);
    ++(doc->ref_count);
    return doc;
}


static void
moo_markup_doc_unref_private (G_GNUC_UNUSED MooMarkupDoc *doc)
{
}


char*
moo_markup_node_get_string (MooMarkupNode *node)
{
    MooMarkupNode *child;
    GString *str = g_string_new ("");

    g_return_val_if_fail (node != NULL, NULL);

    for (child = node->children; child != NULL; child = child->next)
        switch (child->type)
        {
            case MOO_MARKUP_ELEMENT_NODE:
                moo_markup_element_print (MOO_MARKUP_ELEMENT (child), str);
                break;
            case MOO_MARKUP_TEXT_NODE:
                moo_markup_text_node_print (child, str);
                break;
            case MOO_MARKUP_COMMENT_NODE:
                moo_markup_comment_node_print (child, str);
                break;
            default:
                g_assert_not_reached ();
        }

    return g_string_free (str, FALSE);
}


static void
moo_markup_element_print (MooMarkupElement   *node,
                          GString            *str)
{
    guint i;

    g_string_append_printf (str, "<%s", node->name);
    for (i = 0; i < node->n_attrs; ++i)
        g_string_append_printf (str, " %s=\"%s\"",
                                node->attr_names[i],
                                node->attr_vals[i]);

    if (node->children)
    {
        MooMarkupNode *child;

        g_string_append (str, ">");
        for (child = node->children; child != NULL; child = child->next)
            switch (child->type)
            {
                case MOO_MARKUP_ELEMENT_NODE:
                    moo_markup_element_print (MOO_MARKUP_ELEMENT (child), str);
                    break;
                case MOO_MARKUP_COMMENT_NODE:
                    moo_markup_comment_node_print (child, str);
                    break;
                case MOO_MARKUP_TEXT_NODE:
                    moo_markup_text_node_print (child, str);
                    break;
                default:
                    g_assert_not_reached ();
            }
        g_string_append_printf (str, "</%s>", node->name);
    }
    else
        g_string_append (str, "/>");
}


static void
moo_markup_text_node_print (MooMarkupNode  *node,
                            GString        *str)
{
    char *p, *escaped;
    MooMarkupText *text = (MooMarkupText*) node;

    if (!g_utf8_validate (text->text, -1, (const char**) &p))
    {
        g_critical ("invalid UTF8");
        *p = 0;
    }

    escaped = g_markup_escape_text (text->text, -1);
    g_string_append (str, escaped);
    g_free (escaped);
}


static void
moo_markup_comment_node_print (MooMarkupNode *node,
                               GString       *str)
{
    char *p;
    MooMarkupText *text = (MooMarkupText*) node;

    if (!g_utf8_validate (text->text, -1, (const char**) &p))
    {
        g_critical ("invalid UTF8");
        *p = 0;
    }

    g_string_append (str, text->text);
}


const char*
moo_markup_get_content (MooMarkupNode *node)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node), NULL);
    return MOO_MARKUP_ELEMENT(node)->content;
}


static MooMarkupElement*
get_element (MooMarkupNode *node,
             char         **path)
{
    MooMarkupNode *child;

    if (!path[0])
        return MOO_MARKUP_ELEMENT (node);

    for (child = node->children; child; child = child->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (child))
        {
            if (!strcmp (path[0], child->name))
                return get_element (child, ++path);
        }
    }

    return NULL;
}


MooMarkupNode*
moo_markup_get_element (MooMarkupNode      *node,
                        const char         *path)
{
    char **p;
    MooMarkupElement *elm;

    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node) ||
                          MOO_MARKUP_IS_DOC (node), NULL);

    p = g_strsplit (path, "/", 0);
    g_return_val_if_fail (p != NULL, NULL);

    elm = get_element (node, p);

    g_strfreev (p);
    return (MooMarkupNode*)elm;
}


const char*
moo_markup_get_prop (MooMarkupNode      *node,
                     const char         *prop_name)
{
    guint i;
    MooMarkupElement *elm;

    g_return_val_if_fail (node != NULL && prop_name != NULL, NULL);
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node), NULL);

    elm = MOO_MARKUP_ELEMENT(node);

    for (i = 0 ; i < elm->n_attrs; ++i)
        if (!strcmp (prop_name, elm->attr_names[i]))
            return elm->attr_vals[i];

    return NULL;
}


void
moo_markup_set_prop (MooMarkupNode      *elm,
                     const char         *prop_name,
                     const char         *val)
{
    guint i;
    char **attr_names, **attr_vals;
    MooMarkupElement *node;

    g_return_if_fail (elm != NULL && prop_name != NULL);
    g_return_if_fail (MOO_MARKUP_IS_ELEMENT (elm));
    g_return_if_fail (!val || g_utf8_validate (val, -1, NULL));

    set_modified (elm, TRUE);

    /* XXX validate prop_name and val */

    node = MOO_MARKUP_ELEMENT (elm);

    for (i = 0 ; i < node->n_attrs; ++i)
        if (!strcmp (prop_name, node->attr_names[i]))
        {
            if (val)
            {
                g_free (node->attr_vals[i]);
                node->attr_vals[i] = g_strdup (val);
                return;
            }
            else if (node->n_attrs == 1)
            {
                g_strfreev (node->attr_names);
                g_strfreev (node->attr_vals);
                node->attr_names = NULL;
                node->attr_vals = NULL;
                node->n_attrs = 0;
                return;
            }
            else
            {
                guint j;

                /* XXX just move them */

                attr_names = g_new (char*, node->n_attrs);
                attr_vals = g_new (char*, node->n_attrs);

                for (j = 0; j < i; ++j)
                {
                    attr_names[j] = node->attr_names[j];
                    attr_vals[j] = node->attr_vals[j];
                }

                for (j = i + 1; j < node->n_attrs; ++j)
                {
                    attr_names[j-1] = node->attr_names[j];
                    attr_vals[j-1] = node->attr_vals[j];
                }

                attr_names[node->n_attrs - 1] = NULL;
                attr_vals[node->n_attrs - 1] = NULL;

                g_free (node->attr_names[i]);
                g_free (node->attr_vals[i]);
                g_free (node->attr_names);
                g_free (node->attr_vals);

                node->attr_names = attr_names;
                node->attr_vals = attr_vals;
                node->n_attrs--;

                return;
            }
        }

    if (!val)
        return;

    g_return_if_fail (g_utf8_validate (prop_name, -1, NULL));

    attr_names = g_new (char*, node->n_attrs + 2);
    attr_vals = g_new (char*, node->n_attrs + 2);

    for (i = 0; i < node->n_attrs; ++i)
    {
        attr_names[i] = node->attr_names[i];
        attr_vals[i] = node->attr_vals[i];
    }

    attr_names[node->n_attrs] = g_strdup (prop_name);
    attr_vals[node->n_attrs] = g_strdup (val);
    attr_names[node->n_attrs + 1] = NULL;
    attr_vals[node->n_attrs + 1] = NULL;

    g_free (node->attr_names);
    g_free (node->attr_vals);

    node->attr_names = attr_names;
    node->attr_vals = attr_vals;
    node->n_attrs++;
}


int
moo_markup_int_prop (MooMarkupNode *node,
                     const char    *prop_name,
                     int            default_val)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node), default_val);
    g_return_val_if_fail (prop_name != NULL, default_val);
    return _moo_convert_string_to_int (moo_markup_get_prop (node, prop_name),
                                       default_val);
}

guint
moo_markup_uint_prop (MooMarkupNode *node,
                      const char    *prop_name,
                      guint          default_val)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node), default_val);
    g_return_val_if_fail (prop_name != NULL, default_val);
    return _moo_convert_string_to_uint (moo_markup_get_prop (node, prop_name),
                                        default_val);
}

gboolean
moo_markup_bool_prop (MooMarkupNode *node,
                      const char    *prop_name,
                      gboolean       default_val)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node), default_val);
    g_return_val_if_fail (prop_name != NULL, default_val);
    return _moo_convert_string_to_bool (moo_markup_get_prop (node, prop_name),
                                        default_val);
}


void
moo_markup_set_bool_prop (MooMarkupNode *node,
                          const char    *prop_name,
                          gboolean       val)
{
    moo_markup_set_prop (node, prop_name,
                         _moo_convert_bool_to_string (val));
}


MooMarkupNode*
moo_markup_get_root_element (MooMarkupDoc       *doc,
                             const char         *name)
{
    MooMarkupNode *child;

    for (child = doc->children; child; child = child->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (child))
        {
            if (!name || !strcmp (child->name, name))
                return child;
        }
    }

    return NULL;
}


char*
moo_markup_element_get_path (MooMarkupElement *elm)
{
    GString *path;
    MooMarkupNode *node;

    g_return_val_if_fail (elm != NULL, NULL);

    path = g_string_new (elm->name);

    for (node = elm->parent;
         node != NULL && MOO_MARKUP_IS_ELEMENT (node);
         node = node->parent)
    {
        g_string_prepend (path, "/");
        g_string_prepend (path, node->name);
    }

    return g_string_free (path, FALSE);
}


void
moo_markup_delete_node (MooMarkupNode *node)
{
    MooMarkupNode *parent, *child, *next, *prev;

    g_return_if_fail (node != NULL);
    g_return_if_fail (node->parent != NULL);

    set_modified (node, TRUE);
    parent = node->parent;

    for (child = parent->children; child != NULL && child != node;
         child = child->next) ;

    g_return_if_fail (child == node);

    next = node->next;
    prev = node->prev;

    if (parent->children == node)
    {
        g_assert (node->prev == NULL);
        parent->children = next;
    }

    if (parent->last == node)
    {
        g_assert (node->next == NULL);
        parent->last = prev;
    }

    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;

    moo_markup_node_free (node);
}


MooMarkupNode*
moo_markup_create_root_element (MooMarkupDoc       *doc,
                                const char         *name)
{
    MooMarkupNode *elm;

    g_return_val_if_fail (MOO_MARKUP_IS_DOC (doc), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    elm = moo_markup_get_root_element (doc, name);

    if (elm)
        return elm;

    elm = moo_markup_element_new (doc, MOO_MARKUP_NODE (doc), name, NULL, NULL);

    return elm;
}


static MooMarkupNode*
create_element (MooMarkupNode      *node,
                char              **path)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node) || MOO_MARKUP_IS_DOC (node), NULL);
    g_return_val_if_fail (path && path[0], NULL);

    if (path[1])
    {
        MooMarkupNode *child = NULL;

        for (child = node->children; child != NULL; child = child->next)
            if (MOO_MARKUP_IS_ELEMENT (child) && child->name && !strcmp (child->name, path[0]))
                break;

        if (!child)
            child = moo_markup_element_new (node->doc, MOO_MARKUP_NODE (node),
                                            path[0], NULL, NULL);

        return create_element (child, ++path);
    }
    else
    {
        return moo_markup_element_new (node->doc, MOO_MARKUP_NODE (node), path[0], NULL, NULL);
    }
}


MooMarkupNode*
moo_markup_create_element (MooMarkupNode      *parent,
                           const char         *path)
{
    char **pieces;
    MooMarkupNode *elm;

    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (parent) ||
                          MOO_MARKUP_IS_DOC (parent), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    pieces = g_strsplit (path, "/", 0);
    g_return_val_if_fail (pieces != NULL, NULL);

    elm = create_element (parent, pieces);

    g_strfreev (pieces);
    return elm;
}


MooMarkupNode *
moo_markup_create_text_element (MooMarkupNode      *parent,
                                const char         *path,
                                const char         *content)
{
    MooMarkupElement *elm;

    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (parent) ||
                          MOO_MARKUP_IS_DOC (parent), NULL);
    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (!content || g_utf8_validate (content, -1, NULL), NULL);

    elm = (MooMarkupElement*) moo_markup_create_element (parent, path);

    if (!elm)
        return NULL;

    content = MOO_NZS (content);

    moo_markup_text_node_new (MOO_MARKUP_TEXT_NODE,
                              elm->doc, MOO_MARKUP_NODE (elm),
                              content, strlen (content));

    g_free (elm->content);
    elm->content = g_strdup (content);

    return MOO_MARKUP_NODE (elm);
}


void
moo_markup_set_content (MooMarkupNode *node,
                        const char    *content)
{
    MooMarkupElement *elm;

    g_return_if_fail (MOO_MARKUP_IS_ELEMENT (node));

    elm = MOO_MARKUP_ELEMENT (node);
    g_free (elm->content);
    elm->content = g_strdup (content);

    while (node->children)
        moo_markup_delete_node (node->children);

    if (content)
        moo_markup_text_node_new (MOO_MARKUP_TEXT_NODE,
                                  elm->doc, node,
                                  content, strlen (content));
}


#if 0
MooMarkupNode*
moo_markup_create_file_element (MooMarkupNode      *parent,
                                const char         *path,
                                const char         *filename)
{
    char *filename_utf8;
    MooMarkupNode *elm;

    g_return_val_if_fail (filename != NULL, NULL);

    filename_utf8 = g_filename_display_name (filename);

    if (!filename_utf8)
    {
        g_warning ("could not convert '%s' to utf8",
                   filename);
        return NULL;
    }

    elm = moo_markup_create_text_element (parent, path, filename_utf8);

    g_free (filename_utf8);
    return elm;
}

char *
moo_markup_get_file_content (MooMarkupNode *node)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (node), NULL);

    if (!MOO_MARKUP_ELEMENT(node)->content)
        return NULL;

    return g_filename_from_utf8 (MOO_MARKUP_ELEMENT(node)->content,
                                 -1, NULL, NULL, NULL);
}


gboolean
moo_markup_save (MooMarkupDoc       *doc,
                 const char         *filename,
                 GError            **error)
{
    char *text;
    gboolean result;

    g_return_val_if_fail (MOO_MARKUP_IS_DOC (doc), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    text = moo_markup_node_get_string (MOO_MARKUP_NODE (doc));
    g_return_val_if_fail (text != NULL, FALSE);

    result = _moo_save_file_utf8 (filename, text, -1, error);
    g_free (text);
    return result;
}
#endif


#define INDENT_CHAR ' '

static gboolean
write_pretty_element (MooMarkupElement *elm,
                      MooFileWriter    *writer,
                      guint             indent,
                      guint             indent_size)
{
    gboolean isdir = FALSE;
    gboolean empty = TRUE;
    MooMarkupNode *child;
    gboolean result = TRUE;
    char *fill;
    guint i;

    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (elm), FALSE);
    g_return_val_if_fail (writer != NULL, FALSE);

    for (child = elm->children; child != NULL; child = child->next)
    {
        if (elm->content)
            empty = FALSE;

        if (MOO_MARKUP_IS_ELEMENT (child))
        {
            isdir = TRUE;
            empty = FALSE;
            break;
        }
    }

    fill = g_strnfill (indent, INDENT_CHAR);

    moo_file_writer_write (writer, fill, indent);
    moo_file_writer_printf (writer, "<%s", elm->name);

    for (i = 0; result && i < elm->n_attrs; ++i)
    {
        char *attr = g_markup_escape_text (elm->attr_vals[i], -1);
        result = moo_file_writer_printf (writer, " %s=\"%s\"",
                                         elm->attr_names[i],
                                         attr);
        g_free (attr);
    }

    if (empty)
    {
        moo_file_writer_write (writer, "/>\n", -1);
    }
    else if (isdir)
    {
        moo_file_writer_write (writer, ">\n", -1);

        for (child = elm->children; result && child != NULL; child = child->next)
            if (MOO_MARKUP_IS_ELEMENT (child))
                result = write_pretty_element (MOO_MARKUP_ELEMENT (child), writer,
                                               indent + indent_size, indent_size);

        moo_file_writer_printf (writer, "%s</%s>\n",
                                fill, elm->name);
    }
    else
    {
        char *escaped = g_markup_escape_text (elm->content, -1);
        moo_file_writer_printf (writer, ">%s</%s>\n",
                                escaped, elm->name);
        g_free (escaped);
    }

    g_free (fill);
    return result;
}
#undef INDENT_CHAR


gboolean
moo_markup_write_pretty (MooMarkupDoc  *doc,
                         MooFileWriter *writer,
                         guint          indent)
{
    MooMarkupNode *child;
    gboolean result = TRUE;

    g_return_val_if_fail (MOO_MARKUP_IS_DOC (doc), FALSE);
    g_return_val_if_fail (writer != NULL, FALSE);

    result = moo_file_writer_write (writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!-- -*- xml -*- -->\n", -1);

    for (child = doc->children; result && child != NULL; child = child->next)
        if (MOO_MARKUP_IS_ELEMENT (child))
            result = write_pretty_element (MOO_MARKUP_ELEMENT (child), writer, 0, indent);

    return result;
}


#if 0
char *
moo_markup_node_get_pretty_string (MooMarkupNode *node,
                                   guint          indent)
{
    GString *str;
    MooMarkupNode *child;

    g_return_val_if_fail (node != NULL, NULL);

    str = g_string_new (NULL);

    for (child = node->children; child != NULL; child = child->next)
        if (MOO_MARKUP_IS_ELEMENT (child))
            format_pretty_element (MOO_MARKUP_ELEMENT (child), str, 0, indent);

    return g_string_free (str, FALSE);
}
#endif


MooMarkupNode *
MOO_MARKUP_NODE_CHECK_CAST (gpointer n)
{
    MooMarkupNode *node = (MooMarkupNode*) n;
    g_return_val_if_fail (n != NULL, NULL);
    g_return_val_if_fail (node->type == MOO_MARKUP_DOC_NODE ||
                          node->type == MOO_MARKUP_ELEMENT_NODE ||
                          node->type == MOO_MARKUP_TEXT_NODE ||
                          node->type == MOO_MARKUP_COMMENT_NODE,
                          NULL);
    return node;
}

MooMarkupDoc *
MOO_MARKUP_DOC_CHECK_CAST (gpointer n)
{
    MooMarkupNode *node = (MooMarkupNode*) n;
    g_return_val_if_fail (n != NULL, NULL);
    g_return_val_if_fail (node->type == MOO_MARKUP_DOC_NODE, NULL);
    return (MooMarkupDoc*) n;
}

MooMarkupElement *
MOO_MARKUP_ELEMENT_CHECK_CAST (gpointer n)
{
    MooMarkupNode *node = (MooMarkupNode*) n;
    g_return_val_if_fail (n != NULL, NULL);
    g_return_val_if_fail (node->type == MOO_MARKUP_ELEMENT_NODE, NULL);
    return (MooMarkupElement*) n;
}

MooMarkupText *
MOO_MARKUP_TEXT_CHECK_CAST (gpointer n)
{
    MooMarkupNode *node = (MooMarkupNode*) n;
    g_return_val_if_fail (n != NULL, NULL);
    g_return_val_if_fail (node->type == MOO_MARKUP_TEXT_NODE, NULL);
    return (MooMarkupText*) n;
}

MooMarkupComment *
MOO_MARKUP_COMMENT_CHECK_CAST (gpointer n)
{
    MooMarkupNode *node = (MooMarkupNode*) n;
    g_return_val_if_fail (n != NULL, NULL);
    g_return_val_if_fail (node->type == MOO_MARKUP_COMMENT_NODE, NULL);
    return (MooMarkupComment*) n;
}


gboolean
moo_parse_markup_file (const char         *filename,
                       const GMarkupParser *parser,
                       gpointer            data,
                       GError            **error)
{
    GMarkupParseContext *ctx;
    MooFileReader *reader;
    char buf[1024];
    gsize size;
    gboolean seen_error = FALSE;

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (parser != NULL, FALSE);
    g_return_val_if_fail (!error || !*error, FALSE);

    if (!(reader = moo_text_reader_new (filename, error)))
        return FALSE;

    ctx = g_markup_parse_context_new (parser, G_MARKUP_TREAT_CDATA_AS_TEXT, data, NULL);

    while (TRUE)
    {
        if (!moo_file_reader_read (reader, buf, sizeof buf, &size, error))
        {
            seen_error = TRUE;
            break;
        }

        if (!size)
            break;

        if (!g_markup_parse_context_parse (ctx, buf, size, error))
        {
            seen_error = TRUE;
            break;
        }
    }

    if (!seen_error && !g_markup_parse_context_end_parse (ctx, error))
        seen_error = TRUE;

    g_markup_parse_context_free (ctx);
    moo_file_reader_close (reader);

    return !seen_error;
}
