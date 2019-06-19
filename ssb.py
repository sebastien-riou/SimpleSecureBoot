#!/usr/bin/env python3
import os
import sys
import runpy
from intelhex import IntelHex
from rsa_pkcs1_15_sha256 import *
import subprocess


def bytes_length(x):
    return (x.bit_length() + 7) // 8

if (len(sys.argv) > 5) | (len(sys.argv) < 5) :
    print("ERROR: incorrect arguments")
    print("Usage:")
    print("ssb.py <start addr> <size> <ihex file> <RSA key file>")
    exit()

start_addr = int(sys.argv[1],0)
size = int(sys.argv[2],0)
ihexf = sys.argv[3]
keyf = sys.argv[4]

ih = IntelHex()
iho = IntelHex()
ih.loadhex(ihexf)
file_globals = runpy.run_path(keyf)
d=file_globals["d"]
e=file_globals["e"]
n=file_globals["n"]
key_size = bytes_length(n)*8
print("Signing %d bytes using %d bits RSA key"%(size,key_size))

#restrict alignements to ease the implementation on device side
assert(0 == (start_addr % 32))
assert(0 == ((size+4) % 32))

message_bytes = bytearray()
for addr in range(start_addr,start_addr+size):
    b=ih[addr]
    iho[addr]=b
    message_bytes.append(b)
    #print("%04X %02X "%(addr,b))

#append the start address info on 4 bytes, little endian    
if ih.start_addr is None:
    print("ERROR: no start address defined in the hex file")
    exit(-1)
    
try:
    start=ih.start_addr['EIP']
except:
    start=ih.start_addr['IP']+(ih.start_addr['CS']<<4)
#print(ih.start_addr)
#print("start=0x%08x"%start)
start_bytes = start.to_bytes(4,byteorder='little')
for i in range(0,4):
    b=start_bytes[i]
    iho[start_addr+size+i]=b
    message_bytes.append(b)
size+=4
    
sig_bytes=RSA_SIGN_PKCS1_V1_5_SHA256(message_bytes,d,n)
sig = int.from_bytes(sig_bytes,byteorder='little')
#print("signature: 0x%x"%sig)

assert(bytes_length(n)==bytes_length(sig))
#print("e: 0x%x"%e)
#print("n: 0x%x"%n)

#sanity check using python implementation
RSA_VERIFY_PKCS1_V1_5_SHA256(message_bytes,sig_bytes,e,n)

sig_bytes = sig.to_bytes(bytes_length(n),byteorder='little')
for i in range(0,len(sig_bytes)):
    iho[start_addr+size+i] = sig_bytes[i]

 
iho.write_hex_file(ihexf+".signed.ihex")
sys.stdout.flush()  
try:
    test_path=os.path.abspath(os.path.join(os.path.dirname(__file__),'c99','test'))
    build_path=os.path.join(test_path,'build.py')
    cmd = [ sys.executable, build_path, "%d"%key_size, "%d"%size, os.path.abspath(ihexf+".signed.ihex") ]
    print(' '.join(cmd))
    subprocess.run(cmd,check=True,cwd=test_path, shell=False)
except:
    print()
    #print n as a C array of 32 bit words
    print("static const uint32_t ssb_e = 0x%08X;"%e)
    print("static const uint32_t ssb_n[%d] = {"%(key_size//32))
    for i in range(0,key_size,32):
        print("0x%08X,"%(0xFFFFFFFF & (n>>i)),end="")
        if 0==((i+32)%256):
            print()
    print("};")
    print("\nERROR: C code needs update (most likely key don't match)\n")
    exit(-1)
