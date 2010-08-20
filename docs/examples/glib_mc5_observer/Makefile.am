INCLUDES = $(TELEPATHY_GLIB_CFLAGS)
LDADD = $(TELEPATHY_GLIB_LIBS)

noinst_PROGRAMS = example example2

example_SOURCES = \
	example-observer.c example-observer.h \
	example.c

example2_SOURCES = \
	example2.c

include $(top_srcdir)/docs/rsync-dist.make
