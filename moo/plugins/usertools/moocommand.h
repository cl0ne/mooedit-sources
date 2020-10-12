/*
 *   moocommand.h
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

#include <gtk/gtk.h>
#include "../support/moooutputfilter.h"
#include "moousertools-enums.h"

G_BEGIN_DECLS


#define MOO_TYPE_COMMAND                    (moo_command_get_type ())
#define MOO_COMMAND(object)                 (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_COMMAND, MooCommand))
#define MOO_COMMAND_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_COMMAND, MooCommandClass))
#define MOO_IS_COMMAND(object)              (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_COMMAND))
#define MOO_IS_COMMAND_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_COMMAND))
#define MOO_COMMAND_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_COMMAND, MooCommandClass))

#define MOO_TYPE_COMMAND_CONTEXT            (moo_command_context_get_type ())
#define MOO_COMMAND_CONTEXT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_COMMAND_CONTEXT, MooCommandContext))
#define MOO_COMMAND_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_COMMAND_CONTEXT, MooCommandContextClass))
#define MOO_IS_COMMAND_CONTEXT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_COMMAND_CONTEXT))
#define MOO_IS_COMMAND_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_COMMAND_CONTEXT))
#define MOO_COMMAND_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_COMMAND_CONTEXT, MooCommandContextClass))

#define MOO_TYPE_COMMAND_FACTORY            (moo_command_factory_get_type ())
#define MOO_COMMAND_FACTORY(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_COMMAND_FACTORY, MooCommandFactory))
#define MOO_COMMAND_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_COMMAND_FACTORY, MooCommandFactoryClass))
#define MOO_IS_COMMAND_FACTORY(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_COMMAND_FACTORY))
#define MOO_IS_COMMAND_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_COMMAND_FACTORY))
#define MOO_COMMAND_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_COMMAND_FACTORY, MooCommandFactoryClass))

#define MOO_TYPE_COMMAND_DATA               (moo_command_data_get_type ())

typedef struct _MooCommandContextPrivate    MooCommandContextPrivate;
typedef struct _MooCommandData              MooCommandData;

struct MooCommandContext : public GObject
{
    MooCommandContextPrivate *priv;
};

struct MooCommandContextClass : public GObjectClass
{
};

struct MooCommand : public GObject
{
    /* read-only */
    MooCommandOptions options;
};

struct MooCommandClass : public GObjectClass
{
    gboolean    (*check_sensitive)  (MooCommand         *cmd,
                                     MooEdit            *doc);
    void        (*run)              (MooCommand         *cmd,
                                     MooCommandContext  *ctx);
};

struct MooCommandFactory : public GObject
{
    char *name;
    char *display_name;
    char **keys;
    guint n_keys;
    char *extension;
};

struct MooCommandFactoryClass : public GObjectClass
{
    MooCommand *(*create_command) (MooCommandFactory *factory,
                                   MooCommandData    *data,
                                   const char        *options);
    GtkWidget  *(*create_widget)  (MooCommandFactory *factory);
    void        (*load_data)      (MooCommandFactory *factory,
                                   GtkWidget         *widget,
                                   MooCommandData    *data);
    gboolean    (*save_data)      (MooCommandFactory *factory,
                                   GtkWidget         *widget,
                                   MooCommandData    *data);
    gboolean    (*data_equal)     (MooCommandFactory *factory,
                                   MooCommandData    *data1,
                                   MooCommandData    *data2);
};


GType               moo_command_get_type            (void) G_GNUC_CONST;
GType               moo_command_context_get_type    (void) G_GNUC_CONST;
GType               moo_command_data_get_type       (void) G_GNUC_CONST;
GType               moo_command_factory_get_type    (void) G_GNUC_CONST;

MooCommand         *moo_command_create              (const char         *name,
                                                     const char         *options,
                                                     MooCommandData     *data);

void                moo_command_run                 (MooCommand         *cmd,
                                                     MooCommandContext  *ctx);
gboolean            moo_command_check_context       (MooCommand         *cmd,
                                                     MooCommandContext  *ctx);
gboolean            moo_command_check_sensitive     (MooCommand         *cmd,
                                                     MooEdit            *doc);

void                moo_command_set_options         (MooCommand         *cmd,
                                                     MooCommandOptions   options);

MooCommandOptions   moo_parse_command_options       (const char         *string);

void                moo_command_factory_register    (const char         *name,
                                                     const char         *display_name,
                                                     MooCommandFactory  *factory,
                                                     char              **data_keys,
                                                     const char         *extension);
MooCommandFactory  *moo_command_factory_lookup      (const char         *name);
/* returns list of MooCommandFactory instances, list should be freed */
GSList             *moo_command_list_factories      (void);


MooCommandData     *moo_command_data_new            (guint               len);

MooCommandData     *moo_command_data_ref            (MooCommandData     *data);
void                moo_command_data_unref          (MooCommandData     *data);
void                moo_command_data_set            (MooCommandData     *data,
                                                     guint               index_,
                                                     const char         *value);
void                moo_command_data_set_code       (MooCommandData     *data,
                                                     const char         *code);
const char         *moo_command_data_get            (MooCommandData     *data,
                                                     guint               index_);
const char         *moo_command_data_get_code       (MooCommandData     *data);


MooCommandContext  *moo_command_context_new         (MooEdit            *doc,
                                                     MooEditWindow      *window);

void                moo_command_context_set_doc     (MooCommandContext  *ctx,
                                                     MooEdit            *doc);
void                moo_command_context_set_window  (MooCommandContext  *ctx,
                                                     MooEditWindow      *window);
MooEdit            *moo_command_context_get_doc     (MooCommandContext  *ctx);
MooEditWindow      *moo_command_context_get_window  (MooCommandContext  *ctx);

typedef MooOutputFilter *(*MooCommandFilterFactory) (const char         *id,
                                                     gpointer            data);

void                moo_command_filter_register     (const char         *id,
                                                     const char         *name,
                                                     MooCommandFilterFactory factory_func,
                                                     gpointer            data,
                                                     GDestroyNotify      data_notify);
void                moo_command_filter_unregister   (const char         *id);
/* returns name */
const char         *moo_command_filter_lookup       (const char         *id);
/* list of ids, must be freed together with content */
GSList             *moo_command_filter_list         (void);
MooOutputFilter    *moo_command_filter_create       (const char         *id);


G_END_DECLS

#ifdef __cplusplus

#include "mooutils/mooutils-cpp.h"

MOO_DEFINE_GOBJ_TRAITS(MooCommandContext, MOO_TYPE_COMMAND_CONTEXT)

#endif // __cplusplus
