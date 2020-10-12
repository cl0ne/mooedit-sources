/*
 * gtkfontsel.* copied and modified to allow choosing monospace fonts
 */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkFontSelection widget for Gtk+, by Damon Chaplin, May 1998.
 * Based on the GnomeFontSelector widget, by Elliot Lee, but major changes.
 * The GnomeFontSelector was derived from app/text_tool.c in the GIMP.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef MOO_FONTSEL_H
#define MOO_FONTSEL_H


#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOO_TYPE_FONT_SELECTION              (moo_font_selection_get_type ())
#define MOO_FONT_SELECTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_FONT_SELECTION, MooFontSelection))
#define MOO_FONT_SELECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FONT_SELECTION, MooFontSelectionClass))
#define MOO_IS_FONT_SELECTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_FONT_SELECTION))
#define MOO_IS_FONT_SELECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FONT_SELECTION))
#define MOO_FONT_SELECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FONT_SELECTION, MooFontSelectionClass))


#define MOO_TYPE_FONT_SELECTION_DIALOG              (moo_font_selection_dialog_get_type ())
#define MOO_FONT_SELECTION_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_FONT_SELECTION_DIALOG, MooFontSelectionDialog))
#define MOO_FONT_SELECTION_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FONT_SELECTION_DIALOG, MooFontSelectionDialogClass))
#define MOO_IS_FONT_SELECTION_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_FONT_SELECTION_DIALOG))
#define MOO_IS_FONT_SELECTION_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FONT_SELECTION_DIALOG))
#define MOO_FONT_SELECTION_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FONT_SELECTION_DIALOG, MooFontSelectionDialogClass))


typedef struct _MooFontSelection	     MooFontSelection;
typedef struct _MooFontSelectionClass	     MooFontSelectionClass;

typedef struct _MooFontSelectionDialog	     MooFontSelectionDialog;
typedef struct _MooFontSelectionDialogClass  MooFontSelectionDialogClass;

struct _MooFontSelection
{
  GtkVBox parent_instance;

  GtkWidget *font_entry;
  GtkWidget *family_list;
  GtkWidget *font_style_entry;
  GtkWidget *face_list;
  GtkWidget *size_entry;
  GtkWidget *size_list;
  GtkWidget *pixels_button;
  GtkWidget *points_button;
  GtkWidget *filter_button;
  GtkWidget *preview_entry;

  PangoFontFamily *family;	/* Current family */
  PangoFontFace *face;		/* Current face */

  gint size;

  guint monospace : 1;
  guint filter_visible : 1;
};

struct _MooFontSelectionClass
{
  GtkVBoxClass parent_class;

  /* Padding for future expansion */
  void (*_moo_reserved1) (void);
  void (*_moo_reserved2) (void);
  void (*_moo_reserved3) (void);
  void (*_moo_reserved4) (void);
};


struct _MooFontSelectionDialog
{
  GtkDialog parent_instance;

  /*< private >*/
  GtkWidget *fontsel;

  GtkWidget *main_vbox;
  GtkWidget *action_area;
  /*< public >*/
  GtkWidget *ok_button;
  GtkWidget *apply_button;
  GtkWidget *cancel_button;

  /*< private >*/

  /* If the user changes the width of the dialog, we turn auto-shrink off.
   * (Unused now, autoshrink doesn't mean anything anymore -Yosh)
   */
  gint dialog_width;
  gboolean auto_resize;
};

struct _MooFontSelectionDialogClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_moo_reserved1) (void);
  void (*_moo_reserved2) (void);
  void (*_moo_reserved3) (void);
  void (*_moo_reserved4) (void);
};



/*****************************************************************************
 * MooFontSelection functions.
 *   see the comments in the MooFontSelectionDialog functions.
 *****************************************************************************/

GType	   moo_font_selection_get_type		(void) G_GNUC_CONST;
GtkWidget* moo_font_selection_new		(void);
gchar*	   moo_font_selection_get_font_name	(MooFontSelection *fontsel);

gboolean              moo_font_selection_set_font_name    (MooFontSelection *fontsel,
							   const gchar      *fontname);
const gchar* moo_font_selection_get_preview_text (MooFontSelection *fontsel);
void                  moo_font_selection_set_preview_text (MooFontSelection *fontsel,
							   const gchar      *text);

void       moo_font_selection_set_monospace         (MooFontSelection *fontsel,
                                                     gboolean          monospace);
