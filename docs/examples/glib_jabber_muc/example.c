#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/telepathy-glib.h>

static GMainLoop *loop = NULL;

static void
handle_error (const GError *error)
{
  if (error)
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

  /* channels has the D-Bus type a(oa{sv}), which decomposes to:
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
      const char *type, *id;

      tp_value_array_unpack (channel, 2,
          &object_path,
          &map);

      type = tp_asv_get_string (map, TP_PROP_CHANNEL_CHANNEL_TYPE);
      id = tp_asv_get_string (map, TP_PROP_CHANNEL_TARGET_ID);

      g_print ("New channel %s: %s\n", type, id);
    }
}
/* end ex.channel.contactlist.user-defined.glib */

static void
get_channels_cb (TpProxy  *conn,
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
create_roomlist_cb (TpConnection *conn,
    const char *object_path,
    GHashTable *props,
    const GError *in_error,
    gpointer user_data,
    GObject *weak_obj)
{
  handle_error (in_error);
  GError *error = NULL;

  g_print (" > create_roomlist_cb (%s)\n", object_path);

  TpChannel *channel = tp_channel_new_from_properties (conn,
      object_path, props, &error);
  handle_error (error);

  tp_asv_dump (props);

  /* we didn't really want this channel anyway */
  tp_cli_channel_call_close (channel, -1, NULL, NULL, NULL, NULL);
  g_object_unref (channel);
}

static void
list_properties_cb (TpProxy *channel,
    const GPtrArray *available_properties,
    const GError *in_error,
    gpointer user_data,
    GObject *weak_obj)
{
  GArray *req;
  int i;

  handle_error (in_error);

  g_print (" > list_properties_cb\n");

  req = g_array_sized_new (FALSE, FALSE, sizeof (guint),
      available_properties->len);

  /* @available_properties is a GPtrArray of GValueArray structs
   * of signature (ussu) */
  for (i = 0; i < available_properties->len; i++)
    {
      GValueArray *prop = g_ptr_array_index (available_properties, i);
      guint id, flags;
      const char *name, *sig;

      tp_value_array_unpack (prop, 4,
          &id,
          &name,
          &sig,
          &flags);

      g_print ("%u %s (%s) %x\n", id, name, sig, flags);

      /* pack the readable properties into a GArray */
      if (flags & TP_PROPERTY_FLAG_READ)
        {
          req = g_array_append_val (req, id);
        }
    }

  // FIXME: what happens to req ?
}


static void
get_capabilities (TpConnection *connection,
    const GPtrArray *capabilities,
    const GError *in_error,
    gpointer user_data,
    GObject *weak_obj)
{
  int i;

  if (in_error != NULL)
    {
      g_print ("ERROR: %s\n", in_error->message);
      return;
    }

  g_print ("get capabilities\n");

  for (i = 0; i < capabilities->len; i++)
    {
      GValueArray *values = g_ptr_array_index (capabilities, i);

      g_print (" - %u :: %s\n",
        g_value_get_uint (g_value_array_get_nth (values, 0)),
        g_value_get_string (g_value_array_get_nth (values, 1)));
    }
}


static void
get_contact_capabilities (TpConnection *connection,
    GHashTable *capabilities,
    const GError *in_error,
    gpointer user_data,
    GObject *weak_obj)
{
  GHashTableIter iter;
  gpointer k;
  GPtrArray *v;

  if (in_error != NULL)
    {
      g_print ("ERROR: %s\n", in_error->message);
      return;
    }

  g_print ("get contact capabilities\n");

  g_hash_table_iter_init (&iter, capabilities);
  while (g_hash_table_iter_next (&iter, &k, (gpointer) &v))
    {
      int handle = GPOINTER_TO_INT (k);
      int i;

      g_print ("h = %i\n", handle);

      for (i = 0; i < v->len; i++)
        {
          GValueArray *values;

          g_print (" - Requestable channel type %i\n", i + 1);

          values = g_ptr_array_index (v, i);
          tp_asv_dump (g_value_get_boxed (g_value_array_get_nth (values, 0)));
        }
    }
}


static void
_muc_channel_ready (GObject *channel,
    GAsyncResult *res,
    gpointer user_data)
{
  const TpIntSet *members;
  GArray *handles;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (channel, res, &error))
    {
      handle_error (error);
    }

  g_print ("MUC channel (%s) ready\n",
      tp_channel_get_identifier (TP_CHANNEL (channel)));

  /* exciting things about MUC channels are stored as Telepathy
   * Properties (not D-Bus properties). This interface is a little
   * awkward.
   * First we need to get a list of available properties */
  tp_cli_properties_interface_call_list_properties (channel, -1,
      list_properties_cb, NULL, NULL, NULL);

  members = tp_channel_group_get_members (TP_CHANNEL (channel));
  handles = tp_intset_to_array (members);

  tp_cli_connection_interface_contact_capabilities_call_get_contact_capabilities (
      tp_channel_borrow_connection (TP_CHANNEL (channel)),
      -1, handles,
      get_contact_capabilities,
      NULL, NULL, NULL);

  tp_cli_connection_interface_capabilities_call_get_capabilities (
      tp_channel_borrow_connection (TP_CHANNEL (channel)),
      -1, handles,
      get_capabilities,
      NULL, NULL, NULL);

  g_array_free (handles, TRUE);
}


static void
create_muc_cb (TpConnection *conn,
    gboolean yours,
    const char *object_path,
    GHashTable *props,
    const GError *in_error,
    gpointer user_data,
    GObject *weak_obj)
{
  GError *error = NULL;

  handle_error (in_error);

  g_print (" > create_muc_cb (%s)\n", object_path);

  TpChannel *channel = tp_channel_new_from_properties (conn,
      object_path, props, &error);
  handle_error (error);

  tp_proxy_prepare_async (channel, NULL, _muc_channel_ready, NULL);
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

  g_print (" > conn_ready\n");

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

      /* begin example.channel.roomlist.requestglib */
      /* request a RoomList channel */
      GHashTable *map = tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_ROOM_LIST,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_NONE,
        /* we omit TargetHandle because it's anonymous */
        NULL);

      tp_cli_connection_interface_requests_call_create_channel (
          TP_CONNECTION (conn), -1, map,
          create_roomlist_cb,
          NULL, NULL, NULL);

      g_hash_table_destroy (map);
      /* end example.channel.roomlist.requestglib */

      /* make a connection to a MUC channel */
      map = tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_ROOM,
        TP_PROP_CHANNEL_TARGET_ID, G_TYPE_STRING, "test@conference.collabora.co.uk",
        NULL);

      tp_cli_connection_interface_requests_call_ensure_channel (
          TP_CONNECTION (conn), -1, map,
          create_muc_cb,
          NULL, NULL, NULL);

      g_hash_table_destroy (map);
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
