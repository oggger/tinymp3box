#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"



void *fwalker_go(void *p) {
  Thread *self = (Thread*)p;
  EventQueue *q = self->eventq;
  Event *e;
  while(!self->stop) {
    EventQueue_Wait(q);
    e = EventQueue_Pop(q);
    if (e == NULL) continue;
    if (e->cmd == ADMIN_THREAD_STOP) {
      printf("thread exit");
      Event_Free(e);
      break;
    }
    printf("got event cmd=%08x\n", e->cmd);
    Event_Free(e);
  }
  printf(">> thread ends.\n");
  self->isStarted = 0;
  return NULL;
}

Thread fwalker_thread = {
 .name = "file_walker",
 .start = fwalker_go,
 .isStarted = 0,
 .stop = 0,
};


int main(int argc, char* argv[]) {
  char cmd[32];
  int ret;
  while(1) {
    printf("$> ");
    ret = scanf("%s", cmd);
    printf(">> cmd = %s\n", cmd);
    if (ret <=0 ) continue;
    
    if (!strcmp(cmd, "quit")) { printf("thanks.\n"); exit(0); }

    if (!strcmp(cmd, "fwalk")) {
      if (fwalker_thread.isStarted != 1)
	Thread_Start(&fwalker_thread);
    }

    if (!strcmp(cmd, "fwalk_end")) {
      Thread_Stop(&fwalker_thread);
    }

    if (!strcmp(cmd, "x")) {
      EventQueue_Push(fwalker_thread.eventq, Event_Create(rand()%256, NULL, 0));
    }
  }

  return 0;
}
