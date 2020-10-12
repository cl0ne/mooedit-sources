/*
 *   moofileicon.c
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

#include "mooutils/moofileicon.h"
#include "mooutils/moo-mime.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/moostock.h"
#include "moo-pixbufs.h"
#include <mooglib/moo-glib.h>
#include <gtk/gtk.h>
#include <string.h>

void
moo_file_icon_for_file (MooFileIcon *icon,
                        G_GNUC_UNUSED const char *path)
{
    const char *mime_type = NULL;

    g_return_if_fail (icon != NULL);

#ifndef __WIN32__
    if (path)
        mime_type = moo_get_mime_type_for_file (path, NULL);
#else
    MOO_IMPLEMENT_ME
#endif

    if (!mime_type || !mime_type[0])
        mime_type = MOO_MIME_TYPE_UNKNOWN;

    icon->mime_type = mime_type;
    icon->type = MOO_ICON_MIME;
    icon->emblem = (MooIconEmblem) 0;
}

GdkPixbuf *
moo_get_icon_for_path (const char  *path,
                       GtkWidget   *widget,
                       GtkIconSize  size)
{
    MooFileIcon icon = {0};
    moo_file_icon_for_file (&icon, path);
    return moo_file_icon_get_pixbuf (&icon, widget, size);
}

MooFileIcon *
moo_file_icon_new (void)
{
    return g_slice_new (MooFileIcon);
}

MooFileIcon *
moo_file_icon_copy (MooFileIcon *icon)
{
    return icon ? g_slice_dup (MooFileIcon, icon) : NULL;
}

void
moo_file_icon_free (MooFileIcon *icon)
{
    if (icon)
        g_slice_free (MooFileIcon, icon);
}


/********************************************************************/
/* Icon cache
 */

#define MOO_ICON_EMBLEM_LEN 2

MOO_DEBUG_INIT(icons, FALSE)

typedef struct {
    GdkPixbuf **special_icons[MOO_ICON_INVALID];
    GHashTable *mime_icons;
} MooIconCache;


static MooIconCache *moo_icon_cache_new             (void);
static void          moo_icon_cache_free            (MooIconCache   *cache);

static MooIconCache *moo_icon_cache_get_for_screen  (GdkScreen      *screen,
                                                     GtkIconSize     size);

static GdkPixbuf    *create_special_icon            (GtkWidget      *widget,
                                                     MooIconType     type,
                                                     GtkIconSize     size);
static GdkPixbuf    *create_mime_icon               (GtkWidget      *widget,
                                                     const char     *mime_type,
                                                     GtkIconSize     size);
static GdkPixbuf    *create_fallback_icon           (GtkWidget      *widget,
                                                     GtkIconSize     size);
static GdkPixbuf    *add_emblem                     (GdkPixbuf      *original,
                                                     MooIconEmblem   emblem,
                                                     GtkIconSize     size);
static GdkPixbuf    *get_named_icon                 (GtkIconTheme   *icon_theme,
                                                     const char     *icon_name,
                                                     int             pixel_size);
static GdkPixbuf    *get_stock_icon                 (GtkWidget      *widget,
                                                     const char     *stock_id,
                                                     GtkIconSize     size);


static GdkPixbuf **
pixbuf_array_new (void)
{
    return g_new0 (GdkPixbuf*, MOO_ICON_EMBLEM_LEN);
}

static void
pixbuf_array_free (GdkPixbuf **pixbufs)
{
    if (pixbufs)
    {
        guint i;
        for (i = 0; i < MOO_ICON_EMBLEM_LEN; ++i)
            if (pixbufs[i])
                g_object_unref (pixbufs[i]);
        g_free (pixbufs);
    }
}

static void
pixbuf_array_set (GdkPixbuf    **array,
                  MooIconEmblem  flags,
                  GdkPixbuf     *pixbuf)
{
    GdkPixbuf *tmp;

    tmp = array[flags];
    array[flags] = pixbuf;

    if (pixbuf)
        g_object_ref (pixbuf);
    if (tmp)
        g_object_unref (tmp);
}

static MooIconCache *
moo_icon_cache_new (void)
{
    guint i;
    MooIconCache *cache = g_new0 (MooIconCache, 1);

    for (i = 0; i < MOO_ICON_INVALID; ++i)
        cache->special_icons[i] = NULL;

    cache->mime_icons = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                               (GDestroyNotify) pixbuf_array_free);

    return cache;
}


