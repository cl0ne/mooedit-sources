#ifndef MOO_GTK_H
#define MOO_GTK_H

#include <mooglib/moo-glib.h>
#include <gtk/gtk.h>

#ifndef GTK_WIDGET_REALIZED
#define GTK_WIDGET_REALIZED(w) (gtk_widget_get_realized (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_MAPPED
#define GTK_WIDGET_MAPPED(w) (gtk_widget_get_mapped (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE(w) (gtk_widget_get_visible (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_DRAWABLE
#define GTK_WIDGET_DRAWABLE(w) (gtk_widget_is_drawable (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_SENSITIVE
#define GTK_WIDGET_SENSITIVE(w) (gtk_widget_get_sensitive (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_HAS_FOCUS
#define GTK_WIDGET_HAS_FOCUS(w) (gtk_widget_has_focus (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_CAN_FOCUS
#define GTK_WIDGET_CAN_FOCUS(w) (gtk_widget_get_can_focus (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_IS_SENSITIVE
#define GTK_WIDGET_IS_SENSITIVE(w) (gtk_widget_is_sensitive (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_TOPLEVEL
#define GTK_WIDGET_TOPLEVEL(w) (gtk_widget_is_toplevel (GTK_WIDGET (w)))
#endif

#ifndef GTK_WIDGET_STATE
#define GTK_WIDGET_STATE(w) (gtk_widget_get_state (GTK_WIDGET (w)))
#endif


#if defined(GTK_DISABLE_DEPRECATED)

inline static void
_moo_noop_gtk_toolbar_set_tooltips (G_GNUC_UNUSED GtkToolbar *toolbar,
                                    G_GNUC_UNUSED gboolean    enable)
{
}

#define gtk_toolbar_set_tooltips _moo_noop_gtk_toolbar_set_tooltips

#endif /* GTK_DISABLE_DEPRECATED */


#if defined(GTK_DISABLE_DEPRECATED)

inline static void
_moo_gtk_action_connect_proxy (GtkAction *action,
                               GtkWidget *proxy)
{
    gtk_activatable_set_related_action (GTK_ACTIVATABLE (proxy), action);
}

#define gtk_action_connect_proxy _moo_gtk_action_connect_proxy

#endif /* GTK_DISABLE_DEPRECATED */

#if GTK_CHECK_VERSION(2,24,0)
#undef GTK_WIDGET_REALIZED
#define GTK_WIDGET_REALIZED(w) gtk_widget_get_realized (GTK_WIDGET (w))
#endif // GTK_CHECK_VERSION(2,24,0)

#if GTK_CHECK_VERSION(2,22,0) && defined(GTK_DISABLE_DEPRECATED)

inline static void
_moo_noop_gtk_dialog_set_has_separator (G_GNUC_UNUSED GtkDialog *dialog,
                                        G_GNUC_UNUSED gboolean setting)
{
}

#define gtk_dialog_set_has_separator _moo_noop_gtk_dialog_set_has_separator

#define GTK_WIDGET_SET_CAN_FOCUS(w) gtk_widget_set_can_focus (GTK_WIDGET (w), TRUE)
#define GTK_WIDGET_SET_REALIZED(w) gtk_widget_set_realized (GTK_WIDGET (w), TRUE)
#define GTK_WIDGET_SET_NO_WINDOW(w) gtk_widget_set_has_window (GTK_WIDGET (w), FALSE)
#define GTK_WIDGET_SET_MAPPED(w) gtk_widget_set_mapped (GTK_WIDGET (w), TRUE)

#define GTK_WIDGET_UNSET_CAN_FOCUS(w) gtk_widget_set_can_focus (GTK_WIDGET (w), FALSE)
#define GTK_WIDGET_UNSET_REALIZED(w) gtk_widget_set_realized (GTK_WIDGET (w), FALSE)
#define GTK_WIDGET_UNSET_NO_WINDOW(w) gtk_widget_set_has_window (GTK_WIDGET (w), TRUE)
#define GTK_WIDGET_UNSET_MAPPED(w) gtk_widget_set_mapped (GTK_WIDGET (w), FALSE)

#else /* gtk-2.22.0 && DISABLE_DEPRECATED */

#define GTK_WIDGET_SET_CAN_FOCUS(w) GTK_WIDGET_SET_FLAGS ((w), GTK_CAN_FOCUS)
#define GTK_WIDGET_SET_REALIZED(w) GTK_WIDGET_SET_FLAGS ((w), GTK_REALIZED)
#define GTK_WIDGET_SET_NO_WINDOW(w) GTK_WIDGET_SET_FLAGS ((w), GTK_NO_WINDOW)
#define GTK_WIDGET_SET_MAPPED(w) GTK_WIDGET_SET_FLAGS ((w), GTK_MAPPED)

#define GTK_WIDGET_UNSET_CAN_FOCUS(w) GTK_WIDGET_UNSET_FLAGS ((w), GTK_CAN_FOCUS)
#define GTK_WIDGET_UNSET_REALIZED(w) GTK_WIDGET_UNSET_FLAGS ((w), GTK_REALIZED)
#define GTK_WIDGET_UNSET_NO_WINDOW(w) GTK_WIDGET_UNSET_FLAGS ((w), GTK_NO_WINDOW)
#define GTK_WIDGET_UNSET_MAPPED(w) GTK_WIDGET_UNSET_FLAGS ((w), GTK_MAPPED)

#endif /* gtk-2.22.0 && DISABLE_DEPRECATED */


#if GTK_CHECK_VERSION(2,24,0) && defined(GTK_DISABLE_DEPRECATED)

inline static void
_moo_gdk_drawable_get_size (GdkDrawable *drawable,
                            gint        *width,
                            gint        *height)
{
    if (width)
        *width = gdk_window_get_width (GDK_WINDOW (drawable));
    if (height)
        *height = gdk_window_get_height (GDK_WINDOW (drawable));
}

#define gdk_drawable_get_size _moo_gdk_drawable_get_size

#else /* gtk-2.242.0 && DISABLE_DEPRECATED */

#endif /* gtk-2.24.0 && DISABLE_DEPRECATED */


#endif /* MOO_GTK_H */
