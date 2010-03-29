/*
 * A user interface for controlling the presence of my various Telepathy
 * accounts.
 *
 * Authors:
 * 	Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <gtk/gtk.h>

#include <telepathy-glib/account-manager.h>

#include "presence-window.h"
#include "presence-widget.h"

static void
dump_children (GtkWidget *widget,
               gpointer   callback_data)
{
  g_print (" %s %s (%p)\n",
      (char *) callback_data, G_OBJECT_TYPE_NAME (widget), widget);

  if (GTK_IS_CONTAINER (widget))
    {
      char *str = g_strdup_printf ("%s-", (char *) callback_data);
      gtk_container_foreach (GTK_CONTAINER (widget), dump_children, str);
      g_free (str);
    }
}

static void
account_removed (TpAccountManager *am,
                 TpAccount        *acct,
                 GtkWidget        *widget)
{
  gtk_widget_destroy (widget);
}

static void
account_created (TpAccountManager *am,
                 TpAccount        *acct,
                 PresenceWindow   *window)
{
  GtkWidget *widget = presence_widget_new (acct);
  presence_window_add_widget (window, PRESENCE_WIDGET (widget));
}

static void
account_manager_ready (TpAccountManager *am,
                       PresenceWindow   *window)
{
  GList *l, *accounts = tp_account_manager_get_valid_accounts (am);
  for (l = accounts; l != NULL; l = l->next)
    {
      TpAccount *acct = TP_ACCOUNT (l->data);

      GtkWidget *widget = presence_widget_new (acct);
      presence_window_add_widget (window, PRESENCE_WIDGET (widget));

      g_signal_connect (am, "account-removed",
          G_CALLBACK (account_removed), widget);
    }

  g_list_free (accounts);

  g_signal_connect (am, "account-enabled",
      G_CALLBACK (account_created), window);
}

static void
_am_ready (GObject      *am,
           GAsyncResult *res,
           gpointer      window)
{
  GError *error = NULL;

  if (!tp_account_manager_prepare_finish (TP_ACCOUNT_MANAGER (am), res,
      &error))
    {
      g_error ("ERROR: %s", error->message);
    }

  account_manager_ready (TP_ACCOUNT_MANAGER (am), PRESENCE_WINDOW (window));
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  /* let's use Empathy's status icons */
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  gtk_icon_theme_prepend_search_path (icon_theme,
      "/usr/share/empathy/icons/");

  GtkWidget *window = presence_window_new ();

  TpAccountManager *am = tp_account_manager_dup ();

  /* we want to request some AM features */
  GQuark features[] = { TP_ACCOUNT_MANAGER_FEATURE_CORE }; // FIXME const?
  tp_account_manager_prepare_async (am, features, _am_ready, window);

  gtk_widget_show (window);

  gtk_main ();
}
