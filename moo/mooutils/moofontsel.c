/*
 * gtkfontsel.* copied and modified to allow choosing monospace fonts
 */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Massively updated for Pango by Owen Taylor, May 2000
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <glib/gprintf.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "mooutils/moofontsel.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moocompat.h"

#define P_(String) dgettext ("gtk20-properties", String)


/* This is the default text shown in the preview entry, though the user
   can set it. Remember that some fonts only have capital letters. */
#define PREVIEW_TEXT "abcdefghijk ABCDEFGHIJK"

/* This is the initial and maximum height of the preview entry (it expands
   when large font sizes are selected). Initial height is also the minimum. */
#define INITIAL_PREVIEW_HEIGHT 44
#define MAX_PREVIEW_HEIGHT 300

/* These are the sizes of the font, style & size lists. */
#define FONT_LIST_HEIGHT	136
#define FONT_LIST_WIDTH		190
#define FONT_STYLE_LIST_WIDTH	170
#define FONT_SIZE_LIST_WIDTH	60

/* These are what we use as the standard font sizes, for the size list.
 */
static const guint16 font_sizes[] = {
  8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 26, 28,
  32, 36, 40, 48, 56, 64, 72
};

enum {
  PROP_0,
  PROP_FONT_NAME,
  PROP_PREVIEW_TEXT,
  PROP_MONOSPACE,
  PROP_FILTER_VISIBLE
};


enum {
  FAMILY_COLUMN,
  FAMILY_NAME_COLUMN
};

enum {
  FACE_COLUMN,
  FACE_NAME_COLUMN
};

enum {
  SIZE_COLUMN
};

static void    moo_font_selection_class_init	     (MooFontSelectionClass *klass);
static void    moo_font_selection_set_property       (GObject         *object,
						      guint            prop_id,
						      const GValue    *value,
						      GParamSpec      *pspec);
static void    moo_font_selection_get_property       (GObject         *object,
						      guint            prop_id,
						      GValue          *value,
						      GParamSpec      *pspec);
static void    moo_font_selection_init		     (MooFontSelection      *fontsel);
static void    moo_font_selection_screen_changed     (GtkWidget		    *widget,
						      GdkScreen             *previous_screen);

/* These are the callbacks & related functions. */
static void     moo_font_selection_select_font           (GtkTreeSelection *selection,
							  gpointer          data);
static void     moo_font_selection_show_available_fonts  (MooFontSelection *fs);

static void     moo_font_selection_show_available_styles (MooFontSelection *fs);
static void     moo_font_selection_select_best_style     (MooFontSelection *fs);
static void     moo_font_selection_select_style          (GtkTreeSelection *selection,
							  gpointer          data);

static void     moo_font_selection_select_best_size      (MooFontSelection *fs);
static void     moo_font_selection_show_available_sizes  (MooFontSelection *fs,
							  gboolean          first_time);
static void     moo_font_selection_size_activate         (GtkWidget        *w,
							  gpointer          data);
static gboolean moo_font_selection_size_focus_out        (MooFontSelection *fs);
static void     moo_font_selection_select_size           (GtkTreeSelection *selection,
							  gpointer          data);

static void     moo_font_selection_scroll_on_map         (MooFontSelection *fs);

static void     moo_font_selection_preview_changed       (MooFontSelection *fontsel);
static void     moo_font_selection_filter_changed        (MooFontSelection *fontsel);

/* Misc. utility functions. */
static void    moo_font_selection_load_font          (MooFontSelection *fs);
static void    moo_font_selection_update_preview     (MooFontSelection *fs);

/* FontSelectionDialog */
static void    moo_font_selection_dialog_class_init  (MooFontSelectionDialogClass *klass);
static void    moo_font_selection_dialog_init	     (MooFontSelectionDialog *fontseldiag);

G_DEFINE_TYPE(MooFontSelection, moo_font_selection, GTK_TYPE_VBOX)


