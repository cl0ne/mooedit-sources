#
#  terminal.py
#
#  Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
#  Copyright (C) 2014 by Yannick Duchêne
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

import os
import moo
import gtk
import gobject
import pango
from moo import _
from moo import D_

try:
    import vte
except ImportError:
    moo._cancel_plugin_loading()

TERMINAL_PLUGIN_ID = "Terminal"
COLOR_SCHEME_KEY = 'Plugins/Terminal/color_scheme'
SHELL_KEY = 'Plugins/Terminal/shell'
FONT_KEY = 'Plugins/Terminal/font'
moo.prefs_new_key_string(COLOR_SCHEME_KEY, 'Default')
moo.prefs_new_key_string(SHELL_KEY, None)
moo.prefs_new_key_string(FONT_KEY, None)


def shell_supports_pushd(shell):
   # There is no counterpart function to test whether or not a shell supports
   # the “cd” command: they all support it, even MS‑DOS's “command.com” did.
   name = os.path.split(shell)[1]
   # If you happen know another shell which supports “pushd”, add its name to
   # the list
   return name in ["bash"]


class Terminal(vte.Terminal):
    def __init__(self):
        vte.Terminal.__init__(self)

        cs_name = moo.prefs_get_string(COLOR_SCHEME_KEY)
        cs = find_color_scheme(cs_name)
        self.set_color_scheme(cs)
        self.set_allow_bold(False)

        font_name = moo.prefs_get_string(FONT_KEY)
        if font_name:
            self.set_font_from_string(font_name)

        self._support_pushd = False

    def set_support_pushd(self, status):
        self._support_pushd = status

    def set_color_scheme(self, cs):
        self.__cs = cs
        if cs:
            cs.set_on_terminal(self)

    def color_scheme_item_activated(self, item, color_scheme):
        self.set_color_scheme(color_scheme)
        if color_scheme.colors:
            moo.prefs_set_string(COLOR_SCHEME_KEY, color_scheme.name)
        else:
            moo.prefs_set_string(COLOR_SCHEME_KEY, None)

    def font_item_activated(self, *whatever):
        dlg = gtk.FontSelectionDialog(D_("Pick a Font", "gtk20"))
        # dlg.fontsel.set_property('monospace', True)
        old_font = self.get_font()
        if old_font:
            dlg.set_font_name(old_font.to_string())
        if dlg.run() == gtk.RESPONSE_OK:
            font_name = dlg.get_font_name()
            if font_name:
                new_font = pango.FontDescription(font_name)
                if old_font != new_font:
                    self.set_font_from_string(font_name)
                    moo.prefs_set_string(FONT_KEY, font_name)
        dlg.destroy()

    def fill_settings_menu(self, menu):
        item = gtk.ImageMenuItem(gtk.STOCK_SELECT_FONT)
        item.connect('activate', self.font_item_activated)
        menu.append(item)

        item = gtk.ImageMenuItem(gtk.STOCK_SELECT_COLOR)
        submenu = gtk.Menu()
        item.set_submenu(submenu)
        group = None
        for cs in color_schemes:
            child = gtk.RadioMenuItem(group, cs.name, False)
            group = child
            submenu.append(child)
            if self.__cs == cs:
                child.set_active(True)
            child.connect('activate', self.color_scheme_item_activated, cs)
        menu.append(item)

    def popup_menu(self, event=None):
        menu = gtk.Menu()

        item = gtk.ImageMenuItem(gtk.STOCK_COPY)
        item.connect('activate', lambda *whatever: self.copy_clipboard())
        item.set_sensitive(self.get_has_selection())
        menu.append(item)
        item = gtk.ImageMenuItem(gtk.STOCK_PASTE)
        item.connect('activate', lambda *whatever: self.paste_clipboard())
        menu.append(item)

        item = gtk.MenuItem(_("“cd” to current file directory"))
        item.connect(
            'activate',
            lambda *whatever: self.goto_file_dir(False))
        menu.append(item)
        if self._support_pushd:
            item = gtk.MenuItem(_("“pushd” to current file directory"))
            item.connect(
                'activate',
                lambda *whatever: self.goto_file_dir(True))
            menu.append(item)

        menu.append(gtk.SeparatorMenuItem())
        item = gtk.MenuItem(D_("Input _Methods", "gtk20"))
        submenu = gtk.Menu()
        item.set_submenu(submenu)
        self.im_append_menuitems(submenu)
        menu.append(item)

        menu.append(gtk.SeparatorMenuItem())
        item = gtk.ImageMenuItem(gtk.STOCK_PROPERTIES)
        submenu = gtk.Menu()
        item.set_submenu(submenu)
        menu.append(item)

        self.fill_settings_menu(submenu)

        if event is not None:
            button = event.button
            time = event.time
        else:
            button = 0
            time = 0
        menu.show_all()
        menu.popup(None, None, None, button, time)

    def goto_file_dir(self, pushd=False):
        # TODO: be able to check if the user was editing a command. Would be
        # nice to able to not append to a command being edited and to only
        # send this command only if there's nothing entered at the current
        # shell prompt. Additionally, being able to disable the corresponding
        # contextual menu entry under the same conditions.
        ed = moo.Editor.instance()
        doc = ed.get_active_doc()
        path = doc.get_filename()
        path = os.path.split(path)[0]
        if pushd:
            command = "pushd"
        else:
            command = "cd"
        command += " '" + path + "'\n"
        self.feed_child(command)

    def do_button_press_event(self, event):
        if event.button != 3 or event.type != gtk.gdk.BUTTON_PRESS:
            return vte.Terminal.do_button_press_event(self, event)
        self.grab_focus()
        self.popup_menu(event)
        return True

    def do_popup_menu(self):
        self.popup_menu()
        return True

