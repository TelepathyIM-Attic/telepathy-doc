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

import libxml2
import sys, os.path

doc = libxml2.parseFile (sys.argv[1])
examplesdir = sys.argv[2]

# find all example elements that have the 'file' attribute
examples = doc.xpathEval ("//example[@file]")

for example in examples:

	if example.hasProp ('id'): id = example.prop ('id')
	else: id = None

	filename = os.path.join (examplesdir, example.prop ('file'))
	example.unsetProp ('file') # unset the file property


	# attempt to load the file
	try:
		f = open (filename)
		contents = f.read ().rstrip ()
	except IOError, e:
		print e
		continue
	
	if id:
		print "Including `%s' from `%s'..." % (id, filename)
		
		begin = False
		lines = []
		for line in contents.split ('\n'):
			if begin and line.find ('end %s' % id) != -1:
				break
			elif begin:
				lines.append (line)
			elif not begin and line.find ('begin %s' % id) != -1:
				begin = True
				continue

		if lines != []: contents = '\n'.join (lines)

	else:
		print "Including file `%s'..." % filename

	n = example.newChild (None, 'programlisting', None)
	cdata = doc.newCDataBlock (contents, len (contents))
	n.addChild (cdata)

	f.close ()

doc.saveFormatFile (sys.argv[3], True)
