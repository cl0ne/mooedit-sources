require('munit')

root = 'lua/fake/key/root'
count = 0

function new_key()
  count = count + 1
  return root .. '/key' .. tostring(count)
end

key = new_key()
moo.prefs_new_key_bool(key)
tassert(moo.prefs_get_bool(key) == false)
moo.prefs_set_bool(key, true)
tassert(moo.prefs_get_bool(key) == true)
moo.prefs_set_bool(key, false)

key = new_key()
moo.prefs_new_key_bool(key, true)
tassert(moo.prefs_get_bool(key) == true)
moo.prefs_set_bool(key, false)
tassert(moo.prefs_get_bool(key) == false)
moo.prefs_set_bool(key, true)

key = new_key()
moo.prefs_new_key_int(key)
tassert(moo.prefs_get_int(key) == 0)
moo.prefs_set_int(key, 8)
tassert(moo.prefs_get_int(key) == 8)
moo.prefs_set_int(key, 0)

key = new_key()
moo.prefs_new_key_int(key, 15)
tassert(moo.prefs_get_int(key) == 15)
moo.prefs_set_int(key, 8)
tassert(moo.prefs_get_int(key) == 8)
moo.prefs_set_int(key, 15)

key = new_key()
moo.prefs_new_key_string(key)
tassert(moo.prefs_get_string(key) == nil)
moo.prefs_set_string(key, 'blah')
tassert(moo.prefs_get_string(key) == 'blah')
moo.prefs_set_string(key, nil)
tassert(moo.prefs_get_string(key) == nil)

key = new_key()
moo.prefs_new_key_string(key, 'foo')
tassert(moo.prefs_get_string(key) == 'foo')
moo.prefs_set_string(key, 'blah')
tassert(moo.prefs_get_string(key) == 'blah')
moo.prefs_set_string(key, nil)
tassert(moo.prefs_get_string(key) == nil)
moo.prefs_set_string(key, 'foo')

key = new_key()
moo.prefs_new_key_string(key)
tassert(moo.prefs_get_filename(key) == nil)
moo.prefs_set_string(key, '/tmp/foo')
tassert(moo.prefs_get_filename(key) == '/tmp/foo')
moo.prefs_set_string(key, 'file:///tmp/foo')
tassert(moo.prefs_get_file(key).get_path() == '/tmp/foo')
moo.prefs_set_file(key, gtk.GFile.new_for_path('/tmp/bar'))
tassert(moo.prefs_get_file(key).equal(gtk.GFile.new_for_path('/tmp/bar')))
tassert(moo.prefs_get_string(key) == 'file:///tmp/bar')
moo.prefs_set_filename(key, '/tmp/baz')
tassert(moo.prefs_get_filename(key) == '/tmp/baz')
tassert(moo.prefs_get_string(key) == '/tmp/baz')
moo.prefs_set_string(key, nil)
