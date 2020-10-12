/*
 *   moobigpaned.c
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
 * class:MooBigPaned: (parent GtkFrame) (constructable) (moo.private 1)
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "moobigpaned.h"
#include "marshals.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "mooutils-misc.h"
#include "moocompat.h"

typedef struct {
    GSList *order; /* ids */
    int size;
    int sticky;
    char *active;
} MooPanedConfig;

typedef struct {
    MooPanedConfig paned[4];
    GHashTable *panes;
} MooBigPanedConfig;

typedef struct {
    int        bbox_size;
    GdkRegion *bbox_region;
    GdkRegion *def_region;
} DZ;

struct _MooBigPanedPrivate {
    MooPanePosition order[4]; /* inner is paned[order[3]] */
    GtkWidget   *inner;
    GtkWidget   *outer;

    MooBigPanedConfig *config;
    GHashTable  *panes;

    int          drop_pos;
    int          drop_button_index;
    GdkRectangle drop_rect;
    GdkRectangle drop_button_rect;
    GdkWindow   *drop_outline;
    DZ *dz;
    GdkRegion    *drop_region;
    guint         drop_region_is_buttons : 1;
};

static void     moo_big_paned_finalize      (GObject        *object);
static void     moo_big_paned_set_property  (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void     moo_big_paned_get_property  (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static gboolean moo_big_paned_expose        (GtkWidget      *widget,
                                             GdkEventExpose *event,
                                             MooBigPaned    *paned);

static void     child_set_pane_size         (GtkWidget      *child,
                                             int             size,
                                             MooBigPaned    *paned);
static void     sticky_pane_notify          (GtkWidget      *child,
                                             GParamSpec     *pspec,
                                             MooBigPaned    *paned);
static void     active_pane_notify          (GtkWidget      *child,
                                             GParamSpec     *pspec,
                                             MooBigPaned    *paned);

static gboolean check_children_order        (MooBigPaned    *paned);

static void     handle_drag_start           (MooPaned       *child,
                                             GtkWidget      *pane_widget,
                                             MooBigPaned    *paned);
static void     handle_drag_motion          (MooPaned       *child,
                                             GtkWidget      *pane_widget,
                                             MooBigPaned    *paned);
static void     handle_drag_end             (MooPaned       *child,
                                             GtkWidget      *pane_widget,
                                             gboolean        drop,
                                             MooBigPaned    *paned);

static void     config_changed              (MooBigPaned    *paned);

static MooBigPanedConfig *config_new        (void);
static void               config_free       (MooBigPanedConfig *config);
static MooBigPanedConfig *config_parse      (const char        *string);
static char              *config_serialize  (MooBigPanedConfig *config);


/* MOO_TYPE_BIG_PANED */
G_DEFINE_TYPE (MooBigPaned, moo_big_paned, GTK_TYPE_FRAME)

enum {
    PROP_0,
    PROP_PANE_ORDER,
    PROP_ENABLE_HANDLE_DRAG,
    PROP_ENABLE_DETACHING,
    PROP_HANDLE_CURSOR_TYPE
};

enum {
    CONFIG_CHANGED,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

static void
moo_big_paned_class_init (MooBigPanedClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MooBigPanedPrivate));

    gobject_class->finalize = moo_big_paned_finalize;
    gobject_class->set_property = moo_big_paned_set_property;
    gobject_class->get_property = moo_big_paned_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_PANE_ORDER,
                                     g_param_spec_pointer ("pane-order",
                                             "pane-order",
                                             "pane-order",
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_HANDLE_DRAG,
                                     g_param_spec_boolean ("enable-handle-drag",
                                             "enable-handle-drag",
                                             "enable-handle-drag",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_CONSTRUCT | G_PARAM_WRITABLE)));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_DETACHING,
                                     g_param_spec_boolean ("enable-detaching",
                                             "enable-detaching",
                                             "enable-detaching",
                                             FALSE,
                                             (GParamFlags) (G_PARAM_CONSTRUCT | G_PARAM_WRITABLE)));

    g_object_class_install_property (gobject_class,
                                     PROP_HANDLE_CURSOR_TYPE,
                                     g_param_spec_enum ("handle-cursor-type",
                                             "handle-cursor-type",
                                             "handle-cursor-type",
                                             GDK_TYPE_CURSOR_TYPE,
                                             GDK_HAND2,
                                             (GParamFlags) (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));

    signals[CONFIG_CHANGED] =
            g_signal_new ("config-changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          0,
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}


#define NTH_CHILD(paned,n) paned->paned[paned->priv->order[n]]

static void
moo_big_paned_init (MooBigPaned *paned)
{
    int i;

    paned->priv = G_TYPE_INSTANCE_GET_PRIVATE (paned,
                                               MOO_TYPE_BIG_PANED,
                                               MooBigPanedPrivate);

    paned->priv->panes = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                g_free, g_object_unref);
    paned->priv->config = config_new ();

    paned->priv->drop_pos = -1;

    /* XXX destroy */
    for (i = 0; i < 4; ++i)
    {
        GtkWidget *child;

        paned->paned[i] = child =
                GTK_WIDGET (g_object_new (MOO_TYPE_PANED,
                                          "pane-position", (MooPanePosition) i,
                                          (const char*) NULL));

        g_object_ref_sink (child);
        gtk_widget_show (child);

        g_signal_connect_after (child, "set-pane-size",
                                G_CALLBACK (child_set_pane_size),
                                paned);
        g_signal_connect (child, "notify::sticky-pane",
                          G_CALLBACK (sticky_pane_notify),
                          paned);
        g_signal_connect (child, "notify::active-pane",
                          G_CALLBACK (active_pane_notify),
                          paned);
        g_signal_connect (child, "handle-drag-start",
                          G_CALLBACK (handle_drag_start),
                          paned);
        g_signal_connect (child, "handle-drag-motion",
                          G_CALLBACK (handle_drag_motion),
                          paned);
        g_signal_connect (child, "handle-drag-end",
                          G_CALLBACK (handle_drag_end),
                          paned);
    }

    paned->priv->order[0] = MOO_PANE_POS_LEFT;
    paned->priv->order[1] = MOO_PANE_POS_RIGHT;
    paned->priv->order[2] = MOO_PANE_POS_TOP;
    paned->priv->order[3] = MOO_PANE_POS_BOTTOM;

    paned->priv->inner = NTH_CHILD (paned, 3);
    paned->priv->outer = NTH_CHILD (paned, 0);

    gtk_container_add (GTK_CONTAINER (paned), NTH_CHILD (paned, 0));

    for (i = 0; i < 3; ++i)
        gtk_container_add (GTK_CONTAINER (NTH_CHILD (paned, i)), NTH_CHILD (paned, i+1));

    g_assert (check_children_order (paned));
}


