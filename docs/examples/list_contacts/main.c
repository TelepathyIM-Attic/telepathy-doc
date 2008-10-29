/* Copyright 2008 Collabora Ltd
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

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/connection.h>
#include <glib/gprintf.h>

GMainLoop *mainloop = NULL;
TpDBusDaemon *bus_daemon;
TpConnection *connection;

void on_connection_status_changed(TpConnection *proxy,
  guint arg_Status,
  guint arg_Reason,
  gpointer user_data,
  GObject *weak_object)
{
  switch(arg_Status)
    {
      case TP_CONNECTION_STATUS_CONNECTED:
        g_printf ("Connection status: Connected: reason=%u.\n", arg_Reason);

        /* Disconnect the connection.
           Otherwise it will be orphaned. */
        g_printf ("Disconnecting...\n");
        tp_cli_connection_call_disconnect (connection, -1, NULL, NULL,
            NULL, NULL);
        break;

      case TP_CONNECTION_STATUS_CONNECTING:
        g_printf ("Connection status: Connecting: reason=%u.\n", arg_Reason);
        break;

      case TP_CONNECTION_STATUS_DISCONNECTED:
        g_printf ("Connection status: Disconnected: reason=%u.\n", arg_Reason);
        g_object_unref (connection);
        g_main_loop_quit (mainloop);
        break;

      default:
        g_printf ("Connection status: Unknown status.\n");
        break;
    }
}

void
got_connection (TpConnectionManager *connection_manager,
                const gchar *service_name,
                const gchar *object_path,
                const GError *request_connection_error,
                gpointer user_data,
                GObject *weak_object)
{
  TpProxySignalConnection *signal_connection;
  GError *error = NULL;

  if (request_connection_error != NULL)
    {
      g_printf ("RequestConnection failed: %s\n",
          request_connection_error->message);
      g_main_loop_quit (mainloop);
      return;
    }

  connection = tp_connection_new (bus_daemon, service_name, object_path, &error);

  if (error != NULL)
    {
      g_printf ("tp_connection_new() failed: %s\n", error->message);
      g_clear_error (&error);
      g_main_loop_quit (mainloop);
      return;
    }

  g_printf("DEBUG: Connection created.\n");

  /* React to connection status changes,
   * including errors when we try to connect: */
  signal_connection = tp_cli_connection_connect_to_status_changed (connection,
      &on_connection_status_changed,
      NULL, /* user_data */
      NULL, /* destroy_callback */
      NULL, /* weak_object */
      &error);

  if (error)
    {
      g_printf ("couldn't connect to StatusChanged: %s\n", error->message);
      g_clear_error (&error);
      g_main_loop_quit (mainloop);
      return;
    }

  /* Connect the connection: */
  g_printf ("DEBUG: Calling Connect()\n");
  tp_cli_connection_call_connect (connection, -1, NULL, NULL, NULL, NULL);
}


int
main (int argc, char **argv)
{
  g_type_init ();

  /* Create the main loop: */
  mainloop = g_main_loop_new (NULL, FALSE);

  bus_daemon = tp_dbus_daemon_new (tp_get_bus ());

  /* Get the connection manager: */
  GError *error = NULL;
  TpConnectionManager *connection_manager = 
    tp_connection_manager_new (bus_daemon, "gabble", NULL, &error);
  if (error)
    {
      g_printf ("tp_connection_manager_new() failed: %s\n", error->message);
      g_clear_error (&error);
      return 1;
    }

  /* Get the connection : */
  GHashTable *parameters = g_hash_table_new (NULL, NULL);

  GValue value_account = { 0, };
  g_value_init (&value_account, G_TYPE_STRING);
  g_value_set_static_string (&value_account, "murrayc@murrayc.com");
  g_hash_table_insert (parameters, "account", &value_account);

  GValue value_password = { 0, };
  g_value_init (&value_password, G_TYPE_STRING);
  g_value_set_static_string (&value_password, "passwordTODO");
  g_hash_table_insert (parameters, "password", &value_password);

  /* Call RequestConnection; it will return asynchronously by calling got_connection */
  tp_cli_connection_manager_call_request_connection (connection_manager, -1,
      "jabber", parameters, got_connection, NULL, NULL, NULL);

  g_value_unset (&value_account);
  g_value_unset (&value_password);
  g_hash_table_unref (parameters);

  g_main_loop_run (mainloop);

  g_object_unref (connection_manager);

  g_main_loop_unref (mainloop);
  g_object_unref (bus_daemon);

  return 0;
}