static void
moo_icon_cache_free (MooIconCache *cache)
{
    if (cache)
    {
        guint i;
        for (i = 0; i < MOO_ICON_INVALID; ++i)
            pixbuf_array_free (cache->special_icons[i]);
        g_hash_table_destroy (cache->mime_icons);
        g_free (cache);
    }
}


static GdkPixbuf *
moo_icon_cache_get_special (MooIconCache   *cache,
                            MooIconType     type,
                            MooIconEmblem   flags)
{
    g_return_val_if_fail (type && type < MOO_ICON_INVALID, NULL);
    g_return_val_if_fail (flags < MOO_ICON_EMBLEM_LEN, NULL);
    return cache->special_icons[type] ? cache->special_icons[type][flags] : NULL;
}


static GdkPixbuf *
moo_icon_cache_get_mime (MooIconCache   *cache,
                         const char     *mime,
                         MooIconEmblem   flags)
{
    GdkPixbuf **pixbufs;
    pixbufs = (GdkPixbuf**) g_hash_table_lookup (cache->mime_icons, mime);
    return pixbufs ? pixbufs[flags] : NULL;
}


static GdkPixbuf *
moo_icon_cache_get (MooIconCache   *cache,
                    MooIconType     type,
                    const char     *mime,
                    MooIconEmblem   flags)
{
    g_assert (type < MOO_ICON_INVALID);
    g_assert ((type == 0 && mime != NULL) || (type != 0 && mime == NULL));
    return mime ? moo_icon_cache_get_mime (cache, mime, flags) :
                  moo_icon_cache_get_special (cache, type, flags);
}


static void
moo_icon_cache_set_special (MooIconCache   *cache,
                            MooIconType     type,
                            MooIconEmblem   flags,
                            GdkPixbuf      *pixbuf)
{
    GdkPixbuf *tmp;

    g_return_if_fail (type && type < MOO_ICON_INVALID);
    g_return_if_fail (flags < MOO_ICON_EMBLEM_LEN);

    if (!cache->special_icons[type])
        cache->special_icons[type] = pixbuf_array_new ();

    tmp = cache->special_icons[type][flags];
    cache->special_icons[type][flags] = pixbuf;

    if (pixbuf)
        g_object_ref (pixbuf);
    if (tmp)
        g_object_unref (pixbuf);
}


static void
moo_icon_cache_set_mime (MooIconCache *cache,
                         const char   *mime,
                         MooIconEmblem flags,
                         GdkPixbuf    *pixbuf)
{
    GdkPixbuf **array;

    array = (GdkPixbuf**) g_hash_table_lookup (cache->mime_icons, mime);

    if (!array)
    {
        array = pixbuf_array_new ();
        g_hash_table_insert (cache->mime_icons, g_strdup (mime), array);
    }

    pixbuf_array_set (array, flags, pixbuf);
}


static void
moo_icon_cache_set (MooIconCache *cache,
                    MooIconType   type,
                    const char   *mime,
                    MooIconEmblem flags,
                    GdkPixbuf    *pixbuf)
{
    g_assert (type < MOO_ICON_INVALID);
    g_assert ((type == 0 && mime != NULL) || (type != 0 && mime == NULL));
    if (mime)
        moo_icon_cache_set_mime (cache, mime, flags, pixbuf);
    else
        moo_icon_cache_set_special (cache, type, flags, pixbuf);
}


static MooIconCache *
moo_icon_cache_get_for_screen (GdkScreen   *screen,
                               GtkIconSize  size)
{
    GHashTable *hash;
    MooIconCache *cache;
    GtkIconTheme *icon_theme;

    icon_theme = gtk_icon_theme_get_for_screen (screen);
    g_return_val_if_fail (icon_theme != NULL, NULL);

    hash = (GHashTable*) g_object_get_data (G_OBJECT (icon_theme), "moo-icon-cache");

    if (!hash)
    {
        hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
                                      (GDestroyNotify) moo_icon_cache_free);
        g_object_set_data_full (G_OBJECT (icon_theme), "moo-icon-cache", hash,
                                (GDestroyNotify) g_hash_table_destroy);
    }

    cache = (MooIconCache*) g_hash_table_lookup (hash, GINT_TO_POINTER (size));

    if (!cache)
    {
        cache = moo_icon_cache_new ();
        g_hash_table_insert (hash, GINT_TO_POINTER (size), cache);
    }

    return cache;
}


