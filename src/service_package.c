#include "skynet.h"
#include "skynet_socket.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define TIMEOUT "1000" // 10s

struct response {
	size_t sz;
	void * msg;
};

struct request {
	uint32_t source;
	int session;
};

struct queue {
	int cap;
	int sz;
	int head;
	int tail;
	char * buffer;
};

struct package {
	 uint32_t   manager;
	 int fd;
	 int heartbeat;
	 int recv;
	 int init;
	 int closed;

	 int header_sz;
	 uint8_t header[2];
	 int uncomplete_sz;
	 struct response uncomplete;

	 struct queue request;
	 struct queue response;
};

static void
queue_init(struct queue *q, int sz) {
	q->head = 0;
	q->tail = 0;
	q->sz = sz;
	q->cap = 4;
	q->buffer = skynet_malloc(q->cap * q->sz);
}

static void 
queue_exit(struct queue * q) {
	skynet_free(q->buffer);
	q->buffer = NULL;
}

static int
queue_empty(struct queue *q) {
	return q->head == q->tail;
}

static int
queue_pop(struct queue *q,   void* result) {
	if(q->head == q->tail) {
		return 1;
	}
	memcpy(result, q->buffer + q->head * )
}