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

  // read single arg = N messages

  if (narg != 2) {
    printf("Syntax: client_mpi_two N\n");
    exit(1);
  }
  int n = atoi(args[1]);

  // this is the client application
  // setup communication with server application
  // create inter-communicator

  MPI_Comm intercomm; 
  char port[MPI_MAX_PORT_NAME];

  if (me == 0) {
    FILE *fp = NULL;
    while (!fp) {
      fp = fopen("tmp.couple","r");
      if (!fp) sleep(1);
    }
    fgets(port,MPI_MAX_PORT_NAME,fp);
    printf("Client port: %s\n",port);
    fclose(fp);
  }
  
  MPI_Bcast(port,MPI_MAX_PORT_NAME,MPI_CHAR,0,world);
  MPI_Comm_connect(port,MPI_INFO_NULL,0,world,&intercomm); 
  if (me == 0) printf("CLIENT comm connect\n");
  if (me == 0) unlink("tmp.couple");

  // proc 0 sends N times with tag 0 to server proc 0
  // recv reply from server and bcast to other procs in world
  // final send with tag 1 to exit

  MPI_Status status;
  int send,recv;

  send = 100;
  for (int i = 0; i < n; i++) {
    if (me == 0) {
      MPI_Send(&send,1,MPI_INT,0,0,intercomm);
      MPI_Recv(&recv,1,MPI_INT,0,0,intercomm,&status);
    }
    MPI_Bcast(&recv,1,MPI_INT,0,world);
    if (me == 0) printf("Client value me %d i %d recv %d\n",me,i,recv);
    MPI_Barrier(world);
  }

  if (me == 0) {
    MPI_Send(&send,1,MPI_INT,0,1,intercomm);
    MPI_Recv(&recv,1,MPI_INT,0,0,intercomm,&status);
  }

  // clean up

  MPI_Comm_free(&intercomm);
  MPI_Close_port(port);
  MPI_Finalize();
}
