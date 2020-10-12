/*
 *   moocombo.h
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

#ifndef MOO_COMBO_H
#define MOO_COMBO_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_COMBO              (moo_combo_get_type ())
#define MOO_COMBO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_COMBO, MooCombo))
#define MOO_COMBO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_COMBO, MooComboClass))
#define MOO_IS_COMBO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_COMBO))
#define MOO_IS_COMBO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_COMBO))
#define MOO_COMBO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_COMBO, MooComboClass))

typedef struct _MooCombo         MooCombo;
typedef struct _MooComboPrivate  MooComboPrivate;
typedef struct _MooComboClass    MooComboClass;

struct _MooCombo
{
    GtkTable parent;
    MooComboPrivate *priv;
    GtkWidget *entry;
};

struct _MooComboClass
{
    GtkTableClass parent_class;

    void    (*changed)  (MooCombo   *combo);

    void    (*popup)    (MooCombo   *combo);
    void    (*popdown)  (MooCombo   *combo);

    gboolean (*popup_key_press) (MooCombo    *combo,
                                 GdkEventKey *event);
};

typedef char    *(*MooComboGetTextFunc)     (GtkTreeModel  *model,
                                             GtkTreeIter   *iter,
                                             gpointer       data);
typedef gboolean (*MooComboRowSeparatorFunc)(GtkTreeModel  *model,
                                             GtkTreeIter   *iter,
                                             gpointer       data);



GType           moo_combo_get_type          (void) G_GNUC_CONST;

GtkWidget      *moo_combo_new               (void);

GtkWidget      *moo_combo_new_text          (void);
void            moo_combo_add_text          (MooCombo       *combo,
                                             const char     *text);

void            moo_combo_popup             (MooCombo       *combo);
void            moo_combo_popdown           (MooCombo       *combo);
gboolean        moo_combo_popup_shown       (MooCombo       *combo);
void            moo_combo_update_popup      (MooCombo       *combo);

void            moo_combo_set_active_iter   (MooCombo       *combo,
                                             GtkTreeIter    *iter);
gboolean        moo_combo_get_active_iter   (MooCombo       *combo,
                                             GtkTreeIter    *iter);
void            moo_combo_set_active        (MooCombo       *combo,
                                             int             row);

GtkTreeModel   *moo_combo_get_model         (MooCombo       *combo);
void            moo_combo_set_model         (MooCombo       *combo,
                                             GtkTreeModel   *model);

void            moo_combo_set_row_separator_func (MooCombo  *combo,
                                             MooComboRowSeparatorFunc func,
                                             gpointer        data);

void            moo_combo_set_text_column   (MooCombo       *combo,
                                             int             column);
int             moo_combo_get_text_column   (MooCombo       *combo);

void            moo_combo_set_get_text_func (MooCombo       *combo,
                                             MooComboGetTextFunc func,
                                             gpointer        data);
char           *moo_combo_get_text_at_iter  (MooCombo       *combo,
                                             GtkTreeIter    *iter);

void            moo_combo_set_use_button    (MooCombo       *combo,
                                             gboolean        use);


/***************************************************************************/
/* GtkEntry and GtkEditable interface
 */

void        moo_combo_entry_set_text                (MooCombo       *combo,
                                                     const char     *text);
const char *moo_combo_entry_get_text                (MooCombo       *combo);

void        moo_combo_select_region                 (MooCombo       *combo,
                                                     int             start,
                                                     int             end);
void        moo_combo_entry_set_activates_default   (MooCombo       *combo,
                                                     gboolean        setting);


G_END_DECLS

#endif /* MOO_COMBO_H */
