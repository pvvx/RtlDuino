#!/usr/bin/env python
#-*- coding: utf:8 -*-

import sys
import struct
import ntpath

argv = sys.argv
argc = len(argv)
 
if argc < 2:
    print 'Usage: python '+argv[0]+' file_name [word_in_line]'
    quit()

dwords_in_line = 8
if argc == 3:
	dwords_in_line = int(argv[2])
	if not 0 < dwords_in_line < 33:
		dwords_in_line = 6
count = 0

try:
    f = open(argv[1], 'rb')

except:
    print 'Error: %s is not found.' % argv[1]
    quit()

f.seek(0,2)	# move the cursor to the end of the file
size = f.tell()
f.seek(0,0)
filename, ext = ntpath.splitext(ntpath.basename(argv[1]))
print 'unsigned char %s_code[]' % filename, ' = {'
print '\t',
count = 0
while True:
	w = f.read(1)
	if not w: break
	count += 1
	if count == size:
		print '0x%02x };' % struct.unpack('<B', w) 
	else:
		print '0x%02x,' % struct.unpack('<B', w), 	
		if count % dwords_in_line == 0:
			print
			print '\t',
f.close()