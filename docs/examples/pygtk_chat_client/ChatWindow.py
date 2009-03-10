import gtk
import gobject

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

gobject.type_register(ChatWindow)
