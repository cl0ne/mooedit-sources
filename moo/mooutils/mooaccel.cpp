/*
 *   mooaccel.c
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

#include "mooutils/mooaccel.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-tests.h"
#include "mooutils/moocompat.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include <locale.h>

#define MOO_ACCEL_PREFS_KEY "Shortcuts"

#if defined(GDK_WINDOWING_QUARTZ)
#define COMMAND_MASK GDK_META_MASK
#else
#define COMMAND_MASK GDK_CONTROL_MASK
#endif


static void          accel_map_changed      (GtkAccelMap        *map,
                                             gchar              *accel_path,
                                             guint               accel_key,
                                             GdkModifierType     accel_mods);

static const char   *get_accel              (const char         *accel_path);
static void          prefs_new_accel        (const char         *accel_path,
                                             const char         *default_accel);
static const char   *prefs_get_accel        (const char         *accel_path);
static void          prefs_set_accel        (const char         *accel_path,
                                             const char         *accel);

static void          my_gtk_accelerator_parse (const char       *accel,
                                             guint              *key,
                                             GdkModifierType    *mods);

static void          moo_modify_accel_real  (const char         *accel_path,
                                             const char         *new_accel,
                                             gboolean            set_gtk);

static char         *_moo_accel_normalize   (const char         *accel);


static GHashTable *moo_accel_map = NULL;            /* char* -> char* */
static GHashTable *moo_default_accel_map = NULL;    /* char* -> char* */


static void
init_accel_map (void)
{
    if (!moo_accel_map)
    {
        moo_accel_map =
                g_hash_table_new_full (g_str_hash, g_str_equal,
                                       (GDestroyNotify) g_free,
                                       (GDestroyNotify) g_free);
        moo_default_accel_map =
                g_hash_table_new_full (g_str_hash, g_str_equal,
                                       (GDestroyNotify) g_free,
                                       (GDestroyNotify) g_free);

        g_signal_connect (gtk_accel_map_get (), "changed",
                          G_CALLBACK (accel_map_changed),
                          NULL);
    }
}


static void
accel_map_changed (G_GNUC_UNUSED GtkAccelMap *map,
                   gchar            *accel_path,
                   guint             accel_key,
                   GdkModifierType   accel_mods)
{
    char *new_accel;

    if (accel_key)
        new_accel = gtk_accelerator_name (accel_key, accel_mods);
    else
        new_accel = NULL;

    moo_modify_accel_real (accel_path, new_accel, FALSE);

    g_free (new_accel);
}


static char *
accel_path_to_prefs_key (const char *accel_path,
                         gboolean    global)
{
    if (accel_path && accel_path[0] == '<')
    {
        accel_path = strchr (accel_path, '/');
        if (accel_path)
            accel_path++;
    }

    if (!accel_path || !accel_path[0])
        return NULL;

    if (global)
        return g_strdup_printf (MOO_ACCEL_PREFS_KEY "/%s:global", accel_path);
    else
        return g_strdup_printf (MOO_ACCEL_PREFS_KEY "/%s", accel_path);
}


static void
prefs_set_accel (const char *accel_path,
                 const char *accel)
{
    char *key;

    g_return_if_fail (accel != NULL);

    key = accel_path_to_prefs_key (accel_path, FALSE);
    g_return_if_fail (key != NULL);

    g_return_if_fail (moo_prefs_key_registered (key));
    moo_prefs_set_string (key, accel);

    g_free (key);
}


gboolean
_moo_accel_prefs_get_global (const char *accel_path)
{
    char *key;
    gboolean retval;

    g_return_val_if_fail (accel_path != NULL, FALSE);

    key = accel_path_to_prefs_key (accel_path, TRUE);
    g_return_val_if_fail (key != NULL, FALSE);

    /* XXX fix prefs for this */
    moo_prefs_new_key_bool (key, FALSE);
    retval = moo_prefs_get_bool (key);

    g_free (key);
    return retval;
}

