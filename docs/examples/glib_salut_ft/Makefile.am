INCLUDES = $(TELEPATHY_GLIB_CFLAGS)
LDADD = $(TELEPATHY_GLIB_LIBS)

noinst_PROGRAMS = \
	receiver \
	sender

receiver_SOURCES = \
	receiver.c

sender_SOURCES = \
	sender.c

if HAVE_GNIO
noinst_PROGRAMS += \
	gnio-receiver

gnio_receiver_CFLAGS = \
	$(GNIO_CFLAGS) \
	$(TELEPATHY_GLIB_CFLAGS)

gnio_receiver_LDADD = \
	$(GNIO_LIBS) \
	$(TELEPATHY_GLIB_LIBS)

gnio_receiver_SOURCES = \
	gnio-receiver.c
endif

include $(top_srcdir)/docs/rsync-dist.make
