#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

////////////////////////////////////////////////////////////////////////////////
// Event ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Event *Event_Create(uint32_t cmd, void *payload, uint32_t plen) {
  Event *e;
  e = calloc(1, sizeof(Event));
  e->cmd = cmd;
  e->prev = e->next = NULL;
  if (plen > 0 && payload != NULL) {
    e->payload = malloc(plen);
    memcpy(e->payload, payload, plen);
    e->payload_len = plen;
  }
  return e;
}

int Event_Free(Event *e) {
  if (e->payload != NULL) {
    free(e->payload);
  }
  free(e);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Event Queue /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static inline void _eventq_lock(EventQueue *q) { pthread_mutex_lock(&q->mutex); }
static inline void _eventq_unlock(EventQueue *q) { pthread_mutex_unlock(&q->mutex); } 

EventQueue *EventQueue_Init(void) {
  EventQueue *q;
  q = calloc(1, sizeof(EventQueue));
  pthread_mutex_init(&q->mutex, NULL);
  pthread_cond_init(&q->cond, NULL);
  q->head = q->tail = NULL;
  return q;
}

int EventQueue_Free(EventQueue *q) {
  pthread_mutex_lock(&q->mutex);
  pthread_cond_signal(&q->cond);
  pthread_cond_destroy(&q->cond);
  pthread_mutex_unlock(&q->mutex);
  pthread_mutex_destroy(&q->mutex);
  //@FIXME: queued event should be freed.
  free(q);
  return 0;  
}

int EventQueue_Wait(EventQueue *q) {
  _eventq_lock(q);
  if (q->head == NULL) {
    _eventq_unlock(q);
    return 1;
  }
  pthread_cond_wait(&q->cond, &q->mutex);
  _eventq_unlock(q);
  return 0;
}

int EventQueue_Push(EventQueue *q, Event *e) {
  _eventq_lock(q);
  if (q->tail == NULL) {
    q->tail = q->head = e;
    e->prev = e->next = e;
  } else {
    q->tail->next = e;
    e->prev = q->tail;
    q->tail = e;
  }
  _eventq_unlock(q);
  pthread_cond_signal(&q->cond);
  return 0;
}

Event *EventQueue_Pop(EventQueue *q) {
  Event *e = NULL;
  _eventq_lock(q);
  if (q->head == NULL) {
    _eventq_unlock(q);
    return NULL;
  }
  if (q->head->next == q->head) { //only one event.
    e = q->head;
    q->head = q->tail = NULL;
    _eventq_unlock(q);
    return e;
  }
  e = q->head;
  q->head = e->next;
  q->head->prev = q->head;
  e->next = e->prev = e;
  _eventq_unlock(q);
  return e;
}

////////////////////////////////////////////////////////////////////////////////
// Thread //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int Thread_Start(Thread *t) {
  int ret;
  t->eventq = EventQueue_Init();
  ret = pthread_create(&t->thread, NULL, t->start, t);
  t->isStarted = 1;
  printf(">> thread %s created. ret = %d\n", t->name, ret);
  return 0;
}

int Thread_Stop(Thread *t) {
  void *pret;
  t->stop = 1;
  EventQueue_Push(t->eventq, Event_Create(ADMIN_THREAD_STOP, NULL, 0));
  pthread_join(t->thread, &pret);
  EventQueue_Free(t->eventq);
  t->stop = 0;
  t->isStarted = 0;
  return 0;
}

/*
  Example of a thread implementation.
void *thread(void *p) {
  Thread *self = (Thread*)p;
  EventQueue *q = self->eventq;
  Event *e;
  while(!self->stop) {
    EventQueue_Wait(q);
    e = EventQueue_Pop(q);
    if (e == NULL) continue;
    if (e->cmd == ADMIN_THREAD_STOP) {
      print("thread exit");
      Event_Free(e);
      break;
    }
  }
  print(">> thread ends.\n");
  self->isStarted = 0;
  return NULL;
}
thread example = {
 .name = "example_thread",
 .start = thread,
 .isStarted = 0,
 .stop = 0,
};
*/