void
_moo_accel_prefs_set_global (const char *accel_path,
                             gboolean    global)
{
    char *key;

    g_return_if_fail (accel_path != NULL);

    key = accel_path_to_prefs_key (accel_path, TRUE);
    g_return_if_fail (key != NULL);

    if (moo_prefs_key_registered (key))
    {
        moo_prefs_set_bool (key, global);
    }
    else if (global)
    {
        moo_prefs_new_key_bool (key, FALSE);
        moo_prefs_set_bool (key, global);
    }

    g_free (key);
}


static void
prefs_new_accel (const char *accel_path,
                 const char *default_accel)
{
    char *key;

    g_return_if_fail (accel_path != NULL && default_accel != NULL);

    key = accel_path_to_prefs_key (accel_path, FALSE);
    g_return_if_fail (key != NULL);

    if (!moo_prefs_key_registered (key))
        moo_prefs_new_key_string(key, default_accel);

    g_free (key);
}


static const char *
prefs_get_accel (const char *accel_path)
{
    const char *accel;
    char *key;

    key = accel_path_to_prefs_key (accel_path, FALSE);
    g_return_val_if_fail (key != NULL, NULL);

    accel = moo_prefs_get_string (key);

    g_free (key);
    return accel;
}


static void
set_accel (const char *accel_path,
           const char *accel,
           gboolean    set_gtk)
{
    guint accel_key = 0;
    GdkModifierType accel_mods = (GdkModifierType) 0;
    const char *old_accel;

    g_return_if_fail (accel_path != NULL && accel != NULL);

    init_accel_map ();

    old_accel = get_accel (accel_path);

    if (old_accel && !strcmp (old_accel, accel))
        return;

    if (*accel)
    {
        my_gtk_accelerator_parse (accel, &accel_key, &accel_mods);

        if (accel_key)
        {
            g_hash_table_insert (moo_accel_map,
                                 g_strdup (accel_path),
                                 gtk_accelerator_name (accel_key, accel_mods));
        }
        else
        {
            g_warning ("could not parse accelerator '%s'", accel);
            g_hash_table_insert (moo_accel_map,
                                 g_strdup (accel_path),
                                 g_strdup (""));
        }
    }
    else
    {
        g_hash_table_insert (moo_accel_map,
                             g_strdup (accel_path),
                             g_strdup (""));
    }

    if (set_gtk)
    {
        g_signal_handlers_block_by_func (gtk_accel_map_get (),
                                         (gpointer) accel_map_changed,
                                         NULL);

        if (!gtk_accel_map_change_entry (accel_path, accel_key, accel_mods, TRUE))
            g_warning ("could not set accel '%s' for accel_path '%s'",
                       accel, accel_path);

        g_signal_handlers_unblock_by_func (gtk_accel_map_get (),
                                           (gpointer) accel_map_changed,
                                           NULL);
    }
}


static const char *
get_accel (const char *accel_path)
{
    g_return_val_if_fail (accel_path != NULL, NULL);
    init_accel_map ();
    return (const char*) g_hash_table_lookup (moo_accel_map, accel_path);
}


static const char *
get_default_accel (const char *accel_path)
{
    g_return_val_if_fail (accel_path != NULL, NULL);
    init_accel_map ();
    return (const char*) g_hash_table_lookup (moo_default_accel_map, accel_path);
}


const char *
_moo_get_accel (const char *accel_path)
{
    const char *accel = get_accel (accel_path);
    g_return_val_if_fail (accel != NULL, "");
    return accel;
}


const char *
_moo_get_default_accel (const char *accel_path)
{
    const char *accel = get_default_accel (accel_path);
    g_return_val_if_fail (accel != NULL, "");
    return accel;
}


