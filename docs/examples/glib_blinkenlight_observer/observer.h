#ifndef __OBSERVER_H__
#define __OBSERVER_H__

#include <glib-object.h>
#include <telepathy-glib/dbus-properties-mixin.h>

G_BEGIN_DECLS

#define TYPE_OBSERVER	(observer_get_type ())
#define OBSERVER(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_OBSERVER, Observer))
#define OBSERVER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_OBSERVER, ObserverClass))
#define IS_OBSERVER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_OBSERVER))
#define IS_OBSERVER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_OBSERVER))
#define OBSERVER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_OBSERVER, ObserverClass))

typedef struct _Observer Observer;
struct _Observer
{
	GObject parent;
};

typedef struct _ObserverClass ObserverClass;
struct _ObserverClass
{
	GObjectClass parent_class;
	TpDBusPropertiesMixinClass dbus_props_class;
};

GType observer_get_type (void);
Observer *observer_new (void);

G_END_DECLS

#endif
