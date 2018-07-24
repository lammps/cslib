//  Hello World server, based on test code in ZMQ tutorial

#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

int main (void)
{
  //  Socket to talk to clients
  void *context = zmq_ctx_new ();
  void *responder = zmq_socket (context, ZMQ_REP);
  int rc = zmq_bind (responder, "tcp://*:5555");
  assert (rc == 0);

  char buffer [10];

  while (1) {
    zmq_recv (responder, buffer, 10, 0);
    if (strncmp(buffer,"DONE",4) == 0) break;
    printf ("Received Hello\n");
    //sleep (1);          //  Do some 'work'
    zmq_send (responder, "World", 5, 0);
  }

  zmq_send (responder, "DONE", 4, 0);
  return 0;
}
