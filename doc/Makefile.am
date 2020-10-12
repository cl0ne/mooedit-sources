BUILT_SOURCES =

if MOO_DEV_MODE

$(srcdir)/built/medit-defines.ent: medit-defines.ent.in $(top_builddir)/config.status
	$(AM_V_GEN)cd $(top_builddir) && ./config.status --silent --file=doc/medit-defines.ent
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	$(AM_V_at)mv medit-defines.ent $(srcdir)/built/

gendocbook_files =				\
	$(top_srcdir)/api/gendocbook.py		\
	$(top_srcdir)/api/mpi/__init__.py	\
	$(top_srcdir)/api/mpi/module.py		\
	$(top_srcdir)/api/mpi/docbookwriter.py

script_docbook_sources =			\
	built/medit-defines.ent			\
	built/script-python.docbook		\
	built/script-lua.docbook		\
	built/script-lua-gtk.docbook

$(srcdir)/built/script-python.docbook: $(gendocbook_files) script-python.tmpl.docbook $(top_srcdir)/api/moo.xml
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/api/gendocbook.py \
		--python --template $(srcdir)/script-python.tmpl.docbook \
		$(top_srcdir)/api/moo.xml > script-python.docbook.tmp && \
		mv script-python.docbook.tmp $(srcdir)/built/script-python.docbook

$(srcdir)/built/script-lua.docbook: $(gendocbook_files) script-lua.tmpl.docbook $(top_srcdir)/api/moo.xml $(top_srcdir)/api/gtk.xml
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/api/gendocbook.py \
		--lua --template $(srcdir)/script-lua.tmpl.docbook \
		--import $(top_srcdir)/api/gtk.xml \
		$(top_srcdir)/api/moo.xml > script-lua.docbook.tmp && \
		mv script-lua.docbook.tmp $(srcdir)/built/script-lua.docbook

$(srcdir)/built/script-lua-gtk.docbook: $(gendocbook_files) script-lua-gtk.tmpl.docbook $(top_srcdir)/api/gtk.xml
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	$(AM_V_GEN)$(MOO_PYTHON) $(top_srcdir)/api/gendocbook.py \
		--lua --template $(srcdir)/script-lua-gtk.tmpl.docbook \
		$(top_srcdir)/api/gtk.xml > script-lua-gtk.docbook.tmp && \
		mv script-lua-gtk.docbook.tmp $(srcdir)/built/script-lua-gtk.docbook

# $(srcdir)/help/script-python.html: built/script-python.docbook built/medit-defines.ent script.xsl
# 	$(AM_V_at)$(MKDIR_P) $(srcdir)/help/
# 	$(AM_V_GEN)$(XSLTPROC) --output script-python.html.tmp \
# 		$(srcdir)/script.xsl $(srcdir)/built/script-python.docbook \
# 		&& mv script-python.html.tmp $(srcdir)/help/script-python.html
#
# $(srcdir)/help/script-lua.html: built/script-lua.docbook built/medit-defines.ent script.xsl
# 	$(AM_V_at)$(MKDIR_P) $(srcdir)/help/
# 	$(AM_V_GEN)$(XSLTPROC) --output script-lua.html.tmp \
# 		$(srcdir)/script.xsl $(srcdir)/built/script-lua.docbook \
# 		&& mv script-lua.html.tmp $(srcdir)/help/script-lua.html
#
# $(srcdir)/help/script-lua-gtk.html: built/script-lua-gtk.docbook built/medit-defines.ent script.xsl
# 	$(AM_V_at)$(MKDIR_P) $(srcdir)/help/
# 	$(AM_V_GEN)$(XSLTPROC) --output script-lua-gtk.html.tmp \
# 		$(srcdir)/script.xsl $(srcdir)/built/script-lua-gtk.docbook \
# 		&& mv script-lua-gtk.html.tmp $(srcdir)/help/script-lua-gtk.html

$(srcdir)/help/script/index.html: $(script_docbook_sources) script-book.xsl
	$(AM_V_GEN)cd $(srcdir) && $(XSLTPROC) --xinclude script-book.xsl script.docbook

$(srcdir)/help/medit.css: medit.css
	$(AM_V_at)$(MKDIR_P) $(srcdir)/help/
	$(AM_V_GEN)cp $(srcdir)/medit.css $(srcdir)/help/

$(srcdir)/built/lgpl.no-fancy-chars: $(top_srcdir)/COPYING
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	tr -d '\014' < $(top_srcdir)/COPYING > lgpl.no-fancy-chars.tmp && \
		 mv lgpl.no-fancy-chars.tmp $(srcdir)/built/lgpl.no-fancy-chars

