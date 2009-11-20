#ifndef __EXAMPLE_HANDLER_H__
#define __EXAMPLE_HANDLER_H__

#include <glib-object.h>
#include <telepathy-glib/dbus-properties-mixin.h>

G_BEGIN_DECLS

#define TYPE_EXAMPLE_HANDLER	(example_handler_get_type ())
#define EXAMPLE_HANDLER(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EXAMPLE_HANDLER, ExampleHandler))
#define EXAMPLE_HANDLER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_EXAMPLE_HANDLER, ExampleHandlerClass))
#define IS_EXAMPLE_HANDLER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EXAMPLE_HANDLER))
#define IS_EXAMPLE_HANDLER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_EXAMPLE_HANDLER))
#define EXAMPLE_HANDLER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EXAMPLE_HANDLER, ExampleHandlerClass))

typedef struct _ExampleHandler ExampleHandler;

struct _ExampleHandler
{
	GObject parent;
};

typedef struct _ExampleHandlerClass ExampleHandlerClass;
struct _ExampleHandlerClass
{
	GObjectClass parent_class;
	TpDBusPropertiesMixinClass dbus_props_class;
};

GType example_handler_get_type (void);
ExampleHandler *example_handler_new (void);

G_END_DECLS

#endif
