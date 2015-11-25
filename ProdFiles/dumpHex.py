#!/usr/bin/python

import sys
from intelhex import IntelHex

hexFile = IntelHex(sys.argv[1])

hexFile.dump()
