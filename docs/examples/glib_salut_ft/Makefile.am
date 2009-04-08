INCLUDES = $(TELEPATHY_GLIB_CFLAGS)
LDADD = $(TELEPATHY_GLIB_LIBS)

noinst_PROGRAMS = \
	receiver \
	sender

receiver_SOURCES = \
	receiver.c

sender_SOURCES = \
	sender.c

include $(top_srcdir)/docs/rsync-dist.make
