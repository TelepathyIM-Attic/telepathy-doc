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
_completed_channel_request (GObject *request,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_account_channel_request_create_channel_finish (
        TP_ACCOUNT_CHANNEL_REQUEST (request), res, &error))
    {
      g_print ("Failed to request channel: %s\n", error->message);
      g_clear_error (&error);
    }
  else
    {
      g_print ("Successfully requested channel\n");
    }

  g_main_loop_quit (loop);
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
  gboolean call = FALSE, video = TRUE;

  if (!tp_proxy_prepare_finish (account, res, &error))
    handle_error (error);

  g_message ("Using %s interface %s video",
      call ? "Call" : "StreamedMedia",
      video ? "with" : "without");

  props = tp_asv_new (
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
      G_TYPE_UINT,
      TP_HANDLE_TYPE_CONTACT,

      TP_PROP_CHANNEL_TARGET_ID,
      G_TYPE_STRING,
      targetid,

      NULL);

  if (call)
    {
      tp_asv_set_string (props, TP_PROP_CHANNEL_CHANNEL_TYPE,
          /* Future: TP_IFACE_CHANNEL_TYPE_CALL */
          "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT");
      tp_asv_set_boolean (props,
          /* Future: TP_PROP_CHANNEL_TYPE_CALL_INITIAL_AUDIO */
          "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT.InitialAudio",
          TRUE);

      if (video)
        tp_asv_set_boolean (props,
            /* Future: TP_PROP_CHANNEL_TYPE_CALL_INITIAL_VIDEO */
            "org.freedesktop.Telepathy.Channel.Type.Call.DRAFT.InitialVideo",
            TRUE);
    }
  else
    {
      tp_asv_set_string (props, TP_PROP_CHANNEL_CHANNEL_TYPE,
          TP_IFACE_CHANNEL_TYPE_STREAMED_MEDIA);
      tp_asv_set_boolean (props,
          TP_PROP_CHANNEL_TYPE_STREAMED_MEDIA_INITIAL_AUDIO, TRUE);

      if (video)
        tp_asv_set_boolean (props,
            TP_PROP_CHANNEL_TYPE_STREAMED_MEDIA_INITIAL_VIDEO, TRUE);
    }

  request = tp_account_channel_request_new (TP_ACCOUNT (account), props,
      TP_USER_ACTION_TIME_CURRENT_TIME);

  tp_account_channel_request_create_channel_async (request, NULL,
      NULL, _completed_channel_request, NULL);

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
