#include "moousertools-enums.h"

GType
moo_command_options_get_type (void)
{
    static GType etype;
    if (G_UNLIKELY (!etype))
    {
        static const GFlagsValue values[] = {
            { MOO_COMMAND_NEED_DOC, (char*) "MOO_COMMAND_NEED_DOC", (char*) "MOO_COMMAND_NEED_DOC" },
            { MOO_COMMAND_NEED_FILE, (char*) "MOO_COMMAND_NEED_FILE", (char*) "MOO_COMMAND_NEED_FILE" },
            { MOO_COMMAND_NEED_SAVE, (char*) "MOO_COMMAND_NEED_SAVE", (char*) "MOO_COMMAND_NEED_SAVE" },
            { MOO_COMMAND_NEED_SAVE_ALL, (char*) "MOO_COMMAND_NEED_SAVE_ALL", (char*) "MOO_COMMAND_NEED_SAVE_ALL" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static ("MooCommandOptions", values);
    }
    return etype;
}


