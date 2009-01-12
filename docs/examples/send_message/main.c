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
#include <telepathy-glib/contact.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/interfaces.h> /* For TP_IFACE_CHANNEL_TYPE_TEXT */
#include <glib/gprintf.h>

GMainLoop *mainloop = NULL;
TpDBusDaemon *bus_daemon = NULL;
TpConnection *connection = NULL;
TpContact *contact = NULL;
TpChannel *text_channel = NULL;

/* A utility function to make our debug output easier to read. */
const gchar* get_reason_description (TpConnectionStatusReason reason)
{
  switch (reason)
    {
      case TP_CONNECTION_STATUS_REASON_NONE_SPECIFIED:
        return "None specified";
      case TP_CONNECTION_STATUS_REASON_REQUESTED:
        return "Requested";
      case TP_CONNECTION_STATUS_REASON_NETWORK_ERROR:
        return "Network error";
      case TP_CONNECTION_STATUS_REASON_AUTHENTICATION_FAILED:
        return "Authentication failed";
      case TP_CONNECTION_STATUS_REASON_ENCRYPTION_ERROR:
        return "Encryption Error";
      case TP_CONNECTION_STATUS_REASON_NAME_IN_USE:
        return "Name in use";
      case TP_CONNECTION_STATUS_REASON_CERT_NOT_PROVIDED:
        return "Certificate not provided";
      case TP_CONNECTION_STATUS_REASON_CERT_UNTRUSTED:
        return "Certificate untrusted";
      case TP_CONNECTION_STATUS_REASON_CERT_EXPIRED:
        return "Certificate expired";
      case TP_CONNECTION_STATUS_REASON_CERT_NOT_ACTIVATED:
        return "Certificate not activated";
      case TP_CONNECTION_STATUS_REASON_CERT_HOSTNAME_MISMATCH:
        return "Certificate hostname mismatch";
      case TP_CONNECTION_STATUS_REASON_CERT_FINGERPRINT_MISMATCH:
        return "Certificate fingerprint mismatch";
      case TP_CONNECTION_STATUS_REASON_CERT_SELF_SIGNED:
        return "Cerficate is self signed";
      case TP_CONNECTION_STATUS_REASON_CERT_OTHER_ERROR:
        return "Other certificate error";
      default:
        return "Unknown reason";
   }
}

void on_send (TpChannel *proxy,
  const GError *error,
  gpointer user_data,
  GObject *weak_object)
{
  if (error)
    {
      g_printf ("tp_cli_channel_type_text_call_send () failed: %s\n", error->message);
      return;
    }

  g_printf("DEBUG: Message sent.\n");
}

void on_text_channel_is_ready (TpChannel *channel,
  const GError *error,
  gpointer user_data)
{
  if (error)
    {
      g_printf ("tp_channel_call_when_ready () failed: %s\n", error->message);
      return;
    }

  g_printf("DEBUG: Text channel is ready.\n");

  //Send a message:

  TpProxyPendingCall* pending = tp_cli_channel_type_text_call_send (channel, 
    -1, /* timeout */
    0, /* Channel_Text_Message_Type_Normal */
    "Hello from Murray's telepathy example. Send murrayc@openismus.com an email if this arrived.",
    &on_send,
    NULL, NULL, NULL);
}

void on_connection_create_channel(TpConnection *proxy,
  const gchar *object_path,
  GHashTable *out_Properties,
  const GError *error,
  gpointer user_data,
  GObject *weak_object)
{
  if (error)
    {
      g_printf ("tp_cli_connection_interface_requests_call_create_channel () failed: %s\n", error->message);
      return;
    }

  g_printf("DEBUG: Text channel created.\n");

  /* Create the proxy object for the channel: */
  GError *inner_error = NULL;
  text_channel = tp_channel_new (connection, 
    object_path, 
    TP_IFACE_CHANNEL_TYPE_TEXT,
    TP_HANDLE_TYPE_CONTACT,
    tp_contact_get_handle (contact),
    &inner_error);

  if (inner_error)
    {
      g_printf ("tp_channel_new () failed: %s\n", error->message);
      g_clear_error (&inner_error);
      return;
    }

  /* Wait for the channel to be ready: */
  tp_channel_call_when_ready (text_channel, on_text_channel_is_ready, NULL);
}

