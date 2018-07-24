#!/bin/env python

# serial client example in Python

import sys,math,time
import numpy as np
import ctypes
from cslib import CSlib

BIGOFFSET = 3000000000
FLOATOFFSET = 0.5

def error(txt):
  print "ERROR:",txt
  sys.exit(1)

# main program
# command-line args

if len(sys.argv) != 5:
  print "Syntax: python client.py mode N Niter dtype"
  print "  mode = file or zmq"
  print "  N = length of data vectors"
  print "  Niter = # of iterations"
  print "  dtype = 1/2/3 = list, Numpy, ctypes"
  sys.exit(1)

mode = sys.argv[1]
nlen = int(sys.argv[2])
niter = int(sys.argv[3])
dtype = int(sys.argv[4])

if nlen < 1: error("Nlen < 1")
if niter < 1: error("Niter < 1")
if dtype < 1 or dtype > 3: error("Dtype < 1 or dtype > 3")

# setup messaging

if mode == "file": cs = CSlib(0,mode,"tmp.couple",None)
elif mode == "zmq": cs = CSlib(0,mode,"localhost:5555",None)
else: error("Invalid mode")

# setup data according to dtype

nper = 3

if dtype == 1:
  sint = nlen*[0]
  sint64 = nlen*[0]
  sfloat = nlen*[0]
  sdouble = nlen*[0]
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
  
for i in range(nlen):
  sint[i] = i+1
  sint64[i] = BIGOFFSET + i+1
  sfloat[i] = FLOATOFFSET + i+1
  sdouble[i] = BIGOFFSET + FLOATOFFSET + i+1
  for i in range(nlen):
    for j in range(nper):
      sarray[i][j] = sdouble[i]

oneint = 1;
oneint64 = BIGOFFSET + 1;
onefloat = FLOATOFFSET + 1;
onedouble = BIGOFFSET + FLOATOFFSET + 1;

sstring = "hello"

# client loop

tstart = time.time()

for iter in range(niter):

  # send message to server

  cs.send(1,10)
  cs.pack(1,1,nlen,sint)
  cs.pack(2,2,nlen,sint64)
  cs.pack(3,3,nlen,sfloat)
  cs.pack(4,4,nlen,sdouble)
  if dtype == 1:
    svec = sum(sarray,[])
    cs.pack(5,4,nper*nlen,svec)
  else: cs.pack(5,4,nper*nlen,sarray)
  cs.pack_int(6,oneint)
  cs.pack_int64(7,oneint64)
  cs.pack_float(8,onefloat)
  cs.pack_double(9,onedouble)
  cs.pack_string(10,sstring)

  # recv message from server
  
  msgID,nfield,fieldID,fieldtype,fieldlen = cs.recv()
  rint = cs.unpack(1,tflag=dtype)
  rint64 = cs.unpack(2,tflag=dtype)
  rfloat = cs.unpack(3,tflag=dtype)
  rdouble = cs.unpack(4,tflag=dtype)
  rvec = cs.unpack(5,tflag=dtype)
  if dtype == 1:
    rarray = [rvec[i:i+nper] for i in range(0,len(rvec),nper)] 
  elif dtype == 2:
    rarray = np.reshape(rvec,(nlen,nper))
  oneint = cs.unpack_int(6)
  oneint64 = cs.unpack_int64(7)
  onefloat = cs.unpack_float(8)
  onedouble = cs.unpack_double(9)
  rstring = cs.unpack_string(10)
  
  # copy server data to client data
  
  m = 0
  for i in range(nlen):
    sint[i] = rint[i]
    sint64[i] = rint64[i]
    sfloat[i] = rfloat[i]
    sdouble[i] = rdouble[i]
    if dtype <= 2:
      for j in range(nper):
        sarray[i][j] = rarray[i][j]
    else:
      for j in range(nper):
        sarray[i][j] = rvec[m]
        m += 1
        
tstop = time.time()

# all-done message to server
  
cs.send(0,0)
cs.recv()

# error check
  
delta = 0.0;

for i in range(nlen):
  one = i+1 + niter - sint[i];
  delta = max(delta,math.fabs(one));
  one = BIGOFFSET + i+1 + niter - sint64[i];
  delta = max(delta,math.fabs(one));
  one = FLOATOFFSET + i+1 + niter - sfloat[i];
  delta = max(delta,math.fabs(one));
  one = BIGOFFSET+FLOATOFFSET + i+1 + niter - sdouble[i];
  delta = max(delta,math.fabs(one));
  for j in range(nper):
    one = BIGOFFSET+FLOATOFFSET + i+1 + niter - sarray[i][j];
    delta = max(delta,math.fabs(one));

one = 1 + niter - oneint
delta = max(delta,math.fabs(one))
one = BIGOFFSET+1 + niter - oneint64
delta = max(delta,math.fabs(one))
one = FLOATOFFSET+1 + niter - onefloat
delta = max(delta,math.fabs(one))
one = BIGOFFSET+FLOATOFFSET+1 + niter - onedouble
delta = max(delta,math.fabs(one))

# stats

print "Nsend,nrecv:",cs.extract(1),cs.extract(2)
print "Initial/final first int:",1,sint[0]
print "Initial/final last int:",nlen,sint[nlen-1]
print "Initial/final first int64:",BIGOFFSET+1,sint64[0]
print "Initial/final last int64:",BIGOFFSET+nlen,sint64[nlen-1]
print "Initial/final first float:",FLOATOFFSET+1,sfloat[0]
print "Initial/final last float:",FLOATOFFSET+nlen,sfloat[nlen-1]
print "Initial/final first double:",BIGOFFSET+FLOATOFFSET+1,sdouble[0]
print "Initial/final last double:",BIGOFFSET+FLOATOFFSET+nlen,sdouble[nlen-1]
print "Initial/final first array:",BIGOFFSET+FLOATOFFSET+1,sarray[0][0]
print "Initial/final last array:",BIGOFFSET+FLOATOFFSET+nlen, \
  sarray[nlen-1][nper-1]
print "Initial/final int scalar:",1,oneint
print "Initial/final int64 scalar:",BIGOFFSET+1,oneint64
print "Initial/final float scalar:",FLOATOFFSET+1,onefloat
print "Initial/final double scalar:",BIGOFFSET+FLOATOFFSET+1,onedouble
print "Initial/final string:",sstring,rstring

print "Max error for any value:",delta
print "CPU time: %g" % (tstop-tstart)
datumsize = 4 + 8 + 4 + 8 + 1
if dtype > 1: datumsize += nper*8
bandwith = 2.0 * nlen * datumsize / (tstop-tstart) / 1024/1024
print "Message bandwidth: %g MB/sec" % bandwith

# clean-up

del cs
