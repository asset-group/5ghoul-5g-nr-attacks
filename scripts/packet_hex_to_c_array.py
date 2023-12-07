#!/usr/bin/python
import sys

if len(sys.argv) > 1:
	data = bytearray(sys.argv[1].decode('hex'))

	c_array = ''

	c_array = 'uint8_t packet[] = { '
	for idx, b in enumerate(data):
	    c_array += hex(b)
	    if idx is not len(data) - 1:
	        c_array += ', '
	        if idx != 0 and idx % 8 == 0:
	            c_array += '\n'
	c_array += ' };'

	print(c_array)
else:
	print "Insert hex array as argument"
