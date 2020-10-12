/*
 *   moofile.c
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

/*
 *  TODO!!! fix this mess
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "moofileview/moofilesystem.h"
#include "moofileview/moofile-private.h"
#include "mooutils/moofileicon.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooutils-debug.h"
#include "marshals.h"
#include "mooutils/moo-mime.h"
#include <mooglib/moo-glib.h>
#include <mooglib/moo-stat.h>

#include <string.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#if defined(HAVE_CARBON)
#include <CoreServices/CoreServices.h>
#endif
#include <gtk/gtk.h>

MOO_DEBUG_INIT(file, FALSE)

#define NORMAL_PRIORITY         G_PRIORITY_DEFAULT_IDLE
#define NORMAL_TIMEOUT          0.04
#define BACKGROUND_PRIORITY     G_PRIORITY_LOW
#define BACKGROUND_TIMEOUT      0.001

#define TIMER_CLEAR(timer)  \
G_STMT_START {              \
    g_timer_start (timer);  \
    g_timer_stop (timer);   \
} G_STMT_END

static MooIconType      get_folder_icon (const char     *path);
static MooIconEmblem    get_icon_flags  (const MooFile  *file);

MOO_DEFINE_BOXED_TYPE_R (MooFile, _moo_file)

#define MAKE_PATH(dirname,file) g_build_filename (dirname, file->name, NULL)

void
_moo_file_find_mime_type (MooFile    *file,
                          const char *path)
{
    file->mime_type = moo_get_mime_type_for_file (path, file->statbuf);

    if (!file->mime_type || !file->mime_type[0])
    {
        /* this should not happen */
        _moo_message ("oops, %s", file->display_name);
        file->mime_type = MOO_MIME_TYPE_UNKNOWN;
    }

    file->flags |= MOO_FILE_HAS_MIME_TYPE;
}


#if defined(HAVE_CARBON)

struct MooCollationKey {
    guint len;
    UCCollationValue buf[1];
};

static MooCollationKey *
create_collation_key (CollatorRef  collator,
                      const gchar *str,
                      gssize       len)
{
  MooCollationKey *key = NULL;
  UniChar *str_utf16 = NULL;
  ItemCount actual_len;
  ItemCount try_len;
  glong len_utf16;
  OSStatus ret;
  UCCollationValue buf[512];

  if (!collator)
    goto error;

  str_utf16 = g_utf8_to_utf16 (str, len, NULL, &len_utf16, NULL);
  try_len = len_utf16 * 5 + 2;

  if (try_len <= sizeof buf)
    {
      ret = UCGetCollationKey (collator, str_utf16, len_utf16,
                               sizeof buf, &actual_len, buf);

      if (ret == 0)
        {
          key = g_malloc (sizeof (MooCollationKey) + (actual_len - 1) * sizeof (UCCollationValue));
          memcpy (key->buf, buf, actual_len * sizeof (UCCollationValue));
        }
      else if (ret == kCollateBufferTooSmall)
        try_len *= 2;
      else
        goto error;
    }

  if (!key)
    {
      key = g_malloc (sizeof (MooCollationKey) + (try_len - 1) * sizeof (UCCollationValue));
      ret = UCGetCollationKey (collator, str_utf16, len_utf16,
                               try_len, &actual_len, key->buf);

      if (ret == kCollateBufferTooSmall)
        {
          try_len *= 2;
          key = g_realloc (key, sizeof (MooCollationKey) + (actual_len - 1) * sizeof (UCCollationValue));
          ret = UCGetCollationKey (collator, str_utf16, len_utf16,
                                   try_len, &actual_len, key->buf);
        }

      if (ret != 0)
        goto error;
    }

  key->len = actual_len;

  g_free (str_utf16);
  return key;

error:
  g_free (str_utf16);
  g_free (key);
  key = g_new (MooCollationKey, 1);
  key->len = 0;
  return key;
}

G_GNUC_UNUSED static MooCollationKey *
moo_get_collation_key (const char *str,
                       gssize      len)
{
  static CollatorRef collator;

  if (G_UNLIKELY (!collator))
    {
      UCCreateCollator (NULL, 0, kUCCollateStandardOptions, &collator);

      if (!collator)
        g_warning ("UCCreateCollator failed");
    }

  return create_collation_key (collator, str, len);
}

