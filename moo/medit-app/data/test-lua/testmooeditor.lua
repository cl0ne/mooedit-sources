require("munit")

editor = moo.Editor.instance()

tassert(moo.CLOSE_RESPONSE_CANCEL ~= nil)
tassert(moo.CLOSE_RESPONSE_CONTINUE ~= nil)

function check_args_ew(editor, window)
  tassert(editor == moo.Editor.instance())
  tassert(editor == window.get_editor())
  window.get_tabs()
end

function check_args_ed(editor, doc)
  tassert(editor == moo.Editor.instance())
  tassert(editor == doc.get_editor())
  doc.get_display_basename()
end

function check_args_w(window)
  tassert(moo.Editor.instance() == window.get_editor())
  window.get_tabs()
end

function check_args_e(editor)
  tassert(moo.Editor.instance() == editor)
end

function check_args_d(doc)
  tassert(moo.Editor.instance() == doc.get_editor())
  doc.get_display_basename()
end

function check_args_f(gfile)
  gfile.get_path()
end

function test_will_close_window()
  local seen_editor_will_close_window = 0
  local seen_window_will_close = 0

  cb_id = editor.connect('will-close-window',
    function(editor, window)
      check_args_ew(editor, window)
      seen_editor_will_close_window = seen_editor_will_close_window + 1
    end)

  window = editor.new_window()

  window.connect('will-close',
    function(window)
      check_args_w(window)
      seen_window_will_close = seen_window_will_close + 1
    end)

  tassert(editor.close_window(window))

  window = editor.new_window()
  tassert(window.close())

  tassert(seen_editor_will_close_window == 2)
  tassert(seen_window_will_close == 1)

  editor.disconnect(cb_id)

  window = editor.new_window()
  tassert(editor.close_window(window))
  window = editor.new_window()
  tassert(window.close())

  tassert(seen_editor_will_close_window == 2)
  tassert(seen_window_will_close == 1)
end

function test_before_close_window()
  local seen_editor_before_close_window = 0
  local seen_window_before_close = 0

  cb_id = editor.connect('before-close-window',
    function(editor, window)
      check_args_ew(editor, window)
      seen_editor_before_close_window = seen_editor_before_close_window + 1
      return moo.CLOSE_RESPONSE_CANCEL
    end)

  window = editor.new_window()

  tassert(not editor.close_window(window))
  tassert(seen_editor_before_close_window == 1)
  tassert(not window.close())
  tassert(seen_editor_before_close_window == 2)

  tassert(not moo.App.instance().quit())
  tassert(seen_editor_before_close_window == 3)

  editor.disconnect(cb_id)

  cb_id = window.connect('before-close',
    function(window)
      check_args_w(window)
      seen_window_before_close = seen_window_before_close + 1
      return moo.CLOSE_RESPONSE_CANCEL
    end)

  tassert(not editor.close_window(window))
  tassert(seen_editor_before_close_window == 3)
  tassert(seen_window_before_close == 1)
  tassert(not window.close())
  tassert(seen_editor_before_close_window == 3)
  tassert(seen_window_before_close == 2)

  window.disconnect(cb_id)

  cb_id = editor.connect('before-close-window',
    function(editor, window)
      check_args_ew(editor, window)
      seen_editor_before_close_window = seen_editor_before_close_window + 1
      return moo.CLOSE_RESPONSE_CONTINUE
    end)

  window.connect('before-close',
    function(window)
      check_args_w(window)
      seen_window_before_close = seen_window_before_close + 1
      return moo.CLOSE_RESPONSE_CONTINUE
    end)

  tassert(window.close())
  tassert(seen_editor_before_close_window == 4)
  tassert(seen_window_before_close == 3)

  editor.disconnect(cb_id)
end

function test_bad_callback()
  window = editor.new_window()

  n_callbacks = 0

  cb_id1 = editor.connect('before-close-window', function(editor, window)
    check_args_ew(editor, window)
    n_callbacks = n_callbacks + 1
    error('lua error')
  end)
  cb_id2 = editor.connect('will-close-window', function(editor, window)
    check_args_ew(editor, window)
    n_callbacks = n_callbacks + 1
    error('lua error')
  end)
  window.connect('before-close', function(window)
    check_args_w(window)
    n_callbacks = n_callbacks + 1
    error('lua error')
  end)
  window.connect('will-close', function(window)
    check_args_w(window)
    n_callbacks = n_callbacks + 1
    error('lua error')
  end)
  window.connect('before-close', function(window)
    check_args_w(window)
    n_callbacks = n_callbacks + 1
    return nil
  end)
  window.connect('will-close', function(window)
    check_args_w(window)
    n_callbacks = n_callbacks + 1
    -- return nothing
  end)

  tassert(cb_id1 ~= nil and cb_id1 ~= 0)
  tassert(cb_id2 ~= nil and cb_id2 ~= 0)

  was_silent = moo.test_set_silent_messages(true)
  tassert(editor.close_window(window))
  moo.test_set_silent_messages(was_silent)

  tassert(n_callbacks == 6)

  editor.disconnect(cb_id1)
  editor.disconnect(cb_id2)
