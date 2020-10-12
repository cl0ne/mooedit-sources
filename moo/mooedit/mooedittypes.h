#ifndef MOO_EDIT_TYPES_H
#define MOO_EDIT_TYPES_H

#include <gtk/gtk.h>
#include <mooutils/mooarray.h>
#include <mooutils/moolist.h>
#include <mooutils/mootype-macros.h>
#include <mooedit/mooedit-enums.h>

G_BEGIN_DECLS

typedef struct MooOpenInfo MooOpenInfo;
typedef struct MooSaveInfo MooSaveInfo;
typedef struct MooReloadInfo MooReloadInfo;

typedef struct MooEdit MooEdit;
typedef struct MooEditView MooEditView;
typedef struct MooEditWindow MooEditWindow;
typedef struct MooEditor MooEditor;
typedef struct MooEditTab MooEditTab;

MOO_DECLARE_OBJECT_ARRAY (MooEdit, moo_edit)
MOO_DECLARE_OBJECT_ARRAY (MooEditView, moo_edit_view)
MOO_DECLARE_OBJECT_ARRAY (MooEditTab, moo_edit_tab)
MOO_DECLARE_OBJECT_ARRAY (MooEditWindow, moo_edit_window)
MOO_DEFINE_SLIST (MooEditList, moo_edit_list, MooEdit)

MOO_DECLARE_OBJECT_ARRAY (MooOpenInfo, moo_open_info)

#define MOO_TYPE_LINE_END (moo_type_line_end ())
GType   moo_type_line_end   (void) G_GNUC_CONST;

#define MOO_EDIT_RELOAD_ERROR (moo_edit_reload_error_quark ())
#define MOO_EDIT_SAVE_ERROR (moo_edit_save_error_quark ())

MOO_DECLARE_QUARK (moo-edit-reload-error, moo_edit_reload_error_quark)
MOO_DECLARE_QUARK (moo-edit-save-error, moo_edit_save_error_quark)

G_END_DECLS

#endif /* MOO_EDIT_TYPES_H */
