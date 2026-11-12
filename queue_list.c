#include "queue.h"

#include <stdlib.h>

/*
 * File implémentée avec une liste simplement chaînée.
 *
 * front pointe sur le premier élément (celui qui sortira en premier),
 * rear pointe sur le dernier élément (celui qui vient d'entrer).
 */
typedef struct QueueNode {
    Item value;
    struct QueueNode *next;
} QueueNode;

struct Queue {
    QueueNode *front;
    QueueNode *rear;
    size_t size;
};

Queue *queue_create(void)
{
    Queue *queue = malloc(sizeof *queue);
    if (queue == NULL) {
        return NULL;
    }

    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

void queue_destroy(Queue *queue)
{
    if (queue == NULL) {
        return;
    }

    QueueNode *current = queue->front;
    while (current != NULL) {
        QueueNode *next = current->next;
        free(current);
        current = next;
    }

    free(queue);
}

DsStatus queue_enqueue(Queue *queue, Item x)
{
    if (queue == NULL) {
        return DS_BADARG;
    }

    QueueNode *node = malloc(sizeof *node);
    if (node == NULL) {
        return DS_NOMEM;
    }

    node->value = x;
    node->next = NULL;

    if (queue->rear == NULL) {
        queue->front = node;
        queue->rear = node;
    } else {
        queue->rear->next = node;
        queue->rear = node;
    }

    queue->size++;
    return DS_OK;
}

DsStatus queue_dequeue(Queue *queue, Item *out)
{
    if (queue == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (queue->front == NULL) {
        return DS_EMPTY;
    }

    QueueNode *node = queue->front;
    *out = node->value;
    queue->front = node->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(node);
    queue->size--;
    return DS_OK;
}

DsStatus queue_front(const Queue *queue, Item *out)
{
    if (queue == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (queue->front == NULL) {
        return DS_EMPTY;
    }

    *out = queue->front->value;
    return DS_OK;
}

bool queue_is_empty(const Queue *queue)
{
    return queue == NULL || queue->front == NULL;
}

size_t queue_size(const Queue *queue)
{
    return queue == NULL ? 0 : queue->size;
}
