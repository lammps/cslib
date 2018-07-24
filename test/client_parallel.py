#!/bin/env python

# parallel client example in Python

import sys,math,time
import numpy as np
import ctypes
from mpi4py import MPI
from cslib import CSlib

BIGOFFSET = 3000000000
FLOATOFFSET = 0.5

def error(txt):
  if me == 0: print "ERROR:",txt
  sys.exit(1)

# main program
# MPI setup

world = MPI.COMM_WORLD
me = world.rank
nprocs = world.size

# command-line args

if len(sys.argv) != 5:
  print "Syntax: mpirun -np P python client_parallel.py mode N Niter type"
  print "  mode = file or zmq or mpi/one or mpi/two"
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

mpifree = 0
         
if mode == "file": cs = CSlib(0,mode,"tmp.couple",world)
elif mode == "zmq": cs = CSlib(0,mode,"localhost:5555",world)
elif mode == "mpi/one":
  mpifree = 1
  onlyclient = world.Split(0,me)
  me = onlyclient.rank
  nprocs = onlyclient.size
  both = world
  world = onlyclient
  cs = CSlib(0,mode,both,world) 
elif mode == "mpi/two": cs = CSlib(0,mode,"tmp.couple",world)
else: error("Invalid mode")

# setup data according to dtype

nper = 3
nlocal = nlen/nprocs
if me < nlen % nprocs: nlocal += 1

ids = nlocal*[0]
m = 0
for i in range(me,nlen,nprocs):
  ids[m] = i+1
  m += 1

if dtype == 1:
  sint = nlocal*[0]
  sint64 = nlocal*[0]
  sfloat = nlocal*[0]
  sdouble = nlocal*[0]
  sarray = [[0]*nper for i in range(nlocal)]

elif dtype == 2:
  sint = np.zeros(nlocal,np.intc)
  sint64 = np.zeros(nlocal,np.int64)
  sfloat = np.zeros(nlocal,np.float32)
  sdouble = np.zeros(nlocal,np.float)
  sarray = np.zeros((nlocal,nper),np.float)

elif dtype == 3:
  sint = (nlocal * ctypes.c_int)()
  sint64 = (nlocal * ctypes.c_longlong)()
  sfloat = (nlocal * ctypes.c_float)()
  sdouble = (nlocal * ctypes.c_double)()
  sarray = (nlocal * (nper * ctypes.c_double))()

for i in range(nlocal):
  sint[i] = ids[i]
  sint64[i] = BIGOFFSET + ids[i]
  sfloat[i] = FLOATOFFSET + ids[i]
  sdouble[i] = BIGOFFSET + FLOATOFFSET + ids[i]
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
  cs.pack_parallel(1,1,nlocal,ids,1,sint)
  cs.pack_parallel(2,2,nlocal,ids,1,sint64)
  cs.pack_parallel(3,3,nlocal,ids,1,sfloat)
  cs.pack_parallel(4,4,nlocal,ids,1,sdouble)
  if dtype == 1:
    svec = sum(sarray,[])
    cs.pack_parallel(5,4,nlocal,ids,nper,svec)
  else: cs.pack_parallel(5,4,nlocal,ids,nper,sarray)
  cs.pack_int(6,oneint)
  cs.pack_int64(7,oneint64)
  cs.pack_float(8,onefloat)
  cs.pack_double(9,onedouble)
  cs.pack_string(10,sstring)

  # recv message from server
  
  msgID,nfield,fieldID,fieldtype,fieldlen = cs.recv()
  rint = cs.unpack_parallel(1,nlocal,ids,1,tflag=dtype)
  rint64 = cs.unpack_parallel(2,nlocal,ids,1,tflag=dtype)
  rfloat = cs.unpack_parallel(3,nlocal,ids,1,tflag=dtype)
  rdouble = cs.unpack_parallel(4,nlocal,ids,1,tflag=dtype)
  rvec = cs.unpack_parallel(5,nlocal,ids,nper,tflag=dtype)
  if dtype == 1:
    rarray = [rvec[i:i+nper] for i in range(0,len(rvec),nper)] 
  elif dtype == 2:
    rarray = np.reshape(rvec,(nlocal,nper))
  oneint = cs.unpack_int(6)
  oneint64 = cs.unpack_int64(7)
  onefloat = cs.unpack_float(8)
  onedouble = cs.unpack_double(9)
  rstring = cs.unpack_string(10)

  # copy server data to client data
  
  m = 0
  for i in range(nlocal):
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

# all done
  
cs.send(0,0)
cs.recv()

# error check
  
delta = 0.0;

for i in range(nlocal):
  one = ids[i] + niter - sint[i];
  delta = max(delta,math.fabs(one));
  one = BIGOFFSET + ids[i] + niter - sint64[i];
  delta = max(delta,math.fabs(one));
  one = FLOATOFFSET + ids[i] + niter - sfloat[i];
  delta = max(delta,math.fabs(one));
  one = BIGOFFSET+FLOATOFFSET + ids[i] + niter - sdouble[i];
  delta = max(delta,math.fabs(one));
  for j in range(nper):
    one = BIGOFFSET+FLOATOFFSET + ids[i] + niter - sarray[i][j];
    delta = max(delta,math.fabs(one));

one = 1 + niter - oneint
delta = max(delta,math.fabs(one))
one = BIGOFFSET+1 + niter - oneint64
delta = max(delta,math.fabs(one))
one = FLOATOFFSET+1 + niter - onefloat
delta = max(delta,math.fabs(one))
one = BIGOFFSET+FLOATOFFSET+1 + niter - onedouble
delta = max(delta,math.fabs(one))

vdelta = np.zeros(1,np.float)
alldelta = np.zeros(1,np.float)
vdelta[0] = delta
world.Allreduce(vdelta,alldelta,op=MPI.MAX)
delta = alldelta[0]

# stats

if me == 0:
  print "Nsend,nrecv:",cs.extract(1),cs.extract(2)
  print "Initial/final first int:",1,sint[0]
  print "Initial/final last int:",ids[nlocal-1],sint[nlocal-1]
  print "Initial/final first int64:",BIGOFFSET+1,sint64[0]
  print "Initial/final last int64:",BIGOFFSET+ids[nlocal-1],sint64[nlocal-1]
  print "Initial/final first float:",FLOATOFFSET+1,sfloat[0]
  print "Initial/final last float:",FLOATOFFSET+ids[nlocal-1],sfloat[nlocal-1]
  print "Initial/final first double:",BIGOFFSET+FLOATOFFSET+1,sdouble[0]
  print "Initial/final last double:",BIGOFFSET+FLOATOFFSET+ids[nlocal-1], \
    sdouble[nlocal-1]
  print "Initial/final first array:",BIGOFFSET+FLOATOFFSET+1,sarray[0][0]
  print "Initial/final last array:",BIGOFFSET+FLOATOFFSET+ids[nlocal-1], \
    sarray[nlocal-1][nper-1]
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

if mpifree: world.Free()
del cs