static gboolean
check_children_order (MooBigPaned *paned)
{
    int i;

    if (GTK_BIN (paned)->child != NTH_CHILD (paned, 0))
        return FALSE;

    for (i = 0; i < 3; ++i)
        if (GTK_BIN (NTH_CHILD (paned, i))->child != NTH_CHILD (paned, i+1))
                return FALSE;

    return TRUE;
}


void
moo_big_paned_set_pane_order (MooBigPaned *paned,
                              int         *order)
{
    MooPanePosition new_order[4];
    int i;
    GtkWidget *child;

    g_return_if_fail (MOO_IS_BIG_PANED (paned));
    g_return_if_fail (order != NULL);

    for (i = 0; i < 4; ++i)
    {
        g_return_if_fail (0 <= order[i] && order[i] < 4);
        new_order[i] = (MooPanePosition) order[i];
    }

    g_return_if_fail (check_children_order (paned));

    for (i = 0; i < 4; ++i)
    {
        if (new_order[i] != paned->priv->order[i])
            break;
    }

    if (i == 4)
        return;

    child = moo_big_paned_get_child (paned);

    if (child)
        g_object_ref (child);

    gtk_container_remove (GTK_CONTAINER (paned), NTH_CHILD (paned, 0));
    for (i = 0; i < 3; ++i)
        gtk_container_remove (GTK_CONTAINER (NTH_CHILD (paned, i)), NTH_CHILD (paned, i+1));
    if (child)
        gtk_container_remove (GTK_CONTAINER (NTH_CHILD (paned, 3)), child);

    for (i = 0; i < 4; ++i)
        paned->priv->order[i] = new_order[i];

    gtk_container_add (GTK_CONTAINER (paned), NTH_CHILD (paned, 0));

    for (i = 0; i < 3; ++i)
        gtk_container_add (GTK_CONTAINER (NTH_CHILD (paned, i)), NTH_CHILD (paned, i+1));

    paned->priv->inner = NTH_CHILD (paned, 3);
    paned->priv->outer = NTH_CHILD (paned, 0);

    if (child)
    {
        gtk_container_add (GTK_CONTAINER (paned->priv->inner), child);
        g_object_unref (child);
    }

    g_assert (check_children_order (paned));
    g_object_notify (G_OBJECT (paned), "pane-order");
}


static void
moo_big_paned_finalize (GObject *object)
{
    MooBigPaned *paned = MOO_BIG_PANED (object);
    int i;

    g_hash_table_destroy (paned->priv->panes);
    config_free (paned->priv->config);

    for (i = 0; i < 4; ++i)
        g_object_unref (paned->paned[i]);

    if (paned->priv->drop_outline)
    {
        g_critical ("oops");
        gdk_window_set_user_data (paned->priv->drop_outline, NULL);
        gdk_window_destroy (paned->priv->drop_outline);
    }

    G_OBJECT_CLASS (moo_big_paned_parent_class)->finalize (object);
}


GtkWidget *
moo_big_paned_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_BIG_PANED, (const char*) NULL));
}


static void
config_changed (MooBigPaned *paned)
{
    g_signal_emit (paned, signals[CONFIG_CHANGED], 0);
}

static void
child_set_pane_size (GtkWidget   *child,
                     int          size,
                     MooBigPaned *paned)
{
    MooPanePosition pos;

    g_object_get (child, "pane-position", &pos, NULL);
    g_return_if_fail (paned->paned[pos] == child);

    paned->priv->config->paned[pos].size = size;
    config_changed (paned);
}

static void
sticky_pane_notify (GtkWidget   *child,
                    G_GNUC_UNUSED GParamSpec *pspec,
                    MooBigPaned *paned)
{
    MooPanePosition pos;
    gboolean sticky;

    g_object_get (child, "pane-position", &pos,
                  "sticky-pane", &sticky, NULL);
    g_return_if_fail (paned->paned[pos] == child);

    if (paned->priv->config->paned[pos].sticky != sticky)
    {
        paned->priv->config->paned[pos].sticky = sticky;
        config_changed (paned);
    }
}

static void
active_pane_notify (GtkWidget   *child,
                    G_GNUC_UNUSED GParamSpec *pspec,
                    MooBigPaned *paned)
{
    MooPanePosition pos;
    MooPane *pane = NULL;
    const char *id = NULL;
    MooPanedConfig *pc;

    g_object_get (child, "pane-position", &pos,
                  "active-pane", &pane, NULL);
    g_return_if_fail (paned->paned[pos] == child);

    if (pane)
        id = moo_pane_get_id (pane);

    pc = &paned->priv->config->paned[pos];
    if ((id && !pc->active) || (!id && pc->active) ||
        (id && pc->active && strcmp (id, pc->active) != 0))
    {
        g_free (pc->active);
        pc->active = g_strdup (id);
        config_changed (paned);
    }

    if (pane)
        g_object_unref (pane);
}

static void
pane_params_changed (MooPane       *pane,
                     G_GNUC_UNUSED GParamSpec *pspec,
                     MooBigPaned   *paned)
{
    const char *id;
    MooPaneParams *params;

    id = moo_pane_get_id (pane);
    g_return_if_fail (id != NULL);

    params = moo_pane_get_params (pane);
    g_hash_table_insert (paned->priv->config->panes, g_strdup (id), params);

    config_changed (paned);
}


static MooPaneParams *
get_pane_config (MooBigPaned     *paned,
                 const char      *id,
                 MooPanePosition *position,
                 int             *index)
{
    MooPaneParams *params;
    int pos;
    gboolean found = FALSE;

    g_return_val_if_fail (id != NULL, NULL);

    if (!(params = (MooPaneParams*) g_hash_table_lookup (paned->priv->config->panes, id)))
        return NULL;

    for (pos = 0; pos < 4 && !found; ++pos)
    {
        GSList *link;
        int index_here = 0;

        for (link = paned->priv->config->paned[pos].order;
             link != NULL && !found;
             link = link->next)
        {
            if (strcmp ((const char*) link->data, id) == 0)
            {
                *index = index_here;
                *position = (MooPanePosition) pos;
                found = TRUE;
            }
            else if (g_hash_table_lookup (paned->priv->panes, link->data))
            {
                index_here += 1;
            }
        }
    }

    if (!found)
    {
        g_critical ("%s: oops", G_STRFUNC);
        params = NULL;
    }

    return params;
}

