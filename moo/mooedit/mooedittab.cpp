/**
 * class:MooEditTab: (parent GtkVBox) (moo.doc-object-name tab): document tab object
 **/

#include "mooedittab-impl.h"
#include "mooeditview-impl.h"
#include "mooeditwindow-impl.h"
#include "mooedit-impl.h"
#include <mooutils/moocompat.h>

struct MooEditTab
{
    GtkVBox base;

    MooEditProgress *progress;

    GtkWidget *hpaned;
    GtkWidget *vpaned1;
    GtkWidget *vpaned2;

    MooEdit *doc;
    MooEditView *active_view;
};

MOO_DEFINE_GOBJ_TRAITS(MooEditTab, MOO_TYPE_EDIT_TAB)

struct MooEditTabClass
{
    GtkVBoxClass base_class;
};

MOO_DEFINE_OBJECT_ARRAY (MooEditTab, moo_edit_tab)

G_DEFINE_TYPE (MooEditTab, moo_edit_tab, GTK_TYPE_VBOX)

/**************************************************************************************************
 *
 * MooEditTab
 *
 *************************************************************************************************/

static void
moo_edit_tab_init (MooEditTab *tab)
{
    gtk_box_set_homogeneous (GTK_BOX (tab), FALSE);
    tab->hpaned = gtk_hpaned_new ();
    tab->vpaned1 = gtk_vpaned_new ();
    tab->vpaned2 = gtk_vpaned_new ();
    gtk_paned_pack1 (GTK_PANED (tab->hpaned), tab->vpaned1, TRUE, FALSE);
    gtk_paned_pack2 (GTK_PANED (tab->hpaned), tab->vpaned2, TRUE, FALSE);
    gtk_widget_show (tab->hpaned);
    gtk_box_pack_start (GTK_BOX (tab), tab->hpaned, TRUE, TRUE, 0);
}

static void
moo_edit_tab_dispose (GObject *object)
{
    MooEditTab *tab = MOO_EDIT_TAB (object);

    if (tab->doc)
    {
        g_object_unref (tab->doc);
        tab->doc = NULL;
    }

    G_OBJECT_CLASS (moo_edit_tab_parent_class)->dispose (object);
}

static void
moo_edit_tab_class_init (MooEditTabClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = moo_edit_tab_dispose;
}

static GtkWidget *
create_view_scrolled_window (MooEditView *view)
{
    GtkWidget *swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin),
                                         GTK_SHADOW_ETCHED_IN);

    gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (view));
    gtk_widget_show_all (swin);
    return swin;
}

static GtkWidget *
create_view_in_scrolled_window (MooEditTab *tab)
{
    MooEditView *view = _moo_edit_view_new (tab->doc);
    _moo_edit_view_set_tab (view, tab);
    return create_view_scrolled_window (view);
}

MooEditTab *
_moo_edit_tab_new (MooEdit *doc)
{
    MooEditView *view;
    MooEditTab *tab;
    GtkWidget *swin;

    g_return_val_if_fail (moo_edit_get_n_views (doc) == 1, NULL);

    tab = g::object_new<MooEditTab>();
    tab->doc = g::object_ref (doc);

    view = moo_edit_get_view (doc);
    _moo_edit_view_set_tab (view, tab);

    swin = create_view_scrolled_window (view);
    gtk_paned_pack1 (GTK_PANED (tab->vpaned1), swin, TRUE, FALSE);
    gtk_widget_show (tab->vpaned1);
    gtk_widget_show (GTK_WIDGET (tab));

    return tab;
}

/**
 * moo_edit_tab_get_doc:
 **/
MooEdit *
moo_edit_tab_get_doc (MooEditTab *tab)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), NULL);
    return tab->doc;
}

/**
 * moo_edit_tab_get_views:
 *
 * Returns: (transfer full)
 **/
MooEditViewArray *
moo_edit_tab_get_views (MooEditTab *tab)
{
    MooEditViewArray *views;
    int i;

    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), NULL);

    views = moo_edit_view_array_new ();

    for (i = 0; i < 2; ++i)
    {
        GList *children = gtk_container_get_children (GTK_CONTAINER (i == 0 ? tab->vpaned1 : tab->vpaned2));
        while (children)
        {
            MooEditView *view = MOO_EDIT_VIEW (gtk_bin_get_child (GTK_BIN (children->data)));
            moo_edit_view_array_append (views, view);
            children = g_list_delete_link (children, children);
        }
    }

    return views;
}

/**
 * moo_edit_tab_get_active_view:
 **/
