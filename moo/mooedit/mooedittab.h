#ifndef MOO_EDIT_TAB_H
#define MOO_EDIT_TAB_H

#include <mooedit/mooedittypes.h>

G_BEGIN_DECLS

MooEdit             *moo_edit_tab_get_doc                   (MooEditTab     *tab);
MooEditViewArray    *moo_edit_tab_get_views                 (MooEditTab     *tab);
MooEditView         *moo_edit_tab_get_active_view           (MooEditTab     *tab);
MooEditWindow       *moo_edit_tab_get_window                (MooEditTab     *tab);

G_END_DECLS

#endif /* MOO_EDIT_TAB_H */
