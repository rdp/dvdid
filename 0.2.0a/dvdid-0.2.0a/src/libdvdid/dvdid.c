/* $Id: dvdid.c 3259 2009-10-16 23:29:54Z chris $ */

#include <assert.h>
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

#if defined(DVDID_WINDOWS)
#include <errno.h>
#include <windows.h>
#define DIR_SEP "\\"
#define W_DIR_SEP L"\\"
#endif

#if defined(DVDID_FREEBSD)
#include <sys/param.h> /* for __FreeBSD_VERSION */
#endif

#if defined(DVDID_POSIX) || defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#define DIR_SEP "/"
#endif

/* Internal version of dvdid_fileinfo_t with next member too */
typedef struct dvdid__fileinfo_s dvdid__fileinfo_t;

typedef struct dvdid__medium_spec_s dvdid__medium_spec_t;
typedef struct dvdid__medium_spec_dir_s dvdid__medium_spec_dir_t;
typedef struct dvdid__medium_spec_file_s dvdid__medium_spec_file_t;


#define DVDID_MAX_DIRS 7
#define DVDID_MAX_FILES 3
struct dvdid_hashinfo_s {
  dvdid_medium_t medium;
  dvdid__fileinfo_t *fi_first[DVDID_MAX_DIRS]; /* First element in linked list of fileinfos */
  uint8_t *data_buf[DVDID_MAX_FILES];
  size_t data_size[DVDID_MAX_FILES];
};

struct dvdid__fileinfo_s {
  dvdid__fileinfo_t *next;
  
  uint64_t creation_time; /* Creation time as a win32 FILETIME */
  uint32_t size;  /* Lowest 32bits of filesize */
  char *name;     /* File Name */
};

struct dvdid__medium_spec_dir_s {
  const dvdid_dir_t dir;
  const char *const path;
  const int optional;
  dvdid_status_t (*const fixup_size)(dvdid_medium_t medium, dvdid_dir_t dir, dvdid_fileinfo_t *fi);
  const dvdid_status_t err;
};

struct dvdid__medium_spec_file_s {
  const dvdid_file_t file;
  const char *const path;
  const int optional;
  const size_t max_size;
  const dvdid_status_t err;
};

struct dvdid__medium_spec_s {
  const dvdid_medium_t medium;

  const int dir_count;
  const dvdid__medium_spec_dir_t dir[DVDID_MAX_DIRS];

  const int file_count;
  const dvdid__medium_spec_file_t file[DVDID_MAX_FILES];
};



const dvdid__medium_spec_t *const dvdid_speclist[];

static dvdid_status_t hashinfo_createinit(const char *path, dvdid_medium_t medium, dvdid_hashinfo_t **hi, int *errn);

#if defined(DVDID_WINDOWS)

static dvdid_status_t detect_medium_win32(const char *path, const  dvdid__medium_spec_t *const *spec_first, dvdid_medium_t *medium, int *errn);
static dvdid_status_t hashinfo_add_dir_files_win32(const char *path, const dvdid__medium_spec_dir_t *spec, dvdid_hashinfo_t *hi, int *errn);
static dvdid_status_t hashinfo_add_file_by_path_win32(const char *path, const dvdid__medium_spec_file_t *spec, dvdid_hashinfo_t *hi, int *errn);
#define detect_medium detect_medium_win32
#define hashinfo_add_dir_files hashinfo_add_dir_files_win32
#define hashinfo_add_file_by_path hashinfo_add_file_by_path_win32

#elif defined(DVDID_POSIX) || defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)

static dvdid_status_t detect_medium_unixlike(const char *path, const  dvdid__medium_spec_t *const *spec_first, dvdid_medium_t *medium, int *errn);
static dvdid_status_t hashinfo_add_dir_files_unixlike(const char *path, const dvdid__medium_spec_dir_t *spec, dvdid_hashinfo_t *hi, int *errn);
static dvdid_status_t hashinfo_add_file_by_path_unixlike(const char *path, const dvdid__medium_spec_file_t *spec, dvdid_hashinfo_t *hi, int *errn);
#define detect_medium detect_medium_unixlike
#define hashinfo_add_dir_files hashinfo_add_dir_files_unixlike
#define hashinfo_add_file_by_path hashinfo_add_file_by_path_unixlike

#else

static dvdid_status_t detect_medium_void(const char *path, const  dvdid__medium_spec_t *const *spec_first, dvdid_medium_t *medium, int *errn);
static dvdid_status_t hashinfo_add_dir_files_void(const char *path, const dvdid__medium_spec_dir_t *spec, dvdid_hashinfo_t *hi, int *errn);
static dvdid_status_t hashinfo_add_file_by_path_void(const char *path, const dvdid__medium_spec_file_t *spec, dvdid_hashinfo_t *hi, int *errn);
#define detect_medium detect_medium_void
#define hashinfo_add_dir_files hashinfo_add_dir_files_void
#define hashinfo_add_file_by_path hashinfo_add_file_by_path_void

#endif

static dvdid_status_t vcd_fixup_size(dvdid_medium_t medium, dvdid_dir_t dir, dvdid_fileinfo_t *fi);

static void fileinfo_list_sort(dvdid__fileinfo_t **fi_first);
static dvdid_status_t fileinfo_list_append(dvdid__fileinfo_t **fi_first, const dvdid_fileinfo_t *fi);
static dvdid_status_t copyset_data(uint8_t **d_data, size_t *d_size, size_t max_size, const uint8_t *buf, size_t size);

static void crc64_fileinfo(uint64_t *crc64, const dvdid__fileinfo_t *fi);
static void crc64_buf(uint64_t *crc64, const uint8_t *buf, size_t buf_len);
static inline void crc64_uint32(uint64_t *crc64, uint32_t l);
static inline void crc64_uint64(uint64_t *crc64, uint64_t ll);

static inline void crc64_init(uint64_t *crc64);
static inline void crc64_done(uint64_t *crc64);
static inline void crc64_byte(uint64_t *crc64, uint8_t b);

#define STRTOUPPER(string) do { char *c; for (c = (string); *c != '\0'; c++) { *c = (char) toupper(*c); } } while (0)


/* Main API functions */

DVDID_API(dvdid_status_t) dvdid_calculate(uint64_t *discid, const char* path, int *errn) {

  dvdid_medium_t medium;
  dvdid_hashinfo_t *hi;
  dvdid_status_t rv;

  rv = detect_medium(path, dvdid_speclist, &medium, errn);
  if (rv != DVDID_STATUS_OK) { return rv; }

  rv = hashinfo_createinit(path, medium, &hi, errn);
  if (rv != DVDID_STATUS_OK) { return rv; }

  rv = dvdid_calculate2(discid, hi);
  if (rv != DVDID_STATUS_OK) { *errn = 0; }

  dvdid_hashinfo_free(hi);

  return rv;

}

