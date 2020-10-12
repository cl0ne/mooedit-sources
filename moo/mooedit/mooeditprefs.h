/*
 *   mooeditprefs.h
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

#ifndef MOO_EDIT_PREFS_H
#define MOO_EDIT_PREFS_H

#include <mooedit/mooeditor.h>

G_BEGIN_DECLS


#define MOO_EDIT_PREFS_PREFIX "Editor"

GtkWidget  *moo_edit_prefs_page_new_1   (MooEditor  *editor);
GtkWidget  *moo_edit_prefs_page_new_2   (MooEditor  *editor);
GtkWidget  *moo_edit_prefs_page_new_3   (MooEditor  *editor);
GtkWidget  *moo_edit_prefs_page_new_4   (MooEditor  *editor);
GtkWidget  *moo_edit_prefs_page_new_5   (MooEditor  *editor);

/* defined in mooeditprefs.c */
const char *moo_edit_setting            (const char *setting_name);

#define MOO_EDIT_PREFS_TITLE_FORMAT             "window_title"
#define MOO_EDIT_PREFS_TITLE_FORMAT_NO_DOC      "window_title_no_doc"
#define MOO_EDIT_PREFS_USE_TABS                 "use_tabs"
#define MOO_EDIT_PREFS_OPEN_NEW_WINDOW          "open_new_window"

#define MOO_EDIT_PREFS_SAVE_SESSION             "save_session"

#define MOO_EDIT_PREFS_SPACES_NO_TABS           "spaces_instead_of_tabs"
#define MOO_EDIT_PREFS_INDENT_WIDTH             "indent_width"
#define MOO_EDIT_PREFS_TAB_WIDTH                "tab_width"
#define MOO_EDIT_PREFS_AUTO_INDENT              "auto_indent"
#define MOO_EDIT_PREFS_TAB_INDENTS              "tab_indents"
#define MOO_EDIT_PREFS_BACKSPACE_INDENTS        "backspace_indents"

#define MOO_EDIT_PREFS_AUTO_SYNC                "auto_sync"
#define MOO_EDIT_PREFS_AUTO_SAVE                "auto_save"
#define MOO_EDIT_PREFS_AUTO_SAVE_INTERVAL       "auto_save_interval"
#define MOO_EDIT_PREFS_MAKE_BACKUPS             "make_backups"
#define MOO_EDIT_PREFS_STRIP                    "strip"
#define MOO_EDIT_PREFS_ADD_NEWLINE              "add_newline"

#define MOO_EDIT_PREFS_COLOR_SCHEME             "color_scheme"

#define MOO_EDIT_PREFS_SMART_HOME_END           "smart_home_end"
#define MOO_EDIT_PREFS_WRAP_ENABLE              "wrapping_enable"
#define MOO_EDIT_PREFS_WRAP_WORDS               "wrapping_dont_split_words"
#define MOO_EDIT_PREFS_ENABLE_HIGHLIGHTING      "enable_highlighting"
#define MOO_EDIT_PREFS_HIGHLIGHT_MATCHING       "highlight_matching_brackets"
#define MOO_EDIT_PREFS_HIGHLIGHT_MISMATCHING    "highlight_mismatching_brackets"
#define MOO_EDIT_PREFS_HIGHLIGHT_CURRENT_LINE   "highlight_current_line"
#define MOO_EDIT_PREFS_DRAW_RIGHT_MARGIN        "draw_right_margin"
#define MOO_EDIT_PREFS_RIGHT_MARGIN_OFFSET      "right_margin_offset"
#define MOO_EDIT_PREFS_SHOW_LINE_NUMBERS        "show_line_numbers"
#define MOO_EDIT_PREFS_SHOW_TABS                "show_tabs"
#define MOO_EDIT_PREFS_SHOW_SPACES              "show_spaces"
#define MOO_EDIT_PREFS_SHOW_TRAILING_SPACES     "show_trailing_spaces"
#define MOO_EDIT_PREFS_FONT                     "font"
#define MOO_EDIT_PREFS_LINE_NUMBERS_FONT        "line_numbers_font"

#define MOO_EDIT_PREFS_LAST_DIR                 "last_dir"
#define MOO_EDIT_PREFS_PDF_LAST_DIR             "pdf_last_dir"

#define MOO_EDIT_PREFS_DIALOGS                  "dialogs"
#define MOO_EDIT_PREFS_DIALOG_OPEN              MOO_EDIT_PREFS_DIALOGS "/open"
#define MOO_EDIT_PREFS_DIALOGS_OPEN_FOLLOWS_DOC "open_dialog_follows_doc"

#define MOO_EDIT_PREFS_QUICK_SEARCH_FLAGS       "quick_search_flags"
#define MOO_EDIT_PREFS_SEARCH_FLAGS             "search_flags"

#define MOO_EDIT_PREFS_ENCODINGS                "encodings"
#define MOO_EDIT_PREFS_ENCODING_SAVE            "encoding_save"

#define MOO_EDIT_ENCODING_LOCALE "LOCALE"
const char *_moo_get_default_encodings      (void);
const char *_moo_edit_get_default_encoding  (void);

G_END_DECLS

#endif /* MOO_EDIT_PREFS_H */
