// parallel client example in C

#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cslib_wrap.h"

#define BIGOFFSET 3000000000
#define FLOATOFFSET 0.5

#define MAX(A,B) ((A) > (B) ? (A) : (B))

void *smalloc(int);
void sfree(void *);
double **smalloc2d(int, int);
void sfree2d(double **);
void error_all(const char *);
void error_one(const char *);

MPI_Comm world;
int me,nprocs;

// main program

int main(int argc, char **argv)
{
  // MPI setup

  MPI_Init(&argc,&argv);
  world = MPI_COMM_WORLD;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);

  // command-line args

  if (argc != 4) {
    if (me == 0) {
      printf("Syntax: mpirun -np P client_parallel_c mode N Niter\n");
      printf("  mode = file or zmq or mpi/one or mpi/two\n");
      printf("  N = length of data vectors\n");
      printf("  Niter = # of iterations\n");
    }
    exit(1);
  }

  char *mode = argv[1];
  int nlen = atoi(argv[2]);
  int niter = atoi(argv[3]);
  
  if (nlen < 1) error_all("Nlen < 1");
  if (niter < 1) error_all("Niter < 1");

  // setup messaging
  
  void *cs;
  int mpifree = 0;

  if (strcmp(mode,"file") == 0)
    cslib_open(0,mode,"tmp.couple",&world,&cs);
  else if (strcmp(mode,"zmq") == 0)
    cslib_open(0,mode,"localhost:5555",&world,&cs);
  else if (strcmp(mode,"mpi/one") == 0) {
    mpifree = 1;
    MPI_Comm onlyclient;
    MPI_Comm_split(world,0,me,&onlyclient);
    MPI_Comm_rank(onlyclient,&me);
    MPI_Comm_size(onlyclient,&nprocs);
    MPI_Comm both = world;
    world = onlyclient;
    cslib_open(0,mode,&both,&world,&cs);
  } else if (strcmp(mode,"mpi/two") == 0)
    cslib_open(0,mode,"tmp.couple",&world,&cs);
  else error_all("Invalid mode");

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

  for (int i = 0; i < nlocal; i++) {
    sint[i] = ids[i];
    sint64[i] = BIGOFFSET + ids[i];
    sfloat[i] = FLOATOFFSET + ids[i];
    sdouble[i] = BIGOFFSET + FLOATOFFSET + ids[i];
    for (int j = 0; j < nper; j++)
      sarray[i][j] = sdouble[i];
  }

  int oneint = 1;
  int64_t oneint64 = BIGOFFSET + 1;
  float onefloat = FLOATOFFSET + 1;
  double onedouble = BIGOFFSET + FLOATOFFSET + 1;

  char *sstring = (char *) "hello";
  char *rstring;

  // client loop 

  int nfield;
  int *fieldID,*fieldtype,*fieldlen;

  double tstart = MPI_Wtime();
  
  for (int iter = 0; iter < niter; iter++) {

    // send message to server

    cslib_send(cs,1,10);
    cslib_pack_parallel(cs,1,1,nlocal,ids,1,sint);
    cslib_pack_parallel(cs,2,2,nlocal,ids,1,sint64);
    cslib_pack_parallel(cs,3,3,nlocal,ids,1,sfloat);
    cslib_pack_parallel(cs,4,4,nlocal,ids,1,sdouble);
    cslib_pack_parallel(cs,5,4,nlocal,ids,nper,&sarray[0][0]);
    cslib_pack_int(cs,6,oneint);
    cslib_pack_int64(cs,7,oneint64);
    cslib_pack_float(cs,8,onefloat);
    cslib_pack_double(cs,9,onedouble);
    cslib_pack_string(cs,10,sstring);

    // recv message from server

    int msgID = cslib_recv(cs,&nfield,&fieldID,&fieldtype,&fieldlen);
    cslib_unpack_parallel(cs,1,nlocal,ids,1,rint);
    cslib_unpack_parallel(cs,2,nlocal,ids,1,rint64);
    cslib_unpack_parallel(cs,3,nlocal,ids,1,rfloat);
    cslib_unpack_parallel(cs,4,nlocal,ids,1,rdouble);
    cslib_unpack_parallel(cs,5,nlocal,ids,nper,&rarray[0][0]);
    oneint = cslib_unpack_int(cs,6);
    oneint64 = cslib_unpack_int64(cs,7);
    onefloat = cslib_unpack_float(cs,8);
    onedouble = cslib_unpack_double(cs,9);
    rstring = cslib_unpack_string(cs,10);

    // copy server data to client data

    for (int i = 0; i < nlocal; i++) {
      sint[i] = rint[i];
      sint64[i] = rint64[i];
      sfloat[i] = rfloat[i];
      sdouble[i] = rdouble[i];
      for (int j = 0; j < nper; j++)
	sarray[i][j] = rarray[i][j];
    }
  }

  double tstop = MPI_Wtime();

  // all-done message to server

  cslib_send(cs,0,0);
  int msgID = cslib_recv(cs,&nfield,&fieldID,&fieldtype,&fieldlen);

  // error check
  
  double one;
  double delta = 0.0;

  for (int i; i < nlocal; i++) {
    one = ids[i] + niter - sint[i];
    delta = MAX(delta,fabs(one));
    one = BIGOFFSET + ids[i] + niter - sint64[i];
    delta = MAX(delta,fabs(one));
    one = FLOATOFFSET + ids[i] + niter - sfloat[i];
    delta = MAX(delta,fabs(one));
    one = BIGOFFSET+FLOATOFFSET + ids[i] + niter - sdouble[i];
    delta = MAX(delta,fabs(one));
    for (int j = 0; j < nper; j++) {
      one = BIGOFFSET+FLOATOFFSET + ids[i] + niter - sarray[i][j];
      delta = MAX(delta,fabs(one));
    }
  }

  one = 1 + niter - oneint;
  delta = MAX(delta,fabs(one));
  one = BIGOFFSET+1 + niter - oneint64;
  delta = MAX(delta,fabs(one));
  one = FLOATOFFSET+1 + niter - onefloat;
  delta = MAX(delta,fabs(one));
  one = BIGOFFSET+FLOATOFFSET+1 + niter - onedouble;
  delta = MAX(delta,fabs(one));

  double alldelta;
  MPI_Allreduce(&delta,&alldelta,1,MPI_DOUBLE,MPI_MAX,world);
  delta = alldelta;

  // stats

  int nsend = cslib_extract(cs,1);
  int nrecv = cslib_extract(cs,2);

  if (me == 0) {
    printf("Nsend,nrecv: %d %d\n",nsend,nrecv);
    printf("Initial/final first int: %d %d\n",1,sint[0]);
    printf("Initial/final last int: %d %d\n",ids[nlocal-1],sint[nlocal-1]);
    printf("Initial/final first int64: %ld %lld\n",BIGOFFSET+1,sint64[0]);
    printf("Initial/final last int64: %ld %lld\n",
           BIGOFFSET+ids[nlocal-1],sint64[nlocal-1]);
    printf("Initial/final first float: %g %g\n",FLOATOFFSET+1,sfloat[0]);
    printf("Initial/final last float: %g %g\n",
           FLOATOFFSET+ids[nlocal-1],sfloat[nlocal-1]);
    printf("Initial/final first double: %15.12g %15.12g\n",
           BIGOFFSET+FLOATOFFSET+1,sdouble[0]);
    printf("Initial/final last double: %15.12g %15.12g\n",
           BIGOFFSET+FLOATOFFSET+ids[nlocal-1],sdouble[nlocal-1]);
    printf("Initial/final first array: %15.12g %15.12g\n",
           BIGOFFSET+FLOATOFFSET+1,sarray[0][0]);
    printf("Initial/final last array: %15.12g %15.12g\n",
           BIGOFFSET+FLOATOFFSET+ids[nlocal-1],sarray[nlocal-1][nper-1]);
    printf("Initial/final int scalar: %d %d\n",1,oneint);
    printf("Initial/final int64 scalar: %lld %lld\n",BIGOFFSET+1,oneint64);
    printf("Initial/final float scalar: %g %g\n",FLOATOFFSET+1,onefloat);
    printf("Initial/final double scalar: %15.12g %15.12g\n",
           BIGOFFSET+FLOATOFFSET+1,onedouble);
    printf("Initial/final string: %s %s\n",sstring,rstring);
    
    printf("Max error for any value: %g\n",delta);
    printf("CPU time: %g\n",tstop-tstart);
    int datumsize = sizeof(int) + sizeof(int64_t) + sizeof(float) +
      sizeof(double) + nper*sizeof(double);
    double bandwidth = 2.0 * nlen * datumsize / (tstop-tstart) / 1024/1024;
    printf("Message bandwidth: %g MB/sec\n",bandwidth);
  }

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
  
  cslib_close(cs);
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
    error_one(str);
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

void error_all(const char *str)
{
  if (me == 0) printf("ERROR: %s\n",str);
  MPI_Abort(world,1);
}

void error_one(const char *str)
{
  printf("ERROR: %s\n",str);
  MPI_Abort(world,1);
}
