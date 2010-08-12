/*
 * An example of talking to MC5 to get available connections
 */

#include <glib.h>

#include <telepathy-glib/telepathy-glib.h>

static GMainLoop *loop = NULL;
static guint pending_requests = 0;

static void
get_statuses_cb (TpProxy      *conn,
                 const GValue *value,
                 const GError *in_error,
                 gpointer      user_data,
                 GObject      *weak_obj)
{
  GHashTable *map;
  GHashTableIter iter;
  const char *k;
  GValueArray *v;
  GError *error = NULL;

  if (in_error) g_error ("%s", in_error->message);

  g_return_if_fail (G_VALUE_HOLDS (value, TP_HASH_TYPE_SIMPLE_STATUS_SPEC_MAP));

  g_print ("%s\n", tp_proxy_get_object_path (conn));

  map = g_value_get_boxed (value);

  g_hash_table_iter_init (&iter, map);
  while (g_hash_table_iter_next (&iter, (gpointer) &k, (gpointer) &v))
    {
      char *str;

      g_print ("%s -> (", k);

      str = g_strdup_value_contents (g_value_array_get_nth (v, 0));
      g_print ("%s, ", str);
      g_free (str);

      str = g_strdup_value_contents (g_value_array_get_nth (v, 1));
      g_print ("%s, ", str);
      g_free (str);

      str = g_strdup_value_contents (g_value_array_get_nth (v, 2));
      g_print ("%s)\n", str);
      g_free (str);
    }

  pending_requests--;
  if (pending_requests == 0)
    g_main_loop_quit (loop);
}


static void
_conn_ready (GObject *conn,
    GAsyncResult *res,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (conn, res, &error))
    g_error ("%s", error->message);

  /* request the Statuses property */
  tp_cli_dbus_properties_call_get (conn, -1,
      TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
      "Statuses",
      get_statuses_cb,
      NULL, NULL, NULL);
}


static void
_account_ready (GObject *account,
    GAsyncResult *res,
    gpointer user_data)
{
  TpConnection *conn;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, res, &error))
    g_error ("%s", error->message);

  conn = tp_account_get_connection (TP_ACCOUNT (account));
  if (conn == NULL)
    {
      g_print ("Account %s not connected\n",
          tp_account_get_display_name (TP_ACCOUNT (account)));

      pending_requests--;
      return;
    }

  g_print ("Account %s: connection path = %s\n",
      tp_account_get_display_name (TP_ACCOUNT (account)),
      tp_proxy_get_object_path (conn));

  /* prepare the connection */
  tp_proxy_prepare_async (conn, NULL, _conn_ready, NULL);
}


static void
_am_ready (GObject *am,
    GAsyncResult *res,
    gpointer user_data)
{
  GList *accounts, *l;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (am, res, &error))
    g_error ("%s", error->message);

  /* the list of valid accounts (the ValidAccounts property) */
  accounts = tp_account_manager_get_valid_accounts (TP_ACCOUNT_MANAGER (am));

  for (l = accounts; l != NULL; l = l->next)
    {
      TpAccount *account = TP_ACCOUNT (l->data);

      /* prepare each account */
      tp_proxy_prepare_async (account, NULL, _account_ready, NULL);

      pending_requests++;
    }

  g_list_free (accounts);
}


int
main (int argc,
    char **argv)
{
  TpAccountManager *am;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  /* get the Account Manager */
  am = tp_account_manager_dup ();
  if (am == NULL)
    g_error ("Couldn't not connect to Account Manager");

  /* prepare the AM */
  tp_proxy_prepare_async (am, NULL, _am_ready, NULL);

  g_main_loop_run (loop);

  g_object_unref (am);
}