end

function test_will_save()
  path = moo.tempnam()
  si = moo.SaveInfo.new(path)
  doc = editor.new_doc()

  doc.set_modified(true)

  seen_doc_will_save = 0
  seen_editor_will_save = 0
  seen_doc_after_save = 0
  seen_editor_after_save = 0

  doc.connect('will-save', function(doc, gfile)
    check_args_d(doc)
    check_args_f(gfile)
    seen_doc_will_save = seen_doc_will_save + 1
    tassert(doc.is_modified())
  end)

  cb_id1 = editor.connect('will-save', function(editor, doc, gfile)
    check_args_e(editor)
    check_args_d(doc)
    check_args_f(gfile)
    seen_editor_will_save = seen_editor_will_save + 1
    tassert(doc.is_modified())
  end)

  doc.connect('after-save', function(doc)
    check_args_d(doc)
    seen_doc_after_save = seen_doc_after_save + 1
    tassert(not doc.is_modified())
  end)

  cb_id2 = editor.connect('after-save', function(editor, doc)
    check_args_e(editor)
    check_args_d(doc)
    seen_editor_after_save = seen_editor_after_save + 1
    tassert(not doc.is_modified())
  end)

  tassert(doc.save_as(si))

  tassert(seen_doc_will_save == 1)
  tassert(seen_editor_will_save == 1)
  tassert(seen_doc_after_save == 1)
  tassert(seen_editor_after_save == 1)

  tassert(doc.close())

  editor.disconnect(cb_id1)
  editor.disconnect(cb_id2)
end

function test_will_close_doc()
  local seen_editor_will_close_doc = 0
  local seen_doc_will_close = 0

  cb_id = editor.connect('will-close-doc',
    function(editor, doc)
      check_args_ed(editor, doc)
      seen_editor_will_close_doc = seen_editor_will_close_doc + 1
    end)

  doc = editor.new_doc()

  doc.connect('will-close',
    function(doc)
      check_args_d(doc)
      seen_doc_will_close = seen_doc_will_close + 1
    end)

  tassert(editor.close_doc(doc))

  doc = editor.new_doc()
  tassert(doc.close())

  tassert(seen_editor_will_close_doc == 2)
  tassert(seen_doc_will_close == 1)

  editor.disconnect(cb_id)

  doc = editor.new_doc()
  tassert(editor.close_doc(doc))
  doc = editor.new_doc()
  tassert(doc.close())

  tassert(seen_editor_will_close_doc == 2)
  tassert(seen_doc_will_close == 1)
end

function test_before_save()
  local seen_editor_before_save = 0
  local seen_doc_before_save = 0

  cb_id = editor.connect('before-save',
    function(editor, doc, gfile)
      check_args_ed(editor, doc)
      check_args_f(gfile)
      seen_editor_before_save = seen_editor_before_save + 1
      return moo.SAVE_RESPONSE_CANCEL
    end)

  doc = editor.new_doc()
  path = moo.tempnam()
  si = moo.SaveInfo.new(path)

  tassert(not editor.save_as(doc, si))
  tassert(seen_editor_before_save == 1)
  tassert(not doc.save_as(si))
  tassert(seen_editor_before_save == 2)

  editor.disconnect(cb_id)

  cb_id = doc.connect('before-save',
    function(doc, gfile)
      check_args_d(doc)
      check_args_f(gfile)
      seen_doc_before_save = seen_doc_before_save + 1
      return moo.SAVE_RESPONSE_CANCEL
    end)

  tassert(not editor.save_as(doc, si))
  tassert(seen_editor_before_save == 2)
  tassert(seen_doc_before_save == 1)
  tassert(not doc.save_as(si))
  tassert(seen_editor_before_save == 2)
  tassert(seen_doc_before_save == 2)

  doc.disconnect(cb_id)

  cb_id = editor.connect('before-save',
    function(editor, doc, gfile)
      check_args_ed(editor, doc)
      check_args_f(gfile)
      seen_editor_before_save = seen_editor_before_save + 1
      return moo.SAVE_RESPONSE_CONTINUE
    end)

  doc.connect('before-save',
    function(doc, gfile)
      check_args_d(doc)
      check_args_f(gfile)
      seen_doc_before_save = seen_doc_before_save + 1
      return moo.SAVE_RESPONSE_CONTINUE
    end)

  tassert(doc.save_as(si))
  tassert(seen_editor_before_save == 3)
  tassert(seen_doc_before_save == 3)
  tassert(doc.save())
  tassert(editor.save(doc))
  tassert(seen_editor_before_save == 5)
  tassert(seen_doc_before_save == 5)

  tassert(doc.close())

  editor.disconnect(cb_id)
end

test_will_close_window()
-- moo.spin_main_loop(0.1)
test_before_close_window()
-- moo.spin_main_loop(0.1)
test_bad_callback()
-- moo.spin_main_loop(0.1)
test_will_save()
-- moo.spin_main_loop(0.1)
test_will_close_doc()
-- moo.spin_main_loop(0.1)
test_before_save()
