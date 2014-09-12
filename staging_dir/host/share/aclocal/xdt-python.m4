dnl $Id$
dnl
dnl Copyright (c) 2002-2006
dnl         The Xfce development team. All rights reserved.
dnl
dnl Written for Xfce by Benedikt Meurer <benny@xfce.org>.
dnl
dnl This program is free software; you can redistribute it and/or modify it
dnl under the terms of the GNU General Public License as published by the Free
dnl Software Foundation; either version 2 of the License, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
dnl more details.
dnl
dnl You should have received a copy of the GNU General Public License along with
dnl this program; if not, write to the Free Software Foundation, Inc., 59 Temple
dnl Place, Suite 330, Boston, MA  02111-1307  USA
dnl
dnl xdt-python
dnl ----------
dnl  Miscellanous Python related autoconf checks. Based on prior
dnl  work by the Python community.
dnl



dnl XDT_CHECK_PYTHON_HEADERS([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl 
dnl Checks if the target system has the ability to create Python
dnl extensions, that is, if all required Python headers are found.
dnl Executes ACTION-IF-FOUND if all required headers are found, else
dnl ACTION-IF-NOT-FOUND.
dnl
dnl In addition, this macro defines (and substitutes) PYTHON_INCLUDES
dnl with the required C preprocessor flags to find the python headers.
dnl
AC_DEFUN([XDT_CHECK_PYTHON_HEADERS],
[
  dnl check for Python interpreter first
  AC_REQUIRE([AM_PATH_PYTHON])

  AC_MSG_CHECKING([for headers required to compile python extensions])

  dnl the PYTHON_INCLUDES first
  xdt_python_PREFIX=`${PYTHON} -c "import sys; print sys.prefix"`
  xdt_python_EPREFIX=`${PYTHON} -c "import sys; print sys.exec_prefix"`
  xdt_python_OSNAME=`${PYTHON} -c "import os; print os.name"`
  case "${xdt_python_OSNAME}" in
    nt)
      xdt_python_INCLUDES="include"
      ;;
    *)
      xdt_python_INCLUDES="include/python${PYTHON_VERSION}"
      ;;
  esac
  PYTHON_INCLUDES="-I${xdt_python_PREFIX}/${xdt_python_INCLUDES}"
  if test x"${xdt_python_EPREFIX}" != x"${xdt_python_PREFIX}"; then
    PYTHON_INCLUDES="${PYTHON_INCLUDES} -I${xdt_python_EPREFIX}/${xdt_python_INCLUDES}"
  fi
  AC_SUBST([PYTHON_INCLUDES])

  dnl now check if the headers exists
  save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS ${PYTHON_INCLUDES}"
  AC_TRY_CPP([#include <Python.h>],
  [
    AC_MSG_RESULT([found])
    $1
  ],
  [
    AC_MSG_RESULT([not found])
    $2
  ])
  CPPFLAGS="$save_CPPFLAGS"
])



dnl vim:set ts=2 sw=2 et ai:
