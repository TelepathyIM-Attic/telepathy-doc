#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/telepathy-glib.h>

static GMainLoop *loop = NULL;

static void
handle_error (const GError *error)
{
  if (error != NULL)
    {
      g_error ("ERROR: %s", error->message);
    }
}

static void
tube_state_changed_cb (TpChannel *channel,
           guint      state,
           gpointer   user_data,
           GObject   *weak_obj)
{
  g_print ("Tube state changed %i\n", state);
}

static void
dbus_names_changed_cb (TpChannel    *channel,
           GHashTable   *added,
           const GArray *removed,
           gpointer      user_data,
           GObject      *weak_obj)
{
  GHashTableIter iter;
  gpointer key, value;
  guint handle;
  char *address;

  g_print ("::DBusNamesChanged\n");

  g_print ("Added:\n");
  g_hash_table_iter_init (&iter, added);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      handle = GPOINTER_TO_UINT (key);
      address = (char *) value;

      g_print (" - %u: %s\n", handle, address);
    }
}

static void
tube_accept_cb (TpChannel  *channel,
          const char  *address,
          const GError  *in_error,
          gpointer   user_data,
          GObject    *weak_obj)
{
  handle_error (in_error);

  g_print (" > tube_accept_cb (%s)\n", address);
}

static void
_handle_dbus_channel (TpSimpleHandler *handler,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    GList *requests,
    gint64 user_action_time,
    TpHandleChannelsContext *context,
    gpointer user_data)
{
  GList *l;
  GError *error = NULL;

  for (l = channels; l != NULL; l = l->next)
    {
      TpChannel *channel = TP_CHANNEL (l->data);

      g_print (" > incoming channel (%s)\n",
          tp_channel_get_identifier (channel));

      if (tp_channel_get_channel_type_id (channel) !=
          TP_IFACE_QUARK_CHANNEL_TYPE_DBUS_TUBE)
        continue;

      tp_cli_channel_interface_tube_connect_to_tube_channel_state_changed (
          channel, tube_state_changed_cb,
          NULL, NULL, NULL, &error);
      handle_error (error);

      tp_cli_channel_type_dbus_tube_connect_to_dbus_names_changed (
          channel, dbus_names_changed_cb,
          NULL, NULL, NULL, &error);
      handle_error (error);

      /* accept the channel */
      tp_cli_channel_type_dbus_tube_call_accept (channel, -1,
          TP_SOCKET_ACCESS_CONTROL_LOCALHOST,
          tube_accept_cb, NULL, NULL, NULL);
    }

  tp_handle_channels_context_accept (context);
}


static void
_muc_channel_ready (GObject *request,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_account_channel_request_ensure_channel_finish (
        TP_ACCOUNT_CHANNEL_REQUEST (request), res, &error))
    handle_error (error);

  g_print ("MUC channel ensured\n");
}


static void
_account_ready (GObject *account,
    GAsyncResult *res,
    gpointer user_data)
{
  TpDBusDaemon *dbus;
  TpBaseClient *handler;
  TpAccountChannelRequest *request;
  GHashTable *props;
  char **argv = (char **) user_data;
  char *targetid = argv[2];
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, res, &error))
    handle_error (error);

  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    handle_error (error);

  /* set up a handler for the Tube we're going to receive */
  handler = tp_simple_handler_new (dbus,
      TRUE, FALSE, "DBusTubeHandler", TRUE,
      _handle_dbus_channel, NULL, NULL);

  tp_base_client_take_handler_filter (handler,
      tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE,
        G_TYPE_STRING,
        TP_IFACE_CHANNEL_TYPE_DBUS_TUBE,

        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        G_TYPE_UINT,
        TP_HANDLE_TYPE_ROOM,

        TP_PROP_CHANNEL_TARGET_ID,
        G_TYPE_STRING,
        targetid,

        TP_PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME,
        G_TYPE_STRING,
        "com.example.Telepathy.DbusTube",

        NULL));

  if (!tp_base_client_register (handler, &error))
    handle_error (error);

  /* ensure the MUC channel, but let the default handler handle it */
  props = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_ROOM,
      TP_PROP_CHANNEL_TARGET_ID, G_TYPE_STRING, targetid,
      NULL);

  request = tp_account_channel_request_new (TP_ACCOUNT (account),
      props, G_MAXINT64 /* current time */);

  tp_account_channel_request_ensure_channel_async (request,
      NULL, NULL, _muc_channel_ready, NULL);

  g_hash_table_destroy (props);
  g_object_unref (request);
  g_object_unref (dbus);
}


int
main (int argc,
    char **argv)
{
  TpDBusDaemon *dbus;
  TpAccount *account;
  char *account_path;
  gpointer user_data = argv;
  GError *error = NULL;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    handle_error (error);

  account_path = g_strconcat (TP_ACCOUNT_OBJECT_PATH_BASE, argv[1], NULL);
  account = tp_account_new (dbus, account_path, &error);
  if (account == NULL)
    handle_error (error);

  g_free (account_path);

  tp_proxy_prepare_async (account, NULL, _account_ready, user_data);

  g_main_loop_run (loop);

  g_object_unref (dbus);
  g_object_unref (account);

  return 0;
}
