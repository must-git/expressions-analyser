#include "deque.h"

#include <stdint.h>
#include <stdlib.h>

/*
 * Deque implémentée avec un tableau circulaire dynamique.
 *
 * Les éléments logiques sont :
 *   data[front], data[(front + 1) % capacity], ...,
 *   data[(front + size - 1) % capacity]
 */
#define DEQUE_INITIAL_CAPACITY 8

struct Deque {
    Item *data;
    size_t capacity;
    size_t size;
    size_t front;
};

static DsStatus deque_grow(Deque *deque)
{
    size_t new_capacity;

    if (deque->capacity > SIZE_MAX / 2) {
        return DS_NOMEM;
    }
    new_capacity = deque->capacity * 2;

    if (new_capacity > SIZE_MAX / sizeof *deque->data) {
        return DS_NOMEM;
    }

    Item *new_data = malloc(new_capacity * sizeof *new_data);
    if (new_data == NULL) {
        return DS_NOMEM;
    }

    for (size_t i = 0; i < deque->size; i++) {
        new_data[i] = deque->data[(deque->front + i) % deque->capacity];
    }

    free(deque->data);
    deque->data = new_data;
    deque->capacity = new_capacity;
    deque->front = 0;
    return DS_OK;
}

Deque *deque_create(void)
{
    Deque *deque = malloc(sizeof *deque);
    if (deque == NULL) {
        return NULL;
    }

    deque->data = malloc(DEQUE_INITIAL_CAPACITY * sizeof *deque->data);
    if (deque->data == NULL) {
        free(deque);
        return NULL;
    }

    deque->capacity = DEQUE_INITIAL_CAPACITY;
    deque->size = 0;
    deque->front = 0;
    return deque;
}

void deque_destroy(Deque *deque)
{
    if (deque == NULL) {
        return;
    }
    free(deque->data);
    free(deque);
}

DsStatus deque_push_front(Deque *deque, Item x)
{
    if (deque == NULL) {
        return DS_BADARG;
    }

    if (deque->size == deque->capacity) {
        DsStatus status = deque_grow(deque);
        if (status != DS_OK) {
            return status;
        }
    }

    deque->front = (deque->front == 0) ? deque->capacity - 1
                                       : deque->front - 1;
    deque->data[deque->front] = x;
    deque->size++;
    return DS_OK;
}

DsStatus deque_push_back(Deque *deque, Item x)
{
    if (deque == NULL) {
        return DS_BADARG;
    }

    if (deque->size == deque->capacity) {
        DsStatus status = deque_grow(deque);
        if (status != DS_OK) {
            return status;
        }
    }

    deque->data[(deque->front + deque->size) % deque->capacity] = x;
    deque->size++;
    return DS_OK;
}

DsStatus deque_pop_front(Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->size == 0) {
        return DS_EMPTY;
    }

    *out = deque->data[deque->front];
    deque->front = (deque->front + 1) % deque->capacity;
    deque->size--;

    if (deque->size == 0) {
        deque->front = 0;
    }
    return DS_OK;
}

DsStatus deque_pop_back(Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->size == 0) {
        return DS_EMPTY;
    }

    size_t index = (deque->front + deque->size - 1) % deque->capacity;
    *out = deque->data[index];
    deque->size--;

    if (deque->size == 0) {
        deque->front = 0;
    }
    return DS_OK;
}

DsStatus deque_front(const Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->size == 0) {
        return DS_EMPTY;
    }

    *out = deque->data[deque->front];
    return DS_OK;
}

DsStatus deque_back(const Deque *deque, Item *out)
{
    if (deque == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (deque->size == 0) {
        return DS_EMPTY;
    }

    size_t index = (deque->front + deque->size - 1) % deque->capacity;
    *out = deque->data[index];
    return DS_OK;
}

bool deque_is_empty(const Deque *deque)
{
    return deque == NULL || deque->size == 0;
}

size_t deque_size(const Deque *deque)
{
    return deque == NULL ? 0 : deque->size;
}
