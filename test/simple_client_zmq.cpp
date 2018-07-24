//  Hello World client, based on test code in ZMQ tutorial

#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main (int argc, char **argv)
{
  // read single arg = N messages

  if (argc != 2) {
    printf("Syntax: simple_client_zmq N\n");
    exit(1);
  }
  int n = atoi(argv[1]);

  printf ("Connecting to hello world server\n");
  void *context = zmq_ctx_new ();
  void *requester = zmq_socket (context, ZMQ_REQ);
  zmq_connect (requester, "tcp://localhost:5555");

  int request_nbr;
  char buffer [10];

  for (request_nbr = 0; request_nbr < n; request_nbr++) {
    printf ("Sending Hello %d\n", request_nbr);
    zmq_send (requester, "Hello", 5, 0);
    zmq_recv (requester, buffer, 10, 0);
    printf ("Received World %d\n", request_nbr);
  }

  zmq_send (requester, "DONE", 4, 0);
  zmq_recv (requester, buffer, 10, 0);

  zmq_close (requester);
  zmq_ctx_destroy (context);
  return 0;
}
