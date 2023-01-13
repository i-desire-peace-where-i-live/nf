AC_INIT([notefinder], [0.1], [gforgx@protonmail.com])
AC_PROG_CC
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

PERL_CPPFLAGS=`which perl 2>/dev/null && perl -MExtUtils::Embed -e ccopts`
PERL_LIBS=`which perl 2>/dev/null && perl -MExtUtils::Embed -e ldopts`

old_CPPFLAGS="$CPPFLAGS"
old_LIBS="$LIBS"
CPPFLAGS="$PERL_CPPFLAGS $CPPFLAGS"
LIBS="$PERL_LIBS $LIBS"

AC_CHECK_FUNCS(perl_alloc, AC_DEFINE(HAVE_PERL, 1, [Define to 1 if Perl headers are present]))

CPPFLAGS="$old_CPPFLAGS"
LIBS="$old_LIBS"

PKG_CHECK_MODULES(GTK, gtk+-3.0, [AC_DEFINE([HAVE_GTK_3], 1, [Define to 1 if GTK+ 3 is present])])
PKG_CHECK_MODULES(SQLITE, sqlite3, [AC_DEFINE([HAVE_SQLITE], 1, [Define to 1 if SQLite is present])])

AC_OUTPUT

