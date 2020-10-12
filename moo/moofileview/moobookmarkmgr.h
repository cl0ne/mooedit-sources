/*
 *   moobookmarkmgr.h
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

#ifndef MOO_BOOKMARK_MGR_H
#define MOO_BOOKMARK_MGR_H

#include <gtk/gtk.h>
#include <mooutils/moouixml.h>

G_BEGIN_DECLS


#define MOO_TYPE_BOOKMARK_MGR                (_moo_bookmark_mgr_get_type ())
#define MOO_BOOKMARK_MGR(object)             (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_BOOKMARK_MGR, MooBookmarkMgr))
#define MOO_BOOKMARK_MGR_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_BOOKMARK_MGR, MooBookmarkMgrClass))
#define MOO_IS_BOOKMARK_MGR(object)          (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_BOOKMARK_MGR))
#define MOO_IS_BOOKMARK_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_BOOKMARK_MGR))
#define MOO_BOOKMARK_MGR_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_BOOKMARK_MGR, MooBookmarkMgrClass))

#define MOO_TYPE_BOOKMARK                    (_moo_bookmark_get_type ())

typedef enum {
    MOO_BOOKMARK_MGR_COLUMN_BOOKMARK = 0,
    MOO_BOOKMARK_MGR_NUM_COLUMNS
} MooBookmarkMgrModelColumn;

typedef struct _MooBookmark             MooBookmark;
typedef struct _MooBookmarkMgr          MooBookmarkMgr;
typedef struct _MooBookmarkMgrPrivate   MooBookmarkMgrPrivate;
typedef struct _MooBookmarkMgrClass     MooBookmarkMgrClass;

struct _MooBookmark {
    char *path;
    char *display_path;
    char *label;
    char *icon_stock_id;
    GdkPixbuf *pixbuf;
};

struct _MooBookmarkMgr
{
    GObject parent;
    MooBookmarkMgrPrivate *priv;
};

struct _MooBookmarkMgrClass
{
    GObjectClass parent_class;

    void (*changed)   (MooBookmarkMgr *mgr);

    void (*activate)  (MooBookmarkMgr *mgr,
                       MooBookmark    *bookmark,
                       GObject        *user);
};


GType           _moo_bookmark_get_type      (void) G_GNUC_CONST;
GType           _moo_bookmark_mgr_get_type  (void) G_GNUC_CONST;

MooBookmark    *_moo_bookmark_new           (const char     *name,
                                             const char     *path,
                                             const char     *icon);
void            _moo_bookmark_free          (MooBookmark    *bookmark);

MooBookmarkMgr *_moo_bookmark_mgr_new       (void);
GtkTreeModel   *_moo_bookmark_mgr_get_model (MooBookmarkMgr *mgr);

void            _moo_bookmark_mgr_add       (MooBookmarkMgr *mgr,
                                             MooBookmark    *bookmark);

GtkWidget      *_moo_bookmark_mgr_get_editor(MooBookmarkMgr *mgr);

void            _moo_bookmark_mgr_add_user  (MooBookmarkMgr *mgr,
                                             gpointer        user, /* GObject* */
                                             MooActionCollection *actions,
                                             MooUiXml       *xml,
                                             const char     *path);
void            _moo_bookmark_mgr_remove_user(MooBookmarkMgr *mgr,
                                             gpointer        user); /* GObject* */


G_END_DECLS

#endif /* MOO_BOOKMARK_MGR_H */
