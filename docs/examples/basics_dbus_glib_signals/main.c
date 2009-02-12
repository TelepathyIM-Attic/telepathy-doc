/* Copyright 2009 Collabora Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <dbus/dbus-glib.h>
#include <glib.h>

GMainLoop *mainloop = NULL;

void on_proxy_signal_device_added (DBusGProxy *object, const char *path,
  gpointer user_data)
{
  g_print("Signal: Device Added: path=%s\n", path);
}

int
main (int argc, char **argv)
{
  g_type_init ();

  /* Create the main loop: */
  mainloop = g_main_loop_new (NULL, FALSE);

  /* Connect to the bus: */
  GError *error = 0;
  DBusGConnection *connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (error)
    {
      g_printf ("dbus_g_bus_get() failed: %s\n", error->message);
      g_clear_error (&error);
      g_main_loop_quit (mainloop);
      return 1;
    }

  /* Get a proxy for the remote object: */
  DBusGProxy *proxy = dbus_g_proxy_new_for_name (connection,
    "org.freedesktop.Hal", /* name */
    "/org/freedesktop/Hal/Manager", /* path */
    "org.freedesktop.Hal.Manager"); /* interface */


  /* Connect to a signal on the interface: */
  /* TODO: This doesn't work, though the Python example does. */
  dbus_g_proxy_add_signal (proxy, "DeviceAdded", G_TYPE_STRING, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (proxy, "DeviceAdded", 
    G_CALLBACK (on_proxy_signal_device_added), NULL, NULL);


  /* Run the main loop, 
   * to keep our application alive while we wait for responses from D-Bus.
   * This function returns when we call g_main_loop_quit() from elsewhere.
   */
  g_main_loop_run (mainloop);

  /* Clean up: */
  g_main_loop_unref (mainloop);

  return 0;
}
