/* $Id: main.c 2871 2009-09-14 12:13:12Z chris $ */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dvdid/dvdid.h>


int main(int argc, char **argv) {

  uint64_t dvdid;
  dvdid_status_t i;
  int e;

  if (argc != 2) {
    fprintf(stderr, PACKAGE_NAME " " PACKAGE_VERSION "\nUsage %s [DVD Path]\n", argv[0]);
    return 1;
  }

  i = dvdid_calculate(&dvdid, argv[1], &e);

  switch (i) {
    case (DVDID_STATUS_OK):
      fprintf(stdout, "%08" PRIx32 "|%08" PRIx32, (uint32_t) (dvdid >> 32), (uint32_t) dvdid);
      return 0;

    case (DVDID_STATUS_PLATFORM_UNSUPPORTED):
      fprintf(stderr, "Platform not supported\n");
      return 2;

    case (DVDID_STATUS_MALLOC_ERROR):
      fprintf(stderr, "Out of memory\n");
      return 3;

      /* NB: strerror(0) should be safe */
    case (DVDID_STATUS_READ_VIDEO_TS_ERROR):
      fprintf(stderr, "Error reading VIDEO_TS: %s\n", strerror(e));
      return 3;

    case (DVDID_STATUS_READ_VMGI_ERROR):
      fprintf(stderr, "Error reading VIDEO_TS.IFO: %s\n", strerror(e));
      return 3;

    case (DVDID_STATUS_READ_VTS01I_ERROR):
      fprintf(stderr, "Error reading VTS_01_0.IFO: %s\n", strerror(e));
      return 3;

    default:
      fprintf(stderr, "An unknown error has occured (%d): %s\n", (int) i, strerror(e));
      return 4;

  }
}

