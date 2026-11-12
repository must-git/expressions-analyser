#include "stack.h"

#include <stdlib.h>

/*
 * Pile implémentée avec une liste simplement chaînée.
 *
 * Chaque nœud contient une valeur et un pointeur vers le nœud précédent.
 * Le sommet de la pile est le premier nœud de la liste.
 */
typedef struct StackNode {
    Item value;
    struct StackNode *next;
} StackNode;

struct Stack {
    StackNode *top;
    size_t size;
};

Stack *stack_create(void)
{
    Stack *stack = malloc(sizeof *stack);
    if (stack == NULL) {
        return NULL;
    }

    stack->top = NULL;
    stack->size = 0;
    return stack;
}

void stack_destroy(Stack *stack)
{
    if (stack == NULL) {
        return;
    }

    StackNode *current = stack->top;
    while (current != NULL) {
        StackNode *next = current->next;
        free(current);
        current = next;
    }

    free(stack);
}

DsStatus stack_push(Stack *stack, Item x)
{
    if (stack == NULL) {
        return DS_BADARG;
    }

    StackNode *node = malloc(sizeof *node);
    if (node == NULL) {
        return DS_NOMEM;
    }

    node->value = x;
    node->next = stack->top;
    stack->top = node;
    stack->size++;
    return DS_OK;
}

DsStatus stack_pop(Stack *stack, Item *out)
{
    if (stack == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (stack->top == NULL) {
        return DS_EMPTY;
    }

    StackNode *node = stack->top;
    *out = node->value;
    stack->top = node->next;
    free(node);
    stack->size--;
    return DS_OK;
}

DsStatus stack_top(const Stack *stack, Item *out)
{
    if (stack == NULL || out == NULL) {
        return DS_BADARG;
    }
    if (stack->top == NULL) {
        return DS_EMPTY;
    }

    *out = stack->top->value;
    return DS_OK;
}

bool stack_is_empty(const Stack *stack)
{
    return stack == NULL || stack->top == NULL;
}

size_t stack_size(const Stack *stack)
{
    return stack == NULL ? 0 : stack->size;
}
