#!/bin/sh

# serial build of lib and test apps

cd ../src; make shlib_serial; cd ../test
make serial

# serial runs with C++

echo %%% Run serial C++ file %%%
client file 10 2 &
server file 10

sleep 1

echo %%% Run serial C++ zmq %%%
client zmq 10 2 &
server zmq 10

sleep 1

# serial runs with C

echo %%% Run serial C file %%%
client_c file 10 2 &
server_c file 10

sleep 1

echo %%% Run serial C zmq %%%
client_c zmq 10 2 &
server_c zmq 10

sleep 1

# serial runs with Fortran

echo %%% Run serial Fortran file %%%
client_f90 file 10 2 &
server_f90 file 10

sleep 1

echo %%% Run serial Fortran zmq %%%
client_f90 zmq 10 2 &
server_f90 zmq 10

sleep 1

# serial runs with Python (3 dtype options)

echo %%% Run serial Python file %%%
python client.py file 10 2 1 &
python server.py file 10 1

sleep 1

echo %%% Run serial Python zmq %%%
python client.py zmq 10 2 1 &
python server.py zmq 10 1

sleep 1

echo %%% Run serial Python file %%%
python client.py file 10 2 2 &
python server.py file 10 2

sleep 1

echo %%% Run serial Python zmq %%%
python client.py zmq 10 2 2 &
python server.py zmq 10 2

sleep 1

echo %%% Run serial Python file %%%
python client.py file 10 2 3 &
python server.py file 10 3

sleep 1

echo %%% Run serial Python zmq %%%
python client.py zmq 10 2 3 &
python server.py zmq 10 3