void
_moo_accel_register (const char *accel_path,
                     const char *default_accel)
{
    char *freeme = NULL;

    g_return_if_fail (accel_path && accel_path[0]);
    g_return_if_fail (default_accel != NULL);

    init_accel_map ();

    if (get_default_accel (accel_path))
        return;

    if (default_accel[0])
    {
        if (!(freeme = _moo_accel_normalize (default_accel)))
            return;
        default_accel = freeme;
    }

    if (default_accel[0])
    {
        guint accel_key = 0;
        GdkModifierType accel_mods = (GdkModifierType) 0;

        my_gtk_accelerator_parse (default_accel, &accel_key, &accel_mods);

        if (accel_key)
            g_hash_table_insert (moo_default_accel_map,
                                 g_strdup (accel_path),
                                 gtk_accelerator_name (accel_key, accel_mods));
        else
            g_warning ("could not parse accelerator '%s'", default_accel);
    }
    else
    {
        g_hash_table_insert (moo_default_accel_map,
                             g_strdup (accel_path),
                             g_strdup (""));
    }

    prefs_new_accel (accel_path, default_accel);
    set_accel (accel_path, prefs_get_accel (accel_path), TRUE);

    g_free (freeme);
}


static void
moo_modify_accel_real (const char *accel_path,
                       const char *new_accel,
                       gboolean    set_gtk)
{
    char *freeme = NULL;

    g_return_if_fail (accel_path != NULL);
    g_return_if_fail (get_accel (accel_path) != NULL);

    if (!new_accel)
        new_accel = "";

    if (new_accel[0])
    {
        freeme = _moo_accel_normalize (new_accel);
        new_accel = freeme;
        g_return_if_fail (new_accel != NULL);
    }

    set_accel (accel_path, new_accel, set_gtk);
    prefs_set_accel (accel_path, new_accel);

    g_free (freeme);
}

void
_moo_modify_accel (const char *accel_path,
                   const char *new_accel)
{
    moo_modify_accel_real (accel_path, new_accel, TRUE);
}


char *
_moo_get_accel_label (const char *accel)
{
    guint key = 0;
    GdkModifierType mods = (GdkModifierType) 0;

    g_return_val_if_fail (accel != NULL, g_strdup (""));

    if (*accel)
        my_gtk_accelerator_parse (accel, &key, &mods);

    if (key)
        return gtk_accelerator_get_label (key, mods);
    else
        return g_strdup ("");
}

static gboolean
need_workaround_for_671562 (guint keyval)
{
    switch (keyval)
    {
        case GDK_F1:
        case GDK_F2:
        case GDK_F3:
        case GDK_F4:
        case GDK_F5:
        case GDK_F6:
        case GDK_F7:
        case GDK_F8:
        case GDK_F9:
        case GDK_F10:
        case GDK_F11:
        case GDK_F12:
            return TRUE;

        default:
            return FALSE;
    }
}

/* Wrapper for gdk_keymap_translate_keyboard_state with a workaround
   for https://bugzilla.gnome.org/show_bug.cgi?id=671562 */
gboolean
moo_keymap_translate_keyboard_state (GdkKeymap           *keymap,
                                     guint                hardware_keycode,
                                     GdkModifierType      state,
                                     gint                 group,
                                     guint               *keyval_p,
                                     gint                *effective_group,
                                     gint                *level,
                                     GdkModifierType     *consumed_modifiers_p)
{
    guint keyval = 0;
    GdkModifierType consumed_modifiers = (GdkModifierType) 0;
    gboolean retval = 
        gdk_keymap_translate_keyboard_state (keymap, hardware_keycode, state, group,
                                             &keyval, effective_group, level,
                                             &consumed_modifiers);

    /* Check whether Shift mask needs to be added back */
    if ((state & GDK_SHIFT_MASK) && (consumed_modifiers & GDK_SHIFT_MASK) && 
        need_workaround_for_671562 (keyval))
    {
        consumed_modifiers = (GdkModifierType) (consumed_modifiers & ~GDK_SHIFT_MASK);
    }

    if (keyval_p)
        *keyval_p = keyval;
    if (consumed_modifiers_p)
        *consumed_modifiers_p = consumed_modifiers;
    return retval;
}

