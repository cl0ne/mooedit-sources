docparser_files =		\
	parsedocs.py		\
	mdp/__init__.py		\
	mdp/module.py		\
	mdp/docparser.py	\
	mdp/xmlwriter.py

gendefs_files =			\
	gendefs.py		\
	mpi/__init__.py		\
	mpi/module.py		\
	mpi/defswriter.py

genlua_files =			\
	genlua.py		\
	mpi/__init__.py		\
	mpi/module.py		\
	mpi/luawriter.py

gendocs_files =			\
	gendocs.py		\
	gendocbook.py		\
	mpi/__init__.py		\
	mpi/module.py		\
	mpi/docbookwriter.py

EXTRA_DIST =			\
	$(docparser_files)	\
	$(gendefs_files)	\
	$(genlua_files)		\
	$(gendocs_files)	\
	sourcefiles.mak		\
	moo.xml			\
	gtk.xml

BUILT_SOURCES =

if MOO_DEV_MODE

include sourcefiles.mak

BUILT_SOURCES += moo.xml.stamp
moo.xml.stamp: $(docparser_files) $(source_files)
	$(AM_V_GEN)$(MOO_PYTHON) $(srcdir)/parsedocs.py \
		--source-dir $(top_srcdir)/moo/mooapp \
		--source-dir $(top_srcdir)/moo/mooedit \
		--source-dir $(top_srcdir)/moo/moofileview \
		--source-dir $(top_srcdir)/moo/mooutils \
		--source-dir $(top_srcdir)/moo/moopython \
		--source-dir $(top_srcdir)/moo/plugins/usertools \
		--source-dir $(top_srcdir)/moo/plugins/support \
		--source-file $(top_srcdir)/moo/moolua/medit-lua.cpp \
		--source-file $(top_srcdir)/moo/moolua/medit-lua.h \
                --skip 'moofontsel.*' \
		--output moo.xml.tmp
	$(AM_V_at)cmp -s moo.xml.tmp $(srcdir)/moo.xml || mv moo.xml.tmp $(srcdir)/moo.xml
	$(AM_V_at)rm -f moo.xml.tmp
	$(AM_V_at)echo stamp > moo.xml.stamp

BUILT_SOURCES += gtk.xml.stamp
gtk.xml.stamp: $(docparser_files) $(top_srcdir)/moo/moolua/gtk-api.c $(top_srcdir)/moo/moolua/gtk-api.h
	$(AM_V_at)$(MKDIR_P) moolua
	$(AM_V_GEN)$(MOO_PYTHON) $(srcdir)/parsedocs.py \
		--source-file $(top_srcdir)/moo/moolua/gtk-api.c \
		--source-file $(top_srcdir)/moo/moolua/gtk-api.h \
		--module Gtk \
		--output gtk.xml.tmp
	$(AM_V_at)cmp -s gtk.xml.tmp $(srcdir)/gtk.xml || mv gtk.xml.tmp $(srcdir)/gtk.xml
	$(AM_V_at)rm -f gtk.xml.tmp
	$(AM_V_at)echo stamp > gtk.xml.stamp

endif