static MooCollationKey *
moo_get_collation_key_for_filename (const char *str,
                                    gssize      len)
{
  static CollatorRef collator;

  if (G_UNLIKELY (!collator))
    {
      /* http://developer.apple.com/qa/qa2004/qa1159.html */
      UCCreateCollator (NULL, 0,
                        kUCCollateComposeInsensitiveMask
                         | kUCCollateWidthInsensitiveMask
                         | kUCCollateCaseInsensitiveMask
                         | kUCCollateDigitsOverrideMask
                         | kUCCollateDigitsAsNumberMask
                         | kUCCollatePunctuationSignificantMask,
                        &collator);

      if (!collator)
        g_warning ("UCCreateCollator failed");
    }

  return create_collation_key (collator, str, len);
}

int
_moo_collation_key_cmp (const MooCollationKey *key1,
                        const MooCollationKey *key2)
{
  SInt32 order = 0;

  g_return_val_if_fail (key1 != NULL, 0);
  g_return_val_if_fail (key2 != NULL, 0);

  UCCompareCollationKeys (key1->buf, key1->len,
                          key2->buf, key2->len,
                          NULL, &order);
  return order;
}

gsize
_moo_collation_key_size (MooCollationKey *key)
{
    return key ?
            sizeof (MooCollationKey) +
                key->len * sizeof (UCCollationValue) -
                sizeof (UCCollationValue) :
            0;
}

#else

#define moo_get_collation_key_for_filename(fn,len) ((MooCollationKey*)g_utf8_collate_key_for_filename(fn,len))

int
_moo_collation_key_cmp (const MooCollationKey *key1,
                        const MooCollationKey *key2)
{
  return strcmp ((const char*) key1, (const char*) key2);
}

gsize
_moo_collation_key_size (MooCollationKey *key)
{
    return key ? strlen ((char*) key) + 1 : 0;
}

#endif


/********************************************************************/
/* MooFile
 */

MooFile *
_moo_file_new (const char *dirname,
               const char *basename)
{
    MooFile *file = NULL;
    char *path = NULL;
    char *display_name = NULL;

    g_return_val_if_fail (dirname != NULL, NULL);
    g_return_val_if_fail (basename && basename[0], NULL);

    path = g_build_filename (dirname, basename, NULL);
    g_return_val_if_fail (path != NULL, NULL);

    display_name = g_filename_display_basename (path);
    g_assert (g_utf8_validate (display_name, -1, NULL));

    if (!display_name)
    {
        g_free (path);
        g_return_val_if_fail (display_name != NULL, NULL);
    }

    file = g_slice_new0 (MooFile);
    file->ref_count = 1;

    file->name = g_strdup (basename);

    file->display_name = g_utf8_normalize (display_name, -1, G_NORMALIZE_ALL);
    file->case_display_name = g_utf8_casefold (file->display_name, -1);
    file->collation_key = moo_get_collation_key_for_filename (file->display_name, -1);

    g_free (path);
    g_free (display_name);

#ifndef __WIN32__
    if (basename[0] == '.')
        file->info = MOO_FILE_INFO_IS_HIDDEN;
#endif

    return file;
}


MooFile *
_moo_file_ref (MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    file->ref_count++;
    return file;
}


void
_moo_file_unref (MooFile *file)
{
    if (file && !--file->ref_count)
    {
        g_free (file->name);
        g_free (file->display_name);
        g_free (file->case_display_name);
        g_free (file->collation_key);
        g_free (file->link_target);
        g_slice_free (MgwStatBuf, file->statbuf);
        g_slice_free (MooFile, file);
    }
}


