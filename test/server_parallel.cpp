// parallel server example in C++

#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cslib.h"

using namespace CSLIB_NS;

void *smalloc(int);
void sfree(void *);
double **smalloc2d(int, int);
void sfree2d(double **);
void error(const char *);

// main program

int main(int argc, char **argv)
{
  // MPI setup

  MPI_Init(&argc,&argv);
  MPI_Comm world = MPI_COMM_WORLD;
  int me,nprocs;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);

  // command-line args

  if (argc != 3) {
    if (me == 0) {
      printf("Syntax: mpirun -np P server_parallel mode N\n");
      printf("  mode = file or zmq or mpi/one or mpi/two\n");
      printf("  N = length of data vectors\n");
    }
    MPI_Abort(world,1);
  }

  char *mode = argv[1];
  int nlen = atoi(argv[2]);
  
  if (nlen < 1) error("Nlen < 1");

  // setup messaging

  CSlib *cs;
  int mpifree = 0;

  if (strcmp(mode,"file") == 0)
    cs = new CSlib(1,mode,"tmp.couple",&world);
  else if (strcmp(mode,"zmq") == 0)
    cs = new CSlib(1,mode,"*:5555",&world);
  else if (strcmp(mode,"mpi/one") == 0) {
    mpifree = 1;
    MPI_Comm onlyserver;
    MPI_Comm_split(world,1,me,&onlyserver);
    MPI_Comm_rank(onlyserver,&me);
    MPI_Comm_size(onlyserver,&nprocs);
    MPI_Comm both = world;
    world = onlyserver;
    cs = new CSlib(1,mode,&both,&world);
  } else if (strcmp(mode,"mpi/two") == 0)
    cs = new CSlib(1,mode,(char *) "tmp.couple",&world);
  else error("Invalid mode");

  // setup data

  int nper = 3;
  int nlocal = nlen/nprocs;
  if (me < nlen % nprocs) nlocal++;

  int *ids = (int *) smalloc(nlocal*sizeof(int));
  int m = 0;
  for (int i = me; i < nlen; i += nprocs) ids[m++] = i+1;

  int *sint = (int *) smalloc(nlocal*sizeof(int));
  int64_t *sint64 = (int64_t *) smalloc(nlocal*sizeof(int64_t));
  float *sfloat = (float *) smalloc(nlocal*sizeof(float));
  double *sdouble = (double *) smalloc(nlocal*sizeof(double));
  double **sarray = smalloc2d(nlocal,nper);

  int *rint = (int *) smalloc(nlocal*sizeof(int));
  int64_t *rint64 = (int64_t *) smalloc(nlocal*sizeof(int64_t));
  float *rfloat = (float *) smalloc(nlocal*sizeof(float));
  double *rdouble = (double *) smalloc(nlocal*sizeof(double));
  double **rarray = smalloc2d(nlocal,nper);

  char *sstring = (char *) "world";

  // endless server loop 

  int nfield;
  int *fieldID,*fieldtype,*fieldlen;

  while (1) {

    // recv message from client
    // msgID = 0 = all-done message

    int msgID = cs->recv(nfield,fieldID,fieldtype,fieldlen);
    if (msgID == 0) break;

    cs->unpack_parallel(1,nlocal,ids,1,rint);
    cs->unpack_parallel(2,nlocal,ids,1,rint64);
    cs->unpack_parallel(3,nlocal,ids,1,rfloat);
    cs->unpack_parallel(4,nlocal,ids,1,rdouble);
    cs->unpack_parallel(5,nlocal,ids,nper,&rarray[0][0]);
    int oneint = cs->unpack_int(6);
    int64_t oneint64 = cs->unpack_int64(7);
    float onefloat = cs->unpack_float(8);
    double onedouble = cs->unpack_double(9);
    char *rstring = cs->unpack_string(10);

    // increment values

    for (int i = 0; i < nlocal; i++) {
      sint[i] = rint[i] + 1;
      sint64[i] = rint64[i] + 1;
      sfloat[i] = rfloat[i] + 1.0;
      sdouble[i] = rdouble[i] + 1.0;
      for (int j = 0; j < 3; j++)
	sarray[i][j] = rarray[i][j] + 1.0;
    }
	   
    // send message to client

    cs->send(1,10);
    cs->pack_parallel(1,1,nlocal,ids,1,sint);
    cs->pack_parallel(2,2,nlocal,ids,1,sint64);
    cs->pack_parallel(3,3,nlocal,ids,1,sfloat);
    cs->pack_parallel(4,4,nlocal,ids,1,sdouble);
    cs->pack_parallel(5,4,nlocal,ids,nper,&sarray[0][0]);
    cs->pack_int(6,oneint+1);
    cs->pack_int64(7,oneint64+1);
    cs->pack_float(8,onefloat+1);
    cs->pack_double(9,onedouble+1);
    cs->pack_string(10,sstring);
  }

  // final reply to client

  cs->send(0,0);

  // clean-up

  sfree(ids);

  sfree(sint);
  sfree(sint64);
  sfree(sfloat);
  sfree(sdouble);
  sfree2d(sarray);

  sfree(rint);
  sfree(rint64);
  sfree(rfloat);
  sfree(rdouble);
  sfree2d(rarray);

  delete cs;
  if (mpifree) MPI_Comm_free(&world);
  MPI_Finalize();
}

// utility functions
  
void *smalloc(int nbytes)
{
  if (nbytes == 0) return NULL;
  void *ptr = malloc(nbytes);
  if (ptr == NULL) {
    char str[128];
    sprintf(str,"Failed to allocate %d bytes",nbytes);
    error(str);
  }
  return ptr;
}

void sfree(void *ptr)
{
  if (ptr == NULL) return;
  free(ptr);
}

double **smalloc2d(int n1, int n2)
{
  int nbytes = n1*n2 * sizeof(double);
  double *data = (double *) smalloc(nbytes);
  nbytes = n1 * sizeof(double *);
  double **array = (double **) smalloc(nbytes);
  
  int n = 0;
  for (int i = 0; i < n1; i++) {
    array[i] = &data[n];
    n += n2;
  }
  return array;
}
 
void sfree2d(double **array)
{
  if (array == NULL) return;
  free(array[0]);
  free(array);
}

void error(const char *str)
{
  printf("ERROR: %s\n",str);
  exit(1);
}
