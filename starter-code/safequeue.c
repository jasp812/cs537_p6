#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "safequeue.h"


struct safequeue *q;

void create_queue(int qsize) {
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->space, NULL);
    pthread_cond_init(&q->fill, NULL);


    pthread_mutex_lock(&q->lock);
    q = malloc(sizeof(struct safequeue) + qsize * sizeof(struct element));
    q->capacity = qsize;
    q->size = 0;
    pthread_mutex_unlock(&q->lock);
}

void add_work(struct element req) {
    pthread_mutex_lock(&q->lock);
    while(q->size == q->capacity) {
        pthread_cond_wait(&q->space, &q->lock);
    }
    insert(req);
    pthread_cond_signal(&q->fill);
    pthread_mutex_unlock(&q->lock);
}

struct element get_work() {
    pthread_mutex_lock(&q->lock);
    while(q->size == 0) {
        pthread_cond_wait(&q->fill, &q->lock);
    }
    struct element ret = extractMax();
    pthread_cond_signal(&q->space);
    pthread_mutex_unlock(&q->lock);
    return ret;
}

struct element get_work_nonblocking() {
    pthread_mutex_lock(&q->lock);
    if(q->size == 0) {
        struct element ret = {.delay = "", .method = "", .path = "", .prio = -1};
        printf("Queue is currently empty\n");
        return ret;
    }
    struct element ret = extractMax();
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
    while (i > 0 && q->reqs[parent(i)].prio < q->reqs[i].prio) {
 
        // Swap parent and current node
        struct element temp = q->reqs[parent(i)];
        q->reqs[parent(i)] = q->reqs[i];
        q->reqs[i] = temp;

 
        // Update i to parent of i
        i = parent(i);
    }
}
 
// Function to shift down the node in
// order to maintain the heap property
void shiftDown(int i)
{
    int maxIndex = i;
 
    // Left Child
    int l = leftChild(i);
 
    if (l <= q->size && q->reqs[l].prio > q->reqs[maxIndex].prio) {
        maxIndex = l;
    }
 
    // Right Child
    int r = rightChild(i);
 
    if (r <= q->size && q->reqs[r].prio > q->reqs[maxIndex].prio) {
        maxIndex = r;
    }
 
    // If i not same as maxIndex
    if (i != maxIndex) {
        struct element temp = q->reqs[maxIndex];
        q->reqs[maxIndex] = q->reqs[i];
        q->reqs[i] = temp;
        shiftDown(maxIndex);
    }
}
 
// Function to insert a new element
// in the Binary Heap
void insert(struct element p)
{
    

    q->size = q->size + 1;
    q->reqs[q->size] = p;
 
    // Shift Up to maintain heap property
    shiftUp(q->size);
}
 
// Function to extract the element with
// maximum priority
struct element extractMax()
{
    
    struct element result = q->reqs[0];
 
    // Replace the value at the root
    // with the last leaf
    q->reqs[0] = q->reqs[q->size];
    q->size = q->size - 1;
 
    // Shift down the replaced element
    // to maintain the heap property
    shiftDown(0);
    return result;
}
 
// Function to change the priority
// of an element
void changePriority(int i, struct element p)
{
    
    struct element oldp = q->reqs[i];
    q->reqs[i] = p;
 
    if (p.prio > oldp.prio) {
        shiftUp(i);
    }
    else {
        shiftDown(i);
    }
}
 
// Function to get value of the current
// maximum element
struct element getMax()
{
    
    return q->reqs[0];
}
 
// Function to remove the element
// located at given index
// void remove(int i)
// {
    
//     q->reqs[i] = getMax();
//     q->reqs[i].prio++;
 
//     // Shift the node to the root
//     // of the heap
//     shiftUp(i);
 
//     // Extract the node
//     extractMax();
// }