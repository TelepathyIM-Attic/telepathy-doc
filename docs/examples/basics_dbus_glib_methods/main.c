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

void on_proxy_call_notify (DBusGProxy *proxy,
  DBusGProxyCall *call_id,
  void *user_data)
{
  GError *error = 0;
  guint result = 0;
  dbus_g_proxy_end_call (proxy, call_id, &error, 
    G_TYPE_UINT, &result, /* Return value. */
    G_TYPE_INVALID);

  if (error)
    {
      g_printf ("dbus_g_proxy_begin_call() failed: %s\n", error->message);
      g_clear_error (&error);
    }
  else
    {
      g_printf ("dbus_g_proxy_begin_call() succeeded, returning %u\n", result);
    }

  /* Stop the main loop to allow main() to finish, 
   * stopping the program: */
  g_main_loop_quit (mainloop);
}

int
main (int argc, char **argv)
{
  g_type_init ();

  /* Create the main loop: */
  mainloop = g_main_loop_new (NULL, FALSE);

  /* begin dbus-glib-methods-get-bus */
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
  /* end dbus-glib-methods-get-bus */

  /* begin dbus-glib-methods-get-proxy */
  /* Get a proxy for the remote object: */
  DBusGProxy *proxy = dbus_g_proxy_new_for_name (connection,
    "org.freedesktop.Notifications", /* name */
    "/org/freedesktop/Notifications", /* path */
    "org.freedesktop.Notifications"); /* interface */
  /* end dbus-glib-methods-get-proxy */

  /* Call a method on the interface  of the remote object: */

  /* begin dbus-glib-methods-call-method */
  /* Create an empty GHashTable for one of the parameters: */
  GHashTable* actions = g_hash_table_new (NULL, NULL);  

  /* Call the method: */
  dbus_g_proxy_begin_call (proxy, "Notify", 
    &on_proxy_call_notify, NULL, /* user_data */ 
    NULL, /* destroy notification */
    G_TYPE_STRING, "dbus-glib example",
    G_TYPE_UINT, 0,
    G_TYPE_STRING, "", /* icon_name */ 
    G_TYPE_STRING, "Example Notification",
    G_TYPE_STRING, "This is an example notification via dbus-glib.",
    G_TYPE_STRV, 0,
    dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), actions,
    G_TYPE_INT, 0,
    G_TYPE_INVALID,
    G_TYPE_INT,
    G_TYPE_INVALID);
  g_hash_table_unref (actions);
  actions = NULL;
  /* end dbus-glib-methods-call-method */

  /* Run the main loop, 
   * to keep our application alive while we wait for responses from D-Bus.
   * This function returns when we call g_main_loop_quit() from elsewhere.
   */
  g_main_loop_run (mainloop);

  /* Clean up: */
  g_main_loop_unref (mainloop);

  return 0;
}
