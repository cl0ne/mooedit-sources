#pragma once

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS

typedef struct mgw_time_t mgw_time_t;

struct mgw_time_t
{
    gint64 value;
};

const struct tm *mgw_localtime(const mgw_time_t *timep);
const struct tm *mgw_localtime_r(const mgw_time_t *timep, struct tm *result, mgw_errno_t *err);
mgw_time_t mgw_time(mgw_time_t *t, mgw_errno_t *err);

G_END_DECLS
