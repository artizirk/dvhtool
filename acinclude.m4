dnl AC_REPLACE_GNU_GETOPT
AC_DEFUN([AC_REPLACE_GNU_GETOPT],
[AC_CHECK_FUNCS([getopt_long], , [LIBOBJS="$LIBOBJS getopt.o getopt1.o"])
AC_SUBST(LIBOBJS)dnl
])

dnl Test for __attribute__
AC_DEFUN([VH___ATTRIBUTE__], [
AC_MSG_CHECKING(for __attribute__)
AC_CACHE_VAL(ac_cv___attribute__, [
AC_TRY_COMPILE([
#include <stdlib.h>

static void foo(void) __attribute__ ((noreturn));

static void
foo(void)
{
  exit(1);
}
],
[
],
ac_cv___attribute__=yes,
ac_cv___attribute__=no)])
if test "$ac_cv___attribute__" = "yes"; then
  AC_DEFINE(HAVE___ATTRIBUTE__, 1, [define if your compiler has __attribute__])
fi
AC_MSG_RESULT($ac_cv___attribute__)
])
