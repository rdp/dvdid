/* $Id: dvdid2.h 1743 2009-03-21 20:04:26Z chris $ */

#ifndef DVDID__DVDID2_H
#define DVDID__DVDID2_H

#include <stdint.h>


#include "export.h"


#include "dvdid.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct dvdid_hashinfo_s dvdid_hashinfo_t;
typedef struct dvdid_fileinfo_s dvdid_fileinfo_t;


struct dvdid_fileinfo_s {
  uint64_t creation_time; /* creation time as a Win32 FILETIME */
  uint32_t size;  /* lowest 32bits of file size */
  char *name;     /* filename */
};


DVDID_API(dvdid_status_t) dvdid_calculate2(uint64_t *dvdid, const dvdid_hashinfo_t *hi);

/* Create a hashinfo struct.  Returns non-zero on error */
DVDID_API(dvdid_status_t) dvdid_hashinfo_create(dvdid_hashinfo_t **hi);

/* Add a file to the hashinfo struct.  The fileinfo will be copied, 
  and memory allocated as appropriate.  Returns non-zero on error, in
  which case dvdid_hashinfo_free must be called on the hashinfo struct
  as it's not guaranteed to be useable */
DVDID_API(dvdid_status_t) dvdid_hashinfo_addfile(dvdid_hashinfo_t *hi, const dvdid_fileinfo_t *fi);

/* Sets the pointer to the data read from VIDEO_TS.IFO.  This buffer 
  will be copied, so does not need to be valid until dvd_hashinfo_free is
  called */
/* We need at most the first DVDID_HASHINFO_VXXI_MAXBUF bytes of the file */
#define DVDID_HASHINFO_VXXI_MAXBUF 0x10000
DVDID_API(dvdid_status_t) dvdid_hashinfo_set_vmgi(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size);

/* As above for VTS_01_0.IFO */
DVDID_API(dvdid_status_t) dvdid_hashinfo_set_vts01i(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size);

/* Having added the necessary files and set pointers to the necessary buffers,
  perform any additional init work before dvdid_calculate2 gets called */
DVDID_API(dvdid_status_t) dvdid_hashinfo_init(dvdid_hashinfo_t *hi);

/* Free hashinfo struct one finished with */
DVDID_API(void) dvdid_hashinfo_free(dvdid_hashinfo_t *hi);


#ifdef __cplusplus
}
#endif


#endif