static void
add_pane_id_to_list (MooBigPaned    *paned,
                     MooPanedConfig *pc,
                     const char     *id,
                     int             index)
{
    GSList *link;
    int real_index;

    g_return_if_fail (index >= 0);

    for (link = pc->order, real_index = 0;
         link != NULL;
         link = link->next, ++real_index)
    {
        if (index == 0)
            break;

        if (g_hash_table_lookup (paned->priv->panes, link->data))
            index -= 1;
    }

    pc->order = g_slist_insert (pc->order, g_strdup (id), real_index);
}

static void
add_pane_config (MooBigPaned     *paned,
                 const char      *id,
                 MooPane         *pane,
                 MooPanePosition  pos,
                 int              index)
{
    MooPaneParams *params;

    g_return_if_fail (index >= 0);
    g_return_if_fail (!g_hash_table_lookup (paned->priv->config->panes, id));

    params = moo_pane_get_params (pane);
    g_hash_table_insert (paned->priv->config->panes, g_strdup (id), params);

    add_pane_id_to_list (paned, &paned->priv->config->paned[pos], id, index);

    config_changed (paned);
}

static void
move_pane_config (MooBigPaned     *paned,
                  const char      *id,
                  MooPanePosition  old_pos,
                  MooPanePosition  new_pos,
                  int              new_index)
{
    GSList *old_link;
    MooBigPanedConfig *config = paned->priv->config;
    MooPanedConfig *old_pc;

    g_return_if_fail (new_index >= 0);
    g_return_if_fail (g_hash_table_lookup (config->panes, id) != NULL);

    old_pc = &config->paned[old_pos];
    old_link = g_slist_find_custom (old_pc->order, id, (GCompareFunc) strcmp);
    g_return_if_fail (old_link != NULL);

    g_free (old_link->data);
    old_pc->order = g_slist_delete_link (old_pc->order, old_link);
    add_pane_id_to_list (paned, &paned->priv->config->paned[new_pos], id, new_index);

    config_changed (paned);
}


void
moo_big_paned_set_config (MooBigPaned *paned,
                          const char  *string)
{
    MooBigPanedConfig *config;
    int pos;

    g_return_if_fail (MOO_IS_BIG_PANED (paned));

    if (!string || !string[0])
        return;

    for (pos = 0; pos < 4; ++pos)
    {
        if (paned->priv->config->paned[pos].order != NULL)
        {
            g_critical ("%s: this function may be called only once "
                        "and before any panes are added", G_STRFUNC);
            return;
        }
    }

    if (g_hash_table_size (paned->priv->config->panes) != 0)
    {
        g_critical ("%s: this function may be called only once "
                    "and before any panes are added", G_STRFUNC);
        return;
    }

    if (!(config = config_parse (string)))
        return;

    config_free (paned->priv->config);
    paned->priv->config = config;

    for (pos = 0; pos < 4; ++pos)
    {
        if (config->paned[pos].size > 0)
            moo_paned_set_pane_size (MOO_PANED (paned->paned[pos]),
                                     config->paned[pos].size);
        if (config->paned[pos].sticky >= 0)
            moo_paned_set_sticky_pane (MOO_PANED (paned->paned[pos]),
                                       config->paned[pos].sticky);
    }
}

char *
moo_big_paned_get_config (MooBigPaned *paned)
{
    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    return config_serialize (paned->priv->config);
}


MooPane *
moo_big_paned_insert_pane (MooBigPaned        *paned,
                           GtkWidget          *pane_widget,
                           const char         *id,
                           MooPaneLabel       *pane_label,
                           MooPanePosition     position,
                           int                 index)
{
    MooPane *pane;
    MooPaneParams *params = NULL;

    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (pane_widget), NULL);
    g_return_val_if_fail (position < 4, NULL);

    if (id && moo_big_paned_lookup_pane (paned, id) != NULL)
    {
        g_critical ("%s: pane with id '%s' already exists",
                    G_STRFUNC, id);
        return NULL;
    }

    if (id)
        params = get_pane_config (paned, id, &position, &index);

    if (index < 0)
        index = moo_paned_n_panes (MOO_PANED (paned->paned[position]));

    pane = moo_paned_insert_pane (MOO_PANED (paned->paned[position]),
                                  pane_widget, pane_label, index);

    if (pane && id)
    {
        _moo_pane_set_id (pane, id);

        if (params)
            moo_pane_set_params (pane, params);

        g_hash_table_insert (paned->priv->panes,
                             g_strdup (id),
                             g_object_ref (pane));

        g_signal_connect (pane, "notify::params",
                          G_CALLBACK (pane_params_changed),
                          paned);
        if (!params)
            add_pane_config (paned, id, pane, position, index);

        if (paned->priv->config->paned[position].active &&
            !strcmp (paned->priv->config->paned[position].active, id))
                moo_pane_open (pane);
    }

    return pane;
}


void
moo_big_paned_reorder_pane (MooBigPaned    *paned,
                            GtkWidget      *pane_widget,
                            MooPanePosition new_position,
                            int             new_index)
{
    MooPane *pane;
    MooPaned *child;
    MooPanePosition old_position;
    int old_index;
    const char *id;

    g_return_if_fail (MOO_IS_BIG_PANED (paned));
    g_return_if_fail (GTK_IS_WIDGET (pane_widget));
    g_return_if_fail (new_position < 4);

    pane = moo_big_paned_find_pane (paned, pane_widget, &child);
    g_return_if_fail (pane != NULL);

    g_object_get (child, "pane-position", &old_position, NULL);
    old_index = moo_paned_get_pane_num (child, pane_widget);
    g_return_if_fail (old_index >= 0);

    if (new_index < 0)
    {
        if (old_position == new_position)
            new_index = (int) moo_paned_n_panes (child) - 1;
        else
            new_index = moo_paned_n_panes (MOO_PANED (paned->paned[new_position]));
    }

    if (old_position == new_position && old_index == new_index)
        return;

    pane = moo_paned_get_pane (child, pane_widget);

    if (old_position == new_position)
    {
        _moo_paned_reorder_child (child, pane, new_index);
    }
    else
    {
        g_object_ref (pane);

        moo_paned_remove_pane (child, pane_widget);
        _moo_paned_insert_pane (MOO_PANED (paned->paned[new_position]), pane, new_index);
        moo_pane_open (pane);

        g_object_unref (pane);
    }

    if ((id = moo_pane_get_id (pane)))
        move_pane_config (paned, id, old_position, new_position, new_index);
}


void
moo_big_paned_add_child (MooBigPaned *paned,
                         GtkWidget   *child)
{
    g_return_if_fail (MOO_IS_BIG_PANED (paned));
    gtk_container_add (GTK_CONTAINER (paned->priv->inner), child);
}


void
moo_big_paned_remove_child (MooBigPaned *paned)
{
    g_return_if_fail (MOO_IS_BIG_PANED (paned));
    gtk_container_remove (GTK_CONTAINER (paned->priv->inner),
                          GTK_BIN (paned->priv->inner)->child);
}


