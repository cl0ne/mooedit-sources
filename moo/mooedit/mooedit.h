/*
 *   mooedit.h
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

#include <mooedit/mooeditconfig.h>
#include <mooedit/mooedit-enums.h>
#include <mooedit/mooedittypes.h>
#include <mooedit/mooeditfileinfo.h>
#include <mooutils/mooprefs.h>

G_BEGIN_DECLS


#define MOO_TYPE_EDIT                       (moo_edit_get_type ())
#define MOO_EDIT(object)                    (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_EDIT, MooEdit))
#define MOO_EDIT_CLASS(klass)               (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_EDIT, MooEditClass))
#define MOO_IS_EDIT(object)                 (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_EDIT))
#define MOO_IS_EDIT_CLASS(klass)            (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_EDIT))
#define MOO_EDIT_GET_CLASS(obj)             (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_EDIT, MooEditClass))

typedef struct MooEditPrivate  MooEditPrivate;
typedef struct MooEditClass    MooEditClass;

struct MooEdit
{
    GObject parent;
    MooEditConfig *config;
    MooEditPrivate *priv;
};

struct MooEditClass
{
    GObjectClass parent_class;

    /* emitted when filename, modified status, or file on disk
       are changed. for use in editor to adjust title bar, etc. */
    void (* doc_status_changed) (MooEdit    *edit);

    void (* filename_changed)   (MooEdit    *edit);

    /**signal:MooEdit**/
    void                (* will_close)  (MooEdit *doc);

    /**signal:MooEdit**/
    MooSaveResponse     (* before_save) (MooEdit *doc,
                                         GFile   *file);
    /**signal:MooEdit**/
    void                (* will_save)   (MooEdit *doc,
                                         GFile   *file);
    /**signal:MooEdit**/
    void                (* after_save)  (MooEdit *doc);
};


GType                moo_edit_get_type                  (void) G_GNUC_CONST;

MooEditor           *moo_edit_get_editor                (MooEdit            *doc);
MooEditWindow       *moo_edit_get_window                (MooEdit            *doc);
MooEditTab          *moo_edit_get_tab                   (MooEdit            *doc);
MooEditViewArray    *moo_edit_get_views                 (MooEdit            *doc);
MooEditView         *moo_edit_get_view                  (MooEdit            *doc);
int                  moo_edit_get_n_views               (MooEdit            *doc);

GtkTextBuffer       *moo_edit_get_buffer                (MooEdit            *doc);

GFile               *moo_edit_get_file                  (MooEdit            *edit);

char                *moo_edit_get_filename              (MooEdit            *edit);
char                *moo_edit_get_uri                   (MooEdit            *edit);
const char          *moo_edit_get_display_name          (MooEdit            *edit);
const char          *moo_edit_get_display_basename      (MooEdit            *edit);

const char          *moo_edit_get_encoding              (MooEdit            *edit);
void                 moo_edit_set_encoding              (MooEdit            *edit,
                                                         const char         *encoding);

char                *moo_edit_get_lang_id               (MooEdit            *edit);

MooLineEndType       moo_edit_get_line_end_type         (MooEdit            *edit);
void                 moo_edit_set_line_end_type         (MooEdit            *edit,
                                                         MooLineEndType      le);

gboolean             moo_edit_is_empty                  (MooEdit            *edit);
gboolean             moo_edit_is_untitled               (MooEdit            *edit);
gboolean             moo_edit_is_modified               (MooEdit            *edit);
void                 moo_edit_set_modified              (MooEdit            *edit,
                                                         gboolean            modified);
gboolean             moo_edit_get_clean                 (MooEdit            *edit);
void                 moo_edit_set_clean                 (MooEdit            *edit,
                                                         gboolean            clean);
MooEditStatus        moo_edit_get_status                (MooEdit            *edit);
MooEditState         moo_edit_get_state                 (MooEdit            *edit);

gboolean             moo_edit_reload                    (MooEdit            *edit,
                                                         MooReloadInfo      *info,
                                                         GError            **error);
gboolean             moo_edit_save                      (MooEdit            *edit,
                                                         GError            **error);
gboolean             moo_edit_save_as                   (MooEdit            *edit,
                                                         MooSaveInfo        *info,
                                                         GError            **error);
gboolean             moo_edit_save_copy                 (MooEdit            *edit,
                                                         MooSaveInfo        *info,
                                                         GError            **error);
gboolean             moo_edit_close                     (MooEdit            *edit);

void                 moo_edit_comment_selection         (MooEdit            *edit);
void                 moo_edit_uncomment_selection       (MooEdit            *edit);


G_END_DECLS

#ifdef __cplusplus

#include <moocpp/gstr.h>

MOO_DEFINE_GOBJ_TRAITS(MooEdit, MOO_TYPE_EDIT);

#endif // __cplusplus