static void
moo_font_selection_class_init (MooFontSelectionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->set_property = moo_font_selection_set_property;
  gobject_class->get_property = moo_font_selection_get_property;

  widget_class->screen_changed = moo_font_selection_screen_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        P_("Font name"),
                                                        P_("The X string that represents this font"),
                                                        NULL,
                                                        (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_PREVIEW_TEXT,
                                   g_param_spec_string ("preview-text",
                                                        P_("Preview text"),
                                                        P_("The text to display in order to demonstrate the selected font"),
                                                        PREVIEW_TEXT,
                                                        (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_MONOSPACE,
                                   g_param_spec_boolean ("monospace",
                                                         P_("monospace"),
                                                         P_("monospace"),
                                                         FALSE,
                                                         (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_FILTER_VISIBLE,
                                   g_param_spec_boolean ("filter-visible",
                                                         P_("filter-visible"),
                                                         P_("filter-visible"),
                                                         TRUE,
                                                         (GParamFlags) G_PARAM_READWRITE));
}

static void
moo_font_selection_set_property (GObject         *object,
				 guint            prop_id,
				 const GValue    *value,
				 GParamSpec      *pspec)
{
  MooFontSelection *fontsel;

  fontsel = MOO_FONT_SELECTION (object);

  switch (prop_id)
    {
    case PROP_FONT_NAME:
      moo_font_selection_set_font_name (fontsel, g_value_get_string (value));
      break;
    case PROP_PREVIEW_TEXT:
      moo_font_selection_set_preview_text (fontsel, g_value_get_string (value));
      break;
    case PROP_MONOSPACE:
      moo_font_selection_set_monospace (fontsel, g_value_get_boolean (value));
      break;
    case PROP_FILTER_VISIBLE:
      moo_font_selection_set_filter_visible (fontsel, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void moo_font_selection_get_property (GObject         *object,
					     guint            prop_id,
					     GValue          *value,
					     GParamSpec      *pspec)
{
  MooFontSelection *fontsel;

  fontsel = MOO_FONT_SELECTION (object);

  switch (prop_id)
    {
    case PROP_FONT_NAME:
      g_value_set_string (value, moo_font_selection_get_font_name (fontsel));
      break;
    case PROP_PREVIEW_TEXT:
      g_value_set_string (value, moo_font_selection_get_preview_text (fontsel));
      break;
    case PROP_MONOSPACE:
      g_value_set_boolean (value, fontsel->monospace != 0);
      break;
    case PROP_FILTER_VISIBLE:
      g_value_set_boolean (value, fontsel->filter_visible != 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Handles key press events on the lists, so that we can trap Enter to
 * activate the default button on our own.
 */
static gboolean
list_row_activated (GtkWidget *widget)
{
  GtkWindow *window;

  window = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (widget)));
  if (!GTK_WIDGET_TOPLEVEL (window))
    window = NULL;

  if (window
      && widget != gtk_window_get_default_widget(window)
      && !(widget == gtk_window_get_focus(window) &&
      (!gtk_window_get_default_widget(window) || !GTK_WIDGET_SENSITIVE(gtk_window_get_default_widget(window)))))
    {
      gtk_window_activate_default (window);
    }

  return TRUE;
}

static void
moo_font_selection_init (MooFontSelection *fontsel)
{
  GtkWidget *scrolled_win;
  GtkWidget *text_box;
  GtkWidget *table, *label;
  GtkWidget *font_label, *style_label;
  GtkWidget *vbox;
  GtkListStore *model;
  GtkTreeViewColumn *column;
  GList *focus_chain = NULL;
  AtkObject *atk_obj;

  fontsel->monospace = FALSE;
  fontsel->filter_visible = TRUE;

  gtk_widget_push_composite_child ();

  gtk_box_set_spacing (GTK_BOX (fontsel), 12);
  fontsel->size = 12 * PANGO_SCALE;

  /* Create the table of font, style & size. */
  table = gtk_table_new (3, 3, FALSE);
  gtk_widget_show (table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_box_pack_start (GTK_BOX (fontsel), table, TRUE, TRUE, 0);

  fontsel->size_entry = gtk_entry_new ();
  gtk_widget_set_size_request (fontsel->size_entry, 20, -1);
  gtk_widget_show (fontsel->size_entry);
  gtk_table_attach (GTK_TABLE (table), fontsel->size_entry, 2, 3, 1, 2,
		    GTK_FILL, 0, 0, 0);
  g_signal_connect (fontsel->size_entry, "activate",
		    G_CALLBACK (moo_font_selection_size_activate),
		    fontsel);
  g_signal_connect_data (fontsel->size_entry, "focus_out_event",
                         G_CALLBACK (moo_font_selection_size_focus_out),
                         fontsel, NULL,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  font_label = gtk_label_new_with_mnemonic (D_("_Family:", "gtk20"));
  gtk_misc_set_alignment (GTK_MISC (font_label), 0.0, 0.5);
  gtk_widget_show (font_label);
  gtk_table_attach (GTK_TABLE (table), font_label, 0, 1, 0, 1,
		    GTK_FILL, 0, 0, 0);

  style_label = gtk_label_new_with_mnemonic (D_("_Style:", "gtk20"));
  gtk_misc_set_alignment (GTK_MISC (style_label), 0.0, 0.5);
  gtk_widget_show (style_label);
  gtk_table_attach (GTK_TABLE (table), style_label, 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);

  label = gtk_label_new_with_mnemonic (D_("Si_ze:", "gtk20"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 fontsel->size_entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
		    GTK_FILL, 0, 0, 0);


  /* Create the lists  */

  model = gtk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FAMILY_COLUMN */
			      G_TYPE_STRING); /* FAMILY_NAME_COLUMN */
  fontsel->family_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  g_object_unref (model);

  g_signal_connect (fontsel->family_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  column = gtk_tree_view_column_new_with_attributes ("Family",
						     gtk_cell_renderer_text_new (),
						     "text", FAMILY_NAME_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fontsel->family_list), column);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (fontsel->family_list), FALSE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->family_list)),
			       GTK_SELECTION_BROWSE);

  gtk_label_set_mnemonic_widget (GTK_LABEL (font_label), fontsel->family_list);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled_win,
			       FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_win), fontsel->family_list);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (fontsel->family_list);
  gtk_widget_show (scrolled_win);

  gtk_table_attach (GTK_TABLE (table), scrolled_win, 0, 1, 1, 3,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);

  model = gtk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FACE_COLUMN */
			      G_TYPE_STRING); /* FACE_NAME_COLUMN */
  fontsel->face_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  g_object_unref (model);
  g_signal_connect (fontsel->face_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  gtk_label_set_mnemonic_widget (GTK_LABEL (style_label), fontsel->face_list);

  column = gtk_tree_view_column_new_with_attributes ("Face",
						     gtk_cell_renderer_text_new (),
						     "text", FACE_NAME_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fontsel->face_list), column);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (fontsel->face_list), FALSE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->face_list)),
			       GTK_SELECTION_BROWSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled_win,
			       FONT_STYLE_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_win), fontsel->face_list);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (fontsel->face_list);
  gtk_widget_show (scrolled_win);
  gtk_table_attach (GTK_TABLE (table), scrolled_win, 1, 2, 1, 3,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);

  focus_chain = g_list_append (focus_chain, fontsel->size_entry);

  model = gtk_list_store_new (1, G_TYPE_INT);
  fontsel->size_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  g_object_unref (model);
  g_signal_connect (fontsel->size_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  column = gtk_tree_view_column_new_with_attributes ("Size",
						     gtk_cell_renderer_text_new (),
						     "text", SIZE_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (fontsel->size_list), column);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (fontsel->size_list), FALSE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->size_list)),
			       GTK_SELECTION_BROWSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (scrolled_win), fontsel->size_list);
  gtk_widget_set_size_request (scrolled_win, -1, FONT_LIST_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_widget_show (fontsel->size_list);
  gtk_widget_show (scrolled_win);
  gtk_table_attach (GTK_TABLE (table), scrolled_win, 2, 3, 2, 3,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);

  gtk_container_set_focus_chain (GTK_CONTAINER (table), focus_chain);
  g_list_free (focus_chain);

  /* Insert the fonts. */
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->family_list)), "changed",
		    G_CALLBACK (moo_font_selection_select_font), fontsel);

  g_signal_connect_data (fontsel->family_list, "map",
                         G_CALLBACK (moo_font_selection_scroll_on_map),
                         fontsel, NULL,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->face_list)), "changed",
		    G_CALLBACK (moo_font_selection_select_style), fontsel);

  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->size_list)), "changed",
		    G_CALLBACK (moo_font_selection_select_size), fontsel);
  atk_obj = gtk_widget_get_accessible (fontsel->size_list);
  if (GTK_IS_ACCESSIBLE (atk_obj))
    {
      /* Accessibility support is enabled.
       * Make the label ATK_RELATON_LABEL_FOR for the size list as well.
       */
      AtkObject *atk_label;
      AtkRelationSet *relation_set;
      AtkRelation *relation;
      AtkObject *obj_array[1];
      GPtrArray *array;

      atk_label = gtk_widget_get_accessible (label);
      relation_set = atk_object_ref_relation_set (atk_obj);
      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_LABELLED_BY);
      if (relation)
        {
          array = atk_relation_get_target (relation);
          g_ptr_array_add (array, atk_label);
        }
      else
        {
          obj_array[0] = atk_label;
          relation = atk_relation_new (obj_array, 1, ATK_RELATION_LABELLED_BY);
          atk_relation_set_add (relation_set, relation);
        }
      g_object_unref (relation_set);

      relation_set = atk_object_ref_relation_set (atk_label);
      relation = atk_relation_set_get_relation_by_type (relation_set, ATK_RELATION_LABEL_FOR);
      if (relation)
        {
          array = atk_relation_get_target (relation);
          g_ptr_array_add (array, atk_obj);
        }
      else
        {
          obj_array[0] = atk_obj;
          relation = atk_relation_new (obj_array, 1, ATK_RELATION_LABEL_FOR);
          atk_relation_set_add (relation_set, relation);
        }
      g_object_unref (relation_set);
    }


  vbox = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (fontsel), vbox, FALSE, TRUE, 0);

  fontsel->filter_button = gtk_check_button_new_with_label (_("Show only fixed width fonts"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fontsel->filter_button), fontsel->monospace != 0);
  g_signal_connect_swapped (fontsel->filter_button, "toggled",
                            G_CALLBACK (moo_font_selection_filter_changed), fontsel);
  if (fontsel->filter_visible)
    gtk_widget_show (fontsel->filter_button);
  gtk_box_pack_start (GTK_BOX (vbox), fontsel->filter_button, FALSE, FALSE, 0);

  /* create the text entry widget */
  label = gtk_label_new_with_mnemonic (D_("_Preview:", "gtk20"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

  text_box = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (text_box);
  gtk_box_pack_start (GTK_BOX (vbox), text_box, FALSE, TRUE, 0);

  fontsel->preview_entry = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), fontsel->preview_entry);

  gtk_widget_show (fontsel->preview_entry);
  g_signal_connect_swapped (fontsel->preview_entry, "changed",
                            G_CALLBACK (moo_font_selection_preview_changed), fontsel);
  gtk_widget_set_size_request (fontsel->preview_entry,
			       -1, INITIAL_PREVIEW_HEIGHT);
  gtk_box_pack_start (GTK_BOX (text_box), fontsel->preview_entry,
		      TRUE, TRUE, 0);

  gtk_widget_pop_composite_child();
}

GtkWidget *
moo_font_selection_new (void)
{
  MooFontSelection *fontsel;

  fontsel = g_object_new (MOO_TYPE_FONT_SELECTION, NULL);

  return GTK_WIDGET (fontsel);
}

static void
moo_font_selection_refresh (MooFontSelection *fontsel)
{
  if (gtk_widget_has_screen (GTK_WIDGET (fontsel)))
    {
      moo_font_selection_show_available_fonts (fontsel);
      moo_font_selection_show_available_sizes (fontsel, TRUE);
      moo_font_selection_show_available_styles (fontsel);
    }
}

static void
moo_font_selection_screen_changed (GtkWidget *widget,
				   G_GNUC_UNUSED GdkScreen *previous_screen)
{
  MooFontSelection *fontsel = MOO_FONT_SELECTION (widget);
  moo_font_selection_refresh (fontsel);
}

static void
moo_font_selection_preview_changed (MooFontSelection *fontsel)
{
  g_object_notify (G_OBJECT (fontsel), "preview_text");
}

