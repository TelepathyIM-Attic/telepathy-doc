/*
 * presence-widget.h
 *
 * PresenceWidget
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#ifndef __PRESENCE_WIDGET_H__
#define __PRESENCE_WIDGET_H__

#include <gtk/gtk.h>

#include <telepathy-glib/account.h>

G_BEGIN_DECLS

#define TYPE_PRESENCE_WIDGET	(presence_widget_get_type ())
#define PRESENCE_WIDGET(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PRESENCE_WIDGET, PresenceWidget))
#define PRESENCE_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_PRESENCE_WIDGET, PresenceWidgetClass))
#define IS_PRESENCE_WIDGET(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PRESENCE_WIDGET))
#define IS_PRESENCE_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_PRESENCE_WIDGET))
#define PRESENCE_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PRESENCE_WIDGET, PresenceWidgetClass))

typedef struct _PresenceWidget PresenceWidget;
typedef struct _PresenceWidgetClass PresenceWidgetClass;

struct _PresenceWidget
{
  GtkTable parent;
};

struct _PresenceWidgetClass
{
  GtkTableClass parent_class;
};

GType presence_widget_get_type (void);

GtkWidget *presence_widget_new (TpAccount *account);

G_END_DECLS

#endif
