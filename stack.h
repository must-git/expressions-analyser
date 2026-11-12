#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stddef.h>
#include "ds_status.h"

/* TAD Pile (LIFO). Type opaque : la représentation est cachée dans le .c.
   Deux implémentations interchangeables : stack_array.c et stack_list.c. */
typedef struct Stack Stack;

Stack   *stack_create(void);                       /* NULL si échec mémoire     */
void     stack_destroy(Stack *s);                  /* accepte NULL              */
bool     stack_is_empty(const Stack *s);
size_t   stack_size(const Stack *s);
DsStatus stack_push(Stack *s, Item x);             /* DS_OK ou DS_NOMEM         */
DsStatus stack_pop(Stack *s, Item *out);           /* DS_EMPTY si pile vide     */
DsStatus stack_top(const Stack *s, Item *out);     /* DS_EMPTY si pile vide     */

#endif /* STACK_H */