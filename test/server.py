#!/bin/env python

import sys,math
import numpy as np
import ctypes
from cslib import CSlib

def error(txt):
  print "ERROR:",txt
  sys.exit(1)

# main program
# command-line args

if len(sys.argv) != 4:
  print "Syntax: python server.py mode N dtype"
  print "  mode = file or zmq"
  print "  N = length of data vectors"
  print "  dtype = 1/2/3 = list, Numpy, ctypes"
  sys.exit(1)

mode = sys.argv[1]
nlen = int(sys.argv[2])
dtype = int(sys.argv[3])

if nlen < 1: error("Nlen < 1")
if dtype < 1 or dtype > 3: error("Dtype < 1 or dtype > 3")

# setup messaging

if mode == "file": cs = CSlib(1,mode,"tmp.couple",None)
elif mode == "zmq": cs = CSlib(1,mode,"*:5555",None)
else: error("Invalid mode")

# setup data according to dtype

nper = 3

if dtype == 1:
  sint = nlen*[0]
  sint64 = nlen*[0]
  sfloat = nlen*[0.0]
  sdouble = nlen*[0.0]
  sarray = [[0]*nper for i in range(nlen)]
  
elif dtype == 2:
  sint = np.zeros(nlen,np.intc)
  sint64 = np.zeros(nlen,np.int64)
  sfloat = np.zeros(nlen,np.float32)
  sdouble = np.zeros(nlen,np.float)
  sarray = np.zeros((nlen,nper),np.float)

elif dtype == 3:
  sint = (nlen * ctypes.c_int)()
  sint64 = (nlen * ctypes.c_longlong)()
  sfloat = (nlen * ctypes.c_float)()
  sdouble = (nlen * ctypes.c_double)()
  sarray = (nlen * (nper * ctypes.c_double))()

sstring = "world"

# endless server loop

while 1:

  # recv message from client
  # msgID = 0 = all-done message

  msgID,nfield,fieldID,fieldtype,fieldlen = cs.recv()
  if msgID == 0: break

  rint = cs.unpack(1,tflag=dtype)
  rint64 = cs.unpack(2,tflag=dtype)
  rfloat = cs.unpack(3,tflag=dtype)
  rdouble = cs.unpack(4,tflag=dtype)
  rvec = cs.unpack(5,tflag=dtype)
  if dtype == 1:
    rarray = [rvec[i:i+nper] for i in range(0,len(rvec),nper)] 
  elif dtype == 2:
    rarray = np.reshape(rvec,(nlen,nper))
  oneint = cs.unpack_int(6);
  oneint64 = cs.unpack_int64(7);
  onefloat = cs.unpack_float(8);
  onedouble = cs.unpack_double(9);
  rstring = cs.unpack_string(10);

  # increment values
  
  m = 0
  for i in range(nlen):
    sint[i] = rint[i] + 1
    sint64[i] = rint64[i] + 1
    sfloat[i] = rfloat[i] + 1.0
    sdouble[i] = rdouble[i] + 1.0
    if dtype <= 2:
      for j in range(nper):
        sarray[i][j] = rarray[i][j] + 1.0
    else:
      for j in range(nper):
        sarray[i][j] = rvec[m] + 1.0
        m += 1

  # send message to client
  
  cs.send(1,10)
  cs.pack(1,1,nlen,sint)
  cs.pack(2,2,nlen,sint64)
  cs.pack(3,3,nlen,sfloat)
  cs.pack(4,4,nlen,sdouble)
  if dtype == 1:
    svec = sum(sarray,[])
    cs.pack(5,4,nper*nlen,svec)
  else: cs.pack(5,4,nper*nlen,sarray)
  cs.pack_int(6,oneint+1)
  cs.pack_int64(7,oneint64+1)
  cs.pack_float(8,onefloat+1)
  cs.pack_double(9,onedouble+1)
  cs.pack_string(10,sstring)

# final reply to client
  
cs.send(0,0)

# clean-up

del cs
