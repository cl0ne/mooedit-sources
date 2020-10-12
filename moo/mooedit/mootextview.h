/*
 *   mootextview.h
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

#pragma once

#include <gtk/gtk.h>
#include <mooedit/mooindenter.h>
#include <mooedit/moolang.h>
#include <mooedit/mootextsearch.h>
#include <mooedit/mootextstylescheme.h>
#include <mooedit/mooedit-enums.h>

G_BEGIN_DECLS


#define MOO_TYPE_TEXT_VIEW              (moo_text_view_get_type ())
#define MOO_TEXT_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_TEXT_VIEW, MooTextView))
#define MOO_TEXT_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_TEXT_VIEW, MooTextViewClass))
#define MOO_IS_TEXT_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_TEXT_VIEW))
#define MOO_IS_TEXT_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_TEXT_VIEW))
#define MOO_TEXT_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_TEXT_VIEW, MooTextViewClass))


typedef struct MooTextView         MooTextView;
typedef struct MooTextViewPrivate  MooTextViewPrivate;
typedef struct MooTextViewClass    MooTextViewClass;

struct MooTextView
{
    GtkTextView  parent;

    MooTextViewPrivate *priv;
};

struct MooTextViewClass
{
    GtkTextViewClass parent_class;

    gboolean (* char_inserted)      (MooTextView    *view,
                                     GtkTextIter    *where, /* points to position after the char */
                                     const char     *character); /* single character as string */

    gboolean (* line_mark_clicked)  (MooTextView    *view,
                                     int             line);

    /* these are made signals for convenience */
    void (* find_interactive)       (MooTextView    *view);
    void (* replace_interactive)    (MooTextView    *view);
    void (* find_next_interactive)  (MooTextView    *view);
    void (* find_prev_interactive)  (MooTextView    *view);
    void (* goto_line_interactive)  (MooTextView    *view);
    void (* find_word_at_cursor)    (MooTextView    *view,
                                     gboolean        forward);

    /* methods */
    /* adjusts start and end so that selection bound goes to start
       and insert goes to end,
       returns whether selection is not empty */
    gboolean (* extend_selection)   (MooTextView    *view,
                                     MooTextSelectionType type,
                                     GtkTextIter    *insert,
                                     GtkTextIter    *selection_bound);

    void (*apply_style_scheme)      (MooTextView    *view,
                                     MooTextStyleScheme *scheme);

    MooTextCursor (*get_text_cursor)(MooTextView    *view,
                                     int             x,  /* buffer coordinates */
                                     int             y); /* buffer coordinates */
};


GType        moo_text_view_get_type                 (void) G_GNUC_CONST;

void         moo_text_view_set_buffer_type          (MooTextView        *view,
                                                     GType               type);

void         moo_text_view_select_all               (MooTextView        *view);

char        *moo_text_view_get_selection            (GtkTextView        *view);
char        *moo_text_view_get_text                 (GtkTextView        *view);
gboolean     moo_text_view_has_selection            (MooTextView        *view);
gboolean     moo_text_view_has_text                 (MooTextView        *view);

gboolean     moo_text_view_can_redo                 (MooTextView        *view);
gboolean     moo_text_view_can_undo                 (MooTextView        *view);
gboolean     moo_text_view_redo                     (MooTextView        *view);
gboolean     moo_text_view_undo                     (MooTextView        *view);

void         moo_text_view_set_font_from_string     (MooTextView        *view,
                                                     const char         *font);

MooIndenter *moo_text_view_get_indenter             (MooTextView        *view);
void         moo_text_view_set_indenter             (MooTextView        *view,
                                                     MooIndenter        *indenter);

void         moo_text_view_get_cursor               (GtkTextView        *view,
                                                     GtkTextIter        *iter);
int          moo_text_view_get_cursor_line          (GtkTextView        *view);
void         moo_text_view_move_cursor              (MooTextView        *view,
                                                     int                 line,
                                                     int                 offset,
                                                     gboolean            offset_visual,
                                                     gboolean            in_idle);

void         moo_text_view_set_highlight_current_line
                                                    (MooTextView        *view,
                                                     gboolean            highlight);
void         moo_text_view_set_highlight_current_line_unfocused
                                                    (MooTextView        *view,
                                                     gboolean            highlight);
void         moo_text_view_set_current_line_color   (MooTextView        *view,
                                                     const char         *color);
void         moo_text_view_set_draw_right_margin    (MooTextView        *view,
                                                     gboolean            draw);
void         moo_text_view_set_right_margin_color   (MooTextView        *view,
                                                     const char         *color);
void         moo_text_view_set_right_margin_offset  (MooTextView        *view,
                                                     guint               offset);
MooTextStyleScheme *moo_text_view_get_style_scheme  (MooTextView        *view);
void         moo_text_view_set_style_scheme         (MooTextView        *view,
                                                     MooTextStyleScheme *scheme);

void         moo_text_view_set_show_line_numbers    (MooTextView        *view,
                                                     gboolean            show);
void         moo_text_view_set_tab_width            (MooTextView        *view,
                                                     guint               width);

GtkTextTag  *moo_text_view_lookup_tag               (MooTextView        *view,
                                                     const char         *name);

MooLang     *moo_text_view_get_lang                 (MooTextView        *view);
void         moo_text_view_set_lang                 (MooTextView        *view,
                                                     MooLang            *lang);
void         moo_text_view_set_lang_by_id           (MooTextView        *view,
                                                     const char         *id);

void         moo_text_view_set_word_chars           (MooTextView        *view,
                                                     const char         *word_chars);

void         moo_text_view_add_child_in_border      (MooTextView        *view,
                                                     GtkWidget          *child,
                                                     GtkTextWindowType   which_border);

void         moo_text_view_indent                   (MooTextView        *view);
void         moo_text_view_unindent                 (MooTextView        *view);


G_END_DECLS

#ifdef __cplusplus

MOO_DEFINE_GOBJ_TRAITS(MooTextView, MOO_TYPE_TEXT_VIEW);

#endif // __cplusplus
