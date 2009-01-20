# Copyright 2009 Collabora Ltd
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

import dbus

# Connect to the bus:
bus = dbus.SessionBus()

# Get a proxy for the remote object:
proxy = bus.get_object('org.freedesktop.Notifications',
                       '/org/freedesktop/Notifications',
                       'org.freedesktop.Notifications')


# Call a method on the interface  of the remote object: */
actions = dbus.Array('s')
hints = dbus.Dictionary({}, signature=dbus.Signature('sv'))
notification_id = proxy.Notify("dbus python example", 
  (dbus.UInt32)(0),
  '', # icon-name
  'Example Notification', 
  'This is an example notification via dbus with Python.', 
  actions, 
  hints, 
  0)