void
_moo_file_stat (MooFile    *file,
                const char *dirname)
{
    char *fullname;
    mgw_errno_t err;

    g_return_if_fail (file != NULL);

    fullname = g_build_filename (dirname, file->name, NULL);

    file->info = MOO_FILE_INFO_EXISTS;
    file->flags = MOO_FILE_HAS_STAT;

    g_free (file->link_target);
    file->link_target = NULL;

    if (!file->statbuf)
        file->statbuf = g_slice_new (MgwStatBuf);

    if (mgw_lstat (fullname, file->statbuf, &err) != 0)
    {
        if (err.value == MGW_ENOENT)
        {
            MOO_DEBUG_CODE({
                gchar *display_name = g_filename_display_name (fullname);
                _moo_message ("file '%s' does not exist", display_name);
                g_free (display_name);
            });
            file->info = 0;
        }
        else
        {
            MOO_DEBUG_CODE({
                gchar *display_name = g_filename_display_name (fullname);
                _moo_message ("error getting information for '%s': %s",
                              display_name, mgw_strerror (err));
                g_free (display_name);
            });
            file->info = MOO_FILE_INFO_IS_LOCKED | MOO_FILE_INFO_EXISTS;
            file->flags = 0;
        }
    }
    else
    {
#ifndef __WIN32__
        if (file->statbuf->islnk)
        {
            static char buf[1024];
            gssize len;

            file->info |= MOO_FILE_INFO_IS_LINK;

            if (mgw_stat (fullname, file->statbuf, &err) != 0)
            {
                if (err.value == MGW_ENOENT)
                {
                    MOO_DEBUG_CODE({
                        gchar *display_name = g_filename_display_name (fullname);
                        _moo_message ("file '%s' is a broken link", display_name);
                        g_free (display_name);
                    });
                    file->info = MOO_FILE_INFO_IS_LINK;
                }
                else
                {
                    MOO_DEBUG_CODE({
                        gchar *display_name = g_filename_display_name (fullname);
                        _moo_message ("error getting information for '%s': %s",
                                      display_name, mgw_strerror (err));
                        g_free (display_name);
                    });
                    file->info = MOO_FILE_INFO_IS_LOCKED | MOO_FILE_INFO_EXISTS;
                    file->flags = 0;
                }
            }

            errno = 0;
            len = readlink (fullname, buf, 1024);
            err.value = errno;

            if (len == -1)
            {
                MOO_DEBUG_CODE({
                    gchar *display_name = g_filename_display_name (fullname);
                    _moo_message ("error getting link target for '%s': %s",
                                  display_name, mgw_strerror (err));
                    g_free (display_name);
                });
            }
            else
            {
                file->link_target = g_strndup (buf, len);
            }
        }
#endif // !__WIN32__
    }

    if ((file->info & MOO_FILE_INFO_EXISTS) &&
         !(file->info & MOO_FILE_INFO_IS_LOCKED))
    {
        if (file->statbuf->isdir)
            file->info |= MOO_FILE_INFO_IS_DIR;
        else if (file->statbuf->isblk)
            file->info |= MOO_FILE_INFO_IS_BLOCK_DEV;
        else if (file->statbuf->ischr)
            file->info |= MOO_FILE_INFO_IS_CHAR_DEV;
        else if (file->statbuf->isfifo)
            file->info |= MOO_FILE_INFO_IS_FIFO;
        else if (file->statbuf->issock)
            file->info |= MOO_FILE_INFO_IS_SOCKET;
    }

    if (file->info & MOO_FILE_INFO_IS_DIR)
    {
        file->flags |= MOO_FILE_HAS_MIME_TYPE;
        file->flags |= MOO_FILE_HAS_ICON;
    }

    file->icon = _moo_file_get_icon_type (file, dirname);

    if (file->name[0] == '.')
        file->info |= MOO_FILE_INFO_IS_HIDDEN;

    g_free (fullname);
}

void
_moo_file_free_statbuf (MooFile *file)
{
    g_return_if_fail (file != NULL);
    g_slice_free (MgwStatBuf, file->statbuf);
    file->statbuf = NULL;
    file->flags &= ~MOO_FILE_HAS_STAT;
}


gboolean
_moo_file_test (const MooFile  *file,
                MooFileInfo     test)
{
    g_return_val_if_fail (file != NULL, FALSE);
    return file->info & test;
}


const char *
_moo_file_display_name (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    return file->display_name;
}

const MooCollationKey *
_moo_file_collation_key (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    return file->collation_key;
}

