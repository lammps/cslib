// serial client example in C

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#include "cslib_wrap.h"

#define BIGOFFSET 3000000000
#define FLOATOFFSET 0.5

#define MAX(A,B) ((A) > (B) ? (A) : (B))

void *smalloc(int);
void sfree(void *);
double **smalloc2d(int, int);
void sfree2d(double **);
void error(const char *);
double timer();

// main program

int main(int argc, char **argv)
{
  // command-line args

  if (argc != 4) {
    printf("Syntax: client_c mode N Niter\n");
    printf("  mode = file or zmq\n");
    printf("  N = length of data vectors\n");
    printf("  Niter = # of iterations\n");
    exit(1);
  }

  char *mode = argv[1];
  int nlen = atoi(argv[2]);
  int niter = atoi(argv[3]);
  
  if (nlen < 1) error("Nlen < 1");
  if (niter < 1) error("Niter < 1");

  // setup messaging
  
  void *cs;
  
  if (strcmp(mode,"file") == 0)
    cslib_open(0,mode,"tmp.couple",NULL,&cs);
  else if (strcmp(mode,"zmq") == 0)
    cslib_open(0,mode,"localhost:5555",NULL,&cs);
  else error("Invalid mode");

  // setup data

  int nper = 3;

  int *sint = (int *) smalloc(nlen*sizeof(int));
  int64_t *sint64 = (int64_t *) smalloc(nlen*sizeof(int64_t));
  float *sfloat = (float *) smalloc(nlen*sizeof(float));
  double *sdouble = (double *) smalloc(nlen*sizeof(double));
  double **sarray = smalloc2d(nlen,nper);
  
  for (int i = 0; i < nlen; i++) {
    sint[i] = i+1;
    sint64[i] = BIGOFFSET + i+1;
    sfloat[i] = FLOATOFFSET + i+1;
    sdouble[i] = BIGOFFSET + FLOATOFFSET + i+1;
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

  double tstart = timer();
  
  for (int iter = 0; iter < niter; iter++) {

    // send message to server

    cslib_send(cs,1,10);
    cslib_pack(cs,1,1,nlen,sint);
    cslib_pack(cs,2,2,nlen,sint64);
    cslib_pack(cs,3,3,nlen,sfloat);
    cslib_pack(cs,4,4,nlen,sdouble);
    cslib_pack(cs,5,4,nper*nlen,&sarray[0][0]);
    cslib_pack_int(cs,6,oneint);
    cslib_pack_int64(cs,7,oneint64);
    cslib_pack_float(cs,8,onefloat);
    cslib_pack_double(cs,9,onedouble);
    cslib_pack_string(cs,10,sstring);

    // recv message from server

    int msgID = cslib_recv(cs,&nfield,&fieldID,&fieldtype,&fieldlen);
    int *rint = (int *) cslib_unpack(cs,1);
    int64_t *rint64 = (int64_t *) cslib_unpack(cs,2);
    float *rfloat = (float *) cslib_unpack(cs,3);
    double *rdouble = (double *) cslib_unpack(cs,4);
    double *rarray = (double *) cslib_unpack(cs,5);
    oneint = cslib_unpack_int(cs,6);
    oneint64 = cslib_unpack_int64(cs,7);
    onefloat = cslib_unpack_float(cs,8);
    onedouble = cslib_unpack_double(cs,9);
    rstring = cslib_unpack_string(cs,10);

    // copy server data to client data

    int m = 0;
    for (int i = 0; i < nlen; i++) {
      sint[i] = rint[i];
      sint64[i] = rint64[i];
      sfloat[i] = rfloat[i];
      sdouble[i] = rdouble[i];
      for (int j = 0; j < nper; j++)
	sarray[i][j] = rarray[m++];
    }
  }

  double tstop = timer();

  // all-done message to server

  cslib_send(cs,0,0);
  int msgID = cslib_recv(cs,&nfield,&fieldID,&fieldtype,&fieldlen);

  // error check
  
  double one;
  double delta = 0.0;

  for (int i; i < nlen; i++) {
    one = i+1 + niter - sint[i];
    delta = MAX(delta,fabs(one));
    one = BIGOFFSET + i+1 + niter - sint64[i];
    delta = MAX(delta,fabs(one));
    one = FLOATOFFSET + i+1 + niter - sfloat[i];
    delta = MAX(delta,fabs(one));
    one = BIGOFFSET+FLOATOFFSET + i+1 + niter - sdouble[i];
    delta = MAX(delta,fabs(one));
    for (int j = 0; j < nper; j++) {
      one = BIGOFFSET+FLOATOFFSET + i+1 + niter - sarray[i][j];
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

  // stats

  int nsend = cslib_extract(cs,1);
  int nrecv = cslib_extract(cs,2);

  printf("Nsend,nrecv: %d %d\n",nsend,nrecv);
  printf("Initial/final first int: %d %d\n",1,sint[0]);
  printf("Initial/final last int: %d %d\n",nlen,sint[nlen-1]);
  printf("Initial/final first int64: %ld %lld\n",BIGOFFSET+1,sint64[0]);
  printf("Initial/final last int64: %ld %lld\n",BIGOFFSET+nlen,sint64[nlen-1]);
  printf("Initial/final first float: %g %g\n",FLOATOFFSET+1,sfloat[0]);
  printf("Initial/final last float: %g %g\n",FLOATOFFSET+nlen,sfloat[nlen-1]);
  printf("Initial/final first double: %15.12g %15.12g\n",
	 BIGOFFSET+FLOATOFFSET+1,sdouble[0]);
  printf("Initial/final last double: %15.12g %15.12g\n",
	 BIGOFFSET+FLOATOFFSET+nlen,sdouble[nlen-1]);
  printf("Initial/final first array: %15.12g %15.12g\n",
	 BIGOFFSET+FLOATOFFSET+1,sarray[0][0]);
  printf("Initial/final last array: %15.12g %15.12g\n",
	 BIGOFFSET+FLOATOFFSET+nlen,sarray[nlen-1][nper-1]);
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

  // clean-up

  sfree(sint);
  sfree(sint64);
  sfree(sfloat);
  sfree(sdouble);
  sfree2d(sarray);
  
  cslib_close(cs);
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
 
double timer()
{
  double time;
  struct timeval tv;

  gettimeofday(&tv,NULL);
  time = 1.0 * tv.tv_sec + 1.0e-6 * tv.tv_usec;
  return time;
}

