/*
 * presence-chooser.c
 *
 * PresenceChooser
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <telepathy-glib/enums.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>

#include "presence-chooser.h"

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PRESENCE_CHOOSER, PresenceChooserPrivate))

G_DEFINE_TYPE (PresenceChooser, presence_chooser, GTK_TYPE_COMBO_BOX_ENTRY);

/* provided by presence-widget.c */
extern const char *presence_icons[];

typedef struct _PresenceChooserPrivate PresenceChooserPrivate;
struct _PresenceChooserPrivate
{
  TpAccount *account;
  GtkListStore *store;
};

enum /* properties */
{
  PROP_0,
  PROP_ACCOUNT
};

enum /* columns */
{
  ICON_NAME,
  PRESENCE_TYPE,
  STATUS,
  CAN_HAVE_MESSAGE,
  STATUS_MESSAGE,
  N_COLUMNS
};

static void
presence_chooser_get_property (GObject    *self,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PresenceChooserPrivate *priv = GET_PRIVATE (self);

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
presence_chooser_set_property (GObject      *self,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PresenceChooserPrivate *priv = GET_PRIVATE (self);

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
presence_chooser_set_statuses (PresenceChooser *self,
                               GHashTable      *statuses)
{
  g_return_if_fail (IS_PRESENCE_CHOOSER (self));
  g_return_if_fail (statuses != NULL);

  PresenceChooserPrivate *priv = GET_PRIVATE (self);

  gtk_list_store_clear (priv->store);

  GHashTableIter iter;
  gpointer k, v;

  g_hash_table_iter_init (&iter, statuses);
  while (g_hash_table_iter_next (&iter, &k, &v))
    {
      char *status = k;
      GValueArray *array = v;

      guint type;
      gboolean set_on_self, can_have_message;

      tp_value_array_unpack (array, 3,
          &type, &set_on_self, &can_have_message);

      if (!set_on_self) continue;

      gtk_list_store_insert_with_values (priv->store, NULL, -1,
          ICON_NAME, presence_icons[type],
          PRESENCE_TYPE, type,
          STATUS, status,
          CAN_HAVE_MESSAGE, can_have_message,
          STATUS_MESSAGE, status,
          -1);
    }
}

static void
_get_property_statuses (TpProxy      *conn,
                        const GValue *value,
                        const GError *error,
                        gpointer      user_data,
                        GObject      *self)
{
  if (error != NULL)
    {
      g_warning ("%s", error->message);
      return;
    }

  g_return_if_fail (G_VALUE_HOLDS (value, TP_HASH_TYPE_SIMPLE_STATUS_SPEC_MAP));

  GHashTable *statuses = g_value_get_boxed (value);
  presence_chooser_set_statuses (PRESENCE_CHOOSER (self), statuses);
}

static void
_connection_ready (TpConnection *conn,
                   const GError *error,
                   gpointer      self)
{
  if (error != NULL)
    {
      g_warning ("%s", error->message);
      return;
    }

  if (tp_proxy_has_interface (conn,
        TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE))
    {
      /* request the Statuses property */
      tp_cli_dbus_properties_call_get (conn, -1,
          TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
          "Statuses",
          _get_property_statuses,
          NULL, NULL, G_OBJECT (self));
    }
}

static void
_status_changed (PresenceChooser *self,
                 guint            old_status,
                 guint            new_status,
                 guint            reason,
                 TpAccount       *account)
{
  TpConnection *conn = tp_account_get_connection (account);

  if (conn == NULL) return;
  else if (new_status == TP_CONNECTION_STATUS_CONNECTED ||
           new_status == TP_CONNECTION_STATUS_DISCONNECTED)
    {
      tp_connection_call_when_ready (conn, _connection_ready, self);
    }
}

static void
presence_chooser_constructed (GObject *self)
{
  PresenceChooserPrivate *priv = GET_PRIVATE (self);

  g_signal_connect_swapped (priv->account, "status-changed",
      G_CALLBACK (_status_changed), self);

  _status_changed (PRESENCE_CHOOSER (self), 0,
      tp_account_get_connection_status (priv->account, NULL),
      0, priv->account);
}

static void
presence_chooser_dispose (GObject *self)
{
  PresenceChooserPrivate *priv = GET_PRIVATE (self);

  if (priv->account != NULL)
    {
      g_object_unref (priv->account);
      priv->account = NULL;
    }

  G_OBJECT_CLASS (presence_chooser_parent_class)->dispose (self);
}

static void
presence_chooser_class_init (PresenceChooserClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->get_property = presence_chooser_get_property;
  gobject_class->set_property = presence_chooser_set_property;
  gobject_class->constructed  = presence_chooser_constructed;
  gobject_class->dispose      = presence_chooser_dispose;

  g_object_class_install_property (gobject_class,
      PROP_ACCOUNT,
      g_param_spec_object ("account",
                           "account",
                           "Telepathy Account",
                           TP_TYPE_ACCOUNT,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (gobject_class, sizeof (PresenceChooserPrivate));
}

static void
presence_chooser_init (PresenceChooser *self)
{
  PresenceChooserPrivate *priv = GET_PRIVATE (self);

  priv->store = gtk_list_store_new (N_COLUMNS,
      G_TYPE_STRING,  /* ICON_NAME */
      G_TYPE_UINT,    /* PRESENCE_TYPE */
      G_TYPE_STRING,  /* STATUS */
      G_TYPE_BOOLEAN, /* CAN_HAVE_MESSAGE */
      G_TYPE_STRING   /* STATUS_MESSAGE */
    );

  gtk_combo_box_set_model (GTK_COMBO_BOX (self), GTK_TREE_MODEL (priv->store));

  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self));

  GtkCellRenderer *renderer;
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self), renderer,
      "icon-name", ICON_NAME,
      NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self), renderer,
      "text", STATUS_MESSAGE,
      NULL);
}

GtkWidget *
presence_chooser_new (TpAccount *account)
{
  g_return_val_if_fail (TP_IS_ACCOUNT (account), NULL);

  return g_object_new (TYPE_PRESENCE_CHOOSER,
      "account", account,
      NULL);
}
