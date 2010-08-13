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
           guint    state,
           gpointer    user_data,
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
  guint key;
  char *value;

  g_print ("::DBusNamesChanged\n");

  g_print ("Added:\n");
  g_hash_table_iter_init (&iter, added);
  while (g_hash_table_iter_next (&iter,
        (gpointer *) &key, (gpointer *) &value))
    g_print (" - %u: %s\n", key, value);
}


static void
tube_offer_cb (TpChannel  *channel,
         const char  *address,
         const GError  *in_error,
         gpointer     user_data,
         GObject    *weak_obj)
{
  handle_error (in_error);

  g_print (" > tube_offer_cb (%s)\n", address);
}


static void
_tube_channel_ready (GObject *request,
    GAsyncResult *res,
    gpointer user_data)
{
  TpChannel *channel;
  GHashTable *parameters;
  GError *error = NULL;

  channel = tp_account_channel_request_create_and_handle_channel_finish (
      TP_ACCOUNT_CHANNEL_REQUEST (request), res, NULL, &error);
  if (channel == NULL)
    handle_error (error);

  g_print (" > channel_ready (%s)\n",
      tp_channel_get_identifier (channel));

  /* FIXME: this will all be replaced by TpDBusTube */
  tp_cli_channel_interface_tube_connect_to_tube_channel_state_changed (
      channel, tube_state_changed_cb,
      NULL, NULL, NULL, &error);
  handle_error (error);

  tp_cli_channel_type_dbus_tube_connect_to_dbus_names_changed (
      channel, dbus_names_changed_cb,
      NULL, NULL, NULL, &error);
  handle_error (error);

  g_print ("Offering Tube...\n");
  parameters = tp_asv_new (
      "SomeKey", G_TYPE_STRING, "SomeValue",
      "IntKey", G_TYPE_INT, 42,
      NULL);

  tp_cli_channel_type_dbus_tube_call_offer (channel, -1,
      parameters, TP_SOCKET_ACCESS_CONTROL_LOCALHOST,
      tube_offer_cb, NULL, NULL, NULL);

  g_hash_table_destroy (parameters);
}


static GHashTable *
_tp_asv_copy (GHashTable *in)
{
  GHashTable *out;

  if (in == NULL)
    return NULL;

  out = tp_asv_new (NULL, NULL);

  /* keys are not copied (always assumed to be static strings) */
  tp_g_hash_table_update (out, in, NULL, (GBoxedCopyFunc) tp_g_value_slice_dup);

  return out;
}


static void
_muc_channel_ready (GObject *request,
    GAsyncResult *res,
    gpointer user_data)
{
  TpAccountChannelRequest *request2;
  GHashTable *props;
  GError *error = NULL;

  if (!tp_account_channel_request_ensure_channel_finish (
      TP_ACCOUNT_CHANNEL_REQUEST (request), res, &error))
    handle_error (error);

  g_print ("MUC channel ensured\n");

  /* Dial up a D-Bus Tube using the same request */
  props = _tp_asv_copy (tp_account_channel_request_get_request (
      TP_ACCOUNT_CHANNEL_REQUEST (request)));

  tp_asv_set_string (props,
      TP_PROP_CHANNEL_CHANNEL_TYPE,
      TP_IFACE_CHANNEL_TYPE_DBUS_TUBE);
  tp_asv_set_string (props,
      TP_PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME,
      "com.example.Telepathy.DbusTube");

  request2 = tp_account_channel_request_new (
      tp_account_channel_request_get_account (
        TP_ACCOUNT_CHANNEL_REQUEST (request)),
      props, G_MAXINT64 /* current time */);

  tp_account_channel_request_create_and_handle_channel_async (request2,
      NULL, _tube_channel_ready, NULL);

  g_hash_table_destroy (props);
}


static void
_account_ready (GObject *account,
    GAsyncResult *res,
    gpointer user_data)
{
  TpAccountChannelRequest *request;
  GHashTable *props;
  char **argv = (char **) user_data;
  char *targetid = argv[2];
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, res, &error))
    handle_error (error);

  props = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, TP_TYPE_HANDLE, TP_HANDLE_TYPE_ROOM,
      TP_PROP_CHANNEL_TARGET_ID, G_TYPE_STRING, targetid,
      NULL);

  request = tp_account_channel_request_new (TP_ACCOUNT (account),
      props, G_MAXINT64 /* current time */);

  /* ensure this channel, but let the default handler handle it */
  tp_account_channel_request_ensure_channel_async (request,
      NULL, NULL, _muc_channel_ready, NULL);

  g_hash_table_destroy (props);
  g_object_unref (request);
}


int
main (int argc, char **argv)
{
  TpDBusDaemon *dbus;
  TpAccount *account;
  char *account_path;
  gpointer user_data = argv;
  GError *error = NULL;

  g_type_init ();

  if (argc != 3)
    g_error ("Must provide account and MUC name!");

  /* create a main loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* acquire a connection to the D-Bus daemon */
  dbus = tp_dbus_daemon_dup (&error);
  if (dbus == NULL)
    handle_error (error);

  /* get the complete path of the account */
  account_path = g_strconcat (TP_ACCOUNT_OBJECT_PATH_BASE, argv[1], NULL);

  /* begin ex.basics.language-bindings.telepathy-glib.ready */
  /* get the account */
  account = tp_account_new (dbus, account_path, &error);
  if (account == NULL)
    handle_error (error);

  /* prepare the core account features */
  tp_proxy_prepare_async (account, NULL, _account_ready, user_data);
  /* end ex.basics.language-bindings.telepathy-glib.ready */

  g_free (account_path);

  g_main_loop_run (loop);

  g_object_unref (dbus);
  g_object_unref (account);

  return 0;
}
