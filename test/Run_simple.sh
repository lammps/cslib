#!/bin/sh

# build of simple apps, no CSlib required

make simple

# simple runs

echo %%% Run simple zmq %%%
simple_client_zmq 10 &
simple_server_zmq

echo %%% Run simple mpi/one %%%
mpirun -np 3 simple_client_mpi_one 10 : -np 4 simple_server_mpi_one

echo %%% Run simple mpi/two %%%
mpirun -np 3 simple_client_mpi_two 10 &
mpirun -np 4 simple_server_mpi_two