docbook_files =				\
	medit.docbook			\
	prefs.docbook			\
	regex.docbook			\
	user-tools.docbook		\
	license.docbook

docbook_sources =			\
	$(docbook_files)		\
	built/lgpl.no-fancy-chars	\
	built/medit-defines.ent

$(srcdir)/help/index.html: $(docbook_sources) medit.xsl medit-common.xsl
	$(AM_V_GEN)cd $(srcdir) && $(XSLTPROC) --xinclude medit.xsl medit.docbook

$(srcdir)/help/help.html: $(docbook_sources) medit-single.xsl medit-common.xsl
	$(AM_V_at)$(MKDIR_P) $(srcdir)/help/
	$(AM_V_GEN)$(XSLTPROC) --xinclude --output help.html.tmp $(srcdir)/medit-single.xsl $(srcdir)/medit.docbook && \
		mv help.html.tmp $(srcdir)/help/help.html

toc.xml: $(docbook_sources)
	$(AM_V_GEN)$(XSLTPROC) --output toc.xml --xinclude \
		--stringparam chunk.first.sections 1 \
		http://docbook.sourceforge.net/release/xsl/current/html/maketoc.xsl $(srcdir)/medit.docbook

moo-help-sections.h.stamp: $(docbook_files) toc.xml genhelpsectionsh.py
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	$(AM_V_GEN)$(MOO_PYTHON) $(srcdir)/genhelpsectionsh.py --toc=toc.xml --srcdir=$(srcdir) $(docbook_files) > moo-help-sections.h.tmp
	$(AM_V_at)cmp -s moo-help-sections.h.tmp $(srcdir)/built/moo-help-sections.h || \
		mv moo-help-sections.h.tmp $(srcdir)/built/moo-help-sections.h
	$(AM_V_at)rm -f moo-help-sections.h.tmp
	$(AM_V_at)echo stamp > moo-help-sections.h.stamp

png_files = prefs-file-filters.png prefs-file-selector.png
dest_png_files = $(addprefix $(srcdir)/help/img/,$(png_files))

$(srcdir)/help/img/%.png: img/%.png
	$(AM_V_at)$(MKDIR_P) $(srcdir)/help/img/
	$(AM_V_GEN)cp $(srcdir)/img/$*.png $(srcdir)/help/img/

all-am: doc
doc:						\
	$(srcdir)/help/index.html		\
	$(srcdir)/help/help.html		\
	$(srcdir)/help/script/index.html	\
	$(srcdir)/help/medit.css		\
	$(srcdir)/built/medit.1			\
	$(dest_png_files)			\
	moo-help-sections.h.stamp

$(srcdir)/built/man-medit.t2t: man-medit.t2t.in $(top_builddir)/config.status
	$(AM_V_at)$(MKDIR_P) $(srcdir)/built
	$(AM_V_GEN)cd $(top_builddir) && ./config.status --silent --file=doc/man-medit.t2t
	$(AM_V_at)mv man-medit.t2t $(srcdir)/built/man-medit.t2t

$(srcdir)/built/medit.1: built/man-medit.t2t
	$(AM_V_GEN)$(TXT2TAGS) --target=man --outfile=- $(srcdir)/built/man-medit.t2t | grep -v "cmdline: txt2tags" \
		> medit.1.tmp && mv medit.1.tmp $(srcdir)/built/medit.1

endif

EXTRA_DIST = help built/moo-help-sections.h built/medit.1

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(MOO_HELP_DIR)/img $(DESTDIR)$(MOO_HELP_DIR)/script
	cd $(srcdir) && $(INSTALL_DATA) help/*.html help/*.css $(DESTDIR)$(MOO_HELP_DIR)
	cd $(srcdir) && $(INSTALL_DATA) help/script/*.html $(DESTDIR)$(MOO_HELP_DIR)/script
	cd $(srcdir) && $(INSTALL_DATA) help/img/*.png $(DESTDIR)$(MOO_HELP_DIR)/img

uninstall-local:
	rm -f $(DESTDIR)$(MOO_HELP_DIR)/*.html \
		$(DESTDIR)$(MOO_HELP_DIR)/*.css \
		$(DESTDIR)$(MOO_HELP_DIR)/script/*.html \
		$(DESTDIR)$(MOO_HELP_DIR)/img/*.png

if MOO_OS_UNIX
if MOO_BUILD_APP
dist_man_MANS = built/medit.1
endif MOO_BUILD_APP
endif MOO_OS_UNIX
