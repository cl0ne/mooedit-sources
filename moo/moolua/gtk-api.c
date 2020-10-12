/**
 * class:GObject
 **/

/**
 * class:GFile: (parent GObject) (moo.python 0)
 **/

/**
 * class:GdkPixbuf: (parent GObject) (moo.python 0) (moo.private 1)
 **/

/**
 * class:GtkObject: (parent GObject) (moo.python 0)
 **/

/**
 * class:GtkAccelGroup: (parent GObject) (moo.python 0) (moo.private 1)
 **/

/**
 * class:GtkWidget: (parent GtkObject) (moo.python 0)
 **/

/**
 * class:GtkContainer: (parent GtkWidget) (moo.python 0)
 **/

/**
 * class:GtkBin: (parent GtkContainer) (moo.python 0)
 **/

/**
 * class:GtkWindow: (parent GtkBin) (moo.python 0)
 **/

/**
 * class:GtkBox: (parent GtkContainer) (moo.python 0)
 **/

/**
 * class:GtkVBox: (parent GtkBox) (moo.python 0)
 **/

/**
 * class:GtkTextView: (parent GtkContainer) (moo.python 0)
 **/

/**
 * class:GtkTextBuffer: (parent GObject) (moo.python 0)
 **/

/**
 * boxed:GtkTextIter: (moo.python 0)
 **/

/**
 * boxed:GdkRectangle: (moo.python 0)
 **/

/**
 * enum:GtkResponseType: (moo.python 0)
 *
 * @GTK_RESPONSE_NONE:
 * @GTK_RESPONSE_REJECT:
 * @GTK_RESPONSE_ACCEPT:
 * @GTK_RESPONSE_DELETE_EVENT:
 * @GTK_RESPONSE_OK:
 * @GTK_RESPONSE_CANCEL:
 * @GTK_RESPONSE_CLOSE:
 * @GTK_RESPONSE_YES:
 * @GTK_RESPONSE_NO:
 * @GTK_RESPONSE_APPLY:
 * @GTK_RESPONSE_HELP:
 **/


/****************************************************************************
 *
 * GObject
 *
 */

/**
 * g_object_connect: (moo.python 0)
 *
 * @gobj:
 * @signal: (type const-utf8): signal name
 * @callback: function to call when the signal is emitted
 *
 * Connect a signal handler.
 *
 * <example>
 * <title>GObject.connect<!-- -->()</title>
 * <programlisting>
 * <n/>editor.connect("before-save", function(editor, doc, file)<nl/>
 * <n/>  if #<!-- -->doc.get_text<!-- -->() % 2 ~= 0 then<nl/>
 * <n/>    moo.error_dialog("Won't save",<nl/>
 * <n/>                     "Odd number of characters in a file " ..<nl/>
 * <n/>                     "is bad for your hard drive, I am not " ..<nl/>
 * <n/>                     "going to save this.",<nl/>
 * <n/>                     doc.get_window<!-- -->())<nl/>
 * <n/>    return moo.SAVE_RESPONSE_CANCEL<nl/>
 * <n/>  else<nl/>
 * <n/>    return moo.SAVE_RESPONSE_CONTINUE<nl/>
 * <n/>  end<nl/>
 * <n/>end)
 * </programlisting>
 * </example>
 *
 * Returns: id of connected signal handler. Use g_object_disconnect() to remove it.
 **/

/**
 * g_object_connect_after: (moo.python 0)
 *
 * @gobj:
 * @signal: (type const-utf8): signal name
 * @callback: function to call when the signal is emitted
 **/

/**
 * g_object_disconnect: (moo.python 0)
 *
 * @gobj:
 * @handler_id: signal handler id returned from g_object_connect()
 *
 * Disconnect a signal handler.
 **/

/**
 * g_object_signal_handler_block: (moo.python 0)
 *
 * @gobj:
 * @handler_id: signal handler id returned from g_object_connect()
 *
 * Temporarily blocks signal handler so it will not be called when
 * the signal it's connected to is emitted. Call g_object_signal_handler_unblock()
 * to re-enable the signal handler.
 **/

/**
 * g_object_signal_handler_unblock: (moo.python 0)
 *
 * @gobj:
 * @handler_id: signal handler id returned from g_object_connect()
 *
 * Re-enables signal handler disabled by g_object_signal_handler_block().
 **/

/**
 * g_object_set_property: (moo.python 0) (moo.lua.cfunc moo_lua_cfunc_set_property)
 *
 * @gobj: (type GObject*)
 * @property_name: (type const-utf8):
 * @value: (type value)
 **/

