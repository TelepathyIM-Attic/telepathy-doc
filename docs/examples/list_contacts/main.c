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
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>
#include <glib/gprintf.h>

//TODO: Pass these around in user_data:
GMainLoop *mainloop = NULL;
TpDBusDaemon *bus_daemon = NULL;
TpConnection *connection = NULL;
guint contact_list_handle = 0;
const TpIntSet* channel_members_set = NULL;

void disconnect ()
{
  if (!connection)
    return;

  g_printf ("DEBUG: Disconnecting.\n");
  tp_cli_connection_call_disconnect (connection, -1, NULL, NULL,
    NULL, NULL); /* Also destroys the connection object. */
  connection = NULL;
}

void show_contact_information (TpContact *contact)
{
  g_printf ("Contact: Identifier=%s\n", 
    tp_contact_get_identifier (contact));

  g_printf ("  Alias=%s\n", 
    tp_contact_get_alias (contact));

  g_printf ("  Presence Status=%s\n", 
    tp_contact_get_presence_status (contact));
}

void on_connection_get_contacts_by_handle (TpConnection *connection,
  guint n_contacts,
  TpContact * const *contacts,
  guint n_failed,
  const TpHandle *failed,
  const GError *error,
  gpointer user_data,
  GObject *weak_object)
{
  if (error)
      g_printf ("tp_connection_get_contacts_by_handle() failed: %s\n", error->message);

  guint i = 0;
  for (i = 0; i < n_contacts; ++i)
    {
      TpContact *contact = contacts[i];

      if (contact)
        show_contact_information (contact);
    }

  /* Disconnect the connection.
     Otherwise it will be orphaned. */
  disconnect();
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

  /* Actually get the information now that the connection is ready: */


  /* Get a GArray instead of a TpIntSet,
   * so we can easily create a normal array: */
  GArray *members_array = g_array_new (FALSE, TRUE, sizeof(guint));
  TpIntSetIter iter = TP_INTSET_ITER_INIT (channel_members_set );
  while (tp_intset_iter_next (&iter))
    {
      g_array_append_val (members_array, iter.element);
      g_printf("DEBUG: handle: %d\n", iter.element); 
    }
  channel_members_set = NULL; /* We don't need this anymore. */  

  g_printf("DEBUG: members_array size=%u\n", members_array->len);


  /* Get information about each contact: */ 
  guint n_handles = members_array->len;
  TpHandle* handles = (TpHandle*)g_array_free (members_array, FALSE);
  members_array = NULL;
  
  /* The extra optional information that we are interested in: */
  TpContactFeature features[] = {TP_CONTACT_FEATURE_ALIAS, 
    TP_CONTACT_FEATURE_AVATAR_TOKEN, TP_CONTACT_FEATURE_PRESENCE};

  g_printf ("DEBUG: Calling tp_connection_get_contacts_by_handle()\n");

  tp_connection_get_contacts_by_handle (connection,
    n_handles, handles,
    sizeof (features) / sizeof(TpContactFeature), features,
    &on_connection_get_contacts_by_handle,
    NULL, /* user_data */
    NULL, /* destroy */
    NULL /* weak_object */);
  g_free (handles);
}

void on_channel_ready (TpChannel *channel,
  const GError *error,
  gpointer user_data)
{
  if (error)
  {
    g_printf ("tp_channel_call_when_ready() failed: %s\n", error->message);
    return;
  }


  /* List the channel members: */
  channel_members_set = tp_channel_group_get_members (channel);
  if (!channel_members_set )
    {
      g_printf ("tp_channel_group_get_members() returned NULL.\n");
      return;
    }

  g_printf("DEBUG: Number of members: %u\n", tp_intset_size (channel_members_set ));

 
  /* tp_connection_get_contacts_by_handle() requires the connection to be 
   * ready: */
  tp_connection_call_when_ready (connection, 
    &on_connection_ready,
    NULL);
}

void on_connection_request_channel (TpConnection *proxy,
  const gchar *channel_dbus_path,
  const GError *error,
  gpointer user_data,
  GObject *weak_object)
{
  if (error)
    {
      g_printf ("tp_cli_connection_call_request_channel() failed: %s\n", error->message);
      return;
    }

  g_printf("DEBUG: channel D-Bus path received: %s\n", channel_dbus_path);


  /* Create the channel proxy for the ContactList's Group interface: */
  GError *error_inner = NULL;
  TpChannel *channel = tp_channel_new (connection, 
    channel_dbus_path, /* object_path */
    TP_IFACE_CHANNEL_TYPE_CONTACT_LIST, /* optional_channel_type */
    TP_HANDLE_TYPE_LIST, /* optional_handle_type */
    contact_list_handle, /* optional_handle */
    &error_inner);

  if (error_inner)
    {
      g_printf ("tp_channel_new() failed: %s\n", error_inner->message);
      g_clear_error (&error_inner);
      return;
    }

  /* Wait until the channel is ready for use: */
  tp_channel_call_when_ready (channel, 
    &on_channel_ready,
    NULL);
}

void on_connection_request_handles (TpConnection *proxy,
  const GArray *handles_array,
  const GError *error,
  gpointer user_data,
  GObject *weak_object)
{
  if (error)
    {
      g_printf ("tp_cli_connection_call_request_handles() failed: %s\n", error->message);
      return;
    }

  if (!handles_array || handles_array->len == 0)
    {
      g_printf ("No handles received.\n");
      return;
    }
 
  contact_list_handle = g_array_index (handles_array, guint, 0);
  g_printf("DEBUG: Count of handles received: %u\n", handles_array->len);
  g_printf("DEBUG: subscribe handle received: %u\n", contact_list_handle);


  /* Request the channel: */
  tp_cli_connection_call_request_channel (connection, 
    -1, /* timeout */
    TP_IFACE_CHANNEL_TYPE_CONTACT_LIST, /* in_Type */
    TP_HANDLE_TYPE_LIST, /* in_Handle_Type - the correct type for ContactList */
    contact_list_handle, /* in_Handle */
    TRUE, /* in_Suppress_Handler */
    &on_connection_request_channel, NULL, /* user_data */
    NULL, /* destroy */
    NULL /* weak_ref */);
}


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

void on_connection_status_changed (TpConnection *proxy,
  guint arg_Status,
  guint arg_Reason,
  gpointer user_data,
  GObject *weak_object)
{
  switch(arg_Status)
    {
      case TP_CONNECTION_STATUS_CONNECTED:
        g_printf ("Connection status: Connected (reason: %s)\n", get_reason_description (arg_Reason));

        /* Get the contacts information for this connection,
         * and then disconnect the connection: */
        
        /* Request the handle for the ContactList, which lists.
         * We need this handle to request the Group channel: */
        const gchar **identifier_names = (const gchar **)g_malloc0(2 * sizeof(char*));
        identifier_names[0] = "subscribe";
        tp_cli_connection_call_request_handles (connection, 
          -1, /* timeout */
          TP_HANDLE_TYPE_LIST, /* in_Handle_Type - the correct type for ContactList for server-defined lists, such as "subscribe". */
          identifier_names, /* in_Names */
          &on_connection_request_handles, NULL, /* user_data */
          NULL, /* destroy */
          NULL /* weak_ref */);

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
  g_value_set_static_string (value, "passwordTODO");
  g_hash_table_insert (parameters, "password", value);

  /* This jabber-specific parameter can avoid clashes with 
     other telepathy clients that use the default jabber 
     resource name. */
  value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, "telepathy-doc list_contacts example");
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
