/*
 *   mootextprint-private.h
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

#ifndef MOO_TEXT_PRINT_PRIVATE_H
#define MOO_TEXT_PRINT_PRIVATE_H

#include <mooedit/mootextprint.h>

G_BEGIN_DECLS


#define MOO_TYPE_PRINT_SETTINGS (_moo_print_settings_get_type ())


typedef enum {
    MOO_PRINT_WRAP          = 1 << 0,
    MOO_PRINT_ELLIPSIZE     = 1 << 1,
    MOO_PRINT_USE_STYLES    = 1 << 2,
    MOO_PRINT_HEADER        = 1 << 3,
    MOO_PRINT_FOOTER        = 1 << 4,
    MOO_PRINT_LINE_NUMBERS  = 1 << 5
} MooPrintFlags;

typedef enum {
    MOO_PRINT_POS_LEFT,
    MOO_PRINT_POS_CENTER,
    MOO_PRINT_POS_RIGHT
} MooPrintPos;

typedef struct {
    gboolean do_print;
    PangoFontDescription *font;
    char *format[3];
    gboolean separator;

    PangoLayout *layout;
    double text_height;
    double separator_before;
    double separator_after;
    double separator_height;
    gpointer parsed_format[3];
} MooPrintHeaderFooter;

typedef struct {
    char *font; /* overrides font set in the doc */
    char *ln_font;
    guint ln_step;
    MooPrintFlags flags;
    PangoWrapMode wrap_mode;
    MooPrintHeaderFooter *header;
    MooPrintHeaderFooter *footer;
} MooPrintSettings;


GType   _moo_print_settings_get_type            (void) G_GNUC_CONST;

GtkWindow *_moo_print_operation_get_parent      (MooPrintOperation  *print);
int     _moo_print_operation_get_n_pages        (MooPrintOperation  *print);


G_END_DECLS

#endif /* MOO_TEXT_PRINT_PRIVATE_H */
