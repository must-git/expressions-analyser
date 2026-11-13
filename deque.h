#ifndef DEQUE_H
#define DEQUE_H

#include <stdbool.h>
#include <stddef.h>

#include "ds_status.h"

/* TAD Deque (file à double extrémité).
   Deux implémentations interchangeables : deque_circ.c et deque_dll.c. */
typedef struct Deque Deque;

Deque   *deque_create(void);                        /* NULL si échec mémoire  */
void     deque_destroy(Deque *deque);               /* accepte NULL           */
bool     deque_is_empty(const Deque *deque);
size_t   deque_size(const Deque *deque);
DsStatus deque_push_front(Deque *deque, Item x);    /* DS_OK ou DS_NOMEM      */
DsStatus deque_push_back(Deque *deque, Item x);     /* DS_OK ou DS_NOMEM      */
DsStatus deque_pop_front(Deque *deque, Item *out);  /* DS_EMPTY si vide       */
DsStatus deque_pop_back(Deque *deque, Item *out);   /* DS_EMPTY si vide       */
DsStatus deque_front(const Deque *deque, Item *out);/* DS_EMPTY si vide       */
DsStatus deque_back(const Deque *deque, Item *out); /* DS_EMPTY si vide       */

#endif /* DEQUE_H */
