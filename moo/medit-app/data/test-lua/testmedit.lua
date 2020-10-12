require("munit")
require("_moo.os")

app = moo.App.instance()
editor = app.get_editor()

local __docs_to_cleanup = {}
local function add_doc_to_cleanup(doc)
  table.insert(__docs_to_cleanup, doc)
end
local function cleanup()
  for i, doc in pairs(__docs_to_cleanup) do
    doc.set_modified(false)
  end
end

local function test_active_window()
  w1 = editor.get_active_window()
  tassert(w1 ~= nil, 'editor.get_active_window() ~= nil')
  tassert(w1 == w1, 'w1 == w1')
  tassert(#editor.get_windows() == 1, 'one window')
  w2 = editor.new_window()
  tassert(w2 ~= nil, 'editor.new_window() ~= nil')
  tassert(w1 ~= w2, 'old window != new window')
  tassert(#editor.get_windows() == 2, 'two window')
  editor.set_active_window(w2)
  moo.spin_main_loop(0.3)
  tassert(w2 == editor.get_active_window(), 'w2 == editor.get_active_window()')
  if _moo.os.name == 'posix' then
    editor.set_active_window(w1)
    moo.spin_main_loop(0.3)
    tassert(w1 == editor.get_active_window(), 'w1 == editor.get_active_window()')
  end
  editor.close_window(w1)
  tassert(#editor.get_windows() == 1, 'two window')
  tassert(w2 == editor.get_active_window(), 'w2 == editor.get_active_window()')
end

local function test_selection()
  doc = editor.new_doc()
  add_doc_to_cleanup(doc)
  tassert(doc.get_text() == '')
  tassert(doc.get_start_pos().get_offset() == 1)
  tassert(doc.get_end_pos().get_offset() == 1)
  tassert(doc.get_text(1, 1) == '')
  doc.insert_text('a')
  doc.insert_text('b', 1)
  tassert(doc.get_text() == 'ba')
  tassert(doc.get_start_pos().get_offset() == 1)
  tassert(doc.get_end_pos().get_offset() == 3)
  tassert(doc.get_text(1, 2) == 'b')
  text = [[abcdefg
abcdefghij
1234567890
]]
  doc.replace_text(doc.get_start_pos(), doc.get_end_pos(), text)
  tassert(doc.get_start_pos().get_offset() == 1)
  tassert(doc.get_end_pos().get_offset() == #text + 1)
  doc.select_range(2, 3)
  tassert(doc.get_selected_text() == 'b')
  doc.select_range(3, 4)
  tassert(doc.get_selected_text() == 'c')
  doc.select_lines(1)
  tassert(doc.get_selected_text() == 'abcdefg\n')
end

-- test_active_window()
test_selection()
cleanup()
