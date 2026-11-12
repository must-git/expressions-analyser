#include "queue.h"

#include <stdint.h>
#include <stdlib.h>

/*
 * File implémentée avec un tableau circulaire dynamique.
 *
 * Les éléments logiques sont :
 *   data[front], data[(front + 1) % capacity], ..., data[(front + size - 1) % capacity]
 */
#define QUEUE_INITIAL_CAPACITY 8

struct Queue {
    Item *data;
    size_t capacity;
    size_t size;
    size_t front;
};

static DsStatus queue_grow(Queue *queue)
{
    size_t new_capacity;

    if (queue->capacity > SIZE_MAX / 2) {
        return DS_NOMEM;
    }
    new_capacity = queue->capacity * 2;

    if (new_capacity > SIZE_MAX / sizeof *queue->data) {
        return DS_NOMEM;
    }

    Item *new_data = malloc(new_capacity * sizeof *new_data);
    if (new_data == NULL) {
        return DS_NOMEM;
    }

    for (size_t i = 0; i < queue->size; i++) {
        new_data[i] = queue->data[(queue->front + i) % queue->capacity];
    }

    free(queue->data);
    queue->data = new_data;
    queue->capacity = new_capacity;
    queue->front = 0;
    return DS_OK;
}

Queue *queue_create(void)
{
    Queue *queue = malloc(sizeof *queue);
    if (queue == NULL) {
        return NULL;
    }

    queue->data = malloc(QUEUE_INITIAL_CAPACITY * sizeof *queue->data);
    if (queue->data == NULL) {
        free(queue);
        return NULL;
    }

    queue->capacity = QUEUE_INITIAL_CAPACITY;
    queue->size = 0;
    queue->front = 0;
    return queue;
}

void queue_destroy(Queue *queue)
{
    if (queue == NULL) {
        return;
    }
    free(queue->data);
    free(queue);
}

DsStatus queue_enqueue(Queue *queue, Item x)
{
    if (queue == NULL) {
        return DS_BADARG;
    }

    if (queue->size == queue->capacity) {
        DsStatus status = queue_grow(queue);
        if (status != DS_OK) {
            return status;
        }
    }

    queue->data[(queue->front + queue->size) % queue->capacity] = x;
    queue->size++;
    return DS_OK;
}

DsStatus queue_dequeue(Queue *queue, Item *out)
{
    if (queue == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (queue->size == 0) {
        return DS_EMPTY;
    }

    *out = queue->data[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    if (queue->size == 0) {
        queue->front = 0;
    }
    return DS_OK;
}

DsStatus queue_front(const Queue *queue, Item *out)
{
    if (queue == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (queue->size == 0) {
        return DS_EMPTY;
    }

    *out = queue->data[queue->front];
    return DS_OK;
}

bool queue_is_empty(const Queue *queue)
{
    return queue == NULL || queue->size == 0;
}

size_t queue_size(const Queue *queue)
{
    return queue == NULL ? 0 : queue->size;
}