static GdkPixbuf *
add_arrow (GdkPixbuf     *original,
           GtkIconSize    size)
{
    GdkPixbuf *emblem, *pixbuf;
    static GdkPixbuf *arrow = NULL;
    static GdkPixbuf *small_arrow = NULL;

    if (!arrow)
    {
        arrow = gdk_pixbuf_new_from_inline (-1, SYMLINK_ARROW, TRUE, NULL);
        g_return_val_if_fail (arrow != NULL, GDK_PIXBUF (g_object_ref (original)));
    }

    if (!small_arrow)
    {
        small_arrow = gdk_pixbuf_new_from_inline (-1, SYMLINK_ARROW_SMALL, TRUE, NULL);
        g_return_val_if_fail (arrow != NULL, GDK_PIXBUF (g_object_ref (original)));
    }

    if (size == GTK_ICON_SIZE_MENU)
        emblem = small_arrow;
    else
        emblem = arrow;

    pixbuf = gdk_pixbuf_copy (original);
    g_return_val_if_fail (pixbuf != NULL, GDK_PIXBUF (g_object_ref (original)));

    gdk_pixbuf_composite (emblem, pixbuf,
                          0,
                          gdk_pixbuf_get_height (pixbuf) - gdk_pixbuf_get_height (emblem),
                          gdk_pixbuf_get_width (emblem),
                          gdk_pixbuf_get_height (emblem),
                          0,
                          gdk_pixbuf_get_height (pixbuf) - gdk_pixbuf_get_height (emblem),
                          1, 1,
                          GDK_INTERP_BILINEAR,
                          255);

    return pixbuf;
}


static GdkPixbuf *
add_emblem (GdkPixbuf     *original,
            G_GNUC_UNUSED MooIconEmblem flags,
            GtkIconSize    size)
{
    g_assert (flags == MOO_ICON_EMBLEM_LINK);
    g_assert (GDK_IS_PIXBUF (original));
    return add_arrow (original, size);
}


static void
pixels_from_icon_size (GdkScreen   *screen,
                       GtkIconSize  size,
                       int         *widthp,
                       int         *heightp)
{
    GtkSettings *settings;
    int width, height;

    settings = gtk_settings_get_for_screen (screen);

    if (!gtk_icon_size_lookup_for_settings (settings, size, &width, &height))
    {
        g_critical ("invalid icon size");
        size = GTK_ICON_SIZE_MENU;
        gtk_icon_size_lookup_for_settings (settings, size, &width, &height);
    }

    if (widthp)
        *widthp = width;
    if (heightp)
        *heightp = height;
}


static GdkPixbuf *
get_named_icon (GtkIconTheme *icon_theme,
                const char   *icon_name,
                int           pixel_size)
{
    GdkPixbuf *pixbuf;

    pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                       icon_name,
                                       pixel_size,
                                       GTK_ICON_LOOKUP_USE_BUILTIN,
                                       NULL);

    /* happens with crystalsvg for some reason */
    if (pixbuf && (gdk_pixbuf_get_width (pixbuf) < 5 ||
                   gdk_pixbuf_get_height (pixbuf) < 5))
    {
        moo_dmsg ("got zero size icon '%s'", icon_name);
        g_object_unref (pixbuf);
        pixbuf = NULL;
    }

    return pixbuf;
}


static GdkPixbuf *
get_stock_icon (GtkWidget   *widget,
                const char  *stock_id,
                GtkIconSize  size)
{
    return gtk_widget_render_icon (widget, stock_id, size, NULL);
}


static GdkPixbuf *
create_named_icon (GtkIconTheme   *icon_theme,
                   GtkWidget      *widget,
                   GtkIconSize     size,
                   int             pixel_size,
                   const char     *fallback_stock,
                   ...)
{
    GdkPixbuf *pixbuf = NULL;
    va_list args;
    char *name;

    va_start (args, fallback_stock);

    while (!pixbuf && (name = va_arg (args, char *)))
    {
        pixbuf = get_named_icon (icon_theme, name, pixel_size);

        if (!pixbuf)
            moo_dmsg ("could not load '%s' icon", name);
    }

    va_end (args);

    if (!pixbuf && fallback_stock)
    {
        pixbuf = gtk_widget_render_icon (widget, fallback_stock, size, NULL);
        if (!pixbuf)
            moo_dmsg ("could not load stock '%s' icon", fallback_stock);
    }

    if (!pixbuf)
        pixbuf = get_stock_icon (widget, MOO_STOCK_FILE, size);

    return pixbuf;
}


