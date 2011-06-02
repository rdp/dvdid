#!/bin/sh
# $Id: autogen.sh 1661 2009-03-20 17:27:34Z chris $

(cd doc && ./builddocs.sh)
autoreconf --install --force --verbose
