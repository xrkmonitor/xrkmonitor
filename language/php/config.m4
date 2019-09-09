dnl $Id$
dnl config.m4 for extension xrkmonitor

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(xrkmonitor, for xrkmonitor support,
dnl Make sure that the comment is aligned:
dnl [  --with-xrkmonitor             Include xrkmonitor support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(xrkmonitor, whether to enable xrkmonitor support,
dnl Make sure that the comment is aligned:
[  --enable-xrkmonitor           Enable xrkmonitor support])

if test "$PHP_XRKMONITOR" != "no"; then
  PHP_ADD_INCLUDE(/usr/include/mtreport_api/)
  PHP_ADD_LIBRARY_WITH_PATH(mtreport_api, /usr/lib64, XRKMONITOR_SHARED_LIBADD)

  PHP_SUBST(XRKMONITOR_SHARED_LIBADD)
  PHP_NEW_EXTENSION(xrkmonitor, xrkmonitor.c, $ext_shared)
fi
