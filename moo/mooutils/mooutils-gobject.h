/*
 *   mooutils-gobject.h
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

#ifndef MOO_UTILS_GOBJECT_H
#define MOO_UTILS_GOBJECT_H

#include <gtk/gtk.h>
#include <mooutils/mooclosure.h>

G_BEGIN_DECLS


/*****************************************************************************/
/* GType type
 */

#define MOO_TYPE_GTYPE (_moo_gtype_get_type())
GType           _moo_gtype_get_type         (void) G_GNUC_CONST;
GType           _moo_value_get_gtype        (const GValue   *value);


/*****************************************************************************/
/* Converting values forth and back
 */

gboolean        _moo_value_type_supported   (GType           type);
gboolean        _moo_value_convert           (const GValue   *src,
                                             GValue         *dest);
gboolean        _moo_value_equal             (const GValue   *a,
                                             const GValue   *b);

gboolean        _moo_value_change_type       (GValue         *val,
                                             GType           new_type);

double          _moo_value_convert_to_double (const GValue   *val);
const char     *_moo_value_convert_to_string (const GValue   *val);
gboolean        _moo_value_convert_from_string (const char   *string,
                                               GValue       *val);

gboolean        _moo_convert_string_to_bool (const char     *string,
                                             gboolean        default_val);
int             _moo_convert_string_to_int  (const char     *string,
                                             int             default_val);
guint           _moo_convert_string_to_uint (const char     *string,
                                             guint           default_val);
const char     *_moo_convert_bool_to_string (gboolean        value);
const char     *_moo_convert_int_to_string  (int             value);


/*****************************************************************************/
/* GParameter array manipulation
 */

void            _moo_param_array_free       (GParameter *array,
                                             guint       len);


/*****************************************************************************/
/* Signal that does not require class method
 */

guint _moo_signal_new_cb (const gchar        *signal_name,
                          GType               itype,
                          GSignalFlags        signal_flags,
                          GCallback           handler,
                          GSignalAccumulator  accumulator,
                          gpointer            accu_data,
                          GSignalCMarshaller  c_marshaller,
                          GType               return_type,
                          guint               n_params,
                          ...);


/*****************************************************************************/
/* Property watch
 */

typedef struct _MooObjectWatch MooObjectWatch;
typedef struct _MooObjectWatchClass MooObjectWatchClass;
typedef void (*MooObjectWatchNotify) (MooObjectWatch *watch);

struct _MooObjectWatch {
    MooObjectWatchClass *klass;
    MooObjectPtr *source;
    MooObjectPtr *target;
    GDestroyNotify notify;
    gpointer notify_data;
    guint id;
};

struct _MooObjectWatchClass {
    MooObjectWatchNotify source_notify;
    MooObjectWatchNotify target_notify;
    MooObjectWatchNotify destroy;
};

MooObjectWatch *_moo_object_watch_alloc (gsize                size,
                                         MooObjectWatchClass *klass,
                                         gpointer             source,
                                         gpointer             target,
                                         GDestroyNotify       notify,
                                         gpointer             notify_data);
#define _moo_object_watch_new(Type_,klass_,src_,tgt_,notify_,data_) \
    ((Type_*) _moo_object_watch_alloc (sizeof (Type_), klass_, src_, tgt_, notify_, data_))

void        moo_bind_sensitive      (GtkWidget          *toggle_btn,
                                     GtkWidget          *dependent,
                                     gboolean            invert);

guint       moo_bind_bool_property  (gpointer            target,
                                     const char         *target_prop,
                                     gpointer            source,
                                     const char         *source_prop,
                                     gboolean            invert);


/*****************************************************************************/
/* Data store
 */

typedef struct _MooData MooData;

MooData *_moo_data_new              (GHashFunc       hash_func,
                                     GEqualFunc      key_equal_func,
                                     GDestroyNotify  key_destroy_func);

/* these accept NULL */
void     _moo_data_unref            (MooData        *data);
#define  _moo_data_destroy  _moo_data_unref

void     _moo_data_insert_value     (MooData        *data,
                                     gpointer        key,
                                     const GValue   *value);
void     _moo_data_insert_ptr       (MooData        *data,
                                     gpointer        key,
                                     gpointer        value,
                                     GDestroyNotify  destroy);

void     _moo_data_remove           (MooData        *data,
                                     gpointer        key);
void     _moo_data_clear            (MooData        *data);

/* dest must not be initialized */
gboolean _moo_data_get_value        (MooData        *data,
                                     gpointer        key,
                                     GValue         *dest);
gpointer _moo_data_get_ptr          (MooData        *data,
                                     gpointer        key);


G_END_DECLS

#endif /* MOO_UTILS_GOBJECT_H */