GtkWidget *
moo_big_paned_get_child (MooBigPaned *paned)
{
    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    return GTK_BIN(paned->priv->inner)->child;
}


gboolean
moo_big_paned_remove_pane (MooBigPaned *paned,
                           GtkWidget   *widget)
{
    MooPaned *child;
    MooPane *pane;
    const char *id;

    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), FALSE);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

    pane = moo_big_paned_find_pane (paned, widget, &child);

    if (!pane)
        return FALSE;

    if ((id = moo_pane_get_id (pane)))
        g_hash_table_remove (paned->priv->panes, id);

    return moo_paned_remove_pane (child, widget);
}


MooPane *
moo_big_paned_lookup_pane (MooBigPaned *paned,
                           const char  *pane_id)
{
    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    g_return_val_if_fail (pane_id != NULL, NULL);
    return MOO_PANE (g_hash_table_lookup (paned->priv->panes, pane_id));
}


#define PROXY_FUNC(name)                                    \
void                                                        \
moo_big_paned_##name (MooBigPaned *paned,                   \
                      GtkWidget   *widget)                  \
{                                                           \
    MooPane *pane;                                          \
    MooPaned *child = NULL;                                 \
                                                            \
    g_return_if_fail (MOO_IS_BIG_PANED (paned));            \
    g_return_if_fail (GTK_IS_WIDGET (widget));              \
                                                            \
    pane = moo_big_paned_find_pane (paned, widget, &child); \
    g_return_if_fail (pane != NULL);                        \
                                                            \
    moo_paned_##name (child, pane);                         \
}

PROXY_FUNC (open_pane)
PROXY_FUNC (present_pane)
PROXY_FUNC (attach_pane)
PROXY_FUNC (detach_pane)

#undef PROXY_FUNC

void
moo_big_paned_hide_pane (MooBigPaned *paned,
                         GtkWidget   *widget)
{
    MooPaned *child = NULL;

    g_return_if_fail (MOO_IS_BIG_PANED (paned));
    g_return_if_fail (GTK_IS_WIDGET (widget));

    moo_big_paned_find_pane (paned, widget, &child);
    g_return_if_fail (child != NULL);

    moo_paned_hide_pane (child);
}


MooPaned *
moo_big_paned_get_paned (MooBigPaned    *paned,
                         MooPanePosition position)
{
    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    g_return_val_if_fail (position < 4, NULL);
    return MOO_PANED (paned->paned[position]);
}


MooPane *
moo_big_paned_find_pane (MooBigPaned    *paned,
                         GtkWidget      *widget,
                         MooPaned      **child_paned)
{
    int i;
    MooPane *pane;

    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

    if (child_paned)
        *child_paned = NULL;

    for (i = 0; i < 4; ++i)
    {
        pane = moo_paned_get_pane (MOO_PANED (paned->paned[i]), widget);

        if (pane)
        {
            if (child_paned)
                *child_paned = MOO_PANED (paned->paned[i]);
            return pane;
        }
    }

    return NULL;
}