static void
moo_font_selection_filter_changed (MooFontSelection *fontsel)
{
    gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (fontsel->filter_button));
    if (!active != !fontsel->monospace)
      moo_font_selection_set_monospace (fontsel, active);
}

static void
scroll_to_selection (GtkTreeView *tree_view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
      gtk_tree_view_scroll_to_cell (tree_view, path, NULL, TRUE, 0.5, 0.5);
      gtk_tree_path_free (path);
    }
}

static void
set_cursor_to_iter (GtkTreeView *view,
		    GtkTreeIter *iter)
{
  GtkTreeModel *model = gtk_tree_view_get_model (view);
  GtkTreePath *path = gtk_tree_model_get_path (model, iter);

  gtk_tree_view_set_cursor (view, path, NULL, FALSE);

  gtk_tree_path_free (path);
}

/* This is called when the list is mapped. Here we scroll to the current
   font if necessary. */
static void
moo_font_selection_scroll_on_map (MooFontSelection *fontsel)
{
  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (GTK_TREE_VIEW (fontsel->family_list));

  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (GTK_TREE_VIEW (fontsel->face_list));

  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (GTK_TREE_VIEW (fontsel->size_list));
}

/* This is called when a family is selected in the list. */
static void
moo_font_selection_select_font (GtkTreeSelection *selection,
				gpointer          data)
{
  MooFontSelection *fontsel;
  GtkTreeModel *model;
  GtkTreeIter iter;

  fontsel = MOO_FONT_SELECTION (data);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      PangoFontFamily *family;

      gtk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      if (fontsel->family != family)
	{
	  fontsel->family = family;

	  moo_font_selection_show_available_styles (fontsel);
	  moo_font_selection_select_best_style (fontsel);
	}

      g_object_unref (family);
    }
}

static int
cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);

  return g_utf8_collate (a_name, b_name);
}

static void
moo_font_selection_show_available_fonts (MooFontSelection *fontsel)
{
  GtkListStore *model;
  PangoFontFamily **families;
  PangoFontFamily *match_family = NULL;
  gint n_families, i;
  GtkTreeIter match_row;

  model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (fontsel->family_list)));

  pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (fontsel)),
			       &families, &n_families);
  qsort (families, n_families, sizeof (PangoFontFamily *), cmp_families);

  gtk_list_store_clear (model);

  for (i=0; i<n_families; i++)
    {
      const gchar *name = pango_font_family_get_name (families[i]);
      GtkTreeIter iter;

      if (fontsel->monospace && !pango_font_family_is_monospace (families[i]))
          continue;

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  FAMILY_COLUMN, families[i],
			  FAMILY_NAME_COLUMN, name,
			  -1);

      if (!match_family)
        {
          match_family = families[i];
          match_row = iter;
        }

      if (!fontsel->monospace)
        {
          if (!g_ascii_strcasecmp (name, "sans"))
	    {
	      match_family = families[i];
	      match_row = iter;
	    }
        }
      else
        {
          if (!g_ascii_strcasecmp (name, "monospace"))
            {
              match_family = families[i];
              match_row = iter;
            }
        }
    }

  if (!match_family && fontsel->monospace)
    {
      g_free (families);
      fontsel->monospace = FALSE;
      moo_font_selection_show_available_fonts (fontsel);
      fontsel->monospace = TRUE;
      return;
    }

  fontsel->family = match_family;
  if (match_family)
    {
      set_cursor_to_iter (GTK_TREE_VIEW (fontsel->family_list), &match_row);
    }

  g_free (families);
}

static int
compare_font_descriptions (const PangoFontDescription *a, const PangoFontDescription *b)
{
  int val = strcmp (pango_font_description_get_family (a), pango_font_description_get_family (b));
  if (val != 0)
    return val;

  if (pango_font_description_get_weight (a) != pango_font_description_get_weight (b))
    return pango_font_description_get_weight (a) - pango_font_description_get_weight (b);

  if (pango_font_description_get_style (a) != pango_font_description_get_style (b))
    return pango_font_description_get_style (a) - pango_font_description_get_style (b);

  if (pango_font_description_get_stretch (a) != pango_font_description_get_stretch (b))
    return pango_font_description_get_stretch (a) - pango_font_description_get_stretch (b);

  if (pango_font_description_get_variant (a) != pango_font_description_get_variant (b))
    return pango_font_description_get_variant (a) - pango_font_description_get_variant (b);

  return 0;
}

static int
faces_sort_func (const void *a, const void *b)
{
  PangoFontDescription *desc_a = pango_font_face_describe (*(PangoFontFace **)a);
  PangoFontDescription *desc_b = pango_font_face_describe (*(PangoFontFace **)b);

  int ord = compare_font_descriptions (desc_a, desc_b);

  pango_font_description_free (desc_a);
  pango_font_description_free (desc_b);

  return ord;
}

static gboolean
font_description_style_equal (const PangoFontDescription *a,
			      const PangoFontDescription *b)
{
  return (pango_font_description_get_weight (a) == pango_font_description_get_weight (b) &&
	  pango_font_description_get_style (a) == pango_font_description_get_style (b) &&
	  pango_font_description_get_stretch (a) == pango_font_description_get_stretch (b) &&
	  pango_font_description_get_variant (a) == pango_font_description_get_variant (b));
}

/* This fills the font style list with all the possible style combinations
   for the current font family. */
static void
moo_font_selection_show_available_styles (MooFontSelection *fontsel)
{
  gint n_faces, i;
  PangoFontFace **faces;
  PangoFontDescription *old_desc;
  GtkListStore *model;
  GtkTreeIter match_row;
  PangoFontFace *match_face = NULL;

  model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (fontsel->face_list)));

  if (fontsel->face)
    old_desc = pango_font_face_describe (fontsel->face);
  else
    old_desc= NULL;

  pango_font_family_list_faces (fontsel->family, &faces, &n_faces);
  qsort (faces, n_faces, sizeof (PangoFontFace *), faces_sort_func);

  gtk_list_store_clear (model);

  for (i=0; i < n_faces; i++)
    {
      GtkTreeIter iter;
      const gchar *str = pango_font_face_get_face_name (faces[i]);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  FACE_COLUMN, faces[i],
			  FACE_NAME_COLUMN, str,
			  -1);

      if (i == 0)
	{
	  match_row = iter;
	  match_face = faces[i];
	}
      else if (old_desc)
	{
	  PangoFontDescription *tmp_desc = pango_font_face_describe (faces[i]);

	  if (font_description_style_equal (tmp_desc, old_desc))
	    {
	      match_row = iter;
	      match_face = faces[i];
	    }

	  pango_font_description_free (tmp_desc);
	}
    }

  if (old_desc)
    pango_font_description_free (old_desc);

  fontsel->face = match_face;
  if (match_face)
    {
      set_cursor_to_iter (GTK_TREE_VIEW (fontsel->face_list), &match_row);
    }

  g_free (faces);
}

/* This selects a style when the user selects a font. It just uses the first
   available style at present. I was thinking of trying to maintain the
   selected style, e.g. bold italic, when the user selects different fonts.
   However, the interface is so easy to use now I'm not sure it's worth it.
   Note: This will load a font. */
static void
moo_font_selection_select_best_style (MooFontSelection *fontsel)
{
  GtkTreeIter iter;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (fontsel->face_list));

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      set_cursor_to_iter (GTK_TREE_VIEW (fontsel->face_list), &iter);
      scroll_to_selection (GTK_TREE_VIEW (fontsel->face_list));
    }

  moo_font_selection_show_available_sizes (fontsel, FALSE);
  moo_font_selection_select_best_size (fontsel);
}


/* This is called when a style is selected in the list. */
static void
moo_font_selection_select_style (GtkTreeSelection *selection,
				 gpointer          data)
{
  MooFontSelection *fontsel = MOO_FONT_SELECTION (data);
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      PangoFontFace *face;

      gtk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      fontsel->face = face;

      g_object_unref (face);
    }

  moo_font_selection_show_available_sizes (fontsel, FALSE);
  moo_font_selection_select_best_size (fontsel);
}

