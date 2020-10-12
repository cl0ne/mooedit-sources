/*
 *   mooaction.c
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

/**
 * class:MooAction: (parent GtkAction) (moo.private 1)
 **/

#include "mooutils/mooaction-private.h"
#include "mooutils/mooactionbase-private.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooactiongroup.h"
#include <string.h>


static void _moo_action_set_closure (MooAction  *action,
                                     MooClosure *closure);


void
_moo_action_ring_the_bells_it_has_tooltip (GtkAction *action)
{
    char *tooltip;
    g_object_get (action, "tooltip", &tooltip, NULL);
    g_object_set (action, "tooltip", tooltip, NULL);
    g_free (tooltip);
}


gpointer
_moo_action_get_window (gpointer action)
{
    GtkActionGroup *group;
    MooActionCollection *collection;

    g_return_val_if_fail (GTK_IS_ACTION (action), NULL);

    group = _moo_action_get_group (action);
    g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);

    collection = _moo_action_group_get_collection (MOO_ACTION_GROUP (group));
    return _moo_action_collection_get_window (collection);
}


#define DEFINE_ACTION_TYPE(TypeName, type_name, TYPE_PARENT)                \
                                                                            \
static void type_name##_init            (TypeName           *self);         \
static void type_name##_class_init      (TypeName##Class    *klass);        \
static void type_name##_set_property    (GObject            *object,        \
                                         guint               property_id,   \
                                         const GValue       *value,         \
                                         GParamSpec         *pspec);        \
static void type_name##_get_property    (GObject            *object,        \
                                         guint               property_id,   \
                                         GValue             *value,         \
                                         GParamSpec         *pspec);        \
static gpointer type_name##_parent_class = NULL;                            \
                                                                            \
static void                                                                 \
type_name##_class_intern_init (gpointer klass)                              \
{                                                                           \
    GObjectClass *object_class = G_OBJECT_CLASS (klass);                    \
                                                                            \
    object_class->set_property = type_name##_set_property;                  \
    object_class->get_property = type_name##_get_property;                  \
    _moo_action_base_init_class (object_class);                             \
                                                                            \
    type_name##_parent_class = g_type_class_peek_parent (klass);            \
    type_name##_class_init ((TypeName##Class*) klass);                      \
}                                                                           \
                                                                            \
