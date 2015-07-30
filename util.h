#include <stdint.h>
#include <pthread.h>

#define ADMIN_THREAD_STOP 0x98765432
typedef struct Event_s {
  uint32_t cmd;
  void *payload;
  uint32_t payload_len;
  struct Event_s *prev, *next;
} Event;

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  /* pop out <- head ....... tail <- append */
  Event *head;
  Event *tail;
} EventQueue;

typedef struct {
  const char *name;
  pthread_t thread;
  EventQueue *eventq;
  void *(*start)(void*);
  char isStarted;
  char stop;
} Thread;


Event *Event_Create(uint32_t cmd, void *payload, uint32_t plen);
int Event_Free(Event *e);

EventQueue *EventQueue_Init(void);
int EventQueue_Free(EventQueue *q);
int EventQueue_Wait(EventQueue *q);
int EventQueue_Push(EventQueue *q, Event *e);
Event *EventQueue_Pop(EventQueue *q);

int Thread_Start(Thread *t);
int Thread_Stop(Thread *t);