static void
moo_font_selection_show_available_sizes (MooFontSelection *fontsel,
					 gboolean          first_time)
{
  guint i;
  GtkListStore *model;
  gchar buffer[128];
  gchar *p;

  model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (fontsel->size_list)));

  /* Insert the standard font sizes */
  if (first_time)
    {
      gtk_list_store_clear (model);

      for (i = 0; i < G_N_ELEMENTS (font_sizes); i++)
	{
	  GtkTreeIter iter;

	  gtk_list_store_append (model, &iter);
	  gtk_list_store_set (model, &iter, SIZE_COLUMN, font_sizes[i], -1);

	  if (font_sizes[i] * PANGO_SCALE == fontsel->size)
	    set_cursor_to_iter (GTK_TREE_VIEW (fontsel->size_list), &iter);
	}
    }
  else
    {
      GtkTreeIter iter;
      gboolean found = FALSE;

      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);
      for (i = 0; i < G_N_ELEMENTS (font_sizes) && !found; i++)
	{
	  if (font_sizes[i] * PANGO_SCALE == fontsel->size)
	    {
	      set_cursor_to_iter (GTK_TREE_VIEW (fontsel->size_list), &iter);
	      found = TRUE;
	    }

	  gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter);
	}

      if (!found)
	{
	  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fontsel->size_list));
	  gtk_tree_selection_unselect_all (selection);
	}
    }

  /* Set the entry to the new size, rounding to 1 digit,
   * trimming of trailing 0's and a trailing period
   */
  g_snprintf (buffer, sizeof (buffer), "%.1f", fontsel->size / (1.0 * PANGO_SCALE));
  if (strchr (buffer, '.'))
    {
      p = buffer + strlen (buffer) - 1;
      while (*p == '0')
	p--;
      if (*p == '.')
	p--;
      p[1] = '\0';
    }

  /* Compare, to avoid moving the cursor unecessarily */
  if (strcmp (gtk_entry_get_text (GTK_ENTRY (fontsel->size_entry)), buffer) != 0)
    gtk_entry_set_text (GTK_ENTRY (fontsel->size_entry), buffer);
}

static void
moo_font_selection_select_best_size (MooFontSelection *fontsel)
{
  moo_font_selection_load_font (fontsel);
}

static void
moo_font_selection_set_size (MooFontSelection *fontsel,
			     gint              new_size)
{
  if (fontsel->size != new_size)
    {
      fontsel->size = new_size;

      moo_font_selection_show_available_sizes (fontsel, FALSE);
      moo_font_selection_load_font (fontsel);
    }
}

/* If the user hits return in the font size entry, we change to the new font
   size. */
static void
moo_font_selection_size_activate (GtkWidget   *w,
                                  gpointer     data)
{
  MooFontSelection *fontsel;
  gint new_size;
  const gchar *text;

  fontsel = MOO_FONT_SELECTION (data);

  text = gtk_entry_get_text (GTK_ENTRY (fontsel->size_entry));
  new_size = (gint) MAX (0.1, atof (text) * PANGO_SCALE + 0.5);

  if (fontsel->size != new_size)
    moo_font_selection_set_size (fontsel, new_size);
  else
    list_row_activated (w);
}

static gboolean
moo_font_selection_size_focus_out (MooFontSelection *fontsel)
{
  gint new_size;
  const gchar *text;

  text = gtk_entry_get_text (GTK_ENTRY (fontsel->size_entry));
  new_size = (gint) MAX (0.1, atof (text) * PANGO_SCALE + 0.5);

  moo_font_selection_set_size (fontsel, new_size);

  return TRUE;
}

/* This is called when a size is selected in the list. */
static void
moo_font_selection_select_size (GtkTreeSelection *selection,
				gpointer          data)
{
  MooFontSelection *fontsel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint new_size;

  fontsel = MOO_FONT_SELECTION (data);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, SIZE_COLUMN, &new_size, -1);
      moo_font_selection_set_size (fontsel, new_size * PANGO_SCALE);
    }
}

static void
moo_font_selection_load_font (MooFontSelection *fontsel)
{
  moo_font_selection_update_preview (fontsel);
}

static PangoFontDescription *
moo_font_selection_get_font_description (MooFontSelection *fontsel)
{
  PangoFontDescription *font_desc;

  if (fontsel->face)
    {
      font_desc = pango_font_face_describe (fontsel->face);
      pango_font_description_set_size (font_desc, fontsel->size);
    }
  else
    font_desc = pango_font_description_from_string ("Sans 10");

  return font_desc;
}

/* This sets the font in the preview entry to the selected font, and tries to
   make sure that the preview entry is a reasonable size, i.e. so that the
   text can be seen with a bit of space to spare. But it tries to avoid
   resizing the entry every time the font changes.
   This also used to shrink the preview if the font size was decreased, but
   that made it awkward if the user wanted to resize the window themself. */
static void
moo_font_selection_update_preview (MooFontSelection *fontsel)
{
  GtkRcStyle *rc_style;
  gint new_height;
  GtkRequisition old_requisition;
  GtkWidget *preview_entry = fontsel->preview_entry;
  const gchar *text;

  gtk_widget_get_child_requisition (preview_entry, &old_requisition);

  rc_style = gtk_rc_style_new ();
  rc_style->font_desc = moo_font_selection_get_font_description (fontsel);

  gtk_widget_modify_style (preview_entry, rc_style);
  g_object_unref (rc_style);

  gtk_widget_size_request (preview_entry, NULL);

  /* We don't ever want to be over MAX_PREVIEW_HEIGHT pixels high. */
  new_height = CLAMP (preview_entry->requisition.height, INITIAL_PREVIEW_HEIGHT, MAX_PREVIEW_HEIGHT);

  if (new_height > old_requisition.height || new_height < old_requisition.height - 30)
    gtk_widget_set_size_request (preview_entry, -1, new_height);

  /* This sets the preview text, if it hasn't been set already. */
  text = gtk_entry_get_text (GTK_ENTRY (preview_entry));
  if (strlen (text) == 0)
    gtk_entry_set_text (GTK_ENTRY (preview_entry), D_(PREVIEW_TEXT, "gtk20"));
  gtk_editable_set_position (GTK_EDITABLE (preview_entry), 0);
}


/*****************************************************************************
 * These functions are the main public interface for getting/setting the font.
 *****************************************************************************/

/**
 * moo_font_selection_get_font_name:
 * @fontsel: a #MooFontSelection
 *
 * Gets the currently-selected font name.  Note that this can be a different
 * string than what you set with moo_font_selection_set_font_name(), as
 * the font selection widget may normalize font names and thus return a string
 * with a different structure.  For example, "Helvetica Italic Bold 12" could be
 * normalized to "Helvetica Bold Italic 12".  Use pango_font_description_equal()
 * if you want to compare two font descriptions.
 *
 * Return value: A string with the name of the current font, or #NULL if no font
 * is selected.  You must free this string with g_free().
 **/
gchar *
moo_font_selection_get_font_name (MooFontSelection *fontsel)
{
  gchar *result;

  PangoFontDescription *font_desc = moo_font_selection_get_font_description (fontsel);
  result = pango_font_description_to_string (font_desc);
  pango_font_description_free (font_desc);

  return result;
}


/* This sets the current font, selecting the appropriate list rows.
   First we check the fontname is valid and try to find the font family
   - i.e. the name in the main list. If we can't find that, then just return.
   Next we try to set each of the properties according to the fontname.
   Finally we select the font family & style in the lists. */

/**
 * moo_font_selection_set_font_name:
 * @fontsel: a #MooFontSelection
 * @fontname: a font name like "Helvetica 12" or "Times Bold 18"
 *
 * Sets the currently-selected font.  Note that the @fontsel needs to know the
 * screen in which it will appear for this to work; this can be guaranteed by
 * simply making sure that the @fontsel is inserted in a toplevel window before
 * you call this function.
 *
 * Return value: #TRUE if the font could be set successfully; #FALSE if no such
 * font exists or if the @fontsel doesn't belong to a particular screen yet.
 **/
