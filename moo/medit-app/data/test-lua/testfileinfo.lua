require("munit")

editor = moo.Editor.instance()

filenames = { moo.tempnam(), moo.tempnam(), moo.tempnam() }
encs = { 'UTF-8', 'UTF-16', 'UCS-4' }

for i = 1,3 do
  if i == 1 then
    oi1 = moo.OpenInfo.new(filenames[i]).dup()
    oi2 = moo.OpenInfo.new(filenames[i], encs[i]).dup()
    si = moo.SaveInfo.new(filenames[i], encs[i]).dup()
  elseif i == 2 then
    oi1 = moo.OpenInfo.new_file(gtk.GFile.new_for_path(filenames[i])).dup()
    oi2 = moo.OpenInfo.new_file(gtk.GFile.new_for_path(filenames[i]), encs[i]).dup()
    si = moo.SaveInfo.new_file(gtk.GFile.new_for_path(filenames[i]), encs[i]).dup()
  else
    oi1 = moo.OpenInfo.new_uri(gtk.GFile.new_for_path(filenames[i]).get_uri()).dup()
    oi2 = moo.OpenInfo.new_uri(gtk.GFile.new_for_path(filenames[i]).get_uri(), encs[i]).dup()
    si = moo.SaveInfo.new_uri(gtk.GFile.new_for_path(filenames[i]).get_uri(), encs[i]).dup()
  end

  ri = moo.ReloadInfo.new(encs[i]).dup()

  doc = editor.new_file(oi1)
  doc.set_text('blah')
  tassert(doc.save())
  tassert(doc.save_as(si))
  tassert(doc.close())
  doc = editor.open_file(oi2)
  tassert(doc.get_text() == 'blah')
  tassert(doc.reload())
  tassert(doc.reload(ri))
  tassert(doc.close())
end

oi = moo.OpenInfo.new(filenames[1])
tassert(oi.get_flags() == 0)
oi.set_flags(moo.OPEN_FLAG_NEW_WINDOW)
tassert(oi.get_flags() == moo.OPEN_FLAG_NEW_WINDOW)
oi.add_flags(moo.OPEN_FLAG_RELOAD)
tassert(oi.get_flags() == moo.OPEN_FLAG_NEW_WINDOW + moo.OPEN_FLAG_RELOAD)
tassert(oi.get_line() == 0)
oi.set_line(14)
tassert(oi.get_line() == 14)

oi = moo.OpenInfo.new{filenames[1], flags=moo.OPEN_FLAG_NEW_WINDOW, line=23, encoding='UTF-8'}
tassert(oi.get_flags() == moo.OPEN_FLAG_NEW_WINDOW)
tassert(oi.get_line() == 23)
tassert(oi.get_encoding() == 'UTF-8')
tassert(oi.get_filename() == filenames[1])

ri = moo.ReloadInfo.new()
tassert(ri.get_line() == 0)
ri.set_line(5)
tassert(ri.get_line() == 5)
