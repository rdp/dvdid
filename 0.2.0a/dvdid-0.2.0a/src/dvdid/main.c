/* $Id: main.c 3094 2009-10-11 11:17:36Z chris $ */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dvdid/dvdid.h>


int main(int argc, char **argv) {

  uint64_t dvdid;
  dvdid_status_t rv;
  int errn;

  if (argc != 2) {
    fprintf(stderr, PACKAGE_NAME " " PACKAGE_VERSION "\nUsage %s [DVD Path]\n", argv[0]);
    return 1;
  }

  rv = dvdid_calculate(&dvdid, argv[1], &errn);

  if (rv == DVDID_STATUS_OK) {

    fprintf(stdout, "%08" PRIx32 "|%08" PRIx32 "\n", (uint32_t) (dvdid >> 32), (uint32_t) dvdid);
    return 0;

  } else {

    fprintf(stderr, "%s", dvdid_error_string(rv));
    if (errn != 0) { fprintf(stderr, ": %s", strerror(errn)); }
    fprintf(stderr, "\n");

    /* No longer set return value based upon failure reason, the codes were 
       broken and overlapped, and were unlikely to be useful */
    return 2;

  }

}

