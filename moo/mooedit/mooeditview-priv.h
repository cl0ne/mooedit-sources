#ifndef MOO_EDIT_VIEW_PRIV_H
#define MOO_EDIT_VIEW_PRIV_H

#include "mooedit/mooeditview-impl.h"

G_BEGIN_DECLS

struct MooEditViewPrivate
{
    MooEdit *doc;
    MooEditor *editor;
    MooEditTab *tab;
    GtkTextMark *fake_cursor_mark;
};

G_END_DECLS

#endif /* MOO_EDIT_VIEW_PRIV_H */
