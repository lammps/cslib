# Makefile for CSlib tests

SHELL = /bin/sh

# compiler/linker settings

CC =		g++
F90 =		gfortran
MPICC = 	mpicxx
MPIF90 = 	mpif90
CCFLAGS =	-g -O3 -I../src
LINKFLAGS =	-g
LIB =		-L../src -lcsmpi
LIBNOMPI =	-L../src -lcsnompi

# targets

help:
	@echo 'make all           build all the CSlib and simple apps'
	@echo 'make serial        build the CSlib serial apps, except fortran'
	@echo 'make parallel      build the CSlib parallel apps, except fortran'
	@echo 'make f90           build the CSlib fortran apps, serial & parallel'
	@echo 'make simple        build the simple apps'
	@echo 'make pair          build a single pair of c/s apps (list below)'
	@echo 'make clean         remove all *.o files'
	@echo 'make clean-all     remove *.o and exe files'
	@echo ''
	@echo 'single pairs of test apps:'
	@echo '  C++: serial_c++, parallel_c++'
	@echo '  C:   serial_c, parallel_c'
	@echo '  F90: serial_f90, parallel_f90'
	@echo 'single pairs of simple apps:'
	@echo '  simple_zmq, simple_mpi_one, simple_mpi_two'

all:		serial parallel f90
serial:		serial_c++ serial_c
parallel: 	parallel_c++ parallel_c
f90:		serial_f90 parallel_f90
simple:		simple_zmq simple_mpi_one simple_mpi_two

serial_c++:
	$(CC) $(CCFLAGS) -c client.cpp
	$(CC) client.o $(LINKFLAGS) $(LIBNOMPI) -o client
	$(CC) $(CCFLAGS) -c server.cpp
	$(CC) server.o $(LINKFLAGS) $(LIBNOMPI) -o server

serial_c:
	$(CC) $(CCFLAGS) -c client_c.cpp
	$(CC) client_c.o $(LINKFLAGS) $(LIBNOMPI) -o client_c
	$(CC) $(CCFLAGS) -c server_c.cpp
	$(CC) server_c.o $(LINKFLAGS) $(LIBNOMPI) -o server_c

serial_f90:
	$(F90) $(CCFLAGS) -c cslib_wrap.f90
	$(F90) $(CCFLAGS) -c client_f90.f90
	$(F90) client_f90.o cslib_wrap.o $(LINKFLAGS) $(LIBNOMPI) -o client_f90
	$(F90) $(CCFLAGS) -c server_f90.f90
	$(F90) server_f90.o cslib_wrap.o $(LINKFLAGS) $(LIBNOMPI) -o server_f90

parallel_c++:
	$(MPICC) $(CCFLAGS) -c client_parallel.cpp
	$(MPICC) client_parallel.o $(LINKFLAGS) $(LIB) -o client_parallel
	$(MPICC) $(CCFLAGS) -c server_parallel.cpp
	$(MPICC) server_parallel.o $(LINKFLAGS) $(LIB) -o server_parallel

parallel_c:
	$(MPICC) $(CCFLAGS) -c client_parallel_c.cpp
	$(MPICC) client_parallel_c.o $(LINKFLAGS) $(LIB) -o client_parallel_c
	$(MPICC) $(CCFLAGS) -c server_parallel_c.cpp
	$(MPICC) server_parallel_c.o $(LINKFLAGS) $(LIB) -o server_parallel_c

parallel_f90:
	$(F90) $(CCFLAGS) -c cslib_wrap.f90
	$(MPIF90) $(CCFLAGS) -c client_parallel_f90.f90
	$(MPIF90) client_parallel_f90.o cslib_wrap.o \
		$(LINKFLAGS) $(LIB) -o client_parallel_f90
	$(MPIF90) $(CCFLAGS) -c server_parallel_f90.f90
	$(MPIF90) server_parallel_f90.o cslib_wrap.o \
		$(LINKFLAGS) $(LIB) -o server_parallel_f90

simple_zmq:
	$(CC) $(CCFLAGS) simple_client_zmq.cpp -lzmq -o simple_client_zmq
	$(CC) $(CCFLAGS) simple_server_zmq.cpp -lzmq -o simple_server_zmq

simple_mpi_one:
	$(MPICC) $(CCFLAGS) simple_client_mpi_one.cpp -o simple_client_mpi_one
	$(MPICC) $(CCFLAGS) simple_server_mpi_one.cpp -o simple_server_mpi_one

simple_mpi_two:
	$(MPICC) $(CCFLAGS) simple_client_mpi_two.cpp -o simple_client_mpi_two
	$(MPICC) $(CCFLAGS) simple_server_mpi_two.cpp -o simple_server_mpi_two

clean:
	@rm -f *.o

clean-all:
	@rm -f *.o client server client_c server_c client_f90 server_90 \
	client_parallel server_parallel client_parallel_c server_parallel_c \
	client_parallel_f90 server_parallel_f90 \
	simple_client_mpi_one simple_client_mpi_two simple_client_zmq \
	simple_server_mpi_one simple_server_mpi_two simple_server_zmq 