gboolean
moo_font_selection_set_font_name (MooFontSelection *fontsel,
				  const gchar      *fontname)
{
  PangoFontFamily *new_family = NULL;
  PangoFontFace *new_face = NULL;
  PangoFontFace *fallback_face = NULL;
  PangoFontDescription *new_desc;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeIter match_iter;
  gboolean valid;
  const gchar *new_family_name;

  g_return_val_if_fail (MOO_IS_FONT_SELECTION (fontsel), FALSE);
  g_return_val_if_fail (gtk_widget_has_screen (GTK_WIDGET (fontsel)), FALSE);

  new_desc = pango_font_description_from_string (fontname);
  new_family_name = pango_font_description_get_family (new_desc);

  if (!new_family_name)
    return FALSE;

  /* Check to make sure that this is in the list of allowed fonts
   */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (fontsel->family_list));
  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      PangoFontFamily *family;

      gtk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);

      if (g_ascii_strcasecmp (pango_font_family_get_name (family),
			      new_family_name) == 0)
	new_family = family;

      g_object_unref (family);

      if (new_family)
	break;
    }

  if (!new_family)
    return FALSE;

  fontsel->family = new_family;
  set_cursor_to_iter (GTK_TREE_VIEW (fontsel->family_list), &iter);
  moo_font_selection_show_available_styles (fontsel);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (fontsel->face_list));
  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      PangoFontFace *face;
      PangoFontDescription *tmp_desc;

      gtk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      tmp_desc = pango_font_face_describe (face);

      if (font_description_style_equal (tmp_desc, new_desc))
	new_face = face;

      if (!fallback_face)
	{
	  fallback_face = face;
	  match_iter = iter;
	}

      pango_font_description_free (tmp_desc);
      g_object_unref (face);

      if (new_face)
	{
	  match_iter = iter;
	  break;
	}
    }

  if (!new_face)
    new_face = fallback_face;

  fontsel->face = new_face;
  set_cursor_to_iter (GTK_TREE_VIEW (fontsel->face_list), &match_iter);

  moo_font_selection_set_size (fontsel, pango_font_description_get_size (new_desc));

  g_object_notify (G_OBJECT (fontsel), "font_name");

  pango_font_description_free (new_desc);

  return TRUE;
}


/* This returns the text in the preview entry. You should copy the returned
   text if you need it. */
const gchar*
moo_font_selection_get_preview_text  (MooFontSelection *fontsel)
{
  return gtk_entry_get_text (GTK_ENTRY (fontsel->preview_entry));
}


/* This sets the text in the preview entry. */
void
moo_font_selection_set_preview_text  (MooFontSelection *fontsel,
				      const gchar	  *text)
{
  gtk_entry_set_text (GTK_ENTRY (fontsel->preview_entry), text);
}


void
moo_font_selection_set_monospace (MooFontSelection *fontsel,
                                  gboolean          monospace)
{
  g_return_if_fail (MOO_IS_FONT_SELECTION (fontsel));

  if (!monospace != !fontsel->monospace)
    {
      fontsel->monospace = monospace != 0;
      moo_font_selection_refresh (fontsel);
      if (fontsel->filter_button)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fontsel->filter_button),
                                      monospace != 0);
      g_object_notify (G_OBJECT (fontsel), "monospace");
    }
}


void
moo_font_selection_set_filter_visible (MooFontSelection *fontsel,
                                       gboolean          visible)
{
  g_return_if_fail (MOO_IS_FONT_SELECTION (fontsel));

  if (!visible != !fontsel->filter_visible)
    {
      fontsel->filter_visible = visible != 0;
      if (fontsel->filter_button)
        g_object_set (fontsel->filter_button, "visible", visible, NULL);
      g_object_notify (G_OBJECT (fontsel), "filter-visible");
    }
}


/*****************************************************************************
 * MooFontSelectionDialog
 *****************************************************************************/

G_DEFINE_TYPE(MooFontSelectionDialog, moo_font_selection_dialog, GTK_TYPE_DIALOG)

static void
moo_font_selection_dialog_class_init (G_GNUC_UNUSED MooFontSelectionDialogClass *klass)
{
}

static void
moo_font_selection_dialog_init (MooFontSelectionDialog *fontseldiag)
{
  GtkDialog *dialog = GTK_DIALOG (fontseldiag);

  gtk_dialog_set_has_separator (dialog, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (dialog->action_area), 6);

  gtk_widget_push_composite_child ();

  gtk_window_set_resizable (GTK_WINDOW (fontseldiag), TRUE);

  fontseldiag->main_vbox = dialog->vbox;

  fontseldiag->fontsel = moo_font_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (fontseldiag->fontsel), 5);
  gtk_widget_show (fontseldiag->fontsel);
  gtk_box_pack_start (GTK_BOX (fontseldiag->main_vbox),
		      fontseldiag->fontsel, TRUE, TRUE, 0);

  /* Create the action area */
  fontseldiag->action_area = dialog->action_area;

  fontseldiag->cancel_button = gtk_dialog_add_button (dialog,
                                                      GTK_STOCK_CANCEL,
                                                      GTK_RESPONSE_CANCEL);

  fontseldiag->apply_button = gtk_dialog_add_button (dialog,
                                                     GTK_STOCK_APPLY,
                                                     GTK_RESPONSE_APPLY);
  gtk_widget_hide (fontseldiag->apply_button);

  fontseldiag->ok_button = gtk_dialog_add_button (dialog,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_OK);
  gtk_widget_grab_default (fontseldiag->ok_button);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (fontseldiag),
					   GTK_RESPONSE_OK,
					   GTK_RESPONSE_APPLY,
					   GTK_RESPONSE_CANCEL,
					   -1);

  gtk_window_set_title (GTK_WINDOW (fontseldiag),
                        D_("Font Selection", "gtk20"));

  gtk_widget_pop_composite_child ();

  gtk_dialog_set_has_separator (dialog, FALSE);
}

GtkWidget*
moo_font_selection_dialog_new (const gchar *title)
{
  MooFontSelectionDialog *fontseldiag;

  fontseldiag = g_object_new (MOO_TYPE_FONT_SELECTION_DIALOG, NULL);

  if (title)
    gtk_window_set_title (GTK_WINDOW (fontseldiag), title);

  return GTK_WIDGET (fontseldiag);
}

/**
 * moo_font_selection_dialog_get_font_name:
 * @fsd: a #MooFontSelectionDialog
 *
 * Gets the currently-selected font name.  Note that this can be a different
 * string than what you set with moo_font_selection_dialog_set_font_name(), as
 * the font selection widget may normalize font names and thus return a string
 * with a different structure.  For example, "Helvetica Italic Bold 12" could be
 * normalized to "Helvetica Bold Italic 12".  Use pango_font_description_equal()
 * if you want to compare two font descriptions.
 *
 * Return value: A string with the name of the current font, or #NULL if no font
 * is selected.  You must free this string with g_free().
 **/
gchar*
moo_font_selection_dialog_get_font_name (MooFontSelectionDialog *fsd)
{
  return moo_font_selection_get_font_name (MOO_FONT_SELECTION (fsd->fontsel));
}

gboolean
moo_font_selection_dialog_set_font_name (MooFontSelectionDialog *fsd,
					 const gchar	  *fontname)
{
  return moo_font_selection_set_font_name (MOO_FONT_SELECTION (fsd->fontsel), fontname);
}

const gchar*
moo_font_selection_dialog_get_preview_text (MooFontSelectionDialog *fsd)
{
  return moo_font_selection_get_preview_text (MOO_FONT_SELECTION (fsd->fontsel));
}

void
moo_font_selection_dialog_set_preview_text (MooFontSelectionDialog *fsd,
					    const gchar	           *text)
{
  moo_font_selection_set_preview_text (MOO_FONT_SELECTION (fsd->fontsel), text);
}



/*****************************************************************************/
/* MooFontButton
 */

#define MOO_FONT_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MOO_TYPE_FONT_BUTTON, MooFontButtonPrivate))

struct _MooFontButtonPrivate
{
  gchar         *title;

  gchar         *fontname;

  guint         use_font : 1;
  guint         use_size : 1;
  guint         show_style : 1;
  guint         show_size : 1;
  guint         monospace : 1;
  guint         filter_visible : 1;

  GtkWidget     *font_dialog;
  GtkWidget     *inside;
  GtkWidget     *font_label;
  GtkWidget     *size_label;
};

/* Signals */
enum
{
  FONT_SET,
  LAST_SIGNAL
};

