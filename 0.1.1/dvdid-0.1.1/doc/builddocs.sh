#!/bin/sh
# $Id: builddocs.sh 384 2008-02-28 17:20:46Z chris $

rm -rf html/api doxytmp dvdid.tag 

if doxygen Doxyfile
then
    mv doxytmp/html html/api
    rm -rf doxytmp
else
    echo "*** Warning: Doxygen not found / failed; documentation will not be built."
    touch dvdid.tag
    mkdir -p html/api
    # For now, a there needs to be at least one file in html/api else
    # errors are generated when the empty documenation is installed.
    echo "Doxygen not found; documentation wasn't built." > html/api/README
fi
