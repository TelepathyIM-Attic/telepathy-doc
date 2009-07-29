/*
 * An example of talking to MC5 to get available connections
 */

#include <glib.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>

#include "example-observer.h"

static GMainLoop *loop = NULL;

int
main (int argc, char **argv)
{
  GError *error = NULL;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  DBusGConnection *dbus = tp_get_bus ();

  ExampleObserver *example_observer = example_observer_new ();

  dbus_g_connection_register_g_object (dbus,
      "/org/freedesktop/Telepathy/Client/ExampleObserver",
      G_OBJECT (example_observer));

  g_main_loop_run (loop);
}
