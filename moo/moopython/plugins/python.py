#
#  python.py
#
#  Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
#
#  This file is part of medit.  medit is free software; you can
#  redistribute it and/or modify it under the terms of the
#  GNU Lesser General Public License as published by the
#  Free Software Foundation; either version 2.1 of the License,
#  or (at your option) any later version.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with medit.  If not, see <http://www.gnu.org/licenses/>.
#

import gtk
import gobject
import pango
import os

import moo
from moo import _
import medit.runpython

MASTER_FILE = 'PYTHON_MASTER_FILE'

try:
    import pyconsole
    have_pyconsole = True
except ImportError:
    have_pyconsole = False

PLUGIN_ID = "Python"

class Plugin(moo.Plugin):
    def do_init(self):
        editor = moo.Editor.instance()
        xml = editor.get_ui_xml()

        if xml is None:
            # standalone python module, not in medit
            return False

        self.ui_merge_id = xml.new_merge_id()

        if have_pyconsole:
            moo.window_class_add_action(moo.EditWindow, "PythonConsole",
                                        display_name=_("Python Console"),
                                        label=_("Python Console"),
                                        callback=self.show_console)
            xml.add_item(self.ui_merge_id, "ToolsMenu",
                         "PythonConsole", "PythonConsole", -1)

        """ Run file """
        moo.window_class_add_action(moo.EditWindow, "RunFile",
                                    display_name=_("Run File"),
                                    label=_("Run File"),
                                    stock_id=moo.STOCK_EXECUTE,
                                    default_accel="<shift>F9",
                                    callback=self.run_file)
        moo.edit_window_set_action_filter("RunFile", moo.ACTION_CHECK_SENSITIVE, "langs:python")
        xml.add_item(self.ui_merge_id, "ToolsMenu",
                     "RunFile", "RunFile", -1)

        return True

    def do_deinit(self):
        editor = moo.Editor.instance()
        xml = editor.get_ui_xml()
        xml.remove_ui(self.ui_merge_id)
        moo.window_class_remove_action(moo.EditWindow, "PythonConsole")
        moo.window_class_remove_action(moo.EditWindow, "RunFile")
        moo.window_class_remove_action(moo.EditWindow, "ReloadPythonPlugins")

    def show_console(self, window):
        window = gtk.Window()
        swin = gtk.ScrolledWindow()
        swin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
        window.add(swin)

        dic = {}
        dic['editor'] = moo.Editor.instance()
        dic['window'] = moo.Editor.instance().get_active_window()
        dic['view'] = moo.Editor.instance().get_active_view()
        dic['doc'] = moo.Editor.instance().get_active_doc()
        console_type = pyconsole.ConsoleType(moo.TextView)
        console = console_type(use_rlcompleter=False, start_script=
                               "import moo\nimport gtk\n", locals=dic)
        console.set_property("highlight-current-line", False)
        editor = moo.Editor.instance()
        console.set_lang_by_id("python-console")
        console.modify_font(pango.FontDescription("Monospace"))

        swin.add(console)
        window.set_default_size(400,300)
        window.set_title("pythony")
        window.show_all()

    def run_file(self, window):
        filename = os.environ.get(MASTER_FILE)

        if filename is None:
            doc = window.get_active_doc()

            if not doc:
                return
            if not doc.get_filename() or doc.get_status() & moo.EDIT_STATUS_MODIFIED:
                if not doc.save():
                    return

            filename = doc.get_filename()

        r = medit.runpython.Runner(window)
        r.run(filename)

    def do_detach_win(self, window):
        window.remove_pane(PLUGIN_ID)


gobject.type_register(Plugin)
__plugin__ = Plugin
