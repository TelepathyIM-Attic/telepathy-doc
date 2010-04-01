INCLUDES = $(TELEPATHY_GLIB_CFLAGS)
LDADD = $(TELEPATHY_GLIB_LIBS)

noinst_PROGRAMS = blinkenlight-observer

blinkenlight_observer_SOURCES = \
	observer.c observer.h \
	channel.c channel.h \
	main.c

include $(top_srcdir)/docs/rsync-dist.make
