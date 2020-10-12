/*
 *   mooaccel.h
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

#ifndef MOO_ACCEL_H
#define MOO_ACCEL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


void         _moo_accel_register            (const char     *accel_path,
                                             const char     *default_accel);

const char  *_moo_get_accel                 (const char     *accel_path);
const char  *_moo_get_default_accel         (const char     *accel_path);

void         _moo_modify_accel              (const char     *accel_path,
                                             const char     *new_accel);

gboolean     _moo_accel_prefs_get_global    (const char     *accel_path);
void         _moo_accel_prefs_set_global    (const char     *accel_path,
                                             gboolean        global);

char        *_moo_get_accel_label           (const char     *accel);
gboolean     _moo_accel_parse               (const char     *accel,
                                             guint          *key,
                                             GdkModifierType *mods);

void          moo_accel_translate_event     (GtkWidget       *widget,
                                             GdkEventKey     *event,
                                             guint           *keyval,
                                             GdkModifierType *mods);
gboolean      moo_accel_check_event         (GtkWidget       *widget,
                                             GdkEventKey     *event,
                                             guint            keyval,
                                             GdkModifierType  mods);

gboolean moo_keymap_translate_keyboard_state (GdkKeymap           *keymap,
                                              guint                hardware_keycode,
                                              GdkModifierType      state,
                                              gint                 group,
                                              guint               *keyval,
                                              gint                *effective_group,
                                              gint                *level,
                                              GdkModifierType     *consumed_modifiers);


#define MOO_ACCEL_MODS_MASK (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK | GDK_META_MASK)

#ifndef GDK_WINDOWING_QUARTZ

#define MOO_ACCEL_CTRL "<Ctrl>"
#define MOO_ACCEL_CTRL_MASK GDK_CONTROL_MASK

#define MOO_ACCEL_HELP_KEY  GDK_F1
#define MOO_ACCEL_HELP_MODS ((GdkModifierType)0)

#else /* GDK_WINDOWING_QUARTZ */

#define MOO_ACCEL_CTRL "<Meta>"
#define MOO_ACCEL_CTRL_MASK GDK_META_MASK

#define MOO_ACCEL_HELP_KEY  GDK_question
#define MOO_ACCEL_HELP_MODS GDK_META_MASK

#endif /* GDK_WINDOWING_QUARTZ */

#define MOO_ACCEL_NEW MOO_ACCEL_CTRL "N"
#define MOO_ACCEL_OPEN MOO_ACCEL_CTRL "O"
#define MOO_ACCEL_SAVE MOO_ACCEL_CTRL "S"
#define MOO_ACCEL_SAVE_AS MOO_ACCEL_CTRL "<Shift>S"
#define MOO_ACCEL_CLOSE MOO_ACCEL_CTRL "W"

#define MOO_ACCEL_UNDO MOO_ACCEL_CTRL "Z"
#define MOO_ACCEL_REDO MOO_ACCEL_CTRL "<Shift>Z"
#define MOO_ACCEL_CUT MOO_ACCEL_CTRL "X"
#define MOO_ACCEL_COPY MOO_ACCEL_CTRL "C"
#define MOO_ACCEL_PASTE MOO_ACCEL_CTRL "V"
#define MOO_ACCEL_SELECT_ALL MOO_ACCEL_CTRL "A"

#define MOO_ACCEL_PAGE_SETUP MOO_ACCEL_CTRL "<Shift>P"
#define MOO_ACCEL_PRINT MOO_ACCEL_CTRL "P"


G_END_DECLS

#endif /* MOO_ACCEL_H */
