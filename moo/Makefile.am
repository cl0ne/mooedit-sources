EXTRA_DIST =
BUILT_SOURCES =
CLEANFILES =
bin_PROGRAMS =
noinst_LIBRARIES =

AM_CPPFLAGS = $(MOO_CPPFLAGS) -I$(top_srcdir)/doc/built -Imooutils/glade
AM_CFLAGS = $(MOO_CFLAGS)
AM_CXXFLAGS = $(MOO_CXXFLAGS)

moo_sources =
built_moo_sources =
moo_libadd =

plugins_sources =
built_plugins_sources =

moo_srcdir = $(srcdir)
moo_builddir = .

moo_sources += moo-config.h

moo_sources +=				\
	mooglib/moo-glib.h		\
	mooglib/moo-stat.h		\
	mooglib/moo-time.h
EXTRA_DIST +=                           \
	mooglib/moo-glib.cpp

if !MOO_OS_WIN32
moo_sources += mooglib/moo-glib.cpp
endif

EXTRA_DIST += marshals.list
built_moo_sources += marshals.h
marshals.h: marshals.list
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --prefix=_moo_marshal --header $(srcdir)/marshals.list > marshals.h.tmp \
		&& mv marshals.h.tmp marshals.h

include mooedit/Makefile.incl
include xdgmime/Makefile.incl
include mooutils/Makefile.incl
include moofileview/Makefile.incl
include gtksourceview/Makefile.incl
include eggsmclient/Makefile.incl
include mooapp/Makefile.incl
include moolua/Makefile.incl
include moopython/Makefile.incl
include plugins/Makefile.incl
include medit-app/Makefile.incl
include medit-module/Makefile.incl
include moocpp/Makefile.incl

BUILT_SOURCES += $(built_moo_sources) $(built_plugins_sources)
CLEANFILES += $(built_moo_sources) $(built_plugins_sources)

test:
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) test

# glade/%-gxml.h: glade/%.glade $(top_srcdir)/tools/glade2c.py
# 	$(MKDIR_P) glade
# 	$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

mooutils/%-gxml.h: mooutils/glade/%.glade $(top_srcdir)/tools/glade2c.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

moofileview/%-gxml.h: moofileview/glade/%.glade $(top_srcdir)/tools/glade2c.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

mooedit/%-gxml.h: mooedit/glade/%.glade $(top_srcdir)/tools/glade2c.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

mooapp/%-gxml.h: mooapp/glade/%.glade $(top_srcdir)/tools/glade2c.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

plugins/%-gxml.h: plugins/glade/%.glade $(top_srcdir)/tools/glade2c.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

plugins/usertools/%-gxml.h: plugins/usertools/glade/%.glade $(top_srcdir)/tools/glade2c.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/glade2c.py $< > $@.tmp && mv $@.tmp $@

# %-ui.h: %.xml $(top_srcdir)/tools/xml2h.py
# 	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/xml2h.py $< $@.tmp $*_ui_xml && mv $@.tmp $@

moofileview/%-ui.h: moofileview/%.xml $(top_srcdir)/tools/xml2h.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/xml2h.py $< $@.tmp $*_ui_xml && mv $@.tmp $@

mooedit/%-ui.h: mooedit/%.xml $(top_srcdir)/tools/xml2h.py
	$(AM_V_at) $(MKDIR_P) `dirname $@`
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/tools/xml2h.py $< $@.tmp $*_ui_xml && mv $@.tmp $@
