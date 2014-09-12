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
dnl xdt-xfce
dnl --------
dnl  Xfce specific M4 macros.
dnl



dnl XDT_XFCE_PANEL_PLUGIN(varname, [version = 4.2.0])
dnl
dnl This macro is intended to be used by panel plugin writers. It
dnl detects the xfce4-panel package on the target system and sets
dnl "varname"_CFLAGS, "varname"_LIBS, "varname"_REQUIRED_VERSION
dnl and "varname"_VERSION properly. The parameter "version"
dnl specifies the minimum required version of xfce4-panel (defaults
dnl to 4.2.0 if not given).
dnl
dnl This macro also takes care of handling special panel versions,
dnl like the threaded panel, that was used during the Xfce 4.2.0
dnl development, and automatically sets up the correct flags.
dnl
dnl In addition, this macro defines "varname"_PLUGINSDIR (and
dnl marks it for substitution), which points to the directory
dnl where the panel plugin should be installed to. You should
dnl use this variable in your Makefile.am.
dnl
AC_DEFUN([XDT_XFCE_PANEL_PLUGIN],
[
  dnl Check for the xfce4-panel package
  XDT_CHECK_PACKAGE([$1], [xfce4-panel-1.0], [m4_default([$2], [4.2.0])])

  dnl Check if the panel is threaded (only for old panels)
  xdt_cv_CFLAGS=$$1_CFLAGS
  AC_MSG_CHECKING([whether the Xfce panel is threaded])
  if $PKG_CONFIG --max-version=4.1.90 xfce4-panel-1.0 >/dev/null 2>&1; then
    AC_MSG_RESULT([yes])
    xdt_cv_CFLAGS="$xdt_cv_CFLAGS -DXFCE_PANEL_THREADED=1"
    xdt_cv_CFLAGS="$xdt_cv_CFLAGS -DXFCE_PANEL_LOCK\(\)=gdk_threads_enter\(\)"
    xdt_cv_CFLAGS="$xdt_cv_CFLAGS -DXFCE_PANEL_UNLOCK\(\)=gdk_threads_leave\(\)"
  else
    AC_MSG_RESULT([no])
    xdt_cv_CFLAGS="$xdt_cv_CFLAGS -DXFCE_PANEL_LOCK\(\)=do{}while\(0\)"
    xdt_cv_CFLAGS="$xdt_cv_CFLAGS -DXFCE_PANEL_UNLOCK\(\)=do{}while\(0\)"
  fi
  $1_CFLAGS="$xdt_cv_CFLAGS"

  dnl Check where to put the plugins to
  AC_MSG_CHECKING([where to install panel plugins])
  $1_PLUGINSDIR=$libdir/xfce4/panel-plugins
  AC_SUBST([$1_PLUGINSDIR])
  AC_MSG_RESULT([$$1_PLUGINSDIR])
])



dnl XDT_XFCE_MCS_PLUGIN(varname, [version = 4.2.0])
dnl
dnl This macro is intented to be used by MCS plugin writers. It
dnl detects the MCS manager package on the target system and sets
dnl "varname"_CFLAGS, "varname"_LIBS, "varname"_REQUIRED_VERSION,
dnl and "varname"_VERSION. The parameter "version" allows you
dnl to specify the minimum required version of xfce-mcs-manager
dnl (defaults to 4.2.0 if not given).
dnl
dnl In addition, this macro defines "varname"_PLUGINSDIR (and
dnl marks it for substitution), which points to the directory
dnl where the MCS plugin should be installed to. You should use
dnl this variable in your Makefile.am.
dnl
AC_DEFUN([XDT_XFCE_MCS_PLUGIN],
[
  dnl Check for the xfce-mcs-manager package
  XDT_CHECK_PACKAGE([$1], [xfce-mcs-manager], [m4_default([$2], [4.2.0])])

  dnl Check where to put the plugins to
  AC_MSG_CHECKING([where to install MCS plugins])
  $1_PLUGINSDIR=$libdir/xfce4/mcs-plugins
  AC_SUBST([$1_PLUGINSDIR])
  AC_MSG_RESULT([$$1_PLUGINSDIR])
])



dnl XFCE_PANEL_PLUGIN(varname, version)
dnl
dnl Simple wrapper for XDT_XFCE_PANEL_PLUGIN(varname, version). Kept
dnl for backward compatibility. Will be removed in the future.
dnl
AC_DEFUN([XFCE_PANEL_PLUGIN],
[
  XDT_XFCE_PANEL_PLUGIN([$1], [$2])
])



dnl XFCE_MCS_PLUGIN(varname, version)
dnl
dnl Simple wrapper for XDT_XFCE_MCS_PLUGIN(varname, version). Kept
dnl for backward compatibility. Will be removed in the future.
dnl
AC_DEFUN([XFCE_MCS_PLUGIN],
[
  XDT_XFCE_MCS_PLUGIN([$1], [$2])
])

