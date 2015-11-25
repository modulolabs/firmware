#!/usr/bin/python

import sys
from intelhex import IntelHex


def mergeAVR() :
    bootHex = IntelHex(sys.argv[2])
    appHex = IntelHex(sys.argv[3])
    combinedHex = IntelHex()

    # Insert the application in the first 6kB
    for i in range(1024*6) :
        combinedHex[i] = appHex[i]

    # Insert the bootloader in the last 2kB
    for i in range(1024*6,1024*8) :
        combinedHex[i] = bootHex[i]

    # The first two bytes are the reset vector. They come from the bootloader.
    combinedHex[0] = bootHex[0]
    combinedHex[1] = bootHex[1]

    # When the bootloader exits, it will execute the instruction at byte address
    # 0x17FE (the byte immediately before the bootloader). We need to arrange
    # for that instruction to be an rjmp to the application. Normally the
    # firt instruction (at address 0) is an rjmp to the application, so we can
    # determine the app start address from that.

    # Word address of the app start vector
    appStartAddress = 0xFFF & ( appHex[0] + (appHex[1] << 8))

    # Word address of the trampoline instruction
    trampolineAddress = (6*1024)/2 - 1

    # Determine the offset between the trampoline and app start addresses.
    # Mask it to 12 bits.
    trampoline = 0xFFF & (appStartAddress-trampolineAddress)

    # The rjmp opcode is 0xC000. The lower twelve bits are the address offset.
    trampolineInstruction = 0xC000 | trampoline

    # Insert the instruction at the trampoline address
    combinedHex[trampolineAddress*2] = (trampolineInstruction & 0xFF)
    combinedHex[trampolineAddress*2+1] = (trampolineInstruction >> 8)

    # Write to the combined hex file
    combinedHex.tofile(sys.argv[4], 'hex')



def mergeSAMD() :
    bootHex = IntelHex(sys.argv[2])
    appHex = IntelHex(sys.argv[3])
    combinedHex = IntelHex()

    # Insert the bootloader in the first 8kB
    for i in range(1024*8) :
        combinedHex[i] = bootHex[i]

    # Insert the application in the remaining 120kB
    for i in range(1024*8,1024*128) :
        combinedHex[i] = appHex[i]    

    # Write to the combined hex file
    combinedHex.tofile(sys.argv[4], 'hex')

platform = sys.argv[1]

if platform == '-samd' :
    mergeSAMD()
elif platform == '-avr' :
    mergeAVR()
else :
    print('try: mergeBootloader.py -avr Bootloader.hex Application.hex Combined.hex')
    print(' or: mergeBootloader.py -sam Bootloader.hex Application.hex Combined.hex')


