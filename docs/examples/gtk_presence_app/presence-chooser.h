/*
 * presence-chooser.h
 *
 * PresenceChooser
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#ifndef __PRESENCE_CHOOSER_H__
#define __PRESENCE_CHOOSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_PRESENCE_CHOOSER	(presence_chooser_get_type ())
#define PRESENCE_CHOOSER(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PRESENCE_CHOOSER, PresenceChooser))
#define PRESENCE_CHOOSER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_PRESENCE_CHOOSER, PresenceChooserClass))
#define IS_PRESENCE_CHOOSER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PRESENCE_CHOOSER))
#define IS_PRESENCE_CHOOSER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_PRESENCE_CHOOSER))
#define PRESENCE_CHOOSER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_PRESENCE_CHOOSER, PresenceChooserClass))

typedef struct _PresenceChooser PresenceChooser;
typedef struct _PresenceChooserClass PresenceChooserClass;

struct _PresenceChooser
{
  GtkComboBoxEntry parent;
};

struct _PresenceChooserClass
{
  GtkComboBoxEntryClass parent_class;
};

GType presence_chooser_get_type (void);
void presence_chooser_set_statuses (PresenceChooser *self,
                                    GHashTable *statuses);
GtkWidget *presence_chooser_new (void);

G_END_DECLS

#endif
