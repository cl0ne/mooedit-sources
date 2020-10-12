#
#  insert_date_and_time.py
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
import time
import moo

# List of formats is shamelessly stolen from gedit
formats = [
    "%c",
    "%x",
    "%X",
    "%x %X",
    "%Y-%m-%d %H:%M:%S",
    "%a %b %d %H:%M:%S %Z %Y",
    "%a %b %d %H:%M:%S %Y",
    "%a %d %b %Y %H:%M:%S %Z",
    "%a %d %b %Y %H:%M:%S",
    "%d/%m/%Y",
    "%d/%m/%y",
    "%D",
    "%A %d %B %Y",
    "%A %B %d %Y",
    "%Y-%m-%d",
    "%d %B %Y",
    "%B %d, %Y",
    "%A %b %d",
    "%H:%M:%S",
    "%H:%M",
    "%I:%M:%S %p",
    "%I:%M %p",
    "%H.%M.%S",
    "%H.%M",
    "%I.%M.%S %p",
    "%I.%M %p",
    "%d/%m/%Y %H:%M:%S",
    "%d/%m/%y %H:%M:%S",
]

moo.prefs_new_key_string('Tools/InsertDateAndTime', '%c')

def populate_tree_view(treeview):
    model = gtk.ListStore(str, str)
    curtime = time.localtime()
    default_iter = None
    default_fmt = moo.prefs_get_string('Tools/InsertDateAndTime')

    for fmt in formats:
        iter = model.append([time.strftime(fmt, curtime), fmt])
        if default_fmt == fmt:
            default_iter = iter

    cell = gtk.CellRendererText()
    column = gtk.TreeViewColumn(None, cell, text=0)
    treeview.append_column(column)
    treeview.set_model(model)
    if default_iter is not None:
        treeview.get_selection().select_iter(default_iter)

def get_format_from_list(treeview):
    model, row = treeview.get_selection().get_selected()
    fmt = model[row][1]
    moo.prefs_set_string('Tools/InsertDateAndTime', fmt)
    return fmt

def get_format(parent=None):
    window = gtk.Dialog("Select Format", parent, 0,
                        (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                         gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
    window.set_alternative_button_order([gtk.RESPONSE_ACCEPT, gtk.RESPONSE_REJECT])
    window.set_size_request(250, 300)
    swindow = gtk.ScrolledWindow()
    treeview = gtk.TreeView()
    swindow.add(treeview)
    swindow.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
    swindow.show_all()
    window.vbox.pack_start(swindow)

    populate_tree_view(treeview)
    treeview.set_headers_visible(False)
    treeview.connect('row-activated', lambda tv, path, clmn, dlg:
                                        dlg.response(gtk.RESPONSE_ACCEPT), window)

    fmt = None
    if window.run() == gtk.RESPONSE_ACCEPT:
        fmt = get_format_from_list(treeview)

    window.destroy()
    return fmt

if __name__ == '__main__':
    print get_format()
