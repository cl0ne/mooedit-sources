/*
 *   moocommand-script.cpp
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

#include <config.h>
#include "moocommand-script.h"
#include "plugins/usertools/lua-tool-setup.h"
#ifdef MOO_ENABLE_PYTHON
#include "plugins/usertools/python-tool-setup.h"
#endif
#include "mooedit/mooeditor.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#include "plugins/usertools/mooedittools-script-gxml.h"
#include "moolua/medit-lua.h"
#include "moopython/medit-python.h"
#include <string.h>

struct MooCommandFactoryScript
{
    MooCommandFactory base;
    MooScriptType type;
};

struct _MooCommandScriptPrivate {
    MooScriptType type;
    char *code;
};

G_DEFINE_TYPE (MooCommandScript, _moo_command_script, MOO_TYPE_COMMAND)

typedef struct MooCommandFactoryScript MooCommandFactoryScript;
typedef MooCommandFactoryClass MooCommandFactoryScriptClass;
MOO_DEFINE_TYPE_STATIC (MooCommandFactoryScript, _moo_command_factory_script, MOO_TYPE_COMMAND_FACTORY)

static MooCommand  *moo_command_script_new (MooScriptType     type,
                                            const char       *code,
                                            MooCommandOptions options);

static void
moo_command_script_run_lua (MooCommandScript  *cmd,
                            MooCommandContext *ctx)
{
    GtkTextBuffer *buffer = NULL;
    lua_State *L;

    g_return_if_fail (cmd->code != NULL);

    L = medit_lua_new ();
    g_return_if_fail (L != NULL);

    if (!medit_lua_do_string (L, LUA_TOOL_SETUP_LUA))
    {
        medit_lua_free (L);
        return;
    }

    if (luaL_loadstring (L, cmd->code) != 0)
    {
        const char *msg = lua_tostring (L, -1);
        g_critical ("%s", msg ? msg : "ERROR");
        medit_lua_free (L);
        return;
    }

    if (moo_command_context_get_doc (ctx))
        buffer = moo_edit_get_buffer (moo_command_context_get_doc (ctx));

    if (buffer)
        gtk_text_buffer_begin_user_action (buffer);

    if (lua_pcall (L, 0, 0, 0) != 0)
    {
        const char *msg = lua_tostring (L, -1);
        g_critical ("%s", msg ? msg : "ERROR");
        lua_pop (L, 1);
    }

    if (buffer)
        gtk_text_buffer_end_user_action (buffer);

    medit_lua_free (L);
}

static void
moo_command_script_run_python (MooCommandScript  *cmd,
                               MooCommandContext *ctx)
{
#ifdef MOO_ENABLE_PYTHON
    GtkTextBuffer *buffer = NULL;
    MooPythonState *state;

    g_return_if_fail (cmd->code != NULL);

    state = moo_python_state_new (TRUE);
    g_return_if_fail (state != NULL);

    if (!moo_python_run_string (state, PYTHON_TOOL_SETUP_PY))
    {
        moo_python_state_free (state);
        return;
    }

    if (moo_command_context_get_doc (ctx))
        buffer = moo_edit_get_buffer (moo_command_context_get_doc (ctx));

    if (buffer)
        gtk_text_buffer_begin_user_action (buffer);

    moo_python_run_string (state, cmd->code);

    if (buffer)
        gtk_text_buffer_end_user_action (buffer);

    moo_python_state_free (state);
#else
    g_return_if_reached ();
    (void) cmd;
    (void) ctx;
#endif
}

static void
moo_command_script_run (MooCommand        *cmd_base,
                        MooCommandContext *ctx)
{
    MooCommandScript *cmd = MOO_COMMAND_SCRIPT (cmd_base);

    switch (cmd->type)
    {
        case MOO_SCRIPT_LUA:
            moo_command_script_run_lua (cmd, ctx);
            break;
        case MOO_SCRIPT_PYTHON:
            moo_command_script_run_python (cmd, ctx);
            break;
        default:
            g_return_if_reached ();
    }
}


static void
moo_command_script_dispose (GObject *object)
{
    MooCommandScript *cmd = MOO_COMMAND_SCRIPT (object);

    g_free (cmd->code);
    cmd->code = NULL;

    G_OBJECT_CLASS(_moo_command_script_parent_class)->dispose (object);
}


static MooCommand *
script_factory_create_command (MooCommandFactory *factory_base,
                               MooCommandData    *data,
                               const char        *options)
{
    MooCommandFactoryScript *factory = (MooCommandFactoryScript*) factory_base;
    MooCommand *cmd;
    const char *code;

    code = moo_command_data_get_code (data);
    g_return_val_if_fail (code && *code, NULL);

    cmd = moo_command_script_new (factory->type, code, moo_parse_command_options (options));
    g_return_val_if_fail (cmd != NULL, NULL);

    return cmd;
}


static GtkWidget *
script_factory_create_widget (MooCommandFactory *factory_base)
{
    MooCommandFactoryScript *factory = (MooCommandFactoryScript*) factory_base;
    ScriptPageXml *xml;

    xml = script_page_xml_new ();

    moo_text_view_set_font_from_string (xml->textview, "Monospace");

    switch (factory->type)
    {
        case MOO_SCRIPT_LUA:
            moo_text_view_set_lang_by_id (xml->textview, "lua");
            break;
        case MOO_SCRIPT_PYTHON:
            moo_text_view_set_lang_by_id (xml->textview, "python");
            break;
    }

    return GTK_WIDGET (xml->ScriptPage);
}


static void
script_factory_load_data (G_GNUC_UNUSED MooCommandFactory *factory,
                          GtkWidget      *page,
                          MooCommandData *data)
{
    ScriptPageXml *xml;
    GtkTextBuffer *buffer;
    const char *code;

    xml = script_page_xml_get (page);
    g_return_if_fail (xml != NULL);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xml->textview));

    code = moo_command_data_get_code (data);
    gtk_text_buffer_set_text (buffer, MOO_NZS (code), -1);
}


static gboolean
script_factory_save_data (G_GNUC_UNUSED MooCommandFactory *factory,
                          GtkWidget      *page,
                          MooCommandData *data)
{
    ScriptPageXml *xml;
    const char *code;
    char *new_code;
    gboolean changed = FALSE;

    xml = script_page_xml_get (page);
    g_return_val_if_fail (xml != NULL, FALSE);

    new_code = moo_text_view_get_text (GTK_TEXT_VIEW (xml->textview));
    code = moo_command_data_get_code (data);

    if (!moo_str_equal (code, new_code))
    {
        moo_command_data_set_code (data, new_code);
        changed = TRUE;
    }

    g_free (new_code);
    return changed;
}

static void
_moo_command_factory_script_init (G_GNUC_UNUSED MooCommandFactoryScript *factory)
{
}

static void
_moo_command_factory_script_class_init (MooCommandFactoryClass *klass)
{
    klass->create_command = script_factory_create_command;
    klass->create_widget = script_factory_create_widget;
    klass->load_data = script_factory_load_data;
    klass->save_data = script_factory_save_data;
}

static void
_moo_command_script_class_init (MooCommandScriptClass *klass)
{
    MooCommandFactoryScript *factory;

    G_OBJECT_CLASS (klass)->dispose = moo_command_script_dispose;
    MOO_COMMAND_CLASS (klass)->run = moo_command_script_run;

    factory = (MooCommandFactoryScript*) g_object_new (_moo_command_factory_script_get_type (), (const char*) NULL);
    factory->type = MOO_SCRIPT_LUA;
    moo_command_factory_register ("lua", _("Lua script"), MOO_COMMAND_FACTORY (factory), NULL, ".lua");
    g_object_unref (factory);

#ifdef MOO_ENABLE_PYTHON
    factory = (MooCommandFactoryScript*) g_object_new (_moo_command_factory_script_get_type (), (const char*) NULL);
    factory->type = MOO_SCRIPT_PYTHON;
    moo_command_factory_register ("python", _("Python script"), MOO_COMMAND_FACTORY (factory), NULL, ".py");
    g_object_unref (factory);
#endif
}

static void
_moo_command_script_init (G_GNUC_UNUSED MooCommandScript *cmd)
{
}


static MooCommand *
moo_command_script_new (MooScriptType      type,
                        const char        *code,
                        MooCommandOptions  options)
{
    MooCommandScript *cmd;

    g_return_val_if_fail (code != NULL, NULL);

    cmd = MOO_COMMAND_SCRIPT (g_object_new (MOO_TYPE_COMMAND_SCRIPT, "options", options, (const char*) NULL));
    cmd->code = g_strdup (code);
    cmd->type = type;

    return MOO_COMMAND (cmd);
}
