#!/usr/bin/env python

import sys

import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
import telepathy.client
from telepathy.interfaces import CONNECTION_MANAGER, \
                                 CONNECTION, \
                                 CONNECTION_INTERFACE_SIMPLE_PRESENCE, \
                                 CONNECTION_INTERFACE_CONTACTS
from telepathy.constants import CONNECTION_STATUS_CONNECTED, \
                                CONNECTION_STATUS_DISCONNECTED

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

class Example (object):
    def __init__ (self, account, password):
        """e.g. account  = 'bob@example.com/test'
                password = 'bigbob'
        """

        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        # get the gabble Connection Manager
        self.cm = cm = reg.GetManager('gabble')

        # get the parameters required to make a Jabber connection
        cm[CONNECTION_MANAGER].RequestConnection('jabber',
            {
                'account':  account,
                'password': password,
            },
            reply_handler = self.request_connection_cb,
            error_handler = self.error_cb)

        self.loop = gobject.MainLoop()
        try:
            self.loop.run()
        except KeyboardInterrupt:
            print "Terminating connection..."
            self.disconnect()
            # reengage the mainloop so that we can disconnect cleanly
            self.loop.run()

    def generic_reply (self, *args): pass

    def error_cb (self, error):
        print "Error:", error
        self.disconnect()

    def disconnect (self):
        self.conn[CONNECTION].Disconnect(reply_handler = self.generic_reply,
                                         error_handler = self.error_cb)

    def request_connection_cb (self, bus_name, object_path):
        print bus_name, object_path
        self.conn = conn = telepathy.client.Connection(bus_name, object_path)

        conn[CONNECTION].connect_to_signal('StatusChanged',
            self.status_changed_cb)

        print "Establishing connection..."
        conn[CONNECTION].Connect(reply_handler = self.generic_reply,
                                 error_handler = self.error_cb)

    def status_changed_cb (self, status, reason):
        conn = self.conn

        if status == CONNECTION_STATUS_DISCONNECTED:
            print "Disconnected!"
            self.loop.quit()

        if status != CONNECTION_STATUS_CONNECTED: return

        print 'Carrier Detected' # remember dialup modems?

        # get a list of interfaces on this connection
        conn[CONNECTION].GetInterfaces(reply_handler = self.get_interfaces_cb,
                                       error_handler = self.error_cb)

    def get_interfaces_cb (self, interfaces):
        conn = self.conn

        print "Interfaces:"
        for interface in interfaces:
            print " - %s" % interface

        if CONNECTION_INTERFACE_SIMPLE_PRESENCE in interfaces:
            # request the statuses
            print 'Requesting statuses...'
            conn[DBUS_PROPERTIES].Get(CONNECTION_INTERFACE_SIMPLE_PRESENCE,
                                    'Statuses',
                                    reply_handler = self.get_statuses_cb,
                                    error_handler = self.error_cb)

            # set our presence
            # FIXME: doesn't like this interface
            print 'Setting presence...'
            conn[CONNECTION_INTERFACE_SIMPLE_PRESENCE].SetPresence(
                                    'away',
                                    'At the Movies',
                                    reply_handler = self.generic_reply,
                                    error_handler = self.error_cb)

        if CONNECTION_INTERFACE_CONTACTS in interfaces:
            print 'Requesting contact attribute interfaces...'
            conn[DBUS_PROPERTIES].Get(CONNECTION_INTERFACE_CONTACTS,
                                    'ContactAttributeInterfaces',
                                    reply_handler = self.get_contact_ifaces_cb,
                                    error_handler = self.error_cb)

    def get_statuses_cb (self, value):
        print "Statuses:"

        for (key, value) in value.iteritems():
            print " - %s" % key

    def get_contact_ifaces_cb (self, interfaces):
        print "Contact Attribute Interfaces:"
        for interface in interfaces:
            print " - %s" % interface

if __name__ == '__main__':
    import getpass
    password = getpass.getpass()
    Example(sys.argv[1], password)
