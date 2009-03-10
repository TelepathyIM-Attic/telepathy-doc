#!/usr/bin/env python

import os
import sys

import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
from telepathy.interfaces import CONNECTION_MANAGER, \
                                 CONNECTION, \
                                 CONNECTION_INTERFACE_REQUESTS, \
                                 CHANNEL, \
                                 CHANNEL_TYPE_ROOM_LIST
from telepathy.constants import CONN_MGR_PARAM_FLAG_REQUIRED, \
                                CONNECTION_STATUS_CONNECTED, \
                                CONNECTION_STATUS_DISCONNECTED, \
                                HANDLE_TYPE_NONE

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

mainloop = gobject.MainLoop()

def generic_handler(*args): pass

class Example(object):
    def __init__(self):
        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        self.cm = reg.GetManager('idle') # IRC

        self.cm[CONNECTION_MANAGER].GetParameters('irc',
            reply_handler = self.print_params,
            error_handler = self.error)

    def print_params(self, params):
        for n, f, s, d in params:
            print '%s (%s) - %s' % (n, d, f & CONN_MGR_PARAM_FLAG_REQUIRED)

    def tp_connect(self, server):
        account = os.getlogin()
        self.cm[CONNECTION_MANAGER].RequestConnection('irc',
            {
                'account'   : account,
                'server'    : server
            },
            reply_handler = self.got_connection,
            error_handler = self.error)

    def got_connection(self, bus_name, object_path):
        self.conn = telepathy.client.Connection(bus_name, object_path)

        self.conn[CONNECTION].connect_to_signal('StatusChanged',
            self.status_changed)
        print 'Establishing connection...'
        self.conn[CONNECTION].Connect(reply_handler = generic_handler,
                                      error_handler = self.error)

    def status_changed(self, status, reason):
        if status == CONNECTION_STATUS_DISCONNECTED:
            print 'Disconnected'
            mainloop.quit()

        if status != CONNECTION_STATUS_CONNECTED: return

        print 'Connected'

        self.conn[CONNECTION].GetInterfaces(reply_handler = self.get_interfaces,
                                            error_handler = self.error)

    def get_interfaces(self, interfaces):
        print ' -- Connection Interfaces --'
        for interface in interfaces:
            print ' - %s' % interface

        if CONNECTION_INTERFACE_REQUESTS in interfaces:
            print "telepathy-idle didn't support this before"
        else:
            print 'Falling back to RequestChannel'

            self.conn[CONNECTION].RequestChannel(CHANNEL_TYPE_ROOM_LIST,
                                                 HANDLE_TYPE_NONE, 0,
                                                 True,
                                                 reply_handler = self.got_roomlist,
                                                 error_handler = self.error)

    def got_roomlist(self, channel_path):
        print 'Got Roomlist Channel'
        self.roomlist = channel = telepathy.client.Channel(
            self.conn.service_name, channel_path)
        channel[CHANNEL_TYPE_ROOM_LIST].connect_to_signal('GotRooms',
            self.got_rooms)
        channel[CHANNEL_TYPE_ROOM_LIST].connect_to_signal('ListingRooms',
            self.listing_rooms)
        channel[CHANNEL_TYPE_ROOM_LIST].ListRooms(reply_handler = generic_handler,
                                                  error_handler = self.error)

    def got_rooms(self, rooms):
        for handle, channel_type, info in rooms:
            print " - %(name)s (%(members)i) :: %(subject)s" % info

    def listing_rooms(self, listing):
        print "listing rooms", listing

        if listing == False:
            # close the room list channel
            self.roomlist.Close(reply_handler = generic_handler,
                                error_handler = self.error)

    def error(self, error):
        print 'Error:', error

    def disconnect(self):
        self.conn.Disconnect(reply_handler = generic_handler,
                             error_handler = self.error)

if __name__ == '__main__':
    ex = Example()
    ex.tp_connect(sys.argv[1])
    try:
        mainloop.run()
    except KeyboardInterrupt:
        ex.disconnect()
        mainloop.run()
