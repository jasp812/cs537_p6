#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

typedef struct safequeue{
    int size;
    int capacity;
    pthread_mutex_t lock;
    pthread_cond_t space;
    pthread_cond_t fill;
    struct http_request reqs[];
} safequeue;

extern void create_queue(int);
extern void add_work(struct http_request);
extern struct http_request get_work();
extern struct http_request get_work_nonblocking();
extern int parent(int);
extern int leftChild(int);
extern int rightChild(int);
extern void shiftUp(int);
extern void shiftDown(int);
extern void insert(struct http_request);
extern struct http_request extractMax();
extern void changePriority(int, struct http_request);
extern struct http_request getMax();
// extern void remove(int);

#endif