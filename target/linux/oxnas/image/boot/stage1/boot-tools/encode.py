#!/usr/bin/python 

import sys


def encode (a, outfile):
        # print "a=",a
        b=ord(a)
        for i in range(8) :
                if b & 1 :
                        outfile.write (chr(0x55))
                else :
                        outfile.write (chr(0xAA))
                b >>=1

def main():
        print "number of arguments =" , len(sys.argv)
        if len(sys.argv) < 3 :
                print "usage: encode in_file out_file"
        else :
                infile = open (sys.argv[1],"rb")
                outfile = open (sys.argv[2],"wb")
        a=infile.read(1)
        while a :
                encode(a,outfile)
                a=infile.read(1)

main()
