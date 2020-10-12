/* fake */
gulong      g_object_connect                (GObject        *gobj,
                                             const char     *signal,
                                             SignalClosure  *callback);
gulong      g_object_connect_after          (GObject        *gobj,
                                             const char     *signal,
                                             SignalClosure  *callback);
void        g_object_disconnect             (GObject        *gobj,
                                             gulong          handler_id);
void        g_object_signal_handler_block   (GObject        *gobj,
                                             gulong          handler_id);
void        g_object_signal_handler_unblock (GObject        *gobj,
                                             gulong          handler_id);

/* real */
GFile *                  g_file_new_for_path            (const char *path);
GFile *                  g_file_new_for_uri             (const char *uri);
GFile *                  g_file_new_for_commandline_arg (const char *arg);
GFile *                  g_file_parse_name              (const char *parse_name);
GFile *                  g_file_dup                     (GFile *file);
guint               g_file_hash                         (GFile *file);
gboolean            g_file_equal                        (GFile *file1,
                                                         GFile *file2);
char *                   g_file_get_basename            (GFile *file);
char *                   g_file_get_path                (GFile *file);
char *                   g_file_get_uri                 (GFile *file);
char *                   g_file_get_parse_name          (GFile *file);
GFile *                  g_file_get_parent              (GFile *file);
gboolean            g_file_has_parent                   (GFile *file,
                                                         GFile *parent);
GFile *                  g_file_get_child               (GFile *file,
                                                         const char *name);
GFile *                  g_file_get_child_for_display_name
                                                        (GFile *file,
                                                         const char *display_name,
                                                         GError **error);
gboolean            g_file_has_prefix                   (GFile *file,
                                                         GFile *prefix);
char *                   g_file_get_relative_path       (GFile *parent,
                                                         GFile *descendant);
GFile *                  g_file_resolve_relative_path   (GFile *file,
                                                         const char *relative_path);
gboolean            g_file_is_native                    (GFile *file);
gboolean            g_file_has_uri_scheme               (GFile *file,
                                                         const char *uri_scheme);
char *                   g_file_get_uri_scheme          (GFile *file);


GtkTextBuffer *     gtk_text_iter_get_buffer            (const GtkTextIter *iter);
GtkTextIter *       gtk_text_iter_copy                  (const GtkTextIter *iter);
void                gtk_text_iter_free                  (GtkTextIter *iter);
gint                gtk_text_iter_get_offset            (const GtkTextIter *iter);
gint                gtk_text_iter_get_line              (const GtkTextIter *iter);
gint                gtk_text_iter_get_line_offset       (const GtkTextIter *iter);
gint                gtk_text_iter_get_line_index        (const GtkTextIter *iter);
gint                gtk_text_iter_get_visible_line_index
                                                        (const GtkTextIter *iter);
gint                gtk_text_iter_get_visible_line_offset
                                                        (const GtkTextIter *iter);
gunichar            gtk_text_iter_get_char              (const GtkTextIter *iter);
gchar *             gtk_text_iter_get_slice             (const GtkTextIter *start,
                                                         const GtkTextIter *end);
gchar *             gtk_text_iter_get_text              (const GtkTextIter *start,
                                                         const GtkTextIter *end);
gchar *             gtk_text_iter_get_visible_slice     (const GtkTextIter *start,
                                                         const GtkTextIter *end);
gchar *             gtk_text_iter_get_visible_text      (const GtkTextIter *start,
                                                         const GtkTextIter *end);
GdkPixbuf*          gtk_text_iter_get_pixbuf            (const GtkTextIter *iter);
GSList *            gtk_text_iter_get_marks             (const GtkTextIter *iter);
GSList *            gtk_text_iter_get_toggled_tags      (const GtkTextIter *iter,
                                                         gboolean toggled_on);
GtkTextChildAnchor* gtk_text_iter_get_child_anchor      (const GtkTextIter *iter);
gboolean            gtk_text_iter_begins_tag            (const GtkTextIter *iter,
                                                         GtkTextTag *tag);
gboolean            gtk_text_iter_ends_tag              (const GtkTextIter *iter,
                                                         GtkTextTag *tag);
gboolean            gtk_text_iter_toggles_tag           (const GtkTextIter *iter,
                                                         GtkTextTag *tag);
gboolean            gtk_text_iter_has_tag               (const GtkTextIter *iter,
                                                         GtkTextTag *tag);
GSList *            gtk_text_iter_get_tags              (const GtkTextIter *iter);
gboolean            gtk_text_iter_editable              (const GtkTextIter *iter,
                                                         gboolean default_setting);
gboolean            gtk_text_iter_can_insert            (const GtkTextIter *iter,
                                                         gboolean default_editability);
