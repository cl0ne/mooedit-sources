#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
    MOO_COMMAND_OPTIONS_NONE = 0,
    MOO_COMMAND_NEED_DOC = 1 << 0,
    MOO_COMMAND_NEED_FILE = 1 << 1,
    MOO_COMMAND_NEED_SAVE = 1 << 2,
    MOO_COMMAND_NEED_SAVE_ALL = 1 << 3,
} MooCommandOptions;

GType moo_command_options_get_type (void) G_GNUC_CONST;
#define MOO_TYPE_COMMAND_OPTIONS (moo_command_options_get_type())

G_END_DECLS

#ifdef __cplusplus

#include "mooutils/mooutils-cpp.h"

MOO_DEFINE_FLAGS(MooCommandOptions)

#endif // __cplusplus