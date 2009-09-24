/*
 * presence-chooser.c
 *
 * PresenceChooser
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include "presence-chooser.h"

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PRESENCE_CHOOSER, PresenceChooserPrivate))

G_DEFINE_TYPE (PresenceChooser, presence_chooser, GTK_TYPE_COMBO_BOX_ENTRY);

/* provided by presence-widget.c */
extern const char *presence_icons[];

typedef struct _PresenceChooserPrivate PresenceChooserPrivate;
struct _PresenceChooserPrivate
{
  GtkListStore *store;
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
presence_chooser_class_init (PresenceChooserClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

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

void
presence_chooser_set_statuses (PresenceChooser *self,
                               GHashTable *statuses)
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

      guint type = g_value_get_uint (g_value_array_get_nth (array, 0));
      gboolean set_on_self = g_value_get_boolean (
          g_value_array_get_nth (array, 1));
      gboolean can_have_message = g_value_get_boolean (
          g_value_array_get_nth (array, 2));

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

GtkWidget *
presence_chooser_new (void)
{
  return g_object_new (TYPE_PRESENCE_CHOOSER, NULL);
}
