#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "moo-lua-api.h"

#include "moolua/moo-lua-api-util.h"

void moo_test_coverage_record (const char *lang, const char *function);

// methods of MooAction

// methods of MooActionCollection

// methods of MooApp

static int
cfunc_MooApp_get_editor (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_app_get_editor");
#endif
    MooLuaCurrentFunc cur_func ("MooApp.get_editor");
    MooApp *self = (MooApp*) pself;
    gpointer ret = moo_app_get_editor (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooApp_quit (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_app_quit");
#endif
    MooLuaCurrentFunc cur_func ("MooApp.quit");
    MooApp *self = (MooApp*) pself;
    gboolean ret = moo_app_quit (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooApp_instance (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_app_instance");
#endif
    MooLuaCurrentFunc cur_func ("MooApp.instance");
    gpointer ret = moo_app_instance ();
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

// methods of MooBigPaned

// methods of MooCmdView

static int
cfunc_MooCmdView_run_command (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_cmd_view_run_command");
#endif
    MooLuaCurrentFunc cur_func ("MooCmdView.run_command");
    MooCmdView *self = (MooCmdView*) pself;
    const char* arg0 = moo_lua_get_arg_filename (L, first_arg + 0, "cmd", FALSE);
    const char* arg1 = moo_lua_get_arg_filename_opt (L, first_arg + 1, "working_dir", NULL, TRUE);
    const char* arg2 = moo_lua_get_arg_utf8_opt (L, first_arg + 2, "job_name", NULL, TRUE);
    gboolean ret = moo_cmd_view_run_command (self, arg0, arg1, arg2);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooCmdView_set_filter_by_id (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_cmd_view_set_filter_by_id");
#endif
    MooLuaCurrentFunc cur_func ("MooCmdView.set_filter_by_id");
    MooCmdView *self = (MooCmdView*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "id", FALSE);
    moo_cmd_view_set_filter_by_id (self, arg0);
    return 0;
}

static int
cfunc_MooCmdView_write_with_filter (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_cmd_view_write_with_filter");
#endif
    MooLuaCurrentFunc cur_func ("MooCmdView.write_with_filter");
    MooCmdView *self = (MooCmdView*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "text", FALSE);
    gboolean arg1 = moo_lua_get_arg_bool_opt (L, first_arg + 1, "error", FALSE);
    moo_cmd_view_write_with_filter (self, arg0, arg1);
    return 0;
}

// methods of MooCombo

// methods of MooEdit

static int
cfunc_MooEdit_append_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_append_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.append_text");
    MooEdit *self = (MooEdit*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "text", FALSE);
    moo_edit_append_text (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_begin_non_undoable_action (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_begin_non_undoable_action");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.begin_non_undoable_action");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_begin_non_undoable_action (self);
    return 0;
}

static int
cfunc_MooEdit_begin_user_action (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_begin_user_action");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.begin_user_action");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_begin_user_action (self);
    return 0;
}

static int
cfunc_MooEdit_can_redo (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_can_redo");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.can_redo");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_can_redo (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_can_undo (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_can_undo");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.can_undo");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_can_undo (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_clear (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_clear");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.clear");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_clear (self);
    return 0;
}

static int
cfunc_MooEdit_close (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_close");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.close");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_close (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_comment_selection (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_comment_selection");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.comment_selection");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_comment_selection (self);
    return 0;
}

static int
cfunc_MooEdit_copy (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_copy");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.copy");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_copy (self);
    return 0;
}

static int
cfunc_MooEdit_cut (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_cut");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.cut");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_cut (self);
    return 0;
}

static int
cfunc_MooEdit_delete_selected_lines (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_delete_selected_lines");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.delete_selected_lines");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_delete_selected_lines (self);
    return 0;
}

static int
cfunc_MooEdit_delete_selected_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_delete_selected_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.delete_selected_text");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_delete_selected_text (self);
    return 0;
}

static int
cfunc_MooEdit_delete_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_delete_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.delete_text");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "start", moo_edit_get_buffer (self), &arg0_iter);
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = &arg1_iter;
    moo_lua_get_arg_iter (L, first_arg + 1, "end", moo_edit_get_buffer (self), &arg1_iter);
    moo_edit_delete_text (self, arg0, arg1);
    return 0;
}

static int
cfunc_MooEdit_end_non_undoable_action (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_end_non_undoable_action");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.end_non_undoable_action");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_end_non_undoable_action (self);
    return 0;
}

static int
cfunc_MooEdit_end_user_action (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_end_user_action");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.end_user_action");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_end_user_action (self);
    return 0;
}

static int
cfunc_MooEdit_get_buffer (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_buffer");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_buffer");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_buffer (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEdit_get_char_at_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_char_at_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_char_at_pos");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "pos", moo_edit_get_buffer (self), &arg0_iter);
    gunichar ret = moo_edit_get_char_at_pos (self, arg0);
    return moo_lua_push_gunichar (L, ret);
}

static int
cfunc_MooEdit_get_char_count (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_char_count");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_char_count");
    MooEdit *self = (MooEdit*) pself;
    int ret = moo_edit_get_char_count (self);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_MooEdit_get_clean (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_clean");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_clean");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_get_clean (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_get_cursor_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_cursor_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_cursor_pos");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_cursor_pos (self);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_display_basename (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_display_basename");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_display_basename");
    MooEdit *self = (MooEdit*) pself;
    const char *ret = moo_edit_get_display_basename (self);
    return moo_lua_push_utf8_copy (L, ret);
}

static int
cfunc_MooEdit_get_display_name (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_display_name");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_display_name");
    MooEdit *self = (MooEdit*) pself;
    const char *ret = moo_edit_get_display_name (self);
    return moo_lua_push_utf8_copy (L, ret);
}

static int
cfunc_MooEdit_get_editor (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_editor");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_editor");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_editor (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEdit_get_encoding (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_encoding");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_encoding");
    MooEdit *self = (MooEdit*) pself;
    const char *ret = moo_edit_get_encoding (self);
    return moo_lua_push_utf8_copy (L, ret);
}

static int
cfunc_MooEdit_get_end_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_end_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_end_pos");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_end_pos (self);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_file (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_file");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_file");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_file (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooEdit_get_filename (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_filename");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_filename");
    MooEdit *self = (MooEdit*) pself;
    char *ret = moo_edit_get_filename (self);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_MooEdit_get_lang_id (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_lang_id");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_lang_id");
    MooEdit *self = (MooEdit*) pself;
    char *ret = moo_edit_get_lang_id (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooEdit_get_line_at_cursor (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_line_at_cursor");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_line_at_cursor");
    MooEdit *self = (MooEdit*) pself;
    int ret = moo_edit_get_line_at_cursor (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_MooEdit_get_line_at_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_line_at_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_line_at_pos");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "pos", moo_edit_get_buffer (self), &arg0_iter);
    int ret = moo_edit_get_line_at_pos (self, arg0);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_MooEdit_get_line_count (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_line_count");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_line_count");
    MooEdit *self = (MooEdit*) pself;
    int ret = moo_edit_get_line_count (self);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_MooEdit_get_line_end_type (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_line_end_type");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_line_end_type");
    MooEdit *self = (MooEdit*) pself;
    MooLineEndType ret = moo_edit_get_line_end_type (self);
    return moo_lua_push_int (L, ret);
}

static int
cfunc_MooEdit_get_line_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_line_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_line_text");
    MooEdit *self = (MooEdit*) pself;
    int arg0 = moo_lua_get_arg_index_opt (L, first_arg + 0, "line", -1);
    char *ret = moo_edit_get_line_text (self, arg0);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooEdit_get_line_text_at_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_line_text_at_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_line_text_at_pos");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "pos", moo_edit_get_buffer (self), &arg0_iter);
    char *ret = moo_edit_get_line_text_at_pos (self, arg0);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooEdit_get_n_views (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_n_views");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_n_views");
    MooEdit *self = (MooEdit*) pself;
    int ret = moo_edit_get_n_views (self);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_MooEdit_get_pos_at_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_pos_at_line");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_pos_at_line");
    MooEdit *self = (MooEdit*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "line");
    gpointer ret = moo_edit_get_pos_at_line (self, arg0);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_pos_at_line_end (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_pos_at_line_end");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_pos_at_line_end");
    MooEdit *self = (MooEdit*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "line");
    gpointer ret = moo_edit_get_pos_at_line_end (self, arg0);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_selected_lines (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_selected_lines");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_selected_lines");
    MooEdit *self = (MooEdit*) pself;
    char **ret = moo_edit_get_selected_lines (self);
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_MooEdit_get_selected_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_selected_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_selected_text");
    MooEdit *self = (MooEdit*) pself;
    char *ret = moo_edit_get_selected_text (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooEdit_get_selection_end_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_selection_end_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_selection_end_pos");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_selection_end_pos (self);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_selection_start_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_selection_start_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_selection_start_pos");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_selection_start_pos (self);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_start_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_start_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_start_pos");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_start_pos (self);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_MooEdit_get_status (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_status");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_status");
    MooEdit *self = (MooEdit*) pself;
    MooEditStatus ret = moo_edit_get_status (self);
    return moo_lua_push_int (L, ret);
}

