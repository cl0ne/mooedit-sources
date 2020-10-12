/*
 *   mooeditor.h
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

#ifndef MOO_EDITOR_H
#define MOO_EDITOR_H

#include <mooedit/mooedit.h>
#include <mooedit/mooeditview.h>
#include <mooedit/mooeditwindow.h>
#include <mooedit/mooeditfileinfo.h>
#include <mooutils/moouixml.h>

G_BEGIN_DECLS

enum {
    MOO_EDIT_RELOAD_ERROR_BUSY = 1,
    MOO_EDIT_RELOAD_ERROR_UNTITLED,
    MOO_EDIT_RELOAD_ERROR_CANCELLED
};

enum {
    MOO_EDIT_SAVE_ERROR_BUSY = 1,
    MOO_EDIT_SAVE_ERROR_CANCELLED
};

#define MOO_TYPE_EDITOR                 (moo_editor_get_type ())
#define MOO_EDITOR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_EDITOR, MooEditor))
#define MOO_EDITOR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_EDITOR, MooEditorClass))
#define MOO_IS_EDITOR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_EDITOR))
#define MOO_IS_EDITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_EDITOR))
#define MOO_EDITOR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_EDITOR, MooEditorClass))

typedef struct MooEditorPrivate MooEditorPrivate;
typedef struct MooEditorClass   MooEditorClass;

struct MooEditor
{
    GObject base;
    MooEditorPrivate *priv;
};

struct MooEditorClass
{
    GObjectClass base_class;

    MooCloseResponse     (*before_close_window) (MooEditor     *editor,
                                                 MooEditWindow *window);
    /**signal:MooEditor**/
    void                 (*will_close_window)   (MooEditor     *editor,
                                                 MooEditWindow *window);
    void                 (*after_close_window)  (MooEditor     *editor);

    /**signal:MooEditor**/
    MooCloseResponse     (*will_close_doc)      (MooEditor     *editor,
                                                 MooEdit       *doc);

    /**signal:MooEditor**/
    MooSaveResponse      (*before_save)  (MooEditor     *editor,
                                          MooEdit       *doc,
                                          GFile         *file);
    /**signal:MooEditor**/
    void                 (*will_save)    (MooEditor     *editor,
                                          MooEdit       *doc,
                                          GFile         *file);
    /**signal:MooEditor**/
    void                 (*after_save)   (MooEditor     *editor,
                                          MooEdit       *doc);
};


GType                moo_editor_get_type            (void) G_GNUC_CONST;

MooEditor           *moo_editor_instance            (void);
MooEditor           *moo_editor_create_instance     (gboolean                embedded);

MooEditWindow       *moo_editor_new_window          (MooEditor              *editor);
MooEdit             *moo_editor_new_doc             (MooEditor              *editor,
                                                     MooEditWindow          *window);
MooEdit             *moo_editor_create_doc          (MooEditor              *editor,
                                                     const char             *path,
                                                     const char             *encoding,
                                                     GError                **error);

MooEdit             *moo_editor_new_file            (MooEditor              *editor,
                                                     MooOpenInfo            *info,
                                                     GtkWidget              *parent,
                                                     GError                **error);
MooEdit             *moo_editor_open_file           (MooEditor              *editor,
                                                     MooOpenInfo            *info,
                                                     GtkWidget              *parent,
                                                     GError                **error);
gboolean             moo_editor_open_files          (MooEditor              *editor,
                                                     MooOpenInfoArray       *files,
                                                     GtkWidget              *parent,
                                                     GError                **error);

MooEdit             *moo_editor_open_uri            (MooEditor              *editor,
                                                     const char             *uri,
                                                     const char             *encoding,
                                                     int                     line,
                                                     MooEditWindow          *window);
MooEdit             *moo_editor_open_path           (MooEditor              *editor,
                                                     const char             *path,
                                                     const char             *encoding,
                                                     int                     line,
                                                     MooEditWindow          *window);

gboolean             moo_editor_reload              (MooEditor              *editor,
                                                     MooEdit                *doc,
                                                     MooReloadInfo          *info,
                                                     GError                **error);

gboolean             moo_editor_save                (MooEditor              *editor,
                                                     MooEdit                *doc,
                                                     GError                **error);
gboolean             moo_editor_save_as             (MooEditor              *editor,
                                                     MooEdit                *doc,
                                                     MooSaveInfo            *info,
                                                     GError                **error);
gboolean             moo_editor_save_copy           (MooEditor              *editor,
                                                     MooEdit                *doc,
                                                     MooSaveInfo            *info,
                                                     GError                **error);

MooEdit             *moo_editor_get_doc             (MooEditor              *editor,
                                                     const char             *filename);
MooEdit             *moo_editor_get_doc_for_file    (MooEditor              *editor,
                                                     GFile                  *file);
MooEdit             *moo_editor_get_doc_for_uri     (MooEditor              *editor,
                                                     const char             *uri);

MooEdit             *moo_editor_get_active_doc      (MooEditor              *editor);
void                 moo_editor_set_active_doc      (MooEditor              *editor,
                                                     MooEdit                *doc);
MooEditView         *moo_editor_get_active_view     (MooEditor              *editor);
void                 moo_editor_set_active_view     (MooEditor              *editor,
                                                     MooEditView            *view);
MooEditWindow       *moo_editor_get_active_window   (MooEditor              *editor);
void                 moo_editor_set_active_window   (MooEditor              *editor,
                                                     MooEditWindow          *window);

void                 moo_editor_present             (MooEditor              *editor,
                                                     guint32                 stamp);

MooEditWindowArray  *moo_editor_get_windows         (MooEditor              *editor);
MooEditArray        *moo_editor_get_docs            (MooEditor              *editor);

gboolean             moo_editor_close_window        (MooEditor              *editor,
                                                     MooEditWindow          *window);
gboolean             moo_editor_close_doc           (MooEditor              *editor,
                                                     MooEdit                *doc);
gboolean             moo_editor_close_docs          (MooEditor              *editor,
                                                     MooEditArray           *docs);
gboolean            _moo_editor_close_all           (MooEditor              *editor);

MooUiXml            *moo_editor_get_doc_ui_xml      (MooEditor              *editor);
MooUiXml            *moo_editor_get_ui_xml          (MooEditor              *editor);
void                 moo_editor_set_ui_xml          (MooEditor              *editor,
                                                     MooUiXml               *xml);

void                 moo_editor_set_window_type     (MooEditor              *editor,
                                                     GType                   type);
void                 moo_editor_set_doc_type        (MooEditor              *editor,
                                                     GType                   type);

void                _moo_editor_load_session        (MooEditor              *editor,
                                                     MooMarkupNode          *xml);
void                _moo_editor_save_session        (MooEditor              *editor,
                                                     MooMarkupNode          *xml);


G_END_DECLS

#endif /* MOO_EDITOR_H */
