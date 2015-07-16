#!/usr/bin/python

from PIL import Image

im = Image.open('font.png')
print im.format, im.size, im.mode
data = []
for x in range(im.size[0]) :
    columnData = 0
    for y in range(8) :
        columnData = (columnData << 1) | bool(im.getpixel( (x,y)))
    data.append(columnData)

letters = ([chr(x) for x in range(ord('A'),ord('Z')+1)] + 
          [chr(x) for x in range(ord('a'),ord('z')+1)] + 
          [chr(x) for x in range(ord('0'),ord('9')+1)] )

print letters

with open('font.cpp','w') as cppFile :
    cppFile.write("static const unsigned char font[] PROGMEM = {\n");
    for index,byte in enumerate(data) :
        cppFile.write(hex(byte))
        if (index != len(data)-1) :
            cppFile.write(", ")
        if (byte == 0) :
            cppFile.write("\n")
    cppFile.write("}\n");

print len(data)