static void
moo_big_paned_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    MooBigPaned *paned = MOO_BIG_PANED (object);
    int i;

    switch (prop_id)
    {
        case PROP_PANE_ORDER:
            moo_big_paned_set_pane_order (paned, (int*) g_value_get_pointer (value));
            break;

        case PROP_ENABLE_HANDLE_DRAG:
            for (i = 0; i < 4; ++i)
                g_object_set (paned->paned[i],
                              "enable-handle-drag",
                              g_value_get_boolean (value),
                              NULL);
            break;

        case PROP_ENABLE_DETACHING:
            for (i = 0; i < 4; ++i)
                g_object_set (paned->paned[i],
                              "enable-detaching",
                              g_value_get_boolean (value),
                              NULL);
            break;

        case PROP_HANDLE_CURSOR_TYPE:
            for (i = 0; i < 4; ++i)
                g_object_set (paned->paned[i], "handle-cursor-type",
                              (GdkCursorType) g_value_get_enum (value),
                              NULL);
            g_object_notify (object, "handle-cursor-type");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_big_paned_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    MooBigPaned *paned = MOO_BIG_PANED (object);
    GdkCursorType cursor_type;

    switch (prop_id)
    {
        case PROP_PANE_ORDER:
            g_value_set_pointer (value, paned->priv->order);
            break;

        case PROP_HANDLE_CURSOR_TYPE:
            g_object_get (paned->paned[0], "handle-cursor-type",
                          &cursor_type, NULL);
            g_value_set_enum (value, cursor_type);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


GtkWidget *
moo_big_paned_get_pane (MooBigPaned    *paned,
                        MooPanePosition position,
                        int             index_)
{
    g_return_val_if_fail (MOO_IS_BIG_PANED (paned), NULL);
    g_return_val_if_fail (position < 4, NULL);
    return moo_pane_get_child (moo_paned_get_nth_pane (MOO_PANED (paned->paned[position]), index_));
}


/*****************************************************************************/
/* rearranging panes
 */

static void         create_drop_outline     (MooBigPaned    *paned);
static void         get_drop_area           (MooBigPaned    *paned,
                                             MooPaned       *active_child,
                                             MooPanePosition position,
                                             int             index,
                                             GdkRectangle   *rect,
                                             GdkRectangle   *button_rect);
// static void         invalidate_drop_outline (MooBigPaned    *paned);


static GdkRegion *
region_6 (int x0, int y0,
          int x1, int y1,
          int x2, int y2,
          int x3, int y3,
          int x4, int y4,
          int x5, int y5)
{
    GdkPoint points[6];
    points[0].x = x0; points[0].y = y0;
    points[1].x = x1; points[1].y = y1;
    points[2].x = x2; points[2].y = y2;
    points[3].x = x3; points[3].y = y3;
    points[4].x = x4; points[4].y = y4;
    points[5].x = x5; points[5].y = y5;
    return gdk_region_polygon (points, 6, GDK_WINDING_RULE);
}

static GdkRegion *
region_4 (int x0, int y0,
          int x1, int y1,
          int x2, int y2,
          int x3, int y3)
{
    GdkPoint points[4];
    points[0].x = x0; points[0].y = y0;
    points[1].x = x1; points[1].y = y1;
    points[2].x = x2; points[2].y = y2;
    points[3].x = x3; points[3].y = y3;
    return gdk_region_polygon (points, 4, GDK_WINDING_RULE);
}


#define LEFT(rect) ((rect).x)
#define RIGHT(rect) ((rect).x + (rect).width - 1)
#define TOP(rect) ((rect).y)
#define BOTTOM(rect) ((rect).y + (rect).height - 1)

static void
get_drop_zones (MooBigPaned *paned)
{
    int pos;
    GdkRectangle parent;
    GdkRectangle child_rect;
    GdkRectangle button_rect;
    GdkRectangle drop_rect;
    int bbox_size[4];

    g_return_if_fail (paned->priv->dz == NULL);

    paned->priv->dz = g_new0 (DZ, 4);

    for (pos = 0; pos < 4; ++pos)
    {
        bbox_size[pos] = moo_paned_get_button_box_size (MOO_PANED (paned->paned[pos]));
        if (bbox_size[pos] <= 0)
            bbox_size[pos] = 30;
        paned->priv->dz[pos].bbox_size = bbox_size[pos];
    }

    parent = paned->priv->outer->allocation;

    child_rect = parent;
    child_rect.x += bbox_size[MOO_PANE_POS_LEFT];
    child_rect.width -= bbox_size[MOO_PANE_POS_LEFT] +
                        bbox_size[MOO_PANE_POS_RIGHT];
    child_rect.y += bbox_size[MOO_PANE_POS_TOP];
    child_rect.height -= bbox_size[MOO_PANE_POS_TOP] +
                         bbox_size[MOO_PANE_POS_BOTTOM];

    button_rect = parent;
    button_rect.x += 2*bbox_size[MOO_PANE_POS_LEFT];
    button_rect.width -= 2*bbox_size[MOO_PANE_POS_LEFT] +
                         2*bbox_size[MOO_PANE_POS_RIGHT];
    button_rect.y += 2*bbox_size[MOO_PANE_POS_TOP];
    button_rect.height -= 2*bbox_size[MOO_PANE_POS_TOP] +
                          2*bbox_size[MOO_PANE_POS_BOTTOM];

    drop_rect = button_rect;
    drop_rect.x += button_rect.width / 3;
    drop_rect.y += button_rect.height / 3;
    drop_rect.width -= 2 * button_rect.width / 3;
    drop_rect.height -= 2 * button_rect.height / 3;

    paned->priv->dz[MOO_PANE_POS_TOP].bbox_region =
        region_6 (LEFT (child_rect), TOP (parent),
                  RIGHT (child_rect), TOP (parent),
                  RIGHT (child_rect), TOP (child_rect),
                  RIGHT (button_rect), TOP (button_rect),
                  LEFT (button_rect), TOP (button_rect),
                  LEFT (child_rect), TOP (child_rect));
    paned->priv->dz[MOO_PANE_POS_TOP].def_region =
        region_4 (LEFT (button_rect), TOP (button_rect),
                  RIGHT (button_rect), TOP (button_rect),
                  RIGHT (drop_rect), TOP (drop_rect),
                  LEFT (drop_rect), TOP (drop_rect));

    paned->priv->dz[MOO_PANE_POS_LEFT].bbox_region =
        region_6 (LEFT (parent), TOP (child_rect),
                  LEFT (child_rect), TOP (child_rect),
                  LEFT (button_rect), TOP (button_rect),
                  LEFT (button_rect), BOTTOM (button_rect),
                  LEFT (child_rect), BOTTOM (child_rect),
                  LEFT (parent), BOTTOM (child_rect));
    paned->priv->dz[MOO_PANE_POS_LEFT].def_region =
        region_4 (LEFT (button_rect), TOP (button_rect),
                  LEFT (drop_rect), TOP (drop_rect),
                  LEFT (drop_rect), BOTTOM (drop_rect),
                  LEFT (button_rect), BOTTOM (button_rect));

    paned->priv->dz[MOO_PANE_POS_BOTTOM].bbox_region =
        region_6 (LEFT (child_rect), BOTTOM (parent),
                  LEFT (child_rect), BOTTOM (child_rect),
                  LEFT (button_rect), BOTTOM (button_rect),
                  RIGHT (button_rect), BOTTOM (button_rect),
                  RIGHT (child_rect), BOTTOM (child_rect),
                  RIGHT (child_rect), BOTTOM (parent));
    paned->priv->dz[MOO_PANE_POS_BOTTOM].def_region =
        region_4 (LEFT (button_rect), BOTTOM (button_rect),
                  LEFT (drop_rect), BOTTOM (drop_rect),
                  RIGHT (drop_rect), BOTTOM (drop_rect),
                  RIGHT (button_rect), BOTTOM (button_rect));

    paned->priv->dz[MOO_PANE_POS_RIGHT].bbox_region =
        region_6 (RIGHT (parent), TOP (child_rect),
                  RIGHT (child_rect), TOP (child_rect),
                  RIGHT (button_rect), TOP (button_rect),
                  RIGHT (button_rect), BOTTOM (button_rect),
                  RIGHT (child_rect), BOTTOM (child_rect),
                  RIGHT (parent), BOTTOM (child_rect));
    paned->priv->dz[MOO_PANE_POS_RIGHT].def_region =
        region_4 (RIGHT (button_rect), TOP (button_rect),
                  RIGHT (drop_rect), TOP (drop_rect),
                  RIGHT (drop_rect), BOTTOM (drop_rect),
                  RIGHT (button_rect), BOTTOM (button_rect));

    paned->priv->drop_region = NULL;
    paned->priv->drop_region_is_buttons = FALSE;
}

#undef LEFT
#undef RIGHT
#undef TOP
#undef BOTTOM


static void
handle_drag_start (G_GNUC_UNUSED MooPaned *child,
                   G_GNUC_UNUSED GtkWidget *pane_widget,
                   MooBigPaned *paned)
{
    g_return_if_fail (GTK_WIDGET_REALIZED (paned->priv->outer));

    g_signal_connect (paned->priv->outer, "expose-event",
                      G_CALLBACK (moo_big_paned_expose), paned);

    paned->priv->drop_pos = -1;
    get_drop_zones (paned);
}


static gboolean
get_new_button_index (MooBigPaned *paned,
                      MooPaned    *active_child,
                      int          x,
                      int          y)
{
    int new_button;
    MooPaned *child;

    g_assert (paned->priv->drop_pos >= 0);
    child = MOO_PANED (paned->paned[paned->priv->drop_pos]);

    new_button = _moo_paned_get_button (child, x, y,
                                        paned->priv->outer->window);

    if (child == active_child)
    {
        int n_buttons = moo_paned_n_panes (child);
        if (new_button >= n_buttons || new_button < 0)
            new_button = n_buttons - 1;
    }

    if (new_button != paned->priv->drop_button_index)
    {
        paned->priv->drop_button_index = new_button;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void
get_default_button_index (MooBigPaned *paned,
                          MooPaned    *active_child)
{
    MooPaned *child;

    g_assert (paned->priv->drop_pos >= 0);
    child = MOO_PANED (paned->paned[paned->priv->drop_pos]);

    if (child == active_child)
        paned->priv->drop_button_index =
            _moo_paned_get_open_pane_index (child);
    else
        paned->priv->drop_button_index = -1;
}

static gboolean
get_new_drop_position (MooBigPaned *paned,
                       MooPaned    *active_child,
                       int          x,
                       int          y)
{
    int pos;
    gboolean changed = FALSE;

    if (paned->priv->drop_region)
    {
        g_assert (paned->priv->drop_pos >= 0);

        if (gdk_region_point_in (paned->priv->drop_region, x, y))
        {
            if (!paned->priv->drop_region_is_buttons)
                return FALSE;
            else
                return get_new_button_index (paned, active_child, x, y);
        }

        paned->priv->drop_pos = -1;
        paned->priv->drop_region = NULL;
        changed = TRUE;
    }

    for (pos = 0; pos < 4; ++pos)
    {
        if (gdk_region_point_in (paned->priv->dz[pos].def_region, x, y))
        {
            paned->priv->drop_pos = pos;
            paned->priv->drop_region = paned->priv->dz[pos].def_region;
            paned->priv->drop_region_is_buttons = FALSE;
            get_default_button_index (paned, active_child);
            return TRUE;
        }

        if (gdk_region_point_in (paned->priv->dz[pos].bbox_region, x, y))
        {
            paned->priv->drop_pos = pos;
            paned->priv->drop_region = paned->priv->dz[pos].bbox_region;
            paned->priv->drop_region_is_buttons = TRUE;
            get_new_button_index (paned, active_child, x, y);
            return TRUE;
        }
    }

    return changed;
}

static void
handle_drag_motion (MooPaned       *child,
                    G_GNUC_UNUSED GtkWidget *pane_widget,
                    MooBigPaned    *paned)
{
    int x, y;

    g_return_if_fail (GTK_WIDGET_REALIZED (paned->priv->outer));

    gdk_window_get_pointer (paned->priv->outer->window, &x, &y, NULL);

    if (!get_new_drop_position (paned, child, x, y))
        return;

    if (paned->priv->drop_outline)
    {
        gdk_window_set_user_data (paned->priv->drop_outline, NULL);
        gdk_window_destroy (paned->priv->drop_outline);
        paned->priv->drop_outline = NULL;
    }

    if (paned->priv->drop_pos >= 0)
    {
        get_drop_area (paned, child,
                       (MooPanePosition) paned->priv->drop_pos,
                       paned->priv->drop_button_index,
                       &paned->priv->drop_rect,
                       &paned->priv->drop_button_rect);
        create_drop_outline (paned);
    }
}


static void
cleanup_drag (MooBigPaned *paned)
{
    int pos;

    if (paned->priv->drop_outline)
    {
        gdk_window_set_user_data (paned->priv->drop_outline, NULL);
        gdk_window_destroy (paned->priv->drop_outline);
        paned->priv->drop_outline = NULL;
    }

    paned->priv->drop_pos = -1;
    paned->priv->drop_region = NULL;

    g_signal_handlers_disconnect_by_func (paned->priv->outer,
                                          (gpointer) moo_big_paned_expose,
                                          paned);

    for (pos = 0; pos < 4; ++pos)
    {
        gdk_region_destroy (paned->priv->dz[pos].bbox_region);
        gdk_region_destroy (paned->priv->dz[pos].def_region);
    }

    g_free (paned->priv->dz);
    paned->priv->dz = NULL;
}

static void
handle_drag_end (MooPaned    *child,
                 GtkWidget   *pane_widget,
                 gboolean     drop,
                 MooBigPaned *paned)
{
    int x, y;
    MooPanePosition new_pos;
    int new_index;

    g_return_if_fail (GTK_WIDGET_REALIZED (paned->priv->outer));

    if (!drop)
    {
        cleanup_drag (paned);
        return;
    }

    gdk_window_get_pointer (paned->priv->outer->window, &x, &y, NULL);
    get_new_drop_position (paned, child, x, y);

    if (paned->priv->drop_pos < 0)
    {
        cleanup_drag (paned);
        return;
    }

    new_pos = (MooPanePosition) paned->priv->drop_pos;
    new_index = paned->priv->drop_button_index;
    cleanup_drag (paned);

    moo_big_paned_reorder_pane (paned, pane_widget, new_pos, new_index);
}


static void
get_drop_area (MooBigPaned    *paned,
               MooPaned       *active_child,
               MooPanePosition position,
               int             index,
               GdkRectangle   *rect,
               GdkRectangle   *button_rect)
{
    int width, height, size = 0;
    MooPanePosition active_position;

    width = paned->priv->outer->allocation.width;
    height = paned->priv->outer->allocation.height;

    g_object_get (active_child, "pane-position", &active_position, NULL);
    g_return_if_fail (active_position < 4);

    if (active_position == position)
    {
        size = moo_paned_get_pane_size (active_child) +
                moo_paned_get_button_box_size (active_child);
    }
    else
    {
        switch (position)
        {
            case MOO_PANE_POS_LEFT:
            case MOO_PANE_POS_RIGHT:
                size = width / 3;
                break;
            case MOO_PANE_POS_TOP:
            case MOO_PANE_POS_BOTTOM:
                size = height / 3;
                break;
        }
    }

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
        case MOO_PANE_POS_RIGHT:
            rect->y = paned->priv->outer->allocation.y;
            rect->width = size;
            rect->height = height;
            break;
        case MOO_PANE_POS_TOP:
        case MOO_PANE_POS_BOTTOM:
            rect->x = paned->priv->outer->allocation.x;
            rect->width = width;
            rect->height = size;
            break;
    }

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
            rect->x = paned->priv->outer->allocation.x;
            break;
        case MOO_PANE_POS_RIGHT:
            rect->x = paned->priv->outer->allocation.x + width - size;
            break;
        case MOO_PANE_POS_TOP:
            rect->y = paned->priv->outer->allocation.y;
            break;
        case MOO_PANE_POS_BOTTOM:
            rect->y = paned->priv->outer->allocation.y + height - size;
            break;
    }

    _moo_paned_get_button_position (MOO_PANED (paned->paned[position]),
                                    index, button_rect,
                                    paned->priv->outer->window);
}


#define RECT_POINT_IN(rect,x,y) (x < (rect)->x + (rect)->width &&   \
                                 y < (rect)->height + (rect)->y &&  \
                                 x >= (rect)->x && y >= (rect)->y)

// static int
// get_drop_position (MooBigPaned *paned,
//                    MooPaned    *child,
//                    int          x,
//                    int          y,
//                    int         *button_index)
// {
//     int width, height, i;
//     MooPanePosition position;
//     GdkRectangle rect, button_rect;
//
//     *button_index = -1;
//
//     width = paned->priv->outer->allocation.width;
//     height = paned->priv->outer->allocation.height;
//
//     if (x < paned->priv->outer->allocation.x ||
//         x >= paned->priv->outer->allocation.x + width ||
//         y < paned->priv->outer->allocation.y ||
//         y >= paned->priv->outer->allocation.y + height)
//             return -1;
//
//     g_object_get (child, "pane-position", &position, NULL);
//     g_return_val_if_fail (position < 4, -1);
//
//     get_drop_area (paned, child, position, -1, &rect, &button_rect);
//
//     if (RECT_POINT_IN (&rect, x, y))
//         return position;
//
//     for (i = 0; i < 4; ++i)
//     {
//         if (paned->priv->order[i] == position)
//             continue;
//
//         get_drop_area (paned, child, paned->priv->order[i], -1,
//                        &rect, &button_rect);
//
//         if (RECT_POINT_IN (&rect, x, y))
//             return paned->priv->order[i];
//     }
//
//     return -1;
// }


// static void
// invalidate_drop_outline (MooBigPaned *paned)
// {
//     GdkRectangle line;
//     GdkRegion *outline;
//
//     outline = gdk_region_new ();
//
//     line.x = paned->priv->drop_rect.x;
//     line.y = paned->priv->drop_rect.y;
//     line.width = 2;
//     line.height = paned->priv->drop_rect.height;
//     gdk_region_union_with_rect (outline, &line);
//
//     line.x = paned->priv->drop_rect.x;
//     line.y = paned->priv->drop_rect.y + paned->priv->drop_rect.height;
//     line.width = paned->priv->drop_rect.width;
//     line.height = 2;
//     gdk_region_union_with_rect (outline, &line);
//
//     line.x = paned->priv->drop_rect.x + paned->priv->drop_rect.width;
//     line.y = paned->priv->drop_rect.y;
//     line.width = 2;
//     line.height = paned->priv->drop_rect.height;
//     gdk_region_union_with_rect (outline, &line);
//
//     line.x = paned->priv->drop_rect.x;
//     line.y = paned->priv->drop_rect.y;
//     line.width = paned->priv->drop_rect.width;
//     line.height = 2;
//     gdk_region_union_with_rect (outline, &line);
//
//     gdk_window_invalidate_region (paned->priv->outer->window, outline, TRUE);
//
//     gdk_region_destroy (outline);
// }

static gboolean
moo_big_paned_expose (GtkWidget      *widget,
                      GdkEventExpose *event,
                      MooBigPaned    *paned)
{
    GTK_WIDGET_CLASS(G_OBJECT_GET_CLASS (widget))->expose_event (widget, event);

    if (paned->priv->drop_pos >= 0)
    {
        g_return_val_if_fail (paned->priv->drop_outline != NULL, FALSE);
        gdk_draw_rectangle (paned->priv->drop_outline,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            FALSE, 0, 0,
                            paned->priv->drop_rect.width - 1,
                            paned->priv->drop_rect.height - 1);
        gdk_draw_rectangle (paned->priv->drop_outline,
                            widget->style->fg_gc[GTK_STATE_NORMAL],
                            FALSE, 1, 1,
                            paned->priv->drop_rect.width - 3,
                            paned->priv->drop_rect.height - 3);
    }

    return FALSE;
}

static GdkBitmap *
create_rect_mask (int           width,
                  int           height,
                  GdkRectangle *rect)
{
    GdkBitmap *bitmap;
    GdkGC *gc;
    GdkColor white = {0, 0, 0, 0};
    GdkColor black = {1, 1, 1, 1};

    bitmap = gdk_pixmap_new (NULL, width, height, 1);
    gc = gdk_gc_new (bitmap);

    gdk_gc_set_foreground (gc, &white);
    gdk_draw_rectangle (bitmap, gc, TRUE, 0, 0,
                        width, height);

    gdk_gc_set_foreground (gc, &black);
    gdk_draw_rectangle (bitmap, gc, FALSE, 0, 0,
                        width - 1, height - 1);
    gdk_draw_rectangle (bitmap, gc, FALSE, 1, 1,
                        width - 3, height - 3);

    gdk_draw_rectangle (bitmap, gc, FALSE,
                        rect->x, rect->y,
                        rect->width, rect->height);
    gdk_draw_rectangle (bitmap, gc, FALSE,
                        rect->x + 1, rect->y + 1,
                        rect->width - 2, rect->height - 2);

    g_object_unref (gc);
    return bitmap;
}

static void
create_drop_outline (MooBigPaned *paned)
{
    static GdkWindowAttr attributes;
    int attributes_mask;
    GdkBitmap *mask;
    GdkRectangle button_rect;

    g_return_if_fail (paned->priv->drop_outline == NULL);

    attributes.x = paned->priv->drop_rect.x;
    attributes.y = paned->priv->drop_rect.y;
    attributes.width = paned->priv->drop_rect.width;
    attributes.height = paned->priv->drop_rect.height;
    attributes.window_type = GDK_WINDOW_CHILD;

    attributes.visual = gtk_widget_get_visual (paned->priv->outer);
    attributes.colormap = gtk_widget_get_colormap (paned->priv->outer);
    attributes.wclass = GDK_INPUT_OUTPUT;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    paned->priv->drop_outline = gdk_window_new (paned->priv->outer->window,
                                                &attributes, attributes_mask);
    gdk_window_set_user_data (paned->priv->drop_outline, paned);

    button_rect = paned->priv->drop_button_rect;
    button_rect.x -= paned->priv->drop_rect.x;
    button_rect.y -= paned->priv->drop_rect.y;
    mask = create_rect_mask (paned->priv->drop_rect.width,
                             paned->priv->drop_rect.height,
                             &button_rect);
    gdk_window_shape_combine_mask (paned->priv->drop_outline, mask, 0, 0);
    g_object_unref (mask);

    gdk_window_show (paned->priv->drop_outline);
}


/*************************************************************************/
/* Config
 */

#define CONFIG_STRING_MAGIC "mbpconfig-1.0 "
#define CONFIG_STRING_MAGIC_LEN strlen (CONFIG_STRING_MAGIC)

static MooBigPanedConfig *
config_new (void)
{
    MooBigPanedConfig *config;
    int pos;

    config = g_new0 (MooBigPanedConfig, 1);

    for (pos = 0; pos < 4; pos++)
    {
        config->paned[pos].order = NULL;
        config->paned[pos].size = -1;
        config->paned[pos].sticky = -1;
        config->paned[pos].active = NULL;
    }

    config->panes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                           (GDestroyNotify) moo_pane_params_free);

    return config;
}