DVDID_API(dvdid_status_t) dvdid_calculate2(uint64_t *discid, const dvdid_hashinfo_t *hi) {

  int i;
  const dvdid__fileinfo_t *fi;

  crc64_init(discid);

  for (i = 0; i < DVDID_MAX_DIRS; i++) {
    fi = hi->fi_first[i];
    while (fi != NULL) {

#ifdef DVDID_DEBUG
      fprintf(stderr, "fi: %s %ld %lld\n", fi->name, (long) (fi->size), (long long) (fi->creation_time));
#endif

      crc64_fileinfo(discid, fi);
      fi = fi->next;
    }
  }

  for (i = 0; i < DVDID_MAX_FILES; i++) {
    if (hi->data_buf[i] != NULL) {

#ifdef DVDID_DEBUG
      fprintf(stderr, "fd: %02x %02x %02x %02x ... (%ld bytes)\n"
              , (unsigned int) (hi->data_buf[i][0])
              , (unsigned int) (hi->data_buf[i][1])
              , (unsigned int) (hi->data_buf[i][2])
              , (unsigned int) (hi->data_buf[i][3])
              , (long) (hi->data_size[i]));
#endif

      crc64_buf(discid, hi->data_buf[i], hi->data_size[i]);
    }
  }

  crc64_done(discid);

  return DVDID_STATUS_OK;

}

DVDID_API(dvdid_status_t) dvdid_hashinfo_create(dvdid_hashinfo_t **hi) {

  int i;

  *hi = malloc(sizeof **hi);
  if (*hi == NULL) { return DVDID_STATUS_MALLOC_ERROR; }

  (*hi)->medium = DVDID_MEDIUM_DVD;

  for (i = 0; i < DVDID_MAX_DIRS; i++) {
    (*hi)->fi_first[i] = NULL;
  }

  for (i = 0; i < DVDID_MAX_FILES; i++) {
    (*hi)->data_buf[i] = NULL;
  }

  return DVDID_STATUS_OK;

}

DVDID_API(dvdid_status_t) dvdid_hashinfo_set_medium(dvdid_hashinfo_t *hi, dvdid_medium_t medium) {

  hi->medium = medium;

  return DVDID_STATUS_OK;

}

DVDID_API(dvdid_medium_t) dvdid_hashinfo_get_medium(const dvdid_hashinfo_t *hi){

  return hi->medium;

}

DVDID_API(dvdid_status_t) dvdid_hashinfo_add_fileinfo(dvdid_hashinfo_t *hi, dvdid_dir_t dir, const dvdid_fileinfo_t *fi) {

  const dvdid__medium_spec_t *const *spec;
  int i;

  for (spec = dvdid_speclist; (*spec) != NULL; spec++)  {
    if ((*spec)->medium == hi->medium) { break; }
  }

  for (i = 0; i < (*spec)->dir_count; i++) {
    if ((*spec)->dir[i].dir == dir) { break; }
  }

  assert(i < (*spec)->dir_count);

  return fileinfo_list_append(&(hi->fi_first[i]), fi);

}

DVDID_API(dvdid_status_t) dvdid_hashinfo_add_filedata(dvdid_hashinfo_t *hi, dvdid_file_t file, const uint8_t *buf, size_t size) {

  const dvdid__medium_spec_t *const *spec;
  int i;

  for (spec = dvdid_speclist; (*spec) != NULL; spec++)  {
    if ((*spec)->medium == hi->medium) { break; }
  }

  for (i = 0; i < (*spec)->file_count; i++) {
    if ((*spec)->file[i].file == file) { break; }
  }

  assert(i < (*spec)->file_count);

  return copyset_data(&(hi->data_buf[i]), &(hi->data_size[i]), (*spec)->file[i].max_size, buf, size);

}

DVDID_API(dvdid_status_t) dvdid_hashinfo_init(dvdid_hashinfo_t *hi) {

  int i;

  for (i = 0; i < DVDID_MAX_DIRS; i++) {
    fileinfo_list_sort(&(hi->fi_first[i]));
  }

  return DVDID_STATUS_OK;

}

DVDID_API(void) dvdid_hashinfo_free(dvdid_hashinfo_t *hi) {

  dvdid__fileinfo_t *fi, *fi_tmp;
  int i;

  for (i = 0; i < DVDID_MAX_FILES; i++) {
    if (hi->data_buf[i] != NULL) { free(hi->data_buf[i]); }
  }

  for (i = 0; i < DVDID_MAX_DIRS; i++) {
    fi = hi->fi_first[i];

    while (fi != NULL) {
      free(fi->name);
      
      fi_tmp = fi->next;
      free(fi);
      fi = fi_tmp;;
    }
  }

  free(hi);

}

DVDID_API(const char*) dvdid_error_string(dvdid_status_t status) {
  switch (status) {
  case DVDID_STATUS_OK:                     return "No error";
  case DVDID_STATUS_MALLOC_ERROR:           return "Out of memory";
  case DVDID_STATUS_PLATFORM_UNSUPPORTED:   return "Platform not supported";
  case DVDID_STATUS_DETECT_MEDIUM_ERROR:    return "Error detecting medium"; 
  case DVDID_STATUS_MEDIUM_UNKNOWN:         return "Unknown medium";
  case DVDID_STATUS_FIXUP_SIZE_ERROR:       return "Error converting reported filesize to the size stored in the filesystem";

  case DVDID_STATUS_READ_VIDEO_TS_ERROR:    return "Error reading VIDEO_TS";
  case DVDID_STATUS_READ_VMGI_ERROR:        return "Error reading VIDEO_TS.IFO";
  case DVDID_STATUS_READ_VTS01I_ERROR:      return "Error reading VTS_01_0.IFO";

  case DVDID_STATUS_READ_VCD_ERROR:         return "Error reading VCD"; 
  case DVDID_STATUS_READ_CDI_ERROR:         return "Error reading CDI";
  case DVDID_STATUS_READ_EXT_ERROR:         return "Error reading EXT";
  case DVDID_STATUS_READ_KARAOKE_ERROR:     return "Error reading KARAOKE";
  case DVDID_STATUS_READ_CDDA_ERROR:        return "Error reading CDDA";
  case DVDID_STATUS_READ_MPEGAV_ERROR:      return "Error reading MPEGAV"; 
  case DVDID_STATUS_READ_SEGMENT_ERROR:     return "Error reading SEGMENT"; 
  case DVDID_STATUS_READ_INFO_VCD_ERROR:    return "Error reading INFO.VCD";
  case DVDID_STATUS_READ_ENTRIES_VCD_ERROR: return "Error reading ENTRIES.VCD";

  case DVDID_STATUS_READ_SVCD_ERROR:        return "Error reading SVCD"; 
  case DVDID_STATUS_READ_MPEG2_ERROR:       return "Error reading MPEG2"; 
  case DVDID_STATUS_READ_INFO_SVD_ERROR:    return "Error reading INFO.SVD";
  case DVDID_STATUS_READ_ENTRIES_SVD_ERROR: return "Error reading ENTRIES.SVD";
  case DVDID_STATUS_READ_TRACKS_SVD_ERROR:  return "Error reading TRACKS.SVD";

  default:                                  return "Unknown error";
  }
}




