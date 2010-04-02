/*
 * observer.h - a Telepathy Observer, observes text channels to count their
 *              unread messages
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *   Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

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
