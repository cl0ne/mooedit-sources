ACLOCAL_AMFLAGS = -I m4 $(ACLOCAL_FLAGS)

SUBDIRS = po po-gsv api doc moo

EXTRA_DIST =				\
	tools/genenums.py		\
	tools/glade2c.py		\
	tools/xml2h.py

DISTCHECK_CONFIGURE_FLAGS =
if MOO_DEV_MODE
DISTCHECK_CONFIGURE_FLAGS += MAKEFLAGS=-j3
endif
if MOO_STRICT_MODE
DISTCHECK_CONFIGURE_FLAGS += --enable-strict
endif

CLEANFILES =

include plat/win32/Makefile.incl

if MOO_OS_WIN32
CLEANFILES += plat/win32/installer.iss
plat/win32/installer.iss: $(top_srcdir)/plat/win32/installer.iss.in $(top_builddir)/config.status
	$(AM_V_at)$(MKDIR_P) plat/win32
	$(AM_V_GEN)cd $(top_builddir) && ./config.status --silent --file=plat/win32/installer.iss
installer: plat/win32/installer.iss install
	$(MEDIT_INNO_COMPILER) plat/win32/installer.iss
copy:
	cp moo/medit.exe "/win/Program Files (x86)/medit/bin/"
bd_tmpdir = medit-win-dist-tmp
bd_distdir = $(PACKAGE)-$(VERSION)
bd_pzip = $(PACKAGE)-portable-$(VERSION)
bd_zip = $(PACKAGE)-$(VERSION)
portable: install
	rm -fr $(bd_tmpdir)
	mkdir $(bd_tmpdir)
	cp -lR ${prefix} $(bd_tmpdir)/$(bd_distdir)
	cd $(bd_tmpdir) && zip -r9 $(bd_zip).zip $(bd_distdir)
	echo "This file enables portable mode for medit" > $(bd_tmpdir)/$(bd_distdir)/bin/$(MEDIT_PORTABLE_MAGIC_FILE_NAME)
	cd $(bd_tmpdir) && zip -r9 $(bd_pzip).zip $(bd_distdir)
	mv $(bd_tmpdir)/$(bd_pzip).zip $(bd_tmpdir)/$(bd_zip).zip .
	rm -fr $(bd_tmpdir)
bdist: installer portable
endif

test:
	$(MAKE) $(AM_MAKEFLAGS) all
	$(MAKE) $(AM_MAKEFLAGS) check
	$(MAKE) $(AM_MAKEFLAGS) uninstall
	$(MAKE) $(AM_MAKEFLAGS) check
	$(MAKE) $(AM_MAKEFLAGS) install
	$(MAKE) $(AM_MAKEFLAGS) installcheck
fullcheck:
	$(MAKE) $(AM_MAKEFLAGS) test
	$(MAKE) $(AM_MAKEFLAGS) distcheck

@MOO_PO_SUBDIRS_RULE@
