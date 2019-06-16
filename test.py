#!/usr/bin/env python3
import os
import sys
import subprocess


#sanity check using C99 code
ssb_path=os.path.abspath(os.path.join(os.path.dirname(__file__),'ssb.py'))

def test(start,size,ihex,key):
    cmd = [ sys.executable, 'ssb.py',start,size,ihex,key]
    print(' '.join(cmd))
    subprocess.run(cmd,check=True)

for size in ['0x200','0x4000']:
    for key in ['key512','key1024','key2048','key4096']:
        test('0', size, 'test.ihex',key)
        print()