void
moo_accel_translate_event (GtkWidget       *widget,
                           GdkEventKey     *event,
                           guint           *keyval,
                           GdkModifierType *mods)
{
    GdkKeymap *keymap;
    GdkModifierType consumed;

    g_return_if_fail (event != NULL);

    if (keyval)
        *keyval = 0;
    if (mods)
        *mods = (GdkModifierType) 0;

    if (widget && GTK_WIDGET_REALIZED (widget))
        keymap = gdk_keymap_get_for_display (gtk_widget_get_display (widget));
    else
        keymap = gdk_keymap_get_default ();

    moo_keymap_translate_keyboard_state (keymap, event->hardware_keycode,
                                         (GdkModifierType) event->state, event->group,
                                         keyval, NULL, NULL, &consumed);
    if (mods)
        *mods = (GdkModifierType) (event->state & ~consumed & MOO_ACCEL_MODS_MASK);
}

gboolean
moo_accel_check_event (GtkWidget       *widget,
                       GdkEventKey     *event,
                       guint            keyval,
                       GdkModifierType  mods)
{
    guint ev_keyval;
    GdkModifierType ev_mods;
    moo_accel_translate_event (widget, event, &ev_keyval, &ev_mods);
    return keyval == ev_keyval && mods == ev_mods;
}


/*****************************************************************************/
/* Parsing accelerator strings
 */

/* TODO: multibyte symbols? */
static guint
keyval_from_symbol (char sym)
{
    switch (sym)
    {
        case '!': return GDK_exclam;
        case '"': return GDK_quotedbl;
        case '#': return GDK_numbersign;
        case '$': return GDK_dollar;
        case '%': return GDK_percent;
        case '&': return GDK_ampersand;
        case '\'': return GDK_apostrophe;
#if 0
        case '\'': return GDK_quoteright;
#define GDK_asciicircum 0x05e
#define GDK_grave 0x060
#endif
        case '(': return GDK_parenleft;
        case ')': return GDK_parenright;
        case '*': return GDK_asterisk;
        case '+': return GDK_plus;
        case ',': return GDK_comma;
        case '-': return GDK_minus;
        case '.': return GDK_period;
        case '/': return GDK_slash;
        case ':': return GDK_colon;
        case ';': return GDK_semicolon;
        case '<': return GDK_less;
        case '=': return GDK_equal;
        case '>': return GDK_greater;
        case '?': return GDK_question;
        case '@': return GDK_at;
        case '[': return GDK_bracketleft;
        case '\\': return GDK_backslash;
        case ']': return GDK_bracketright;
        case '_': return GDK_underscore;
        case '`': return GDK_quoteleft;
        case '{': return GDK_braceleft;
        case '|': return GDK_bar;
        case '}': return GDK_braceright;
        case '~': return GDK_asciitilde;
    }

    return 0;
}


static guint
parse_key (const char *string)
{
    char *stripped;
    guint key = 0;

    stripped = g_strstrip (g_strdup (string));

    if (stripped && stripped[0])
    {
        key = gdk_keyval_from_name (stripped);

        if (key == GDK_VoidSymbol)
            key = 0;

        if (!key && !stripped[1])
            key = keyval_from_symbol (stripped[0]);
    }

    g_free (stripped);
    return key;
}


static GdkModifierType
parse_mod (const char *string, gssize len)
{
    GdkModifierType mod = (GdkModifierType) 0;
    char *stripped;

    stripped = g_strstrip (g_ascii_strdown (string, len));

    if (!strcmp (stripped, "alt"))
        mod = GDK_MOD1_MASK;
    else if (!strcmp (stripped, "ctl") ||
              !strcmp (stripped, "ctrl") ||
              !strcmp (stripped, "control"))
        mod = GDK_CONTROL_MASK;
    else if (!strcmp (stripped, "cmd") ||
             !strcmp (stripped, "command"))
        mod = COMMAND_MASK;
    else if (!strcmp (stripped, "meta"))
        mod = GDK_META_MASK;
    else if (!strncmp (stripped, "mod", 3) &&
             '1' <= stripped[3] && stripped[3] <= '5' && !stripped[4])
    {
        switch (stripped[3])
        {
            case '1': mod = GDK_MOD1_MASK; break;
            case '2': mod = GDK_MOD2_MASK; break;
            case '3': mod = GDK_MOD3_MASK; break;
            case '4': mod = GDK_MOD4_MASK; break;
            case '5': mod = GDK_MOD5_MASK; break;
        }
    }
    else if (!strcmp (stripped, "shift") ||
              !strcmp (stripped, "shft"))
        mod = GDK_SHIFT_MASK;
    else if (!strcmp (stripped, "release"))
        mod = GDK_RELEASE_MASK;
    else if (!strcmp (stripped, "primary"))
        mod = COMMAND_MASK;

    g_free (stripped);
    return mod;
}


