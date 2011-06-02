# $Id: Makefile.am 1674 2009-03-20 18:06:17Z chris $
# Many thanks to Josh Coalson for the template for Doxygen support

SUBDIRS = . html

$(srcdir)/dvdid.tag: $(srcdir)/Doxyfile $(top_srcdir)/include/dvdid/dvdid.h
	cd $(srcdir) && ./builddocs.sh

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

doc_DATA = \
	$(srcdir)/dvdid.tag

EXTRA_DIST = \
	builddocs.sh \
	Doxyfile \
	$(doc_DATA)

maintainer-clean-local:
	rm -rf $(srcdir)/dvdid.tag $(srcdir)/api $(srcdir)/doxytmp
