#!/usr/bin/env python3
import os
import sys
from Crypto.PublicKey import RSA

if (len(sys.argv) > 3) | (len(sys.argv) < 0) :
    print("ERROR: incorrect arguments")
    print("Usage:")
    print("genrsakey.py [bitlen] [e]")
    exit()

bitlen=2048
e=65537

if len(sys.argv)>1:
    bitlen=int(sys.argv[1],0)


if len(sys.argv)>2:
    e=int(sys.argv[2],0)

key = RSA.generate(bitlen, None, e)

print("d=%d"%key.d)
print("e=%d"%key.e)
print("n=%d"%key.n)
