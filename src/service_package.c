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
queue_pop(struct queue *q, void *result) {
	if(q->head == q->tail) {
		return 1;
	}
	memcpy(result,q->buffer + q->head * q->sz, q->sz);
	q->head++;
	if(q->head >= q->cap)
		q->head = 0;
	return 0;
}

static void
queue_push(struct queue *q, const void *value) {
	void * slot = q->buffer + q->tail * q->sz;
	++q->tail;
	if(q->tail >= q->cap)
		q->tail = 0;
	if(q->head == q->tail) {
		//full
		assert(q->sz > 0);
		int cap = q->cap * 2;
		char * tmp = skynet_malloc(cap * q->sz);
		int i;
		int head = q->head;
		for(i = 0; i < q->cap; i++) {
			memcpy(tmp + i * q->sz, q->buffer + head * q->sz, q->sz);
			++head;
			if(head >= q->cap) {
				head = 0;
			}
		}
		skynet_free(q->buffer);
		q->head = 0;
		slot = tmp + (q->cap-1) * q->sz;
		q->tail = q->cap;
		q->tail = cap;
		q->buffer = tmp;
	}
	memcpy(slot,value,q->sz);
}

static int
queue_size(struct queue *q) {
	if(q->head > q->tail) {
		return q->tail + q->cap - q->head;
	}
	return q->tail - q->head;
}

static void
command(struct skynet_context *ctx, struct package *P, int session, uint32_t source, const char *msg, size_t sz) {
	switch (msg[0]) {
	case 'R':
		// request a package
		if(P->closed) {
			skynet_send(ctx,0,source,PTYPE_ERROR,session,NULL,0);
			break;
		}
		if(!queue_empty(&P->response)) {
			assert(queue_empty(&P->request));
			struct response resp;
			queue_pop(&P->response, &resp);
			skynet_send(ctx,0,source,PTYPE_RESPONSE | PTYPE_TAG_DONTCOPY, session, resp.msg, resp.sz);
		} else {
			struct request req;
			req.source = source;
			req.session = session;
			queue_push(&P->request , &req);
		}
		break;
	}
}

static void
socket_message(struct skynet_context *ctx, struct package *P, const struct skynet_socket_message * smsg) {
	switch(smsg->type) {
	case SKYNET_SOCKET_TYPE_CONNECT:
		if(P->init == 0 && smsg->id == P->fd) {
			skynet_send(ctx,0,P->manager,PTYPE_TEXT,0,"SUCC",4);		
		}
	}
}

static int
message_handler(struct skynet_context * ctx, void *ud, int type, int session, uint32_t source, const void * msg, size_t sz) {
	struct package *P = ud;
	switch (type) {
	case PTYPE_TEXT:
		command(ctx,P,session, source, msg, sz);
		break;
	case PTYPE_SOCKET:
		socket_message(ctx,P,msg);
		break;
	}
}


/// service 的几个函数
struct package *
package_create(void) {
	struct package * P = skynet_malloc(sizeof(*P));
	memset(P,0,sizeof(*P));
	P->heartbeat = -1;
	P->uncomplete_sz = -1;
	queue_init(&P->request, sizeof(struct request));
	queue_init(&P->response,sizeof(struct response));
	return P;
}

void
package_release(struct package *P) {
	queue_exit(&P->request);
	queue_exit(&P->response);
	skynet_free(P);
}

int
package_init(struct package * P, struct skynet_context *ctx, const char * param) {
	int n = sscanf(param ? param : "", "%u %d", &P->manager, &P->fd);
	if(n != 2 || P->manager == 0 || P->fd == 0) {
		skynet_error(ctx,"Invalid param [%d]",param);
		return 1;
	}
	skynet_socket_start(ctx,P->fd);
	skynet_socket_nodelay(ctx,P->fd);
	

	skynet_callback(ctx,P,message_handler);

	return 0;
}














