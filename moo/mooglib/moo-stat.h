#pragma once

#include <mooglib/moo-glib.h>
#include <mooglib/moo-time.h>

G_BEGIN_DECLS

typedef struct MgwStatBuf MgwStatBuf;

struct MgwStatBuf
{
    mgw_time_t atime;
    mgw_time_t mtime;
    mgw_time_t ctime;

    guint64    size;

    guint      isreg  : 1, // S_ISREG
               isdir  : 1, // S_ISDIR
               islnk  : 1, // S_ISLNK
               issock : 1, // S_ISSOCK
               isfifo : 1, // S_ISFIFO
               ischr  : 1, // S_ISCHR
               isblk  : 1; // S_ISBLK
};

int mgw_stat (const gchar *filename, MgwStatBuf *buf, mgw_errno_t *err);
int mgw_lstat (const gchar *filename, MgwStatBuf *buf, mgw_errno_t *err);

G_END_DECLS