class Plugin(moo.Plugin):
    def do_init(self):
        editor = moo.Editor.instance()
        xml = editor.get_ui_xml()

        if xml is None:
            return False

        self.set_win_plugin_type(WinPlugin)
        return True

    def do_deinit(self):
        pass

    def show_terminal(self, window):
        pane = window.get_pane(TERMINAL_PLUGIN_ID)
        window.paned.present_pane(pane)


class WinPlugin(moo.WinPlugin):
    def start(self, *whatever):
        self.terminal.reset(True, True)

        shell = moo.prefs_get_string(SHELL_KEY)
        if not shell:
            try:
                import os
                import pwd
                shell = pwd.getpwuid(os.getuid())[6]
            except:
                shell = "/bin/sh"

        self.terminal.set_support_pushd(shell_supports_pushd(shell))
        self.terminal.fork_command(shell, [shell])

    def do_create(self):
        label = moo.PaneLabel(icon_name=moo.STOCK_TERMINAL,
                              label_text=_("Terminal"))

        self.terminal = Terminal()
        self.terminal.set_scrollback_lines(1000000)
        self.terminal.connect("child-exited", self.start)
        self.start()

        frame = gtk.Frame()
        hbox = gtk.HBox()
        frame.add(hbox)
        hbox.pack_start(self.terminal)
        scrollbar = gtk.VScrollbar(self.terminal.get_adjustment())
        hbox.pack_start(scrollbar, False, False, 0)
        frame.show_all()

        self.terminal.set_size(self.terminal.get_column_count(), 10)
        self.terminal.set_size_request(10, 10)

        self.pane = self.get_window().add_pane(TERMINAL_PLUGIN_ID, frame, label, moo.PANE_POS_BOTTOM)
        self.icon_title_changed_cb_id = self.terminal.connect('icon-title-changed', self.terminal_icon_title_changed)
        self.terminal_icon_title_changed()

        return True

    def do_destroy(self):
        self.terminal.disconnect(self.icon_title_changed_cb_id)
        self.get_window().remove_pane(TERMINAL_PLUGIN_ID)

    def terminal_icon_title_changed(self, *whatever):
        self.pane.set_frame_text(self.terminal.get_icon_title())


class ColorScheme(object):
    def __init__(self, name, colors):
        object.__init__(self)
        self.name = name
        if colors is None:
            self.colors = None
        else:
            self.colors = [gtk.gdk.color_parse(c) for c in colors]

    def set_on_terminal(self, term):
        if self.colors is not None:
            term.set_colors(self.colors[0], self.colors[1], self.colors[2:10] + self.colors[12:20])
        else:
            term.set_colors(term.style.text[gtk.STATE_NORMAL], term.style.base[gtk.STATE_NORMAL], [])


# Color schemes shamelessly stolen from Konsole, the best terminal emulator out there
color_schemes = [ColorScheme(cs[0], cs[1]) for cs in [
    [_("Default"), None],
    [_("Black on White"),
        ['#000000', '#ffffff', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#000000', '#ffffff', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("Black on Light Yellow"),
        ['#000000', '#ffffdd', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#000000', '#ffffdd', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("Marble"),
        ['#ffffff', '#000000', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#ffffff', '#000000', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("Green on Black"),
        ['#18f018', '#000000', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#18f018', '#000000', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("Paper, Light"),
        ['#000000', '#ffffff', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#000000', '#ffffff', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("Paper"),
        ['#000000', '#ffffff', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#000000', '#ffffff', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("Linux Colors"),
        ['#b2b2b2', '#000000', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#ffffff', '#686868', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']],
    [_("VIM Colors"),
        ['#000000', '#ffffff', '#000000', '#c00000', '#008000', '#808000', '#0000c0', '#c000c0', '#008080', '#c0c0c0',
         '#4d4d4d', '#ffffff', '#808080', '#ff6060', '#00ff00', '#ffff00', '#8080ff', '#ff40ff', '#00ffff', '#ffffff']],
    [_("White on Black"),
        ['#ffffff', '#000000', '#000000', '#b21818', '#18b218', '#b26818', '#1818b2', '#b218b2', '#18b2b2', '#b2b2b2',
         '#ffffff', '#000000', '#686868', '#ff5454', '#54ff54', '#ffff54', '#5454ff', '#ff54ff', '#54ffff', '#ffffff']]
]]

def find_color_scheme(name):
    for cs in color_schemes:
        if cs.name == name:
            return cs

gobject.type_register(Terminal)
gobject.type_register(Plugin)
gobject.type_register(WinPlugin)
__plugin__ = Plugin
