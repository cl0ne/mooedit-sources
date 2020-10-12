/*
 *   mooutils-gobject-private.h
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

#ifndef MOO_UTILS_GOBJECT_PRIVATE_H
#define MOO_UTILS_GOBJECT_PRIVATE_H

#include <mooutils/mooutils-gobject.h>

G_BEGIN_DECLS


/*****************************************************************************/
/* GType type
 */

#define MOO_TYPE_PARAM_GTYPE            (_moo_param_gtype_get_type())
#define MOO_IS_PARAM_SPEC_GTYPE(pspec)  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), MOO_TYPE_PARAM_GTYPE))
#define MOO_PARAM_SPEC_GTYPE(pspec)     (G_TYPE_CHECK_INSTANCE_CAST ((pspec), MOO_TYPE_PARAM_GTYPE, MooParamSpecGType))

typedef struct _MooParamSpecGType MooParamSpecGType;

struct _MooParamSpecGType
{
    GParamSpec parent;
    GType base;
};

GType           _moo_param_gtype_get_type   (void) G_GNUC_CONST;

/*****************************************************************************/
/* Property watch
 */

guint       _moo_bind_signal                (gpointer        target,
                                             const char     *target_signal,
                                             gpointer        source,
                                             const char     *source_signal);

typedef void (*MooTransformPropFunc)        (GValue         *target,
                                             const GValue   *source,
                                             gpointer        data);
typedef void (*MooGetPropFunc)              (GObject        *source,
                                             GValue         *target,
                                             gpointer        data);

guint       _moo_add_property_watch         (gpointer        target,
                                             const char     *target_prop,
                                             gpointer        source,
                                             const char     *source_prop,
                                             MooTransformPropFunc transform,
                                             gpointer        transform_data,
                                             GDestroyNotify  destroy_notify);


/*****************************************************************************/
/* Data store
 */

#define MOO_TYPE_PTR    (_moo_ptr_get_type ())
#define MOO_TYPE_DATA   (_moo_data_get_type ())

typedef struct _MooPtr MooPtr;

struct _MooPtr {
    guint ref_count;
    gpointer data;
    GDestroyNotify free_func;
};

GType    _moo_ptr_get_type          (void) G_GNUC_CONST;
GType    _moo_data_get_type         (void) G_GNUC_CONST;



G_END_DECLS

#endif /* MOO_UTILS_GOBJECT_PRIVATE_H */
