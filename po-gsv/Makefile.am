MSGMERGE = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) srcdir=$(srcdir) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE_GSV) --dist
GENPOT   = INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) srcdir=$(srcdir) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE_GSV) --pot
LINGUAS  = $(shell grep -v "^\#" $(srcdir)/LINGUAS)
POFILES  = $(shell LINGUAS="$(LINGUAS)"; for lang in $$LINGUAS; do printf "$$lang.po "; done)
CATALOGS = $(shell LINGUAS="$(LINGUAS)"; for lang in $$LINGUAS; do printf "$$lang.gmo "; done)

EXTRA_DIST = LINGUAS POTFILES.in POTFILES.skip $(POFILES)

SUFFIXES: .po .gmo .mo

.po.mo:
	$(AM_V_GEN)$(MSGFMT) -o $@ $<

.po.gmo:
	$(AM_V_GEN)file=`echo $* | sed 's,.*/,,'`.gmo \
	  && rm -f $$file && $(GMSGFMT) -o $$file $<

all: all-catalogs-$(ENABLE_NLS)
all-catalogs-yes: $(CATALOGS)
all-catalogs-no:
$(CATALOGS): $(srcdir)/Makefile.am

install-data-local: install-catalogs
install-catalogs: install-catalogs-$(ENABLE_NLS)
install-catalogs-no: all
install-catalogs-yes: all
	linguas="$(LINGUAS)"; \
	for lang in $$linguas; do \
	  dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $$dir; \
	  if test -r $$lang.gmo; then \
	    $(INSTALL_DATA) $$lang.gmo $$dir/$(GETTEXT_PACKAGE_GSV).mo; \
	    echo "installing $$lang.gmo as $$dir/$(GETTEXT_PACKAGE_GSV).mo"; \
	  else \
	    $(INSTALL_DATA) $(srcdir)/$$lang.gmo $$dir/$(GETTEXT_PACKAGE_GSV).mo; \
	    echo "installing $(srcdir)/$$lang.gmo as" \
		 "$$dir/$(GETTEXT_PACKAGE_GSV).mo"; \
	  fi; \
	  if test -r $$lang.gmo.m; then \
	    $(INSTALL_DATA) $$lang.gmo.m $$dir/$(GETTEXT_PACKAGE_GSV).mo.m; \
	    echo "installing $$lang.gmo.m as $$dir/$(GETTEXT_PACKAGE_GSV).mo.m"; \
	  else \
	    if test -r $(srcdir)/$$lang.gmo.m ; then \
	      $(INSTALL_DATA) $(srcdir)/$$lang.gmo.m \
		$$dir/$(GETTEXT_PACKAGE_GSV).mo.m; \
	      echo "installing $(srcdir)/$$lang.gmo.m as" \
		   "$$dir/$(GETTEXT_PACKAGE_GSV).mo.m"; \
	    else \
	      true; \
	    fi; \
	  fi; \
	done

uninstall-local:
	linguas="$(LINGUAS)"; \
	for lang in $$linguas; do \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(GETTEXT_PACKAGE_GSV).mo; \
	  rm -f $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(GETTEXT_PACKAGE_GSV).mo.m; \
	done

CLEANFILES = .intltool-merge-cache *.mo *.gmo POTFILES
