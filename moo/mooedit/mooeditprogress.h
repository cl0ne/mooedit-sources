#ifndef MOO_EDIT_PROGRESS_H
#define MOO_EDIT_PROGRESS_H

#include "mooedit.h"
#include "mooeditwindow.h"

G_BEGIN_DECLS

typedef struct MooEditProgress MooEditProgress;

MooEditProgress *_moo_edit_progress_new             (void);
void             _moo_edit_progress_start           (MooEditProgress    *progress,
                                                     const char         *text,
                                                     GDestroyNotify      cancel_func,
                                                     gpointer            cancel_func_data);
void             _moo_edit_progress_set_cancel_func (MooEditProgress    *progress,
                                                     GDestroyNotify      cancel_func,
                                                     gpointer            cancel_func_data);
void             _moo_edit_progress_set_text        (MooEditProgress    *progress,
                                                     const char         *text);

G_END_DECLS

#endif /* MOO_EDIT_PROGRESS_H */
