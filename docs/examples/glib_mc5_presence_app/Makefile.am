INCLUDES = $(TELEPATHY_GLIB_CFLAGS)
LDADD = $(TELEPATHY_GLIB_LIBS)

noinst_PROGRAMS = example

example_SOURCES = \
	connections-monitor.c connections-monitor.h \
	example.c \
	$(BUILT_SOURCES)

BUILT_SOURCES = \
	marshallers.c marshallers.h

marshallers.h: marshallers.list
	glib-genmarshal --header --prefix=_example $< > $@

marshallers.c: marshallers.list
	glib-genmarshal --body --prefix=_example $< > $@

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	marshallers.list

include $(top_srcdir)/docs/rsync-dist.make
