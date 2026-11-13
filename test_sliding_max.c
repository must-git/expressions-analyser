#include "sliding_max.h"

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

#define MAX_VALUES 128

static void expect_case(const Item *values, size_t length, size_t window,
                        const Item *expected)
{
    Item out[MAX_VALUES];
    DsStatus status = sliding_max_compute(values, length, window, out);

    CHECK(status == DS_OK);
    if (status != DS_OK) {
        return;
    }

    size_t result_count = length - window + 1;
    CHECK(result_count <= MAX_VALUES);
    for (size_t i = 0; i < result_count && i < MAX_VALUES; i++) {
        CHECK(out[i] == expected[i]);
    }
}

static void test_basic_cases(void)
{
    static const Item values[] = {1, 3, -1, -3, 5, 3, 6, 7};
    static const Item expected_window_1[] = {1, 3, -1, -3, 5, 3, 6, 7};
    static const Item expected_window_3[] = {3, 3, 5, 5, 6, 7};
    static const Item expected_window_8[] = {7};

    expect_case(values, 8, 1, expected_window_1);
    expect_case(values, 8, 3, expected_window_3);
    expect_case(values, 8, 8, expected_window_8);
}

static void test_repeated_values(void)
{
    static const Item values[] = {5, 5, 5, 5, 5};
    static const Item expected[] = {5, 5, 5, 5};

    expect_case(values, 5, 2, expected);
}

static void test_sorted_sequences(void)
{
    static const Item increasing[] = {1, 2, 3, 4, 5};
    static const Item decreasing[] = {5, 4, 3, 2, 1};
    static const Item expected_increasing[] = {3, 4, 5};
    static const Item expected_decreasing[] = {5, 4, 3};

    expect_case(increasing, 5, 3, expected_increasing);
    expect_case(decreasing, 5, 3, expected_decreasing);
}

static void test_invalid_arguments(void)
{
    Item value = 1;
    Item out[2];

    CHECK(sliding_max_compute(NULL, 1, 1, out) == DS_BADARG);
    CHECK(sliding_max_compute(&value, 1, 1, NULL) == DS_BADARG);
    CHECK(sliding_max_compute(&value, 1, 0, out) == DS_BADARG);
    CHECK(sliding_max_compute(&value, 1, 2, out) == DS_BADARG);
    CHECK(sliding_max_compute(&value, 0, 1, out) == DS_BADARG);
}

static void test_against_brute_force(void)
{
    Item values[100];
    Item out[100];
    Item expected[100];
    unsigned state = 123456789u;

    for (size_t i = 0; i < 100; i++) {
        state = state * 1103515245u + 12345u;
        values[i] = (Item)((state >> 8) % 201u) - 100;
    }

    for (size_t window = 1; window <= 100; window++) {
        DsStatus status = sliding_max_compute(values, 100, window, out);
        CHECK(status == DS_OK);
        if (status != DS_OK) {
            return;
        }

        size_t result_count = 100 - window + 1;
        for (size_t start = 0; start < result_count; start++) {
            Item maximum = values[start];
            for (size_t offset = 1; offset < window; offset++) {
                if (values[start + offset] > maximum) {
                    maximum = values[start + offset];
                }
            }
            expected[start] = maximum;
        }

        for (size_t i = 0; i < result_count; i++) {
            CHECK(out[i] == expected[i]);
        }
    }
}

int main(void)
{
    test_basic_cases();
    test_repeated_values();
    test_sorted_sequences();
    test_invalid_arguments();
    test_against_brute_force();

    if (failures == 0) {
        printf("test_sliding_max : tous les tests sont passes\n");
        return 0;
    }

    fprintf(stderr, "test_sliding_max : %d echec(s)\n", failures);
    return 1;
}
