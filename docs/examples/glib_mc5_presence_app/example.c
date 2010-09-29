#include <string.h>

#include "connections-monitor.h"


static const char *
shorten_account_name (TpAccount *account)
{
  return tp_proxy_get_object_path (account) +
    strlen (TP_ACCOUNT_OBJECT_PATH_BASE);
}


static void
_contact_presence_changed (TpContact *contact,
    TpConnectionPresenceType type,
    const char *status,
    const char *message,
    TpAccount *account)
{
  g_debug ("Contact status changed: %s: %s",
      tp_contact_get_alias (contact),
      status);

  switch (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (contact), "presence")))
    {
      case TP_CONNECTION_PRESENCE_TYPE_UNSET:
      case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
      case TP_CONNECTION_PRESENCE_TYPE_AWAY:
      case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
      case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
      case TP_CONNECTION_PRESENCE_TYPE_ERROR:
        /* contact was originally offline or away */
        switch (type)
          {
            case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
            case TP_CONNECTION_PRESENCE_TYPE_BUSY:
              /* contact is now available/busy */
              g_message ("%s is now online", tp_contact_get_alias (contact));
              break;

            default:
              break;
          }
        break;

      case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
      case TP_CONNECTION_PRESENCE_TYPE_BUSY:
        /* contact was originally available/busy */
        switch (type)
          {
            case TP_CONNECTION_PRESENCE_TYPE_AWAY:
            case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
              /* contact is now away */
              g_message ("%s is away", tp_contact_get_alias (contact));
              break;

            case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
            case TP_CONNECTION_PRESENCE_TYPE_BUSY:
              /* contact is still available/busy */
              break;

            default:
              g_message ("%s is now offline", tp_contact_get_alias (contact));
              break;
          }
        break;
    }
}


static void
_got_contacts (TpConnection *conn,
    guint n_contacts,
    TpContact * const *contacts,
    const char * const *requested_ids,
    GHashTable *failed_ids,
    const GError *in_error,
    gpointer user_data,
    GObject *account)
{
  guint i;

  if (in_error != NULL)
    {
      g_warning ("%s", in_error->message);
      return;
    }

  g_debug ("Loaded %u contacts", n_contacts);

  for (i = 0; i < n_contacts; i++)
    {
      TpContact *contact = contacts[i];

      _contact_presence_changed (contact,
          tp_contact_get_presence_type (contact),
          tp_contact_get_presence_status (contact),
          tp_contact_get_presence_message (contact),
          TP_ACCOUNT (account));

      g_signal_connect (contact, "presence-changed",
          G_CALLBACK (_contact_presence_changed), account);
    }
}


static void
_got_connection (ConnectionsMonitor *monitor,
    TpAccount *account,
    TpConnection *conn,
    GKeyFile *keyfile)
{
  TpContactFeature features[] = {
      TP_CONTACT_FEATURE_ALIAS,
      TP_CONTACT_FEATURE_AVATAR_TOKEN,
      TP_CONTACT_FEATURE_PRESENCE
  };
  char **contacts;
  GError *error = NULL;
  gsize length;

  if (!g_key_file_has_key (keyfile,
        shorten_account_name (account), "contacts",
        NULL))
    return;

  contacts = g_key_file_get_string_list (keyfile,
      shorten_account_name (account), "contacts",
      &length, &error);
  if (error != NULL)
    {
      g_warning ("%s", error->message);
      return;
    }

  tp_connection_get_contacts_by_id (conn,
      length, (const gchar * const*) contacts,
      G_N_ELEMENTS (features), features,
      _got_contacts, NULL, NULL, G_OBJECT (account));

  g_strfreev (contacts);
}


int
main (int argc,
    const char **argv)
{
  GKeyFile *keyfile;
  ConnectionsMonitor *monitor;
  GMainLoop *loop;
  GError *error = NULL;

  g_type_init ();

  keyfile = g_key_file_new ();
  if (!g_key_file_load_from_file (keyfile, argv[1], G_KEY_FILE_NONE, &error))
    g_error ("%s", error->message);

  monitor = connections_monitor_new ();
  g_signal_connect (monitor, "connection",
      G_CALLBACK (_got_connection), keyfile);

  loop = g_main_loop_new (NULL, FALSE);

  /* run the program */
  g_main_loop_run (loop);

  g_main_loop_unref (loop);
  g_object_unref (monitor);
  g_key_file_free (keyfile);
}
