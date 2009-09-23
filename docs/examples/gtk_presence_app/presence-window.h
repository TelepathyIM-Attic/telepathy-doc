/*
 * presence-window.h
 *
 * PresenceWindow
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#ifndef __PRESENCE_WINDOW_H__
#define __PRESENCE_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_PRESENCE_WINDOW	(presence_window_get_type ())
#define PRESENCE_WINDOW(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PRESENCE_WINDOW, PresenceWindow))
#define PRESENCE_WINDOW_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_PRESENCE_WINDOW, PresenceWindowClass))
#define IS_PRESENCE_WINDOW(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PRESENCE_WINDOW))
#define IS_PRESENCE_WINDOW_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_PRESENCE_WINDOW))
#define PRESENCE_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PRESENCE_WINDOW, PresenceWindowClass))

typedef struct _PresenceWindow PresenceWindow;
typedef struct _PresenceWindowClass PresenceWindowClass;

struct _PresenceWindow
{
  GtkWindow parent;
};

struct _PresenceWindowClass
{
  GtkWindowClass parent_class;
};

GType presence_window_get_type (void);
GtkWidget *presence_window_new (void);

G_END_DECLS

#endif
