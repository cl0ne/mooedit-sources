#ifndef MOO_EDIT_SCRIPT_H
#define MOO_EDIT_SCRIPT_H

#include <mooedit/mooedit.h>

G_BEGIN_DECLS

gboolean     moo_edit_can_undo                  (MooEdit            *doc);
gboolean     moo_edit_can_redo                  (MooEdit            *doc);
gboolean     moo_edit_undo                      (MooEdit            *doc);
gboolean     moo_edit_redo                      (MooEdit            *doc);
void         moo_edit_begin_user_action         (MooEdit            *doc);
void         moo_edit_end_user_action           (MooEdit            *doc);
void         moo_edit_begin_non_undoable_action (MooEdit            *doc);
void         moo_edit_end_non_undoable_action   (MooEdit            *doc);

GtkTextIter *moo_edit_get_start_pos             (MooEdit            *doc);
GtkTextIter *moo_edit_get_end_pos               (MooEdit            *doc);
GtkTextIter *moo_edit_get_cursor_pos            (MooEdit            *doc);
void         moo_edit_set_cursor_pos            (MooEdit            *doc,
                                                 const GtkTextIter  *pos);
GtkTextIter *moo_edit_get_selection_start_pos   (MooEdit            *doc);
GtkTextIter *moo_edit_get_selection_end_pos     (MooEdit            *doc);

int          moo_edit_get_char_count            (MooEdit            *doc);
int          moo_edit_get_line_count            (MooEdit            *doc);

int          moo_edit_get_line_at_cursor        (MooEdit            *doc);
int          moo_edit_get_line_at_pos           (MooEdit            *doc,
                                                 const GtkTextIter  *pos);
GtkTextIter *moo_edit_get_pos_at_line           (MooEdit            *doc,
                                                 int                 line);
GtkTextIter *moo_edit_get_pos_at_line_end       (MooEdit            *doc,
                                                 int                 line);
gunichar     moo_edit_get_char_at_pos           (MooEdit            *doc,
                                                 const GtkTextIter  *pos);

char        *moo_edit_get_text                  (MooEdit            *doc,
                                                 const GtkTextIter  *start,
                                                 const GtkTextIter  *end);
char        *moo_edit_get_line_text             (MooEdit            *doc,
                                                 int                 line);
char        *moo_edit_get_line_text_at_pos      (MooEdit            *doc,
                                                 const GtkTextIter  *pos);

void         moo_edit_set_text                  (MooEdit            *doc,
                                                 const char         *text);
void         moo_edit_insert_text               (MooEdit            *doc,
                                                 const char         *text,
                                                 GtkTextIter        *where);
void         moo_edit_replace_text              (MooEdit            *doc,
                                                 GtkTextIter        *start,
                                                 GtkTextIter        *end,
                                                 const char         *text);
void         moo_edit_delete_text               (MooEdit            *doc,
                                                 GtkTextIter        *start,
                                                 GtkTextIter        *end);
void         moo_edit_append_text               (MooEdit            *doc,
                                                 const char         *text);
void         moo_edit_clear                     (MooEdit            *doc);

void         moo_edit_cut                       (MooEdit            *doc);
void         moo_edit_copy                      (MooEdit            *doc);
void         moo_edit_paste                     (MooEdit            *doc);

void         moo_edit_select_range              (MooEdit            *doc,
                                                 const GtkTextIter  *start,
                                                 const GtkTextIter  *end);
void         moo_edit_select_lines              (MooEdit            *doc,
                                                 int                 start,
                                                 int                 end);
void         moo_edit_select_lines_at_pos       (MooEdit            *doc,
                                                 const GtkTextIter  *start,
                                                 const GtkTextIter  *end);
void         moo_edit_select_all                (MooEdit            *doc);

char        *moo_edit_get_selected_text         (MooEdit            *doc);
char       **moo_edit_get_selected_lines        (MooEdit            *doc);
void         moo_edit_delete_selected_text      (MooEdit            *doc);
void         moo_edit_delete_selected_lines     (MooEdit            *doc);
void         moo_edit_replace_selected_text     (MooEdit            *doc,
                                                 const char         *replacement);
void         moo_edit_replace_selected_lines    (MooEdit            *doc,
                                                 char              **replacement);

gboolean     moo_edit_has_selection             (MooEdit        *doc);

G_END_DECLS

#endif /* MOO_EDIT_SCRIPT_H */