static GdkPixbuf *
create_fallback_icon (GtkWidget   *widget,
                      GtkIconSize  size)
{
    int pixel_size;
    GdkScreen *screen;
    GtkIconTheme *icon_theme;

    screen = gtk_widget_get_screen (widget);
    icon_theme = gtk_icon_theme_get_for_screen (screen);
    pixels_from_icon_size (screen, size, NULL, &pixel_size);

    return create_named_icon (icon_theme, widget, size, pixel_size, NULL, NULL);
}


static GdkPixbuf *
create_broken_link_icon (G_GNUC_UNUSED GtkIconTheme *icon_theme,
                         GtkWidget    *widget,
                         GtkIconSize   size)
{
    /* XXX */
    return gtk_widget_render_icon (widget, GTK_STOCK_MISSING_IMAGE, size, NULL);
}

static GdkPixbuf *
create_broken_icon (G_GNUC_UNUSED GtkIconTheme *icon_theme,
                    GtkWidget      *widget,
                    GtkIconSize     size)
{
    /* XXX */
    return gtk_widget_render_icon (widget, GTK_STOCK_MISSING_IMAGE, size, NULL);
}

static GdkPixbuf *
create_special_icon (GtkWidget   *widget,
                     MooIconType  type,
                     GtkIconSize  size)
{
    int pixel_size;
    GdkScreen *screen;
    GtkIconTheme *icon_theme;

    g_assert (type < MOO_ICON_INVALID);
    g_assert (type != MOO_ICON_MIME);

    screen = gtk_widget_get_screen (widget);
    icon_theme = gtk_icon_theme_get_for_screen (screen);
    pixels_from_icon_size (screen, size, NULL, &pixel_size);

    switch (type)
    {
        case MOO_ICON_HOME:
            return create_named_icon (icon_theme, widget, size, pixel_size, GTK_STOCK_HOME,
                                      "user-home", "gnome-fs-home", "folder_home", NULL);
        case MOO_ICON_DESKTOP:
            return create_named_icon (icon_theme, widget, size, pixel_size, GTK_STOCK_DIRECTORY,
                                      "user-desktop", "gnome-fs-desktop", "desktop",
                                      "folder", "gnome-fs-directory", NULL);
        case MOO_ICON_TRASH:
            return create_named_icon (icon_theme, widget, size, pixel_size, GTK_STOCK_DIRECTORY,
                                      "user-trash", "gnome-fs-trash-full", "trashcan_full",
                                      "folder", "gnome-fs-directory", NULL);
        case MOO_ICON_DIRECTORY:
            return create_named_icon (icon_theme, widget, size, pixel_size, GTK_STOCK_DIRECTORY,
                                      "folder", "gnome-fs-directory", NULL);

        case MOO_ICON_BROKEN_LINK:
            return create_broken_link_icon (icon_theme, widget, size);
        case MOO_ICON_NONEXISTENT:
            return create_broken_icon (icon_theme, widget, size);

        case MOO_ICON_BLOCK_DEVICE:
            return create_named_icon (icon_theme, widget, size, pixel_size, GTK_STOCK_HARDDISK,
                                      "drive-harddisk", "gnome-fs-blockdev", "blockdevice", NULL);
        case MOO_ICON_CHARACTER_DEVICE:
            return create_named_icon (icon_theme, widget, size, pixel_size, NULL,
                                      "gnome-fs-chardev", "chardevice", "input-keyboard", NULL);
        case MOO_ICON_FIFO:
            return create_named_icon (icon_theme, widget, size, pixel_size, NULL,
                                      "gnome-fs-fifo", "pipe", NULL);
        case MOO_ICON_SOCKET:
            return create_named_icon (icon_theme, widget, size, pixel_size, NULL,
                                      "gnome-fs-socket", NULL);
        case MOO_ICON_FILE:
            return create_named_icon (icon_theme, widget, size, pixel_size, NULL,
                                      "gnome-fs-regular", "unknown", NULL);
        case MOO_ICON_BLANK:
            return create_fallback_icon (widget, size);

        case MOO_ICON_MIME:
        case MOO_ICON_INVALID:
            g_return_val_if_reached (NULL);
    }

    g_return_val_if_reached (NULL);
}


