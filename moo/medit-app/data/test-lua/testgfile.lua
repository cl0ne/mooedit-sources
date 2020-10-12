require("munit")
os = require("_moo.os")

if os.name == 'nt' then
  name1 = 'c:\\tmp\\foo'
  name2 = 'c:\\tmp'
  uri1 = 'file:///c:/tmp/foo'
else
  name1 = '/tmp/foo'
  name2 = '/tmp'
  uri1 = 'file:///tmp/foo'
end

local function test1()
  f1 = gtk.GFile.new_for_path(name1)
  f2 = f1.dup()
  tassert(f1 ~= f2)
  tassert(f1.get_basename() == 'foo')
  tassert(f1.equal(f2))
  tassert(f1:equal(f2))
  d = f1.get_parent()
  tassert(d.get_path() == name2)
  c = d.get_child('foo')
  tassert(f1.equal(c))
  c = d.get_child_for_display_name('foo')
  tassert(f1.equal(c))
  tassert(f1.get_parse_name() == name1)
  tassert(d.get_relative_path(c) == 'foo')
  tassert(c.get_uri() == uri1)
  tassert(c.get_uri_scheme() == 'file')
  c.hash()
  tassert(c.has_parent())
  tassert(c.has_parent(d))
  tassert(d.has_parent())
  root = d.get_parent()
  tassert(d.has_parent(root))
  tassert(not root.has_parent())
  tassert(d.has_prefix(root))
  tassert(c.has_prefix(root))
  tassert(c.has_prefix(d))
  tassert(not c:has_prefix(c))
  tassert(not d.has_prefix(c))
  tassert(d.has_uri_scheme('file'))
  tassert(d.is_native())
  tassert(d.resolve_relative_path('foo').equal(f1))
  tassert(d.resolve_relative_path('../tmp').equal(d))
end

local function test2()
  f1 = gtk.GFile.new_for_uri('http://example.com/foo/bar.txt')
  f2 = f1.dup()
  tassert(f2 ~= nil)
  tassert(f1 ~= f2)
  tassert(f1.get_basename() == 'bar.txt')
  tassert(f1.equal(f2))
  tassert(f1:equal(f2))
  d = f1.get_parent()
  tassert(d.get_path() == nil)
  tassert(d.get_uri() == 'http://example.com/foo')
  c = d.get_child('bar.txt')
  tassert(f1.equal(c))
  c = d.get_child_for_display_name('bar.txt')
  tassert(f1.equal(c))
  tassert(f1.get_parse_name() == 'http://example.com/foo/bar.txt')
  tassert(c.get_uri_scheme() == 'http')
  c.hash()
  tassert(c.has_parent())
  tassert(c.has_parent(d))
  tassert(d.has_parent())
  root = d.get_parent()
  tassert(d.has_parent(root))
  tassert(not root.has_parent())
  tassert(d.has_uri_scheme('http'))
  tassert(not d.is_native())
  tassert(d.resolve_relative_path('bar.txt').equal(f1))
end

local function test3()
  f = gtk.GFile.parse_name(name1)
  f2 = gtk.GFile.new_for_path(name1)
  tassert(f.equal(f2))
end

local function test4()
  f1 = gtk.GFile.new_for_uri(uri1)
  f2 = gtk.GFile.new_for_path(name1)
  tassert(f1.equal(f2))
end

test1()
-- test2()
test3()
test4()
