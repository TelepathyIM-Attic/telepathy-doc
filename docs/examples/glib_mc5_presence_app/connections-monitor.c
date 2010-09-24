#include <string.h>

#include "connections-monitor.h"
#include "marshallers.h"

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_CONNECTIONS_MONITOR, ConnectionsMonitorPrivate))

G_DEFINE_TYPE (ConnectionsMonitor, connections_monitor, G_TYPE_OBJECT);

enum /* signals */
{
  CONNECTION,
  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

typedef struct _ConnectionsMonitorPrivate ConnectionsMonitorPrivate;
struct _ConnectionsMonitorPrivate
{
  TpAccountManager *am;
};


static const char *
shorten_account_name (TpAccount *account)
{
  return tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);
}


static void
_object_destroyed (gpointer data,
    GObject *ptr)
{
  g_debug ("Object finalized: %s: %p", (char *) data, ptr);
}


static void
_connection_invalidated (TpConnection *conn,
    guint domain,
    int code,
    char *message,
    gpointer user_data)
{
  g_debug ("Unreffing connection: %s", tp_proxy_get_object_path (conn));

  g_object_unref (conn);
}


static void
_connection_prepared (GObject *conn,
    GAsyncResult *res,
    gpointer user_data)
{
  ConnectionsMonitor *self = user_data;
  TpAccount *account;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (conn, res, &error))
    g_error ("%s", error->message);

  g_debug ("Prepared connection: %s", tp_proxy_get_object_path (conn));

  g_signal_connect (conn, "invalidated",
      G_CALLBACK (_connection_invalidated), self);

  account = g_object_get_data (conn, "account");

  g_signal_emit (self, _signals[CONNECTION], 0, account, conn);
}


static void
account_prepare_connection (ConnectionsMonitor *self,
    TpAccount *account)
{
  TpConnection *conn;

  conn = tp_account_get_connection (account);

  if (conn == NULL)
    return;

  g_debug ("Preparing connection: %s", tp_proxy_get_object_path (conn));

  /* reference released when the connection is invalidated */
  g_object_ref (conn);
  g_object_weak_ref (G_OBJECT (conn), _object_destroyed, "Connection");

  /* connection doesn't hold a ref to the account, it shouldn't outlive
   * the account */
  g_object_set_data (G_OBJECT (conn), "account", account);

  tp_proxy_prepare_async (conn, NULL, _connection_prepared, self);
}


static void
_account_notify_connection (GObject *account,
    GParamSpec *pspec,
    gpointer user_data)
{
  ConnectionsMonitor *self = user_data;

  account_prepare_connection (self, TP_ACCOUNT (account));
}


static void
_account_prepared (GObject *account,
    GAsyncResult *res,
    gpointer user_data)
{
  ConnectionsMonitor *self = user_data;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (account, res, &error))
    g_error ("%s", error->message);

  g_debug ("Prepared account %s", shorten_account_name (TP_ACCOUNT (account)));

  account_prepare_connection (self, TP_ACCOUNT (account));

  /* disconnect any old handlers */
  g_signal_handlers_disconnect_by_func (account,
      _account_notify_connection, self);

  g_signal_connect (account, "notify::connection",
      G_CALLBACK (_account_notify_connection), self);
}


static void
prepare_account (ConnectionsMonitor *self,
    TpAccount *account)
{
  g_debug ("Preparing account %s", shorten_account_name (account));

  /* reference released when the account is diabled */
  g_object_ref (account);
  g_object_weak_ref (G_OBJECT (account), _object_destroyed, "Account");

  tp_proxy_prepare_async (account, NULL, _account_prepared, self);
}


static void
_am_account_enabled (TpAccountManager *am,
    TpAccount *account,
    gpointer user_data)
{
  ConnectionsMonitor *self = user_data;

  prepare_account (self, account);
}


static void
_am_account_disabled (TpAccountManager *am,
    TpAccount *account,
    gpointer user_data)
{
  g_debug ("Unreffing account: %s", shorten_account_name (account));

  /* release the reference to the account acquired when we prepared it */
  g_object_unref (account);
}


static void
_am_prepared (GObject *am,
    GAsyncResult *res,
    gpointer user_data)
{
  ConnectionsMonitor *self = user_data;
  GList *accounts, *ptr;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (am, res, &error))
    g_error ("%s", error->message);

  g_debug ("AM prepared");

  /* prepare the valid accounts */
  accounts = tp_account_manager_get_valid_accounts (TP_ACCOUNT_MANAGER (am));

  for (ptr = accounts; ptr != NULL; ptr = ptr->next)
    prepare_account (self, ptr->data);

  g_signal_connect (am, "account-enabled",
      G_CALLBACK (_am_account_enabled), self);
  g_signal_connect (am, "account-disabled",
      G_CALLBACK (_am_account_disabled), self);

  g_list_free (accounts);
}


static void
connections_monitor_finalize (GObject *self)
{
  ConnectionsMonitorPrivate *priv = GET_PRIVATE (self);

  tp_clear_object (&priv->am);

  G_OBJECT_CLASS (connections_monitor_parent_class)->finalize (self);
}


static void
connections_monitor_class_init (ConnectionsMonitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = connections_monitor_finalize;

  _signals[CONNECTION] = g_signal_new ("connection",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (ConnectionsMonitorClass, connection),
      NULL, NULL,
      _example_VOID__OBJECT_OBJECT,
      G_TYPE_NONE,
      2, TP_TYPE_ACCOUNT, TP_TYPE_CONNECTION);

  g_type_class_add_private (gobject_class, sizeof (ConnectionsMonitorPrivate));
}


static void
connections_monitor_init (ConnectionsMonitor *self)
{
  ConnectionsMonitorPrivate *priv = GET_PRIVATE (self);

  /* prepare the Account Manager */
  priv->am = tp_account_manager_dup ();

  tp_proxy_prepare_async (priv->am, NULL, _am_prepared, self);
}

ConnectionsMonitor *
connections_monitor_new (void)
{
  return g_object_new (TYPE_CONNECTIONS_MONITOR, NULL);
}
