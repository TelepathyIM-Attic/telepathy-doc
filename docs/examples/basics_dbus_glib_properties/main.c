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

void on_proxy_call_get (DBusGProxy *proxy,
  DBusGProxyCall *call_id,
  void *user_data)
{
  GError *error = 0;
  GValue result = {0, };
  dbus_g_proxy_end_call (proxy, call_id, &error, 
    G_TYPE_VALUE, &result, /* Return value. */
    G_TYPE_INVALID);

  if (error)
    {
      g_printf ("dbus_g_proxy_begin_call() failed: %s\n", error->message);
      g_clear_error (&error);
    }
  else
    {
      gchar* as_string = g_strdup_value_contents (&result);
      g_printf ("dbus_g_proxy_begin_call() succeeded, returning type=%s, value=%s\n", 
        G_VALUE_TYPE_NAME(&result), as_string);
      g_free (as_string);
    }

  g_value_unset (&result);

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

  /* Get a proxy for the Properties interface of a remote object: */
  DBusGProxy *proxy = dbus_g_proxy_new_for_name (connection,
    "org.freedesktop.Telepathy.Connection.salut.local_xmpp.Murray_20Cumming", /* name */
    "/org/freedesktop/Telepathy/Connection/salut/local_xmpp/Murray_20Cumming", /* path */
    "org.freedesktop.DBus.Properties"); /* interface */

  /* Call the Properties.Get method on the interface of the remote object: */

  /* Call the Get method to get a property value: */
  dbus_g_proxy_begin_call (proxy, "Get" /* property name */,
    &on_proxy_call_get, NULL, /* user_data */ 
    NULL, /* destroy notification */
    G_TYPE_STRING, "org.freedesktop.Telepathy.Connection", /* interface name */
    G_TYPE_STRING, "SelfHandle", /* property name */
    G_TYPE_INVALID,
    G_TYPE_VALUE,
    G_TYPE_INVALID);

  /* Run the main loop, 
   * to keep our application alive while we wait for responses from D-Bus.
   * This function returns when we call g_main_loop_quit() from elsewhere.
   */
  g_main_loop_run (mainloop);

  /* Clean up: */
  g_main_loop_unref (mainloop);

  return 0;
}
