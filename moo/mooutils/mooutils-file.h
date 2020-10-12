#ifndef MOO_UTILS_FILE_H
#define MOO_UTILS_FILE_H

#include <gio/gio.h>
#include <mooutils/mooarray.h>

G_BEGIN_DECLS

MOO_DECLARE_OBJECT_ARRAY_FULL (MooFileArray, moo_file_array, GFile)

G_INLINE_FUNC void
moo_file_free (GFile *file)
{
    if (file)
        g_object_unref (file);
}

gboolean     moo_file_fnmatch           (GFile      *file,
                                         const char *glob);
char        *moo_file_get_display_name  (GFile *file);

G_END_DECLS

#endif /* MOO_UTILS_FILE_H */
