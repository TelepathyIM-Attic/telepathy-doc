#!/usr/bin/env python

import sys

import gtk
import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
import telepathy.client
from telepathy.interfaces import CONNECTION_MANAGER, \
                                 CONNECTION, \
                                 CONNECTION_INTERFACE_REQUESTS, \
                                 CONNECTION_INTERFACE_CONTACTS, \
                                 CONNECTION_INTERFACE_ALIASING, \
                                 CONNECTION_INTERFACE_SIMPLE_PRESENCE, \
                                 CHANNEL, \
                                 CHANNEL_TYPE_CONTACT_LIST, \
                                 CHANNEL_TYPE_TEXT, \
                                 CHANNEL_INTERFACE_GROUP
from telepathy.constants import CONNECTION_STATUS_CONNECTED, \
                                CONNECTION_STATUS_DISCONNECTED, \
                                CONNECTION_PRESENCE_TYPE_AVAILABLE, \
                                CONNECTION_PRESENCE_TYPE_BUSY, \
                                CONNECTION_PRESENCE_TYPE_AWAY, \
                                CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY, \
                                CONNECTION_PRESENCE_TYPE_UNSET, \
                                CONNECTION_PRESENCE_TYPE_UNKNOWN, \
                                CONNECTION_PRESENCE_TYPE_OFFLINE, \
                                HANDLE_TYPE_CONTACT, \
                                HANDLE_TYPE_LIST, \
                                HANDLE_TYPE_GROUP

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

def generic_reply(*args): pass

class Connection(telepathy.client.Connection):
    def __init__(self, sm, bus_name, object_path):
        super(Connection, self).__init__(bus_name, object_path)

        self.sm = sm

        self[CONNECTION].connect_to_signal('StatusChanged', self.status_changed)
        self[CONNECTION].Connect(reply_handler = generic_reply,
                                 error_handler = self.sm.error)

    def status_changed(self, status, reason):
        if status == CONNECTION_STATUS_DISCONNECTED:
            print 'Disconnected!'
            # FIXME: signal disconnection with the SM
            gtk.main_quit()
            return

        elif status != CONNECTION_STATUS_CONNECTED:
            return

        print 'Carrier Detected'
        # NB. this should be a property eventually
        self[CONNECTION].GetInterfaces(reply_handler = self._interfaces_cb,
                                       error_handler = self.sm.error)

    def _interfaces_cb(self, interfaces):
        self.interfaces = interfaces

        self.sm.connection_ready(self)

    def disconnect(self):
        self[CONNECTION].Disconnect(reply_handler = generic_reply,
                                    error_handler = self.sm.error)

    def ensure_channel(self, channel_obj, handle_type, target_id, props = {}):

        d = {
            CHANNEL + '.ChannelType'      : channel_obj.channel_type,
            CHANNEL + '.TargetHandleType' : handle_type,
            CHANNEL + '.TargetID'         : target_id,
        }

        d.update(props)

        def _ensure_channel_error(self, error):
            print 'Channel could not be created for dict: %s' % d

        if CONNECTION_INTERFACE_REQUESTS in self.interfaces:
            self[CONNECTION_INTERFACE_REQUESTS].EnsureChannel(d,
                reply_handler = lambda a, b, c: channel_obj(self, b, c, a),
                error_handler = self._ensure_channel_error)
        else:
            self.sm.error("Requests interface unavailable, get a better CM")

class Channel(telepathy.client.Channel):
    def __init__(self, conn, object_path, props, yours = False):
        self.conn = conn
        self.sm = conn.sm
        self.yours = yours

        self.target_id = props[CHANNEL + '.TargetID']
        self.handle = props[CHANNEL + '.TargetHandle']
        self.handle_type = props[CHANNEL + '.TargetHandleType']

        print 'Channel came up... requesting interfaces'
        super(Channel, self).__init__(conn.service_name, object_path)

        self[DBUS_PROPERTIES].Get (CHANNEL, 'Interfaces',
                                   reply_handler = self._interfaces_cb,
                                   error_handler = self.sm.error)
    
    def close(self):
        self[CHANNEL].Close(reply_handler = generic_reply,
                            error_handler = self.sm.error)

    def get_target_id(self):
        return self.target_id

    def _interfaces_cb(self, interfaces):
        self.interfaces = interfaces

        self.ready()

    def ready(self):
        pass

class ContactList(Channel):
    channel_type = CHANNEL_TYPE_CONTACT_LIST

    def ready(self):
        # get the list of handles
        if CHANNEL_INTERFACE_GROUP in self.interfaces:
            self[DBUS_PROPERTIES].Get(CHANNEL_INTERFACE_GROUP, 'Members',
                reply_handler = self._members_cb,
                error_handler = self.sm.error)
        else:
            print 'Channel does not implement Group... strange'

    def _members_cb(self, handles):
        Contact.lookup_from_handles(self.sm, handles)

class TextChannel(Channel):
    channel_type = CHANNEL_TYPE_TEXT

    def ready(self):
        self.sm.new_text_channel(self)

