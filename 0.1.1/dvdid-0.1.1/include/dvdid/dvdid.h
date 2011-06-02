/* $Id: dvdid.h 2192 2009-06-11 17:04:14Z chris $ */

#ifndef DVDID__DVDID_H
#define DVDID__DVDID_H


#include <stdint.h>


#include "export.h"


#ifdef __cplusplus
extern "C" {
#endif


enum dvdid_status_e {
  DVDID_STATUS_OK = 0,
  DVDID_STATUS_MALLOC_ERROR,

  /* Error that should only be returned by dvdid_calculate (but not test of API) */
  DVDID_STATUS_PLATFORM_UNSUPPORTED,
  DVDID_STATUS_READ_VIDEO_TS_ERROR,
  DVDID_STATUS_READ_VMGI_ERROR,
  DVDID_STATUS_READ_VTS01I_ERROR,
};


typedef enum dvdid_status_e dvdid_status_t;

/*
  If unsucessful, errn will be set to a platform specific error number, or zero if no
  such information is available.  If errn is NULL, the parameter will be ignored.
*/
DVDID_API(dvdid_status_t) dvdid_calculate(uint64_t *dvdid, const char* dvdpath, int *errn);


#ifdef __cplusplus
}
#endif


#endif
