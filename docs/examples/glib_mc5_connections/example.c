/*
 * An example of talking to MC5 to get available connections
 */

#include <glib.h>

#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>

#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/account.h>
#include <telepathy-glib/connection.h>

static GMainLoop *loop = NULL;
static TpDBusDaemon *bus_daemon = NULL;

static void
get_connection_cb (TpProxy      *account,
                   const GValue *value,
                   const GError *in_error,
                   gpointer      user_data,
                   GObject      *weak_obj)
{
  GError *error = NULL;

  if (in_error) g_error ("%s", in_error->message);

  g_return_if_fail (G_VALUE_HOLDS (value, DBUS_TYPE_G_OBJECT_PATH));

  const char *path = g_value_get_boxed (value);

  if (!tp_strdiff (path, "/")) goto out;

  g_print ("Connection Path = %s\n", path);

  TpConnection *conn = tp_connection_new (bus_daemon, NULL, path, &error);
  if (error) g_error ("%s", error->message);

out:
  /* we're done with the Account */
  g_object_unref (account);
}

static void
get_valid_accounts_cb (TpProxy      *am,
                       const GValue *value,
                       const GError *in_error,
                       gpointer      user_data,
                       GObject      *weak_obj)
{
  GError *error = NULL;

  if (in_error) g_error ("%s", in_error->message);

  /* value is an (ao), which is a GPtrArray of allocated strings */
  g_return_if_fail (G_VALUE_HOLDS (value, TP_ARRAY_TYPE_OBJECT_PATH_LIST));

  GPtrArray *array = g_value_get_boxed (value);
  int i;
  for (i = 0; i < array->len; i++)
    {
      const char *path = g_ptr_array_index (array, i);

      g_print ("Account Path = %s\n", path);

      /* set up a TpAccount for each account */
      TpAccount *account = tp_account_new (bus_daemon, path, &error);
      if (error) g_error ("%s", error->message);

      /* request the Connection for each account */
      tp_cli_dbus_properties_call_get (account, -1,
          TP_IFACE_ACCOUNT,
          "Connection",
          get_connection_cb,
          NULL, NULL, NULL);
    }
}

int
main (int argc, char **argv)
{
  GError *error = NULL;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  bus_daemon = tp_dbus_daemon_dup (&error);
  if (error) g_error ("%s", error->message);

  /* establish a connection to the Account Manager */
  TpAccountManager *am = tp_account_manager_new (bus_daemon);

  /* get the list of ValidAccounts */
  tp_cli_dbus_properties_call_get (am, -1,
      TP_IFACE_ACCOUNT_MANAGER,
      "ValidAccounts",
      get_valid_accounts_cb,
      NULL, NULL, NULL);

  g_main_loop_run (loop);
}
