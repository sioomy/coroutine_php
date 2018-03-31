dnl $Id$
dnl config.m4 for extension coroutine_php

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(coroutine_php, for coroutine_php support,
dnl Make sure that the comment is aligned:
dnl [  --with-coroutine_php             Include coroutine_php support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(coroutine_php, whether to enable coroutine_php support,
dnl Make sure that the comment is aligned:
[  --enable-coroutine_php           Enable coroutine_php support])

if test "$PHP_COROUTINE_PHP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-coroutine_php -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/coroutine_php.h"  # you most likely want to change this
  dnl if test -r $PHP_COROUTINE_PHP/$SEARCH_FOR; then # path given as parameter
  dnl   COROUTINE_PHP_DIR=$PHP_COROUTINE_PHP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for coroutine_php files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       COROUTINE_PHP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$COROUTINE_PHP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the coroutine_php distribution])
  dnl fi

  dnl # --with-coroutine_php -> add include path
  dnl PHP_ADD_INCLUDE($COROUTINE_PHP_DIR/include)

  dnl # --with-coroutine_php -> check for lib and symbol presence
  dnl LIBNAME=coroutine_php # you may want to change this
  dnl LIBSYMBOL=coroutine_php # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $COROUTINE_PHP_DIR/$PHP_LIBDIR, COROUTINE_PHP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_COROUTINE_PHPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong coroutine_php lib version or lib not found])
  dnl ],[
  dnl   -L$COROUTINE_PHP_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(COROUTINE_PHP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(coroutine_php, coroutine_php.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