static void
config_free (MooBigPanedConfig *config)
{
    if (config)
    {
        int pos;

        for (pos = 0; pos < 4; pos++)
        {
            g_slist_foreach (config->paned[pos].order, (GFunc) g_free, NULL);
            g_slist_free (config->paned[pos].order);
            g_free (config->paned[pos].active);
        }

        g_hash_table_destroy (config->panes);
        g_free (config);
    }
}

static gboolean
parse_int (const char *string,
           int        *valuep)
{
    long value;
    char *endptr;

    errno = 0;
    value = strtol (string, &endptr, 10);

    if (errno || endptr == string || *endptr != 0 ||
        value < G_MININT || value > G_MAXINT)
            return FALSE;

    *valuep = value;
    return TRUE;
}

static gboolean
parse_bool (const char *string,
            gboolean   *value)
{
    if (!string[0] || string[1] || (string[0] != '0' && string[0] != '1'))
        return FALSE;

    *value = string[0] == '1';
    return TRUE;
}


static gboolean
parse_paned_config (const char     *string,
                    MooPanedConfig *config)
{
    char **tokens, **p;

    tokens = g_strsplit (string, ",", 0);
    if (g_strv_length (tokens) < 3)
    {
        g_critical ("%s: invalid paned config string '%s'",
                    G_STRFUNC, string);
        goto error;
    }

    if (!parse_int (tokens[0], &config->size) || config->size < -1 || config->size > 100000)
    {
        g_critical ("%s: invalid size '%s' in paned config string '%s'",
                    G_STRFUNC, tokens[0], string);
        goto error;
    }

    if (!parse_int (tokens[1], &config->sticky) ||
        config->sticky < -1 || config->sticky > 1)
    {
        g_critical ("%s: invalid boolean '%s' in paned config string '%s'",
                    G_STRFUNC, tokens[1], string);
        goto error;
    }

    if (tokens[2][0])
        config->active = g_strdup (tokens[2]);
    else
        config->active = NULL;

    for (p = tokens + 3; *p; ++p)
        config->order = g_slist_prepend (config->order, g_strdup (*p));
    config->order = g_slist_reverse (config->order);

    g_strfreev (tokens);
    return TRUE;

error:
    g_strfreev (tokens);
    return FALSE;
}

