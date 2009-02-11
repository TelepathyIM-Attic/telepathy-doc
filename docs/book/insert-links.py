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

# FIXME - it would be awesome to be able to derive these mappings directly
# from the Telepathy spec
MAPPINGS = {
    'interfacename': {
    	# interfaces
	'Requests'	: 'Connection.Interface.Requests',
    },

    'methodname': {
    	# methods
	'CreateChannel'	: 'Connection.Interface.Requests.CreateChannel',
	'EnsureChannel'	: 'Connection.Interface.Requests.EnsureChannel',
	'RequestChannel': 'Connection.RequestChannel',
    },
}

import sys
from lxml import etree

doc = etree.parse (sys.stdin)

xpathexpr = '(%s)[not(ancestor::ulink)]' % \
		' | '.join (map (lambda s: '//%s' % s, MAPPINGS.keys ()))
for elem in doc.xpath (xpathexpr):
	if elem.tag not in MAPPINGS: raise Exception ("Got tag that's not in mappings")
	mapping = MAPPINGS[elem.tag]
	if elem.text not in mapping: continue
	suffix = mapping[elem.text]

	# print >> sys.stderr, elem.tag, elem.text

	link = etree.Element ('ulink', url=URL_PREFIX + suffix)
	link.tail = elem.tail
	elem.tail = None

	parent = elem.getparent ()
	parent.replace (elem, link)
	link.append (elem)

sys.stdout.write (etree.tostring (doc))