MooEditView *
moo_edit_tab_get_active_view (MooEditTab *tab)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), NULL);

    if (!tab->active_view)
    {
        int i;
        for (i = 0; !tab->active_view && i < 2; ++i)
        {
            GList *children = gtk_container_get_children (GTK_CONTAINER (i == 0 ? tab->vpaned1 : tab->vpaned2));
            if (children)
                tab->active_view = MOO_EDIT_VIEW (gtk_bin_get_child (GTK_BIN (children->data)));
            g_list_free (children);
        }
    }

    return tab->active_view;
}

void
_moo_edit_tab_set_focused_view (MooEditTab  *tab,
                                MooEditView *view)
{
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    g_return_if_fail (moo_edit_view_get_tab (view) == tab);
    tab->active_view = view;
    _moo_edit_window_set_active_tab (moo_edit_tab_get_window (tab), tab);
}

/**
 * moo_edit_tab_get_window:
 **/
MooEditWindow *
moo_edit_tab_get_window (MooEditTab *tab)
{
    GtkWidget *toplevel;
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), NULL);
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tab));
    if (!toplevel || !GTK_WIDGET_TOPLEVEL (toplevel))
        return NULL;
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (toplevel), NULL);
    return MOO_EDIT_WINDOW (toplevel);
}

static MooEditView *
moo_edit_tab_get_view (MooEditTab *tab, int ipaned, int iview)
{
    GtkWidget *paned;
    GtkWidget *swin;
    g_return_val_if_fail (ipaned == 1 || ipaned == 2, NULL);
    g_return_val_if_fail (iview == 1 || iview == 2, NULL);
    if (ipaned == 1)
        paned = tab->vpaned1;
    else
        paned = tab->vpaned2;
    if (iview == 1)
        swin = gtk_paned_get_child1 (GTK_PANED (paned));
    else
        swin = gtk_paned_get_child2 (GTK_PANED (paned));
    return swin ? MOO_EDIT_VIEW (gtk_bin_get_child (GTK_BIN (swin))) : NULL;
}

void
_moo_edit_tab_focus_next_view (MooEditTab *tab)
{
    int i = 0;
    int cur;
    MooEditView *focus_chain[4] = { 0 };
    MooEditView *view;

    g_return_if_fail (MOO_IS_EDIT_TAB (tab));

    focus_chain[i++] = moo_edit_tab_get_view (tab, 1, 1);

    if (GTK_WIDGET_VISIBLE (tab->vpaned2))
    {
        if ((view = moo_edit_tab_get_view (tab, 2, 1)))
            focus_chain[i++] = view;
        g_assert (view != NULL);
        if ((view = moo_edit_tab_get_view (tab, 2, 2)))
            focus_chain[i++] = view;
    }

    if ((view = moo_edit_tab_get_view (tab, 1, 2)))
        focus_chain[i++] = view;

    for (cur = 0; cur < 4; ++cur)
        if (focus_chain[cur] && GTK_WIDGET_HAS_FOCUS (focus_chain[cur]))
            break;

    cur++;
    if (cur >= 4 || !focus_chain[cur])
        cur = 0;

    gtk_widget_grab_focus (GTK_WIDGET (focus_chain[cur]));
}

/*
 * +-------------+
 * |      |      |
 * |      |      |
 * |      |      |
 * |      |      |
 * |      |      |
 * +-------------+
 */
gboolean
_moo_edit_tab_get_split_horizontal (MooEditTab *tab)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), FALSE);
    return GTK_WIDGET_VISIBLE (tab->vpaned1) &&
           GTK_WIDGET_VISIBLE (tab->vpaned2);
}

/*
 * +-------------+
 * |      |      |
 * |      |      |
 * |      |      |
 * |      |      |
 * |      |      |
 * +-------------+
 */