static void
paned_config_serialize (MooPanedConfig *config,
                        GString        *string)
{
    GSList *l;

    g_string_append_printf (string, "%d,%d,%s", config->size, config->sticky,
                            config->active ? config->active : "");

    for (l = config->order; l != NULL; l = l->next)
    {
        char *id = (char*) l->data;
        g_string_append_printf (string, ",%s", id);
    }
}


static gboolean
parse_pane_config (const char *string,
                   GHashTable *hash)
{
    char **tokens = NULL;
    const char *colon;
    MooPaneParams params = {{-1, -1, -1, -1}, 0, 0, 0};
    gboolean value[3];

    if (!(colon = strchr (string, ':')) || colon == string)
    {
        g_critical ("%s: invalid pane config string '%s'",
                    G_STRFUNC, string);
        goto error;
    }

    tokens = g_strsplit (colon + 1, ",", 0);
    if (g_strv_length (tokens) != 7)
    {
        g_critical ("%s: invalid pane config string '%s'",
                    G_STRFUNC, string);
        goto error;
    }

    if (!parse_int (tokens[0], &params.window_position.x) ||
        !parse_int (tokens[1], &params.window_position.y) ||
        !parse_int (tokens[2], &params.window_position.width) ||
        !parse_int (tokens[3], &params.window_position.height) ||
        params.window_position.width < -1 || params.window_position.width > 100000 ||
        params.window_position.height < -1 || params.window_position.height > 100000)
    {
        g_critical ("%s: invalid window position in pane config string '%s'",
                    G_STRFUNC, string);
        goto error;
    }

    if (!parse_bool (tokens[4], &value[0]) ||
        !parse_bool (tokens[5], &value[1]) ||
        !parse_bool (tokens[6], &value[2]))
    {
        g_critical ("%s: invalid parameters in pane config string '%s'",
                    G_STRFUNC, string);
        goto error;
    }

    params.detached = value[0];
    params.maximized = value[1];
    params.keep_on_top = value[2];

    g_hash_table_insert (hash,
                         g_strndup (string, colon - string),
                         moo_pane_params_copy (&params));

    g_strfreev (tokens);
    return TRUE;

error:
    g_strfreev (tokens);
    return FALSE;
}

