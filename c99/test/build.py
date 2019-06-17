#!/usr/bin/env python3
import os
import sys
from psb import *


try:
    os.remove(a.exe)
except:
    pass
    

#print("cwd="+os.getcwd())    
#gcc --std=c99 -Wall -Werror -I . -I ../inc -DSSB_KEY_LENGTH=$1 -DBUF_SIZE=$2 test.c -o a.exe
compile(["test.c"],output="a.exe",args="--std=c99 -Wall -Werror -DSSB_KEY_LENGTH=%s -DBUF_SIZE=%s"%(sys.argv[1],sys.argv[2]),includes=['.','../inc'] )

#gcc --std=c99 -I . -I ../inc mysha256sum.c -o mysha256sum.exe

#./a.exe $3 
cmd = ["./a.exe",sys.argv[3]]
print(' '.join(cmd))
sys.stdout.flush()  
subprocess.run(cmd,check=True,shell=False)