/* Legacy API functions */

DVDID_API(dvdid_status_t) dvdid_hashinfo_addfile(dvdid_hashinfo_t *hi, const dvdid_fileinfo_t *fi) {
  return dvdid_hashinfo_add_fileinfo(hi, DVDID_DIR_VIDEO_TS, fi);
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_set_vmgi(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size) {
  return dvdid_hashinfo_add_filedata(hi, DVDID_FILE_VMGI, buf, size);
}

DVDID_API(dvdid_status_t) dvdid_hashinfo_set_vts01i(dvdid_hashinfo_t *hi, const uint8_t *buf, size_t size) {
  return dvdid_hashinfo_add_filedata(hi, DVDID_FILE_VTS01I, buf, size);
}




/* Internal functions and structures */

const dvdid__medium_spec_t dvdid_spec_dvd = {
  DVDID_MEDIUM_DVD, 
  1,
  {
    {DVDID_DIR_VIDEO_TS, "VIDEO_TS", 0, NULL, DVDID_STATUS_READ_VIDEO_TS_ERROR},
    {0, NULL, 0, NULL, 0}, 
    {0, NULL, 0, NULL, 0}, 
    {0, NULL, 0, NULL, 0}, 
    {0, NULL, 0, NULL, 0}, 
    {0, NULL, 0, NULL, 0}, 
    {0, NULL, 0, NULL, 0}, 
  },
  2, 
  {
    {DVDID_FILE_VMGI, "VIDEO_TS" DIR_SEP "VIDEO_TS.IFO", 0, DVDID_HASHINFO_VXXI_MAXBUF, DVDID_STATUS_READ_VMGI_ERROR}, 
    {DVDID_FILE_VTS01I, "VIDEO_TS" DIR_SEP "VTS_01_0.IFO", 0, DVDID_HASHINFO_VXXI_MAXBUF, DVDID_STATUS_READ_VTS01I_ERROR}, 
    {0, NULL, 0, 0, 0}, 
  }
};

const dvdid__medium_spec_t dvdid_spec_vcd = {
  DVDID_MEDIUM_VCD, 
  7,
  {
    {DVDID_DIR_VCD, "VCD", 0, NULL, DVDID_STATUS_READ_VCD_ERROR},
    {DVDID_DIR_CDI, "CDI", 1, vcd_fixup_size, DVDID_STATUS_READ_CDI_ERROR}, /* Technically required, but have seen VCDs without */
    {DVDID_DIR_EXT, "EXT", 1, NULL, DVDID_STATUS_READ_EXT_ERROR},
    {DVDID_DIR_KARAOKE, "KARAOKE", 1, NULL, DVDID_STATUS_READ_KARAOKE_ERROR}, 
    {DVDID_DIR_CDDA, "CDDA", 1, vcd_fixup_size, DVDID_STATUS_READ_CDDA_ERROR},
    {DVDID_DIR_MPEGAV, "MPEGAV", 0, vcd_fixup_size, DVDID_STATUS_READ_MPEGAV_ERROR},
    {DVDID_DIR_SEGMENT, "SEGMENT", 1, vcd_fixup_size, DVDID_STATUS_READ_SEGMENT_ERROR},
  },
  2, 
  {
    {DVDID_FILE_INFO_VCD, "VCD" DIR_SEP "INFO.VCD", 0, DVDID_HASHINFO_FILEDATA_MAXSIZE, DVDID_STATUS_READ_INFO_VCD_ERROR}, 
    {DVDID_FILE_ENTRIES_VCD, "VCD" DIR_SEP "ENTRIES.VCD", 0, DVDID_HASHINFO_FILEDATA_MAXSIZE, DVDID_STATUS_READ_ENTRIES_VCD_ERROR}, 
    {0, NULL, 0, 0, 0}, 
  }
};

const dvdid__medium_spec_t dvdid_spec_svcd = {
  DVDID_MEDIUM_SVCD, 
  7,
  {
    {DVDID_DIR_SVCD, "SVCD", 0, NULL, DVDID_STATUS_READ_SVCD_ERROR},
    {DVDID_DIR_CDI, "CDI", 1, vcd_fixup_size, DVDID_STATUS_READ_CDI_ERROR},
    {DVDID_DIR_EXT, "EXT", 1, NULL, DVDID_STATUS_READ_EXT_ERROR}, /* All the contained files are optional, not sure whether the dir is */
    {DVDID_DIR_KARAOKE, "KARAOKE", 1, NULL, DVDID_STATUS_READ_KARAOKE_ERROR},
    {DVDID_DIR_CDDA, "CDDA", 1, vcd_fixup_size, DVDID_STATUS_READ_CDDA_ERROR},
    {DVDID_DIR_MPEG2, "MPEG2", 0, vcd_fixup_size, DVDID_STATUS_READ_MPEG2_ERROR},
    {DVDID_DIR_SEGMENT, "SEGMENT", 1, vcd_fixup_size, DVDID_STATUS_READ_SEGMENT_ERROR},
  },
  3, 
  {
    {DVDID_FILE_INFO_SVD, "SVCD" DIR_SEP "INFO.SVD", 0, DVDID_HASHINFO_FILEDATA_MAXSIZE, DVDID_STATUS_READ_INFO_SVD_ERROR}, 
    {DVDID_FILE_ENTRIES_SVD, "SVCD" DIR_SEP "ENTRIES.SVD", 0, DVDID_HASHINFO_FILEDATA_MAXSIZE, DVDID_STATUS_READ_ENTRIES_SVD_ERROR}, 
    {DVDID_FILE_ENTRIES_SVD, "SVCD" DIR_SEP "TRACKS.SVD", 0, DVDID_HASHINFO_FILEDATA_MAXSIZE, DVDID_STATUS_READ_TRACKS_SVD_ERROR}, 
  }
};

const dvdid__medium_spec_t *const dvdid_speclist[] = {
  &dvdid_spec_dvd, 
  &dvdid_spec_vcd, 
  &dvdid_spec_svcd, 
  NULL
};

static dvdid_status_t hashinfo_createinit(const char *path, const dvdid_medium_t medium, dvdid_hashinfo_t **hi, int *errn) {

  int i;
  dvdid_status_t rv;

  const dvdid__medium_spec_t *const *spec;


  for (spec = dvdid_speclist; *spec != NULL; spec++) {
    if ((*spec)->medium == medium) { break; }
  }

  rv = dvdid_hashinfo_create(hi);
  if (rv != DVDID_STATUS_OK) { return rv; }

  do {
    
    rv = dvdid_hashinfo_set_medium(*hi, medium);
    if (rv != DVDID_STATUS_OK) { break; }
    
    for (i = 0; i < (*spec)->dir_count; i++) {
      rv = hashinfo_add_dir_files(path, &((*spec)->dir[i]), *hi, errn);
      if (rv != DVDID_STATUS_OK) { break; }
    }
    if (rv != DVDID_STATUS_OK) { break; }

    for (i = 0; i < (*spec)->file_count; i++) {
      rv = hashinfo_add_file_by_path(path, &((*spec)->file[i]), *hi, errn);
      if (rv != DVDID_STATUS_OK) { break; }
    }
    if (rv != DVDID_STATUS_OK) { break; }

    rv = dvdid_hashinfo_init(*hi);

  } while(0);

  if (rv != DVDID_STATUS_OK) { dvdid_hashinfo_free(*hi); }
  return rv;

}


#if defined(DVDID_WINDOWS)

static int GetLastErrorToErrno(int LastError) {
  /*
    The error code for various API functions  will be returned by GetLastError().  
    However, these error codes are not compatible with error + strerror(), so 
    attempt conversion for the most likely error, and return 0 otherwise.
  */

  switch (LastError) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    return ENOENT;
    
  case ERROR_ACCESS_DENIED:
    return EACCES;
    
  case ERROR_DIRECTORY:
    return ENOTDIR;
    
  default:
    return 0;
  }
}

