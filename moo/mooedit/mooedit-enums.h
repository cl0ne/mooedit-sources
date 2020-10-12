#ifndef MOO_EDIT_ENUMS_H
#define MOO_EDIT_ENUMS_H

#include <mooedit/mooedit-enum-types.h>

G_BEGIN_DECLS

typedef enum {
    MOO_EDIT_CONFIG_SOURCE_USER = 0,
    MOO_EDIT_CONFIG_SOURCE_FILE = 10,
    MOO_EDIT_CONFIG_SOURCE_FILENAME = 20,
    MOO_EDIT_CONFIG_SOURCE_LANG = 30,
    MOO_EDIT_CONFIG_SOURCE_AUTO = 40
} MooEditConfigSource;

/**
 * enum:MooSaveResponse
 *
 * @MOO_SAVE_RESPONSE_CONTINUE: continue invoking before-save callbacks, save if there aren't any left
 * @MOO_SAVE_RESPONSE_CANCEL: stop invoking before-save callbacks and do not save
 *
 * Values returned by MooEditor::before-save and MooEdit::before-save
 * callbacks.
 **/
typedef enum {
    MOO_SAVE_RESPONSE_CONTINUE = 2,
    MOO_SAVE_RESPONSE_CANCEL = 3
} MooSaveResponse;

typedef enum {
    MOO_EDIT_STATE_NORMAL,
    MOO_EDIT_STATE_LOADING,
    MOO_EDIT_STATE_SAVING,
    MOO_EDIT_STATE_PRINTING
} MooEditState;

typedef enum {
    MOO_EDIT_STATUS_NORMAL           = 0,
    MOO_EDIT_STATUS_MODIFIED_ON_DISK = 1 << 0,
    MOO_EDIT_STATUS_DELETED          = 1 << 1,
    MOO_EDIT_STATUS_CHANGED_ON_DISK  = MOO_EDIT_STATUS_MODIFIED_ON_DISK | MOO_EDIT_STATUS_DELETED,
    MOO_EDIT_STATUS_MODIFIED         = 1 << 2,
    MOO_EDIT_STATUS_NEW              = 1 << 3,
    MOO_EDIT_STATUS_CLEAN            = 1 << 4
} MooEditStatus;

#ifdef __WIN32__
#define MOO_LE_NATIVE_VALUE MOO_LE_WIN32
#else
#define MOO_LE_NATIVE_VALUE MOO_LE_UNIX
#endif

/**
 * enum:MooLineEndType
 *
 * @MOO_LE_UNIX: UNIX line endings (<code>\n</code>)
 * @MOO_LE_WIN32: Windows line endings (<code>\r\n</code>)
 * @MOO_LE_MAC: classic Mac line endings (<code>\r</code>)
 * @MOO_LE_NATIVE: equal to #MOO_LE_UNIX on UNIX systems (including Mac OS X) and #MOO_LE_WIN32 on Windows
 **/
typedef enum {
    MOO_LE_NONE,
    MOO_LE_UNIX,
    MOO_LE_WIN32,
    MOO_LE_MAC,
    MOO_LE_NATIVE = MOO_LE_NATIVE_VALUE
} MooLineEndType;

/**
 * flags:MooOpenFlags
 *
 * @MOO_OPEN_FLAG_NEW_WINDOW: open document in a new window
 * @MOO_OPEN_FLAG_NEW_TAB: open document in existing window (default)
 * @MOO_OPEN_FLAG_RELOAD: reload document if it's already open
 * @MOO_OPEN_FLAG_CREATE_NEW: if file doesn't exist on disk, create a new document
 **/
typedef enum {
    MOO_OPEN_FLAGS_NONE      = 0,
    MOO_OPEN_FLAG_NEW_WINDOW = 1 << 0,
    MOO_OPEN_FLAG_NEW_TAB    = 1 << 1,
    MOO_OPEN_FLAG_RELOAD     = 1 << 2,
    MOO_OPEN_FLAG_CREATE_NEW = 1 << 3
} MooOpenFlags;

typedef enum {
    MOO_TEXT_SELECT_CHARS,
    MOO_TEXT_SELECT_WORDS,
    MOO_TEXT_SELECT_LINES
} MooTextSelectionType;

typedef enum {
    MOO_TEXT_SEARCH_CASELESS = 1 << 0,
    MOO_TEXT_SEARCH_REGEX = 1 << 1,
    MOO_TEXT_SEARCH_WHOLE_WORDS = 1 << 2,
    MOO_TEXT_SEARCH_REPL_LITERAL = 1 << 3
} MooTextSearchFlags;

typedef enum {
    MOO_FIND_REGEX = 1 << 0,
    MOO_FIND_CASELESS = 1 << 1,
    MOO_FIND_IN_SELECTED = 1 << 2,
    MOO_FIND_BACKWARDS = 1 << 3,
    MOO_FIND_WHOLE_WORDS = 1 << 4,
    MOO_FIND_FROM_CURSOR = 1 << 5,
    MOO_FIND_DONT_PROMPT = 1 << 6,
    MOO_FIND_REPL_LITERAL = 1 << 7
} MooFindFlags;

typedef enum {
    MOO_DRAW_WS_NONE     = 0,
    MOO_DRAW_WS_SPACES   = 1 << 0,
    MOO_DRAW_WS_TABS     = 1 << 1,
    MOO_DRAW_WS_TRAILING = 1 << 2
} MooDrawWsFlags;

/**
 * enum:MooActionCheckType: (moo.lua 0) (moo.private 1)
 *
 * @MOO_ACTION_CHECK_SENSITIVE:
 * @MOO_ACTION_CHECK_VISIBLE:
 * @MOO_ACTION_CHECK_ACTIVE:
 **/
typedef enum {
    MOO_ACTION_CHECK_SENSITIVE,
    MOO_ACTION_CHECK_VISIBLE,
    MOO_ACTION_CHECK_ACTIVE
} MooActionCheckType;

typedef enum {
    MOO_TEXT_CURSOR_NONE,
    MOO_TEXT_CURSOR_TEXT,
    MOO_TEXT_CURSOR_ARROW,
    MOO_TEXT_CURSOR_LINK
} MooTextCursor;

G_END_DECLS

#ifdef __cplusplus

#include "mooutils/mooutils-cpp.h"

MOO_DEFINE_FLAGS(MooEditStatus)
MOO_DEFINE_FLAGS(MooOpenFlags)
MOO_DEFINE_FLAGS(MooTextSearchFlags)
MOO_DEFINE_FLAGS(MooFindFlags)
MOO_DEFINE_FLAGS(MooDrawWsFlags)

#endif // __cplusplus

#endif /* MOO_EDIT_ENUMS_H */
