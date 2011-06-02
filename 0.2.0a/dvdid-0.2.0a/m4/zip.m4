dnl $Id: zip.m4 1664 2009-03-20 17:41:32Z chris $
dnl Check for zip to package for Windows

AC_DEFUN([AC_PROG_ZIP], [
  AC_CHECK_PROG(ZIP, zip, zip, no)
  test -z "${ZIP}" && AC_MSG_ERROR(zip was not found on this system)
])
