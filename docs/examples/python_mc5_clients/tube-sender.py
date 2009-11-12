#!/usr/bin/env python

import sys

import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
import telepathy.client
from telepathy.interfaces import CONNECTION_MANAGER, \
                                 CONNECTION, \
                                 CONNECTION_INTERFACE_REQUESTS, \
                                 CHANNEL, \
                                 CHANNEL_INTERFACE_TUBE, \
                                 CHANNEL_TYPE_TEXT, \
                                 CHANNEL_TYPE_DBUS_TUBE
from telepathy.constants import CONNECTION_STATUS_CONNECTED, \
                                CONNECTION_STATUS_DISCONNECTED, \
                                HANDLE_TYPE_ROOM, \
                                SOCKET_ACCESS_CONTROL_LOCALHOST, \
                                TUBE_CHANNEL_STATE_OPEN

DBUS_PROPERTIES = dbus.PROPERTIES_IFACE

class Example (object):
    def __init__ (self, account, password):
        """e.g. account  = 'bob@example.com/test'
                password = 'bigbob'
        """

        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        self._address = None
        self._state = None

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
        print 'Ctrl-C to disconnect'

        # get a list of interfaces on this connection
        conn[CONNECTION].GetInterfaces(reply_handler = self.get_interfaces_cb,
                                       error_handler = self.error_cb)

    def get_interfaces_cb (self, interfaces):
        conn = self.conn
        conn[CONNECTION_INTERFACE_REQUESTS].EnsureChannel({
                CHANNEL + '.ChannelType': CHANNEL_TYPE_TEXT,
                CHANNEL + '.TargetHandleType': HANDLE_TYPE_ROOM,
                CHANNEL + '.TargetID': 'test@conference.collabora.co.uk',
            }, reply_handler = self.ensure_text_channel_cb,
               error_handler = self.error_cb)

    def ensure_text_channel_cb (self, yours, object_path, props):
        print "got text channel"

        # try to open a tube
        conn = self.conn
        conn[CONNECTION_INTERFACE_REQUESTS].CreateChannel({
                CHANNEL + '.ChannelType': CHANNEL_TYPE_DBUS_TUBE,
                CHANNEL + '.TargetHandleType': HANDLE_TYPE_ROOM,
                CHANNEL + '.TargetID': 'test@conference.collabora.co.uk',
                CHANNEL_TYPE_DBUS_TUBE + '.ServiceName': 'org.freedesktop.Telepathy.Examples.TubeClient',
            }, reply_handler = self.ensure_tube_cb,
               error_handler = self.error_cb)

    def ensure_tube_cb (self, object_path, props):
        print "got tube channel"

        channel = telepathy.client.Channel(self.conn.service_name, object_path)
        channel[CHANNEL_INTERFACE_TUBE].connect_to_signal(
                'TubeChannelStateChanged', self.tube_channel_state_changed)
        channel[CHANNEL_TYPE_DBUS_TUBE].connect_to_signal(
                'DBusNamesChanged', self.dbus_names_changed)
        channel[CHANNEL_TYPE_DBUS_TUBE].Offer({},
                SOCKET_ACCESS_CONTROL_LOCALHOST,
                reply_handler = self.offer_tube_cb,
                error_handler = self.error_cb)

    def tube_channel_state_changed (self, state):
        print "Tube state changed:", state
        self._tube_state = state

        self.open_tube()

    def dbus_names_changed (self, added, removed):
        print 'Dbus names changed', added, removed

    def offer_tube_cb (self, address):
        print "Address:", address
        self._address = address

        self.open_tube()

    def open_tube(self):
        if not (self._address and self._tube_state == TUBE_CHANNEL_STATE_OPEN):
            return

        print "OPENING TUBE"

if __name__ == '__main__':
    import getpass
    password = getpass.getpass()
    Example(sys.argv[1], password)
