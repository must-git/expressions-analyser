#include "queue.h"

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

static void test_basic_fifo(void)
{
    Queue *queue = queue_create();
    CHECK(queue != NULL);
    if (queue == NULL) {
        return;
    }

    Item value = 0;

    CHECK(queue_is_empty(queue));
    CHECK(queue_size(queue) == 0);
    CHECK(queue_dequeue(queue, &value) == DS_EMPTY);
    CHECK(queue_front(queue, &value) == DS_EMPTY);

    for (Item i = 1; i <= 5; i++) {
        CHECK(queue_enqueue(queue, i) == DS_OK);
    }

    CHECK(!queue_is_empty(queue));
    CHECK(queue_size(queue) == 5);
    CHECK(queue_front(queue, &value) == DS_OK);
    CHECK(value == 1);

    for (Item i = 1; i <= 5; i++) {
        CHECK(queue_dequeue(queue, &value) == DS_OK);
        CHECK(value == i);
        CHECK(queue_size(queue) == (size_t)(5 - i));
    }

    CHECK(queue_is_empty(queue));
    CHECK(queue_dequeue(queue, &value) == DS_EMPTY);
    CHECK(queue_front(queue, &value) == DS_EMPTY);

    CHECK(queue_enqueue(queue, 99) == DS_OK);
    CHECK(queue_size(queue) == 1);
    CHECK(queue_front(queue, &value) == DS_OK);
    CHECK(value == 99);
    CHECK(queue_dequeue(queue, &value) == DS_OK);
    CHECK(value == 99);
    CHECK(queue_is_empty(queue));

    queue_destroy(queue);
}

static void test_circular_wrap(void)
{
    Queue *queue = queue_create();
    CHECK(queue != NULL);
    if (queue == NULL) {
        return;
    }

    Item value;

    for (Item i = 1; i <= 16; i++) {
        CHECK(queue_enqueue(queue, i) == DS_OK);
    }

    for (Item i = 1; i <= 10; i++) {
        CHECK(queue_dequeue(queue, &value) == DS_OK);
        CHECK(value == i);
    }

    for (Item i = 17; i <= 26; i++) {
        CHECK(queue_enqueue(queue, i) == DS_OK);
    }

    CHECK(queue_size(queue) == 16);
    CHECK(queue_front(queue, &value) == DS_OK);
    CHECK(value == 11);

    for (Item i = 11; i <= 26; i++) {
        CHECK(queue_dequeue(queue, &value) == DS_OK);
        CHECK(value == i);
    }

    CHECK(queue_is_empty(queue));
    queue_destroy(queue);
}

static void test_many_elements(void)
{
    const Item count = 300;

    Queue *queue = queue_create();
    CHECK(queue != NULL);
    if (queue == NULL) {
        return;
    }

    for (Item i = 0; i < count; i++) {
        CHECK(queue_enqueue(queue, i) == DS_OK);
    }

    CHECK(queue_size(queue) == (size_t)count);

    Item value;
    for (Item i = 0; i < count; i++) {
        CHECK(queue_dequeue(queue, &value) == DS_OK);
        CHECK(value == i);
    }

    CHECK(queue_is_empty(queue));
    queue_destroy(queue);
}

static void test_null_arguments(void)
{
    Item value = 1;

    CHECK(queue_is_empty(NULL));
    CHECK(queue_size(NULL) == 0);
    CHECK(queue_enqueue(NULL, value) == DS_BADARG);
    CHECK(queue_dequeue(NULL, &value) == DS_BADARG);
    CHECK(queue_front(NULL, &value) == DS_BADARG);
    queue_destroy(NULL);

    Queue *queue = queue_create();
    CHECK(queue != NULL);
    if (queue != NULL) {
        CHECK(queue_enqueue(queue, value) == DS_OK);
        CHECK(queue_dequeue(queue, NULL) == DS_BADARG);
        CHECK(queue_front(queue, NULL) == DS_BADARG);
        queue_destroy(queue);
    }
}

int main(void)
{
    test_basic_fifo();
    test_circular_wrap();
    test_many_elements();
    test_null_arguments();

    if (failures == 0) {
        printf("test_queue : tous les tests sont passes\n");
        return 0;
    }

    fprintf(stderr, "test_queue : %d echec(s)\n", failures);
    return 1;
}
