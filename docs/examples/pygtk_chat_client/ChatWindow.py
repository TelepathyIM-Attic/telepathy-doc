import gtk
import gobject

from telepathy.interfaces import CHANNEL, \
                                 CHANNEL_TYPE_TEXT, \
                                 CHANNEL_INTERFACE_MESSAGES

from example import generic_reply, Contact

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

class ChatWindow(gtk.Window):
    def __init__(self, channel):
        super(ChatWindow, self).__init__()

        self.sm = channel.sm
        self.channel = channel

        self.set_default_size(300, 300)
        self.set_title('Chat with %s' % channel.target_id)

        vbox = gtk.VBox()
        self.add(vbox)

        sw = gtk.ScrolledWindow()
        vbox.pack_start (sw)

        self.buffer = gtk.TextBuffer()
        tv = gtk.TextView(self.buffer)
        sw.add(tv)

        vbox.show_all()

        self.connect('destroy', lambda s: s.channel.close())

        if CHANNEL_INTERFACE_MESSAGES in channel.interfaces:
            channel[CHANNEL_INTERFACE_MESSAGES].connect_to_signal(
                    'MessageReceived', self._message_received)
            channel[DBUS_PROPERTIES].Get(CHANNEL_INTERFACE_MESSAGES,
                    'PendingMessages',
                    reply_handler = self._get_pending_messages,
                    error_handler = self.sm.error)
        else:
            print 'Iface.Messages not supported, falling back'
            channel[CHANNEL_TYPE_TEXT].connect_to_signal(
                    'Received', self._simple_message_received)
            channel[CHANNEL_TYPE_TEXT].ListPendingMessages(False,
                    reply_handler = self._simple_get_pending_messages,
                    error_handler = self.sm.error)

    def _get_pending_messages(self, messages):
        for message in messages:
            self._message_received(message)

    def _message_received(self, message):
        print 'Received message'

        # we need to acknowledge the message
        msg_id = message[0]['pending-message-id']
        self.channel[CHANNEL_TYPE_TEXT].AcknowledgePendingMessages([msg_id],
            reply_handler = generic_reply,
            error_handler = self.sm.error)

    def _simple_get_pending_messages(self, messages):
        for message in messages:
            self._simple_message_received(*message)

    def _simple_message_received(self, msg_id, timestamp, sender,
                                 msg_type, flags, content):

        def _simple_sender_known():
            contact = self.sm.contacts[sender]
            self._update_buffer(contact.alias, content)

            # we need to acknowledge the message
            self.channel[CHANNEL_TYPE_TEXT].AcknowledgePendingMessages([msg_id],
                reply_handler = generic_reply,
                error_handler = self.sm.error)

        if sender not in self.sm.contacts:
            # we need to look the sender up
            Contact.lookup_from_handles(self.sm, [sender],
                                        callback = _simple_sender_known)
        else:
            _simple_sender_known()

    def _update_buffer (self, sender, msg):
        iter = self.buffer.get_end_iter()
        self.buffer.insert(iter, '%s: %s\n' % (sender, msg))

gobject.type_register(ChatWindow)