GType                                                                       \
type_name##_get_type (void)                                                 \
{                                                                           \
    static GType type;                                                      \
                                                                            \
    if (G_UNLIKELY (!type))                                                 \
    {                                                                       \
        static const GTypeInfo info = {                                     \
            sizeof (TypeName##Class),                                       \
            (GBaseInitFunc) NULL,                                           \
            (GBaseFinalizeFunc) NULL,                                       \
            (GClassInitFunc) type_name##_class_intern_init,                 \
            (GClassFinalizeFunc) NULL,                                      \
            NULL,   /* class_data */                                        \
            sizeof (TypeName),                                              \
            0,      /* n_preallocs */                                       \
            (GInstanceInitFunc) type_name##_init,                           \
            NULL    /* value_table */                                       \
        };                                                                  \
                                                                            \
        static const GInterfaceInfo iface_info = {NULL, NULL, NULL};        \
                                                                            \
        type = g_type_register_static (TYPE_PARENT,                         \
                                       #TypeName,                           \
                                       &info, (GTypeFlags) 0);              \
        g_type_add_interface_static (type,                                  \
                                     MOO_TYPE_ACTION_BASE,                  \
                                     &iface_info);                          \
    }                                                                       \
                                                                            \
    return type;                                                            \
}


DEFINE_ACTION_TYPE (MooAction, moo_action, GTK_TYPE_ACTION)
DEFINE_ACTION_TYPE (MooToggleAction, moo_toggle_action, GTK_TYPE_TOGGLE_ACTION)
DEFINE_ACTION_TYPE (MooRadioAction, moo_radio_action, GTK_TYPE_RADIO_ACTION)


static void
connect_proxy (GtkAction *action,
               GtkWidget *widget)
{
    GtkActionClass *parent_class;

    if (MOO_IS_ACTION (action))
        parent_class = GTK_ACTION_CLASS (moo_action_parent_class);
    else if (MOO_IS_TOGGLE_ACTION (action))
        parent_class = GTK_ACTION_CLASS (moo_toggle_action_parent_class);
    else
        parent_class = GTK_ACTION_CLASS (moo_radio_action_parent_class);

    parent_class->connect_proxy (action, widget);
    g_signal_emit_by_name (action, "connect-proxy", widget);

    _moo_action_base_connect_proxy (action, widget);
}

static void
disconnect_proxy (GtkAction *action,
                  GtkWidget *widget)
{
    GtkActionClass *parent;

    if (MOO_IS_ACTION (action))
        parent = GTK_ACTION_CLASS (moo_action_parent_class);
    else if (MOO_IS_TOGGLE_ACTION (action))
        parent = GTK_ACTION_CLASS (moo_toggle_action_parent_class);
    else
        parent = GTK_ACTION_CLASS (moo_radio_action_parent_class);

    g_signal_emit_by_name (action, "disconnect-proxy", widget);
    parent->disconnect_proxy (action, widget);
}


/*****************************************************************************/
/* MooAction
 */

struct _MooActionPrivate {
    MooClosure *closure;
};

enum {
    MOO_ACTION_BASE_PROPS(ACTION),
    ACTION_PROP_CLOSURE,
    ACTION_PROP_CLOSURE_OBJECT,
    ACTION_PROP_CLOSURE_SIGNAL,
    ACTION_PROP_CLOSURE_CALLBACK,
    ACTION_PROP_CLOSURE_PROXY_FUNC
};


static void
moo_action_init (MooAction *action)
{
    action->priv = G_TYPE_INSTANCE_GET_PRIVATE (action,
                                                MOO_TYPE_ACTION,
                                                MooActionPrivate);
    _moo_action_base_init_instance (action);
}


static void
moo_action_dispose (GObject *object)
{
    MooAction *action = MOO_ACTION (object);

    if (action->priv->closure)
    {
        moo_closure_unref (action->priv->closure);
        action->priv->closure = NULL;
    }

    G_OBJECT_CLASS (moo_action_parent_class)->dispose (object);
}


static void
moo_action_activate (GtkAction *gtkaction)
{
    MooAction *action = MOO_ACTION (gtkaction);

    if (GTK_ACTION_CLASS (moo_action_parent_class)->activate)
        GTK_ACTION_CLASS (moo_action_parent_class)->activate (gtkaction);

    if (action->priv->closure)
        moo_closure_invoke (action->priv->closure);
}


static GObject *
moo_action_constructor (GType                  type,
                        guint                  n_props,
                        GObjectConstructParam *props)
{
    guint i;
    GObject *object;
    MooClosure *closure = NULL;
    gpointer closure_object = NULL;
    const char *closure_signal = NULL;
    GCallback closure_callback = NULL;
    GCallback closure_proxy_func = NULL;

    for (i = 0; i < n_props; ++i)
    {
        const char *name = props[i].pspec->name;
        GValue *value = props[i].value;

        if (!strcmp (name, "closure-object"))
            closure_object = g_value_get_object (value);
        else if (!strcmp (name, "closure-signal"))
            closure_signal = g_value_get_string (value);
        else if (!strcmp (name, "closure-callback"))
            closure_callback = (GCallback) g_value_get_pointer (value);
        else if (!strcmp (name, "closure-proxy-func"))
            closure_proxy_func = (GCallback) g_value_get_pointer (value);
    }

    if (closure_callback || closure_signal)
    {
        if (closure_object)
        {
            closure = _moo_closure_new_simple (closure_object, closure_signal,
                                               closure_callback, closure_proxy_func);
            moo_closure_ref_sink (closure);
        }
        else
            g_critical ("closure data missing");
    }

    object = G_OBJECT_CLASS(moo_action_parent_class)->constructor (type, n_props, props);

    if (closure)
    {
        _moo_action_set_closure (MOO_ACTION (object), closure);
        moo_closure_unref (closure);
    }

    return object;
}


static void
moo_action_class_init (MooActionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MooActionPrivate));

    object_class->dispose = moo_action_dispose;
    object_class->constructor = moo_action_constructor;
    action_class->activate = moo_action_activate;
    action_class->connect_proxy = connect_proxy;
    action_class->disconnect_proxy = disconnect_proxy;

    g_object_class_install_property (object_class, ACTION_PROP_CLOSURE,
                                     g_param_spec_boxed ("closure", "closure", "closure",
                                                         MOO_TYPE_CLOSURE,
                                                         (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (object_class, ACTION_PROP_CLOSURE_OBJECT,
                                     g_param_spec_object ("closure-object", "closure-object", "closure-object",
                                                          G_TYPE_OBJECT, (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (object_class, ACTION_PROP_CLOSURE_SIGNAL,
                                     g_param_spec_string ("closure-signal", "closure-signal", "closure-signal",
                                                          NULL, (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (object_class, ACTION_PROP_CLOSURE_CALLBACK,
                                     g_param_spec_pointer ("closure-callback", "closure-callback", "closure-callback",
                                                           (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (object_class, ACTION_PROP_CLOSURE_PROXY_FUNC,
                                     g_param_spec_pointer ("closure-proxy-func", "closure-proxy-func", "closure-proxy-func",
                                                           (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
}


static void
moo_action_set_property (GObject            *object,
                         guint               property_id,
                         const GValue       *value,
                         GParamSpec         *pspec)
{
    MooAction *action = MOO_ACTION (object);

    switch (property_id)
    {
        case ACTION_PROP_CLOSURE:
            _moo_action_set_closure (action, (MooClosure*) g_value_get_boxed (value));
            break;

        case ACTION_PROP_CLOSURE_OBJECT:
        case ACTION_PROP_CLOSURE_SIGNAL:
        case ACTION_PROP_CLOSURE_CALLBACK:
        case ACTION_PROP_CLOSURE_PROXY_FUNC:
            /* these are handled in the constructor */
            break;

        MOO_ACTION_BASE_SET_PROPERTY (ACTION);

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_action_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
    MooAction *action = MOO_ACTION (object);

    switch (property_id)
    {
        case ACTION_PROP_CLOSURE:
            g_value_set_boxed (value, action->priv->closure);
            break;

        MOO_ACTION_BASE_GET_PROPERTY (ACTION);

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
_moo_action_set_closure (MooAction  *action,
                         MooClosure *closure)
{
    g_return_if_fail (MOO_IS_ACTION (action));

    if (closure == action->priv->closure)
        return;

    if (action->priv->closure)
        moo_closure_unref (action->priv->closure);
    if (closure)
        moo_closure_ref_sink (closure);

    action->priv->closure = closure;
    g_object_notify (G_OBJECT (action), "closure");
}


/*****************************************************************************/
/* MooToggleAction
 */

typedef void (*MooToggleActionCallback)     (gpointer            data,
                                             gboolean            active);

struct _MooToggleActionPrivate {
    MooToggleActionCallback callback;
    MooObjectPtr *ptr;
    gpointer data;
};

enum {
    MOO_ACTION_BASE_PROPS(TOGGLE_ACTION),
    TOGGLE_ACTION_PROP_TOGGLED_CALLBACK,
    TOGGLE_ACTION_PROP_TOGGLED_OBJECT,
    TOGGLE_ACTION_PROP_TOGGLED_DATA
};


static void
moo_toggle_action_init (MooToggleAction *action)
{
    action->priv = G_TYPE_INSTANCE_GET_PRIVATE (action,
                                                MOO_TYPE_TOGGLE_ACTION,
                                                MooToggleActionPrivate);
    _moo_action_base_init_instance (action);
}


static void
moo_toggle_action_dispose (GObject *object)
{
    MooToggleAction *action = MOO_TOGGLE_ACTION (object);

    if (action->priv->ptr)
    {
        _moo_object_ptr_free (action->priv->ptr);
        action->priv->ptr = NULL;
    }

    G_OBJECT_CLASS (moo_toggle_action_parent_class)->dispose (object);
}


static void
moo_toggle_action_toggled (GtkToggleAction *gtkaction)
{
    MooToggleAction *action = MOO_TOGGLE_ACTION (gtkaction);

    if (GTK_TOGGLE_ACTION_CLASS (moo_toggle_action_parent_class)->toggled)
        GTK_TOGGLE_ACTION_CLASS (moo_toggle_action_parent_class)->toggled (gtkaction);

    if (action->priv->callback)
    {
        if (MOO_OBJECT_PTR_GET (action->priv->ptr))
        {
            GObject *obj = MOO_OBJECT_PTR_GET (action->priv->ptr);
            g_object_ref (obj);
            action->priv->callback (obj, gtk_toggle_action_get_active (gtkaction));
            g_object_unref (obj);
        }
        else
        {
            action->priv->callback (action->priv->data, gtk_toggle_action_get_active (gtkaction));
        }
    }
}


static void
_moo_toggle_action_set_callback (MooToggleAction    *action,
                                 MooToggleActionCallback callback,
                                 gpointer            data,
                                 gboolean            object)
{
    g_return_if_fail (MOO_IS_TOGGLE_ACTION (action));
    g_return_if_fail (!object || G_IS_OBJECT (data));

    action->priv->callback = callback;
    if (action->priv->ptr)
        _moo_object_ptr_free (action->priv->ptr);
    action->priv->ptr = NULL;
    action->priv->data = NULL;

    if (callback)
    {
        if (object)
            action->priv->ptr = _moo_object_ptr_new (G_OBJECT (data), NULL, NULL);
        else
            action->priv->data = data;
    }
}


static GObject *
moo_toggle_action_constructor (GType                  type,
                               guint                  n_props,
                               GObjectConstructParam *props)
{
    guint i;
    GObject *object;
    MooToggleAction *action;
    MooToggleActionCallback toggled_callback = NULL;
    gpointer toggled_data = NULL;
    gpointer toggled_object = NULL;

    for (i = 0; i < n_props; ++i)
    {
        const char *name = props[i].pspec->name;
        GValue *value = props[i].value;

        if (!strcmp (name, "toggled-callback"))
            toggled_callback = (MooToggleActionCallback) g_value_get_pointer (value);
        else if (!strcmp (name, "toggled-data"))
            toggled_data = g_value_get_pointer (value);
        else if (!strcmp (name, "toggled-object"))
            toggled_object = g_value_get_object (value);
    }

    object = G_OBJECT_CLASS(moo_toggle_action_parent_class)->constructor (type, n_props, props);
    action = MOO_TOGGLE_ACTION (object);

    if (toggled_callback)
    {
        if (toggled_object)
            _moo_toggle_action_set_callback (action, toggled_callback, toggled_object, TRUE);
        else
            _moo_toggle_action_set_callback (action, toggled_callback, toggled_data, FALSE);
    }

    return object;
}


static void
moo_toggle_action_class_init (MooToggleActionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkActionClass *action_class = GTK_ACTION_CLASS (klass);
    GtkToggleActionClass *toggle_action_class = GTK_TOGGLE_ACTION_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MooToggleActionPrivate));

    object_class->dispose = moo_toggle_action_dispose;
    object_class->constructor = moo_toggle_action_constructor;
    toggle_action_class->toggled = moo_toggle_action_toggled;
    action_class->connect_proxy = connect_proxy;
    action_class->disconnect_proxy = disconnect_proxy;

    g_object_class_install_property (object_class, TOGGLE_ACTION_PROP_TOGGLED_CALLBACK,
                                     g_param_spec_pointer ("toggled-callback", "toggled-callback", "toggled-callback",
                                                           (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (object_class, TOGGLE_ACTION_PROP_TOGGLED_OBJECT,
                                     g_param_spec_object ("toggled-object", "toggled-object", "toggled-object",
                                                          G_TYPE_OBJECT, (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (object_class, TOGGLE_ACTION_PROP_TOGGLED_DATA,
                                     g_param_spec_pointer ("toggled-data", "toggled-data", "toggled-data",
                                                           (GParamFlags) (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
}


static void
moo_toggle_action_set_property (GObject            *object,
                                guint               property_id,
                                const GValue       *value,
                                GParamSpec         *pspec)
{
    switch (property_id)
    {
        case TOGGLE_ACTION_PROP_TOGGLED_CALLBACK:
        case TOGGLE_ACTION_PROP_TOGGLED_OBJECT:
        case TOGGLE_ACTION_PROP_TOGGLED_DATA:
            /* these are handled in the constructor */
            break;

        MOO_ACTION_BASE_SET_PROPERTY (TOGGLE_ACTION);

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_toggle_action_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    switch (property_id)
    {
        MOO_ACTION_BASE_GET_PROPERTY (TOGGLE_ACTION);

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


/*****************************************************************************/
/* MooRadioAction
 */

enum {
    MOO_ACTION_BASE_PROPS(RADIO_ACTION)
};


static void
moo_radio_action_init (MooRadioAction *action)
{
    _moo_action_base_init_instance (action);
}


static void
moo_radio_action_class_init (MooRadioActionClass *klass)
{
    GtkActionClass *action_class = GTK_ACTION_CLASS (klass);
    action_class->connect_proxy = connect_proxy;
    action_class->disconnect_proxy = disconnect_proxy;
}


static void
moo_radio_action_set_property (GObject            *object,
                               guint               property_id,
                               const GValue       *value,
                               GParamSpec         *pspec)
{
    switch (property_id)
    {
        MOO_ACTION_BASE_SET_PROPERTY (RADIO_ACTION);

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_radio_action_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    switch (property_id)
    {
        MOO_ACTION_BASE_GET_PROPERTY (RADIO_ACTION);

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


/**************************************************************************/
/* _moo_sync_toggle_action
 */

typedef struct {
    MooObjectWatch parent;
    GParamSpec *pspec;
    gboolean invert;
} ToggleWatch;

static void action_toggled          (ToggleWatch    *watch);
static void prop_changed            (ToggleWatch    *watch);
static void toggle_watch_destroy    (MooObjectWatch *watch);

static MooObjectWatchClass ToggleWatchClass = {NULL, NULL, toggle_watch_destroy};


static ToggleWatch *
toggle_watch_new (GObject    *master,
                  const char *prop,
                  GtkAction  *action,
                  gboolean    invert)
{
    ToggleWatch *watch;
    GObjectClass *klass;
    GParamSpec *pspec;
    char *signal;

    g_return_val_if_fail (G_IS_OBJECT (master), NULL);
    g_return_val_if_fail (GTK_IS_TOGGLE_ACTION (action), NULL);
    g_return_val_if_fail (prop != NULL, NULL);

    klass = G_OBJECT_CLASS (g_type_class_peek (G_OBJECT_TYPE (master)));
    pspec = g_object_class_find_property (klass, prop);

    if (!pspec)
    {
        g_warning ("no property '%s' in class '%s'",
                   prop, g_type_name (G_OBJECT_TYPE (master)));
        return NULL;
    }

    watch = _moo_object_watch_new (ToggleWatch, &ToggleWatchClass,
                                   master, action, NULL, NULL);

    watch->pspec = pspec;
    watch->invert = invert;

    signal = g_strdup_printf ("notify::%s", prop);

    g_signal_connect_swapped (master, signal,
                              G_CALLBACK (prop_changed),
                              watch);
    g_signal_connect_swapped (action, "toggled",
                              G_CALLBACK (action_toggled),
                              watch);

    g_free (signal);
    return watch;
}


static void
toggle_watch_destroy (MooObjectWatch *watch)
{
    if (MOO_OBJECT_PTR_GET (watch->source))
    {
        g_signal_handlers_disconnect_by_func (MOO_OBJECT_PTR_GET (watch->source),
                                              (gpointer) prop_changed,
                                              watch);
    }

    if (MOO_OBJECT_PTR_GET (watch->target))
    {
        g_assert (GTK_IS_TOGGLE_ACTION (MOO_OBJECT_PTR_GET (watch->target)));
        g_signal_handlers_disconnect_by_func (MOO_OBJECT_PTR_GET (watch->target),
                                              (gpointer) action_toggled,
                                              watch);
    }
}


void
_moo_sync_toggle_action (GtkAction  *action,
                         gpointer    master,
                         const char *prop,
                         gboolean    invert)
{
    ToggleWatch *watch;

    g_return_if_fail (GTK_IS_TOGGLE_ACTION (action));
    g_return_if_fail (G_IS_OBJECT (master));
    g_return_if_fail (prop != NULL);

    watch = toggle_watch_new (G_OBJECT (master), prop, action, invert);
    g_return_if_fail (watch != NULL);

    prop_changed (watch);
}


static void
prop_changed (ToggleWatch *watch)
{
    gboolean value, active, equal;
    gpointer action;

    g_object_get (MOO_OBJECT_PTR_GET (watch->parent.source),
                  watch->pspec->name, &value, NULL);

    action = MOO_OBJECT_PTR_GET (watch->parent.target);
    g_assert (GTK_IS_TOGGLE_ACTION (action));
    active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

    if (!watch->invert)
        equal = !value == !active;
    else
        equal = !value != !active;

    if (!equal)
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), watch->invert ? !value : value);
}


static void
action_toggled (ToggleWatch *watch)
{
    gboolean value, active, equal;
    gpointer action;

    g_object_get (MOO_OBJECT_PTR_GET (watch->parent.source),
                  watch->pspec->name, &value, NULL);

    action = MOO_OBJECT_PTR_GET (watch->parent.target);
    g_assert (GTK_IS_TOGGLE_ACTION (action));
    active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

    if (!watch->invert)
        equal = !value == !active;
    else
        equal = !value != !active;

    if (!equal)
        g_object_set (MOO_OBJECT_PTR_GET (watch->parent.source),
                      watch->pspec->name,
                      watch->invert ? !active : active, NULL);
}
