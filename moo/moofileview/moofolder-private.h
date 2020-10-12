/*
 *   moofolder-private.h
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

#ifndef MOO_FOLDER_PRIVATE_H
#define MOO_FOLDER_PRIVATE_H

#include "moofileview/moofolder.h"
#include "moofileview/moofile-private.h"
#include "moofileview/moofilesystem.h"

G_BEGIN_DECLS


#define MOO_FOLDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FOLDER, MooFolderClass))
#define MOO_IS_FOLDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FOLDER))
#define MOO_FOLDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FOLDER, MooFolderClass))

typedef struct _MooFolderImpl  MooFolderImpl;
typedef struct _MooFolderClass MooFolderClass;

struct _MooFolder
{
    GObject        parent;
    MooFolderImpl *impl;
};

struct _MooFolderClass
{
    GObjectClass    parent_class;

    void     (*deleted)         (MooFolder  *folder);
    void     (*files_added)     (MooFolder  *folder,
                                 GSList     *files);
    void     (*files_changed)   (MooFolder  *folder,
                                 GSList     *files);
    void     (*files_removed)   (MooFolder  *folder,
                                 GSList     *files);
};

typedef struct {
    double names_timer;
    double stat_timer;
    guint stat_counter;
    double icons_timer;
    guint icons_counter;
} Debug;

typedef enum {
    STAGE_NAMES     = 1,
    STAGE_STAT      = 2,
    STAGE_MIME_TYPE = 3
} Stage;

struct _MooFolderImpl {
    MooFolder *proxy;
    Stage done;
    Stage wanted;
    Stage wanted_bg;
    MooFileSystem *fs;
    GDir *dir;
    GHashTable *files; /* basename -> MooFile* */
    GSList *files_copy;
    char *path;
    GSourceFunc populate_func;
    int populate_priority;
    guint populate_idle_id;
    double populate_timeout;
    Debug debug;
    GTimer *timer;
    MooFileWatch *fam;
    guint fam_request;
    guint reload_idle;

    guint deleted : 1;
};


MooFolder   *_moo_folder_new                    (MooFileSystem  *fs,
                                                 const char     *path,
                                                 MooFileFlags    wanted,
                                                 GError        **error);
MooFolder   *_moo_folder_new_with_impl          (MooFolderImpl  *impl);
void         _moo_folder_set_wanted             (MooFolder      *folder,
                                                 MooFileFlags    wanted,
                                                 gboolean        bit_now);
void         _moo_folder_impl_free              (MooFolderImpl  *impl);

void         _moo_file_system_folder_finalized  (MooFileSystem  *fs,
                                                 MooFolder      *folder);
void         _moo_file_system_folder_deleted    (MooFileSystem  *fs,
                                                 MooFolderImpl  *folder);

gsize        _moo_folder_mem_usage              (MooFolder      *folder);


G_END_DECLS

#endif /* MOO_FOLDER_PRIVATE_H */
