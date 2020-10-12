#
#  moo-mod.py
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

""" moo module """
import _moo
import gtk as _gtk
import gobject as _gobject
from _moo import *

# note, these aren't standard gettext functions
_ = gettext
D_ = dgettext

def N_(string):
    return string

class _PluginWontLoad(Exception):
    def __repr__(self):
        return 'PluginWontLoad'

def _cancel_plugin_loading():
    raise _PluginWontLoad()

class _ActionFactory(object):
    def __init__(self, action_id, **kwargs):
        object.__init__(self)
        self.id = action_id

        self.props = {}
        self.fake_props = {}

        for key in kwargs.keys():
            if key in ["callback"]:
                self.fake_props[key] = kwargs[key]
            elif kwargs[key] is not None or key not in ["stock_id"]:
                self.props[key] = kwargs[key]

    def __call__(self, window):
        self.window = window
        action = _gobject.new(Action, name=self.id, **self.props)
        self.set_fake_props(action)
        return action

    def set_fake_props(self, action):
        def _activate(action, callback, window):
            callback(window)

        for key in self.fake_props.keys():
            if key == "callback":
                action.connect("activate", _activate, self.fake_props[key], self.window)
            else:
                raise ValueError("unknown property " + key)

    def set_props(self, action):
        for key in self.props.keys():
            action.set_property(key, self.props[key])

def window_class_add_action(klass, action_id, group=None, **kwargs):
    if kwargs.has_key("factory"):
        _moo.window_class_add_action(klass, action_id, group, kwargs["factory"])
    else:
        _moo.window_class_add_action(klass, action_id, group, _ActionFactory(action_id, **kwargs))
