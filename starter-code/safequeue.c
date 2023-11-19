#include <pthread.h>
#include <sys/types.h>
#include "proxyserver.h"
#include "safequeue.h"
#include <stdio.h>
#include <stdlib.h>

struct safequeue *q;

void create_queue() {
    pthread_mutex_lock(&q->lock);
    q = malloc(sizeof(struct safequeue) + MAX_SIZE * sizeof(struct http_request));
    q->capacity = MAX_SIZE;
    q->size = 0;
    pthread_mutex_unlock(&q->lock);
}

void add_work(struct http_request req) {
    pthread_mutex_lock(&q->lock);
    while(q->size == q->capacity) {
        pthread_cond_wait(&q->space, &q->lock);
    }
    insert(req);
    pthread_cond_signal(&q->fill);
    pthread_mutex_unlock(&q->lock);
}

struct http_request get_work() {
    pthread_mutex_lock(&q->lock);
    while(q->size == 0) {
        pthread_cond_wait(&q->fill, &q->lock);
    }
    struct http_request ret = extractMax();
    pthread_cond_signal(&q->space);
    pthread_mutex_unlock(&q->lock);
    return ret;
}

struct http_request get_work_nonblocking() {
    thread_mutex_lock(&q->lock);
    if(q->size == 0) {
        struct http_request ret = {.delay = "", .method = "", .path = "", .prio = -1};
        printf("Queue is currently empty\n");
        return ret;
    }
    struct http_request ret = extractMax();
    pthread_mutex_unlock(&q->lock);
    return ret;
}

// Function to return the index of the
// parent node of a given node
int parent(int i)
{
 
    return (i - 1) / 2;
}
 
// Function to return the index of the
// left child of the given node
int leftChild(int i)
{
 
    return ((2 * i) + 1);
}
 
// Function to return the index of the
// right child of the given node
int rightChild(int i)
{
 
    return ((2 * i) + 2);
}
 
// Function to shift up the node in order
// to maintain the heap property
void shiftUp(int i)
{
    struct http_request requests[] = q->reqs;
    while (i > 0 && requests[parent(i)].prio < requests[i].prio) {
 
        // Swap parent and current node
        swap(requests[parent(i)], requests[i]);
 
        // Update i to parent of i
        i = parent(i);
    }
}
 
// Function to shift down the node in
// order to maintain the heap property
void shiftDown(int i)
{
    int maxIndex = i;
    struct http_request requests[] = q->reqs;
 
    // Left Child
    int l = leftChild(i);
 
    if (l <= q->size && requests[l].prio > requests[maxIndex].prio) {
        maxIndex = l;
    }
 
    // Right Child
    int r = rightChild(i);
 
    if (r <= q->size && requests[r].prio > requests[maxIndex].prio) {
        maxIndex = r;
    }
 
    // If i not same as maxIndex
    if (i != maxIndex) {
        swap(requests[i], requests[maxIndex]);
        shiftDown(maxIndex);
    }
}
 
// Function to insert a new element
// in the Binary Heap
void insert(struct http_request p)
{
    struct http_request requests[] = q->reqs;

    q->size = q->size + 1;
    requests[q->size] = p;
 
    // Shift Up to maintain heap property
    shiftUp(q->size);
}
 
// Function to extract the element with
// maximum priority
struct http_request extractMax()
{
    struct http_request requests[] = q->reqs;
    struct http_request result = requests[0];
 
    // Replace the value at the root
    // with the last leaf
    requests[0] = requests[q->size];
    q->size = q->size - 1;
 
    // Shift down the replaced element
    // to maintain the heap property
    shiftDown(0);
    return result;
}
 
// Function to change the priority
// of an element
void changePriority(int i, struct http_request p)
{
    struct http_request requests[] = q->reqs;
    struct http_request oldp = requests[i];
    requests[i] = p;
 
    if (p.prio > oldp.prio) {
        shiftUp(i);
    }
    else {
        shiftDown(i);
    }
}
 
// Function to get value of the current
// maximum element
struct http_request getMax()
{
    struct http_request requests[] = q->reqs;
    return requests[0];
}
 
// Function to remove the element
// located at given index
void remove(int i)
{
    struct http_request requests[] = q->reqs;
    requests[i] = getMax();
    requests[i].prio + 1;
 
    // Shift the node to the root
    // of the heap
    shiftUp(i);
 
    // Extract the node
    extractMax();
}