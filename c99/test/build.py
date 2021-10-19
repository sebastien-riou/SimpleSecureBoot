#!/usr/bin/env python3
import os
import sys
from psb import *


try:
    os.remove(test.exe)
    os.remove(testssbl.exe)
except:
    pass
    

#print("cwd="+os.getcwd())    
#gcc --std=c99 -Wall -Werror -I . -I ../inc -DSSB_KEY_LENGTH=$1 -DBUF_SIZE=$2 test.c -o a.exe
compile_args = "--std=c99 -Wall -Werror -Wno-unused-function -DSSB_KEY_LENGTH=%s -DBUF_SIZE=%s"%(sys.argv[1],sys.argv[2])
test=sys.argv[4]

compile([test+".c"],output=test+".exe",args=compile_args,includes=['.','../inc'] )

#gcc --std=c99 -I . -I ../inc mysha256sum.c -o mysha256sum.exe

#./a.exe $3 
cmd = ["./"+test+".exe",sys.argv[3]]
print(' '.join(cmd))
sys.stdout.flush()  
subprocess.run(cmd,check=True,shell=False)
