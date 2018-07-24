#include "mpi.h"
#include "stdlib.h"
#include "stdio.h"

int main(int narg, char **args)
{
  MPI_Init(&narg,&args);
  MPI_Comm world = MPI_COMM_WORLD;

  int me,nprocs;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);

  // read single arg = N messages

  if (narg != 2) {
    printf("Syntax: client_mpi_one N\n");
    exit(1);
  }
  int n = atoi(args[1]);

  // this is the client application
  // setup communication with server application

  MPI_Comm onlyclient;
  MPI_Comm_split(world,0,me,&onlyclient);
  MPI_Comm twocomm = world;
  world = onlyclient;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);
  int otherroot = nprocs;

  // proc 0 sends N times with tag 0 to server proc 0
  // recv reply from server and bcast to other procs in world
  // final send with tag 1 to exit

  MPI_Status status;
  int send,recv;

  send = 100;
  for (int i = 0; i < n; i++) {
    if (me == 0) {
      MPI_Send(&send,1,MPI_INT,otherroot,0,twocomm);
      MPI_Recv(&recv,1,MPI_INT,otherroot,0,twocomm,&status);
    }
    MPI_Bcast(&recv,1,MPI_INT,0,world);
    if (me == 0) printf("Client value me %d i %d recv %d\n",me,i,recv);
    MPI_Barrier(world);
  }

  if (me == 0) {
    MPI_Send(&send,1,MPI_INT,otherroot,1,twocomm);
    MPI_Recv(&recv,1,MPI_INT,otherroot,0,twocomm,&status);
  }

  // clean up

  MPI_Comm_free(&world);
  MPI_Finalize();
}
