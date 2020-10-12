#
#  medit/runpython.py
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

import sys
import os
import re
import gtk
import moo
from moo import _

if os.name == 'nt':
    PYTHON_COMMAND = '"' + sys.exec_prefix + '\\pythonw.exe" -u'
else:
    PYTHON_COMMAND = 'python -u'

PANE_ID = 'PythonOutput'

class Runner(object):
    def __init__(self, window, python_command=PYTHON_COMMAND, pane_id=PANE_ID, pane_label=None):
        self.window = window
        self.python_command = python_command
        self.pane_id = pane_id
        self.pane_label = pane_label

    def __get_output(self):
        return self.window.get_pane(self.pane_id)
    def __ensure_output(self):
        pane = self.__get_output()
        if pane is None:
            label = self.pane_label or moo.PaneLabel(icon_name=moo.STOCK_EXECUTE,
                                                     label_text=_("Python Output"))
            output = moo.CmdView()
            output.set_property("highlight-current-line", True)
            output.set_filter_by_id("python")

            pane = gtk.ScrolledWindow()
            pane.set_shadow_type(gtk.SHADOW_ETCHED_IN)
            pane.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
            pane.add(output)
            pane.show_all()

            pane.output = output
            self.window.add_pane(self.pane_id, pane, label, moo.PANE_POS_BOTTOM)
            self.window.add_stop_client(output)
        return pane

    def run(self, filename=None, args_string=None, working_dir=None):
        pane = self.__get_output()

        if pane is not None and pane.output.running():
            return

        if filename is None:
            doc = self.window.get_active_doc()

            if not doc:
                return
            if not doc.get_filename() or doc.get_status() & moo.EDIT_MODIFIED:
                if not doc.save():
                    return

            filename = doc.get_filename()

        pane = self.__ensure_output()
        pane.output.clear()
        self.window.show_pane(self.pane_id)

        if working_dir is None:
            working_dir = os.path.dirname(filename)
        cmd_line = self.python_command + ' "%s"' % os.path.basename(filename)
        if args_string is not None:
            cmd_line += ' %s' % (args_string,)
        pane.output.run_command(cmd_line, working_dir)
