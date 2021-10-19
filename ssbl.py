#!/usr/bin/env python3
import os
import sys
import runpy
from intelhex import IntelHex
from rsa_pkcs1_15_sha256 import *
import subprocess
from pysatl import Utils

def bytes_length(x):
    return (x.bit_length() + 7) // 8

def print_c99_array32(name,n,bitlength):
    print("static const uint32_t %s[%d] = {"%(name,(bitlength+31)//32))
    for i in range(0,bitlength,32):
        print("0x%08X,"%(0xFFFFFFFF & (n>>i)),end="")
        if 0==((i+32)%256):
            print()
    print("};")

if (len(sys.argv) > 4) | (len(sys.argv) < 4) :
    print("ERROR: incorrect arguments")
    print("Usage:")
    print("ssbl.py <load address> <ihex file> <RSA key file>")
    exit(-1)

load_addr = int(sys.argv[1],0)
ihexf = sys.argv[2]
keyf = sys.argv[3]

ih = IntelHex()
ih.loadhex(ihexf)
file_globals = runpy.run_path(keyf)
d=file_globals["d"]
e=file_globals["e"]
n=file_globals["n"]
key_size = bytes_length(n)

all_sections = ih.segments()
low_addr=all_sections[0][0]
high_addr=all_sections[0][1]
for sec in all_sections:
    low_addr=min(low_addr,sec[0])
    high_addr=max(high_addr,sec[1])
size=high_addr-low_addr
if size>16*1024*1024:
    print("image is larger than 16MB, give up")
    exit(-1)

print("Signing %d bytes using %d bits RSA key"%(size,key_size*8))
print("Load address = %x"%(load_addr))

header_size = 32
dat_padlen = (8-(size % 8)) & 0x7
key_padlen = 4
padlen=4
total_size = header_size + size + dat_padlen + key_size + key_padlen + padlen
#restrict alignements to ease the implementation on device side
assert(0 == (load_addr % 8))
assert(0 == (total_size % 8))

if ih.start_addr is None:
    start_addr = 0xFFFFFFFFFFFFFFFF
else:
    try:
        start_addr=ih.start_addr['EIP']
    except:
        start_addr=ih.start_addr['IP']+(ih.start_addr['CS']<<4)

magic = "ssbl001"
message_bytes = bytearray()
message_bytes += total_size.to_bytes(8,byteorder='little')
message_bytes += magic.encode('utf-8') + bytes(1)
message_bytes += load_addr.to_bytes(8,byteorder='little')
message_bytes += start_addr.to_bytes(8,byteorder='little')

print(Utils.hexstr(message_bytes))

for i in range(0,size):
    b=ih[low_addr+i]
    message_bytes.append(b)
    #print("%04X %02X "%(addr,b))

message_bytes += bytes(dat_padlen)

sig_bytes=RSA_SIGN_PKCS1_V1_5_SHA256(message_bytes,d,n)
sig = int.from_bytes(sig_bytes,byteorder='little')
#print("signature: 0x%x"%sig)

assert(bytes_length(n)==bytes_length(sig))
#print("e: 0x%x"%e)
#print("n: 0x%x"%n)

#sanity check using python implementation
RSA_VERIFY_PKCS1_V1_5_SHA256(message_bytes,sig_bytes,e,n)

message_bytes += sig.to_bytes(bytes_length(n),byteorder='little')
message_bytes += bytes(key_padlen+padlen)
print(len(message_bytes))
print(total_size)
assert(len(message_bytes)==total_size)

print(Utils.hexstr(message_bytes))

iho = IntelHex()
for i in range(0,total_size):
    b = message_bytes[i]
    offset = load_addr+i
    iho[offset] = b

iho.write_hex_file(ihexf+".ssbl.ihex")
sys.stdout.flush()

try:
    test_path=os.path.abspath(os.path.join(os.path.dirname(__file__),'c99','test'))
    build_path=os.path.join(test_path,'build.py')
    cmd = [ sys.executable, build_path, "%d"%(key_size*8), "%d"%size, os.path.abspath(ihexf+".ssbl.ihex"), 'testssbl' ]
    print(' '.join(cmd))
    subprocess.run(cmd,check=True,cwd=test_path, shell=False)
except:
    print()
    actual_key_storage_bitsize = (key_size+key_padlen)*8
    #print n as a C array of 32 bit words
    print("static const uint32_t ssb_e = 0x%08X;"%e)
    print_c99_array32("ssb_n",n,actual_key_storage_bitsize)
    from montgommery_modexp import Montgommery
    m=Montgommery(n)
    print_c99_array32("ssb_n_R",m.R,actual_key_storage_bitsize)
    print_c99_array32("ssb_n_R2",m.R2,actual_key_storage_bitsize)
    print("\nERROR: C code needs update (most likely key don't match)\n")
    exit(-1)
