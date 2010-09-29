dnl configure.ac for template xml docbook
AC_INIT(telepathy-doc, 0.5.0.1)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AM_INIT_AUTOMAKE([-Wno-portability])

AC_PROG_CC
AC_PROG_LIBTOOL

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Use gnome-doc-utils:
GNOME_DOC_INIT([0.9.0])

PKG_CHECK_MODULES(GNOME_DOC_UTILS, gnome-doc-utils)

PKG_CHECK_MODULES(TELEPATHY_GLIB, [
	telepathy-glib >= 0.11.6
	mission-control-plugins >= 5.5
	gtk+-2.0 >= 2.12.0
	gio-unix-2.0 >= 2.21.4
	])
AC_SUBST(TELEPATHY_GLIB_CFLAGS)
AC_SUBST(TELEPATHY_GLIB_LIBS)

AM_PATH_PYTHON()

AC_MSG_CHECKING([Mission Control plugins dir])
MISSION_CONTROL_PLUGINS_DIR=`pkg-config --variable=plugindir mission-control-plugins`
if test "x$MISSION_CONTROL_PLUGINS_DIR" = "x"; then
	AC_MSG_ERROR([could not determine plugindir])
else
	AC_MSG_RESULT([$MISSION_CONTROL_PLUGINS_DIR])
fi
AC_SUBST(MISSION_CONTROL_PLUGINS_DIR)

AC_OUTPUT([
  Makefile
  m4/Makefile

  docs/Makefile
    docs/examples/Makefile
      docs/examples/glib_list_protocols/Makefile
      docs/examples/glib_get_user_defined_groups/Makefile
      docs/examples/glib_get_roster/Makefile
      docs/examples/glib_salut_ft/Makefile
      docs/examples/glib_jabber_muc/Makefile
      docs/examples/glib_telepathy_properties/Makefile
      docs/examples/glib_dbus_tube/Makefile
      docs/examples/glib_stream_tube/Makefile
      docs/examples/glib_text_channel/Makefile
      docs/examples/glib_mc5_connections/Makefile
      docs/examples/glib_mc5_dbus_tube_handler/Makefile
      docs/examples/glib_mc5_observer/Makefile
      docs/examples/glib_blinkenlight_observer/Makefile
      docs/examples/glib_mc5_ft_handler/Makefile
      docs/examples/glib_mc5_presence_app/Makefile
      docs/examples/glib_mc5_request_call/Makefile
      docs/examples/gtk_presence_app/Makefile
      docs/examples/mission_control_plugin/Makefile
      docs/examples/pygtk_chat_client/Makefile
      docs/examples/python_get_parameters/Makefile
      docs/examples/python_iface_messaging/Makefile
      docs/examples/python_irc_channel/Makefile
      docs/examples/python_irc_roomlist/Makefile
      docs/examples/python_mc5_clients/Makefile
      docs/examples/python_simple_presence/Makefile

    docs/book/Makefile
])