static GdkPixbuf *
create_mime_icon_type (GtkIconTheme *icon_theme,
                       const char   *mime_type,
                       int           pixel_size)
{
    GString *icon_name;
    GdkPixbuf *pixbuf = NULL;
    const char *separator;

    /* foo/bar-baz */
    separator = strchr (mime_type, '/');

    if (!separator)
    {
        g_warning ("mime type '%s' is invalid", mime_type);
        return NULL;
    }

    if (!strncmp (mime_type, "text", strlen ("text")))
        pixbuf = get_named_icon (icon_theme, "text-x-generic", pixel_size);
    else if (!strncmp (mime_type, "audio", strlen ("audio")))
        pixbuf = get_named_icon (icon_theme, "audio-x-generic", pixel_size);
    else if (!strncmp (mime_type, "image", strlen ("image")))
        pixbuf = get_named_icon (icon_theme, "image-x-generic", pixel_size);

    if (!pixbuf)
    {
        /* gnome-mime-foo */
        icon_name = g_string_new ("gnome-mime-");
        g_string_append_len (icon_name, mime_type, separator - mime_type);
        pixbuf = get_named_icon (icon_theme, icon_name->str, pixel_size);
        if (pixbuf)
            moo_dmsg ("got icon '%s' for mime type '%s'", icon_name->str, mime_type);
        g_string_free (icon_name, TRUE);
    }

    if (!pixbuf)
    {
        if (!strncmp (mime_type, "text", strlen ("text")))
            pixbuf = get_named_icon (icon_theme, "txt", pixel_size);
        else if (!strncmp (mime_type, "audio", strlen ("audio")))
            pixbuf = get_named_icon (icon_theme, "sound", pixel_size);
        else if (!strncmp (mime_type, "image", strlen ("image")))
            pixbuf = get_named_icon (icon_theme, "image", pixel_size);
    }

    if (!pixbuf)
    {
        /* foo */
        icon_name = g_string_new_len (mime_type, separator - mime_type);
        pixbuf = get_named_icon (icon_theme, icon_name->str, pixel_size);
        if (pixbuf)
            moo_dmsg ("got icon '%s' for mime type '%s'", icon_name->str, mime_type);
        g_string_free (icon_name, TRUE);
    }

    return pixbuf;
}

static GdkPixbuf *
create_mime_icon_exact (GtkIconTheme *icon_theme,
                        const char   *mime_type,
                        int           pixel_size)
{
    GString *icon_name;
    GdkPixbuf *pixbuf = NULL;
    const char *separator;

    /* foo/bar-baz */
    separator = strchr (mime_type, '/');

    if (!separator)
    {
        g_warning ("mime type '%s' is invalid", mime_type);
        return NULL;
    }

    if (!pixbuf)
    {
        /* foo-bar-baz */
        icon_name = g_string_new_len (mime_type, separator - mime_type);
        g_string_append_c (icon_name, '-');
        g_string_append (icon_name, separator + 1);
        pixbuf = get_named_icon (icon_theme, icon_name->str, pixel_size);
        if (pixbuf)
            moo_dmsg ("got icon '%s' for mime type '%s'", icon_name->str, mime_type);
        g_string_free (icon_name, TRUE);
    }

    if (!pixbuf)
    {
        /* gnome-mime-foo-bar-baz */
        icon_name = g_string_new ("gnome-mime-");
        g_string_append_len (icon_name, mime_type, separator - mime_type);
        g_string_append_c (icon_name, '-');
        g_string_append (icon_name, separator + 1);
        pixbuf = get_named_icon (icon_theme, icon_name->str, pixel_size);
        if (pixbuf)
            moo_dmsg ("got icon '%s' for mime type '%s'", icon_name->str, mime_type);
        g_string_free (icon_name, TRUE);
    }

    return pixbuf;
}

