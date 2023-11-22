#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

struct element{
    char *method;
    char *path;
    char *delay;
    int prio;
    int fd;
};

typedef struct safequeue{
    int size;
    int capacity;
    pthread_mutex_t lock;
    pthread_cond_t space;
    pthread_cond_t fill;
    struct element reqs[];
} safequeue;

extern void create_queue(int);
extern struct safequeue get_queue();
extern int add_work(struct element);
extern struct element get_work();
extern struct element get_work_nonblocking();
extern int parent(int);
extern int leftChild(int);
extern int rightChild(int);
extern void shiftUp(int);
extern void shiftDown(int);
extern void insert(struct element);
extern struct element extractMax();
extern void changePriority(int, struct element);
extern struct element getMax();
// extern void remove(int);

#endif