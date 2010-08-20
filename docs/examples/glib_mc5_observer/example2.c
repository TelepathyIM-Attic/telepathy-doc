#include <telepathy-glib/telepathy-glib.h>


static guint
increment_pending (gpointer proxy)
{
  guint pending;

  pending = GPOINTER_TO_UINT (g_object_get_data (proxy, "pending"));
  pending++;
  g_object_set_data (proxy, "pending", GUINT_TO_POINTER (pending));

  g_debug ("%u calls pending", pending);

  return pending;
}


static guint
decrement_pending (gpointer proxy)
{
  guint pending;

  pending = GPOINTER_TO_UINT (g_object_get_data (proxy, "pending"));
  pending--;
  g_object_set_data (proxy, "pending", GUINT_TO_POINTER (pending));

  g_debug ("%u calls pending", pending);

  return pending;
}


static void
respond_to_context (TpObserveChannelsContext *context)
{
  if (decrement_pending (context) == 0)
    {
      g_debug ("ObserveChannels complete");

      tp_observe_channels_context_accept (context);

      g_object_unref (context);
    }
}


static void
list_pending_messaged_cb (TpChannel *channel,
    const GPtrArray *pending,
    const GError *in_error,
    gpointer user_data,
    GObject *context)
{
  respond_to_context (TP_OBSERVE_CHANNELS_CONTEXT (context));

  if (in_error != NULL)
    {
      g_warning ("%s", in_error->message);
      return;
    }
}


static void
channel_group_prepared (GObject *channel,
    GAsyncResult *res,
    gpointer user_data)
{
  TpObserveChannelsContext *context = user_data;
  GError *error = NULL;

  respond_to_context (context);

  if (!tp_proxy_prepare_finish (channel, res, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return;
    }
}


static void
observe_channels (TpSimpleObserver *observer,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    TpChannelDispatchOperation *dispatch,
    GList *requests,
    TpObserveChannelsContext *context,
    gpointer user_data)
{
  GList *l;

  g_debug ("ObserveChannels");

  for (l = channels; l != NULL; l = l->next)
    {
      TpChannel *channel = l->data;
      TpHandleType handle_type;

      /* request the pending message queue */
      tp_cli_channel_type_text_call_list_pending_messages (channel, -1,
          FALSE,
          list_pending_messaged_cb, NULL, NULL, G_OBJECT (context));
      increment_pending (context);

      tp_channel_get_handle (channel, &handle_type);
      if (handle_type == TP_HANDLE_TYPE_ROOM)
        {
          /* prepare the group property */
          GQuark features[] = { TP_CHANNEL_FEATURE_GROUP, 0 };

          tp_proxy_prepare_async (channel, features, channel_group_prepared,
              context);
          increment_pending (context);
        }
    }

  /* hold a reference to @context */
  g_object_ref (context);
  /* delay responding to @context until our callbacks have finished */
  tp_observe_channels_context_delay (context);
}


int
main (int argc,
    const char **argv)
{
  TpDBusDaemon *dbus;
  TpBaseClient *observer;
  GMainLoop *loop;
  GError *error = NULL;

  g_type_init ();

  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    g_error ("%s", error->message);

  observer = tp_simple_observer_new (dbus, TRUE, "ExampleObserver", TRUE,
      observe_channels, NULL, NULL);

  /* Observe both 1-to-1 text channels and MUCs */
  tp_base_client_take_observer_filter (observer, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE,
        G_TYPE_STRING,
        TP_IFACE_CHANNEL_TYPE_TEXT,

        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        G_TYPE_UINT,
        TP_HANDLE_TYPE_CONTACT,

        NULL));

  tp_base_client_take_observer_filter (observer, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE,
        G_TYPE_STRING,
        TP_IFACE_CHANNEL_TYPE_TEXT,

        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
        G_TYPE_UINT,
        TP_HANDLE_TYPE_ROOM,

        NULL));

  if (!tp_base_client_register (observer, &error))
    g_error ("%s", error->message);

  loop = g_main_loop_new (NULL, FALSE);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
  g_object_unref (observer);
  g_object_unref (dbus);
}