static GdkPixbuf *
create_mime_icon (GtkWidget    *widget,
                  const char   *mime_type,
                  GtkIconSize   size)
{
    GdkPixbuf *pixbuf = NULL;
    GtkIconTheme *icon_theme;
    int pixel_size;
    GdkScreen *screen;

    screen = gtk_widget_get_screen (widget);
    icon_theme = gtk_icon_theme_get_for_screen (screen);
    pixels_from_icon_size (screen, size, NULL, &pixel_size);

    if (!strcmp (mime_type, MOO_MIME_TYPE_UNKNOWN))
    {
        pixbuf = get_named_icon (icon_theme, "unknown", pixel_size);

        if (pixbuf)
            moo_dmsg ("got 'unknown' icon for '%s'", mime_type);

        if (!pixbuf)
        {
            pixbuf = create_special_icon (widget, MOO_ICON_FILE, size);

            if (pixbuf)
                moo_dmsg ("got FILE icon for '%s'", mime_type);
        }

        if (!pixbuf)
        {
            moo_dmsg ("getting fallback icon for mime type '%s'", mime_type);
            pixbuf = create_fallback_icon (widget, size);
        }

        return pixbuf;
    }

    pixbuf = create_mime_icon_exact (icon_theme, mime_type, pixel_size);

    if (!pixbuf)
    {
        const char **parent_types = moo_mime_type_list_parents (mime_type);

        if (parent_types && parent_types[0])
        {
            const char **p;

            for (p = parent_types; *p && !pixbuf; ++p)
            {
                pixbuf = create_mime_icon (widget, *p, size);
                if (pixbuf)
                    moo_dmsg ("used mime type '%s' icon for '%s'", *p, mime_type);
            }

            g_free ((gpointer) parent_types);
            return pixbuf;
        }
        else
        {
            pixbuf = create_mime_icon_type (icon_theme, mime_type, size);
        }

        g_free ((gpointer) parent_types);
    }

    if (!pixbuf)
        pixbuf = create_special_icon (widget, MOO_ICON_FILE, size);

    return pixbuf;
}

GdkPixbuf *
moo_file_icon_get_pixbuf (MooFileIcon *icon,
                          GtkWidget   *widget,
                          GtkIconSize  size)
{
    GdkScreen *screen;
    GdkPixbuf *pixbuf;
    GdkPixbuf *original;
    MooIconCache *cache;
    const char *mime_type;

    g_return_val_if_fail (icon != NULL, NULL);
    g_return_val_if_fail (!widget || GTK_IS_WIDGET (widget), NULL);
    g_return_val_if_fail (icon->type < MOO_ICON_INVALID, NULL);
    g_return_val_if_fail (icon->type != MOO_ICON_MIME || icon->mime_type != NULL, NULL);
    g_return_val_if_fail (icon->emblem < MOO_ICON_EMBLEM_LEN, NULL);

    if (icon->type != MOO_ICON_MIME)
        mime_type = NULL;
    else
        mime_type = icon->mime_type;

    if (widget && gtk_widget_has_screen (widget))
        screen = gtk_widget_get_screen (widget);
    else
        screen = gdk_screen_get_default ();

    cache = moo_icon_cache_get_for_screen (screen, size);
    g_return_val_if_fail (cache != NULL, NULL);

    pixbuf = moo_icon_cache_get (cache, icon->type, mime_type, icon->emblem);

    if (pixbuf)
        return pixbuf;

    original = NULL;

    if (icon->emblem)
        original = moo_icon_cache_get (cache, icon->type, mime_type, (MooIconEmblem) 0);

    if (!original)
    {
        if (mime_type)
            original = create_mime_icon (widget, mime_type, size);
        else
            original = create_special_icon (widget, icon->type, size);

        if (!original)
            original = create_fallback_icon (widget, size);

        g_return_val_if_fail (original != NULL, NULL);

        moo_icon_cache_set (cache, icon->type, mime_type, (MooIconEmblem) 0, original);
        g_object_unref (original);
    }

    if (!icon->emblem)
        return original;

    pixbuf = add_emblem (original, icon->emblem, size);
    g_return_val_if_fail (pixbuf != NULL, NULL);
    moo_icon_cache_set (cache, icon->type, mime_type, icon->emblem, pixbuf);
    g_object_unref (pixbuf);

    return pixbuf;
}
