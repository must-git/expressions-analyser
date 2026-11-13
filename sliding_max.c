#include "sliding_max.h"

#include <limits.h>

#include "deque.h"

/*
 * Algorithme en O(n) : la deque contient des indices dont les valeurs sont
 * strictement décroissantes. L'indice en tête est donc toujours le maximum
 * de la fenêtre courante.
 */
DsStatus sliding_max_compute(const Item *values, size_t length, size_t window,
                             Item *out)
{
    if (values == NULL || out == NULL || window == 0 || window > length) {
        return DS_BADARG;
    }
    if (length > (size_t)INT_MAX) {
        return DS_BADARG;
    }

    Deque *deque = deque_create();
    if (deque == NULL) {
        return DS_NOMEM;
    }

    for (size_t i = 0; i < length; i++) {
        while (!deque_is_empty(deque)) {
            Item front_index;
            DsStatus status = deque_front(deque, &front_index);
            if (status != DS_OK) {
                deque_destroy(deque);
                return status;
            }

            if (i >= window && (size_t)front_index <= i - window) {
                status = deque_pop_front(deque, &front_index);
                if (status != DS_OK) {
                    deque_destroy(deque);
                    return status;
                }
            } else {
                break;
            }
        }

        while (!deque_is_empty(deque)) {
            Item back_index;
            DsStatus status = deque_back(deque, &back_index);
            if (status != DS_OK) {
                deque_destroy(deque);
                return status;
            }

            if (values[back_index] <= values[i]) {
                status = deque_pop_back(deque, &back_index);
                if (status != DS_OK) {
                    deque_destroy(deque);
                    return status;
                }
            } else {
                break;
            }
        }

        {
            DsStatus status = deque_push_back(deque, (Item)i);
            if (status != DS_OK) {
                deque_destroy(deque);
                return status;
            }
        }

        if (i + 1 >= window) {
            Item front_index;
            DsStatus status = deque_front(deque, &front_index);
            if (status != DS_OK) {
                deque_destroy(deque);
                return status;
            }
            out[i + 1 - window] = values[front_index];
        }
    }

    deque_destroy(deque);
    return DS_OK;
}
