#include "mpi.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"

int main(int narg, char **args)
{
  MPI_Init(&narg,&args);
  MPI_Comm world = MPI_COMM_WORLD;

  int me,nprocs;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);

  // no args

  if (narg != 1) {
    printf("Syntax: server_mpi\n");
    exit(1);
  }

  // this is the server application
  // setup communication with client application
  // create inter-communicator

  MPI_Comm intercomm; 
  char port[MPI_MAX_PORT_NAME]; 

  MPI_Open_port(MPI_INFO_NULL,port); 

  if (me == 0) {
    printf("Server name: %s\n",port);
    FILE *fp = fopen("tmp.couple","w");
    fprintf(fp,"%s",port);
    fclose(fp);
  }

  MPI_Comm_accept(port,MPI_INFO_NULL,0,world,&intercomm); 
  if (me == 0) printf("SERVER comm accept\n");

  // proc 0 recv from client proc 0 in infinite loop
  // if recv tag = 0, bcast recv value to other procs in world
  // send back 2x the received value to client
  // if recv tag = 1, exit

  MPI_Status status;
  int send,recv,tag;

  int i = 0;
  while (1) {
    if (me == 0) {
      MPI_Recv(&recv,1,MPI_INT,0,MPI_ANY_TAG,intercomm,&status);
      tag = status.MPI_TAG;
    }
    MPI_Bcast(&recv,1,MPI_INT,0,world);
    MPI_Bcast(&tag,1,MPI_INT,0,world);
    if (tag == 0) {
      if (me == 0) printf("Server value me %d i %d recv %d\n",me,i,recv);
      i++;
      send = 2*recv;
      if (me == 0) MPI_Send(&send,1,MPI_INT,0,0,intercomm);
    } else if (tag == 1) break;
    MPI_Barrier(world);
  }
  
  if (me == 0) MPI_Send(&send,1,MPI_INT,0,0,intercomm);

  // clean up

  MPI_Comm_free(&intercomm);
  MPI_Close_port(port);
  MPI_Finalize();
}
