#ifndef MOO_EDIT_TAB_IMPL_H
#define MOO_EDIT_TAB_IMPL_H

#include "mooedittab.h"
#include "mooeditprogress.h"

G_BEGIN_DECLS

MooEditTab      *_moo_edit_tab_new                  (MooEdit        *doc);

MooEditProgress *_moo_edit_tab_create_progress      (MooEditTab     *tab);
void             _moo_edit_tab_destroy_progress     (MooEditTab     *tab);

void             _moo_edit_tab_focus_next_view      (MooEditTab     *tab);
void             _moo_edit_tab_set_focused_view     (MooEditTab     *tab,
                                                     MooEditView    *view);

gboolean         _moo_edit_tab_set_split_horizontal (MooEditTab     *tab,
                                                     gboolean        split);
gboolean         _moo_edit_tab_set_split_vertical   (MooEditTab     *tab,
                                                     gboolean        split);
gboolean         _moo_edit_tab_get_split_horizontal (MooEditTab     *tab);
gboolean         _moo_edit_tab_get_split_vertical   (MooEditTab     *tab);

G_END_DECLS

#endif /* MOO_EDIT_TAB_IMPL_H */
