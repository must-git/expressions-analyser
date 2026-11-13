#include "deque.h"

#include <stdlib.h>

/*
 * Deque implémentée avec une liste doublement chaînée.
 *
 * head pointe sur l'élément de tête, tail sur l'élément de queue.
 * Invariants :
 *   - head == NULL si et seulement si tail == NULL ;
 *   - head->prev == NULL et tail->next == NULL ;
 *   - pour tout nœud x, x->next->prev == x et x->prev->next == x.
 */
typedef struct DequeNode {
    Item value;
    struct DequeNode *prev;
    struct DequeNode *next;
} DequeNode;

struct Deque {
    DequeNode *head;
    DequeNode *tail;
    size_t size;
};

Deque *deque_create(void)
{
    Deque *deque = malloc(sizeof *deque);
    if (deque == NULL) {
        return NULL;
    }

    deque->head = NULL;
    deque->tail = NULL;
    deque->size = 0;
    return deque;
}

void deque_destroy(Deque *deque)
{
    if (deque == NULL) {
        return;
    }

    DequeNode *current = deque->head;
    while (current != NULL) {
        DequeNode *next = current->next;
        free(current);
        current = next;
    }

    free(deque);
}

DsStatus deque_push_front(Deque *deque, Item x)
{
    if (deque == NULL) {
        return DS_BADARG;
    }

    DequeNode *node = malloc(sizeof *node);
    if (node == NULL) {
        return DS_NOMEM;
    }

    node->value = x;
    node->prev = NULL;
    node->next = deque->head;

    if (deque->head == NULL) {
        deque->tail = node;
    } else {
        deque->head->prev = node;
    }
    deque->head = node;
    deque->size++;
    return DS_OK;
}

DsStatus deque_push_back(Deque *deque, Item x)
{
    if (deque == NULL) {
        return DS_BADARG;
    }

    DequeNode *node = malloc(sizeof *node);
    if (node == NULL) {
        return DS_NOMEM;
    }

    node->value = x;
    node->prev = deque->tail;
    node->next = NULL;

    if (deque->tail == NULL) {
        deque->head = node;
    } else {
        deque->tail->next = node;
    }
    deque->tail = node;
    deque->size++;
    return DS_OK;
}

DsStatus deque_pop_front(Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->head == NULL) {
        return DS_EMPTY;
    }

    DequeNode *node = deque->head;
    *out = node->value;
    deque->head = node->next;

    if (deque->head == NULL) {
        deque->tail = NULL;
    } else {
        deque->head->prev = NULL;
    }

    free(node);
    deque->size--;
    return DS_OK;
}

DsStatus deque_pop_back(Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->tail == NULL) {
        return DS_EMPTY;
    }

    DequeNode *node = deque->tail;
    *out = node->value;
    deque->tail = node->prev;

    if (deque->tail == NULL) {
        deque->head = NULL;
    } else {
        deque->tail->next = NULL;
    }

    free(node);
    deque->size--;
    return DS_OK;
}

DsStatus deque_front(const Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->head == NULL) {
        return DS_EMPTY;
    }

    *out = deque->head->value;
    return DS_OK;
}

DsStatus deque_back(const Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->tail == NULL) {
        return DS_EMPTY;
    }

    *out = deque->tail->value;
    return DS_OK;
}

bool deque_is_empty(const Deque *deque)
{
    return deque == NULL || deque->head == NULL;
}

size_t deque_size(const Deque *deque)
{
    return deque == NULL ? 0 : deque->size;
}
