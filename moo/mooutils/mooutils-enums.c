
/* Generated data (by glib-mkenums) */

#include "mooutils/mooutils-enums.h"
#include "mooutils/moodialogs.h"
/* MooSaveChangesResponse */
GType
moo_save_changes_response_get_type (void)
{
    static GType etype;

    if (G_UNLIKELY (!etype))
    {
        static const GEnumValue values[] = {
            { MOO_SAVE_CHANGES_RESPONSE_CANCEL, (char*) "MOO_SAVE_CHANGES_RESPONSE_CANCEL", (char*) "cancel" },
            { MOO_SAVE_CHANGES_RESPONSE_SAVE, (char*) "MOO_SAVE_CHANGES_RESPONSE_SAVE", (char*) "save" },
            { MOO_SAVE_CHANGES_RESPONSE_DONT_SAVE, (char*) "MOO_SAVE_CHANGES_RESPONSE_DONT_SAVE", (char*) "dont-save" },
            { 0, NULL, NULL }
        };

        etype = g_enum_register_static ("MooSaveChangesResponse", values);
    }

    return etype;
}
#include "mooutils/moofiledialog.h"
/* MooFileDialogType */
GType
moo_file_dialog_type_get_type (void)
{
    static GType etype;

    if (G_UNLIKELY (!etype))
    {
        static const GEnumValue values[] = {
            { MOO_FILE_DIALOG_OPEN, (char*) "MOO_FILE_DIALOG_OPEN", (char*) "open" },
            { MOO_FILE_DIALOG_OPEN_ANY, (char*) "MOO_FILE_DIALOG_OPEN_ANY", (char*) "open-any" },
            { MOO_FILE_DIALOG_SAVE, (char*) "MOO_FILE_DIALOG_SAVE", (char*) "save" },
            { MOO_FILE_DIALOG_OPEN_DIR, (char*) "MOO_FILE_DIALOG_OPEN_DIR", (char*) "open-dir" },
            { 0, NULL, NULL }
        };

        etype = g_enum_register_static ("MooFileDialogType", values);
    }

    return etype;
}
#include "mooutils/moouixml.h"
/* MooUiNodeType */
GType
moo_ui_node_type_get_type (void)
{
    static GType etype;

    if (G_UNLIKELY (!etype))
    {
        static const GEnumValue values[] = {
            { MOO_UI_NODE_CONTAINER, (char*) "MOO_UI_NODE_CONTAINER", (char*) "container" },
            { MOO_UI_NODE_WIDGET, (char*) "MOO_UI_NODE_WIDGET", (char*) "widget" },
            { MOO_UI_NODE_ITEM, (char*) "MOO_UI_NODE_ITEM", (char*) "item" },
            { MOO_UI_NODE_SEPARATOR, (char*) "MOO_UI_NODE_SEPARATOR", (char*) "separator" },
            { MOO_UI_NODE_PLACEHOLDER, (char*) "MOO_UI_NODE_PLACEHOLDER", (char*) "placeholder" },
            { 0, NULL, NULL }
        };

        etype = g_enum_register_static ("MooUiNodeType", values);
    }

    return etype;
}
/* MooUiNodeFlags */
GType
moo_ui_node_flags_get_type (void)
{
    static GType etype;

    if (G_UNLIKELY (!etype))
    {
        static const GFlagsValue values[] = {
            { MOO_UI_NODE_ENABLE_EMPTY, (char*) "MOO_UI_NODE_ENABLE_EMPTY", (char*) "empty" },
            { 0, NULL, NULL }
        };

        etype = g_flags_register_static ("MooUiNodeFlags", values);
    }

    return etype;
}
/* MooUiWidgetType */
GType
moo_ui_widget_type_get_type (void)
{
    static GType etype;

    if (G_UNLIKELY (!etype))
    {
        static const GEnumValue values[] = {
            { MOO_UI_MENUBAR, (char*) "MOO_UI_MENUBAR", (char*) "menubar" },
            { MOO_UI_MENU, (char*) "MOO_UI_MENU", (char*) "menu" },
            { MOO_UI_TOOLBAR, (char*) "MOO_UI_TOOLBAR", (char*) "toolbar" },
            { 0, NULL, NULL }
        };

        etype = g_enum_register_static ("MooUiWidgetType", values);
    }

    return etype;
}
#include "mooutils/moowindow.h"
/* MooCloseResponse */
GType
moo_close_response_get_type (void)
{
    static GType etype;

    if (G_UNLIKELY (!etype))
    {
        static const GEnumValue values[] = {
            { MOO_CLOSE_RESPONSE_CONTINUE, (char*) "MOO_CLOSE_RESPONSE_CONTINUE", (char*) "continue" },
            { MOO_CLOSE_RESPONSE_CANCEL, (char*) "MOO_CLOSE_RESPONSE_CANCEL", (char*) "cancel" },
            { 0, NULL, NULL }
        };

        etype = g_enum_register_static ("MooCloseResponse", values);
    }

    return etype;
}

/* Generated data ends here */

