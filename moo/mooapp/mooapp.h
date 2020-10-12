/*
 *   mooapp/mooapp.h
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

#ifndef MOO_APP_H
#define MOO_APP_H

#include <mooedit/mooeditor.h>

G_BEGIN_DECLS


#define MOO_TYPE_APP                (moo_app_get_type ())
#define MOO_APP(object)             (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_APP, MooApp))
#define MOO_APP_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_APP, MooAppClass))
#define MOO_IS_APP(object)          (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_APP))
#define MOO_IS_APP_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_APP))
#define MOO_APP_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_APP, MooAppClass))


typedef struct _MooApp        MooApp;
typedef struct _MooAppPrivate MooAppPrivate;
typedef struct _MooAppClass   MooAppClass;

struct _MooApp
{
    GObject          parent;
    MooAppPrivate   *priv;
};

struct _MooAppClass
{
    GObjectClass parent_class;

    /**signal:MooApp**/
    void        (*started)      (MooApp         *app);
    /**signal:MooApp**/
    void        (*quit)         (MooApp         *app);

    /**signal:MooApp**/
    void        (*load_session) (MooApp         *app);
    /**signal:MooApp**/
    void        (*save_session) (MooApp         *app);

    void        (*init_plugins) (MooApp         *app);
};


GType            moo_app_get_type               (void) G_GNUC_CONST;

MooApp          *moo_app_instance               (void);

gboolean         moo_app_init                   (MooApp                 *app);
int              moo_app_run                    (MooApp                 *app);
gboolean         moo_app_quit                   (MooApp                 *app);

void             moo_app_set_exit_status        (MooApp                 *app,
                                                 int                     value);

void             moo_app_load_session           (MooApp                 *app);

MooEditor       *moo_app_get_editor             (MooApp                 *app);

void             moo_app_prefs_dialog           (GtkWidget              *parent);
void             moo_app_about_dialog           (GtkWidget              *parent);

char            *moo_app_get_system_info        (MooApp                 *app);

MooUiXml        *moo_app_get_ui_xml             (MooApp                 *app);
void             moo_app_set_ui_xml             (MooApp                 *app,
                                                 MooUiXml               *xml);

gboolean         moo_app_send_msg               (const char             *pid,
                                                 const char             *data,
                                                 gssize                  len);

gboolean         moo_app_send_files             (MooOpenInfoArray       *files,
                                                 guint32                 stamp,
                                                 const char             *pid);
void             moo_app_open_files             (MooApp                 *app,
                                                 MooOpenInfoArray       *files,
                                                 guint32                 stamp);
void             moo_app_run_script             (MooApp                 *app,
                                                 const char             *script);


G_END_DECLS

#endif /* MOO_APP_H */