void       moo_font_selection_set_filter_visible    (MooFontSelection *fontsel,
                                                     gboolean          visible);

/*****************************************************************************
 * MooFontSelectionDialog functions.
 *   most of these functions simply call the corresponding function in the
 *   MooFontSelection.
 *****************************************************************************/

GType	   moo_font_selection_dialog_get_type	(void) G_GNUC_CONST;
GtkWidget* moo_font_selection_dialog_new	(const gchar	  *title);

/* This returns the X Logical Font Description fontname, or NULL if no font
   is selected. Note that there is a slight possibility that the font might not
   have been loaded OK. You should call moo_font_selection_dialog_get_font()
   to see if it has been loaded OK.
   You should g_free() the returned font name after you're done with it. */
gchar*	 moo_font_selection_dialog_get_font_name    (MooFontSelectionDialog *fsd);

/* This sets the currently displayed font. It should be a valid X Logical
   Font Description font name (anything else will be ignored), e.g.
   "-adobe-courier-bold-o-normal--25-*-*-*-*-*-*-*"
   It returns TRUE on success. */
gboolean moo_font_selection_dialog_set_font_name    (MooFontSelectionDialog *fsd,
						     const gchar	*fontname);

/* This returns the text in the preview entry. You should copy the returned
   text if you need it. */
const gchar* moo_font_selection_dialog_get_preview_text (MooFontSelectionDialog *fsd);

/* This sets the text in the preview entry. It will be copied by the entry,
   so there's no need to g_strdup() it first. */
void	 moo_font_selection_dialog_set_preview_text (MooFontSelectionDialog *fsd,
						     const gchar	    *text);



/* MooFontButton is a button widget that allow user to select a font.
 */

#define MOO_TYPE_FONT_BUTTON             (moo_font_button_get_type ())
#define MOO_FONT_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_FONT_BUTTON, MooFontButton))
#define MOO_FONT_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FONT_BUTTON, MooFontButtonClass))
#define MOO_IS_FONT_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_FONT_BUTTON))
#define MOO_IS_FONT_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FONT_BUTTON))
#define MOO_FONT_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FONT_BUTTON, MooFontButtonClass))

typedef struct _MooFontButton        MooFontButton;
typedef struct _MooFontButtonClass   MooFontButtonClass;
typedef struct _MooFontButtonPrivate MooFontButtonPrivate;

struct _MooFontButton {
  GtkButton button;

  /*< private >*/
  MooFontButtonPrivate *priv;
};

struct _MooFontButtonClass {
  GtkButtonClass parent_class;

  /* font_set signal is emitted when font is chosen */
  void (* font_set) (MooFontButton *gfp);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType                 moo_font_button_get_type       (void) G_GNUC_CONST;
GtkWidget            *moo_font_button_new            (void);
GtkWidget            *moo_font_button_new_with_font  (const gchar   *fontname);

const gchar *moo_font_button_get_title      (MooFontButton *font_button);
void                  moo_font_button_set_title      (MooFontButton *font_button,
                                                      const gchar   *title);
gboolean              moo_font_button_get_use_font   (MooFontButton *font_button);
void                  moo_font_button_set_use_font   (MooFontButton *font_button,
                                                      gboolean       use_font);
gboolean              moo_font_button_get_use_size   (MooFontButton *font_button);
void                  moo_font_button_set_use_size   (MooFontButton *font_button,
                                                      gboolean       use_size);
const gchar* moo_font_button_get_font_name  (MooFontButton *font_button);
gboolean              moo_font_button_set_font_name  (MooFontButton *font_button,
                                                      const gchar   *fontname);
gboolean              moo_font_button_get_show_style (MooFontButton *font_button);
void                  moo_font_button_set_show_style (MooFontButton *font_button,
                                                      gboolean       show_style);
gboolean              moo_font_button_get_show_size  (MooFontButton *font_button);
void                  moo_font_button_set_show_size  (MooFontButton *font_button,
                                                      gboolean       show_size);
gboolean              moo_font_button_get_monospace  (MooFontButton *font_button);
void                  moo_font_button_set_monospace  (MooFontButton *font_button,
                                                      gboolean       monospace);
gboolean              moo_font_button_get_filter_visible (MooFontButton *font_button);
void                  moo_font_button_set_filter_visible (MooFontButton *font_button,
                                                          gboolean       visible);


G_END_DECLS


#endif /* MOO_FONTSEL_H */