static void
my_gtk_accelerator_parse (const char      *accel,
                          guint           *key,
                          GdkModifierType *mods)
{
    *key = 0;
    *mods = (GdkModifierType) 0;

    while (*accel == '<')
    {
        GdkModifierType m;
        const char *end = strchr (accel, '>');

        if (!end || !(m = parse_mod (accel + 1, end - accel - 1)))
            return;

        *mods = (GdkModifierType) (*mods | m);
        accel = end + 1;
    }

    if (accel[0])
        *key = gdk_keyval_from_name (accel);

    if (*key == GDK_VoidSymbol)
        *key = 0;
}


static gboolean
accel_parse_sep (const char     *accel,
                 const char     *sep,
                 guint          *keyval,
                 GdkModifierType *modifiers)
{
    char **pieces;
    guint n_pieces, i;
    int mod = 0;
    guint key = 0;

    pieces = g_strsplit (accel, sep, 0);
    n_pieces = g_strv_length (pieces);
    g_return_val_if_fail (n_pieces > 1, FALSE);

    for (i = 0; i < n_pieces - 1; ++i)
    {
        GdkModifierType m = parse_mod (pieces[i], -1);

        if (!m)
            goto out;

        mod |= m;
    }

    key = parse_key (pieces[n_pieces-1]);

out:
    g_strfreev (pieces);

    if (!key)
        mod = 0;

    if (keyval)
        *keyval = key;
    if (modifiers)
        *modifiers = (GdkModifierType) mod;
    return key != 0;
}


gboolean
_moo_accel_parse (const char      *accel,
                  guint           *keyval,
                  GdkModifierType *modifiers)
{
    guint key = 0;
    gsize len;
    GdkModifierType mods = (GdkModifierType) 0;
    char *p;

    g_return_val_if_fail (accel && accel[0], FALSE);

    len = strlen (accel);

    if (len == 1)
    {
        key = parse_key (accel);
        goto out;
    }

    if (accel[0] == '<')
    {
        my_gtk_accelerator_parse (accel, &key, &mods);
        goto out;
    }

    if ((p = (char*) strchr (accel, '+')) && p != accel + len - 1)
        return accel_parse_sep (accel, "+", keyval, modifiers);
    else if ((p = (char*) strchr (accel, '-')) && p != accel + len - 1)
        return accel_parse_sep (accel, "-", keyval, modifiers);

    key = parse_key (accel);

out:
    if (keyval)
        *keyval = gdk_keyval_to_lower (key);
    if (modifiers)
        *modifiers = mods;
    return key != 0;
}


static char *
_moo_accel_normalize (const char *accel)
{
    guint key;
    GdkModifierType mods;

    if (!accel || !accel[0])
        return NULL;

    if (_moo_accel_parse (accel, &key, &mods))
    {
        return gtk_accelerator_name (key, mods);
    }
    else
    {
        g_warning ("could not parse accelerator '%s'", accel);
        return NULL;
    }
}


#define TEST_PREFS_KEY_PREFIX "FooBar/"

/* "Because Gtk cares about Mac (and screw compatibility)" */
#if GTK_CHECK_VERSION(2,24,7)
#define CONTROL_MOD "<Primary>"
#define SHIFT_CONTROL "<Primary><Shift>"
#else
#define CONTROL_MOD "<Control>"
#define SHIFT_CONTROL "<Shift><Control>"
#endif

