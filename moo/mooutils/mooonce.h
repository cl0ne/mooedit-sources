#ifndef MOO_ONCE_H
#define MOO_ONCE_H

#include <mooglib/moo-glib.h>
#include <mooutils/mooutils-macros.h>

#define MOO_DO_ONCE_BEGIN                       \
do {                                            \
    static gsize _moo_do_once = 0;              \
    if (g_once_init_enter (&_moo_do_once))      \
    {

#define MOO_DO_ONCE_END                         \
        g_once_init_leave (&_moo_do_once, 1);   \
    }                                           \
} while (0);

#endif /* MOO_ONCE_H */