/**
 * g_object_get_property: (moo.python 0) (moo.lua.cfunc moo_lua_cfunc_get_property)
 *
 * @gobj: (type GObject*)
 * @property_name: (type const-utf8):
 *
 * Returns: (type value)
 **/

/****************************************************************************
 *
 * GFile
 *
 */

/**
 * g_file_new_for_path: (static-method-of GFile)
 *
 * @path: (type const-filename)
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_new_for_uri: (static-method-of GFile)
 *
 * @uri: (type const-utf8)
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_parse_name: (static-method-of GFile)
 *
 * @parse_name: (type const-utf8)
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_dup:
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_hash:
 **/

/**
 * g_file_equal:
 **/

/**
 * g_file_get_basename:
 *
 * Returns: (type filename)
 **/

/**
 * g_file_get_path:
 *
 * Returns: (type filename)
 **/

/**
 * g_file_get_uri:
 *
 * Returns: (type utf8)
 **/

/**
 * g_file_get_parse_name:
 *
 * Returns: (type utf8)
 **/

/**
 * g_file_get_parent:
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_has_parent:
 *
 * @file:
 * @parent: (allow-none) (default NULL)
 **/

/**
 * g_file_get_child:
 *
 * @file:
 * @name: (type const-filename)
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_get_child_for_display_name:
 *
 * @file:
 * @display_name: (type const-utf8)
 * @error:
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_resolve_relative_path:
 *
 * @file:
 * @relative_path: (type const-filename)
 *
 * Returns: (transfer full)
 **/

/**
 * g_file_has_prefix:
 **/

/**
 * g_file_get_relative_path:
 *
 * Returns: (type filename)
 **/

/**
 * g_file_is_native:
 **/

/**
 * g_file_has_uri_scheme:
 *
 * @file:
 * @uri_scheme: (type const-utf8)
 **/

/**
 * g_file_get_uri_scheme:
 *
 * Returns: (type utf8)
 **/


/****************************************************************************
 *
 * GtkTextIter
 *
 */

/**
 * gtk_text_iter_get_buffer:
 **/

/**
 * gtk_text_iter_copy:
 *
 * Returns: (transfer full)
 **/

/**
 * gtk_text_iter_get_offset:
 *
 * Returns: (type index)
 **/

/**
 * gtk_text_iter_get_line:
 *
 * Returns: (type index)
 **/

/**
 * gtk_text_iter_get_line_offset:
 *
 * Returns: (type index)
 **/

/**
 * gtk_text_iter_get_line_index:
 *
 * Returns: (type index)
 **/

/**
 * gtk_text_iter_get_char:
 **/

/**
 * gtk_text_iter_get_text:
 *
 * Returns: (type utf8)
 **/

/**
 * gtk_text_iter_starts_line:
 **/

/**
 * gtk_text_iter_ends_line:
 **/

/**
 * gtk_text_iter_is_cursor_position:
 **/

/**
 * gtk_text_iter_get_chars_in_line:
 **/

/**
 * gtk_text_iter_get_bytes_in_line:
 **/

/**
 * gtk_text_iter_is_end:
 **/

/**
 * gtk_text_iter_is_start:
 **/

/**
 * gtk_text_iter_forward_char:
 **/

/**
 * gtk_text_iter_backward_char:
 **/

/**
 * gtk_text_iter_forward_chars:
 **/

/**
 * gtk_text_iter_backward_chars:
 **/

/**
 * gtk_text_iter_forward_line:
 **/

/**
 * gtk_text_iter_backward_line:
 **/

/**
 * gtk_text_iter_forward_lines:
 **/

/**
 * gtk_text_iter_backward_lines:
 **/

/**
 * gtk_text_iter_forward_cursor_position:
 **/

/**
 * gtk_text_iter_backward_cursor_position:
 **/

/**
 * gtk_text_iter_forward_cursor_positions:
 **/

/**
 * gtk_text_iter_backward_cursor_positions:
 **/

/**
 * gtk_text_iter_set_offset:
 *
 * @iter:
 * @char_offset: (type index)
 **/

/**
 * gtk_text_iter_set_line:
 *
 * @iter:
 * @line_number: (type index)
 **/

/**
 * gtk_text_iter_set_line_offset:
 *
 * @iter:
 * @char_on_line: (type index)
 **/

/**
 * gtk_text_iter_set_line_index:
 *
 * @iter:
 * @byte_on_line: (type index)
 **/

/**
 * gtk_text_iter_forward_to_end:
 **/

/**
 * gtk_text_iter_forward_to_line_end:
 **/

/**
 * gtk_text_iter_equal:
 **/

/**
 * gtk_text_iter_compare:
 **/

/**
 * gtk_text_iter_in_range:
 **/

/**
 * gtk_text_iter_order:
 **/
