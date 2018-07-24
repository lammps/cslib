// serial server example in C++

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
  // command-line args

  if (argc != 3) {
    printf("Syntax: server mode N\n");
    printf("  mode = file or zmq\n");
    printf("  N = length of data vectors\n");
    exit(1);
  }

  char *mode = argv[1];
  int nlen = atoi(argv[2]);
  
  if (nlen < 1) error("Nlen < 1");

  // setup messaging
  
  CSlib *cs;

  if (strcmp(mode,"file") == 0)
    cs = new CSlib(1,mode,"tmp.couple",NULL);
  else if (strcmp(mode,"zmq") == 0)
    cs = new CSlib(1,mode,"*:5555",NULL);
  else error("Invalid mode");

  // setup data

  int nper = 3;

  int *sint = (int *) smalloc(nlen*sizeof(int));
  int64_t *sint64 = (int64_t *) smalloc(nlen*sizeof(int64_t));
  float *sfloat = (float *) smalloc(nlen*sizeof(float));
  double *sdouble = (double *) smalloc(nlen*sizeof(double));
  double **sarray = smalloc2d(nlen,nper);

  char *sstring = (char *) "world";
  
  // endless server loop 

  int nfield;
  int *fieldID,*fieldtype,*fieldlen;

  while (1) {

    // recv message from client
    // msgID = 0 = all-done message

    int msgID = cs->recv(nfield,fieldID,fieldtype,fieldlen);
    if (msgID == 0) break;
    
    int *rint = (int *) cs->unpack(1);
    int64_t *rint64 = (int64_t *) cs->unpack(2);
    float *rfloat = (float *) cs->unpack(3);
    double *rdouble = (double *) cs->unpack(4);
    double *rarray = (double *) cs->unpack(5);
    int oneint = cs->unpack_int(6);
    int64_t oneint64 = cs->unpack_int64(7);
    float onefloat = cs->unpack_float(8);
    double onedouble = cs->unpack_double(9);
    char *rstring = cs->unpack_string(10);

    // increment values

    int m = 0;
    for (int i = 0; i < nlen; i++) {
      sint[i] = rint[i] + 1;
      sint64[i] = rint64[i] + 1;
      sfloat[i] = rfloat[i] + 1.0;
      sdouble[i] = rdouble[i] + 1.0;
      for (int j = 0; j < nper; j++)
	sarray[i][j] = rarray[m++] + 1.0;
    }
	  
    // send message to client

    cs->send(1,10);
    cs->pack(1,1,nlen,sint);
    cs->pack(2,2,nlen,sint64);
    cs->pack(3,3,nlen,sfloat);
    cs->pack(4,4,nlen,sdouble);
    cs->pack(5,4,nper*nlen,&sarray[0][0]);
    cs->pack_int(6,oneint+1);
    cs->pack_int64(7,oneint64+1);
    cs->pack_float(8,onefloat+1);
    cs->pack_double(9,onedouble+1);
    cs->pack_string(10,sstring);
  }

  // final reply to client

  cs->send(0,0);

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