static void
test_moo_accel_register (void)
{
    typedef struct {
        const char *path;
        const char *accel;
        const char *second_accel;
        const char *third_accel;
    } PA;

    guint i;

    PA bad_cases[] = {
        { NULL, "", NULL }, { "", "", NULL }, { "", NULL, NULL },
    };

    PA bad_accels[] = {
        { "<Something>/Foobar/aa", NULL, "" },
        { "<Something>/Foobar/bb", "<dd", "" },
    };

    PA cases[] = {
        { "<Something>/Foobar/a", "", "", "a" },
        { "<Something>/Foobar/b", "", CONTROL_MOD "a", "" },
        { "<Something>/Foobar/c", CONTROL_MOD "b", "", "plus" },
        { "<Something>/Foobar/d", CONTROL_MOD "c", "F4", "F4" },
        { "<Something>/Foobar/e", "F4", "a", "F4" },
    };

    for (i = 0; i < G_N_ELEMENTS (bad_cases); ++i)
    {
        TEST_EXPECT_WARNING (1, "_moo_accel_register(%s, %s)",
                             TEST_FMT_STR (bad_cases[i].path),
                             TEST_FMT_STR (bad_cases[i].accel));
        _moo_accel_register (bad_cases[i].path, bad_cases[i].accel);
        TEST_CHECK_WARNING ();
    }

    for (i = 0; i < G_N_ELEMENTS (bad_accels); ++i)
    {
        TEST_EXPECT_WARNING (1, "_moo_accel_register(%s, %s)",
                             TEST_FMT_STR (bad_accels[i].path),
                             TEST_FMT_STR (bad_accels[i].accel));
        _moo_accel_register (bad_accels[i].path, bad_accels[i].accel);
        TEST_CHECK_WARNING ();

        TEST_EXPECT_WARNING (2, "_moo_get_default_accel(%s) after "
                             "invalid _moo_accel_register(%s, %s)",
                             TEST_FMT_STR (bad_accels[i].path),
                             TEST_FMT_STR (bad_accels[i].path),
                             TEST_FMT_STR (bad_accels[i].accel));
        TEST_ASSERT_STR_EQ_MSG (_moo_get_default_accel (bad_accels[i].path), "",
                                "_moo_get_default_accel(%s) after "
                                "invalid _moo_accel_register(%s, %s)",
                                TEST_FMT_STR (bad_accels[i].path),
                                TEST_FMT_STR (bad_accels[i].path),
                                TEST_FMT_STR (bad_accels[i].accel));
        TEST_ASSERT_STR_EQ_MSG (_moo_get_accel (bad_accels[i].path), "",
                                "_moo_get_accel(%s) after "
                                "invalid _moo_accel_register(%s, %s)",
                                TEST_FMT_STR (bad_accels[i].path),
                                TEST_FMT_STR (bad_accels[i].path),
                                TEST_FMT_STR (bad_accels[i].accel));
        TEST_CHECK_WARNING ();
    }

    for (i = 0; i < G_N_ELEMENTS (cases); ++i)
    {
        const char *path = cases[i].path;
        const char *accel = cases[i].accel;
        const char *second_accel = cases[i].second_accel;
        const char *third_accel = cases[i].third_accel;
        guint key = 0;
        GdkModifierType mods = (GdkModifierType) 0;

        TEST_EXPECT_WARNING (0, "_moo_accel_register and friends for path %s", path);

        _moo_accel_register (path, accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_default_accel (path), accel,
                                "_moo_get_default_accel(%s) after _moo_accel_register(%s, %s)",
                                path, path, accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_accel (path), accel,
                                "_moo_get_accel(%s) after _moo_accel_register(%s, %s)",
                                path, path, accel);

        _moo_accel_register (path, second_accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_default_accel (path), accel,
                                "_moo_get_default_accel(%s) after second _moo_accel_register(%s, %s)",
                                path, path, second_accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_accel (path), accel,
                                "_moo_get_accel(%s) after second _moo_accel_register(%s, %s)",
                                path, path, second_accel);

        _moo_modify_accel (path, second_accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_default_accel (path), accel,
                                "_moo_get_default_accel(%s) after _moo_modify_accel(%s, %s)",
                                path, path, second_accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_accel (path), second_accel,
                                "_moo_get_accel(%s) after _moo_modify_accel(%s, %s)",
                                path, path, second_accel);

        if (*third_accel)
            my_gtk_accelerator_parse (third_accel, &key, &mods);
        gtk_accel_map_change_entry (path, key, mods, FALSE);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_default_accel (path), accel,
                                "_moo_get_default_accel(%s) after gtk_accel_map_change_entry(%s, %s)",
                                path, path, third_accel);
        TEST_ASSERT_STR_EQ_MSG (_moo_get_accel (path), third_accel,
                                "_moo_get_accel(%s) after gtk_accel_map_change_entry(%s, %s)",
                                path, path, third_accel);

        TEST_CHECK_WARNING ();
    }
}

