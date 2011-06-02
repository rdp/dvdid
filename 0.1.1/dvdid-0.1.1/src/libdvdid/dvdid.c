/* $Id: dvdid.c 2819 2009-09-13 16:11:47Z chris $ */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crc64_table.h"

#include "dvdid/dvdid2.h"

/*
  If none of the following get defined then dvdid_calculate won't work, but 
  dvdid_calculate2 can still be used if the hashinfo data is pulled in from 
  elsewhere.
*/
#if defined(_WIN32)
#define DVDID_WINDOWS
#elif defined(__FreeBSD__)
#define DVDID_FREEBSD
#elif defined(__linux__)
#define DVDID_LINUX
#elif defined(__APPLE__)
#define DVDID_APPLE
#elif defined(__unix__)
#define DVDID_POSIX
#endif

#ifdef DVDID_WINDOWS
#include <errno.h>
#include <windows.h>
#endif

#if defined(DVDID_POSIX) || defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#endif

/* Internal version of dvdid_fileinfo_t with next member too */
typedef struct dvdid__fileinfo_s dvdid__fileinfo_t;


struct dvdid_hashinfo_s {
  dvdid__fileinfo_t *fi_first; /* First element in linked list of fileinfos */
  uint8_t *vmgi_buf;
  size_t vmgi_bufsize;
  uint8_t *vts01i_buf;
  size_t vts01i_bufsize;
};

struct dvdid__fileinfo_s {
  dvdid__fileinfo_t *next;
  
  uint64_t creation_time; /* Creation time as a win32 FILETIME */
  uint32_t size;  /* Lowest 32bits of filesize */
  char *name;     /* File Name */
};



#ifdef DVDID_WINDOWS
static dvdid_status_t dvdid_hashinfo_win32_createinit(const char *dvdpath, dvdid_hashinfo_t **hi, int *errn);
#endif
#if defined(DVDID_POSIX) || defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)
static dvdid_status_t dvdid_hashinfo_unixlike_createinit(const char *dvdpath, dvdid_hashinfo_t **hi, int *errn);
#endif
static void  dvdid_hashinfo_sortfiles(dvdid_hashinfo_t *hi);

static void crc64_fileinfo(uint64_t *crc64, const dvdid__fileinfo_t *fi);
static void crc64_buf(uint64_t *crc64, const uint8_t *buf, size_t buf_len);
static void crc64_uint32(uint64_t *crc64, uint32_t l);
static void crc64_uint64(uint64_t *crc64, uint64_t ll);

static void crc64_byte(uint64_t *crc64, uint8_t b);


DVDID_API(dvdid_status_t) dvdid_calculate(uint64_t *dvdid, const char* dvdpath, int *errn) {

  dvdid_hashinfo_t *hi;
  dvdid_status_t i;

#if defined(DVDID_WINDOWS)
  i = dvdid_hashinfo_win32_createinit(dvdpath, &hi, errn);
#elif  defined(DVDID_POSIX) || defined (DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)
  i = dvdid_hashinfo_unixlike_createinit(dvdpath, &hi, errn);
#else
  (void) dvdid; (void) dvdpath; hi = NULL;
  i = DVDID_STATUS_PLATFORM_UNSUPPORTED;
  if (errn != NULL){ *errn = 0; }
#endif
  if (i != DVDID_STATUS_OK) { return i; }

  i = dvdid_calculate2(dvdid, hi);
  if (i != DVDID_STATUS_OK) { dvdid_hashinfo_free(hi); return i; }

  dvdid_hashinfo_free(hi);

  return DVDID_STATUS_OK;
}

