#ifndef __EXAMPLE_OBSERVER_H__
#define __EXAMPLE_OBSERVER_H__

#include <glib-object.h>
#include <telepathy-glib/dbus-properties-mixin.h>

G_BEGIN_DECLS

#define TYPE_EXAMPLE_OBSERVER	(example_observer_get_type ())
#define EXAMPLE_OBSERVER(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EXAMPLE_OBSERVER, ExampleObserver))
#define EXAMPLE_OBSERVER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_EXAMPLE_OBSERVER, ExampleObserverClass))
#define IS_EXAMPLE_OBSERVER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EXAMPLE_OBSERVER))
#define IS_EXAMPLE_OBSERVER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_EXAMPLE_OBSERVER))
#define EXAMPLE_OBSERVER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EXAMPLE_OBSERVER, ExampleObserverClass))

typedef struct _ExampleObserver ExampleObserver;
typedef struct _ExampleObserverClass ExampleObserverClass;

struct _ExampleObserver
{
	GObject parent;
};

struct _ExampleObserverClass
{
	GObjectClass parent_class;
	TpDBusPropertiesMixinClass dbus_props_class;
};

GType example_observer_get_type (void);
ExampleObserver *example_observer_new (void);

G_END_DECLS

#endif