gboolean
_moo_edit_tab_set_split_horizontal (MooEditTab *tab,
                                    gboolean    split)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), FALSE);

    if (!!split == _moo_edit_tab_get_split_horizontal (tab))
        return FALSE;

    if (!split)
    {
        gboolean has_focus;
        MooEditView *view1 = moo_edit_tab_get_view (tab, 2, 1);
        MooEditView *view2 = moo_edit_tab_get_view (tab, 2, 2);

        g_assert (view1 != NULL);

        has_focus = (view1 && GTK_WIDGET_HAS_FOCUS (view1)) ||
                    (view2 && GTK_WIDGET_HAS_FOCUS (view2));

        if (view1)
        {
            _moo_edit_remove_view (tab->doc, view1);
            gtk_container_remove (GTK_CONTAINER (tab->vpaned2), GTK_WIDGET (view1)->parent);
        }

        if (view2)
        {
            _moo_edit_remove_view (tab->doc, view2);
            gtk_container_remove (GTK_CONTAINER (tab->vpaned2), GTK_WIDGET (view2)->parent);
        }

        gtk_widget_hide (tab->vpaned2);

        if (has_focus)
            gtk_widget_grab_focus (GTK_WIDGET (moo_edit_tab_get_view (tab, 1, 1)));
    }
    else
    {
        GtkWidget *swin;
        MooEditView *view1 = moo_edit_tab_get_view (tab, 1, 1);
        MooEditView *view2 = moo_edit_tab_get_view (tab, 1, 2);

        g_assert (view1 != NULL);
        g_assert (!GTK_WIDGET_VISIBLE (tab->vpaned2));
        g_assert (!gtk_container_get_children (GTK_CONTAINER (tab->vpaned2)));

        if (view1)
        {
            swin = create_view_in_scrolled_window (tab);
            gtk_paned_pack1 (GTK_PANED (tab->vpaned2), swin, TRUE, FALSE);
        }

        if (view2)
        {
            swin = create_view_in_scrolled_window (tab);
            gtk_paned_pack2 (GTK_PANED (tab->vpaned2), swin, TRUE, FALSE);
        }

        gtk_widget_show_all (tab->vpaned2);
    }

    return TRUE;
}

/*
 * +--------------+
 * |              |
 * |              |
 * |--------------|
 * |              |
 * |              |
 * +--------------+
 */
gboolean
_moo_edit_tab_get_split_vertical (MooEditTab *tab)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), FALSE);
    return gtk_paned_get_child1 (GTK_PANED (tab->vpaned1)) &&
           gtk_paned_get_child2 (GTK_PANED (tab->vpaned1));
}

/*
 * +--------------+
 * |              |
 * |              |
 * |--------------|
 * |              |
 * |              |
 * +--------------+
 */
gboolean
_moo_edit_tab_set_split_vertical (MooEditTab *tab,
                                  gboolean    split)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), FALSE);

    if (!!split == _moo_edit_tab_get_split_vertical (tab))
        return FALSE;

    if (!split)
    {
        gboolean has_focus;
        MooEditView *view1;
        MooEditView *view2;

        view1 = moo_edit_tab_get_view (tab, 1, 2);
        view2 = moo_edit_tab_get_view (tab, 2, 2);

        g_assert (view1 != NULL);

        has_focus = (view1 && GTK_WIDGET_HAS_FOCUS (view1)) ||
                    (view2 && GTK_WIDGET_HAS_FOCUS (view2));

        if (view1)
        {
            _moo_edit_remove_view (tab->doc, view1);
            gtk_container_remove (GTK_CONTAINER (tab->vpaned1), GTK_WIDGET (view1)->parent);
        }

        if (view2)
        {
            _moo_edit_remove_view (tab->doc, view2);
            gtk_container_remove (GTK_CONTAINER (tab->vpaned2), GTK_WIDGET (view2)->parent);
        }

        if (has_focus)
            gtk_widget_grab_focus (GTK_WIDGET (moo_edit_tab_get_view (tab, 1, 1)));
    }
    else
    {
        GtkWidget *swin;

        swin = create_view_in_scrolled_window (tab);
        gtk_paned_pack2 (GTK_PANED (tab->vpaned1), swin, TRUE, FALSE);

        if (GTK_WIDGET_VISIBLE (tab->vpaned2))
        {
            g_assert (gtk_paned_get_child1 (GTK_PANED (tab->vpaned2)) != NULL);
            swin = create_view_in_scrolled_window (tab);
            gtk_paned_pack2 (GTK_PANED (tab->vpaned2), swin, TRUE, FALSE);
        }
    }

    return TRUE;
}


MooEditProgress *
_moo_edit_tab_create_progress (MooEditTab *tab)
{
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), NULL);
    g_return_val_if_fail (!tab->progress, tab->progress);

    tab->progress = _moo_edit_progress_new ();
    gtk_box_pack_start (GTK_BOX (tab), GTK_WIDGET (tab->progress), FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (tab), GTK_WIDGET (tab->progress), 0);

    return tab->progress;
}

void
_moo_edit_tab_destroy_progress (MooEditTab *tab)
{
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));
    g_return_if_fail (tab->progress != NULL);
    gtk_widget_destroy (GTK_WIDGET (tab->progress));
    tab->progress = NULL;
}