static dvdid_status_t detect_medium_win32(const char *path, const  dvdid__medium_spec_t *const *spec_first, dvdid_medium_t *medium, int *errn) {

  HANDLE file_list;
  WIN32_FIND_DATAW data;
  WIN32_FILE_ATTRIBUTE_DATA file_info;
  WCHAR *path_wide, *specpath_wide, *tmppath_wide;
  BOOL b;
  int l;
  size_t s;

  int identified;
  const dvdid__medium_spec_t *const *spec;

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

  do {

    l = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);

    path_wide = calloc(l, sizeof(*path_wide));

    if (path_wide == NULL) {
      rv = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, path_wide, l);

    do {

      /* First prove that we can read entries from the path specified, so that we can give a useful error message */
      /* We don't look at the entries we read here, using stat later instead, to avoid case issues */

      s = wcslen(path_wide) + wcslen(W_DIR_SEP) + wcslen(W_DIR_SEP L"*") + 1;

      tmppath_wide = calloc(s, sizeof(*tmppath_wide));

      if (tmppath_wide == NULL) {
        rv = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      _snwprintf(tmppath_wide, s, L"%s" W_DIR_SEP L"*", path_wide);

      file_list = FindFirstFileW(tmppath_wide, &data);

      if (file_list == INVALID_HANDLE_VALUE) {
        rv = DVDID_STATUS_DETECT_MEDIUM_ERROR;
        if (errn != NULL) { *errn = GetLastErrorToErrno(GetLastError()); }
        break;
      }

      do {
        /* NOTHING */
      } while (FindNextFileW(file_list, &data));

      FindClose(file_list);


      /* Now try to detect which medium it is */

      identified = 0;

      for (spec = spec_first; *spec != NULL; spec++) {

        l = MultiByteToWideChar(CP_UTF8, 0, (*spec)->dir[0].path, -1, NULL, 0);
        specpath_wide = calloc(l, sizeof(*specpath_wide));

        if (specpath_wide == NULL) {
          rv = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        MultiByteToWideChar(CP_UTF8, 0, (*spec)->dir[0].path, -1, specpath_wide, l);

        do {

          s = wcslen(path_wide) + wcslen(W_DIR_SEP) + wcslen(specpath_wide) + 1;

          tmppath_wide = calloc(s, sizeof(*tmppath_wide));

          if (tmppath_wide == NULL) {
            rv = DVDID_STATUS_MALLOC_ERROR;
            if (errn != NULL) { *errn = 0; }
            break;
          }

          _snwprintf(tmppath_wide, s, L"%s" W_DIR_SEP L"%s", path_wide, specpath_wide);

          b = GetFileAttributesEx(tmppath_wide, GetFileExInfoStandard, &file_info);

          if (b && ((file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)) {
            identified = 1;
            if (medium != NULL) { *medium = (*spec)->medium; }
            break;
          }

        } while(0);

        free(specpath_wide);

        if (identified || rv != DVDID_STATUS_OK) { break; }

      }

      /* Failed to detect medium, but no other errors */
      if (!identified && rv == DVDID_STATUS_OK) {
        rv = DVDID_STATUS_MEDIUM_UNKNOWN;
        if (errn != NULL) { *errn = 0; }
      }

    } while(0);

    free(path_wide);

  } while(0);

  return rv;

}

static dvdid_status_t hashinfo_add_dir_files_win32(const char *path, const dvdid__medium_spec_dir_t *spec, dvdid_hashinfo_t *hi, int *errn) {

  HANDLE file_list;
  WIN32_FIND_DATAW data;
  WCHAR *path_wide, *specpath_wide, *tmppath_wide;
  int l;
  size_t s;

  dvdid_fileinfo_t fi;

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

  do {

    l = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    path_wide = calloc(l, sizeof(*path_wide));

    if (path_wide == NULL) {
      rv = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, path_wide, l);

    do {

     l = MultiByteToWideChar(CP_UTF8, 0, spec->path, -1, NULL, 0);

     specpath_wide = calloc(l, sizeof(*specpath_wide));

      if (specpath_wide == NULL) {
        rv = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      MultiByteToWideChar(CP_UTF8, 0, spec->path, -1, specpath_wide, l);

      do {

        s = wcslen(path_wide) + wcslen(W_DIR_SEP) + wcslen(specpath_wide) + wcslen(W_DIR_SEP L"*") + 1;

        tmppath_wide = calloc(s, sizeof(*tmppath_wide));

        if (tmppath_wide == NULL) {
          rv = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        _snwprintf(tmppath_wide, s, L"%s" W_DIR_SEP L"%s" W_DIR_SEP L"*", path_wide, specpath_wide);

        file_list = FindFirstFileW(tmppath_wide, &data);
        free(tmppath_wide);

        if (file_list == INVALID_HANDLE_VALUE) {
          if (spec->optional && GetLastError() == ERROR_PATH_NOT_FOUND) { /* TODO: Are there any other return values that we should swallow here? */
            /* rv already set */
          } else {
            rv = spec->err;
            if (errn != NULL) { *errn = GetLastErrorToErrno(GetLastError()); }
          }
          break;
        }

        do {
          if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

            fi.creation_time = data.ftCreationTime.dwHighDateTime;
            fi.creation_time <<=32;
            fi.creation_time += data.ftCreationTime.dwLowDateTime;

            fi.size = data.nFileSizeLow;

            l = WideCharToMultiByte(CP_UTF8,0,data.cFileName,-1,NULL,0,NULL,FALSE);
            fi.name = malloc(l);
            if (fi.name == NULL) {
              rv = DVDID_STATUS_MALLOC_ERROR;
              if (errn != NULL) { *errn = 0; }
              break;
            }
            WideCharToMultiByte(CP_UTF8,0,data.cFileName,-1,fi.name,l,NULL,FALSE);

            /* Windows doesn't do this internally.  For compliant DVDs, it doesn't matter, 
               but for non compliant DVDs, it seems better to have this library consistent
               across platforms.  See detail in hashinfo_add_fir_files_unixlike */

            STRTOUPPER(fi.name);

            if (spec->fixup_size != NULL) {
              rv = spec->fixup_size(dvdid_hashinfo_get_medium(hi), spec->dir, &fi);
              if (rv != DVDID_STATUS_OK) {
                if (errn != NULL) { *errn = 0; }
                break;
              }
            }
        
            rv = dvdid_hashinfo_add_fileinfo(hi, spec->dir, &fi);
            free(fi.name);
            if (rv != DVDID_STATUS_OK) {
              if (errn != NULL) { *errn = 0; }
              break;
            }

          }
        } while (FindNextFileW(file_list, &data));

        FindClose(file_list);

      } while (0);

      free(specpath_wide);

    } while (0);

    free(path_wide);

  } while (0);

  return rv;

}

static dvdid_status_t hashinfo_add_file_by_path_win32(const char *path, const dvdid__medium_spec_file_t *spec, dvdid_hashinfo_t *hi, int *errn) {

  FILE *fp;
  WIN32_FILE_ATTRIBUTE_DATA file_info;
  WCHAR *path_wide, *specpath_wide, *tmppath_wide;
  int l;
  BOOL b;

  size_t s, file_size;
  uint8_t *buf;

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

  do {

    l = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);

    path_wide = calloc(l, sizeof(*path_wide));

    if (path_wide == NULL) {
      rv = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    MultiByteToWideChar(CP_UTF8, 0, path, -1, path_wide, l);

    do {

      l = MultiByteToWideChar(CP_UTF8, 0, spec->path, -1, NULL, 0);

      specpath_wide = calloc(l, sizeof(*specpath_wide));

      if (specpath_wide == NULL) {
        rv = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      MultiByteToWideChar(CP_UTF8, 0, spec->path, -1, specpath_wide, l);

      do {

        s = wcslen(path_wide) + wcslen(W_DIR_SEP) + wcslen(specpath_wide) + 1;

        tmppath_wide = calloc(s, sizeof(*tmppath_wide));

        if (tmppath_wide == NULL) {
          rv = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        _snwprintf(tmppath_wide, s, L"%s" W_DIR_SEP L"%s", path_wide, specpath_wide);

        b = GetFileAttributesEx(tmppath_wide, GetFileExInfoStandard, &file_info);

        if (b) {
          fp = _wfopen(tmppath_wide, L"rb");
        } else {
          fp = NULL; /* Silence warnings */
         }

        free(tmppath_wide);

        if (!b) {
          if (spec->optional && GetLastError() == ERROR_FILE_NOT_FOUND) { /* TODO: Are there any other return values that we should swallow here? */
            /* rv already set */
          } else {
            rv = spec->err;
            if (errn != NULL) { *errn = GetLastErrorToErrno(GetLastError()); }
          }
          break;
        }

        if (fp == NULL) {
          rv = spec->err;
          if (errn != NULL) { *errn = errno; }
          break;
        }

        file_size = file_info.nFileSizeLow;

        if (file_size > spec->max_size) { file_size = spec->max_size; }

        buf = malloc(file_size);

        if (buf == NULL) {
          rv = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        do {

          s = fread(buf, 1, file_size, fp);

          fclose(fp);

          if (s != file_size) {
            rv = spec->err;
            if (errn != NULL) { *errn = 0; }
            break;
          }

          rv = dvdid_hashinfo_add_filedata(hi, spec->file, buf, s);

          if (rv != DVDID_STATUS_OK) {
            if (errn != NULL) { *errn = 0; }
            break;
          }

        } while(0);

        free(buf);

      } while (0);

      free(specpath_wide);

    } while (0);

    free(path_wide);

  } while (0);

  return rv;

}

#elif defined(DVDID_POSIX) || defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)

#if defined(DVDID_FREEBSD) || defined(DVDID_LINUX) || defined(DVDID_APPLE)
#define TIMESPEC_TO_FILETIME(t) ((int64_t) ((((int64_t)(t).tv_sec) + 11644473600) * 10000000 + ((int64_t)(t).tv_nsec) / 100))
/* Convert from time_t to int64_t with an epoch of 1601-01-01 and ticks of 10MHz */
/* The POSIX spec guarantees that time_t has an epoch of 1970-01-01 00:00:00 UTC
   and 1 second ticks.  time_t can be integer or floating point, hence time immediate
   cast to int64_t. */
#else
#define TIME_T_TO_FILETIME(t) ((int64_t) ((((int64_t)(t)) + 11644473600) * 10000000))
#endif

static dvdid_status_t detect_medium_unixlike(const char *path, const  dvdid__medium_spec_t *const *spec_first, dvdid_medium_t *medium, int *errn) {

  DIR *dirlist;
  struct dirent direntry, *data;
  struct stat st;
  char *tmppath;
  int i;

  int identified;
  const dvdid__medium_spec_t *const *spec;

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

  do {

    /* First prove that we can read entries from the path specified, so that we can give a useful error message */
    /* We don't look at the entries we read here, using stat later instead, to avoid case issues */

    dirlist = opendir(path);

    if (dirlist == NULL) {
      rv = DVDID_STATUS_DETECT_MEDIUM_ERROR;
      if (errn != NULL) { *errn = errno; }
      break;
    }
    
    while (1) {
      i = readdir_r(dirlist, &direntry, &data);
      if (i != 0) {
        rv = DVDID_STATUS_DETECT_MEDIUM_ERROR;
        if (errn != NULL) { *errn = i; }
        break;
      } else if (data == NULL) {
        break;
      }

    }

    closedir(dirlist);

    if (rv != DVDID_STATUS_OK) { break; }


    /* Now try to detect which medium it is */

    identified = 0;

    for (spec = spec_first; *spec != NULL; spec++) {

      tmppath = malloc(strlen(path) + strlen(DIR_SEP) + strlen((*spec)->dir[0].path) + 1);
      
      if (tmppath == NULL) {
        rv = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      sprintf(tmppath, "%s" DIR_SEP "%s", path, (*spec)->dir[0].path);     

      i = stat(tmppath, &st);

      free(tmppath);
      
      if (i != 0 && errno != ENOENT) { /* TODO: Are there any other return values that we should swallow here? */
        rv = DVDID_STATUS_DETECT_MEDIUM_ERROR;
        if (errn != NULL) { *errn = errno; }
        break;
      } else if (S_ISDIR(st.st_mode)) {
        identified = 1;
        if (medium != NULL) { *medium = (*spec)->medium; }
        break;
      }
    }

    /* Failed to detect medium, but no other errors */
    if (!identified && rv == DVDID_STATUS_OK) {
      rv = DVDID_STATUS_MEDIUM_UNKNOWN;
      if (errn != NULL) { *errn = 0; }
    }

  } while(0);

  return rv;

}

static dvdid_status_t hashinfo_add_dir_files_unixlike(const char *path, const dvdid__medium_spec_dir_t *spec, dvdid_hashinfo_t *hi, int *errn) {

  DIR *dirlist;
  struct dirent direntry, *data;
  struct stat st;
  char *tmppath;
  int i;

  dvdid_fileinfo_t fi;

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

  do {

    tmppath = malloc(strlen(path) + strlen(DIR_SEP) + strlen(spec->path) + 1);

    if (tmppath == NULL) {
      rv = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    sprintf(tmppath, "%s" DIR_SEP "%s", path, spec->path);

    dirlist = opendir(tmppath);

    free(tmppath);
    
    if (dirlist == NULL) {
      if (spec->optional && errno == ENOENT) { /* TODO: Are there any other codes we should swallow? */
        /* rv already set */
      } else {
        rv = spec->err;
        if (errn != NULL) { *errn = errno; }
      }
      break;
    }
    
    while (1) {
      i = readdir_r(dirlist, &direntry, &data);
      if (i != 0) {
        rv = spec->err;
        if (errn != NULL) { *errn = i; }
        break;
      } else if (data == NULL) {
        break;
      }
      
      tmppath = malloc(strlen(path) + strlen(DIR_SEP) + strlen(spec->path) + strlen(DIR_SEP) + strlen(data->d_name) + 1);
      
      sprintf(tmppath, "%s" DIR_SEP "%s" DIR_SEP "%s", path, spec->path, data->d_name);
      
      if (tmppath == NULL) {
        rv = DVDID_STATUS_MALLOC_ERROR;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      i = stat(tmppath, &st);

      free(tmppath);
      
      if (i != 0) {
        rv = spec->err;
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
          rv = DVDID_STATUS_MALLOC_ERROR;
          if (errn != NULL) { *errn = 0; }
          break;
        }

        strcpy(fi.name, data->d_name);
        
        /*
          UDF is case sensitive.  The DVD standard mandates upper case filename, and 
          hence all compliant DVDs will have all upper case file names in VIDEO_TS 
          for UDF filesystem.  Windows uses the UDF filesystem, but performs no case 
          normalisation when calculating the dvdid.  It is hence possible that a non 
          compliant dvd could have a lowercase filename in the VIDEO_TS dir, whose 
          name would go into the CRC64 in lower case on Windows.
          
          Unfortunately, iso9660 isn't case sensitive, and the filesystem can present
          names to use in whatever case it likes; most unix systems choose lowercase.
          As it's quite possible that we will be using the iso9660 filesystem, we 
          force everything to upper case here, and have to accept the possibility that
          we might generate a different dvdid from Windows for a non compliant dvd, 
          even if its mounted using the udf filesystem, and the case was presented to 
          us correctly.
          
          VCD and SVCD use only an iso9660 filesystem, so again we could be presented
          with filenames with any case, (in theory, even under Windows).  The 
          proposed standard hence requires all filenames be uppercased, even on 
          Windows, so this issue won't ever arise.
        */

        STRTOUPPER(fi.name);
        
        if (spec->fixup_size != NULL) {
          rv = spec->fixup_size(dvdid_hashinfo_get_medium(hi), spec->dir, &fi);
          if (rv != DVDID_STATUS_OK) {
            if (errn != NULL) { *errn = 0; }
            break;
          }
        }        

        rv = dvdid_hashinfo_add_fileinfo(hi, spec->dir, &fi);
        
        free(fi.name);

        if (rv != DVDID_STATUS_OK) {
          if (errn != NULL) { *errn = 0; }
          break;
        }
        
      }
    }

    closedir(dirlist);

  } while(0);
  
  return rv;

}

static dvdid_status_t hashinfo_add_file_by_path_unixlike(const char *path, const dvdid__medium_spec_file_t *spec, dvdid_hashinfo_t *hi, int *errn) {

  FILE *fp;
  struct stat st;
  char *tmppath;
  int i;

  size_t s, file_size;
  uint8_t *buf;

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

  do {

    tmppath = malloc(strlen(path) + strlen(DIR_SEP) + strlen(spec->path) + 1);

    if (tmppath == NULL) {
      rv = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    sprintf(tmppath, "%s" DIR_SEP "%s", path, spec->path);

    i = stat(tmppath, &st);

    if (i == 0) {
      fp = fopen(tmppath, "rb");
    } else {
      fp = NULL; /* Silence warnings */
    }

    free(tmppath);
      
    if (i != 0) {
      if (spec->optional && errno == ENOENT) { /* TODO: Are there any other codes we should swallow? */
        /* rv already set */
      } else {
        rv = spec->err;
        if (errn != NULL) { *errn = errno; }
      }
      break;
    }

    if (fp == NULL) {
      rv = spec->err;
      if (errn != NULL) { *errn = errno; }
      break;
    }

    file_size = st.st_size;

    if (file_size > spec->max_size) { file_size = spec->max_size; }

    buf = malloc(file_size);

    if (buf == NULL) {
      rv = DVDID_STATUS_MALLOC_ERROR;
      if (errn != NULL) { *errn = 0; }
      break;
    }

    do {

      s = fread(buf, 1, file_size, fp);

      fclose(fp);

      if (s != file_size) {
        rv = spec->err;
        if (errn != NULL) { *errn = 0; }
        break;
      }

      rv = dvdid_hashinfo_add_filedata(hi, spec->file, buf, s);

      if (rv != DVDID_STATUS_OK) {
        if (errn != NULL) { *errn = 0; }
        break;
      }

    } while(0);

    free(buf);  

  } while (0);

  return rv;

}

#else

static dvdid_status_t detect_medium_void(const char *path, const  dvdid__medium_spec_t *const *spec_first, dvdid_medium_t *medium, int *errn) {
  (void) path; (void) spec_first; (void) medium; (void) errn;
  if (errn != NULL){ *errn = 0; }
  return DVDID_STATUS_PLATFORM_UNSUPPORTED;
}

static dvdid_status_t hashinfo_add_dir_files_void(const char *path, const dvdid__medium_spec_dir_t *spec, dvdid_hashinfo_t *hi, int *errn) {
  (void) path; (void) spec; (void) hi;
  if (errn != NULL){ *errn = 0; }
  return DVDID_STATUS_PLATFORM_UNSUPPORTED;
}

static dvdid_status_t hashinfo_add_file_by_path_void(const char *path, const dvdid__medium_spec_file_t *spec, dvdid_hashinfo_t *hi, int *errn) {
  (void) path; (void) spec; (void) hi;
  if (errn != NULL){ *errn = 0; }
  return DVDID_STATUS_PLATFORM_UNSUPPORTED;
}

#endif


/*

As different operating systems choose to present (S)VCDs to the user differently, the filesize reported
for various key files different across operating systems.  For example, Windows presents any that has 
some sectors in Mode2 with a RIFF header followed by the 2352 byte contents of the sectors comprising the
files.  OSX only does the same, but only for AVSEQnn.DAT and MUSICnn.DAT within the MPEGAV dir.

In order to get consistent behaviour, a well defined value for each filesize needs to be chosen.  For this,
the filesize field as stored on the physical disc is used, as this is unambiguously defined for all discs,
and readily accessible to any program ripping VCDs using RAW sector access.

The following function attemps platform specific recreation of the filesize as stored on the disc, given 
the filesize reported by the operating system.  It errs of the side of caution, returning an error in
unexpected cases, so that we can get reports, and code specifically for these cases

*/

/* This function should only be called for dirs likely to contain Form2 files, i.e.
   CDI, MPEG(AV|2), CDDA, SEGMENT */

#define CD_ISO9660_SECTOR_SIZE (2048)
#define CD_RAW_SECTOR_SIZE (2352)
#define RIFF_HEADER_SIZE (44)
static dvdid_status_t vcd_fixup_size(dvdid_medium_t medium, dvdid_dir_t dir, dvdid_fileinfo_t *fi) {

  dvdid_status_t rv;


  rv = DVDID_STATUS_OK;

#if defined(DVDID_WINDOWS)

  if (dir == DVDID_DIR_CDI && fi->size == 1510028 && strcmp(fi->name, "CDI_IMAG.RTF") == 0) {

    /* Special case for Nero created discs.  The size in the filesystem is supposed to be, sectors * 2048, 
       but nero uses sectors * 2336 for "CDI_IMAG.RTF".  As the size presented by windows is 
       (size in the filesystem) / 2048 * 2352 + 44
       the value has been truncated read from the filesystem has been truncated, and attempting to reverse 
       this won't take us back to the original.  Hence, we have to handle this special case. */

    fi->size = 1315168; /* 1315168 == 563 * 2336.  N.B. floor(1315268/2048)*2352+44 == 1510028 */

  } else if ( 
             ( dir == DVDID_DIR_CDI 
               && strcmp(fi->name, "CDI_IMAG.RTF") == 0 ) /* we assume that it'll always have the mode2 flag set, and hence
                                                             Windows will report it's size as per 2352 * sectors + 44 */
             || ( dir == DVDID_DIR_CDDA 
                  && strlen(fi->name) == 11
                  && strncmp(fi->name, "AUDIO", 5) == 0
                  && strncmp(fi->name + 7, ".DAT", 4) == 0 )

             || ( dir == DVDID_DIR_MPEGAV 
                  && strlen(fi->name) == 11 
                  && (strncmp(fi->name, "AVSEQ", 5) == 0 
                      || strncmp(fi->name, "MUSIC", 5) == 0)
                  && strncmp(fi->name + 7, ".DAT", 4) == 0 )

             || ( dir == DVDID_DIR_MPEG2 
                  && strlen(fi->name) == 11 
                  && strncmp(fi->name, "AVSEQ", 5) == 0 
                  && strncmp(fi->name + 7, ".MPG", 4) == 0 )

             || ( medium == DVDID_MEDIUM_VCD 
                  && dir == DVDID_DIR_SEGMENT 
                  && strlen(fi->name) == 12 
                  && strncmp(fi->name, "ITEM", 4) == 0 
                  && strncmp(fi->name + 8, ".DAT", 4) == 0 )

             || ( medium == DVDID_MEDIUM_SVCD 
                  && dir == DVDID_DIR_SEGMENT 
                  && strlen(fi->name) == 12 
                  && strncmp(fi->name, "ITEM", 4) == 0 
                  && strncmp(fi->name + 8, ".MPG", 4) == 0 )

              ) {

    if ((fi->size % CD_RAW_SECTOR_SIZE) == RIFF_HEADER_SIZE) {

      fi->size = (fi->size - RIFF_HEADER_SIZE) / CD_RAW_SECTOR_SIZE * CD_ISO9660_SECTOR_SIZE;

    } else {

      rv = DVDID_STATUS_FIXUP_SIZE_ERROR;

    }

  } else if (dir == DVDID_DIR_CDI && strncmp(fi->name, "CDI_", 4) == 0) {

    /* Leave the other CDI_ files alone, Windows reports the same size for those as the filesystem */

    /* NOTHING */

  } else {

    /* Raise an error if we get files we didn't expect in one the dirs that we're checking, It's 
       certainly possible to have other files in the directories that we check, but as we don't 
       know how windows reports their size, send back an error in the hope of getting some 
       reports and learning more */

    rv = DVDID_STATUS_FIXUP_SIZE_ERROR;

  }

#elif defined(DVDID_FREEBSD) || defined(DVDID_LINUX)

  /* FreeBSD, and none of the Linux distros encoutered understand (S)VCD, so the size is reported
     exactly as per filesystem */

  if (dir == DVDID_DIR_CDI && fi->size == 1315168 && strcmp(fi->name, "CDI_IMAG.RTF") == 0) {

    /* Special case for Nero created discs as documented above.  
       1315168 % 2048 != 0, so we need to handle it specially */

    /* NOTHING */

  } else if ( 
             ( dir == DVDID_DIR_CDI 
               && strcmp(fi->name, "CDI_IMAG.RTF") == 0 ) 

             || ( dir == DVDID_DIR_CDDA 
                  && strlen(fi->name) == 11
                  && strncmp(fi->name, "AUDIO", 5) == 0
                  && strncmp(fi->name + 7, ".DAT", 4) == 0 )

             || ( dir == DVDID_DIR_MPEGAV 
                  && strlen(fi->name) == 11 
                  && (strncmp(fi->name, "AVSEQ", 5) == 0 
                      || strncmp(fi->name, "MUSIC", 5) == 0)
                  && strncmp(fi->name + 7, ".DAT", 4) == 0 )

             || ( dir == DVDID_DIR_MPEG2 
                  && strlen(fi->name) == 11 
                  && strncmp(fi->name, "AVSEQ", 5) == 0 
                  && strncmp(fi->name + 7, ".MPG", 4) == 0 )

             || ( medium == DVDID_MEDIUM_VCD 
                  && dir == DVDID_DIR_SEGMENT 
                  && strlen(fi->name) == 12 
                  && strncmp(fi->name, "ITEM", 4) == 0 
                  && strncmp(fi->name + 8, ".DAT", 4) == 0 )

             || ( medium == DVDID_MEDIUM_SVCD 
                  && dir == DVDID_DIR_SEGMENT 
                  && strlen(fi->name) == 12 
                  && strncmp(fi->name, "ITEM", 4) == 0 
                  && strncmp(fi->name + 8, ".MPG", 4) == 0 )

              ) {

    /* These are all files that we expect to be a multiple of 2048 bytes, and which are prime 
       candidates for being "translated" as per Windows in a future OS release.  Therefore, 
       make sure that they are a multiple of 2048 bytes, or raise an error if not, so that we can
       code to restore the original filesize (if appropriate given OS version), or add an 
       exception for specific files */


    if ((fi->size % CD_ISO9660_SECTOR_SIZE) == 0) {

      /* NOTHING */

    } else {

      rv = DVDID_STATUS_FIXUP_SIZE_ERROR;

    }

  } else if (dir == DVDID_DIR_CDI && strncmp(fi->name, "CDI_", 4) == 0) {

    /* Leave the other CDI_ files alone */

    /* NOTHING */

  } else {

    /* Raise an error if we get files we didn't expect in one the dirs that we're checking.  We
     can add exceptions for certain files later */

    rv = DVDID_STATUS_FIXUP_SIZE_ERROR;

  }

#elif defined(DVDID_APPLE) /* && VERSION ? */

  (void) medium; 

  /* TODO: As per BSD, Linux, check that all other potential Form2 files are multiples of 2048 bytes */

  if (dir == DVDID_DIR_CDI && fi->size == 1315168 && strcmp(fi->name, "CDI_IMAG.RTF") == 0) {

    /* Special case for Nero created discs as documented above.  
       1315168 % 2048 != 0, so we need to handle it specially */

    /* NOTHING */

  } else if (dir == DVDID_DIR_MPEGAV && 

             /* As per cd9660_vfsops.c:1023 */
             strlen(fi->name) == 11 
             && strncmp(fi->name + 7, ".DAT", 4) == 0 
             && (strncmp(fi->name, "AVSEQ", 5) == 0 
                 || strncmp(fi->name, "MUSIC", 5) == 0) 
             ) {
      
    if ((fi->size % CD_RAW_SECTOR_SIZE) == RIFF_HEADER_SIZE)  {

      fi->size = (fi->size - RIFF_HEADER_SIZE) / CD_RAW_SECTOR_SIZE * CD_ISO9660_SECTOR_SIZE;

    } else {

      rv = DVDID_STATUS_FIXUP_SIZE_ERROR;
  
    }

  } else if ( 
             ( dir == DVDID_DIR_CDI 
               && strcmp(fi->name, "CDI_IMAG.RTF") == 0 ) 

             || ( dir == DVDID_DIR_CDDA 
                  && strlen(fi->name) == 11
                  && strncmp(fi->name, "AUDIO", 5) == 0
                  && strncmp(fi->name + 7, ".DAT", 4) == 0 )

             || ( dir == DVDID_DIR_MPEG2 
                  && strlen(fi->name) == 11 
                  && strncmp(fi->name, "AVSEQ", 5) == 0 
                  && strncmp(fi->name + 7, ".MPG", 4) == 0 )

             || ( medium == DVDID_MEDIUM_VCD 
                  && dir == DVDID_DIR_SEGMENT 
                  && strlen(fi->name) == 12 
                  && strncmp(fi->name, "ITEM", 4) == 0 
                  && strncmp(fi->name + 8, ".DAT", 4) == 0 )

             || ( medium == DVDID_MEDIUM_SVCD 
                  && dir == DVDID_DIR_SEGMENT 
                  && strlen(fi->name) == 12 
                  && strncmp(fi->name, "ITEM", 4) == 0 
                  && strncmp(fi->name + 8, ".MPG", 4) == 0 )

              ) {

    /* These are all files that we expect to be a multiple of 2048 bytes, and which are prime 
       candidates for being "translated" as per Windows in a future OS release, especially 
       given that similar files (viz. MPEGAV/SEQnn.DAT MPEGAV/AVSEQnn.DAT) are already .  Therefore, 
       make sure that they are a multiple of 2048 bytes, or raise an error if not, so that we can
       code to restore the original filesize (if appropriate given OS version), or add an 
       exception for specific files */

    if ((fi->size % CD_ISO9660_SECTOR_SIZE) == 0) {
      
      /* NOTHING */
      
    } else {
      
      rv = DVDID_STATUS_FIXUP_SIZE_ERROR;
      
    }
    
  } else if (dir == DVDID_DIR_CDI && strncmp(fi->name, "CDI_", 4) == 0) {
    
    /* Leave the other CDI_ files alone */
    
    /* NOTHING */
    
  } else {
    
    /* Raise an error if we get files we didn't expect in one the dirs that we're checking.  We
       can add exceptions for certain files later */
    
    rv = DVDID_STATUS_FIXUP_SIZE_ERROR;

  }

#else

  /* Don't know about this platform, so don't guess.  Send back  DVDID_STATUS_FIXUP_SIZE_ERROR rathern
     than a DVDID_STATUS_PLATFORM_UNSUPPORTED to avoid wild goose chases tracking down the problem */

  (void) dir; (void) fi;

  rv = DVDID_STATUS_FIXUP_SIZE_ERROR;
  
#endif

    return rv;
    
}


static void  fileinfo_list_sort(dvdid__fileinfo_t **fi_first) {

  dvdid__fileinfo_t **fi, *fi_tmp;

  /* Alpha sort file info by file names */
  /* TODO: Rigorous analysis of the linked list manipulation */
  do {
    for (fi = fi_first;; fi = &((*fi)->next)) {
      if (*fi == NULL || (*fi)->next == NULL) { return; }
      if (strcmp((*fi)->name, (*fi)->next->name) > 0) {
        fi_tmp = (*fi)->next;
        (*fi)->next = (*fi)->next->next;
        fi_tmp->next = *fi;
        *fi = fi_tmp;
        break;
      }
    }
  } while (1);
}

static dvdid_status_t fileinfo_list_append(dvdid__fileinfo_t **fi_first, const dvdid_fileinfo_t *fi) {

  dvdid__fileinfo_t **fi2;

  /* Get a pointer to the first NULL pointer in the linked list of fileinfos */
  fi2 = fi_first;
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

static dvdid_status_t copyset_data(uint8_t **d_data, size_t *d_size, size_t max_size, const uint8_t *buf, size_t size) {

  *d_size = (size > max_size) ? max_size : size;
  *d_data = malloc(*d_size);

  if (*d_data == NULL) { return DVDID_STATUS_MALLOC_ERROR; }

  memcpy(*d_data, buf, *d_size);

  return DVDID_STATUS_OK;

}


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

static inline void crc64_uint32(uint64_t *crc64, uint32_t l) {
  int i;

  for (i = 0; i < 4; i++) {
    crc64_byte(crc64, (uint8_t) l);
    l >>= 8;
  }
}

static inline void crc64_uint64(uint64_t *crc64, uint64_t ll) {
  int i;

  for (i = 0; i < 8; i++) {
    crc64_byte(crc64, (uint8_t) ll);
    ll >>= 8;
  }
}

/*
  The CRC used is:

    Name   : 
    Width  : 64
    Poly   : 259C84CBA6426349
    Init   : FFFFFFFFFFFFFFFF
    RefIn  : True
    RefOut : True
    XorOut : 0000000000000000
    Check  : 75D4B74F024ECEEA

  as per the standard documented in,

  http://www.cs.waikato.ac.nz/~312/crc.txt

*/

static inline void crc64_init(uint64_t *crc64) {
  *crc64 = UINT64_C(0xffffffffffffffff);
}

static inline void crc64_done(uint64_t *crc64){
  (void) crc64;
}

static inline void crc64_byte(uint64_t *crc64, uint8_t b) {
  (*crc64) = ((*crc64) >> 8) ^ crc64_table[((uint8_t) (*crc64)) ^ b];
}
