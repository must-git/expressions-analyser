#include "stack.h"

#include <stdint.h>
#include <stdlib.h>

/*
 * Pile implémentée avec un tableau dynamique.
 *
 * Invariant : les éléments utiles occupent data[0 .. size-1] et le sommet est
 * data[size-1]. La capacité est agrandie par doublement quand le tableau est
 * plein.
 */
#define STACK_INITIAL_CAPACITY 8

struct Stack {
    Item *data;
    size_t size;
    size_t capacity;
};

Stack *stack_create(void)
{
    Stack *stack = malloc(sizeof *stack);
    if (stack == NULL) {
        return NULL;
    }

    stack->data = malloc(STACK_INITIAL_CAPACITY * sizeof *stack->data);
    if (stack->data == NULL) {
        free(stack);
        return NULL;
    }

    stack->size = 0;
    stack->capacity = STACK_INITIAL_CAPACITY;
    return stack;
}

void stack_destroy(Stack *stack)
{
    if (stack == NULL) {
        return;
    }
    free(stack->data);
    free(stack);
}

DsStatus stack_push(Stack *stack, Item x)
{
    if (stack == NULL) {
        return DS_BADARG;
    }

    if (stack->size == stack->capacity) {
        size_t new_capacity;

        if (stack->capacity > SIZE_MAX / 2) {
            return DS_NOMEM;
        }
        new_capacity = stack->capacity * 2;

        if (new_capacity > SIZE_MAX / sizeof *stack->data) {
            return DS_NOMEM;
        }

        Item *new_data =
            realloc(stack->data, new_capacity * sizeof *new_data);
        if (new_data == NULL) {
            return DS_NOMEM;
        }

        stack->data = new_data;
        stack->capacity = new_capacity;
    }

    stack->data[stack->size] = x;
    stack->size++;
    return DS_OK;
}

DsStatus stack_pop(Stack *stack, Item *out)
{
    if (stack == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (stack->size == 0) {
        return DS_EMPTY;
    }

    stack->size--;
    *out = stack->data[stack->size];
    return DS_OK;
}

DsStatus stack_top(const Stack *stack, Item *out)
{
    if (stack == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (stack->size == 0) {
        return DS_EMPTY;
    }

    *out = stack->data[stack->size - 1];
    return DS_OK;
}

bool stack_is_empty(const Stack *stack)
{
    return stack == NULL || stack->size == 0;
}

size_t stack_size(const Stack *stack)
{
    return stack == NULL ? 0 : stack->size;
}
