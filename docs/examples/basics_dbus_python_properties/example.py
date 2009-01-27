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

def on_get_reply(result):
    print "Get() result: %u \n" % result
    loop.quit()

def on_get_error(error):
    print "Get() failed: %s\n" % str(error)
    loop.quit()

if __name__ == '__main__':

    # Tell the dbus python module to use the Glib mainloop, 
    # which we will start and stop later:
    dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

    # Connect to the bus:
    bus = dbus.SessionBus()

    # Get a proxy for the Properties interface of a remote object:
    try:
        proxy = bus.get_object('org.freedesktop.Telepathy.Connection.salut.local_xmpp.Murray_20Cumming',
            '/org/freedesktop/Telepathy/Connection/salut/local_xmpp/Murray_20Cumming',
            'org.freedesktop.DBus.Properties')
    except dbus.DBusException:
        traceback.print_exc()
        sys.exit(1)


    # Call the Properties.Get method on the interface of the remote object: */

    # Call the method:
    proxy.Get(
        "org.freedesktop.Telepathy.Connection", 
        "SelfHandle",
        reply_handler = on_get_reply, error_handler = on_get_error)

    # Start the mainloop so we can wait until the D-Bus method 
    # returns. We stop the mainloop in our handlers.
    loop = gobject.MainLoop()
    loop.run()



