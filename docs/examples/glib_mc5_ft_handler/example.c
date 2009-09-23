/*
 * An example of talking to MC5 to get available connections
 */

#include <glib.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/defs.h>

#include "example-handler.h"

#define CLIENT_NAME "ExampleFTHandler"

static GMainLoop *loop = NULL;

int
main (int argc, char **argv)
{
  GError *error = NULL;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  TpDBusDaemon *tpdbus = tp_dbus_daemon_dup (NULL);
  DBusGConnection *dbus = tp_get_bus ();

  ExampleHandler *example_handler = example_handler_new ();

  /* register well-known name */
  g_assert (tp_dbus_daemon_request_name (tpdbus,
      TP_CLIENT_BUS_NAME_BASE CLIENT_NAME,
      TRUE, NULL));
  /* register ExampleHandler on the bus */
  dbus_g_connection_register_g_object (dbus,
      TP_CLIENT_OBJECT_PATH_BASE CLIENT_NAME,
      G_OBJECT (example_handler));

  g_main_loop_run (loop);
}