static void
test_moo_accel_normalize (void)
{
    guint i;

    struct {
        const char *input;
        const char *result;
    } cases[] = {
        { NULL, NULL }, { "", NULL }, { "some nonsense", NULL }, { "foobar", NULL },
        { "<<a>", NULL }, { "<Control>Moo", NULL }, { "<Control><Shift>", NULL },
        { "a+", NULL }, { "a-", NULL }, { "a+b", NULL }, { "Ctrl+Shift", NULL },

        { "Tab", "Tab" }, { "<shift>Tab", "<Shift>Tab" },
        { "<Control>a", CONTROL_MOD "a" }, { "<Ctl>b", CONTROL_MOD "b" }, { "<Ctrl>c", CONTROL_MOD "c" },
        { "<ctl>d", CONTROL_MOD "d" }, { "<control>e", CONTROL_MOD "e" },
        { "<ctl><shift>f", SHIFT_CONTROL "f" }, { "<shift><ctrl>g", SHIFT_CONTROL "g" },
        { "F8", "F8" }, { "F12", "F12" }, { "z", "z" }, { "X", "x" }, { "<shift>S", "<Shift>s" },

        { "shift+Tab", "<Shift>Tab" },
        { "Control+a", CONTROL_MOD "a" }, { "Ctl+b", CONTROL_MOD "b" }, { "Ctrl+c", CONTROL_MOD "c" },
        { "ctl+d", CONTROL_MOD "d" }, { "control+e", CONTROL_MOD "e" },
        { "ctl+shift+f", SHIFT_CONTROL "f" }, { "shift+ctrl+G", SHIFT_CONTROL "g" },
        { "F8", "F8" }, { "F12", "F12" }, { "z", "z" }, { "X", "x" }, { "shift+S", "<Shift>s" },

        { "shift-Tab", "<Shift>Tab" },
        { "Control-a", CONTROL_MOD "a" }, { "Ctl-b", CONTROL_MOD "b" }, { "Ctrl-c", CONTROL_MOD "c" },

        { "shift-+", "<Shift>plus" }, { "shift+-", "<Shift>minus" },
        { "shift-plus", "<Shift>plus" }, { "shift+plus", "<Shift>plus" },

#ifdef GDK_WINDOWING_QUARTZ
        { "cmd-a", "<Meta>a" }, { "<Command>a", "<Meta>a" },
#else
        { "cmd-a", CONTROL_MOD "a" }, { "<Command>a", CONTROL_MOD "a" },
#endif
    };

    setlocale (LC_ALL, "C");

    for (i = 0; i < G_N_ELEMENTS (cases); ++i)
    {
        char *result;

        TEST_EXPECT_WARNING (!cases[i].result &&
                             (cases[i].input && cases[i].input[0]),
                             "_moo_accel_normalize(%s)",
                             TEST_FMT_STR (cases[i].input));

        result = _moo_accel_normalize (cases[i].input);

        TEST_CHECK_WARNING ();

        TEST_ASSERT_STR_EQ_MSG (result, cases[i].result,
                                "_moo_accel_normalize(%s)",
                                TEST_FMT_STR (cases[i].input));

        g_free (result);
    }

    setlocale (LC_ALL, "");
}