static int
cfunc_MooEdit_get_tab (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_tab");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_tab");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_tab (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEdit_get_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_text");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = moo_lua_get_arg_iter_opt (L, first_arg + 0, "start", moo_edit_get_buffer (self), &arg0_iter) ? &arg0_iter : NULL;
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = moo_lua_get_arg_iter_opt (L, first_arg + 1, "end", moo_edit_get_buffer (self), &arg1_iter) ? &arg1_iter : NULL;
    char *ret = moo_edit_get_text (self, arg0, arg1);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooEdit_get_uri (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_uri");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_uri");
    MooEdit *self = (MooEdit*) pself;
    char *ret = moo_edit_get_uri (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooEdit_get_view (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_view");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_view");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_view (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEdit_get_views (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_views");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_views");
    MooEdit *self = (MooEdit*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_edit_get_views (self);
    return moo_lua_push_object_array (L, ret, TRUE);
}

static int
cfunc_MooEdit_get_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_get_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.get_window");
    MooEdit *self = (MooEdit*) pself;
    gpointer ret = moo_edit_get_window (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEdit_has_selection (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_has_selection");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.has_selection");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_has_selection (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_insert_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_insert_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.insert_text");
    MooEdit *self = (MooEdit*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "text", FALSE);
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = moo_lua_get_arg_iter_opt (L, first_arg + 1, "where", moo_edit_get_buffer (self), &arg1_iter) ? &arg1_iter : NULL;
    moo_edit_insert_text (self, arg0, arg1);
    return 0;
}

static int
cfunc_MooEdit_is_empty (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_is_empty");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.is_empty");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_is_empty (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_is_modified (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_is_modified");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.is_modified");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_is_modified (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_is_untitled (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_is_untitled");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.is_untitled");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_is_untitled (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_paste (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_paste");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.paste");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_paste (self);
    return 0;
}

static int
cfunc_MooEdit_redo (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_redo");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.redo");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_redo (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEdit_reload (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_reload");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.reload");
    MooEdit *self = (MooEdit*) pself;
    MooReloadInfo *arg0 = (MooReloadInfo*) moo_lua_get_arg_instance_opt (L, first_arg + 0, "info", MOO_TYPE_RELOAD_INFO, TRUE);
    GError *error = NULL;
    gboolean ret = moo_edit_reload (self, arg0, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEdit_replace_selected_lines (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_replace_selected_lines");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.replace_selected_lines");
    MooEdit *self = (MooEdit*) pself;
    char **arg0 = moo_lua_get_arg_strv (L, first_arg + 0, "replacement", TRUE);
    moo_edit_replace_selected_lines (self, arg0);
    g_strfreev (arg0);
    return 0;
}

static int
cfunc_MooEdit_replace_selected_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_replace_selected_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.replace_selected_text");
    MooEdit *self = (MooEdit*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "replacement", FALSE);
    moo_edit_replace_selected_text (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_replace_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_replace_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.replace_text");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "start", moo_edit_get_buffer (self), &arg0_iter);
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = &arg1_iter;
    moo_lua_get_arg_iter (L, first_arg + 1, "end", moo_edit_get_buffer (self), &arg1_iter);
    const char* arg2 = moo_lua_get_arg_utf8 (L, first_arg + 2, "text", FALSE);
    moo_edit_replace_text (self, arg0, arg1, arg2);
    return 0;
}

static int
cfunc_MooEdit_save (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_save");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.save");
    MooEdit *self = (MooEdit*) pself;
    GError *error = NULL;
    gboolean ret = moo_edit_save (self, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEdit_save_as (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_save_as");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.save_as");
    MooEdit *self = (MooEdit*) pself;
    MooSaveInfo *arg0 = (MooSaveInfo*) moo_lua_get_arg_instance (L, first_arg + 0, "info", MOO_TYPE_SAVE_INFO, TRUE);
    GError *error = NULL;
    gboolean ret = moo_edit_save_as (self, arg0, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEdit_save_copy (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_save_copy");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.save_copy");
    MooEdit *self = (MooEdit*) pself;
    MooSaveInfo *arg0 = (MooSaveInfo*) moo_lua_get_arg_instance (L, first_arg + 0, "info", MOO_TYPE_SAVE_INFO, FALSE);
    GError *error = NULL;
    gboolean ret = moo_edit_save_copy (self, arg0, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEdit_select_all (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_select_all");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.select_all");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_select_all (self);
    return 0;
}

static int
cfunc_MooEdit_select_lines (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_select_lines");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.select_lines");
    MooEdit *self = (MooEdit*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "start");
    int arg1 = moo_lua_get_arg_index_opt (L, first_arg + 1, "end", -1);
    moo_edit_select_lines (self, arg0, arg1);
    return 0;
}

static int
cfunc_MooEdit_select_lines_at_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_select_lines_at_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.select_lines_at_pos");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "start", moo_edit_get_buffer (self), &arg0_iter);
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = moo_lua_get_arg_iter_opt (L, first_arg + 1, "end", moo_edit_get_buffer (self), &arg1_iter) ? &arg1_iter : NULL;
    moo_edit_select_lines_at_pos (self, arg0, arg1);
    return 0;
}

static int
cfunc_MooEdit_select_range (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_select_range");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.select_range");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "start", moo_edit_get_buffer (self), &arg0_iter);
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = &arg1_iter;
    moo_lua_get_arg_iter (L, first_arg + 1, "end", moo_edit_get_buffer (self), &arg1_iter);
    moo_edit_select_range (self, arg0, arg1);
    return 0;
}

static int
cfunc_MooEdit_set_clean (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_set_clean");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.set_clean");
    MooEdit *self = (MooEdit*) pself;
    gboolean arg0 = moo_lua_get_arg_bool (L, first_arg + 0, "clean");
    moo_edit_set_clean (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_set_cursor_pos (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_set_cursor_pos");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.set_cursor_pos");
    MooEdit *self = (MooEdit*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "pos", moo_edit_get_buffer (self), &arg0_iter);
    moo_edit_set_cursor_pos (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_set_encoding (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_set_encoding");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.set_encoding");
    MooEdit *self = (MooEdit*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "encoding", FALSE);
    moo_edit_set_encoding (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_set_line_end_type (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_set_line_end_type");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.set_line_end_type");
    MooEdit *self = (MooEdit*) pself;
    MooLineEndType arg0 = (MooLineEndType) moo_lua_get_arg_enum (L, first_arg + 0, "le", MOO_TYPE_LINE_END_TYPE);
    moo_edit_set_line_end_type (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_set_modified (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_set_modified");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.set_modified");
    MooEdit *self = (MooEdit*) pself;
    gboolean arg0 = moo_lua_get_arg_bool (L, first_arg + 0, "modified");
    moo_edit_set_modified (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_set_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_set_text");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.set_text");
    MooEdit *self = (MooEdit*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "text", FALSE);
    moo_edit_set_text (self, arg0);
    return 0;
}

static int
cfunc_MooEdit_uncomment_selection (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_uncomment_selection");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.uncomment_selection");
    MooEdit *self = (MooEdit*) pself;
    moo_edit_uncomment_selection (self);
    return 0;
}

static int
cfunc_MooEdit_undo (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_undo");
#endif
    MooLuaCurrentFunc cur_func ("MooEdit.undo");
    MooEdit *self = (MooEdit*) pself;
    gboolean ret = moo_edit_undo (self);
    return moo_lua_push_bool (L, ret);
}

// methods of MooEditAction

// methods of MooEditBookmark

// methods of MooEditTab

static int
cfunc_MooEditTab_get_active_view (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_tab_get_active_view");
#endif
    MooLuaCurrentFunc cur_func ("MooEditTab.get_active_view");
    MooEditTab *self = (MooEditTab*) pself;
    gpointer ret = moo_edit_tab_get_active_view (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditTab_get_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_tab_get_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditTab.get_doc");
    MooEditTab *self = (MooEditTab*) pself;
    gpointer ret = moo_edit_tab_get_doc (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditTab_get_views (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_tab_get_views");
#endif
    MooLuaCurrentFunc cur_func ("MooEditTab.get_views");
    MooEditTab *self = (MooEditTab*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_edit_tab_get_views (self);
    return moo_lua_push_object_array (L, ret, FALSE);
}

static int
cfunc_MooEditTab_get_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_tab_get_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEditTab.get_window");
    MooEditTab *self = (MooEditTab*) pself;
    gpointer ret = moo_edit_tab_get_window (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

// methods of MooEditView

static int
cfunc_MooEditView_get_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_view_get_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditView.get_doc");
    MooEditView *self = (MooEditView*) pself;
    gpointer ret = moo_edit_view_get_doc (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditView_get_editor (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_view_get_editor");
#endif
    MooLuaCurrentFunc cur_func ("MooEditView.get_editor");
    MooEditView *self = (MooEditView*) pself;
    gpointer ret = moo_edit_view_get_editor (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditView_get_tab (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_view_get_tab");
#endif
    MooLuaCurrentFunc cur_func ("MooEditView.get_tab");
    MooEditView *self = (MooEditView*) pself;
    gpointer ret = moo_edit_view_get_tab (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditView_get_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_view_get_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEditView.get_window");
    MooEditView *self = (MooEditView*) pself;
    gpointer ret = moo_edit_view_get_window (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

// methods of MooEditWindow

static int
cfunc_MooEditWindow_close (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_close");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.close");
    MooEditWindow *self = (MooEditWindow*) pself;
    gboolean ret = moo_edit_window_close (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEditWindow_close_all (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_close_all");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.close_all");
    MooEditWindow *self = (MooEditWindow*) pself;
    gboolean ret = moo_edit_window_close_all (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEditWindow_get_active_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_active_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_active_doc");
    MooEditWindow *self = (MooEditWindow*) pself;
    gpointer ret = moo_edit_window_get_active_doc (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditWindow_get_active_tab (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_active_tab");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_active_tab");
    MooEditWindow *self = (MooEditWindow*) pself;
    gpointer ret = moo_edit_window_get_active_tab (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditWindow_get_active_view (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_active_view");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_active_view");
    MooEditWindow *self = (MooEditWindow*) pself;
    gpointer ret = moo_edit_window_get_active_view (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditWindow_get_docs (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_docs");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_docs");
    MooEditWindow *self = (MooEditWindow*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_edit_window_get_docs (self);
    return moo_lua_push_object_array (L, ret, FALSE);
}

static int
cfunc_MooEditWindow_get_editor (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_editor");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_editor");
    MooEditWindow *self = (MooEditWindow*) pself;
    gpointer ret = moo_edit_window_get_editor (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditWindow_get_n_tabs (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_n_tabs");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_n_tabs");
    MooEditWindow *self = (MooEditWindow*) pself;
    int ret = moo_edit_window_get_n_tabs (self);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_MooEditWindow_get_output (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_output");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_output");
    MooEditWindow *self = (MooEditWindow*) pself;
    gpointer ret = moo_edit_window_get_output (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditWindow_get_tabs (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_tabs");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_tabs");
    MooEditWindow *self = (MooEditWindow*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_edit_window_get_tabs (self);
    return moo_lua_push_object_array (L, ret, FALSE);
}

static int
cfunc_MooEditWindow_get_views (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_get_views");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.get_views");
    MooEditWindow *self = (MooEditWindow*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_edit_window_get_views (self);
    return moo_lua_push_object_array (L, ret, FALSE);
}

static int
cfunc_MooEditWindow_present_output (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_present_output");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.present_output");
    MooEditWindow *self = (MooEditWindow*) pself;
    moo_edit_window_present_output (self);
    return 0;
}

static int
cfunc_MooEditWindow_set_active_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_set_active_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.set_active_doc");
    MooEditWindow *self = (MooEditWindow*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    moo_edit_window_set_active_doc (self, arg0);
    return 0;
}

static int
cfunc_MooEditWindow_set_active_tab (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_set_active_tab");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.set_active_tab");
    MooEditWindow *self = (MooEditWindow*) pself;
    MooEditTab *arg0 = (MooEditTab*) moo_lua_get_arg_instance (L, first_arg + 0, "tab", MOO_TYPE_EDIT_TAB, FALSE);
    moo_edit_window_set_active_tab (self, arg0);
    return 0;
}

static int
cfunc_MooEditWindow_set_active_view (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_edit_window_set_active_view");
#endif
    MooLuaCurrentFunc cur_func ("MooEditWindow.set_active_view");
    MooEditWindow *self = (MooEditWindow*) pself;
    MooEditView *arg0 = (MooEditView*) moo_lua_get_arg_instance (L, first_arg + 0, "view", MOO_TYPE_EDIT_VIEW, FALSE);
    moo_edit_window_set_active_view (self, arg0);
    return 0;
}

// methods of MooEditor

static int
cfunc_MooEditor_close_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_close_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.close_doc");
    MooEditor *self = (MooEditor*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    gboolean ret = moo_editor_close_doc (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEditor_close_docs (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_close_docs");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.close_docs");
    MooEditor *self = (MooEditor*) pself;
    MooEditArray* arg0 = (MooEditArray*) moo_lua_get_arg_object_array (L, first_arg + 0, "docs", MOO_TYPE_EDIT);
    gboolean ret = moo_editor_close_docs (self, arg0);
    moo_object_array_free ((MooObjectArray*) arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEditor_close_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_close_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.close_window");
    MooEditor *self = (MooEditor*) pself;
    MooEditWindow *arg0 = (MooEditWindow*) moo_lua_get_arg_instance (L, first_arg + 0, "window", MOO_TYPE_EDIT_WINDOW, FALSE);
    gboolean ret = moo_editor_close_window (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_MooEditor_get_active_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_active_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_active_doc");
    MooEditor *self = (MooEditor*) pself;
    gpointer ret = moo_editor_get_active_doc (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_active_view (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_active_view");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_active_view");
    MooEditor *self = (MooEditor*) pself;
    gpointer ret = moo_editor_get_active_view (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_active_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_active_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_active_window");
    MooEditor *self = (MooEditor*) pself;
    gpointer ret = moo_editor_get_active_window (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_doc");
    MooEditor *self = (MooEditor*) pself;
    const char* arg0 = moo_lua_get_arg_filename (L, first_arg + 0, "filename", FALSE);
    gpointer ret = moo_editor_get_doc (self, arg0);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_doc_for_file (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_doc_for_file");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_doc_for_file");
    MooEditor *self = (MooEditor*) pself;
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance (L, first_arg + 0, "file", G_TYPE_FILE, FALSE);
    gpointer ret = moo_editor_get_doc_for_file (self, arg0);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_doc_for_uri (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_doc_for_uri");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_doc_for_uri");
    MooEditor *self = (MooEditor*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "uri", FALSE);
    gpointer ret = moo_editor_get_doc_for_uri (self, arg0);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_doc_ui_xml (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_doc_ui_xml");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_doc_ui_xml");
    MooEditor *self = (MooEditor*) pself;
    gpointer ret = moo_editor_get_doc_ui_xml (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_docs (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_docs");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_docs");
    MooEditor *self = (MooEditor*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_editor_get_docs (self);
    return moo_lua_push_object_array (L, ret, FALSE);
}

static int
cfunc_MooEditor_get_ui_xml (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_ui_xml");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_ui_xml");
    MooEditor *self = (MooEditor*) pself;
    gpointer ret = moo_editor_get_ui_xml (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_get_windows (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_get_windows");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.get_windows");
    MooEditor *self = (MooEditor*) pself;
    MooObjectArray *ret = (MooObjectArray*) moo_editor_get_windows (self);
    return moo_lua_push_object_array (L, ret, FALSE);
}

static int
cfunc_MooEditor_new_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_new_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.new_doc");
    MooEditor *self = (MooEditor*) pself;
    MooEditWindow *arg0 = (MooEditWindow*) moo_lua_get_arg_instance_opt (L, first_arg + 0, "window", MOO_TYPE_EDIT_WINDOW, TRUE);
    gpointer ret = moo_editor_new_doc (self, arg0);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_new_file (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_new_file");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.new_file");
    MooEditor *self = (MooEditor*) pself;
    MooOpenInfo *arg0 = (MooOpenInfo*) moo_lua_get_arg_instance (L, first_arg + 0, "info", MOO_TYPE_OPEN_INFO, FALSE);
    GtkWidget *arg1 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, first_arg + 1, "parent", GTK_TYPE_WIDGET, TRUE);
    GError *error = NULL;
    gpointer ret = moo_editor_new_file (self, arg0, arg1, &error);
    int ret_lua = moo_lua_push_object (L, (GObject*) ret, TRUE);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_new_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_new_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.new_window");
    MooEditor *self = (MooEditor*) pself;
    gpointer ret = moo_editor_new_window (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_open_file (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_open_file");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.open_file");
    MooEditor *self = (MooEditor*) pself;
    MooOpenInfo *arg0 = (MooOpenInfo*) moo_lua_get_arg_instance (L, first_arg + 0, "info", MOO_TYPE_OPEN_INFO, FALSE);
    GtkWidget *arg1 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, first_arg + 1, "parent", GTK_TYPE_WIDGET, TRUE);
    GError *error = NULL;
    gpointer ret = moo_editor_open_file (self, arg0, arg1, &error);
    int ret_lua = moo_lua_push_object (L, (GObject*) ret, TRUE);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_open_files (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_open_files");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.open_files");
    MooEditor *self = (MooEditor*) pself;
    MooOpenInfoArray* arg0 = (MooOpenInfoArray*) moo_lua_get_arg_object_array (L, first_arg + 0, "files", MOO_TYPE_OPEN_INFO);
    GtkWidget *arg1 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, first_arg + 1, "parent", GTK_TYPE_WIDGET, TRUE);
    GError *error = NULL;
    gboolean ret = moo_editor_open_files (self, arg0, arg1, &error);
    moo_object_array_free ((MooObjectArray*) arg0);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_open_path (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_open_path");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.open_path");
    bool kwargs = moo_lua_check_kwargs (L, first_arg);
    MooEditor *self = (MooEditor*) pself;
    int arg_idx0 = first_arg + 0;
    if (kwargs)
        arg_idx0 = moo_lua_get_kwarg (L, first_arg, 0 + 1, "path");
    if (arg_idx0 == MOO_NONEXISTING_INDEX)
        moo_lua_arg_error (L, 0, "path", "parameter 'path' missing");
    const char* arg0 = moo_lua_get_arg_filename (L, arg_idx0, "path", FALSE);
    int arg_idx1 = first_arg + 1;
    if (kwargs)
        arg_idx1 = moo_lua_get_kwarg (L, first_arg, 1 + 1, "encoding");
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, arg_idx1, "encoding", NULL, TRUE);
    int arg_idx2 = first_arg + 2;
    if (kwargs)
        arg_idx2 = moo_lua_get_kwarg (L, first_arg, 2 + 1, "line");
    int arg2 = moo_lua_get_arg_int_opt (L, arg_idx2, "line", -1);
    int arg_idx3 = first_arg + 3;
    if (kwargs)
        arg_idx3 = moo_lua_get_kwarg (L, first_arg, 3 + 1, "window");
    MooEditWindow *arg3 = (MooEditWindow*) moo_lua_get_arg_instance_opt (L, arg_idx3, "window", MOO_TYPE_EDIT_WINDOW, TRUE);
    gpointer ret = moo_editor_open_path (self, arg0, arg1, arg2, arg3);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_open_uri (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_open_uri");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.open_uri");
    bool kwargs = moo_lua_check_kwargs (L, first_arg);
    MooEditor *self = (MooEditor*) pself;
    int arg_idx0 = first_arg + 0;
    if (kwargs)
        arg_idx0 = moo_lua_get_kwarg (L, first_arg, 0 + 1, "uri");
    if (arg_idx0 == MOO_NONEXISTING_INDEX)
        moo_lua_arg_error (L, 0, "uri", "parameter 'uri' missing");
    const char* arg0 = moo_lua_get_arg_utf8 (L, arg_idx0, "uri", FALSE);
    int arg_idx1 = first_arg + 1;
    if (kwargs)
        arg_idx1 = moo_lua_get_kwarg (L, first_arg, 1 + 1, "encoding");
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, arg_idx1, "encoding", NULL, TRUE);
    int arg_idx2 = first_arg + 2;
    if (kwargs)
        arg_idx2 = moo_lua_get_kwarg (L, first_arg, 2 + 1, "line");
    int arg2 = moo_lua_get_arg_int_opt (L, arg_idx2, "line", -1);
    int arg_idx3 = first_arg + 3;
    if (kwargs)
        arg_idx3 = moo_lua_get_kwarg (L, first_arg, 3 + 1, "window");
    MooEditWindow *arg3 = (MooEditWindow*) moo_lua_get_arg_instance_opt (L, arg_idx3, "window", MOO_TYPE_EDIT_WINDOW, TRUE);
    gpointer ret = moo_editor_open_uri (self, arg0, arg1, arg2, arg3);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooEditor_reload (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_reload");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.reload");
    MooEditor *self = (MooEditor*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    MooReloadInfo *arg1 = (MooReloadInfo*) moo_lua_get_arg_instance_opt (L, first_arg + 1, "info", MOO_TYPE_RELOAD_INFO, TRUE);
    GError *error = NULL;
    gboolean ret = moo_editor_reload (self, arg0, arg1, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_save (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_save");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.save");
    MooEditor *self = (MooEditor*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    GError *error = NULL;
    gboolean ret = moo_editor_save (self, arg0, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_save_as (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_save_as");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.save_as");
    MooEditor *self = (MooEditor*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    MooSaveInfo *arg1 = (MooSaveInfo*) moo_lua_get_arg_instance_opt (L, first_arg + 1, "info", MOO_TYPE_SAVE_INFO, TRUE);
    GError *error = NULL;
    gboolean ret = moo_editor_save_as (self, arg0, arg1, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_save_copy (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_save_copy");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.save_copy");
    MooEditor *self = (MooEditor*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    MooSaveInfo *arg1 = (MooSaveInfo*) moo_lua_get_arg_instance (L, first_arg + 1, "info", MOO_TYPE_SAVE_INFO, FALSE);
    GError *error = NULL;
    gboolean ret = moo_editor_save_copy (self, arg0, arg1, &error);
    int ret_lua = moo_lua_push_bool (L, ret);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_MooEditor_set_active_doc (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_set_active_doc");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.set_active_doc");
    MooEditor *self = (MooEditor*) pself;
    MooEdit *arg0 = (MooEdit*) moo_lua_get_arg_instance (L, first_arg + 0, "doc", MOO_TYPE_EDIT, FALSE);
    moo_editor_set_active_doc (self, arg0);
    return 0;
}

static int
cfunc_MooEditor_set_active_view (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_set_active_view");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.set_active_view");
    MooEditor *self = (MooEditor*) pself;
    MooEditView *arg0 = (MooEditView*) moo_lua_get_arg_instance (L, first_arg + 0, "view", MOO_TYPE_EDIT_VIEW, FALSE);
    moo_editor_set_active_view (self, arg0);
    return 0;
}

static int
cfunc_MooEditor_set_active_window (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_set_active_window");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.set_active_window");
    MooEditor *self = (MooEditor*) pself;
    MooEditWindow *arg0 = (MooEditWindow*) moo_lua_get_arg_instance (L, first_arg + 0, "window", MOO_TYPE_EDIT_WINDOW, FALSE);
    moo_editor_set_active_window (self, arg0);
    return 0;
}

static int
cfunc_MooEditor_set_ui_xml (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_set_ui_xml");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.set_ui_xml");
    MooEditor *self = (MooEditor*) pself;
    MooUiXml *arg0 = (MooUiXml*) moo_lua_get_arg_instance (L, first_arg + 0, "xml", MOO_TYPE_UI_XML, FALSE);
    moo_editor_set_ui_xml (self, arg0);
    return 0;
}

static int
cfunc_MooEditor_instance (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_editor_instance");
#endif
    MooLuaCurrentFunc cur_func ("MooEditor.instance");
    gpointer ret = moo_editor_instance ();
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

// methods of MooEntry

// methods of MooFileDialog

// methods of MooGladeXml

// methods of MooHistoryCombo

// methods of MooHistoryList

// methods of MooHistoryMgr

// methods of MooLineMark

// methods of MooLineView

static int
cfunc_MooLineView_clear (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_line_view_clear");
#endif
    MooLuaCurrentFunc cur_func ("MooLineView.clear");
    MooLineView *self = (MooLineView*) pself;
    moo_line_view_clear (self);
    return 0;
}

// methods of MooMenuAction

// methods of MooMenuMgr

// methods of MooMenuToolButton

// methods of MooNotebook

// methods of MooOpenInfo

static int
cfunc_MooOpenInfo_add_flags (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_add_flags");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.add_flags");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    MooOpenFlags arg0 = (MooOpenFlags) moo_lua_get_arg_enum (L, first_arg + 0, "flags", MOO_TYPE_OPEN_FLAGS);
    moo_open_info_add_flags (self, arg0);
    return 0;
}

static int
cfunc_MooOpenInfo_dup (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_dup");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.dup");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    gpointer ret = moo_open_info_dup (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooOpenInfo_get_encoding (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_get_encoding");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.get_encoding");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    const char *ret = moo_open_info_get_encoding (self);
    return moo_lua_push_utf8_copy (L, ret);
}

static int
cfunc_MooOpenInfo_get_file (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_get_file");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.get_file");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    gpointer ret = moo_open_info_get_file (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooOpenInfo_get_filename (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_get_filename");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.get_filename");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    char *ret = moo_open_info_get_filename (self);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_MooOpenInfo_get_flags (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_get_flags");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.get_flags");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    MooOpenFlags ret = moo_open_info_get_flags (self);
    return moo_lua_push_int (L, ret);
}

static int
cfunc_MooOpenInfo_get_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_get_line");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.get_line");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    int ret = moo_open_info_get_line (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_MooOpenInfo_get_uri (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_get_uri");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.get_uri");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    char *ret = moo_open_info_get_uri (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooOpenInfo_set_encoding (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_set_encoding");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.set_encoding");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "encoding", TRUE);
    moo_open_info_set_encoding (self, arg0);
    return 0;
}

static int
cfunc_MooOpenInfo_set_flags (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_set_flags");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.set_flags");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    MooOpenFlags arg0 = (MooOpenFlags) moo_lua_get_arg_enum (L, first_arg + 0, "flags", MOO_TYPE_OPEN_FLAGS);
    moo_open_info_set_flags (self, arg0);
    return 0;
}

static int
cfunc_MooOpenInfo_set_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_set_line");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.set_line");
    MooOpenInfo *self = (MooOpenInfo*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "line");
    moo_open_info_set_line (self, arg0);
    return 0;
}

static int
cfunc_MooOpenInfo_new_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_new_file");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.new_file");
    bool kwargs = moo_lua_check_kwargs (L, 1);
    int arg_idx0 = 1 + 0;
    if (kwargs)
        arg_idx0 = moo_lua_get_kwarg (L, 1, 0 + 1, "file");
    if (arg_idx0 == MOO_NONEXISTING_INDEX)
        moo_lua_arg_error (L, 0, "file", "parameter 'file' missing");
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance (L, arg_idx0, "file", G_TYPE_FILE, FALSE);
    int arg_idx1 = 1 + 1;
    if (kwargs)
        arg_idx1 = moo_lua_get_kwarg (L, 1, 1 + 1, "encoding");
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, arg_idx1, "encoding", NULL, TRUE);
    int arg_idx2 = 1 + 2;
    if (kwargs)
        arg_idx2 = moo_lua_get_kwarg (L, 1, 2 + 1, "line");
    int arg2 = moo_lua_get_arg_index_opt (L, arg_idx2, "line", -1);
    int arg_idx3 = 1 + 3;
    if (kwargs)
        arg_idx3 = moo_lua_get_kwarg (L, 1, 3 + 1, "flags");
    MooOpenFlags arg3 = (MooOpenFlags) moo_lua_get_arg_enum_opt (L, arg_idx3, "flags", MOO_TYPE_OPEN_FLAGS, 0);
    gpointer ret = moo_open_info_new_file (arg0, arg1, arg2, arg3);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooOpenInfo_new_uri (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_new_uri");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.new_uri");
    bool kwargs = moo_lua_check_kwargs (L, 1);
    int arg_idx0 = 1 + 0;
    if (kwargs)
        arg_idx0 = moo_lua_get_kwarg (L, 1, 0 + 1, "uri");
    if (arg_idx0 == MOO_NONEXISTING_INDEX)
        moo_lua_arg_error (L, 0, "uri", "parameter 'uri' missing");
    const char* arg0 = moo_lua_get_arg_utf8 (L, arg_idx0, "uri", FALSE);
    int arg_idx1 = 1 + 1;
    if (kwargs)
        arg_idx1 = moo_lua_get_kwarg (L, 1, 1 + 1, "encoding");
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, arg_idx1, "encoding", NULL, TRUE);
    int arg_idx2 = 1 + 2;
    if (kwargs)
        arg_idx2 = moo_lua_get_kwarg (L, 1, 2 + 1, "line");
    int arg2 = moo_lua_get_arg_index_opt (L, arg_idx2, "line", -1);
    int arg_idx3 = 1 + 3;
    if (kwargs)
        arg_idx3 = moo_lua_get_kwarg (L, 1, 3 + 1, "flags");
    MooOpenFlags arg3 = (MooOpenFlags) moo_lua_get_arg_enum_opt (L, arg_idx3, "flags", MOO_TYPE_OPEN_FLAGS, 0);
    gpointer ret = moo_open_info_new_uri (arg0, arg1, arg2, arg3);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooOpenInfo_new (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_open_info_new");
#endif
    MooLuaCurrentFunc cur_func ("MooOpenInfo.new");
    bool kwargs = moo_lua_check_kwargs (L, 1);
    int arg_idx0 = 1 + 0;
    if (kwargs)
        arg_idx0 = moo_lua_get_kwarg (L, 1, 0 + 1, "path");
    if (arg_idx0 == MOO_NONEXISTING_INDEX)
        moo_lua_arg_error (L, 0, "path", "parameter 'path' missing");
    const char* arg0 = moo_lua_get_arg_filename (L, arg_idx0, "path", FALSE);
    int arg_idx1 = 1 + 1;
    if (kwargs)
        arg_idx1 = moo_lua_get_kwarg (L, 1, 1 + 1, "encoding");
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, arg_idx1, "encoding", NULL, TRUE);
    int arg_idx2 = 1 + 2;
    if (kwargs)
        arg_idx2 = moo_lua_get_kwarg (L, 1, 2 + 1, "line");
    int arg2 = moo_lua_get_arg_index_opt (L, arg_idx2, "line", -1);
    int arg_idx3 = 1 + 3;
    if (kwargs)
        arg_idx3 = moo_lua_get_kwarg (L, 1, 3 + 1, "flags");
    MooOpenFlags arg3 = (MooOpenFlags) moo_lua_get_arg_enum_opt (L, arg_idx3, "flags", MOO_TYPE_OPEN_FLAGS, 0);
    gpointer ret = moo_open_info_new (arg0, arg1, arg2, arg3);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

// methods of MooPrefsDialog

// methods of MooPrefsPage

// methods of MooReloadInfo

static int
cfunc_MooReloadInfo_dup (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_reload_info_dup");
#endif
    MooLuaCurrentFunc cur_func ("MooReloadInfo.dup");
    MooReloadInfo *self = (MooReloadInfo*) pself;
    gpointer ret = moo_reload_info_dup (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooReloadInfo_get_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_reload_info_get_line");
#endif
    MooLuaCurrentFunc cur_func ("MooReloadInfo.get_line");
    MooReloadInfo *self = (MooReloadInfo*) pself;
    int ret = moo_reload_info_get_line (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_MooReloadInfo_set_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_reload_info_set_line");
#endif
    MooLuaCurrentFunc cur_func ("MooReloadInfo.set_line");
    MooReloadInfo *self = (MooReloadInfo*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "line");
    moo_reload_info_set_line (self, arg0);
    return 0;
}

static int
cfunc_MooReloadInfo_new (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_reload_info_new");
#endif
    MooLuaCurrentFunc cur_func ("MooReloadInfo.new");
    const char* arg0 = moo_lua_get_arg_utf8_opt (L, 1 + 0, "encoding", NULL, TRUE);
    int arg1 = moo_lua_get_arg_index_opt (L, 1 + 1, "line", -1);
    gpointer ret = moo_reload_info_new (arg0, arg1);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

// methods of MooSaveInfo

static int
cfunc_MooSaveInfo_dup (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_save_info_dup");
#endif
    MooLuaCurrentFunc cur_func ("MooSaveInfo.dup");
    MooSaveInfo *self = (MooSaveInfo*) pself;
    gpointer ret = moo_save_info_dup (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooSaveInfo_new_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_save_info_new_file");
#endif
    MooLuaCurrentFunc cur_func ("MooSaveInfo.new_file");
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance (L, 1 + 0, "file", G_TYPE_FILE, FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "encoding", NULL, TRUE);
    gpointer ret = moo_save_info_new_file (arg0, arg1);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooSaveInfo_new_uri (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_save_info_new_uri");
#endif
    MooLuaCurrentFunc cur_func ("MooSaveInfo.new_uri");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "uri", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "encoding", NULL, TRUE);
    gpointer ret = moo_save_info_new_uri (arg0, arg1);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_MooSaveInfo_new (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_save_info_new");
#endif
    MooLuaCurrentFunc cur_func ("MooSaveInfo.new");
    const char* arg0 = moo_lua_get_arg_filename (L, 1 + 0, "path", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "encoding", NULL, TRUE);
    gpointer ret = moo_save_info_new (arg0, arg1);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

// methods of MooTextBuffer

// methods of MooTextView

static int
cfunc_MooTextView_set_font_from_string (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_text_view_set_font_from_string");
#endif
    MooLuaCurrentFunc cur_func ("MooTextView.set_font_from_string");
    MooTextView *self = (MooTextView*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "font", FALSE);
    moo_text_view_set_font_from_string (self, arg0);
    return 0;
}

static int
cfunc_MooTextView_set_lang_by_id (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_text_view_set_lang_by_id");
#endif
    MooLuaCurrentFunc cur_func ("MooTextView.set_lang_by_id");
    MooTextView *self = (MooTextView*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "lang_id", FALSE);
    moo_text_view_set_lang_by_id (self, arg0);
    return 0;
}

// methods of MooUiXml

static int
cfunc_MooUiXml_add_item (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_add_item");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.add_item");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    const char* arg1 = moo_lua_get_arg_utf8 (L, first_arg + 1, "parent_path", FALSE);
    const char* arg2 = moo_lua_get_arg_utf8_opt (L, first_arg + 2, "name", NULL, TRUE);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "action", FALSE);
    int arg4 = moo_lua_get_arg_index_opt (L, first_arg + 4, "position", -1);
    gpointer ret = moo_ui_xml_add_item (self, arg0, arg1, arg2, arg3, arg4);
    return moo_lua_push_pointer (L, ret, MOO_TYPE_UI_NODE, TRUE);
}

static int
cfunc_MooUiXml_add_ui_from_string (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_add_ui_from_string");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.add_ui_from_string");
    MooUiXml *self = (MooUiXml*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "buffer", FALSE);
    int arg1 = moo_lua_get_arg_int_opt (L, first_arg + 1, "length", -1);
    moo_ui_xml_add_ui_from_string (self, arg0, arg1);
    return 0;
}

static int
cfunc_MooUiXml_create_widget (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_create_widget");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.create_widget");
    MooUiXml *self = (MooUiXml*) pself;
    MooUiWidgetType arg0 = (MooUiWidgetType) moo_lua_get_arg_enum (L, first_arg + 0, "type", MOO_TYPE_UI_WIDGET_TYPE);
    const char* arg1 = moo_lua_get_arg_utf8 (L, first_arg + 1, "path", FALSE);
    MooActionCollection *arg2 = (MooActionCollection*) moo_lua_get_arg_instance (L, first_arg + 2, "actions", MOO_TYPE_ACTION_COLLECTION, FALSE);
    GtkAccelGroup *arg3 = (GtkAccelGroup*) moo_lua_get_arg_instance (L, first_arg + 3, "accel_group", GTK_TYPE_ACCEL_GROUP, FALSE);
    gpointer ret = moo_ui_xml_create_widget (self, arg0, arg1, arg2, arg3);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooUiXml_find_placeholder (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_find_placeholder");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.find_placeholder");
    MooUiXml *self = (MooUiXml*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "name", FALSE);
    gpointer ret = moo_ui_xml_find_placeholder (self, arg0);
    return moo_lua_push_pointer (L, ret, MOO_TYPE_UI_NODE, TRUE);
}

static int
cfunc_MooUiXml_get_node (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_get_node");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.get_node");
    MooUiXml *self = (MooUiXml*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "path", FALSE);
    gpointer ret = moo_ui_xml_get_node (self, arg0);
    return moo_lua_push_pointer (L, ret, MOO_TYPE_UI_NODE, TRUE);
}

static int
cfunc_MooUiXml_get_widget (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_get_widget");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.get_widget");
    MooUiXml *self = (MooUiXml*) pself;
    GtkWidget *arg0 = (GtkWidget*) moo_lua_get_arg_instance (L, first_arg + 0, "widget", GTK_TYPE_WIDGET, FALSE);
    const char* arg1 = moo_lua_get_arg_utf8 (L, first_arg + 1, "path", FALSE);
    gpointer ret = moo_ui_xml_get_widget (self, arg0, arg1);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_MooUiXml_insert (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_insert");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.insert");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    MooUiNode *arg1 = (MooUiNode*) moo_lua_get_arg_instance (L, first_arg + 1, "parent", MOO_TYPE_UI_NODE, FALSE);
    int arg2 = moo_lua_get_arg_index_opt (L, first_arg + 2, "position", -1);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "markup", FALSE);
    moo_ui_xml_insert (self, arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_MooUiXml_insert_after (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_insert_after");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.insert_after");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    MooUiNode *arg1 = (MooUiNode*) moo_lua_get_arg_instance (L, first_arg + 1, "parent", MOO_TYPE_UI_NODE, FALSE);
    MooUiNode *arg2 = (MooUiNode*) moo_lua_get_arg_instance (L, first_arg + 2, "after", MOO_TYPE_UI_NODE, FALSE);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "markup", FALSE);
    moo_ui_xml_insert_after (self, arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_MooUiXml_insert_before (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_insert_before");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.insert_before");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    MooUiNode *arg1 = (MooUiNode*) moo_lua_get_arg_instance (L, first_arg + 1, "parent", MOO_TYPE_UI_NODE, FALSE);
    MooUiNode *arg2 = (MooUiNode*) moo_lua_get_arg_instance (L, first_arg + 2, "before", MOO_TYPE_UI_NODE, FALSE);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "markup", FALSE);
    moo_ui_xml_insert_before (self, arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_MooUiXml_insert_markup (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_insert_markup");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.insert_markup");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    const char* arg1 = moo_lua_get_arg_utf8 (L, first_arg + 1, "parent_path", FALSE);
    int arg2 = moo_lua_get_arg_index_opt (L, first_arg + 2, "position", -1);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "markup", FALSE);
    moo_ui_xml_insert_markup (self, arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_MooUiXml_insert_markup_after (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_insert_markup_after");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.insert_markup_after");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    const char* arg1 = moo_lua_get_arg_utf8 (L, first_arg + 1, "parent_path", FALSE);
    const char* arg2 = moo_lua_get_arg_utf8 (L, first_arg + 2, "after_name", FALSE);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "markup", FALSE);
    moo_ui_xml_insert_markup_after (self, arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_MooUiXml_insert_markup_before (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_insert_markup_before");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.insert_markup_before");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    const char* arg1 = moo_lua_get_arg_utf8 (L, first_arg + 1, "parent_path", FALSE);
    const char* arg2 = moo_lua_get_arg_utf8 (L, first_arg + 2, "before_name", FALSE);
    const char* arg3 = moo_lua_get_arg_utf8 (L, first_arg + 3, "markup", FALSE);
    moo_ui_xml_insert_markup_before (self, arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_MooUiXml_new_merge_id (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_new_merge_id");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.new_merge_id");
    MooUiXml *self = (MooUiXml*) pself;
    guint ret = moo_ui_xml_new_merge_id (self);
    return moo_lua_push_uint64 (L, ret);
}

static int
cfunc_MooUiXml_remove_node (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_remove_node");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.remove_node");
    MooUiXml *self = (MooUiXml*) pself;
    MooUiNode *arg0 = (MooUiNode*) moo_lua_get_arg_instance (L, first_arg + 0, "node", MOO_TYPE_UI_NODE, FALSE);
    moo_ui_xml_remove_node (self, arg0);
    return 0;
}

static int
cfunc_MooUiXml_remove_ui (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_xml_remove_ui");
#endif
    MooLuaCurrentFunc cur_func ("MooUiXml.remove_ui");
    MooUiXml *self = (MooUiXml*) pself;
    guint arg0 = moo_lua_get_arg_uint (L, first_arg + 0, "merge_id");
    moo_ui_xml_remove_ui (self, arg0);
    return 0;
}

// methods of MooWindow

// methods of MooUiNode

static int
cfunc_MooUiNode_get_path (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_node_get_path");
#endif
    MooLuaCurrentFunc cur_func ("MooUiNode.get_path");
    MooUiNode *self = (MooUiNode*) pself;
    char *ret = moo_ui_node_get_path (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_MooUiNode_get_child (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_ui_node_get_child");
#endif
    MooLuaCurrentFunc cur_func ("MooUiNode.get_child");
    MooUiNode *self = (MooUiNode*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "name", FALSE);
    gpointer ret = moo_ui_node_get_child (self, arg0);
    return moo_lua_push_pointer (L, ret, MOO_TYPE_UI_NODE, TRUE);
}

static int
cfunc_error_dialog (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_error_dialog");
#endif
    MooLuaCurrentFunc cur_func ("error_dialog");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "text", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "secondary_text", NULL, TRUE);
    GtkWidget *arg2 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, 1 + 2, "parent", GTK_TYPE_WIDGET, TRUE);
    moo_error_dialog (arg0, arg1, arg2);
    return 0;
}

static int
cfunc_get_data_and_lib_subdirs (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_data_and_lib_subdirs");
#endif
    MooLuaCurrentFunc cur_func ("get_data_and_lib_subdirs");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "subdir", FALSE);
    char **ret = moo_get_data_and_lib_subdirs (arg0);
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_get_data_dirs (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_data_dirs");
#endif
    MooLuaCurrentFunc cur_func ("get_data_dirs");
    char **ret = moo_get_data_dirs ();
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_get_data_subdirs (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_data_subdirs");
#endif
    MooLuaCurrentFunc cur_func ("get_data_subdirs");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "subdir", FALSE);
    char **ret = moo_get_data_subdirs (arg0);
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_get_lib_dirs (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_lib_dirs");
#endif
    MooLuaCurrentFunc cur_func ("get_lib_dirs");
    char **ret = moo_get_lib_dirs ();
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_get_lib_subdirs (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_lib_subdirs");
#endif
    MooLuaCurrentFunc cur_func ("get_lib_subdirs");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "subdir", FALSE);
    char **ret = moo_get_lib_subdirs (arg0);
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_get_named_user_data_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_named_user_data_file");
#endif
    MooLuaCurrentFunc cur_func ("get_named_user_data_file");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "basename", FALSE);
    char *ret = moo_get_named_user_data_file (arg0);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_get_sys_data_subdirs (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_sys_data_subdirs");
#endif
    MooLuaCurrentFunc cur_func ("get_sys_data_subdirs");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "subdir", FALSE);
    char **ret = moo_get_sys_data_subdirs (arg0);
    return moo_lua_push_strv (L, ret);
}

static int
cfunc_get_user_cache_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_user_cache_file");
#endif
    MooLuaCurrentFunc cur_func ("get_user_cache_file");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "basename", FALSE);
    char *ret = moo_get_user_cache_file (arg0);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_get_user_data_dir (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_user_data_dir");
#endif
    MooLuaCurrentFunc cur_func ("get_user_data_dir");
    char *ret = moo_get_user_data_dir ();
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_get_user_data_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_get_user_data_file");
#endif
    MooLuaCurrentFunc cur_func ("get_user_data_file");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "basename", FALSE);
    char *ret = moo_get_user_data_file (arg0);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_info_dialog (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_info_dialog");
#endif
    MooLuaCurrentFunc cur_func ("info_dialog");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "text", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "secondary_text", NULL, TRUE);
    GtkWidget *arg2 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, 1 + 2, "parent", GTK_TYPE_WIDGET, TRUE);
    moo_info_dialog (arg0, arg1, arg2);
    return 0;
}

static int
cfunc_overwrite_file_dialog (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_overwrite_file_dialog");
#endif
    MooLuaCurrentFunc cur_func ("overwrite_file_dialog");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "display_name", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8 (L, 1 + 1, "display_dirname", FALSE);
    GtkWidget *arg2 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, 1 + 2, "parent", GTK_TYPE_WIDGET, TRUE);
    gboolean ret = moo_overwrite_file_dialog (arg0, arg1, arg2);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_prefs_get_bool (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_get_bool");
#endif
    MooLuaCurrentFunc cur_func ("prefs_get_bool");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    gboolean ret = moo_prefs_get_bool (arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_prefs_get_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_get_file");
#endif
    MooLuaCurrentFunc cur_func ("prefs_get_file");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    gpointer ret = moo_prefs_get_file (arg0);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_prefs_get_filename (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_get_filename");
#endif
    MooLuaCurrentFunc cur_func ("prefs_get_filename");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    const char *ret = moo_prefs_get_filename (arg0);
    return moo_lua_push_filename_copy (L, ret);
}

static int
cfunc_prefs_get_int (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_get_int");
#endif
    MooLuaCurrentFunc cur_func ("prefs_get_int");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    int ret = moo_prefs_get_int (arg0);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_prefs_get_string (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_get_string");
#endif
    MooLuaCurrentFunc cur_func ("prefs_get_string");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    const char *ret = moo_prefs_get_string (arg0);
    return moo_lua_push_string_copy (L, ret);
}

static int
cfunc_prefs_new_key_bool (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_new_key_bool");
#endif
    MooLuaCurrentFunc cur_func ("prefs_new_key_bool");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    gboolean arg1 = moo_lua_get_arg_bool_opt (L, 1 + 1, "default_val", FALSE);
    moo_prefs_new_key_bool (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_new_key_int (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_new_key_int");
#endif
    MooLuaCurrentFunc cur_func ("prefs_new_key_int");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    int arg1 = moo_lua_get_arg_int_opt (L, 1 + 1, "default_val", 0);
    moo_prefs_new_key_int (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_new_key_string (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_new_key_string");
#endif
    MooLuaCurrentFunc cur_func ("prefs_new_key_string");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    const char* arg1 = moo_lua_get_arg_string_opt (L, 1 + 1, "default_val", NULL, TRUE);
    moo_prefs_new_key_string (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_set_bool (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_set_bool");
#endif
    MooLuaCurrentFunc cur_func ("prefs_set_bool");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    gboolean arg1 = moo_lua_get_arg_bool (L, 1 + 1, "val");
    moo_prefs_set_bool (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_set_file (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_set_file");
#endif
    MooLuaCurrentFunc cur_func ("prefs_set_file");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    GFile *arg1 = (GFile*) moo_lua_get_arg_instance (L, 1 + 1, "val", G_TYPE_FILE, TRUE);
    moo_prefs_set_file (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_set_filename (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_set_filename");
#endif
    MooLuaCurrentFunc cur_func ("prefs_set_filename");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    const char* arg1 = moo_lua_get_arg_filename (L, 1 + 1, "val", TRUE);
    moo_prefs_set_filename (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_set_int (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_set_int");
#endif
    MooLuaCurrentFunc cur_func ("prefs_set_int");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    int arg1 = moo_lua_get_arg_int (L, 1 + 1, "val");
    moo_prefs_set_int (arg0, arg1);
    return 0;
}

static int
cfunc_prefs_set_string (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_prefs_set_string");
#endif
    MooLuaCurrentFunc cur_func ("prefs_set_string");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "key", FALSE);
    const char* arg1 = moo_lua_get_arg_string (L, 1 + 1, "val", TRUE);
    moo_prefs_set_string (arg0, arg1);
    return 0;
}

static int
cfunc_question_dialog (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_question_dialog");
#endif
    MooLuaCurrentFunc cur_func ("question_dialog");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "text", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "secondary_text", NULL, TRUE);
    GtkWidget *arg2 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, 1 + 2, "parent", GTK_TYPE_WIDGET, TRUE);
    GtkResponseType arg3 = (GtkResponseType) moo_lua_get_arg_enum_opt (L, 1 + 3, "default_response", GTK_TYPE_RESPONSE_TYPE, GTK_RESPONSE_OK);
    gboolean ret = moo_question_dialog (arg0, arg1, arg2, arg3);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_save_changes_dialog (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_save_changes_dialog");
#endif
    MooLuaCurrentFunc cur_func ("save_changes_dialog");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "display_name", FALSE);
    GtkWidget *arg1 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, 1 + 1, "parent", GTK_TYPE_WIDGET, TRUE);
    MooSaveChangesResponse ret = moo_save_changes_dialog (arg0, arg1);
    return moo_lua_push_int (L, ret);
}

static int
cfunc_spin_main_loop (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_spin_main_loop");
#endif
    MooLuaCurrentFunc cur_func ("spin_main_loop");
    double arg0 = moo_lua_get_arg_double (L, 1 + 0, "sec");
    moo_spin_main_loop (arg0);
    return 0;
}

static int
cfunc_tempdir (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_tempdir");
#endif
    MooLuaCurrentFunc cur_func ("tempdir");
    char *ret = moo_tempdir ();
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_tempnam (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_tempnam");
#endif
    MooLuaCurrentFunc cur_func ("tempnam");
    const char* arg0 = moo_lua_get_arg_filename_opt (L, 1 + 0, "extension", NULL, TRUE);
    char *ret = moo_tempnam (arg0);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_test_assert_impl (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_test_assert_impl");
#endif
    MooLuaCurrentFunc cur_func ("test_assert_impl");
    gboolean arg0 = moo_lua_get_arg_bool (L, 1 + 0, "passed");
    const char* arg1 = moo_lua_get_arg_utf8 (L, 1 + 1, "text", FALSE);
    const char* arg2 = moo_lua_get_arg_utf8_opt (L, 1 + 2, "file", NULL, TRUE);
    int arg3 = moo_lua_get_arg_int_opt (L, 1 + 3, "line", -1);
    moo_test_assert_impl (arg0, arg1, arg2, arg3);
    return 0;
}

static int
cfunc_test_set_silent_messages (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_test_set_silent_messages");
#endif
    MooLuaCurrentFunc cur_func ("test_set_silent_messages");
    gboolean arg0 = moo_lua_get_arg_bool (L, 1 + 0, "silent");
    gboolean ret = moo_test_set_silent_messages (arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_warning_dialog (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "moo_warning_dialog");
#endif
    MooLuaCurrentFunc cur_func ("warning_dialog");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "text", FALSE);
    const char* arg1 = moo_lua_get_arg_utf8_opt (L, 1 + 1, "secondary_text", NULL, TRUE);
    GtkWidget *arg2 = (GtkWidget*) moo_lua_get_arg_instance_opt (L, 1 + 2, "parent", GTK_TYPE_WIDGET, TRUE);
    moo_warning_dialog (arg0, arg1, arg2);
    return 0;
}

static const luaL_Reg moo_lua_functions[] = {
    { "error_dialog", cfunc_error_dialog },
    { "get_data_and_lib_subdirs", cfunc_get_data_and_lib_subdirs },
    { "get_data_dirs", cfunc_get_data_dirs },
    { "get_data_subdirs", cfunc_get_data_subdirs },
    { "get_lib_dirs", cfunc_get_lib_dirs },
    { "get_lib_subdirs", cfunc_get_lib_subdirs },
    { "get_named_user_data_file", cfunc_get_named_user_data_file },
    { "get_sys_data_subdirs", cfunc_get_sys_data_subdirs },
    { "get_user_cache_file", cfunc_get_user_cache_file },
    { "get_user_data_dir", cfunc_get_user_data_dir },
    { "get_user_data_file", cfunc_get_user_data_file },
    { "info_dialog", cfunc_info_dialog },
    { "overwrite_file_dialog", cfunc_overwrite_file_dialog },
    { "prefs_get_bool", cfunc_prefs_get_bool },
    { "prefs_get_file", cfunc_prefs_get_file },
    { "prefs_get_filename", cfunc_prefs_get_filename },
    { "prefs_get_int", cfunc_prefs_get_int },
    { "prefs_get_string", cfunc_prefs_get_string },
    { "prefs_new_key_bool", cfunc_prefs_new_key_bool },
    { "prefs_new_key_int", cfunc_prefs_new_key_int },
    { "prefs_new_key_string", cfunc_prefs_new_key_string },
    { "prefs_set_bool", cfunc_prefs_set_bool },
    { "prefs_set_file", cfunc_prefs_set_file },
    { "prefs_set_filename", cfunc_prefs_set_filename },
    { "prefs_set_int", cfunc_prefs_set_int },
    { "prefs_set_string", cfunc_prefs_set_string },
    { "question_dialog", cfunc_question_dialog },
    { "save_changes_dialog", cfunc_save_changes_dialog },
    { "spin_main_loop", cfunc_spin_main_loop },
    { "tempdir", cfunc_tempdir },
    { "tempnam", cfunc_tempnam },
    { "test_assert_impl", cfunc_test_assert_impl },
    { "test_set_silent_messages", cfunc_test_set_silent_messages },
    { "warning_dialog", cfunc_warning_dialog },
    { NULL, NULL }
};

static const luaL_Reg Editor_lua_functions[] = {
    { "instance", cfunc_MooEditor_instance },
    { NULL, NULL }
};

static const luaL_Reg SaveInfo_lua_functions[] = {
    { "new_file", cfunc_MooSaveInfo_new_file },
    { "new_uri", cfunc_MooSaveInfo_new_uri },
    { "new", cfunc_MooSaveInfo_new },
    { NULL, NULL }
};

static const luaL_Reg ReloadInfo_lua_functions[] = {
    { "new", cfunc_MooReloadInfo_new },
    { NULL, NULL }
};

static const luaL_Reg OpenInfo_lua_functions[] = {
    { "new_file", cfunc_MooOpenInfo_new_file },
    { "new_uri", cfunc_MooOpenInfo_new_uri },
    { "new", cfunc_MooOpenInfo_new },
    { NULL, NULL }
};

static const luaL_Reg App_lua_functions[] = {
    { "instance", cfunc_MooApp_instance },
    { NULL, NULL }
};

static void
moo_lua_api_register (void)
{
    static gboolean been_here = FALSE;

    if (been_here)
        return;

    been_here = TRUE;

    MooLuaMethodEntry methods_MooApp[] = {
        { "get_editor", cfunc_MooApp_get_editor },
        { "quit", cfunc_MooApp_quit },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_APP, methods_MooApp);

    MooLuaMethodEntry methods_MooCmdView[] = {
        { "run_command", cfunc_MooCmdView_run_command },
        { "set_filter_by_id", cfunc_MooCmdView_set_filter_by_id },
        { "write_with_filter", cfunc_MooCmdView_write_with_filter },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_CMD_VIEW, methods_MooCmdView);

    MooLuaMethodEntry methods_MooEdit[] = {
        { "append_text", cfunc_MooEdit_append_text },
        { "begin_non_undoable_action", cfunc_MooEdit_begin_non_undoable_action },
        { "begin_user_action", cfunc_MooEdit_begin_user_action },
        { "can_redo", cfunc_MooEdit_can_redo },
        { "can_undo", cfunc_MooEdit_can_undo },
        { "clear", cfunc_MooEdit_clear },
        { "close", cfunc_MooEdit_close },
        { "comment_selection", cfunc_MooEdit_comment_selection },
        { "copy", cfunc_MooEdit_copy },
        { "cut", cfunc_MooEdit_cut },
        { "delete_selected_lines", cfunc_MooEdit_delete_selected_lines },
        { "delete_selected_text", cfunc_MooEdit_delete_selected_text },
        { "delete_text", cfunc_MooEdit_delete_text },
        { "end_non_undoable_action", cfunc_MooEdit_end_non_undoable_action },
        { "end_user_action", cfunc_MooEdit_end_user_action },
        { "get_buffer", cfunc_MooEdit_get_buffer },
        { "get_char_at_pos", cfunc_MooEdit_get_char_at_pos },
        { "get_char_count", cfunc_MooEdit_get_char_count },
        { "get_clean", cfunc_MooEdit_get_clean },
        { "get_cursor_pos", cfunc_MooEdit_get_cursor_pos },
        { "get_display_basename", cfunc_MooEdit_get_display_basename },
        { "get_display_name", cfunc_MooEdit_get_display_name },
        { "get_editor", cfunc_MooEdit_get_editor },
        { "get_encoding", cfunc_MooEdit_get_encoding },
        { "get_end_pos", cfunc_MooEdit_get_end_pos },
        { "get_file", cfunc_MooEdit_get_file },
        { "get_filename", cfunc_MooEdit_get_filename },
        { "get_lang_id", cfunc_MooEdit_get_lang_id },
        { "get_line_at_cursor", cfunc_MooEdit_get_line_at_cursor },
        { "get_line_at_pos", cfunc_MooEdit_get_line_at_pos },
        { "get_line_count", cfunc_MooEdit_get_line_count },
        { "get_line_end_type", cfunc_MooEdit_get_line_end_type },
        { "get_line_text", cfunc_MooEdit_get_line_text },
        { "get_line_text_at_pos", cfunc_MooEdit_get_line_text_at_pos },
        { "get_n_views", cfunc_MooEdit_get_n_views },
        { "get_pos_at_line", cfunc_MooEdit_get_pos_at_line },
        { "get_pos_at_line_end", cfunc_MooEdit_get_pos_at_line_end },
        { "get_selected_lines", cfunc_MooEdit_get_selected_lines },
        { "get_selected_text", cfunc_MooEdit_get_selected_text },
        { "get_selection_end_pos", cfunc_MooEdit_get_selection_end_pos },
        { "get_selection_start_pos", cfunc_MooEdit_get_selection_start_pos },
        { "get_start_pos", cfunc_MooEdit_get_start_pos },
        { "get_status", cfunc_MooEdit_get_status },
        { "get_tab", cfunc_MooEdit_get_tab },
        { "get_text", cfunc_MooEdit_get_text },
        { "get_uri", cfunc_MooEdit_get_uri },
        { "get_view", cfunc_MooEdit_get_view },
        { "get_views", cfunc_MooEdit_get_views },
        { "get_window", cfunc_MooEdit_get_window },
        { "has_selection", cfunc_MooEdit_has_selection },
        { "insert_text", cfunc_MooEdit_insert_text },
        { "is_empty", cfunc_MooEdit_is_empty },
        { "is_modified", cfunc_MooEdit_is_modified },
        { "is_untitled", cfunc_MooEdit_is_untitled },
        { "paste", cfunc_MooEdit_paste },
        { "redo", cfunc_MooEdit_redo },
        { "reload", cfunc_MooEdit_reload },
        { "replace_selected_lines", cfunc_MooEdit_replace_selected_lines },
        { "replace_selected_text", cfunc_MooEdit_replace_selected_text },
        { "replace_text", cfunc_MooEdit_replace_text },
        { "save", cfunc_MooEdit_save },
        { "save_as", cfunc_MooEdit_save_as },
        { "save_copy", cfunc_MooEdit_save_copy },
        { "select_all", cfunc_MooEdit_select_all },
        { "select_lines", cfunc_MooEdit_select_lines },
        { "select_lines_at_pos", cfunc_MooEdit_select_lines_at_pos },
        { "select_range", cfunc_MooEdit_select_range },
        { "set_clean", cfunc_MooEdit_set_clean },
        { "set_cursor_pos", cfunc_MooEdit_set_cursor_pos },
        { "set_encoding", cfunc_MooEdit_set_encoding },
        { "set_line_end_type", cfunc_MooEdit_set_line_end_type },
        { "set_modified", cfunc_MooEdit_set_modified },
        { "set_text", cfunc_MooEdit_set_text },
        { "uncomment_selection", cfunc_MooEdit_uncomment_selection },
        { "undo", cfunc_MooEdit_undo },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_EDIT, methods_MooEdit);

    MooLuaMethodEntry methods_MooEditTab[] = {
        { "get_active_view", cfunc_MooEditTab_get_active_view },
        { "get_doc", cfunc_MooEditTab_get_doc },
        { "get_views", cfunc_MooEditTab_get_views },
        { "get_window", cfunc_MooEditTab_get_window },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_EDIT_TAB, methods_MooEditTab);

    MooLuaMethodEntry methods_MooEditView[] = {
        { "get_doc", cfunc_MooEditView_get_doc },
        { "get_editor", cfunc_MooEditView_get_editor },
        { "get_tab", cfunc_MooEditView_get_tab },
        { "get_window", cfunc_MooEditView_get_window },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_EDIT_VIEW, methods_MooEditView);

    MooLuaMethodEntry methods_MooEditWindow[] = {
        { "close", cfunc_MooEditWindow_close },
        { "close_all", cfunc_MooEditWindow_close_all },
        { "get_active_doc", cfunc_MooEditWindow_get_active_doc },
        { "get_active_tab", cfunc_MooEditWindow_get_active_tab },
        { "get_active_view", cfunc_MooEditWindow_get_active_view },
        { "get_docs", cfunc_MooEditWindow_get_docs },
        { "get_editor", cfunc_MooEditWindow_get_editor },
        { "get_n_tabs", cfunc_MooEditWindow_get_n_tabs },
        { "get_output", cfunc_MooEditWindow_get_output },
        { "get_tabs", cfunc_MooEditWindow_get_tabs },
        { "get_views", cfunc_MooEditWindow_get_views },
        { "present_output", cfunc_MooEditWindow_present_output },
        { "set_active_doc", cfunc_MooEditWindow_set_active_doc },
        { "set_active_tab", cfunc_MooEditWindow_set_active_tab },
        { "set_active_view", cfunc_MooEditWindow_set_active_view },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_EDIT_WINDOW, methods_MooEditWindow);

    MooLuaMethodEntry methods_MooEditor[] = {
        { "close_doc", cfunc_MooEditor_close_doc },
        { "close_docs", cfunc_MooEditor_close_docs },
        { "close_window", cfunc_MooEditor_close_window },
        { "get_active_doc", cfunc_MooEditor_get_active_doc },
        { "get_active_view", cfunc_MooEditor_get_active_view },
        { "get_active_window", cfunc_MooEditor_get_active_window },
        { "get_doc", cfunc_MooEditor_get_doc },
        { "get_doc_for_file", cfunc_MooEditor_get_doc_for_file },
        { "get_doc_for_uri", cfunc_MooEditor_get_doc_for_uri },
        { "get_doc_ui_xml", cfunc_MooEditor_get_doc_ui_xml },
        { "get_docs", cfunc_MooEditor_get_docs },
        { "get_ui_xml", cfunc_MooEditor_get_ui_xml },
        { "get_windows", cfunc_MooEditor_get_windows },
        { "new_doc", cfunc_MooEditor_new_doc },
        { "new_file", cfunc_MooEditor_new_file },
        { "new_window", cfunc_MooEditor_new_window },
        { "open_file", cfunc_MooEditor_open_file },
        { "open_files", cfunc_MooEditor_open_files },
        { "open_path", cfunc_MooEditor_open_path },
        { "open_uri", cfunc_MooEditor_open_uri },
        { "reload", cfunc_MooEditor_reload },
        { "save", cfunc_MooEditor_save },
        { "save_as", cfunc_MooEditor_save_as },
        { "save_copy", cfunc_MooEditor_save_copy },
        { "set_active_doc", cfunc_MooEditor_set_active_doc },
        { "set_active_view", cfunc_MooEditor_set_active_view },
        { "set_active_window", cfunc_MooEditor_set_active_window },
        { "set_ui_xml", cfunc_MooEditor_set_ui_xml },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_EDITOR, methods_MooEditor);

    MooLuaMethodEntry methods_MooLineView[] = {
        { "clear", cfunc_MooLineView_clear },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_LINE_VIEW, methods_MooLineView);

    MooLuaMethodEntry methods_MooOpenInfo[] = {
        { "add_flags", cfunc_MooOpenInfo_add_flags },
        { "dup", cfunc_MooOpenInfo_dup },
        { "get_encoding", cfunc_MooOpenInfo_get_encoding },
        { "get_file", cfunc_MooOpenInfo_get_file },
        { "get_filename", cfunc_MooOpenInfo_get_filename },
        { "get_flags", cfunc_MooOpenInfo_get_flags },
        { "get_line", cfunc_MooOpenInfo_get_line },
        { "get_uri", cfunc_MooOpenInfo_get_uri },
        { "set_encoding", cfunc_MooOpenInfo_set_encoding },
        { "set_flags", cfunc_MooOpenInfo_set_flags },
        { "set_line", cfunc_MooOpenInfo_set_line },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_OPEN_INFO, methods_MooOpenInfo);

    MooLuaMethodEntry methods_MooReloadInfo[] = {
        { "dup", cfunc_MooReloadInfo_dup },
        { "get_line", cfunc_MooReloadInfo_get_line },
        { "set_line", cfunc_MooReloadInfo_set_line },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_RELOAD_INFO, methods_MooReloadInfo);

    MooLuaMethodEntry methods_MooSaveInfo[] = {
        { "dup", cfunc_MooSaveInfo_dup },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_SAVE_INFO, methods_MooSaveInfo);

    MooLuaMethodEntry methods_MooTextView[] = {
        { "set_font_from_string", cfunc_MooTextView_set_font_from_string },
        { "set_lang_by_id", cfunc_MooTextView_set_lang_by_id },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_TEXT_VIEW, methods_MooTextView);

    MooLuaMethodEntry methods_MooUiXml[] = {
        { "add_item", cfunc_MooUiXml_add_item },
        { "add_ui_from_string", cfunc_MooUiXml_add_ui_from_string },
        { "create_widget", cfunc_MooUiXml_create_widget },
        { "find_placeholder", cfunc_MooUiXml_find_placeholder },
        { "get_node", cfunc_MooUiXml_get_node },
        { "get_widget", cfunc_MooUiXml_get_widget },
        { "insert", cfunc_MooUiXml_insert },
        { "insert_after", cfunc_MooUiXml_insert_after },
        { "insert_before", cfunc_MooUiXml_insert_before },
        { "insert_markup", cfunc_MooUiXml_insert_markup },
        { "insert_markup_after", cfunc_MooUiXml_insert_markup_after },
        { "insert_markup_before", cfunc_MooUiXml_insert_markup_before },
        { "new_merge_id", cfunc_MooUiXml_new_merge_id },
        { "remove_node", cfunc_MooUiXml_remove_node },
        { "remove_ui", cfunc_MooUiXml_remove_ui },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_UI_XML, methods_MooUiXml);

    MooLuaMethodEntry methods_MooUiNode[] = {
        { "get_path", cfunc_MooUiNode_get_path },
        { "get_child", cfunc_MooUiNode_get_child },
        { NULL, NULL }
    };
    moo_lua_register_methods (MOO_TYPE_UI_NODE, methods_MooUiNode);

}

void moo_lua_api_add_to_lua (lua_State *L)
{
    moo_lua_api_register ();

    luaL_register (L, "moo", moo_lua_functions);

    moo_lua_register_static_methods (L, "moo", "Editor", Editor_lua_functions);
    moo_lua_register_static_methods (L, "moo", "SaveInfo", SaveInfo_lua_functions);
    moo_lua_register_static_methods (L, "moo", "ReloadInfo", ReloadInfo_lua_functions);
    moo_lua_register_static_methods (L, "moo", "OpenInfo", OpenInfo_lua_functions);
    moo_lua_register_static_methods (L, "moo", "App", App_lua_functions);

    moo_lua_register_enum (L, "moo", MOO_TYPE_ACTION_CHECK_TYPE, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_CLOSE_RESPONSE, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_EDIT_STATUS, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_LINE_END_TYPE, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_OPEN_FLAGS, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_PANE_POSITION, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_SAVE_CHANGES_RESPONSE, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_SAVE_RESPONSE, "MOO_");
    moo_lua_register_enum (L, "moo", MOO_TYPE_UI_WIDGET_TYPE, "MOO_");
}
