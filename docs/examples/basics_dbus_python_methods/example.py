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
import dbus.mainloop.glib
import gobject
import sys
import traceback

def on_notify_reply(result):
    print "Notify() result: %u \n" % result
    loop.quit()

def on_notify_error(error):
    print "Notify() failed: %s\n" % str(error)
    loop.quit()

if __name__ == '__main__':

    # Tell the dbus python module to use the Glib mainloop, 
    # which we will start and stop later:
    dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

    # Connect to the bus:
    bus = dbus.SessionBus()

    # Get a proxy for the remote object:
    try:
        proxy = bus.get_object('org.freedesktop.Notifications',
            '/org/freedesktop/Notifications',
            'org.freedesktop.Notifications')
    except dbus.DBusException:
        traceback.print_exc()
        sys.exit(1)


    # Call a method on the interface of the remote object: */

    # Create empty objects needed for some parameters:
    actions = dbus.Array('s')
    hints = dbus.Dictionary({}, signature=dbus.Signature('sv'))

    # Call the method:
    proxy.Notify(
        "dbus python example", 
        (dbus.UInt32)(0),
        '', # icon-name
        'Example Notification', 
        'This is an example notification via dbus with Python.', 
        actions, 
        hints, 
        0,
        reply_handler = on_notify_reply, error_handler = on_notify_error)

    # Start the mainloop so we can wait until the D-Bus method 
    # returns. We stop the mainloop in our handlers.
    loop = gobject.MainLoop()
    loop.run()