static void
test_moo_get_accel_label (void)
{
    guint i;

    struct {
        const char *input;
        const char *result;
    } cases[] = {
        { NULL, "" }, { "", "" }, { "some nonsense", "" }, { "foobar", "" },
        { "<<a>", "" }, { "<Control>Moo", "" }, { "<Control><Shift>", "" },

        { "Tab", "Tab" },
        { "F8", "F8" }, { "F12", "F12" }, { "z", "Z" }, { "X", "X" },

#ifndef GDK_WINDOWING_QUARTZ
        { "<shift>Tab", "Shift+Tab" }, { "<Control>a", "Ctrl+A" },
        { "<Ctl>b", "Ctrl+B" }, { "<Ctrl>c", "Ctrl+C" },
        { "<ctl>d", "Ctrl+D" }, { "<control>e", "Ctrl+E" },
        { "<ctl><shift>f", "Shift+Ctrl+F" }, { "<shift><ctrl>g", "Shift+Ctrl+G" },
        { "<shift>S", "Shift+S" }
#else
        { "<shift>Tab", "\xe2\x87\xa7""Tab" }, { "<meta>a", "\xe2\x8c\x98""A" },
        { "<meta>b", "\xe2\x8c\x98""B" }, { "<cmd>c", "\xe2\x8c\x98""C" },
        { "<command>d", "\xe2\x8c\x98""D" }, { "<Command>e", "\xe2\x8c\x98""E" },
        { "<Cmd><shift>f", "\xe2\x87\xa7""\xe2\x8c\x98""F" }, { "<shift><Meta>g", "\xe2\x87\xa7""\xe2\x8c\x98""G" },
        { "<shift>S", "\xe2\x87\xa7""S" }
#endif
    };

    setlocale (LC_ALL, "C");

    for (i = 0; i < G_N_ELEMENTS (cases); ++i)
    {
        char *result;

        TEST_EXPECT_WARNING (!cases[i].input, "_moo_get_accel_label(%s)",
                             TEST_FMT_STR (cases[i].input));

        result = _moo_get_accel_label (cases[i].input);

        TEST_CHECK_WARNING ();

        TEST_ASSERT_STR_EQ_MSG (result, cases[i].result,
                                "_moo_get_accel_label(%s)",
                                TEST_FMT_STR (cases[i].input));

        g_free (result);
    }

    setlocale (LC_ALL, "");
}

static void
delete_prefs_keys (void)
{
    GSList *keys = moo_prefs_list_keys (MOO_PREFS_RC);
    while (keys)
    {
        char *key = (char*) keys->data;

        if (g_str_has_prefix (key, "Shortcuts/Foobar/"))
            moo_prefs_delete_key (key);

        g_free (key);
        keys = g_slist_delete_link (keys, keys);
    }
}

static gboolean
test_suite_init (void)
{
    delete_prefs_keys ();
    return TRUE;
}

static void
test_suite_cleanup (void)
{
    delete_prefs_keys ();
}

void
moo_test_mooaccel (void)
{
    MooTestSuite& suite = moo_test_suite_new ("mooaccel", "mooutils/mooaccel.c",
                                              (MooTestSuiteInit) test_suite_init,
                                              (MooTestSuiteCleanup) test_suite_cleanup,
                                              NULL);
    moo_test_suite_add_test (suite, "_moo_get_accel_label", "test of _moo_get_accel_label()",
                             (MooTestFunc) test_moo_get_accel_label, NULL);
    moo_test_suite_add_test (suite, "_moo_accel_normalize", "test of _moo_accel_normalize()",
                             (MooTestFunc) test_moo_accel_normalize, NULL);
    moo_test_suite_add_test (suite, "_moo_accel_register", "test of _moo_accel_register() and friends",
                             (MooTestFunc) test_moo_accel_register, NULL);
}