DVDID_API(dvdid_status_t) dvdid_calculate2(uint64_t *dvdid, const dvdid_hashinfo_t *hi) {

  dvdid__fileinfo_t *fi;

  (*dvdid) = UINT64_C(0xffffffffffffffff);

  fi = hi->fi_first;
  while (fi != NULL) {
    crc64_fileinfo(dvdid, fi);
    fi = fi->next;
  }

  crc64_buf(dvdid, hi->vmgi_buf, hi->vmgi_bufsize);
  crc64_buf(dvdid, hi->vts01i_buf, hi->vts01i_bufsize);

  return DVDID_STATUS_OK;
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_create(dvdid_hashinfo_t **hi) {
  *hi = malloc(sizeof **hi);
  if (*hi == NULL) { return DVDID_STATUS_MALLOC_ERROR; }

  (*hi)->fi_first = NULL;
  (*hi)->vmgi_buf = NULL;
  (*hi)->vts01i_buf = NULL;

  return DVDID_STATUS_OK;
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_addfile(dvdid_hashinfo_t *hi, const dvdid_fileinfo_t *fi) {

  dvdid__fileinfo_t **fi2;

  /* Get a pointer to the first NULL pointer in the linked list of fileinfos */
  fi2 = &(hi->fi_first);
  while (*fi2 != NULL) { fi2 = &((*fi2)->next); }

  *fi2 = malloc(sizeof(**fi2));
  if (*fi2 == NULL) { return DVDID_STATUS_MALLOC_ERROR; }

  (*fi2)->creation_time = fi->creation_time;
  (*fi2)->name = malloc(strlen(fi->name) + 1);
  if ((*fi2)->name == NULL) { free(*fi2); *fi2 = NULL; return DVDID_STATUS_MALLOC_ERROR; }
  strcpy((*fi2)->name, fi->name);
  (*fi2)->size = fi->size;
  (*fi2)->next = NULL;

  return DVDID_STATUS_OK;
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_set_vmgi(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size) {
  hi->vmgi_bufsize = (size > DVDID_HASHINFO_VXXI_MAXBUF) ? DVDID_HASHINFO_VXXI_MAXBUF : size;
  hi->vmgi_buf = malloc(hi->vmgi_bufsize);
  if (hi->vmgi_buf == NULL) { return DVDID_STATUS_MALLOC_ERROR; }
  memcpy(hi->vmgi_buf, buf, hi->vmgi_bufsize);
  return DVDID_STATUS_OK;
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_set_vts01i(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size) {
  hi->vts01i_bufsize = (size > DVDID_HASHINFO_VXXI_MAXBUF) ? DVDID_HASHINFO_VXXI_MAXBUF : size;
  hi->vts01i_buf = malloc(hi->vts01i_bufsize);
  if (hi->vts01i_buf == NULL) { return DVDID_STATUS_MALLOC_ERROR; }
  memcpy(hi->vts01i_buf, buf, hi->vts01i_bufsize);
  return DVDID_STATUS_OK;
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_init(dvdid_hashinfo_t *hi) {
  dvdid_hashinfo_sortfiles(hi);
  return DVDID_STATUS_OK;
}


DVDID_API(void) dvdid_hashinfo_free(dvdid_hashinfo_t *hi) {
  dvdid__fileinfo_t *fi, *fi_prev;

  if (hi->vmgi_buf != NULL) { free(hi->vmgi_buf); }
  if (hi->vts01i_buf != NULL) { free(hi->vts01i_buf); }

  fi = hi->fi_first;

  while (fi != NULL) {
    free(fi->name);

    fi_prev = fi;
    fi = fi->next;
    free(fi_prev);
  }

  free(hi);
}

static void  dvdid_hashinfo_sortfiles(dvdid_hashinfo_t *hi) {

  dvdid__fileinfo_t *fi, *fi_prev;

  /* Alpha sort file info by file names */
  /* TODO: Rigorous analysis of the linked list manipulation */
  do {
    fi_prev = NULL;
    fi = hi->fi_first;
    do {
      if (fi->next == NULL) { return; }
      if (strcmp(fi->name, fi->next->name) > 0) {
        if (fi_prev == NULL) {
          hi->fi_first = fi->next;
          fi->next = fi->next->next;
          hi->fi_first->next = fi;
        } else {
          fi_prev->next = fi->next;
          fi->next = fi->next->next;
          fi_prev->next->next = fi;
        }
        break;
      }
      fi_prev = fi;
      fi = fi->next;
    } while (1);
  } while (1);
}


#ifdef DVDID_WINDOWS
static dvdid_status_t dvdid_hashinfo_win32_createinit(const char *dvdpath, dvdid_hashinfo_t **hi, int *errn) {

  HANDLE file_list;
  WIN32_FIND_DATAW data;

  dvdid_fileinfo_t fi;

  WCHAR *dvdpath_wide;
  WCHAR *path_tmp;

  FILE *fp;

  dvdid_status_t r;
  int l;

  size_t s, vmgi_size, vts01i_size;
  uint8_t *buf;

  r = dvdid_hashinfo_create(hi);
  if (r != DVDID_STATUS_OK) {
    if (errn != NULL) { *errn = 0; }
    return r;
  }


  do {

    buf = malloc(DVDID_HASHINFO_VXXI_MAXBUF);

    if (buf == NULL) {
      r = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    do {

      l = MultiByteToWideChar(CP_UTF8, 0, dvdpath, -1, NULL, 0);
      dvdpath_wide = calloc(l, sizeof(*dvdpath_wide));

      if (dvdpath_wide == NULL) {
        r = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      do {

        MultiByteToWideChar(CP_UTF8, 0, dvdpath, -1, dvdpath_wide, l);

        path_tmp = calloc(wcslen(dvdpath_wide) + wcslen(L"\\VIDEO_TS\\*") + 1, sizeof(*path_tmp));
        if (path_tmp == NULL) {
          r = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        wcscpy(path_tmp, dvdpath_wide);
        wcscat(path_tmp, L"\\VIDEO_TS\\*");

        file_list = FindFirstFileW(path_tmp, &data);
        free(path_tmp);

        if (file_list == INVALID_HANDLE_VALUE) {
          r = DVDID_STATUS_READ_VIDEO_TS_ERROR;
          /*
            The error code for FirstFirstFile will be returned by GetLastError().  However, 
            these error codes are not compatible with error + strerror(), so attempt 
            conversion for the most likely error, and return 0 otherwise.
          */
          if (errn != NULL) {
            switch (GetLastError()) {
              case ERROR_FILE_NOT_FOUND:
              case ERROR_PATH_NOT_FOUND:
                *errn = ENOENT;
                break;

              case ERROR_ACCESS_DENIED:
                *errn = EACCES;
                break;

              case ERROR_DIRECTORY:
                *errn = ENOTDIR;
                break;

              default:
                *errn = 0;
                break;
            }
          }
          break;
        }

        /* Silence warnings */
        vmgi_size = 0; vts01i_size = 0;

        do {
          if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

            fi.creation_time = data.ftCreationTime.dwHighDateTime;
            fi.creation_time <<=32;
            fi.creation_time += data.ftCreationTime.dwLowDateTime;

            fi.size = data.nFileSizeLow;

            if (wcscmp(data.cFileName, L"VIDEO_TS.IFO") == 0) {
              vmgi_size = data.nFileSizeLow;
            } else if (wcscmp(data.cFileName, L"VTS_01_0.IFO") == 0) {
              vts01i_size = data.nFileSizeLow;
            }

            l = WideCharToMultiByte(CP_UTF8,0,data.cFileName,-1,NULL,0,NULL,FALSE);
            fi.name = malloc(l);
            if (fi.name == NULL) {
              r = DVDID_STATUS_MALLOC_ERROR;
              if (errn != NULL) { *errn = 0; }
              break;
            }
            WideCharToMultiByte(CP_UTF8,0,data.cFileName,-1,fi.name,l,NULL,FALSE);

            r = dvdid_hashinfo_addfile(*hi, &fi);
            free(fi.name);
            if (r != DVDID_STATUS_OK) {
              if (errn != NULL) { *errn = 0; }
              break;
            }

          }
        } while (FindNextFileW(file_list, &data));

        FindClose(file_list);

        if (r != DVDID_STATUS_OK) {
          break;
        }

        path_tmp = calloc(wcslen(dvdpath_wide) + wcslen(L"\\VIDEO_TS\\VIDEO_TS.IFO") + 1, sizeof(*path_tmp));
        if (path_tmp == NULL) {
          r = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        wcscpy(path_tmp, dvdpath_wide);
        wcscat(path_tmp, L"\\VIDEO_TS\\VIDEO_TS.IFO");
        fp = _wfopen(path_tmp, L"rb");
        free(path_tmp);

        if (fp == NULL) {
          r = DVDID_STATUS_READ_VMGI_ERROR;
          if (errn != NULL) { *errn = errno; }
          break;
        }

        if (vmgi_size > DVDID_HASHINFO_VXXI_MAXBUF) { vmgi_size = DVDID_HASHINFO_VXXI_MAXBUF; }

        s = fread(buf, 1, vmgi_size, fp);
        fclose(fp);

        r = dvdid_hashinfo_set_vmgi(*hi, buf, s);
        if (r != DVDID_STATUS_OK) {
          if (errn != NULL) { *errn = 0; }
          break;
        }

        if (s != vmgi_size) {
          r = DVDID_STATUS_READ_VMGI_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        path_tmp = calloc(wcslen(dvdpath_wide) + wcslen(L"\\VIDEO_TS\\VTS_01_0.IFO") + 1, sizeof(*path_tmp));
        if (path_tmp == NULL) {
          r = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        wcscpy(path_tmp, dvdpath_wide);
        wcscat(path_tmp, L"\\VIDEO_TS\\VTS_01_0.IFO");
        fp = _wfopen(path_tmp, L"rb");
        free(path_tmp);

        if (fp == NULL) {
          r = DVDID_STATUS_READ_VTS01I_ERROR;
          if (errn != NULL) { *errn = errno; }
          break;
        }

        if (vts01i_size > DVDID_HASHINFO_VXXI_MAXBUF) { vts01i_size = DVDID_HASHINFO_VXXI_MAXBUF; }

        s = fread(buf, 1, vts01i_size, fp);
        fclose(fp);

        if (s != vts01i_size) {
          r = DVDID_STATUS_READ_VTS01I_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        r = dvdid_hashinfo_set_vts01i(*hi, buf, s);
        if (r != DVDID_STATUS_OK) {
          if (errn != NULL) { *errn = 0; }
          break;
        }

      } while (0);

      free(dvdpath_wide);

    } while (0);

    free(buf);

  } while (0);

  if (r != DVDID_STATUS_OK) {
    dvdid_hashinfo_free(*hi);
  } else {
    dvdid_hashinfo_init(*hi);
  }

  return r;

}
#endif

#if defined(DVDID_POSIX) || defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)

#if defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)
#define TIMESPEC_TO_FILETIME(t) ((int64_t) ((((int64_t)(t).tv_sec) + 11644473600) * 10000000 + ((int64_t)(t).tv_nsec) / 100))
/* Convert from time_t to int64_t with an epoch of 1601-01-01 and ticks of 10MHz */
/* The POSIX spec guarantees that time_t has an epoch of 1970-01-01 00:00:00 UTC
   and 1 second ticks.  time_t can be integer or floating point, hence time immediate
   cast to int64_t. */
#else
#define TIME_T_TO_FILETIME(t) ((int64_t) ((((int64_t)(t)) + 11644473600) * 10000000))
#endif

static dvdid_status_t dvdid_hashinfo_unixlike_createinit(const char *dvdpath, dvdid_hashinfo_t **hi, int *errn) {

  DIR *dirlist;
  struct dirent direntry;
  struct dirent *data;
  struct stat st;

  dvdid_fileinfo_t fi;

  char *path_tmp, *cp;

  FILE *fp;

  int i;
  dvdid_status_t r;

  size_t s, vmgi_size, vts01i_size;
  uint8_t *buf;

  r = dvdid_hashinfo_create(hi);
  if (r != DVDID_STATUS_OK) {
    if (errn != NULL) { *errn = 0; }
    return r;
  }

  do {

    buf = malloc(DVDID_HASHINFO_VXXI_MAXBUF);
    if (buf == NULL) {
      r = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    do {

      path_tmp = malloc(strlen(dvdpath) + strlen("/VIDEO_TS") + 1);
      if (path_tmp == NULL) {
        r = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      strcpy(path_tmp, dvdpath);
      strcat(path_tmp, "/VIDEO_TS");

      dirlist = opendir(path_tmp);
      free(path_tmp);

      if (dirlist == NULL) {
        r = DVDID_STATUS_READ_VIDEO_TS_ERROR;
        if (errn != NULL) { *errn = errno; }
        break;
      }

      r = DVDID_STATUS_OK;

      while (1) {
        i = readdir_r(dirlist, &direntry, &data);
        if (i != 0) {
          r = DVDID_STATUS_READ_VIDEO_TS_ERROR;
          if (errn != NULL) { *errn = i; }
          break;
        } else if (data == NULL) {
          break;
        }

        path_tmp = malloc(strlen(dvdpath) + strlen("/VIDEO_TS/") + strlen(data->d_name) + 1);
        strcpy(path_tmp, dvdpath);
        strcat(path_tmp, "/VIDEO_TS/");
        strcat(path_tmp, data->d_name);
      
        if (path_tmp == NULL) {
          r = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }
        i = stat(path_tmp, &st);
        free(path_tmp);

        if (i != 0) {
          r = DVDID_STATUS_READ_VIDEO_TS_ERROR;
          if (errn != NULL) { *errn = errno; }
          break;
        }

        if (!S_ISDIR(st.st_mode)) {

	  /*
            We want to pass the same time as Windows uses for ftCreationTime.  In the absence
            of extended attributes (no tests have been performed on a disc with extended
            attriutes), Windows was observed to use the modification_time from 
            the udf filesystem for this parameter.  POSIX operating systems are most likely to
            make the modification_time available as the mtime.
	   
            Also, FILETIME has an accuracy of 100ns, and the UDF filesystem which windows 
            uses has an accuracy of 1us.  With standard POSIX, we only have an accuracy of 
            1s, but as the iso9660 filesystem also only has an accuracy of 1s, and the udf 
            filesystem is supposed to mirror it, the chances are that the timestamp won't 
            have been rounded and there won't be any problems.
          */

#if defined(DVDID_FREEBSD)
          /* FreeBSD (with __BSD_VISIBLE)*/
          fi.creation_time = TIMESPEC_TO_FILETIME(st.st_mtimespec);
#elif defined(DVDID_LINUX)
          /* Linux (untested, not sure which defines need to be set) */
          fi.creation_time = TIMESPEC_TO_FILETIME(st.st_mtim);
#elif defined(DVDID_APPLE)
          /* Apple */
          fi.creation_time = TIMESPEC_TO_FILETIME(st.st_mtimespec);
#else
          fi.creation_time = TIME_T_TO_FILETIME(st.st_mtime);
#endif

          fi.size = st.st_size;

          fi.name = malloc(strlen(data->d_name) + 1);
          if (fi.name == NULL) {
            r = DVDID_STATUS_MALLOC_ERROR;
            if (errn != NULL) { *errn = 0; }
            break;
          }
	  strcpy(fi.name, data->d_name);

          /* UDF if case sensitive, but iso9660 isn't.  The DVD standard mandates upper 
            case filename, and this will  hence be what goes into the UDF filesystem 
            which windows uses, and will hence be what's used when calculating the dvdid.
            However, at least some unix machines will be using the iso9660 filesystem, and 
            are thus free to show files using whichever case they choose.  We therefore 
            force the filename to uppercase here to keep consistent with windows. */
          for (cp = fi.name; *cp != '\0'; cp++) { *cp = toupper(*cp); }


	  /* Need to perform the strcmp after we've normalized the case */
          if (strcmp(fi.name, "VIDEO_TS.IFO") == 0) {
            vmgi_size = st.st_size;
          } else if (strcmp(fi.name, "VTS_01_0.IFO") == 0) {
            vts01i_size = st.st_size;
          }
	  

          r = dvdid_hashinfo_addfile(*hi, &fi);

          free(fi.name);
          if (r != DVDID_STATUS_OK) {
            if (errn != NULL) { *errn = 0; }
            break;
          }
      
        }
      }

      closedir(dirlist);

      if (r != DVDID_STATUS_OK) {
        break;
      }

      path_tmp = malloc(strlen(dvdpath) + strlen("/VIDEO_TS/VIDEO_TS.IFO") + 1);
      if (path_tmp == NULL) {
        r = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      strcpy(path_tmp, dvdpath);
      strcat(path_tmp, "/VIDEO_TS/VIDEO_TS.IFO");
      fp = fopen(path_tmp, "rb");
      free(path_tmp);

      if (fp == NULL) {
        r = DVDID_STATUS_READ_VMGI_ERROR;
        if (errn != NULL) { *errn = errno; }
        break;
      }

      if (vmgi_size > DVDID_HASHINFO_VXXI_MAXBUF) { vmgi_size = DVDID_HASHINFO_VXXI_MAXBUF; }

      s = fread(buf, 1, vmgi_size, fp);
      fclose(fp);

      if (s != vmgi_size) {
        r = DVDID_STATUS_READ_VMGI_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      r = dvdid_hashinfo_set_vmgi(*hi, buf, s);
      if (r != DVDID_STATUS_OK) {
        if (errn != NULL) { *errn = 0; }
        break;
      }


      path_tmp = malloc(strlen(dvdpath) + strlen("/VIDEO_TS/VTS_01_0.IFO") + 1);
      if (path_tmp == NULL) {
        r = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      strcpy(path_tmp, dvdpath);
      strcat(path_tmp, "/VIDEO_TS/VTS_01_0.IFO");
      fp = fopen(path_tmp, "rb");
      free(path_tmp);

      if (fp == NULL) {
        r = DVDID_STATUS_READ_VTS01I_ERROR;
        if (errn != NULL) { *errn = errno; }
        break;
      }

      if (vts01i_size > DVDID_HASHINFO_VXXI_MAXBUF) { vts01i_size = DVDID_HASHINFO_VXXI_MAXBUF; }

      s = fread(buf, 1, vts01i_size, fp);
      fclose(fp);

      if (s != vts01i_size) {
        r = DVDID_STATUS_READ_VTS01I_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      r = dvdid_hashinfo_set_vts01i(*hi, buf, s);
      if (r != DVDID_STATUS_OK) {
        if (errn != NULL) { *errn = 0; }
        break;
      }

      r = DVDID_STATUS_OK;

    } while (0);

    free(buf);

  } while (0);

  if (r != DVDID_STATUS_OK) {
    dvdid_hashinfo_free(*hi);
  } else {
    dvdid_hashinfo_init(*hi);
  }

  return r;

}
#endif


static void crc64_fileinfo(uint64_t *crc64, const dvdid__fileinfo_t *fi) {

  crc64_uint64(crc64, fi->creation_time);
  crc64_uint32(crc64, fi->size);
  crc64_buf(crc64, (uint8_t *) fi->name, strlen(fi->name) + 1); /* Include trailing nul */

}

static void crc64_buf(uint64_t *crc64, const uint8_t *buf, size_t buf_len) {
  size_t i;

  for (i = 0; i < buf_len; i++) { 
    crc64_byte(crc64, buf[i]); 
  }
}

static void crc64_uint32(uint64_t *crc64, uint32_t l) {
  int i;

  for (i = 0; i < 4; i++) {
    crc64_byte(crc64, (uint8_t) l);
    l >>= 8;
  }
}

static void crc64_uint64(uint64_t *crc64, uint64_t ll) {
  int i;

  for (i = 0; i < 8; i++) {
    crc64_byte(crc64, (uint8_t) ll);
    ll >>= 8;
  }
}

static void crc64_byte(uint64_t *crc64, uint8_t b) {
  (*crc64) = ((*crc64) >> 8) ^ crc64_table[((uint8_t) (*crc64)) ^ b];
}
