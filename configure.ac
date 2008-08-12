dnl configure.ac for template xml docbook
AC_INIT(telepathy-doc, 0.5.0.1)
AM_INIT_AUTOMAKE([-Wno-portability])

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Use gnome-doc-utils:
GNOME_DOC_INIT([0.9.0])

# Get the path to perl, so we can use perl scripts without hard-coding the path.
TELEPATHY_DOCS_CHECK_PERL([5.6.0])

PKG_CHECK_MODULES(TELEPATHY, telepathy-glib >= 0.7.3)

AC_OUTPUT([
  Makefile
  m4/Makefile
  docs/Makefile
  docs/examples/Makefile
  docs/book/Makefile
])