static void
pane_config_serialize (const char    *id,
                       MooPaneParams *params,
                       GString       *string)
{
    g_string_append_printf (string, ";%s:%d,%d,%d,%d,%s,%s,%s",
                            id,
                            params->window_position.x,
                            params->window_position.y,
                            params->window_position.width,
                            params->window_position.height,
                            params->detached ? "1" : "0",
                            params->maximized ? "1" : "0",
                            params->keep_on_top ? "1" : "0");
}


static MooBigPanedConfig *
config_parse (const char *string)
{
    MooBigPanedConfig *config = NULL;
    char **tokens = NULL;
    char **p;
    int pos;

    g_return_val_if_fail (string != NULL, NULL);

    if (strncmp (string, CONFIG_STRING_MAGIC, CONFIG_STRING_MAGIC_LEN) != 0)
    {
        g_critical ("%s: invalid magic in config string '%s'", G_STRFUNC, string);
        goto error;
    }

    string += CONFIG_STRING_MAGIC_LEN;
    tokens = g_strsplit (string, ";", 0);
    if (g_strv_length (tokens) < 4)
    {
        g_critical ("%s: invalid string '%s'", G_STRFUNC, string);
        goto error;
    }

    config = config_new ();

    for (pos = 0; pos < 4; ++pos)
        if (!parse_paned_config (tokens[pos], &config->paned[pos]))
            goto error;

    for (p = tokens + 4; *p; ++p)
        if (!parse_pane_config (*p, config->panes))
            goto error;

    g_strfreev (tokens);
    return config;

error:
    g_strfreev (tokens);
    config_free (config);
    return NULL;
}

static char *
config_serialize (MooBigPanedConfig *config)
{
    GString *string;
    int pos;

    g_return_val_if_fail (config != NULL, NULL);

    string = g_string_new (CONFIG_STRING_MAGIC);

    for (pos = 0; pos < 4; ++pos)
    {
        if (pos != 0)
            g_string_append (string, ";");

        paned_config_serialize (&config->paned[pos], string);
    }

    g_hash_table_foreach (config->panes, (GHFunc) pane_config_serialize, string);

    return g_string_free (string, FALSE);
}
