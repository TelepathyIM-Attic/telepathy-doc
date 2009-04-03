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

URL_PREFIX = 'http://telepathy.freedesktop.org/spec.html#org.freedesktop.Telepathy.'

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

        self._build_function_list(dom)
        self._build_class_list(dom)

    def get_maps(self):
        return {
            'function': self.functions,
            'classname': self.classes,
        }

    def _build_url(self, link):
        return os.path.join(self.urlprefix, link)

    def _build_function_list(self, dom):
        functions = dom.xpath(
            'dh:functions/dh:keyword[@type = "function" or @type = "macro"]',
                                namespaces = self.namespaces)

        self.functions = dict((f.get("name")[:-3],
                               self._build_url(f.get("link")))
                          for f in functions)

    def _build_class_list(self, dom):
        classes = dom.xpath(self.query, type = 'struct',
                                namespaces = self.namespaces)

        self.classes = dict((f.get("name"),
                               self._build_url(f.get("link")))
                          for f in classes)

MAPPINGS = Mapper(
    DevhelpMapper('http://telepathy.freedesktop.org/doc/telepathy-glib/',
        '/usr/share/gtk-doc/html/telepathy-glib/telepathy-glib.devhelp2.gz'),
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
