# $Id: Makefile.am 1675 2009-03-20 18:13:37Z chris $

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)/html

doc_DATA = 

EXTRA_DIST = $(doc_DATA) api

# The install targets don't copy whole directories so we have to
# handle 'api/' specially:
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/api
	(cd $(srcdir)/api && $(INSTALL_DATA) * $(DESTDIR)$(docdir)/api)
uninstall-local:
	rm -rf $(DESTDIR)$(docdir)/api
maintainer-clean-local:
	rm -rf $(srcdir)/api
