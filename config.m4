dnl $Id$
dnl config.m4 for extension play

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(play, for play support,
Make sure that the comment is aligned:
[  --with-play             Include play support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(play, whether to enable play support,
Make sure that the comment is aligned:
[  --enable-play           Enable play support])

if test "$PHP_PLAY" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-play -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/play.h"  # you most likely want to change this
  dnl if test -r $PHP_PLAY/$SEARCH_FOR; then # path given as parameter
  dnl   PLAY_DIR=$PHP_PLAY
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for play files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       PLAY_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$PLAY_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the play distribution])
  dnl fi

  dnl # --with-play -> add include path
  dnl PHP_ADD_INCLUDE($PLAY_DIR/include)

  dnl # --with-play -> check for lib and symbol presence
  dnl LIBNAME=play # you may want to change this
  dnl LIBSYMBOL=play # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PLAY_DIR/$PHP_LIBDIR, PLAY_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_PLAYLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong play lib version or lib not found])
  dnl ],[
  dnl   -L$PLAY_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(PLAY_SHARED_LIBADD)

  PHP_NEW_EXTENSION(play,
        play.c \
        play_core/play_string.c \
        play_core/tool.c \
        play_core/play_manager_action.c \
        play_core/play_global_config.c \
        play_core/play_manager_meta.c \
        play_core/play_socket.c \
        play_core/play_fastcgi.c \
        play_interface/play_interface_action.c \
        play_interface/play_interface_context.c \
        play_interface/play_interface_crontab.c \
        play_interface/play_interface_db.c \
        play_interface/play_interface_input.c \
        play_interface/play_interface_meta_router_abstract.c \
        play_interface/play_interface_output.c \
        play_interface/play_interface_play.c \
        play_interface/play_interface_play_crontab.c \
        play_interface/play_interface_play_init.c \
        play_interface/play_interface_play_reconst.c \
        play_interface/play_interface_processor.c \
        play_interface/play_interface_query.c \
        play_interface/play_interface_render_abstract.c \
        play_interface/play_interface_netkit.c \
        play_interface/play_interface_meta.c \
        play_interface/play_interface_utils.c,
    $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)

    PHP_ADD_BUILD_DIR([$ext_builddir/play_core])
    PHP_ADD_BUILD_DIR([$ext_builddir/play_interface])
    PHP_ADD_BUILD_DIR([$ext_builddir/play_lib/clibs])
    PHP_ADD_INCLUDE(/usr/include/libxml2)
fi
