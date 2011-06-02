# $Id: Makefile.am 1676 2009-03-20 18:20:15Z chris $

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc include m4 src

EXTRA_DIST = \
	autogen.sh \
	dvdid.sln


WINDIST_DISTFILE=$(PACKAGE)-$(VERSION)
WINDIST_DISTFILE_DEV=$(PACKAGE)-$(VERSION)-dev

# We build the documentation and this archive on a unix machine, but
# the Windows binaries on a Windows machine, and hence need to combine
# files from different locations.  This path should points to the dir 
# where the compiled windows binaries are stored.
WINDIST_BINPATH=@WINDIST_BINPATH@

WINDIST_CONFIGS=@WINDIST_CONFIG@
WINDIST_PLATFORMS=@WINDIST_PLATFORM@

WINDIST_CONFIGS_DEV=@WINDIST_DEV_CONFIG@
WINDIST_PLATFORMS_DEV=@WINDIST_DEV_PLATFORM@

windist:
	if [ "$(ZIP)" = "no" ]; then \
		echo "zip not found during configure"; \
	else \
		for PLATFORM in $(WINDIST_PLATFORMS); do \
			DISTFILE="$(WINDIST_DISTFILE)-$$(echo -n $$PLATFORM | tr [A-Z] [a-z]).zip"; \
			echo "$$DISTFILE:"; \
			rm -f "$$DISTFILE"; \
			for CONFIG in $(WINDIST_CONFIGS); do \
				cd "$(WINDIST_BINPATH)/dvdid/$$PLATFORM/$$CONFIG" && $(ZIP) "../../../../$$DISTFILE" dvdid.exe && cd "../../../.."; \
			done; \
		done; \
	fi


windist-dev:
	if [ "$(ZIP)" = "no" ]; then \
		echo "zip not found during configure"; \
	else \
		DISTFILE="$(WINDIST_DISTFILE_DEV).zip"; \
		echo "$$DISTFILE:"; \
		rm -f "$$DISTFILE"; \
		$(ZIP) "$$DISTFILE" doc/html/api/*; \
		$(ZIP) "$$DISTFILE" include/dvdid/*.h; \
		for PLATFORM in $(WINDIST_PLATFORMS_DEV); do \
			for CONFIG in $(WINDIST_CONFIGS_DEV); do \
				$(ZIP) "$$DISTFILE" "$(WINDIST_BINPATH)/dvdid.lib/$$PLATFORM/$$CONFIG/dvdid.lib"; \
				$(ZIP) "$$DISTFILE" "$(WINDIST_BINPATH)/dvdid.dll/$$PLATFORM/$$CONFIG/dvdid.dll"; \
				$(ZIP) "$$DISTFILE" "$(WINDIST_BINPATH)/dvdid.dll/$$PLATFORM/$$CONFIG/dvdid.dll.lib"; \
			done; \
		done; \
	fi


