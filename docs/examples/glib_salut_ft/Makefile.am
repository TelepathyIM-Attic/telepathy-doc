INCLUDES = $(TELEPATHY_GLIB_CFLAGS)
LDADD = $(TELEPATHY_GLIB_LIBS)

noinst_PROGRAMS = \
	receiver \
	sender \
	gnio-receiver \
	gnio-sender

receiver_SOURCES = \
	receiver.c

sender_SOURCES = \
	sender.c

gnio_receiver_CFLAGS = \
	$(GNIO_CFLAGS) \
	$(TELEPATHY_GLIB_CFLAGS)

gnio_receiver_LDADD = \
	$(GNIO_LIBS) \
	$(TELEPATHY_GLIB_LIBS)

gnio_receiver_SOURCES = \
	gnio-receiver.c

gnio_sender_CFLAGS = $(gnio_receiver_CFLAGS)
gnio_sender_LDADD = $(gnio_receiver_LDADD)

gnio_sender_SOURCES = \
	gnio-sender.c

include $(top_srcdir)/docs/rsync-dist.make