const char *
_moo_file_case_display_name (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    return file->case_display_name;
}

const char *
_moo_file_name (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    return file->name;
}


const char *
_moo_file_get_mime_type (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    return file->mime_type;
}


GdkPixbuf *
_moo_file_get_icon (const MooFile  *file,
                    GtkWidget      *widget,
                    GtkIconSize     size)
{
    MooFileIcon icon = {0};

    g_return_val_if_fail (file != NULL, NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

    icon.mime_type = file->mime_type;
    icon.type = file->icon;
    icon.emblem = get_icon_flags (file);
    return moo_file_icon_get_pixbuf (&icon, widget, size);
}


#ifndef __WIN32__
const char *
_moo_file_link_get_target (const MooFile *file)
{
    return file->link_target;
}
#endif


#if 0
MooFileTime
_moo_file_get_mtime (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, 0);
    return file->statbuf.st_mtime;
}

MooFileSize
_moo_file_get_size (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, 0);
    return file->statbuf.st_size;
}

#ifndef __WIN32__
const struct stat *
_moo_file_get_stat (const MooFile *file)
{
    g_return_val_if_fail (file != NULL, NULL);
    if (file->flags & MOO_FILE_HAS_STAT && file->info & MOO_FILE_INFO_EXISTS)
        return &file->statbuf;
    else
        return NULL;
}
#endif
#endif


guint8
_moo_file_get_icon_type (MooFile    *file,
                         const char *dirname)
{
    if (MOO_FILE_IS_BROKEN_LINK (file))
        return MOO_ICON_BROKEN_LINK;

    if (!MOO_FILE_EXISTS (file))
        return MOO_ICON_NONEXISTENT;

    if (MOO_FILE_IS_DIR (file))
    {
        char *path = MAKE_PATH (dirname, file);
        MooIconType icon = get_folder_icon (path);
        g_free (path);
        return icon;
    }

    if (MOO_FILE_IS_SPECIAL (file))
    {
        if (_moo_file_test (file, MOO_FILE_INFO_IS_BLOCK_DEV))
            return MOO_ICON_BLOCK_DEVICE;
        else if (_moo_file_test (file, MOO_FILE_INFO_IS_CHAR_DEV))
            return MOO_ICON_CHARACTER_DEVICE;
        else if (_moo_file_test (file, MOO_FILE_INFO_IS_FIFO))
            return MOO_ICON_FIFO;
        else if (_moo_file_test (file, MOO_FILE_INFO_IS_SOCKET))
            return MOO_ICON_SOCKET;
    }

    if (file->flags & MOO_FILE_HAS_MIME_TYPE && file->mime_type)
        return MOO_ICON_MIME;

    return MOO_ICON_FILE;
}


guint8
_moo_file_icon_blank (void)
{
    return MOO_ICON_BLANK;
}


static MooIconType
get_folder_icon (const char *path)
{
    static const char *home_path = NULL;
    static char *desktop_path = NULL;
    static char *trash_path = NULL;

    if (!path)
        return MOO_ICON_DIRECTORY;

    if (!home_path)
        home_path = g_get_home_dir ();

    if (!home_path)
        return MOO_ICON_DIRECTORY;

    if (!desktop_path)
        desktop_path = g_build_filename (home_path, "Desktop", NULL);

    if (!trash_path)
        trash_path = g_build_filename (desktop_path, "Trash", NULL);

        /* keep this in sync with create_fallback_icon() */
    if (strcmp (home_path, path) == 0)
        return MOO_ICON_HOME;
    else if (strcmp (desktop_path, path) == 0)
        return MOO_ICON_DESKTOP;
    else if (strcmp (trash_path, path) == 0)
        return MOO_ICON_TRASH;
    else
        return MOO_ICON_DIRECTORY;
}


static MooIconEmblem
get_icon_flags (const MooFile *file)
{
    return
#if 0
        (MOO_FILE_IS_LOCKED (file) ? MOO_ICON_EMBLEM_LOCK : 0) |
#endif
        (MOO_FILE_IS_LINK (file) ? MOO_ICON_EMBLEM_LINK : 0);
}
