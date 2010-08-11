#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/telepathy-glib.h>

static GMainLoop *loop = NULL;

static void
handle_error (const GError *error)
{
  if (error != NULL)
    {
      g_print ("ERROR: %s\n", error->message);
      g_main_loop_quit (loop);
    }
}


/* begin ex.channel.contactlist.user-defined.glib */
static void
new_channels_cb (TpConnection *conn,
    const GPtrArray *channels,
    gpointer user_data,
    GObject *weak_obj)
{
  int i;
  GError *error = NULL;

  /* @channels has the D-Bus type a(oa{sv}), which decomposes to:
   *  - a GPtrArray containing a GValueArray for each channel
   *  - each GValueArray contains
   *     - an object path
   *     - an a{sv} map
   */
  for (i = 0; i < channels->len; i++)
    {
      GValueArray *channel = g_ptr_array_index (channels, i);
      char *object_path;
      GHashTable *map;

      const char *channel_type, *target_id;
      int handle_type;

      tp_value_array_unpack (channel, 2,
          &object_path,
          &map);

      channel_type = tp_asv_get_string (map, TP_PROP_CHANNEL_CHANNEL_TYPE);
      handle_type = tp_asv_get_uint32 (map, TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
          NULL);
      target_id = tp_asv_get_string (map, TP_PROP_CHANNEL_TARGET_ID);

      /* ensure this channel is a contact group */
      if (!tp_strdiff (channel_type, TP_IFACE_CHANNEL_TYPE_CONTACT_LIST) &&
          handle_type == TP_HANDLE_TYPE_GROUP)
        {
          g_print ("Got user-defined contact group: %s\n", target_id);
        }
    }
}
/* end ex.channel.contactlist.user-defined.glib */


static void
get_channels_cb (TpProxy *conn,
    const GValue *value,
    const GError *in_error,
    gpointer user_data,
    GObject *weak_obj)
{
  handle_error (in_error);

  g_return_if_fail (G_VALUE_HOLDS (value,
        TP_ARRAY_TYPE_CHANNEL_DETAILS_LIST));

  GPtrArray *channels = g_value_get_boxed (value);

  new_channels_cb (TP_CONNECTION (conn), channels, user_data, weak_obj);
}


static void
_conn_ready (GObject *conn,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (conn, res, &error))
    {
      handle_error (error);
    }

  g_print (" > conn ready\n");

  /* check if the Requests interface is available */
  if (tp_proxy_has_interface_by_id (conn,
        TP_IFACE_QUARK_CONNECTION_INTERFACE_REQUESTS))
    {
      /* request the current channels */
      tp_cli_dbus_properties_call_get (conn, -1,
          TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
          "Channels",
          get_channels_cb,
          NULL, NULL, NULL);

      /* notify of all new channels */
      tp_cli_connection_interface_requests_connect_to_new_channels (
          TP_CONNECTION (conn), new_channels_cb,
          NULL, NULL, NULL, &error);
      handle_error (error);
    }
  else
    {
      g_error ("No requests interface, antique CM?");
    }
}


static void
_account_ready (GObject *account,
    GAsyncResult *res,
    gpointer user_data)
{
  TpConnection *conn;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, res, &error))
    {
      handle_error (error);
    }

  g_print (" > account ready\n");

  /* get the connection */
  conn = tp_account_get_connection (TP_ACCOUNT (account));

  /* prepare the connection */
  tp_proxy_prepare_async (conn, NULL, _conn_ready, NULL);
}


int
main (int argc,
    char **argv)
{
  TpDBusDaemon *dbus;
  TpAccount *account;
  char *account_path;
  GError *error = NULL;

  g_type_init ();

  if (argc != 2)
    {
      g_error ("Must provide an account!");
    }

  /* create a main loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* acquire a connection to the D-Bus daemon */
  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    {
      handle_error (error);
    }

  /* get the complete path of the account */
  account_path = g_strconcat (TP_ACCOUNT_OBJECT_PATH_BASE, argv[1], NULL);

  /* get the account */
  account = tp_account_new (dbus, account_path, &error);
  if (account == NULL)
    {
      handle_error (error);
    }

  g_free (account_path);

  /* prepare the account */
  tp_proxy_prepare_async (account, NULL, _account_ready, NULL);

  g_main_loop_run (loop);

  g_object_unref (dbus);
  g_object_unref (account);

  return 0;
}
