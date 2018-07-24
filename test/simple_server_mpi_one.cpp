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

  // no args

  if (narg != 1) {
    printf("Syntax: server_mpi_two \n");
    exit(1);
  }

  // this is the server application
  // setup communication with client application

  MPI_Comm onlyserver;
  MPI_Comm_split(world,1,me,&onlyserver);
  MPI_Comm twocomm = world;
  world = onlyserver;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);
  int otherroot = 0;

  // cannot do this
  //int nprocs_other;
  //MPI_Comm_size(intercomm,&nprocs_other);
  //printf("SERVER sees %d procs in CLIENT\n",nprocs_other);

  // proc 0 recv from client proc 0 in infinite loop
  // if recv tag = 0, bcast recv value to other procs in world
  // send back 2x the received value to client
  // if recv tag = 1, exit

  MPI_Status status;
  int send,recv,tag;

  int i = 0;
  while (1) {
    if (me == 0) {
      MPI_Recv(&recv,1,MPI_INT,otherroot,MPI_ANY_TAG,twocomm,&status);
      tag = status.MPI_TAG;
    }
    MPI_Bcast(&recv,1,MPI_INT,0,world);
    MPI_Bcast(&tag,1,MPI_INT,0,world);
    if (tag == 0) {
      if (me == 0) printf("Server value me %d i %d recv %d\n",me,i,recv);
      i++;
      send = 2*recv;
      if (me == 0) MPI_Send(&send,1,MPI_INT,otherroot,0,twocomm);
    } else if (tag == 1) break;
    MPI_Barrier(world);
  }
  
  if (me == 0) MPI_Send(&send,1,MPI_INT,0,0,twocomm);

  // clean up

  MPI_Comm_free(&world);
  MPI_Finalize();
}
