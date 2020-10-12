/*
 *   mooeditor-private.h
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

#ifndef MOO_EDITOR_PRIVATE_H
#define MOO_EDITOR_PRIVATE_H

#include "mooedit/mooeditor-impl.h"
#include "mooedit/moolangmgr.h"

G_BEGIN_DECLS

typedef enum {
    OPEN_SINGLE         = 1 << 0,
    ALLOW_EMPTY_WINDOW  = 1 << 1,
    SINGLE_WINDOW       = 1 << 2,
    SAVE_BACKUPS        = 1 << 3,
    STRIP_WHITESPACE    = 1 << 4,
    EMBEDDED            = 1 << 5
} MooEditorOptions;

struct MooEditorPrivate {
    MooEditArray        *windowless;
    MooEditWindowArray  *windows;
    MooUiXml            *doc_ui_xml;
    MooUiXml            *ui_xml;
    MooHistoryMgr       *history;
    MooFileWatch        *file_watch;
    MooEditorOptions     opts;

    GType                window_type;
    GType                doc_type;

    MooLangMgr          *lang_mgr;
};

G_END_DECLS

#ifdef __cplusplus

#include "mooutils/mooutils-cpp.h"

MOO_DEFINE_FLAGS(MooEditorOptions);

#endif // __cplusplus

#endif /* MOO_EDITOR_PRIVATE_H */
