require("munit")

dir = moo.tempdir()
filename = moo.tempnam()
tassert(gtk.GFile.new_for_path(filename).get_parent().get_path() == dir)

