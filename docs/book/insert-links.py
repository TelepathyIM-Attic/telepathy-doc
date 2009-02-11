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

MAPPINGS = {
    'interfacename': {
    	# interfaces
	'Requests'	: 'Connection.Interface.Requests',
    },

    'methodname': {
    	# methods
	'CreateChannel'	: 'Connection.Interface.Requests.CreateChannel',
	'EnsureChannel'	: 'Connection.Interface.Requests.EnsureChannel',
    },
}

import sys
from lxml import etree

doc = etree.parse (sys.stdin)

xpathexpr = ' | '.join (map (lambda s: '//%s' % s, MAPPINGS.keys ()))
for elem in doc.xpath (xpathexpr):
	# ignore nodes that are already linked, can this be done in XPath
	if elem.getparent ().tag == 'ulink': continue

	if elem.tag not in MAPPINGS: continue
	mapping = MAPPINGS[elem.tag]
	if elem.text not in mapping: continue
	suffix = mapping[elem.text]

	parent = elem.getparent ()

	link = etree.Element ('ulink', url=URL_PREFIX + suffix)
	parent.replace (elem, link)
	link.append (elem)

sys.stdout.write (etree.tostring (doc))
