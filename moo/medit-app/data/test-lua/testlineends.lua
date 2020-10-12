require("munit")
require("_moo.os")

editor = moo.Editor.instance()

tassert(moo.LE_NATIVE ~= nil)
tassert(moo.LE_UNIX ~= nil)
tassert(moo.LE_WIN32 ~= nil)
tassert(_moo.os.name == 'nt' or moo.LE_NATIVE ~= moo.LE_WIN32)
tassert(_moo.os.name ~= 'nt' or moo.LE_NATIVE == moo.LE_WIN32)

text_unix = 'line1\nline2\nline3\n'
text_win32 = 'line1\r\nline2\r\nline3\r\n'
text_mix = 'line1\nline2\r\nline3\r\n'
if _moo.os.name == 'nt' then
  text_native = text_win32
else
  text_native = text_unix
end

function read_file(filename)
  local f = assert(io.open(filename, 'rb'))
  local t = f:read("*all")
  f:close()
  return t
end

function save_file(filename, content)
  local f = assert(io.open(filename, 'wb'))
  f:write(content)
  f:close()
end

function test_default()
  doc = editor.new_doc()
  filename = moo.tempnam()
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)
  doc.set_text(text_unix)
  tassert(doc.save_as(moo.SaveInfo.new(filename)))
  tassert(read_file(filename) == text_native)
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)
  doc.set_text(text_win32)
  tassert(doc.save_as(moo.SaveInfo.new(filename)))
  tassert(read_file(filename) == text_native)
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)
  doc.set_modified(false)
  editor.close_doc(doc)
end

function test_set()
  doc = editor.new_doc()
  filename = moo.tempnam()

  tassert(doc.get_line_end_type() == moo.LE_NATIVE)
  doc.set_text(text_unix)
  tassert(doc.save_as(moo.SaveInfo.new(filename)))
  tassert(read_file(filename) == text_native)
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)

  doc.set_line_end_type(moo.LE_UNIX)
  tassert(doc.get_line_end_type() == moo.LE_UNIX)
  tassert(doc.save_as(moo.SaveInfo.new(filename)))
  tassert(read_file(filename) == text_unix)
  tassert(doc.get_line_end_type() == moo.LE_UNIX)

  doc.set_line_end_type(moo.LE_WIN32)
  tassert(doc.get_line_end_type() == moo.LE_WIN32)
  tassert(doc.save_as(moo.SaveInfo.new(filename)))
  tassert(read_file(filename) == text_win32)
  tassert(doc.get_line_end_type() == moo.LE_WIN32)

  doc.set_modified(false)
  editor.close_doc(doc)
end

function test_load()
  filename = moo.tempnam()

  save_file(filename, text_unix)
  doc = editor.open_file(moo.OpenInfo.new(filename))
  tassert(doc.get_filename() == filename)
  tassert(doc.get_line_end_type() == moo.LE_UNIX)
  doc.close()

  save_file(filename, text_win32)
  doc = editor.open_file(moo.OpenInfo.new(filename))
  tassert(doc.get_filename() == filename)
  tassert(doc.get_line_end_type() == moo.LE_WIN32)
  doc.close()
end

function test_mix()
  filename = moo.tempnam()

  save_file(filename, text_mix)
  doc = editor.open_file(moo.OpenInfo.new(filename))
  tassert(doc.get_filename() == filename)
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)

  tassert(doc.save())
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)
  tassert(read_file(filename) == text_unix)

  save_file(filename, text_mix)
  tassert(doc.reload())
  tassert(doc.get_line_end_type() == moo.LE_NATIVE)
  doc.set_line_end_type(moo.LE_WIN32)
  tassert(doc.save())
  tassert(doc.get_line_end_type() == moo.LE_WIN32)
  tassert(read_file(filename) == text_win32)

  doc.close()
end

test_default()
test_set()
test_load()
test_mix()
