dnl configure.ac for template xml docbook
AC_INIT(telepathy-doc, 0.5.0.1)
AM_INIT_AUTOMAKE([-Wno-portability])

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Use gnome-doc-utils:
GNOME_DOC_INIT([0.9.0])

PKG_CHECK_MODULES(GNOME_DOC_UTILS, gnome-doc-utils)

PKG_CHECK_MODULES(TELEPATHY_GLIB, telepathy-glib >= 0.7.3 gtk+-2.0 >= 2.12.0)
AC_SUBST(TELEPATHY_GLIB_CFLAGS)
AC_SUBST(TELEPATHY_GLIB_LIBS)

# Use C:
AC_PROG_CC()

AC_OUTPUT([
  Makefile
  m4/Makefile

  docs/Makefile
    docs/examples/Makefile
      docs/examples/glib_list_protocols/Makefile

    docs/book/Makefile
])
