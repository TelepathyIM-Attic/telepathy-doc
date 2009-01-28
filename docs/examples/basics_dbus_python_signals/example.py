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

def on_signal_device_added(path):
    print "DeviceAdded signal handled: %s \n" % path
    loop.quit()

if __name__ == '__main__':

    # Tell the dbus python module to use the Glib mainloop, 
    # which we will start and stop later:
    dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

    # Connect to the bus:
    bus = dbus.SystemBus()

    # Get a proxy for the remote object:
    try:
        proxy = bus.get_object('org.freedesktop.Hal',
            '/org/freedesktop/Hal/Manager',
            'org.freedesktop.Hal.Manager')
    except dbus.DBusException:
        traceback.print_exc()
        sys.exit(1)


    # Connect to a signal on the interface: */
    proxy.connect_to_signal(
        "DeviceAdded", 
         on_signal_device_added)

    # Start the mainloop so we can wait until the D-Bus method 
    # returns. We stop the mainloop in our handlers.
    loop = gobject.MainLoop()
    loop.run()



