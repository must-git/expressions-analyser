#include "deque.h"

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

static void test_empty_and_single_element(void)
{
    Deque *deque = deque_create();
    CHECK(deque != NULL);
    if (deque == NULL) {
        return;
    }

    Item value = 0;

    CHECK(deque_is_empty(deque));
    CHECK(deque_size(deque) == 0);
    CHECK(deque_front(deque, &value) == DS_EMPTY);
    CHECK(deque_back(deque, &value) == DS_EMPTY);
    CHECK(deque_pop_front(deque, &value) == DS_EMPTY);
    CHECK(deque_pop_back(deque, &value) == DS_EMPTY);

    CHECK(deque_push_back(deque, 42) == DS_OK);
    CHECK(!deque_is_empty(deque));
    CHECK(deque_size(deque) == 1);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == 42);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 42);
    CHECK(deque_pop_back(deque, &value) == DS_OK);
    CHECK(value == 42);
    CHECK(deque_is_empty(deque));

    CHECK(deque_push_front(deque, 7) == DS_OK);
    CHECK(deque_size(deque) == 1);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == 7);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 7);
    CHECK(deque_pop_front(deque, &value) == DS_OK);
    CHECK(value == 7);
    CHECK(deque_is_empty(deque));

    deque_destroy(deque);
}

static void test_both_ends(void)
{
    Deque *deque = deque_create();
    CHECK(deque != NULL);
    if (deque == NULL) {
        return;
    }

    Item value = 0;

    CHECK(deque_push_back(deque, 1) == DS_OK);
    CHECK(deque_push_back(deque, 2) == DS_OK);
    CHECK(deque_push_back(deque, 3) == DS_OK);
    CHECK(deque_push_front(deque, 0) == DS_OK);
    CHECK(deque_push_front(deque, -1) == DS_OK);

    CHECK(deque_size(deque) == 5);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == -1);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 3);

    CHECK(deque_pop_front(deque, &value) == DS_OK);
    CHECK(value == -1);
    CHECK(deque_pop_back(deque, &value) == DS_OK);
    CHECK(value == 3);
    CHECK(deque_pop_front(deque, &value) == DS_OK);
    CHECK(value == 0);
    CHECK(deque_pop_back(deque, &value) == DS_OK);
    CHECK(value == 2);

    CHECK(deque_size(deque) == 1);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == 1);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 1);
    CHECK(deque_pop_front(deque, &value) == DS_OK);
    CHECK(value == 1);
    CHECK(deque_is_empty(deque));

    deque_destroy(deque);
}

static void test_circular_wrap_and_growth(void)
{
    Deque *deque = deque_create();
    CHECK(deque != NULL);
    if (deque == NULL) {
        return;
    }

    Item value = 0;

    for (Item i = 0; i < 8; i++) {
        CHECK(deque_push_back(deque, i) == DS_OK);
    }
    CHECK(deque_size(deque) == 8);

    for (Item i = 0; i < 3; i++) {
        CHECK(deque_pop_front(deque, &value) == DS_OK);
        CHECK(value == i);
    }

    for (Item i = 8; i < 11; i++) {
        CHECK(deque_push_back(deque, i) == DS_OK);
    }

    CHECK(deque_size(deque) == 8);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == 3);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 10);

    CHECK(deque_push_back(deque, 11) == DS_OK);
    CHECK(deque_size(deque) == 9);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 11);

    for (Item i = 3; i <= 11; i++) {
        CHECK(deque_pop_front(deque, &value) == DS_OK);
        CHECK(value == i);
    }
    CHECK(deque_is_empty(deque));

    for (Item i = 0; i < 8; i++) {
        CHECK(deque_push_front(deque, i) == DS_OK);
    }
    CHECK(deque_size(deque) == 8);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == 7);
    CHECK(deque_back(deque, &value) == DS_OK);
    CHECK(value == 0);

    CHECK(deque_push_front(deque, 8) == DS_OK);
    CHECK(deque_size(deque) == 9);
    CHECK(deque_front(deque, &value) == DS_OK);
    CHECK(value == 8);

    for (Item i = 8; i >= 0; i--) {
        CHECK(deque_pop_front(deque, &value) == DS_OK);
        CHECK(value == i);
    }
    CHECK(deque_is_empty(deque));

    deque_destroy(deque);
}

static void test_many_elements(void)
{
    const Item count = 300;

    Deque *deque = deque_create();
    CHECK(deque != NULL);
    if (deque == NULL) {
        return;
    }

    for (Item i = 0; i < count; i++) {
        CHECK(deque_push_back(deque, i) == DS_OK);
    }
    CHECK(deque_size(deque) == (size_t)count);

    Item value = 0;
    for (Item i = 0; i < count; i++) {
        CHECK(deque_pop_front(deque, &value) == DS_OK);
        CHECK(value == i);
    }

    CHECK(deque_is_empty(deque));
    deque_destroy(deque);
}

static void test_null_arguments(void)
{
    Item value = 1;

    CHECK(deque_is_empty(NULL));
    CHECK(deque_size(NULL) == 0);
    CHECK(deque_push_front(NULL, value) == DS_BADARG);
    CHECK(deque_push_back(NULL, value) == DS_BADARG);
    CHECK(deque_pop_front(NULL, &value) == DS_BADARG);
    CHECK(deque_pop_back(NULL, &value) == DS_BADARG);
    CHECK(deque_front(NULL, &value) == DS_BADARG);
    CHECK(deque_back(NULL, &value) == DS_BADARG);
    deque_destroy(NULL);

    Deque *deque = deque_create();
    CHECK(deque != NULL);
    if (deque != NULL) {
        CHECK(deque_push_back(deque, value) == DS_OK);
        CHECK(deque_pop_front(deque, NULL) == DS_BADARG);
        CHECK(deque_pop_back(deque, NULL) == DS_BADARG);
        CHECK(deque_front(deque, NULL) == DS_BADARG);
        CHECK(deque_back(deque, NULL) == DS_BADARG);
        deque_destroy(deque);
    }
}

int main(void)
{
    test_empty_and_single_element();
    test_both_ends();
    test_circular_wrap_and_growth();
    test_many_elements();
    test_null_arguments();

    if (failures == 0) {
        printf("test_deque : tous les tests sont passes\n");
        return 0;
    }

    fprintf(stderr, "test_deque : %d echec(s)\n", failures);
    return 1;
}