gboolean            gtk_text_iter_starts_word           (const GtkTextIter *iter);
gboolean            gtk_text_iter_ends_word             (const GtkTextIter *iter);
gboolean            gtk_text_iter_inside_word           (const GtkTextIter *iter);
gboolean            gtk_text_iter_starts_line           (const GtkTextIter *iter);
gboolean            gtk_text_iter_ends_line             (const GtkTextIter *iter);
gboolean            gtk_text_iter_starts_sentence       (const GtkTextIter *iter);
gboolean            gtk_text_iter_ends_sentence         (const GtkTextIter *iter);
gboolean            gtk_text_iter_inside_sentence       (const GtkTextIter *iter);
gboolean            gtk_text_iter_is_cursor_position    (const GtkTextIter *iter);
gint                gtk_text_iter_get_chars_in_line     (const GtkTextIter *iter);
gint                gtk_text_iter_get_bytes_in_line     (const GtkTextIter *iter);
gboolean            gtk_text_iter_get_attributes        (const GtkTextIter *iter,
                                                         GtkTextAttributes *values);
PangoLanguage*      gtk_text_iter_get_language          (const GtkTextIter *iter);
gboolean            gtk_text_iter_is_end                (const GtkTextIter *iter);
gboolean            gtk_text_iter_is_start              (const GtkTextIter *iter);
gboolean            gtk_text_iter_forward_char          (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_char         (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_chars         (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_chars        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_line          (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_line         (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_lines         (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_lines        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_word_ends     (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_word_starts  (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_word_end      (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_word_start   (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_cursor_position
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_cursor_position
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_cursor_positions
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_cursor_positions
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_sentence_start
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_sentence_starts
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_sentence_end  (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_sentence_ends (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_visible_word_ends
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_visible_word_starts
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_visible_word_end
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_visible_word_start
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_visible_cursor_position
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_visible_cursor_position
                                                        (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_visible_cursor_positions
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_visible_cursor_positions
                                                        (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_forward_visible_line  (GtkTextIter *iter);
gboolean            gtk_text_iter_backward_visible_line (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_visible_lines (GtkTextIter *iter,
                                                         gint count);
gboolean            gtk_text_iter_backward_visible_lines
                                                        (GtkTextIter *iter,
                                                         gint count);
void                gtk_text_iter_set_offset            (GtkTextIter *iter,
                                                         gint char_offset);
void                gtk_text_iter_set_line              (GtkTextIter *iter,
                                                         gint line_number);
void                gtk_text_iter_set_line_offset       (GtkTextIter *iter,
                                                         gint char_on_line);
void                gtk_text_iter_set_line_index        (GtkTextIter *iter,
                                                         gint byte_on_line);
void                gtk_text_iter_set_visible_line_index
                                                        (GtkTextIter *iter,
                                                         gint byte_on_line);
void                gtk_text_iter_set_visible_line_offset
                                                        (GtkTextIter *iter,
                                                         gint char_on_line);
void                gtk_text_iter_forward_to_end        (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_to_line_end   (GtkTextIter *iter);
gboolean            gtk_text_iter_forward_to_tag_toggle (GtkTextIter *iter,
                                                         GtkTextTag *tag);
gboolean            gtk_text_iter_backward_to_tag_toggle
                                                        (GtkTextIter *iter,
                                                         GtkTextTag *tag);
gboolean            (*GtkTextCharPredicate)             (gunichar ch,
                                                         gpointer user_data);
gboolean            gtk_text_iter_forward_find_char     (GtkTextIter *iter,
                                                         GtkTextCharPredicate pred,
                                                         gpointer user_data,
                                                         const GtkTextIter *limit);
gboolean            gtk_text_iter_backward_find_char    (GtkTextIter *iter,
                                                         GtkTextCharPredicate pred,
                                                         gpointer user_data,
                                                         const GtkTextIter *limit);
enum                GtkTextSearchFlags;
gboolean            gtk_text_iter_forward_search        (const GtkTextIter *iter,
                                                         const gchar *str,
                                                         GtkTextSearchFlags flags,
                                                         GtkTextIter *match_start,
                                                         GtkTextIter *match_end,
                                                         const GtkTextIter *limit);
gboolean            gtk_text_iter_backward_search       (const GtkTextIter *iter,
                                                         const gchar *str,
                                                         GtkTextSearchFlags flags,
                                                         GtkTextIter *match_start,
                                                         GtkTextIter *match_end,
                                                         const GtkTextIter *limit);
gboolean            gtk_text_iter_equal                 (const GtkTextIter *lhs,
                                                         const GtkTextIter *rhs);
gint                gtk_text_iter_compare               (const GtkTextIter *lhs,
                                                         const GtkTextIter *rhs);
gboolean            gtk_text_iter_in_range              (const GtkTextIter *iter,
                                                         const GtkTextIter *start,
                                                         const GtkTextIter *end);
void                gtk_text_iter_order                 (GtkTextIter *first,
                                                         GtkTextIter *second);