enum
{
  BUTTON_PROP_0,
  BUTTON_PROP_TITLE,
  BUTTON_PROP_FONT_NAME,
  BUTTON_PROP_USE_FONT,
  BUTTON_PROP_USE_SIZE,
  BUTTON_PROP_SHOW_STYLE,
  BUTTON_PROP_SHOW_SIZE,
  BUTTON_PROP_MONOSPACE,
  BUTTON_PROP_FILTER_VISIBLE
};

/* Prototypes */
static void moo_font_button_init                   (MooFontButton      *font_button);
static void moo_font_button_class_init             (MooFontButtonClass *klass);
static void moo_font_button_finalize               (GObject            *object);
static void moo_font_button_get_property           (GObject            *object,
                                                    guint               param_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void moo_font_button_set_property           (GObject            *object,
                                                    guint               param_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);

static void moo_font_button_realize                 (GtkWidget         *widget);
static void moo_font_button_clicked                 (GtkButton         *button);

/* Dialog response functions */
static void dialog_ok_clicked                       (MooFontButton     *font_button);
static void dialog_cancel_clicked                   (MooFontButton     *font_button);
static void dialog_destroy                          (MooFontButton     *font_button);

/* Auxiliary functions */
static void moo_font_button_update_inside           (MooFontButton     *gfs);
static void moo_font_button_label_use_font          (MooFontButton     *gfs);
static void moo_font_button_update_font_info        (MooFontButton     *gfs);

static gpointer parent_class = NULL;
static guint font_button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(MooFontButton, moo_font_button, GTK_TYPE_BUTTON)


static void
moo_font_button_class_init (MooFontButtonClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;
  GtkButtonClass *button_class;

  gobject_class = (GObjectClass *) klass;
  button_class = (GtkButtonClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = moo_font_button_finalize;
  gobject_class->set_property = moo_font_button_set_property;
  gobject_class->get_property = moo_font_button_get_property;

  widget_class->realize = moo_font_button_realize;

  button_class->clicked = moo_font_button_clicked;

  klass->font_set = NULL;

  /**
   * MooFontButton:title:
   *
   * The title of the font selection dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        P_("Title"),
                                                        P_("The title of the font selection dialog"),
                                                        D_("Pick a Font", "gtk20"),
                                                        G_PARAM_READWRITE));

  /**
   * MooFontButton:font-name:
   *
   * The name of the currently selected font.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        P_("Font name"),
                                                        P_("The name of the selected font"),
                                                        P_("Sans 12"),
                                                        G_PARAM_READWRITE));

  /**
   * MooFontButton:use-font:
   *
   * If this property is set to %TRUE, the label will be drawn in the selected font.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_USE_FONT,
                                   g_param_spec_boolean ("use-font",
                                                         P_("Use font in label"),
                                                         P_("Whether the label is drawn in the selected font"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * MooFontButton:use-size:
   *
   * If this property is set to %TRUE, the label will be drawn with the selected font size.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_USE_SIZE,
                                   g_param_spec_boolean ("use-size",
                                                         P_("Use size in label"),
                                                         P_("Whether the label is drawn with the selected font size"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * MooFontButton:show-style:
   *
   * If this property is set to %TRUE, the name of the selected font style will be shown in the label. For
   * a more WYSIWIG way to show the selected style, see the ::use-font property.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_SHOW_STYLE,
                                   g_param_spec_boolean ("show-style",
                                                         P_("Show style"),
                                                         P_("Whether the selected font style is shown in the label"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  /**
   * MooFontButton:show-size:
   *
   * If this property is set to %TRUE, the selected font size will be shown in the label. For
   * a more WYSIWIG way to show the selected size, see the ::use-size property.
   *
   * Since: 2.4
   */
  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_SHOW_SIZE,
                                   g_param_spec_boolean ("show-size",
                                                         P_("Show size"),
                                                         P_("Whether selected font size is shown in the label"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_MONOSPACE,
                                   g_param_spec_boolean ("monospace",
                                                         P_("monospace"),
                                                         P_("monospace"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   BUTTON_PROP_FILTER_VISIBLE,
                                   g_param_spec_boolean ("filter-visible",
                                                         P_("filter-visible"),
                                                         P_("filter-visible"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  /**
   * MooFontButton::font-set:
   * @widget: the object which received the signal.
   *
   * The ::font-set signal is emitted when the user selects a font. When handling this signal,
   * use moo_font_button_get_font_name() to find out which font was just selected.
   *
   * Since: 2.4
   */
  font_button_signals[FONT_SET] = g_signal_new ("font-set",
                                                G_TYPE_FROM_CLASS (gobject_class),
                                                G_SIGNAL_RUN_FIRST,
                                                G_STRUCT_OFFSET (MooFontButtonClass, font_set),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE, 0);

  g_type_class_add_private (gobject_class, sizeof (MooFontButtonPrivate));
}

static void
moo_font_button_init (MooFontButton *font_button)
{
  font_button->priv = MOO_FONT_BUTTON_GET_PRIVATE (font_button);

  /* Initialize fields */
  font_button->priv->fontname = g_strdup (D_("Sans 12", "gtk20"));
  font_button->priv->use_font = FALSE;
  font_button->priv->use_size = FALSE;
  font_button->priv->show_style = TRUE;
  font_button->priv->show_size = TRUE;
  font_button->priv->font_dialog = NULL;
  font_button->priv->title = g_strdup (D_("Pick a Font", "gtk20"));
  font_button->priv->monospace = FALSE;
  font_button->priv->filter_visible = TRUE;

  font_button->priv->inside = NULL;

  moo_font_button_update_font_info (font_button);
}


static void
moo_font_button_finalize (GObject *object)
{
  MooFontButton *font_button = MOO_FONT_BUTTON (object);

  if (font_button->priv->font_dialog != NULL)
    gtk_widget_destroy (font_button->priv->font_dialog);
  font_button->priv->font_dialog = NULL;

  g_free (font_button->priv->fontname);
  font_button->priv->fontname = NULL;

  g_free (font_button->priv->title);
  font_button->priv->title = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
moo_font_button_set_property (GObject      *object,
                              guint         param_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  MooFontButton *font_button = MOO_FONT_BUTTON (object);

  switch (param_id)
    {
    case BUTTON_PROP_TITLE:
      moo_font_button_set_title (font_button, g_value_get_string (value));
      break;
    case BUTTON_PROP_FONT_NAME:
      moo_font_button_set_font_name (font_button, g_value_get_string (value));
      break;
    case BUTTON_PROP_USE_FONT:
      moo_font_button_set_use_font (font_button, g_value_get_boolean (value));
      break;
    case BUTTON_PROP_USE_SIZE:
      moo_font_button_set_use_size (font_button, g_value_get_boolean (value));
      break;
    case BUTTON_PROP_SHOW_STYLE:
      moo_font_button_set_show_style (font_button, g_value_get_boolean (value));
      break;
    case BUTTON_PROP_SHOW_SIZE:
      moo_font_button_set_show_size (font_button, g_value_get_boolean (value));
      break;
    case BUTTON_PROP_MONOSPACE:
      moo_font_button_set_monospace (font_button, g_value_get_boolean (value));
      break;
    case BUTTON_PROP_FILTER_VISIBLE:
      moo_font_button_set_filter_visible (font_button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
moo_font_button_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  MooFontButton *font_button = MOO_FONT_BUTTON (object);

  switch (param_id)
    {
    case BUTTON_PROP_TITLE:
      g_value_set_string (value, moo_font_button_get_title (font_button));
      break;
    case BUTTON_PROP_FONT_NAME:
      g_value_set_string (value, moo_font_button_get_font_name (font_button));
      break;
    case BUTTON_PROP_USE_FONT:
      g_value_set_boolean (value, moo_font_button_get_use_font (font_button));
      break;
    case BUTTON_PROP_USE_SIZE:
      g_value_set_boolean (value, moo_font_button_get_use_size (font_button));
      break;
    case BUTTON_PROP_SHOW_STYLE:
      g_value_set_boolean (value, moo_font_button_get_show_style (font_button));
      break;
    case BUTTON_PROP_SHOW_SIZE:
      g_value_set_boolean (value, moo_font_button_get_show_size (font_button));
      break;
    case BUTTON_PROP_MONOSPACE:
      g_value_set_boolean (value, moo_font_button_get_monospace (font_button));
      break;
    case BUTTON_PROP_FILTER_VISIBLE:
      g_value_set_boolean (value, moo_font_button_get_filter_visible (font_button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
moo_font_button_realize (GtkWidget *widget)
{
  MooFontButton *font_button = MOO_FONT_BUTTON (widget);

  GTK_WIDGET_CLASS (moo_font_button_parent_class)->realize (widget);
  moo_font_button_update_inside (font_button);

  if (font_button->priv->use_font)
    moo_font_button_label_use_font (font_button);
  else
    gtk_widget_set_style (font_button->priv->font_label, NULL);
}


/**
 * moo_font_button_new:
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Since: 2.4
 */
GtkWidget *
moo_font_button_new (void)
{
  return g_object_new (MOO_TYPE_FONT_BUTTON, NULL);
}

/**
 * moo_font_button_new_with_font:
 * @fontname: Name of font to display in font selection dialog
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Since: 2.4
 */
GtkWidget *
moo_font_button_new_with_font (const gchar *fontname)
{
  return g_object_new (MOO_TYPE_FONT_BUTTON, "font_name", fontname, NULL);
}

/**
 * moo_font_button_set_title:
 * @font_button: a #MooFontButton
 * @title: a string containing the font selection dialog title
 *
 * Sets the title for the font selection dialog.
 *
 * Since: 2.4
 */
void
moo_font_button_set_title (MooFontButton *font_button,
                           const gchar   *title)
{
  gchar *old_title;
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  old_title = font_button->priv->title;
  font_button->priv->title = g_strdup (title);
  g_free (old_title);

  if (font_button->priv->font_dialog)
    gtk_window_set_title (GTK_WINDOW (font_button->priv->font_dialog),
                          font_button->priv->title);

  g_object_notify (G_OBJECT (font_button), "title");
}

/**
 * moo_font_button_get_title:
 * @font_button: a #MooFontButton
 *
 * Retrieves the title of the font selection dialog.
 *
 * Returns: an internal copy of the title string which must not be freed.
 *
 * Since: 2.4
 */
const gchar*
moo_font_button_get_title (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), NULL);

  return font_button->priv->title;
}

/**
 * moo_font_button_get_use_font:
 * @font_button: a #MooFontButton
 *
 * Returns whether the selected font is used in the label.
 *
 * Returns: whether the selected font is used in the label.
 *
 * Since: 2.4
 */
gboolean
moo_font_button_get_use_font (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->use_font;
}

/**
 * moo_font_button_set_use_font:
 * @font_button: a #MooFontButton
 * @use_font: If %TRUE, font name will be written using font chosen.
 *
 * If @use_font is %TRUE, the font name will be written using the selected font.
 *
 * Since: 2.4
 */
void
moo_font_button_set_use_font (MooFontButton *font_button,
                              gboolean       use_font)
{
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  use_font = (use_font != FALSE);

  if (font_button->priv->use_font != use_font)
    {
      font_button->priv->use_font = use_font;

      if (font_button->priv->font_label)
        {
          if (use_font)
            moo_font_button_label_use_font (font_button);
          else
            gtk_widget_set_style (font_button->priv->font_label, NULL);
        }

     g_object_notify (G_OBJECT (font_button), "use-font");
    }
}


/**
 * moo_font_button_get_use_size:
 * @font_button: a #MooFontButton
 *
 * Returns whether the selected size is used in the label.
 *
 * Returns: whether the selected size is used in the label.
 *
 * Since: 2.4
 */
gboolean
moo_font_button_get_use_size (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->use_size;
}

/**
 * moo_font_button_set_use_size:
 * @font_button: a #MooFontButton
 * @use_size: If %TRUE, font name will be written using the selected size.
 *
 * If @use_size is %TRUE, the font name will be written using the selected size.
 *
 * Since: 2.4
 */
void
moo_font_button_set_use_size (MooFontButton *font_button,
                              gboolean       use_size)
{
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  use_size = (use_size != FALSE);
  if (font_button->priv->use_size != use_size)
    {
      font_button->priv->use_size = use_size;

      if (font_button->priv->use_font && font_button->priv->font_label)
        moo_font_button_label_use_font (font_button);

      g_object_notify (G_OBJECT (font_button), "use-size");
    }
}

/**
 * moo_font_button_get_show_style:
 * @font_button: a #MooFontButton
 *
 * Returns whether the name of the font style will be shown in the label.
 *
 * Return value: whether the font style will be shown in the label.
 *
 * Since: 2.4
 **/
gboolean
moo_font_button_get_show_style (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->show_style;
}

/**
 * moo_font_button_set_show_style:
 * @font_button: a #MooFontButton
 * @show_style: %TRUE if font style should be displayed in label.
 *
 * If @show_style is %TRUE, the font style will be displayed along with name of the selected font.
 *
 * Since: 2.4
 */
void
moo_font_button_set_show_style (MooFontButton *font_button,
                                gboolean       show_style)
{
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  show_style = (show_style != FALSE);
  if (font_button->priv->show_style != show_style)
    {
      font_button->priv->show_style = show_style;

      moo_font_button_update_font_info (font_button);

      g_object_notify (G_OBJECT (font_button), "show-style");
    }
}


/**
 * moo_font_button_get_show_size:
 * @font_button: a #MooFontButton
 *
 * Returns whether the font size will be shown in the label.
 *
 * Return value: whether the font size will be shown in the label.
 *
 * Since: 2.4
 **/
gboolean
moo_font_button_get_show_size (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->show_size;
}

/**
 * moo_font_button_set_show_size:
 * @font_button: a #MooFontButton
 * @show_size: %TRUE if font size should be displayed in dialog.
 *
 * If @show_size is %TRUE, the font size will be displayed along with the name of the selected font.
 *
 * Since: 2.4
 */
void
moo_font_button_set_show_size (MooFontButton *font_button,
                               gboolean       show_size)
{
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  show_size = (show_size != FALSE);

  if (font_button->priv->show_size != show_size)
    {
      font_button->priv->show_size = show_size;
      moo_font_button_update_inside (font_button);
      g_object_notify (G_OBJECT (font_button), "show-size");
    }
}


gboolean
moo_font_button_get_monospace (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);
  return font_button->priv->monospace != 0;
}

void
moo_font_button_set_monospace (MooFontButton *font_button,
                               gboolean       monospace)
{
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  if (!font_button->priv->monospace != !monospace)
    {
      font_button->priv->monospace = monospace != 0;

      moo_font_button_update_inside (font_button);

      if (font_button->priv->font_dialog)
        moo_font_selection_set_monospace (MOO_FONT_SELECTION (MOO_FONT_SELECTION_DIALOG (font_button->priv->font_dialog)->fontsel),
                                          monospace);

      g_object_notify (G_OBJECT (font_button), "monospace");
    }
}


gboolean
moo_font_button_get_filter_visible (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);
  return font_button->priv->filter_visible != 0;
}

void
moo_font_button_set_filter_visible (MooFontButton *font_button,
                                    gboolean       visible)
{
  g_return_if_fail (MOO_IS_FONT_BUTTON (font_button));

  if (!font_button->priv->filter_visible != !visible)
    {
      font_button->priv->filter_visible = visible != 0;

      if (font_button->priv->font_dialog)
        moo_font_selection_set_filter_visible (MOO_FONT_SELECTION (MOO_FONT_SELECTION_DIALOG (font_button->priv->font_dialog)->fontsel),
                                               visible);

      g_object_notify (G_OBJECT (font_button), "filter-visible");
    }
}


/**
 * moo_font_button_get_font_name:
 * @font_button: a #MooFontButton
 *
 * Retrieves the name of the currently selected font.
 *
 * Returns: an internal copy of the font name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
moo_font_button_get_font_name (MooFontButton *font_button)
{
  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), NULL);

  return font_button->priv->fontname;
}

/**
 * moo_font_button_set_font_name:
 * @font_button: a #MooFontButton
 * @fontname: Name of font to display in font selection dialog
 *
 * Sets or updates the currently-displayed font in font picker dialog.
 *
 * Returns: Return value of moo_font_selection_dialog_set_font_name() if the
 * font selection dialog exists, otherwise %FALSE.
 *
 * Since: 2.4
 */
gboolean
moo_font_button_set_font_name (MooFontButton *font_button,
                               const gchar    *fontname)
{
  gboolean result;
  gchar *old_fontname;

  g_return_val_if_fail (MOO_IS_FONT_BUTTON (font_button), FALSE);
  g_return_val_if_fail (fontname != NULL, FALSE);

  if (g_ascii_strcasecmp (font_button->priv->fontname, fontname))
    {
      old_fontname = font_button->priv->fontname;
      font_button->priv->fontname = g_strdup (fontname);
      g_free (old_fontname);
    }

  moo_font_button_update_font_info (font_button);

  if (font_button->priv->font_dialog)
    result = moo_font_selection_dialog_set_font_name (MOO_FONT_SELECTION_DIALOG (font_button->priv->font_dialog),
                                                      font_button->priv->fontname);
  else
    result = FALSE;

  g_object_notify (G_OBJECT (font_button), "font-name");

  return result;
}

static void
moo_font_button_clicked (GtkButton *button)
{
  MooFontSelectionDialog *font_dialog;
  MooFontButton    *font_button = MOO_FONT_BUTTON (button);

  if (!font_button->priv->font_dialog)
    {
      GtkWidget *parent;

      parent = gtk_widget_get_toplevel (GTK_WIDGET (font_button));

      font_button->priv->font_dialog = moo_font_selection_dialog_new (font_button->priv->title);

      font_dialog = MOO_FONT_SELECTION_DIALOG (font_button->priv->font_dialog);
      moo_font_selection_set_monospace (MOO_FONT_SELECTION (font_dialog->fontsel),
                                        font_button->priv->monospace);
      moo_font_selection_set_filter_visible (MOO_FONT_SELECTION (font_dialog->fontsel),
                                             font_button->priv->filter_visible);

      if (GTK_WIDGET_TOPLEVEL (parent) && GTK_IS_WINDOW (parent))
        {
          if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (font_dialog)))
            gtk_window_set_transient_for (GTK_WINDOW (font_dialog), GTK_WINDOW (parent));

          gtk_window_set_modal (GTK_WINDOW (font_dialog),
                                gtk_window_get_modal (GTK_WINDOW (parent)));
        }

      g_signal_connect_swapped (font_dialog->ok_button, "clicked",
                                G_CALLBACK (dialog_ok_clicked), font_button);
      g_signal_connect_swapped (font_dialog->cancel_button, "clicked",
                                G_CALLBACK (dialog_cancel_clicked), font_button);
      g_signal_connect_swapped (font_dialog, "destroy",
                                G_CALLBACK (dialog_destroy), font_button);
    }

  if (!GTK_WIDGET_VISIBLE (font_button->priv->font_dialog))
    {
      font_dialog = MOO_FONT_SELECTION_DIALOG (font_button->priv->font_dialog);

      moo_font_selection_dialog_set_font_name (font_dialog, font_button->priv->fontname);

    }

  gtk_window_present (GTK_WINDOW (font_button->priv->font_dialog));
}

static void
dialog_ok_clicked (MooFontButton *font_button)
{
  gtk_widget_hide (font_button->priv->font_dialog);

  g_free (font_button->priv->fontname);
  font_button->priv->fontname = moo_font_selection_dialog_get_font_name (MOO_FONT_SELECTION_DIALOG (font_button->priv->font_dialog));

  /* Set label font */
  moo_font_button_update_font_info (font_button);

  g_object_notify (G_OBJECT (font_button), "font-name");

  /* Emit font_set signal */
  g_signal_emit (font_button, font_button_signals[FONT_SET], 0);
}


static void
dialog_cancel_clicked (MooFontButton *font_button)
{
  gtk_widget_hide (font_button->priv->font_dialog);
}

static void
dialog_destroy (MooFontButton *font_button)
{
  /* Dialog will get destroyed so reference is not valid now */
  font_button->priv->font_dialog = NULL;
}

static GtkWidget *
moo_font_button_create_inside (MooFontButton *font_button)
{
  GtkWidget *widget;

  gtk_widget_push_composite_child ();

  widget = gtk_hbox_new (FALSE, 0);

  font_button->priv->font_label = gtk_label_new (D_("Font", "gtk20"));

  gtk_label_set_justify (GTK_LABEL (font_button->priv->font_label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (widget), font_button->priv->font_label, TRUE, TRUE, 5);

  if (font_button->priv->show_size)
    {
      gtk_box_pack_start (GTK_BOX (widget), gtk_vseparator_new (), FALSE, FALSE, 0);
      font_button->priv->size_label = gtk_label_new ("14");
      gtk_box_pack_start (GTK_BOX (widget), font_button->priv->size_label, FALSE, FALSE, 5);
    }

  gtk_widget_show_all (widget);

  gtk_widget_pop_composite_child ();

  return widget;
}

static void
moo_font_button_update_inside (MooFontButton *font_button)
{
  if (GTK_WIDGET_REALIZED (font_button))
    {
      if (font_button->priv->inside)
        gtk_container_remove (GTK_CONTAINER (font_button), font_button->priv->inside);

      font_button->priv->inside = moo_font_button_create_inside (font_button);
      gtk_container_add (GTK_CONTAINER (font_button), font_button->priv->inside);

      moo_font_button_update_font_info (font_button);
    }
}

static void
moo_font_button_label_use_font (MooFontButton *font_button)
{
  PangoFontDescription *desc;

  if (!font_button->priv->use_font)
    return;

  desc = pango_font_description_from_string (font_button->priv->fontname);

  if (!font_button->priv->use_size)
    pango_font_description_unset_fields (desc, PANGO_FONT_MASK_SIZE);

  if (font_button->priv->font_label)
    gtk_widget_modify_font (font_button->priv->font_label, desc);

  pango_font_description_free (desc);
}

static void
moo_font_button_update_font_info (MooFontButton *font_button)
{
  PangoFontDescription *desc;
  const gchar *family;
  gchar *style;
  gchar *family_style;

  if (!GTK_WIDGET_REALIZED (font_button))
    return;

  if (!font_button->priv->inside)
    {
      font_button->priv->inside = moo_font_button_create_inside (font_button);
      gtk_container_add (GTK_CONTAINER (font_button), font_button->priv->inside);
    }

  desc = pango_font_description_from_string (font_button->priv->fontname);
  family = pango_font_description_get_family (desc);

#if 0
  /* This gives the wrong names, e.g. Italic when the font selection
   * dialog displayed Oblique.
   */
  pango_font_description_unset_fields (desc, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);
  style = pango_font_description_to_string (desc);
  gtk_label_set_text (GTK_LABEL (font_button->priv->style_label), style);
#endif

  style = NULL;
  if (font_button->priv->show_style && family)
    {
      PangoFontFamily **families;
      PangoFontFace **faces;
      gint n_families, n_faces, i;

      n_families = 0;
      pango_context_list_families (gtk_widget_get_pango_context (GTK_WIDGET (font_button)),
                                   &families, &n_families);
      n_faces = 0;
      faces = NULL;
      for (i = 0; i < n_families; i++)
        {
          const gchar *name = pango_font_family_get_name (families[i]);

          if (!g_ascii_strcasecmp (name, family))
            {
              pango_font_family_list_faces (families[i], &faces, &n_faces);
              break;
            }
        }
      g_free (families);

      for (i = 0; i < n_faces; i++)
        {
          PangoFontDescription *tmp_desc = pango_font_face_describe (faces[i]);

          if (font_description_style_equal (tmp_desc, desc))
            {
              style = g_strdup (pango_font_face_get_face_name (faces[i]));
              pango_font_description_free (tmp_desc);
              break;
            }
          else
            pango_font_description_free (tmp_desc);
        }
      g_free (faces);
    }

  if (style == NULL || !g_ascii_strcasecmp (style, "Regular"))
    family_style = g_strdup (family);
  else
    family_style = g_strdup_printf ("%s %s", family, style);

  gtk_label_set_text (GTK_LABEL (font_button->priv->font_label), family_style);

  g_free (style);
  g_free (family_style);

  if (font_button->priv->show_size)
    {
      gchar *size = g_strdup_printf ("%g",
                                     pango_font_description_get_size (desc) / (double)PANGO_SCALE);

      gtk_label_set_text (GTK_LABEL (font_button->priv->size_label), size);

      g_free (size);
    }

  moo_font_button_label_use_font (font_button);

  pango_font_description_free (desc);
}
