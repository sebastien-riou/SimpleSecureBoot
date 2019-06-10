#!/usr/bin/env python3
import os
import sys
from Crypto.Hash import SHA256

if (len(sys.argv) > 2) | (len(sys.argv) < 1) :
    print("ERROR: incorrect arguments")
    print("Usage:")
    print("sha256sum.py <file>")
    exit()

fname = sys.argv[1]

h = SHA256.new()
with open(fname, "rb") as f:
    byte = f.read(1)
    while byte:
        #b=int.from_bytes(byte,byteorder='little')
        #print("%02X "%(b))
        #h.update(b.to_bytes(1,byteorder='little'))
        h.update(bytes(byte))
        byte = f.read(1)

digest_bytes=h.digest()
digest=int.from_bytes( digest_bytes, byteorder='big')
print("%032x"%digest)
