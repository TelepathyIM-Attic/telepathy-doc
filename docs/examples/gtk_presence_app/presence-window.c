/*
 * presence-window.c
 *
 * PresenceWindow
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include "presence-window.h"
#include "presence-widget.h"

#define GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PRESENCE_WINDOW, PresenceWindowPrivate))

G_DEFINE_TYPE (PresenceWindow, presence_window, GTK_TYPE_WINDOW);

typedef struct _PresenceWindowPrivate PresenceWindowPrivate;
struct _PresenceWindowPrivate
{
  GtkWidget *vbox;
};

static void
presence_window_class_init (PresenceWindowClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  g_type_class_add_private (gobject_class, sizeof (PresenceWindowPrivate));
}

static void
presence_window_init (PresenceWindow *self)
{
  PresenceWindowPrivate *priv = GET_PRIVATE (self);

  gtk_window_set_title (GTK_WINDOW (self), "Telepathy Account Presence");

  priv->vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (self), priv->vbox);

  gtk_widget_show (priv->vbox);
}

void
presence_window_add_widget (PresenceWindow *self,
                            PresenceWidget *widget)
{
  g_return_if_fail (IS_PRESENCE_WINDOW (self));
  g_return_if_fail (IS_PRESENCE_WIDGET (widget));

  PresenceWindowPrivate *priv = GET_PRIVATE (self);

  gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET (widget), FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (widget));

  // FIXME: watch for account removal
}

GtkWidget *
presence_window_new (void)
{
  return g_object_new (TYPE_PRESENCE_WINDOW, NULL);
}
