#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/telepathy-glib.h>

static GMainLoop *loop = NULL;

static void
handle_error (const GError *error)
{
  if (error != NULL)
    g_error ("ERROR: %s\n", error->message);
}


static void
_channel_group_ready (GObject *channel,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error;

  if (!tp_proxy_prepare_finish (channel, res, &error))
    handle_error (error);

  g_print ("Handled channel %s\n",
      tp_proxy_get_object_path (channel));
}


static void
_channel_invalidated (TpProxy *channel,
    guint domain,
    guint code,
    const char *message,
    gpointer user_data)
{
  g_print ("Channel invalidated %s\n",
      tp_proxy_get_object_path (channel));

  /* unref the request */
  g_object_set_data (G_OBJECT (channel), "request", NULL);

  /* unref the channel */
  g_object_unref (channel);
}


static void
_channel_rehandled (TpAccountChannelRequest *request,
    TpChannel *channel,
    gint64 user_action_time,
    TpHandleChannelsContext *context,
    gpointer user_data)
{
  g_print ("Channel rehandled %s\n",
      tp_proxy_get_object_path (channel));
}


static void
_channel_ready (GObject *request,
    GAsyncResult *res,
    gpointer user_data)
{
  TpChannel *channel;
  GQuark features[] = { TP_CHANNEL_FEATURE_GROUP, 0 };
  GError *error = NULL;

  channel = tp_account_channel_request_ensure_and_handle_channel_finish (
      TP_ACCOUNT_CHANNEL_REQUEST (request), res, NULL, &error);
  if (channel == NULL)
    handle_error (error);

  /* prepare the GROUP feature on this channel */
  tp_proxy_prepare_async (channel, features, _channel_group_ready, NULL);

  /* store a reference to the request so we can connect re-handled */
  g_object_set_data_full (G_OBJECT (channel), "request",
      g_object_ref (request), g_object_unref);

  /* connect the signals we want to listen to */
  g_signal_connect (channel, "invalidated",
      G_CALLBACK (_channel_invalidated), NULL);

  g_signal_connect (request, "re-handled",
      G_CALLBACK (_channel_rehandled), NULL);
}


static void
_account_ready (GObject *account,
    GAsyncResult *res,
    gpointer user_data)
{
  TpAccountChannelRequest *request;
  GHashTable *props;
  const char *targetid = user_data;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, res, &error))
    handle_error (error);

  props = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE,
      G_TYPE_STRING,
      TP_IFACE_CHANNEL_TYPE_TEXT,

      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
      G_TYPE_UINT,
      TP_HANDLE_TYPE_ROOM,

      TP_PROP_CHANNEL_TARGET_ID,
      G_TYPE_STRING,
      targetid,

      NULL);

  request = tp_account_channel_request_new (TP_ACCOUNT (account), props,
      TP_USER_ACTION_TIME_CURRENT_TIME);

  tp_account_channel_request_ensure_and_handle_channel_async (request,
      NULL, _channel_ready, NULL);

  g_object_unref (request);
  g_hash_table_destroy (props);
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

  if (argc != 3)
    g_error ("Must provide an account and target id!");

  /* create a main loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* acquire a connection to the D-Bus daemon */
  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    handle_error (error);

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
  tp_proxy_prepare_async (account, NULL, _account_ready, argv[2]);

  g_main_loop_run (loop);

  g_object_unref (dbus);
  g_object_unref (account);

  return 0;
}
