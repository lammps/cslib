#!/bin/sh

# parallel build of lib and test apps

cd ../src; make shlib; cd ../test
make parallel
make f90

# parallel runs with C++

echo %%% Run parallel 3/4 C++ file %%%
mpirun -np 3 client_parallel file 10 2 &
mpirun -np 4 server_parallel file 10

sleep 1

echo %%% Run parallel 3/4 C++ zmq %%%
mpirun -np 3 client_parallel zmq 10 2 &
mpirun -np 4 server_parallel zmq 10

sleep 1

echo %%% Run parallel 3/4 C++ mpi/one %%%
mpirun -np 3 client_parallel mpi/one 10 2 : \
-np 4 server_parallel mpi/one 10

sleep 1

echo %%% Run parallel 3/4 C++ mpi/two %%%
mpirun -np 3 client_parallel mpi/two 10 2 &
mpirun -np 4 server_parallel mpi/two 10

sleep 1

# parallel runs with C

echo %%% Run parallel 3/4 C file %%%
mpirun -np 3 client_parallel_c file 10 2 &
mpirun -np 4 server_parallel_c file 10

sleep 1

echo %%% Run parallel 3/4 C zmq %%%
mpirun -np 3 client_parallel_c zmq 10 2 &
mpirun -np 4 server_parallel_c zmq 10

sleep 1

echo %%% Run parallel 3/4 C mpi/one %%%
mpirun -np 3 client_parallel_c mpi/one 10 2 : \
-np 4 server_parallel_c mpi/one 10

sleep 1

echo %%% Run parallel 3/4 C mpi/two %%%
mpirun -np 3 client_parallel_c mpi/two 10 2 &
mpirun -np 4 server_parallel_c mpi/two 10

sleep 1

# parallel runs with Fortran

echo %%% Run parallel 3/4 Fortran file %%%
mpirun -np 3 client_parallel_f90 file 10 2 &
mpirun -np 4 server_parallel_f90 file 10

sleep 1

echo %%% Run parallel 3/4 Fortran zmq %%%
mpirun -np 3 client_parallel_f90 zmq 10 2 &
mpirun -np 4 server_parallel_f90 zmq 10

sleep 1

echo %%% Run parallel 3/4 Fortran mpi/one %%%
mpirun -np 3 client_parallel_f90 mpi/one 10 2 : \
-np 4 server_parallel_f90 mpi/one 10

sleep 1

echo %%% Run parallel 3/4 Fortran mpi/two %%%
mpirun -np 3 client_parallel_f90 mpi/two 10 2 &
mpirun -np 4 server_parallel_f90 mpi/two 10

sleep 1

# parallel runs with Python (3 dtype options)

echo %%% Run parallel 3/4 Python file dtype=1 %%%
mpirun -np 3 python client_parallel.py file 10 2 1 &
mpirun -np 4 python server_parallel.py file 10 1

sleep 1

echo %%% Run parallel 3/4 Python zmq dtype=1 %%%
mpirun -np 3 python client_parallel.py zmq 10 2 1 &
mpirun -np 4 python server_parallel.py zmq 10 1

sleep 1

echo %%% Run parallel 3/4 Python mpi/one dtype=1 %%%
mpirun -np 3 python client_parallel.py mpi/one 10 2 1 : \
-np 4 python server_parallel.py mpi/one 10 1

sleep 1

echo %%% Run parallel 3/4 Python mpi/two dtype=1 %%%
mpirun -np 3 python client_parallel.py mpi/two 10 2 1 &
mpirun -np 4 python server_parallel.py mpi/two 10 1

sleep 1

echo %%% Run parallel 3/4 Python file dtype=2 %%%
mpirun -np 3 python client_parallel.py file 10 2 2 &
mpirun -np 4 python server_parallel.py file 10 2

sleep 1

echo %%% Run parallel 3/4 Python zmq dtype=2 %%%
mpirun -np 3 python client_parallel.py zmq 10 2 2 &
mpirun -np 4 python server_parallel.py zmq 10 2

sleep 1

echo %%% Run parallel 3/4 Python mpi/one dtype=2 %%%
mpirun -np 3 python client_parallel.py mpi/one 10 2 2 : \
-np 4 python server_parallel.py mpi/one 10 2

sleep 1

echo %%% Run parallel 3/4 Python mpi/two dtype=2 %%%
mpirun -np 3 python client_parallel.py mpi/two 10 2 2 &
mpirun -np 4 python server_parallel.py mpi/two 10 2

sleep 1

echo %%% Run parallel 3/4 Python file dtype=3 %%%
mpirun -np 3 python client_parallel.py file 10 2 3 &
mpirun -np 4 python server_parallel.py file 10 3

sleep 1

echo %%% Run parallel 3/4 Python zmq dtype=3 %%%
mpirun -np 3 python client_parallel.py zmq 10 2 3 &
mpirun -np 4 python server_parallel.py zmq 10 3

sleep 1

echo %%% Run parallel 3/4 Python mpi/one dtype=3 %%%
mpirun -np 3 python client_parallel.py mpi/one 10 2 3 : \
-np 4 python server_parallel.py mpi/one 10 3

sleep 1

echo %%% Run parallel 3/4 Python mpi/two dtype=3 %%%
mpirun -np 3 python client_parallel.py mpi/two 10 2 3 &
mpirun -np 4 python server_parallel.py mpi/two 10 3