class Contact(object):
    def __init__(self, sm, handle, attributes={}):
        self.sm = sm
        self.handle = handle

        for key, value in attributes.iteritems():
            if key == CONNECTION + '/contact-id':
                self.contact_id = value
            elif key == CONNECTION_INTERFACE_ALIASING + '/alias':
                self.alias = value
            elif key == CONNECTION_INTERFACE_SIMPLE_PRESENCE + '/presence':
                self.presence = value

    @classmethod
    def lookup_from_handles(self, sm, handles, callback = None):
        # these are the interfaces we're seeking
        interfaces = [
                 CONNECTION_INTERFACE_ALIASING,
                 CONNECTION_INTERFACE_SIMPLE_PRESENCE,
            ]

        # work out what interfaces are available
        interfaces = list (set(interfaces) & set(sm.conn.interfaces))
        interfaces += [ CONNECTION ]

        def _new_handle_attributes_cb(map):
            for handle, attributes in map.iteritems():
                contact = Contact (sm, handle, attributes)
                sm.contacts[handle] = contact

            sm.contacts_updated(map.keys())

            if callback is not None: callback()

        # look them up via the contacts interface
        if CONNECTION_INTERFACE_CONTACTS in sm.conn.interfaces:
            sm.conn[CONNECTION_INTERFACE_CONTACTS].GetContactAttributes(
                handles, interfaces, False,
                reply_handler = _new_handle_attributes_cb,
                error_handler = sm.error)

    def get_state(self):
        return self.presence[0]

    def get_status(self):
        if self.presence[2] != '':
            return self.presence[2]
        else:
            return self.presence[1]

    def __repr__(self):
        return 'Contact(%s)' % self.contact_id

    def __cmp__(self, other):
        ordering_map = {
            CONNECTION_PRESENCE_TYPE_AVAILABLE      : 0,
            CONNECTION_PRESENCE_TYPE_BUSY           : 1,
            CONNECTION_PRESENCE_TYPE_AWAY           : 2,
            CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY  : 3,
            CONNECTION_PRESENCE_TYPE_UNSET          : 4,
            CONNECTION_PRESENCE_TYPE_UNKNOWN        : 4,
            CONNECTION_PRESENCE_TYPE_OFFLINE        : 5,
        }

        c = cmp (ordering_map[self.get_state()], ordering_map[other.get_state()])
        if c != 0: return c
        c = cmp (self.alias, other.alias)
        return c

class StateMachine(gobject.GObject):
    __gsignals__ = {
        'contacts-updated'  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                               (gobject.TYPE_PYOBJECT,)),
        'new-chat'          : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                               (gobject.TYPE_PYOBJECT,)),
    }

    def __init__(self):
        super(StateMachine, self).__init__()

        self.contacts = {}

    def tp_connect(self, account, password):
        """e.g. account  = 'bob@example.com/test'
                password = 'bigbob'
        """

        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        # get the gabble Connection Manager
        self.cm = cm = reg.GetManager('gabble')

        print 'Connecting...'
        cm[CONNECTION_MANAGER].RequestConnection('jabber',
            {
                'account':  account,
                'password': password,
            },
            reply_handler = lambda a, b: Connection (self, a, b),
            error_handler = self.error)

    def error(self, error):
        print "Telepathy Error: %s" % error
        print "Disconnecting..."
        self.tp_disconnect()

    def connection_ready(self, conn):
        print "Connection Ready"
        self.conn = conn

        # set up signals for interfaces we're interested in
        if CONNECTION_INTERFACE_ALIASING in conn.interfaces:
            conn[CONNECTION_INTERFACE_ALIASING].connect_to_signal(
                'AliasesChanged', self._aliases_changed)

        if CONNECTION_INTERFACE_SIMPLE_PRESENCE in conn.interfaces:
            conn[CONNECTION_INTERFACE_SIMPLE_PRESENCE].connect_to_signal(
                'PresencesChanged', self._presences_changed)

        if CONNECTION_INTERFACE_REQUESTS in conn.interfaces:
            conn[CONNECTION_INTERFACE_REQUESTS].connect_to_signal(
                'NewChannels', self._new_channels)
        else:
            self.sm.error("Requests interface unavailable, get a better CM")

        # request the contact lists
        print 'Requesting roster...'
        self.conn.ensure_channel (ContactList, HANDLE_TYPE_LIST, 'subscribe')
        self.conn.ensure_channel (ContactList, HANDLE_TYPE_LIST, 'publish')

    def new_text_channel(self, channel):
        self.emit('new-chat', channel)

    def _aliases_changed(self, aliases):
        for handle, alias in aliases:
            if handle not in self.contacts: continue
            self.contacts[handle].alias = alias

        self.contacts_updated(map(lambda (h, a): h, aliases))

    def _presences_changed(self, presences):
        for handle, presence in presences.iteritems():
            if handle not in self.contacts: continue
            self.contacts[handle].presence = presence

        self.contacts_updated(presences.keys())

    def _new_channels(self, channels):
        for channel_path, props in channels:
            channel_type = props[CHANNEL + '.ChannelType']

            if channel_type == TextChannel.channel_type:
                TextChannel(self.conn, channel_path, props)

    def contacts_updated(self, handles):
        contacts = [ self.contacts[h] for h in handles if h in self.contacts]

        self.emit('contacts-updated', contacts)

    def tp_disconnect(self):
        print 'Disconnecting...'
        try:
            self.conn.disconnect()
        except:
            gtk.main_quit()

gobject.type_register(StateMachine)

if __name__ == '__main__':
    import getpass

    from RosterWindow import RosterWindow

    account = sys.argv[1]
    password = getpass.getpass()
    
    sm = StateMachine()
    roster = RosterWindow(sm)
    roster.show()

    sm.tp_connect(account, password)
    roster.connect ('destroy', lambda w: sm.tp_disconnect())
        
    try:
        print 'Running...'
        gtk.main()
    except KeyboardInterrupt:
        print "Terminating connection..."
        sm.tp_disconnect()
        # reengage the mainloop so that we can disconnect cleanly
        gtk.main()
