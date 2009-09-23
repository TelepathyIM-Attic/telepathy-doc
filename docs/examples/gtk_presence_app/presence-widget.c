/*
 * presence-widget.c
 *
 * PresenceWidget
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <string.h>

#include "presence-widget.h"

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PRESENCE_WIDGET, PresenceWidgetPrivate))

G_DEFINE_TYPE (PresenceWidget, presence_widget, GTK_TYPE_TABLE);

typedef struct _PresenceWidgetPrivate PresenceWidgetPrivate;
struct _PresenceWidgetPrivate
{
  TpAccount *account;

  GtkWidget *enabled_check;
  GtkWidget *status_icon;
  GtkWidget *status_message;

  gint updating_ui_lock;
};

enum /* properties */
{
  PROP_0,
  PROP_ACCOUNT
};

/* presence icons */
static const char *presence_icons[NUM_TP_CONNECTION_PRESENCE_TYPES] = {
    "empathy-offline",
    "empathy-offline",
    "empathy-available",
    "empathy-away",
    "empathy-extended-away",
    "empathy-offline",
    "empathy-busy",
    "empathy-offline",
    "empathy-offline",
};

static const char *default_messages[NUM_TP_CONNECTION_PRESENCE_TYPES] = {
    "Unset",
    "Offline",
    "Available",
    "Away",
    "Extended Away",
    "Hidden",
    "Busy",
    "Unknown",
    "Error"
};

static void
presence_widget_get_property (GObject    *self,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  switch (prop_id)
    {
      case PROP_ACCOUNT:
        g_value_set_object (value, priv->account);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
presence_widget_set_property (GObject      *self,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  switch (prop_id)
    {
      case PROP_ACCOUNT:
        priv->account = g_value_dup_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
_notify_enabled (PresenceWidget *self,
                 GParamSpec     *pspec,
                 TpAccount      *account)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  priv->updating_ui_lock++;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->enabled_check),
      tp_account_is_enabled (account));
  priv->updating_ui_lock--;
}

static void
_notify_display_name (PresenceWidget *self,
                      GParamSpec     *pspec,
                      TpAccount      *account)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  gtk_button_set_label (GTK_BUTTON (priv->enabled_check),
      tp_account_get_display_name (account));
}

static void
_notify_presence (PresenceWidget *self,
                  GParamSpec     *pspec,
                  TpAccount      *account)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);
  TpConnectionPresenceType presence = tp_account_get_presence (account);

  const char *icon_name = presence_icons[presence];

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->status_icon),
      icon_name, GTK_ICON_SIZE_MENU);
}

static void
_notify_status_message (PresenceWidget *self,
                        GParamSpec     *pspec,
                        TpAccount      *account)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);
  const char *msg = tp_account_get_status_message (account);

  if (strlen (msg) == 0)
    {
      TpConnectionPresenceType presence = tp_account_get_presence (account);
      msg = default_messages[presence];
    }

  gtk_label_set_text (GTK_LABEL (priv->status_message), msg);
}

static void
presence_widget_constructed (GObject *self)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  g_signal_connect_swapped (priv->account, "notify::enabled",
      G_CALLBACK (_notify_enabled), self);
  g_signal_connect_swapped (priv->account, "notify::display-name",
      G_CALLBACK (_notify_display_name), self);
  g_signal_connect_swapped (priv->account, "notify::presence",
      G_CALLBACK (_notify_presence), self);
  g_signal_connect_swapped (priv->account, "notify::status-message",
      G_CALLBACK (_notify_status_message), self);

  _notify_enabled (PRESENCE_WIDGET (self), NULL, priv->account);
  _notify_display_name (PRESENCE_WIDGET (self), NULL, priv->account);
  _notify_presence (PRESENCE_WIDGET (self), NULL, priv->account);
  _notify_status_message (PRESENCE_WIDGET (self), NULL, priv->account);
}

static void
presence_widget_class_init (PresenceWidgetClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->constructed  = presence_widget_constructed;
  gobject_class->get_property = presence_widget_get_property;
  gobject_class->set_property = presence_widget_set_property;

  g_object_class_install_property (gobject_class,
      PROP_ACCOUNT,
      g_param_spec_object ("account",
                           "account",
                           "Telepathy Account",
                           TP_TYPE_ACCOUNT,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (gobject_class, sizeof (PresenceWidgetPrivate));
}

static void
_enabled_toggled (PresenceWidget  *self,
                  GtkToggleButton *button)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  if (priv->updating_ui_lock > 0) return;

  tp_account_set_enabled_async (priv->account,
      gtk_toggle_button_get_active (button),
      NULL, NULL);
}

static void
presence_widget_init (PresenceWidget *self)
{
  PresenceWidgetPrivate *priv = GET_PRIVATE (self);

  gtk_table_resize (GTK_TABLE (self), 2, 2);

  priv->enabled_check = gtk_check_button_new ();
  gtk_table_attach (GTK_TABLE (self), priv->enabled_check,
      0, 2, 0, 1,
      GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect_swapped (priv->enabled_check, "toggled",
      G_CALLBACK (_enabled_toggled), self);

  priv->status_icon = gtk_image_new ();
  gtk_table_attach (GTK_TABLE (self), priv->status_icon,
      0, 1, 1, 2,
      GTK_FILL, GTK_FILL, 0, 0);

  priv->status_message = gtk_label_new ("");
  gtk_table_attach (GTK_TABLE (self), priv->status_message,
      1, 2, 1, 2,
      GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show_all (GTK_WIDGET (self));
  gtk_widget_hide (GTK_WIDGET (self));
}

GtkWidget *
presence_widget_new (TpAccount *account)
{
  g_return_val_if_fail (TP_IS_ACCOUNT (account), NULL);

  return g_object_new (TYPE_PRESENCE_WIDGET,
      "account", account,
      NULL);
}
