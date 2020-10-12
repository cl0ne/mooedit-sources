#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gtk-lua-api.h"

#include "moolua/moo-lua-api-util.h"

void moo_test_coverage_record (const char *lang, const char *function);

// methods of GFile

static int
cfunc_GFile_dup (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_dup");
#endif
    MooLuaCurrentFunc cur_func ("GFile.dup");
    GFile *self = (GFile*) pself;
    gpointer ret = g_file_dup (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_GFile_equal (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_equal");
#endif
    MooLuaCurrentFunc cur_func ("GFile.equal");
    GFile *self = (GFile*) pself;
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance (L, first_arg + 0, "file2", G_TYPE_FILE, FALSE);
    gboolean ret = g_file_equal (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GFile_get_basename (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_basename");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_basename");
    GFile *self = (GFile*) pself;
    char *ret = g_file_get_basename (self);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_GFile_get_child (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_child");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_child");
    GFile *self = (GFile*) pself;
    const char* arg0 = moo_lua_get_arg_filename (L, first_arg + 0, "name", FALSE);
    gpointer ret = g_file_get_child (self, arg0);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_GFile_get_child_for_display_name (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_child_for_display_name");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_child_for_display_name");
    GFile *self = (GFile*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "display_name", FALSE);
    GError *error = NULL;
    gpointer ret = g_file_get_child_for_display_name (self, arg0, &error);
    int ret_lua = moo_lua_push_object (L, (GObject*) ret, FALSE);
    ret_lua += moo_lua_push_error (L, error);
    return ret_lua;
}

static int
cfunc_GFile_get_parent (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_parent");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_parent");
    GFile *self = (GFile*) pself;
    gpointer ret = g_file_get_parent (self);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_GFile_get_parse_name (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_parse_name");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_parse_name");
    GFile *self = (GFile*) pself;
    char *ret = g_file_get_parse_name (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_GFile_get_path (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_path");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_path");
    GFile *self = (GFile*) pself;
    char *ret = g_file_get_path (self);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_GFile_get_relative_path (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_relative_path");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_relative_path");
    GFile *self = (GFile*) pself;
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance (L, first_arg + 0, "descendant", G_TYPE_FILE, FALSE);
    char *ret = g_file_get_relative_path (self, arg0);
    return moo_lua_push_filename (L, ret);
}

static int
cfunc_GFile_get_uri (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_uri");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_uri");
    GFile *self = (GFile*) pself;
    char *ret = g_file_get_uri (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_GFile_get_uri_scheme (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_get_uri_scheme");
#endif
    MooLuaCurrentFunc cur_func ("GFile.get_uri_scheme");
    GFile *self = (GFile*) pself;
    char *ret = g_file_get_uri_scheme (self);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_GFile_has_parent (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_has_parent");
#endif
    MooLuaCurrentFunc cur_func ("GFile.has_parent");
    GFile *self = (GFile*) pself;
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance_opt (L, first_arg + 0, "parent", G_TYPE_FILE, TRUE);
    gboolean ret = g_file_has_parent (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GFile_has_prefix (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_has_prefix");
#endif
    MooLuaCurrentFunc cur_func ("GFile.has_prefix");
    GFile *self = (GFile*) pself;
    GFile *arg0 = (GFile*) moo_lua_get_arg_instance (L, first_arg + 0, "prefix", G_TYPE_FILE, FALSE);
    gboolean ret = g_file_has_prefix (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GFile_has_uri_scheme (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_has_uri_scheme");
#endif
    MooLuaCurrentFunc cur_func ("GFile.has_uri_scheme");
    GFile *self = (GFile*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "uri_scheme", FALSE);
    gboolean ret = g_file_has_uri_scheme (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GFile_hash (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_hash");
#endif
    MooLuaCurrentFunc cur_func ("GFile.hash");
    GFile *self = (GFile*) pself;
    guint ret = g_file_hash (self);
    return moo_lua_push_uint64 (L, ret);
}

static int
cfunc_GFile_is_native (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_is_native");
#endif
    MooLuaCurrentFunc cur_func ("GFile.is_native");
    GFile *self = (GFile*) pself;
    gboolean ret = g_file_is_native (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GFile_resolve_relative_path (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_resolve_relative_path");
#endif
    MooLuaCurrentFunc cur_func ("GFile.resolve_relative_path");
    GFile *self = (GFile*) pself;
    const char* arg0 = moo_lua_get_arg_filename (L, first_arg + 0, "relative_path", FALSE);
    gpointer ret = g_file_resolve_relative_path (self, arg0);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_GFile_new_for_path (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_new_for_path");
#endif
    MooLuaCurrentFunc cur_func ("GFile.new_for_path");
    const char* arg0 = moo_lua_get_arg_filename (L, 1 + 0, "path", FALSE);
    gpointer ret = g_file_new_for_path (arg0);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_GFile_new_for_uri (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_new_for_uri");
#endif
    MooLuaCurrentFunc cur_func ("GFile.new_for_uri");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "uri", FALSE);
    gpointer ret = g_file_new_for_uri (arg0);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

static int
cfunc_GFile_parse_name (G_GNUC_UNUSED lua_State *L)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_file_parse_name");
#endif
    MooLuaCurrentFunc cur_func ("GFile.parse_name");
    const char* arg0 = moo_lua_get_arg_utf8 (L, 1 + 0, "parse_name", FALSE);
    gpointer ret = g_file_parse_name (arg0);
    return moo_lua_push_object (L, (GObject*) ret, FALSE);
}

// methods of GObject

static int
cfunc_GObject_connect (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_object_connect");
#endif
    MooLuaCurrentFunc cur_func ("GObject.connect");
    GObject *self = (GObject*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "signal", FALSE);
    MooLuaSignalClosure *arg1 = moo_lua_get_arg_signal_closure (L, first_arg + 1, "callback");
    gulong ret = g_object_connect (self, arg0, arg1);
    g_closure_unref ((GClosure*) arg1);
    return moo_lua_push_uint64 (L, ret);
}

static int
cfunc_GObject_connect_after (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_object_connect_after");
#endif
    MooLuaCurrentFunc cur_func ("GObject.connect_after");
    GObject *self = (GObject*) pself;
    const char* arg0 = moo_lua_get_arg_utf8 (L, first_arg + 0, "signal", FALSE);
    MooLuaSignalClosure *arg1 = moo_lua_get_arg_signal_closure (L, first_arg + 1, "callback");
    gulong ret = g_object_connect_after (self, arg0, arg1);
    g_closure_unref ((GClosure*) arg1);
    return moo_lua_push_uint64 (L, ret);
}

static int
cfunc_GObject_disconnect (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_object_disconnect");
#endif
    MooLuaCurrentFunc cur_func ("GObject.disconnect");
    GObject *self = (GObject*) pself;
    gulong arg0 = moo_lua_get_arg_ulong (L, first_arg + 0, "handler_id");
    g_object_disconnect (self, arg0);
    return 0;
}

static int
cfunc_GObject_signal_handler_block (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_object_signal_handler_block");
#endif
    MooLuaCurrentFunc cur_func ("GObject.signal_handler_block");
    GObject *self = (GObject*) pself;
    gulong arg0 = moo_lua_get_arg_ulong (L, first_arg + 0, "handler_id");
    g_object_signal_handler_block (self, arg0);
    return 0;
}

static int
cfunc_GObject_signal_handler_unblock (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "g_object_signal_handler_unblock");
#endif
    MooLuaCurrentFunc cur_func ("GObject.signal_handler_unblock");
    GObject *self = (GObject*) pself;
    gulong arg0 = moo_lua_get_arg_ulong (L, first_arg + 0, "handler_id");
    g_object_signal_handler_unblock (self, arg0);
    return 0;
}

// methods of GdkPixbuf

// methods of GtkAccelGroup

// methods of GtkBin

// methods of GtkBox

// methods of GtkContainer

// methods of GtkObject

// methods of GtkTextBuffer

// methods of GtkTextView

// methods of GtkVBox

// methods of GtkWidget

// methods of GtkWindow

// methods of GdkRectangle

// methods of GtkTextIter

static int
cfunc_GtkTextIter_get_buffer (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_buffer");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_buffer");
    GtkTextIter *self = (GtkTextIter*) pself;
    gpointer ret = gtk_text_iter_get_buffer (self);
    return moo_lua_push_object (L, (GObject*) ret, TRUE);
}

static int
cfunc_GtkTextIter_copy (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_copy");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.copy");
    GtkTextIter *self = (GtkTextIter*) pself;
    gpointer ret = gtk_text_iter_copy (self);
    return moo_lua_push_boxed (L, ret, GTK_TYPE_TEXT_ITER, FALSE);
}

static int
cfunc_GtkTextIter_get_offset (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_offset");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_offset");
    GtkTextIter *self = (GtkTextIter*) pself;
    int ret = gtk_text_iter_get_offset (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_GtkTextIter_get_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    int ret = gtk_text_iter_get_line (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_GtkTextIter_get_line_offset (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_line_offset");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_line_offset");
    GtkTextIter *self = (GtkTextIter*) pself;
    int ret = gtk_text_iter_get_line_offset (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_GtkTextIter_get_line_index (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_line_index");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_line_index");
    GtkTextIter *self = (GtkTextIter*) pself;
    int ret = gtk_text_iter_get_line_index (self);
    return moo_lua_push_index (L, ret);
}

static int
cfunc_GtkTextIter_get_char (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_char");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_char");
    GtkTextIter *self = (GtkTextIter*) pself;
    gunichar ret = gtk_text_iter_get_char (self);
    return moo_lua_push_gunichar (L, ret);
}

static int
cfunc_GtkTextIter_get_text (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_text");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_text");
    GtkTextIter *self = (GtkTextIter*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "end", NULL, &arg0_iter);
    char *ret = gtk_text_iter_get_text (self, arg0);
    return moo_lua_push_utf8 (L, ret);
}

static int
cfunc_GtkTextIter_starts_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_starts_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.starts_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_starts_line (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_ends_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_ends_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.ends_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_ends_line (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_is_cursor_position (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_is_cursor_position");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.is_cursor_position");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_is_cursor_position (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_get_chars_in_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_chars_in_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_chars_in_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    int ret = gtk_text_iter_get_chars_in_line (self);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_GtkTextIter_get_bytes_in_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_get_bytes_in_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.get_bytes_in_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    int ret = gtk_text_iter_get_bytes_in_line (self);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_GtkTextIter_is_end (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_is_end");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.is_end");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_is_end (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_is_start (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_is_start");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.is_start");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_is_start (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_forward_char (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_char");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_char");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_forward_char (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_backward_char (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_backward_char");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.backward_char");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_backward_char (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_forward_chars (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_chars");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_chars");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_int (L, first_arg + 0, "count");
    gboolean ret = gtk_text_iter_forward_chars (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_backward_chars (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_backward_chars");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.backward_chars");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_int (L, first_arg + 0, "count");
    gboolean ret = gtk_text_iter_backward_chars (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_forward_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_forward_line (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_backward_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_backward_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.backward_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_backward_line (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_forward_lines (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_lines");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_lines");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_int (L, first_arg + 0, "count");
    gboolean ret = gtk_text_iter_forward_lines (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_backward_lines (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_backward_lines");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.backward_lines");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_int (L, first_arg + 0, "count");
    gboolean ret = gtk_text_iter_backward_lines (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_forward_cursor_position (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_cursor_position");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_cursor_position");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_forward_cursor_position (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_backward_cursor_position (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_backward_cursor_position");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.backward_cursor_position");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_backward_cursor_position (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_forward_cursor_positions (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_cursor_positions");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_cursor_positions");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_int (L, first_arg + 0, "count");
    gboolean ret = gtk_text_iter_forward_cursor_positions (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_backward_cursor_positions (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_backward_cursor_positions");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.backward_cursor_positions");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_int (L, first_arg + 0, "count");
    gboolean ret = gtk_text_iter_backward_cursor_positions (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_set_offset (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_set_offset");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.set_offset");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "char_offset");
    gtk_text_iter_set_offset (self, arg0);
    return 0;
}

static int
cfunc_GtkTextIter_set_line (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_set_line");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.set_line");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "line_number");
    gtk_text_iter_set_line (self, arg0);
    return 0;
}

static int
cfunc_GtkTextIter_set_line_offset (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_set_line_offset");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.set_line_offset");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "char_on_line");
    gtk_text_iter_set_line_offset (self, arg0);
    return 0;
}

static int
cfunc_GtkTextIter_set_line_index (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_set_line_index");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.set_line_index");
    GtkTextIter *self = (GtkTextIter*) pself;
    int arg0 = moo_lua_get_arg_index (L, first_arg + 0, "byte_on_line");
    gtk_text_iter_set_line_index (self, arg0);
    return 0;
}

static int
cfunc_GtkTextIter_forward_to_end (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_to_end");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_to_end");
    GtkTextIter *self = (GtkTextIter*) pself;
    gtk_text_iter_forward_to_end (self);
    return 0;
}

static int
cfunc_GtkTextIter_forward_to_line_end (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_forward_to_line_end");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.forward_to_line_end");
    GtkTextIter *self = (GtkTextIter*) pself;
    gboolean ret = gtk_text_iter_forward_to_line_end (self);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_equal (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_equal");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.equal");
    GtkTextIter *self = (GtkTextIter*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "rhs", NULL, &arg0_iter);
    gboolean ret = gtk_text_iter_equal (self, arg0);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_compare (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_compare");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.compare");
    GtkTextIter *self = (GtkTextIter*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "rhs", NULL, &arg0_iter);
    int ret = gtk_text_iter_compare (self, arg0);
    return moo_lua_push_int64 (L, ret);
}

static int
cfunc_GtkTextIter_in_range (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_in_range");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.in_range");
    GtkTextIter *self = (GtkTextIter*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "start", NULL, &arg0_iter);
    GtkTextIter arg1_iter;
    GtkTextIter *arg1 = &arg1_iter;
    moo_lua_get_arg_iter (L, first_arg + 1, "end", NULL, &arg1_iter);
    gboolean ret = gtk_text_iter_in_range (self, arg0, arg1);
    return moo_lua_push_bool (L, ret);
}

static int
cfunc_GtkTextIter_order (gpointer pself, G_GNUC_UNUSED lua_State *L, G_GNUC_UNUSED int first_arg)
{
#ifdef MOO_ENABLE_COVERAGE
    moo_test_coverage_record ("lua", "gtk_text_iter_order");
#endif
    MooLuaCurrentFunc cur_func ("GtkTextIter.order");
    GtkTextIter *self = (GtkTextIter*) pself;
    GtkTextIter arg0_iter;
    GtkTextIter *arg0 = &arg0_iter;
    moo_lua_get_arg_iter (L, first_arg + 0, "second", NULL, &arg0_iter);
    gtk_text_iter_order (self, arg0);
    return 0;
}

static const luaL_Reg gtk_lua_functions[] = {
    { NULL, NULL }
};

static const luaL_Reg GFile_lua_functions[] = {
    { "new_for_path", cfunc_GFile_new_for_path },
    { "new_for_uri", cfunc_GFile_new_for_uri },
    { "parse_name", cfunc_GFile_parse_name },
    { NULL, NULL }
};

static void
gtk_lua_api_register (void)
{
    static gboolean been_here = FALSE;

    if (been_here)
        return;

    been_here = TRUE;

    MooLuaMethodEntry methods_GFile[] = {
        { "dup", cfunc_GFile_dup },
        { "equal", cfunc_GFile_equal },
        { "get_basename", cfunc_GFile_get_basename },
        { "get_child", cfunc_GFile_get_child },
        { "get_child_for_display_name", cfunc_GFile_get_child_for_display_name },
        { "get_parent", cfunc_GFile_get_parent },
        { "get_parse_name", cfunc_GFile_get_parse_name },
        { "get_path", cfunc_GFile_get_path },
        { "get_relative_path", cfunc_GFile_get_relative_path },
        { "get_uri", cfunc_GFile_get_uri },
        { "get_uri_scheme", cfunc_GFile_get_uri_scheme },
        { "has_parent", cfunc_GFile_has_parent },
        { "has_prefix", cfunc_GFile_has_prefix },
        { "has_uri_scheme", cfunc_GFile_has_uri_scheme },
        { "hash", cfunc_GFile_hash },
        { "is_native", cfunc_GFile_is_native },
        { "resolve_relative_path", cfunc_GFile_resolve_relative_path },
        { NULL, NULL }
    };
    moo_lua_register_methods (G_TYPE_FILE, methods_GFile);

    MooLuaMethodEntry methods_GObject[] = {
        { "connect", cfunc_GObject_connect },
        { "connect_after", cfunc_GObject_connect_after },
        { "disconnect", cfunc_GObject_disconnect },
        { "get_property", moo_lua_cfunc_get_property },
        { "set_property", moo_lua_cfunc_set_property },
        { "signal_handler_block", cfunc_GObject_signal_handler_block },
        { "signal_handler_unblock", cfunc_GObject_signal_handler_unblock },
        { NULL, NULL }
    };
    moo_lua_register_methods (G_TYPE_OBJECT, methods_GObject);

    MooLuaMethodEntry methods_GtkTextIter[] = {
        { "get_buffer", cfunc_GtkTextIter_get_buffer },
        { "copy", cfunc_GtkTextIter_copy },
        { "get_offset", cfunc_GtkTextIter_get_offset },
        { "get_line", cfunc_GtkTextIter_get_line },
        { "get_line_offset", cfunc_GtkTextIter_get_line_offset },
        { "get_line_index", cfunc_GtkTextIter_get_line_index },
        { "get_char", cfunc_GtkTextIter_get_char },
        { "get_text", cfunc_GtkTextIter_get_text },
        { "starts_line", cfunc_GtkTextIter_starts_line },
        { "ends_line", cfunc_GtkTextIter_ends_line },
        { "is_cursor_position", cfunc_GtkTextIter_is_cursor_position },
        { "get_chars_in_line", cfunc_GtkTextIter_get_chars_in_line },
        { "get_bytes_in_line", cfunc_GtkTextIter_get_bytes_in_line },
        { "is_end", cfunc_GtkTextIter_is_end },
        { "is_start", cfunc_GtkTextIter_is_start },
        { "forward_char", cfunc_GtkTextIter_forward_char },
        { "backward_char", cfunc_GtkTextIter_backward_char },
        { "forward_chars", cfunc_GtkTextIter_forward_chars },
        { "backward_chars", cfunc_GtkTextIter_backward_chars },
        { "forward_line", cfunc_GtkTextIter_forward_line },
        { "backward_line", cfunc_GtkTextIter_backward_line },
        { "forward_lines", cfunc_GtkTextIter_forward_lines },
        { "backward_lines", cfunc_GtkTextIter_backward_lines },
        { "forward_cursor_position", cfunc_GtkTextIter_forward_cursor_position },
        { "backward_cursor_position", cfunc_GtkTextIter_backward_cursor_position },
        { "forward_cursor_positions", cfunc_GtkTextIter_forward_cursor_positions },
        { "backward_cursor_positions", cfunc_GtkTextIter_backward_cursor_positions },
        { "set_offset", cfunc_GtkTextIter_set_offset },
        { "set_line", cfunc_GtkTextIter_set_line },
        { "set_line_offset", cfunc_GtkTextIter_set_line_offset },
        { "set_line_index", cfunc_GtkTextIter_set_line_index },
        { "forward_to_end", cfunc_GtkTextIter_forward_to_end },
        { "forward_to_line_end", cfunc_GtkTextIter_forward_to_line_end },
        { "equal", cfunc_GtkTextIter_equal },
        { "compare", cfunc_GtkTextIter_compare },
        { "in_range", cfunc_GtkTextIter_in_range },
        { "order", cfunc_GtkTextIter_order },
        { NULL, NULL }
    };
    moo_lua_register_methods (GTK_TYPE_TEXT_ITER, methods_GtkTextIter);

}

void gtk_lua_api_add_to_lua (lua_State *L)
{
    gtk_lua_api_register ();

    luaL_register (L, "gtk", gtk_lua_functions);

    moo_lua_register_static_methods (L, "gtk", "GFile", GFile_lua_functions);

    moo_lua_register_enum (L, "gtk", GTK_TYPE_RESPONSE_TYPE, "GTK_");
}
