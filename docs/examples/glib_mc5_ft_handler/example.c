/*
 * Example for using TpSimpleHandler
 */

#include <glib.h>

#include <telepathy-glib/telepathy-glib.h>

#define CLIENT_NAME "ExampleFTHandler"

static GMainLoop *loop = NULL;


static void
handle_channels (TpSimpleHandler *handler,
    TpAccount *account,
    TpConnection *conn,
    GList *channels,
    GList *requests,
    gint64 user_action_time,
    TpHandleChannelsContext *context,
    gpointer user_data)
{
  GList *l;
  GError *error = NULL;

  g_print (" > handle_channels\n");
  g_print ("     account = %s\n", tp_proxy_get_object_path (account));
  g_print ("     connection = %s\n", tp_proxy_get_object_path (conn));

  /* @channels is a GList<TpChannel> with the CORE feature prepared */
  for (l = channels; l != NULL; l = l->next)
    {
      TpChannel *channel = TP_CHANNEL (l->data);

      g_print ("     channel = %s\n", tp_proxy_get_object_path (channel));
    }

  /* we need to accept, delay or fail the HandleChannels request */
  g_set_error (&error, TP_ERRORS, TP_ERROR_NOT_IMPLEMENTED,
      "This handler doesn't actually handle channels");
  tp_handle_channels_context_fail (context, error);
  g_clear_error (&error);
}


int
main (int argc,
    char **argv)
{
  TpDBusDaemon *dbus;
  TpBaseClient *handler;
  GError *error = NULL;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    g_error ("%s", error->message);

  /* begin ex.channel-dispatcher.clients.impl.tpsimplehandler */
  /* create a new Handler */
  handler = tp_simple_handler_new (dbus, FALSE, FALSE, CLIENT_NAME, FALSE,
      handle_channels, NULL, NULL);

  /* add a channel filter */
  tp_base_client_take_handler_filter (handler, tp_asv_new (
        /* only FT channels */
        TP_PROP_CHANNEL_CHANNEL_TYPE,
        G_TYPE_STRING,
        TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER,
        /* we always need a TargetHandleType so that the Handler's
         * capabilities are exported via the Connection Manager */
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        G_TYPE_UINT,
        TP_HANDLE_TYPE_CONTACT,

        NULL));

  /* register the Handler on the D-Bus */
  if (!tp_base_client_register (handler, &error))
    g_error ("%s", error->message);
  /* end ex.channel-dispatcher.clients.impl.tpsimplehandler */

  g_main_loop_run (loop);

  g_object_unref (dbus);
  g_object_unref (handler);
}
