#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include "ds_status.h"

/* TAD File (FIFO). Type opaque.
   Deux implémentations interchangeables : queue_circ.c et queue_list.c. */
typedef struct Queue Queue;

Queue   *queue_create(void);                        /* NULL si échec mémoire  */
void     queue_destroy(Queue *q);                   /* accepte NULL           */
bool     queue_is_empty(const Queue *q);
size_t   queue_size(const Queue *q);
DsStatus queue_enqueue(Queue *q, Item x);           /* DS_OK ou DS_NOMEM      */
DsStatus queue_dequeue(Queue *q, Item *out);        /* DS_EMPTY si file vide  */
DsStatus queue_front(const Queue *q, Item *out);    /* DS_EMPTY si file vide  */

#endif /* QUEUE_H */