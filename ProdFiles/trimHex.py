#!/usr/bin/python

import sys
from intelhex import IntelHex

hexFile = IntelHex(sys.argv[1])
trimHex = IntelHex()

#for i in hexFile.addresses() :
#    if hexFile[i] != 0xFF :
#        trimHex[i] = hexFile[i]

for i in range(0, 128*1024, 16) :
    writeRow = False
    for j in range(i, i+16) :
        if hexFile[j] != 0xFF :
            writeRow = True
            break

    if writeRow :
        for j in range(i, i+16) :
            trimHex[j] = hexFile[j]

# Write to the combined hex file
trimHex.tofile(sys.argv[2], 'hex')
