#!/usr/bin/env python
#
# Insert links into a Docbook document.
#
# This program searches through a Docbook document for instances of
# <interfacename>, <methodname>, etc. and attempts to ulink them back
# to a specification.
#
# Copyright (C) 2009 Collabora Ltd.
#
# Authors: Davyd Madeley <davyd.madeley@collabora.co.uk>

import sys
import os.path
from lxml import etree

DEVHELP_NS = 'http://www.devhelp.net/book'

class Mapper(object):
    def __init__(self, *maps):
        self.maps = maps

    def get_maps(self):
        map_ = {}

        for mapper in self.maps:
            for key, submap in mapper.get_maps().iteritems():
                if key not in map_: map_[key] = {}

                for name, url in submap.iteritems():
                    if name in map_[key]:
                        print >> sys.stderr, \
                                    "WARNING: %s could be ambiguous" % name

                    map_[key][name] = url

        return map_

class DevhelpMapper(object):
    query = 'dh:functions/dh:keyword[@type = $type]'
    namespaces = {
        'dh': DEVHELP_NS,
    }

    def __init__(self, urlprefix, filename):
        self.urlprefix = urlprefix

        dom = etree.parse(filename)
        self.build_maps(dom)

    def build_maps(self, dom):
        self._build_function_list(dom)
        self._build_class_list(dom)

    def get_maps(self):
        return {
            'function': self.functions,
            'classname': self.classes,
        }

    def build_map(self, items, name_func = lambda n: n):
        return dict((name_func(i.get("name")), self._build_url(i.get("link")))
                    for i in items)


    def xpath_query(self, dom, query = None, **kwargs):
        if query is None: query = self.query
        return dom.xpath(query, namespaces = self.namespaces, **kwargs)

    def _build_url(self, link):
        return os.path.join(self.urlprefix, link)

    def _build_function_list(self, dom):
        functions = self.xpath_query(dom,
            'dh:functions/dh:keyword[@type = "function" or @type = "macro"]')
        self.functions = self.build_map(functions, lambda n: n[:-3])

    def _build_class_list(self, dom):
        classes = self.xpath_query(dom, type = 'struct')
        self.classes = self.build_map(classes)

class SpecMapper(DevhelpMapper):
    query = 'dh:functions/dh:keyword[starts-with(string(@name), $type)]'

    def build_maps(self, dom):
        self._build_interface_list(dom)
        self._build_method_list(dom)

    def get_maps(self):
        return {
            'interfacename': self.interfaces,
            'methodname': self.methods,
        }

    def _build_interface_list(self, dom):
        interfaces = self.xpath_query(dom, type = 'Interface ')
        self.interfaces = self.build_map(interfaces,
            lambda n: n.rsplit('.', 1)[-1])

    def _build_method_list(self, dom):
        methods = self.xpath_query(dom, type = 'Method ')
        self.methods = self.build_map(methods,
            lambda n: n.rsplit('.', 1)[-1])

MAPPINGS = Mapper(
    DevhelpMapper('http://telepathy.freedesktop.org/doc/telepathy-glib/',
        '/usr/share/gtk-doc/html/telepathy-glib/telepathy-glib.devhelp2.gz'),
    SpecMapper('http://telepathy.freedesktop.org/spec',
        '/usr/share/gtk-doc/html/telepathy-spec/telepathy-spec.devhelp2'),
).get_maps()

try:
    filename = sys.argv[1]
    doc = etree.parse (filename)
    doc.xinclude ()
except IndexError:
    doc = etree.parse (sys.stdin)

xpathexpr = '(%s)[not(ancestor::ulink)]' % \
		' | '.join (map (lambda s: '//%s' % s, MAPPINGS.keys ()))
for elem in doc.xpath (xpathexpr):
	if elem.tag not in MAPPINGS: raise Exception ("Got tag that's not in mappings")
	mapping = MAPPINGS[elem.tag]
	if elem.text not in mapping: continue
	url = mapping[elem.text]

	# print >> sys.stderr, elem.tag, elem.text

	link = etree.Element ('ulink', url=url)
	link.tail = elem.tail
	elem.tail = None

	parent = elem.getparent ()
	parent.replace (elem, link)
	link.append (elem)

sys.stdout.write (etree.tostring (doc))
