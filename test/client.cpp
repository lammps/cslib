// serial client example in C++

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#include "cslib.h"

using namespace CSLIB_NS;

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
  struct XYZ {
    double x,y,z;
  };

  // command-line args

  if (argc != 4) {
    printf("Syntax: client mode N Niter\n");
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
  
  CSlib *cs;

  if (strcmp(mode,"file") == 0)
    cs = new CSlib(0,mode,"tmp.couple",NULL);
  else if (strcmp(mode,"zmq") == 0)
    cs = new CSlib(0,mode,"localhost:5555",NULL);
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

    cs->send(1,10);
    cs->pack(1,1,nlen,sint);
    cs->pack(2,2,nlen,sint64);
    cs->pack(3,3,nlen,sfloat);
    cs->pack(4,4,nlen,sdouble);
    cs->pack(5,4,nper*nlen,&sarray[0][0]);
    cs->pack_int(6,oneint);
    cs->pack_int64(7,oneint64);
    cs->pack_float(8,onefloat);
    cs->pack_double(9,onedouble);
    cs->pack_string(10,sstring);

    // recv message from server

    int msgID = cs->recv(nfield,fieldID,fieldtype,fieldlen);
    int *rint = (int *) cs->unpack(1);
    int64_t *rint64 = (int64_t *) cs->unpack(2);
    float *rfloat = (float *) cs->unpack(3);
    double *rdouble = (double *) cs->unpack(4);
    double *rarray = (double *) cs->unpack(5);
    oneint = cs->unpack_int(6);
    oneint64 = cs->unpack_int64(7);
    onefloat = cs->unpack_float(8);
    onedouble = cs->unpack_double(9);
    onedouble = cs->unpack_double(9);
    rstring = cs->unpack_string(10);

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

  cs->send(0,0);
  int msgID = cs->recv(nfield,fieldID,fieldtype,fieldlen);

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

  printf("Nsend,nrecv: %d %d\n",cs->nsend,cs->nrecv);
  printf("Initial/final first int: %d %d\n",1,sint[0]);
  printf("Initial/final last int: %d %d\n",nlen,sint[nlen-1]);
  printf("Initial/final first int64: %lld %lld\n",BIGOFFSET+1,sint64[0]);
  printf("Initial/final last int64: %lld %lld\n",BIGOFFSET+nlen,sint64[nlen-1]);
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
  
  delete cs;
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
