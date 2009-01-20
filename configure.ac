dnl configure.ac for template xml docbook
AC_INIT(telepathy-doc, 0.5.0.1)
AM_INIT_AUTOMAKE([-Wno-portability])

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Use gnome-doc-utils:
GNOME_DOC_INIT([0.9.0])

# Get the path to perl, so we can use perl scripts without hard-coding the path.
TELEPATHY_DOCS_CHECK_PERL([5.6.0])

# We depend on GTK+ too, because that allows us to create more realistic 
# examples that are easier to play with. But we will try to write only a small 
# amount of UI code. 
PKG_CHECK_MODULES(TELEPATHY_DOCS, telepathy-glib >= 0.7.3 gtk+-2.0 >= 2.12.0)

# Use C:
AC_PROG_CC()

AC_OUTPUT([
  Makefile
  m4/Makefile

  docs/Makefile
    docs/examples/Makefile
      docs/examples/basics_dbus_glib/Makefile
      docs/examples/basics_dbus_python/Makefile
      docs/examples/connect/Makefile
      docs/examples/list_all_protocols/Makefile
      docs/examples/list_contacts/Makefile
      docs/examples/set_presence/Makefile
      docs/examples/send_message/Makefile

    docs/book/Makefile
])