void on_get_contacts_by_id (TpConnection *connection,
  guint n_contacts,
  TpContact * const *contacts,
  const gchar * const *requested_ids,
  GHashTable *failed_id_errors,
  const GError *error,
  gpointer user_data,
  GObject *weak_object)
{
  if (error)
    {
      g_printf ("tp_connection_get_contacts_by_id () failed: %s\n", error->message);
      return;
    }

  /* We only requested one contact: */
  if(n_contacts < 1)
    {
      g_printf ("tp_connection_get_contacts_by_id () returned no contacts\n");
      return;
    }

  contact = contacts[0];
  if(!contact)
    {
      g_printf ("tp_connection_get_contacts_by_id () returned NULL contact\n");
      return;
    }

  g_printf("DEBUG: Contact found: %s.\n", tp_contact_get_identifier (contact));

 
  //Get a text Channel for this contact:
  GHashTable* properties = g_hash_table_new (NULL, NULL);

  GValue *value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, TP_IFACE_CHANNEL_TYPE_TEXT); /* Rather than streamed media, or a contact list, for instance. */
  g_hash_table_insert (properties, TP_IFACE_CHANNEL ".ChannelType", value);

  value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, tp_contact_get_identifier (contact));
  g_hash_table_insert (properties, TP_IFACE_CHANNEL ".TargetID", value);

  /* Note that we must use g_value_set_int() instead of g_value_set_enum() 
   * because these enums have no GTypes:
   * https://bugs.freedesktop.org/show_bug.cgi?id=18055
   */
  GValue *value_enum = tp_g_value_slice_new (G_TYPE_INT);
  g_printf("DEBUG: TP_HANDLE_TYPE_CONTACT=%d\n", TP_HANDLE_TYPE_CONTACT);
  g_value_set_int (value_enum, TP_HANDLE_TYPE_CONTACT); /* Rather than a ROOM. */
  g_hash_table_insert (properties, TP_IFACE_CHANNEL ".TargetHandleType", value_enum);

  /* Note that this uses the new Requests interface, which is not yet 
   * implemented by all Telepathy ConnectionManagers. 
   * The old way (working with all ConnectionManagers), was the RequestChannel 
   * interface, via tp_cli_connection_call_request_channel().
   */
  g_printf("DEBUG: Calling tp_cli_connection_interface_requests_call_create_channel().\n");
  tp_cli_connection_interface_requests_call_create_channel (connection, 
    -1, /* timeout */
    properties,
    &on_connection_create_channel,
    NULL, NULL, NULL);
  g_printf("DEBUG: Called tp_cli_connection_interface_requests_call_create_channel().\n");
}

void on_connection_ready (TpConnection *connection,
  const GError *error,
  gpointer user_data)
{
  if (error)
    {
      g_printf ("tp_connection_call_when_ready() failed: %s\n", error->message);
      return;
    }

  //This crashes:
  const gchar * ids[1];
  ids[0] = "daniel.kitta@jabber.org"; //TODO: Rename this so he doesn't get spam.
  tp_connection_get_contacts_by_id (connection,
    1, ids, 
    0, NULL, /* features */
    &on_get_contacts_by_id,
    NULL, NULL, NULL);
}

void on_connection_status_changed(TpConnection *proxy,
  guint arg_Status,
  guint arg_Reason,
  gpointer user_data,
  GObject *weak_object)
{
  switch(arg_Status)
    {
      case TP_CONNECTION_STATUS_CONNECTED:
        g_printf ("Connection status: Connected (reason: %s)\n", get_reason_description (arg_Reason));

        /* Set the presence: */

        /* Most functions require the connection to be ready: */
        tp_connection_call_when_ready (connection, 
          &on_connection_ready,
          NULL);

        break;

      case TP_CONNECTION_STATUS_CONNECTING:
        g_printf ("Connection status: Connecting (reason: %s)\n", get_reason_description (arg_Reason));

        break;

      case TP_CONNECTION_STATUS_DISCONNECTED:
        g_printf ("Connection status: Disconnected (reason: %s)\n", get_reason_description (arg_Reason));

        /* Finish with the connection object: */
        if (connection)
          {
            g_object_unref (connection);
            connection = NULL;
          }

        /* Stop the application: */
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
  g_printf ("DEBUG: Calling Connect().\n");
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
  GHashTable *parameters = g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) tp_g_value_slice_free);

  GValue *value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, "murrayc@murrayc.com");
  g_hash_table_insert (parameters, "account", value);

  value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, "paswordTODO");
  g_hash_table_insert (parameters, "password", value);

  /* This jabber-specific parameter can avoid clashes with 
     other telepathy clients that use the default jabber 
     resource name. */
  value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, "telepathy-doc send_message example");
  g_hash_table_insert (parameters, "resource", value);

  
  /* Call RequestConnection; it will return asynchronously by calling got_connection */
  tp_cli_connection_manager_call_request_connection (connection_manager, -1,
      "jabber", parameters, got_connection, NULL, NULL, NULL);

  g_hash_table_unref (parameters);
  parameters = NULL;


  /* Run the main loop, 
   * to keep our application alive while we wait for responses from telepathy.
   * This function returns when we call g_main_loop_quit() from elsewhere.
   */
  g_main_loop_run (mainloop);

  /* Clean up: */
  g_object_unref (connection_manager);
  g_main_loop_unref (mainloop);
  g_object_unref (bus_daemon);

  return 0;
}
