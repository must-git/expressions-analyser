#include "stack.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "ECHEC %s:%d : %s\n", __FILE__, __LINE__,          \
                    #condition);                                               \
            failures++;                                                        \
        }                                                                      \
    } while (0)

static void test_basic_operations(void)
{
    Stack *stack = stack_create();
    CHECK(stack != NULL);
    if (stack == NULL) {
        return;
    }

    Item value = 0;

    CHECK(stack_is_empty(stack));
    CHECK(stack_size(stack) == 0);
    CHECK(stack_pop(stack, &value) == DS_EMPTY);
    CHECK(stack_top(stack, &value) == DS_EMPTY);

    for (Item i = 1; i <= 5; i++) {
        CHECK(stack_push(stack, i) == DS_OK);
    }

    CHECK(!stack_is_empty(stack));
    CHECK(stack_size(stack) == 5);
    CHECK(stack_top(stack, &value) == DS_OK);
    CHECK(value == 5);

    for (Item i = 5; i >= 1; i--) {
        CHECK(stack_pop(stack, &value) == DS_OK);
        CHECK(value == i);
    }

    CHECK(stack_is_empty(stack));
    CHECK(stack_size(stack) == 0);
    CHECK(stack_pop(stack, &value) == DS_EMPTY);

    CHECK(stack_push(stack, 42) == DS_OK);
    CHECK(stack_size(stack) == 1);
    CHECK(stack_top(stack, &value) == DS_OK);
    CHECK(value == 42);
    CHECK(stack_pop(stack, &value) == DS_OK);
    CHECK(value == 42);
    CHECK(stack_is_empty(stack));

    stack_destroy(stack);
}

static void test_growth_and_lifo(void)
{
    const Item count = 250;

    Stack *stack = stack_create();
    CHECK(stack != NULL);
    if (stack == NULL) {
        return;
    }

    for (Item i = 0; i < count; i++) {
        CHECK(stack_push(stack, i) == DS_OK);
    }

    CHECK(stack_size(stack) == (size_t)count);

    Item value;
    CHECK(stack_top(stack, &value) == DS_OK);
    CHECK(value == count - 1);

    for (Item i = count - 1; i >= 0; i--) {
        CHECK(stack_pop(stack, &value) == DS_OK);
        CHECK(value == i);
    }

    CHECK(stack_is_empty(stack));
    CHECK(stack_size(stack) == 0);

    stack_destroy(stack);
}

static void test_null_arguments(void)
{
    Item value = 1;

    CHECK(stack_is_empty(NULL));
    CHECK(stack_size(NULL) == 0);
    CHECK(stack_push(NULL, value) == DS_BADARG);
    CHECK(stack_pop(NULL, &value) == DS_BADARG);
    CHECK(stack_top(NULL, &value) == DS_BADARG);
    stack_destroy(NULL);

    Stack *stack = stack_create();
    CHECK(stack != NULL);
    if (stack != NULL) {
        CHECK(stack_push(stack, value) == DS_OK);
        CHECK(stack_pop(stack, NULL) == DS_BADARG);
        CHECK(stack_top(stack, NULL) == DS_BADARG);
        stack_destroy(stack);
    }
}

int main(void)
{
    test_basic_operations();
    test_growth_and_lifo();
    test_null_arguments();

    if (failures == 0) {
        printf("test_stack : tous les tests sont passes\n");
        return 0;
    }

    fprintf(stderr, "test_stack : %d echec(s)\n", failures);
    return 1;
}
