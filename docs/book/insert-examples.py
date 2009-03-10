#!/usr/bin/env python
#
# Insert examples into a Docbook document.
#
# This program searches the code for instances of:
#    <example file="some file">
# and adds the contents of that file to the example.
#
# Additionally, if an id is defined for the example. It will only attempt
# to include the section between the commented lines: 'begin id' and 'end id'
#
# Copyright (C) 2009 Collabora Ltd.
#
# Authors: Davyd Madeley <davyd.madeley@collabora.co.uk>

import sys
import os.path
import re
from lxml import etree

doc = etree.parse (sys.stdin)
examplesdir = sys.argv[1]

included_files = {}

# find all example elements that have the 'file' attribute
examples = doc.xpath ("//example[@file]")

for example in examples:

	id = example.get ('id')

	nicename = example.get ('file')
	filename = os.path.join (examplesdir, nicename)

	# del (example.attrib['file']) # unset the file property

	# attempt to load the file
	try:
		f = open (filename)
		contents = f.read ().rstrip ()
	except IOError, e:
		print >> sys.stderr, e
		continue
	
	if nicename not in included_files:
		included_files[nicename] = []
	if id is not None:
		included_files[nicename].append (id)

	if id:
		print >> sys.stderr, "Including `%s' from `%s'..." % (id, filename)

		begin_re = re.compile ('begin %s(\\s|$)' % id)
		end_re = re.compile ('end %s(\\s|$)' % id)

		begin = False
		lines = []
		for line in contents.split ('\n'):
			if begin and end_re.search (line):
				break
			elif begin:
				lines.append (line)
			elif not begin and begin_re.search (line):
				begin = True
				continue

		if lines != []: contents = '\n'.join (lines)

	else:
		print >> sys.stderr, "Including file `%s'..." % filename

	etree.SubElement (example, 'programlisting').text = etree.CDATA (contents)
	p = etree.SubElement (example, 'para')
	etree.SubElement (p, 'link', linkend='appendix.source-code.%s' % nicename).text = "Complete Source Code"

	f.close ()

# build an appendix of source files
appendix = doc.xpath ('/book/appendix[@id="source-code"]')[0]
for nicename in included_files:
	filename = os.path.join (examplesdir, nicename)

	s = etree.SubElement (appendix, 'sect1',
				id='appendix.source-code.%s' % nicename)
	etree.SubElement (s, 'title').text = nicename

	try:
		f = open (filename)
		contents = f.read ().rstrip ()
	except IOError, e:
		print >> sys.stderr, e
		continue
	
	# find the starting offsets for each id in the file
	def get_tuple (prefix, id):
		str = '%s %s' % (prefix, id)
		m = re.search (str + '(\\s|$)', contents)
		if m is None: idx = None
		else:
			idx = m.span()[0]
		return (idx, prefix, id, len (str))
	offsets = map (lambda id: get_tuple ('begin', id),
			included_files[nicename]) + \
		  map (lambda id: get_tuple ('end', id),
		  	included_files[nicename])
	offsets.sort () # sort into ascending order
	
	pl = etree.SubElement (s, 'programlisting')
	cumoff = 0
	elem = None

	for (offset, prefix, id, l) in offsets:
		if offset is None: continue

		# append the CDATA to elem
		cdata = contents[cumoff:offset]
		if elem is not None: elem.tail = cdata
		else: pl.text = cdata

		cumoff = offset + l

		# FIXME - I'd like something better than emphasis
		em = etree.Element ('emphasis')
		em.text = prefix.title () + " "
		etree.SubElement (em, 'xref', linkend=id)
		pl.append (em)

		elem = em

	if elem is None:
		pl.text = contents[cumoff:]
	else:
		elem.tail = contents[cumoff:]

sys.stdout.write (etree.tostring (doc))
