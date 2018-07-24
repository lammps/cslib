#!/bin/env python

import sys,math
import numpy as np
import ctypes
from mpi4py import MPI
from cslib import CSlib

# MPI setup

world = MPI.COMM_WORLD
me = world.rank
nprocs = world.size

# command-line args

if len(sys.argv) != 4:
  print "Syntax: mpirun -np P python server_parallel.py mode N type"
  print "  mode = file or zmq or mpi/one or mpi/two"
  print "  N = length of data vectors"
  print "  type = 1/2/3 = list, Numpy, ctypes"
  sys.exit(1)

mode = sys.argv[1]
nlen = int(sys.argv[2])
dtype = int(sys.argv[3])

if nlen < 1: error("Nlen < 1")
if dtype < 1 or dtype > 3: error("Type < 1 or type > 3")

# setup messaging

mpifree = 0
         
if mode == "file":
  cs = CSlib(1,mode,"tmp.couple",world)
elif mode == "zmq":
  cs = CSlib(1,mode,"*:5555",world)
elif mode == "mpi/one":
  mpifree = 1
  onlyserver = world.Split(1,me)
  me = onlyserver.rank
  nprocs = onlyserver.size
  both = world
  world = onlyserver
  cs = CSlib(1,mode,both,world)
elif mode == "mpi/two":
  cs = CSlib(1,mode,"tmp.couple",world)
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
  sfloat = nlocal*[0.0]
  sdouble = nlocal*[0.0]
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

sstring = "world"

# endless server loop

while 1:

  # recv message from client
  # msgID = 0 = all-done message

  msgID,nfield,fieldID,fieldtype,fieldlen = cs.recv()
  if msgID == 0: break

  rint = cs.unpack_parallel(1,nlocal,ids,1,tflag=dtype)
  rint64 = cs.unpack_parallel(2,nlocal,ids,1,tflag=dtype)
  rfloat = cs.unpack_parallel(3,nlocal,ids,1,tflag=dtype)
  rdouble = cs.unpack_parallel(4,nlocal,ids,1,tflag=dtype)
  rvec = cs.unpack_parallel(5,nlocal,ids,nper,tflag=dtype)
  if dtype == 1:
    rarray = [rvec[i:i+nper] for i in range(0,len(rvec),nper)] 
  elif dtype == 2:
    rarray = np.reshape(rvec,(nlocal,nper))
  oneint = cs.unpack_int(6);
  oneint64 = cs.unpack_int64(7);
  onefloat = cs.unpack_float(8);
  onedouble = cs.unpack_double(9);
  rstring = cs.unpack_string(10);

  # increment values

  m = 0
  for i in range(nlocal):
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
  cs.pack_parallel(1,1,nlocal,ids,1,sint)
  cs.pack_parallel(2,2,nlocal,ids,1,sint64)
  cs.pack_parallel(3,3,nlocal,ids,1,sfloat)
  cs.pack_parallel(4,4,nlocal,ids,1,sdouble)
  if dtype == 1:
    svec = sum(sarray,[])
    cs.pack_parallel(5,4,nlocal,ids,nper,svec)
  else: cs.pack_parallel(5,4,nlocal,ids,nper,sarray)
  cs.pack_int(6,oneint+1)
  cs.pack_int64(7,oneint64+1)
  cs.pack_float(8,onefloat+1)
  cs.pack_double(9,onedouble+1)
  cs.pack_string(10,sstring)

# final reply to client

cs.send(0,0)

# clean-up

if mpifree: world.Free()
del cs
