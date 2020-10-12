#include "config.h"
#include "mooutils-file.h"
#include "mooutils.h"
#include <fnmatch.h>

MOO_DEFINE_OBJECT_ARRAY_FULL (MooFileArray, moo_file_array, GFile)

char *
moo_file_get_display_name (GFile *file)
{
    g_return_val_if_fail (G_IS_FILE (file), NULL);
    return g_file_get_parse_name (file);
}

gboolean
moo_file_fnmatch (GFile      *file,
                  const char *glob)
{
    char *filename;
    gboolean ret;

    g_return_val_if_fail (G_IS_FILE (file), FALSE);
    g_return_val_if_fail (glob != NULL, FALSE);

    filename = g_file_get_path (file);
    g_return_val_if_fail (filename != NULL, FALSE);

    ret = fnmatch (glob, filename, 0) == 0;

    g_free (filename);
    return ret;
}
