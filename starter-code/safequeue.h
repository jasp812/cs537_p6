#include <pthread.h>
#include <sys/types.h>
#include "proxyserver.h"



typedef struct safequeue{
    int size;
    int capacity;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t space = PTHREAD_COND_INITIALIZER;
    pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
    struct http_request reqs[];
};

extern void create_queue();
extern void add_work(struct http_request);
extern struct http_request get_work();
extern struct http_request get_work_nonblocking();